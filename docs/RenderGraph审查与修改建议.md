# RenderGraph 代码审查与修改建议

本文档是对当前 RenderGraph 系统的代码实现、两份发展路线文档（`RenderGraph发展路线.md` 和 `RenderGraph后续开发接手路线.md`）的全面审查，评估方案是否长期稳定可靠，并提出修改建议。

审查日期：2026-05-30

## 一、总体判断

当前 RenderGraph 的实现质量和路线文档方向都相当高。渐进式接入的设计哲学（先接管执行顺序，再引入资源描述，最后接管 barrier 和生命周期）完全正确，遵循了行业最佳实践（Granite/Frostbite/UE5 的 RenderGraph 模式）。代码层面的 Compile/Execute 分离、Validate 检查、Dump 调试、Allocator 的 NeedsRecreate 机制都实现得很扎实。这套路线可以直接作为后续开发的依据。

但存在几个如果现在不处理、将来会成为架构债务的问题，以及路线文档中缺失的关键阶段。

---

## 二、代码层面的具体问题

### 2.1 AccessType 枚举缺少关键类型（高优先级）

当前定义：

```cpp
enum class RenderGraphAccessType
{
    Unknown = 0,
    SampledTexture,           // shader 采样读取
    ColorAttachment,          // color attachment 写入
    DepthStencilAttachment,   // depth/stencil attachment 写入
    TransferSrc,              // 拷贝源读取
    TransferDst,              // 拷贝目标写入
    Present                   // swapchain present
};
```

缺少以下关键类型：

**StorageImage（UAV / 读写纹理）**。后续引入 compute-based pass（GPU culling、mipmap generation、Hi-Z 构建、SSAO/SSR compute 版本）时，必须通过 storage image 来表达读写。对应 `VK_IMAGE_USAGE_STORAGE_BIT` 和 `VK_IMAGE_LAYOUT_GENERAL`。

**DepthStencilRead（深度只读）**。当前只有 DepthStencilAttachment（写入类）访问。EditorGrid 等 pass 需要基于深度测试但不写入深度，用 DepthStencilAttachment 语义不准确，会导致后续 barrier 生成时插入不必要的写屏障和 storeOp 判断错误。对应 `VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL` 或 `VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL`。

**修改建议**：

```cpp
enum class RenderGraphAccessType
{
    Unknown = 0,
    SampledTexture,
    ColorAttachment,
    DepthStencilAttachment,
    DepthStencilRead,         // 新增：深度只读（基于深度的测试，不写深度）
    StorageImage,             // 新增：compute shader 读写纹理
    TransferSrc,
    TransferDst,
    Present
};
```

同步更新 `GetImageUsageForAccess()`、`GetImageLayoutForAccess()`、`IsWriteAccess()` 三个辅助函数。

### 2.2 CompileResult 不记录 pass 间依赖边（中优先级）

当前 `Compile()` 计算了每个资源的 ResourceLifetime（FirstWritePass、LastReadPass），但没有构建显式的 pass 依赖图。即：

```
Pass A writes resource R, Pass B reads resource R → A 必须在 B 之前
```

这一步对后续拓扑排序、barrier 自动生成、并行执行分析都是必需数据。如果不提前准备，后续做自动排序时会发现缺少基础信息。

**修改建议**：在 `RenderGraphCompileResult` 中新增字段：

```cpp
// 每个 pass 的直接前驱 pass 列表。
// PassDependencies[i] = {j, k, ...} 表示 pass i 依赖于 pass j, k 的输出。
std::vector<std::vector<uint32_t>> PassDependencies;
```

在 `Compile()` 中收集写入资源时，对每个 write 检查哪些后续 pass 读取了该资源，建立边。这个改动不影响现有功能，数据可以直接用于后续阶段的拓扑排序。

### 2.3 Allocator 缺少多帧飞行安全（高优先级）

当前 `RenderGraphAllocator::m_TransientImages` 跨帧持久，`NeedsRecreate()` 在 detect 到需要重建时直接替换 `record.Image`。如果 Vulkan 使用 2-3 个 in-flight frame：

```
Frame N:    创建 transient image A，记录命令使用 A
Frame N+1:  判定 A 需要重建，替换为 A'，但 Frame N 的 GPU 命令可能仍在执行中使用 A
```

旧 image 被销毁时 GPU 仍在使用它，触发 validation error 甚至崩溃。

**修改建议**：给 Allocator 传入 `uint32_t frameIndex`，每个 Image 维护一个 retire fence 或使用环形缓冲区（per-frame pool）。最简单的方案：维护 `m_PendingImages` 队列，每帧将替换下的旧 Image 推入队列，在 GPU 完成该帧后（通过 fence 确认）再释放。

### 2.4 Compile 每帧重算（高优先级）

当前 `Execute()` 每次都无条件调用 `Compile()`。对于 pass 结构不变化的帧（绝大多数帧），重新计算 lifetimes、access summaries 和依赖边是浪费的 CPU 时间。随着 pass 数量增长到 20+，这个开销会变得可感知。

**修改建议**：引入 dirty 标记机制：

```cpp
bool m_CompileDirty = true;

// Reset()、ImportRenderTarget()、AddPass()、CreateTexture() 中设置 dirty
void RenderGraph::Reset() {
    m_Resources.clear();
    m_Passes.clear();
    m_CompileResult = {};
    m_CompileDirty = true;
}

// Compile() 检查 dirty
const RenderGraphCompileResult& RenderGraph::Compile() {
    if (!m_CompileDirty)
        return m_CompileResult;
    // ... 原有编译逻辑 ...
    m_CompileDirty = false;
    return m_CompileResult;
}
```

EditorRenderer 的 `BuildRenderGraph()` 每帧调用 `Reset()` 和 `ImportRenderTarget()`/`AddPass()`，这些操作会自动设置 dirty，所以行为不会改变。但对于未来可能出现的"只改了资源参数、pass 结构不变"的帧，就可以跳过重新编译。

### 2.5 RenderGraphContext 的 imported/transient 接口不统一（中优先级）

当前 `RenderGraphContext` 提供了两个独立方法：

```cpp
VulkanRenderTarget& GetRenderTarget(RenderGraphResourceID id) const;   // imported
VulkanImage& GetTransientImage(RenderGraphResourceID id) const;        // transient
```

Pass callback 内部需要判断资源类型来调用不同方法，这破坏了"RenderGraph 应该向 pass 隐藏资源来源"的设计目标。在阶段 3-4 迁移 Lighting/GBuffer 到 transient 时，pass 代码需要跟着改动，增加了迁移风险。

**修改建议**：在引入 `VulkanRenderTargetView`（接手路线阶段 1）的同时，新增统一接口：

```cpp
struct RenderGraphAttachmentView {
    VulkanImage* Image = nullptr;
    VkImageView ImageView = VK_NULL_HANDLE;
    uint32_t Width = 0;
    uint32_t Height = 0;
    VkFormat Format = VK_FORMAT_UNDEFINED;
    VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkClearValue ClearValue{};
};

// RenderGraphContext 新增
RenderGraphAttachmentView GetAttachmentView(RenderGraphResourceID id) const;
```

该方法内部判断 resource type，对 Imported 从 `VulkanRenderTarget` 提取信息，对 Transient 从 `Allocator` 提取信息，对外暴露统一接口。这样 pass 代码不感知底层是 imported 还是 transient。

---

## 三、路线文档需要补充的关键阶段

两份路线文档（长期路线 8 个 Phase，接手路线 9 个 Stage）总体合理，但缺少以下关键阶段：

### 3.1 缺少：Transient 资源内存别名 Aliasing（高优先级）

当前 Allocator 为每个 transient 资源创建独立的 VkImage，不做复用。对于一个典型的延迟渲染管线：

```
GBuffer.BaseColor (1920x1080 RGBA16F)
GBuffer.Normal    (1920x1080 RGBA16F)
GBuffer.Material  (1920x1080 RGBA8)
GBuffer.Emissive  (1920x1080 RGBA16F)
GBuffer.Depth     (1920x1080 D32)
Lighting.Color    (1920x1080 RGBA16F)
```

仅 GBuffer + Lighting 就消耗约 75MB VRAM（不考虑对齐）。随着后处理链（Bloom 降采样链、SSAO/SSR 中间结果、TAA history buffer、shadow map）的引入，VRAM 消耗会迅速膨胀。

这是 RenderGraph 模式的核心价值之一——利用 ResourceLifetime 分析，将生命周期不重叠的资源分配到同一块物理内存。

**修改建议**：在接手路线的阶段 4（GBuffer 迁移）之后、阶段 5（barrier）之前，插入一个 **Aliasing** 阶段。

实现可以用简单的首次适配（first-fit）分配器：

```
1. 遍历所有 transient 资源，按 (FirstWritePass, LastReadPass) 排序
2. 为每个资源分配内存块
3. 使用空闲列表（free list），在 LastReadPass 之后回收
4. 新资源优先复用已回收的内存块
5. VkImage 创建时使用 VK_IMAGE_CREATE_ALIAS_BIT
```

参考实现：Granite 的 `MemoryAllocator`（first-fit with free list），UE5 的 `FRDGBuilder`（更复杂的 tile-based 方案）。

### 3.2 缺少：GPU 性能分析集成（中优先级）

当前 Dump 只输出 pass 顺序和资源信息，没有任何 GPU timing 数据。实际引擎开发中，每帧每个 pass 的 GPU 耗时是最核心的调试数据。

**修改建议**：在阶段 5（barrier）或 6（executor/feature）之后增加一个阶段：

```
1. 初始化 VkQueryPool (timestamp queries)
2. Compile 阶段为每个 pass 分配 query index
3. Execute 时在每个 pass 前后插入 vkCmdWriteTimestamp
4. 帧结束后 vkGetQueryPoolResults 回读
5. Dump 输出 Pass GPU Time (ms)
6. ImGui 面板可视化展示各 pass 时间占比
```

### 3.3 阶段 8 过度膨胀（中优先级）

长期路线 `RenderGraph发展路线.md` 的 Phase 8 一口气塞进了：

- 拓扑排序
- pass culling
- async compute
- multi-queue
- GPU profiling
- editor graph view
- shadow / SSAO / SSR / Bloom / TAA 接入

这些应该拆成至少三个独立阶段，否则难以判断进度。

**修改建议**——将 Phase 8 拆分为：

| 新阶段 | 内容 | 依赖 |
|--------|------|------|
| Phase 8 | 拓扑排序 + pass culling（基于依赖图的自动排序和死代码消除） | Phase 2 Compile 依赖边 |
| Phase 9 | GPU profiling + editor graph view（可视化调试） | Phase 8 |
| Phase 10 | async compute + multi-queue（并行执行） | Phase 8, Phase 7 barrier |
| Phase 11+ | shadow / SSAO / SSR / Bloom / TAA（后处理特效接入，每个独立） | Phase 7+ |

### 3.4 其他遗漏

**缺少编译结果缓存（Compile Caching）**。见上文 2.4 节。应在 long-term roadmap 中明确提及。

**缺少序列化能力**。对于 frame debugger、帧回放、自动化测试，能将一帧的 RenderGraph 结构序列化到文件是重要能力。应在 CompileResult 或 RenderGraph 中预留 `Serialize()` / `Deserialize()` 接口。

**缺少 subresource 级访问跟踪**。当前整个 image 被视为原子资源。但 mipmap 生成链和 depth-stencil 分离访问需要 subresource 粒度。短期不需要实现，但在 `RenderGraphResourceAccess` 中预留 `VkImageSubresourceRange` 字段以保持向前兼容。

---

## 四、架构层面的长期考量

### 4.1 RenderGraph 与 EditorRenderer 的耦合

当前 `m_RenderGraph` 是 EditorRenderer 的成员变量，RenderGraph 类本身在 engine 层但只有 editor 使用。随着引擎从编辑器原型走向游戏运行时，RenderGraph 应被 GameRenderer 或 SceneRenderer 也能使用。这不是紧急问题，但接口设计上应保持 RenderGraph 独立于任何特定 renderer。

### 4.2 Lambda-only 执行模型

当前所有 pass 通过 `SetExecute(lambda)` 注册回调。Lambda 无法序列化、无法自省、难以做 GPU 调试回溯。接手路线阶段 6 的 executor/feature 模式是正确的解耦方向，但应考虑把 executor 接口设计得足够通用，使 lambda 成为 executor 的一个子集。

```cpp
class IRenderGraphPassExecutor {
public:
    virtual ~IRenderGraphPassExecutor() = default;
    virtual void Execute(RenderGraphContext& context) = 0;
    virtual const char* GetDebugName() const { return "Unknown"; }
    // 后续可扩展：virtual void GatherPipelineRequests(...) = 0;
    // 后续可扩展：virtual void GatherDescriptorWrites(...) = 0;
};
```

---

## 五、修改优先级汇总

### 高优先级（建议在下一个开发周期处理）

| # | 问题 | 影响 | 改动范围 |
|---|------|------|----------|
| 1 | AccessType 补充 StorageImage 和 DepthStencilRead | 不改则后续 compute pass 无法正确表达访问语义 | RenderGraphResource.h, .cpp |
| 2 | Compile 引入 dirty 标记，避免每帧重算 | CPU 性能浪费，pass 增多后更明显 | RenderGraph.h, .cpp |
| 3 | Allocator 考虑多帧飞行安全 | 不改则 in-flight frame 场景下可能 crash | RenderGraphAllocator.h, .cpp |

### 中优先级（开发到对应阶段前处理）

| # | 问题 | 影响 |
|---|------|------|
| 4 | 在 Allocator 之后插入 Aliasing 阶段 | 不改则 VRAM 随 pass 数线性增长 |
| 5 | 引入统一的 RenderGraphAttachmentView | 降低 Lighting/GBuffer 迁移到 transient 时的 pass 改动量 |
| 6 | 拆分 Phase 8 为多个独立阶段 | 提高路线文档的可操作性 |
| 7 | CompileResult 中加入 PassDependencies | 为拓扑排序和 barrier 生成准备基础数据 |

### 低优先级（预留接口即可）

| # | 问题 | 影响 |
|---|------|------|
| 8 | RenderGraphResourceAccess 预留 SubresourceRange 字段 | 为 mipmap chain 和 depth-stencil 分离访问预留扩展点 |
| 9 | CompileResult 预留 Serialize/Deserialize 接口 | 为 frame debugger 和帧回放预留扩展点 |
| 10 | GPU profiling 和 editor graph view 作为独立阶段加入路线 | 补齐调试工具链 |

---

## 六、验证说明

上述所有修改建议均向后兼容，不会影响当前已完成的 Phases 1-4 功能（Graph Shell、Compile、Dump、Transient Texture Desc、Allocator）。每个修改都可以独立提交和验证，验证标准与现有路线文档一致：

- `Release|x64` 编译通过
- 编辑器视口正常显示
- Vulkan validation 不出现新的 layout/barrier 报警
- Grid / Skybox / Picking 行为不变

---

## 七、相关文件清单

审查涉及的文件：

```
engine/src/render/graph/RenderGraph.h
engine/src/render/graph/RenderGraph.cpp
engine/src/render/graph/RenderGraphResource.h
engine/src/render/graph/RenderGraphCompiled.h
engine/src/render/graph/RenderGraphAllocator.h
engine/src/render/graph/RenderGraphAllocator.cpp
engine/src/render/graph/RenderGraphContext.h
engine/src/render/graph/RenderGraphContext.cpp
renderer/src/scene/EditorRenderer.h
renderer/src/scene/EditorRenderer.cpp
docs/RenderGraph发展路线.md
docs/RenderGraph后续开发接手路线.md
VULKAN_RENDERING_FRAMEWORK.md
```
