# RenderGraph 发展路线

本文档记录 Kita 渲染管线中 RenderGraph 的长期设计路线、当前阶段状态、下一步开发任务和后续会话接手要点。

## 1. 背景与目标

当前编辑器视口渲染流程主要由 `renderer/src/scene/EditorRenderer.cpp` 手写顺序编排，典型流程为：

```text
GBuffer
  -> DeferredLighting
  -> Skybox
  -> CopyDepth
  -> ToneMap
  -> EditorGrid
  -> Picking
```

现有 Pass 体系位于 `engine/src/render/pass/` 与相关目录中，例如：

- `BasePass`
- `DeferredLightingPass`
- `ToneMappingPass`
- `SkyboxPass`
- `EditorGridPass`
- `ViewportPickingPass`

这些 pass 负责具体渲染逻辑。RenderGraph 的长期目标不是立即替代这些 pass，而是替代 `EditorRenderer` 中手写的执行顺序、资源依赖、layout/barrier 管理和中间资源生命周期管理。

最终理想效果：

```text
EditorRenderer / EditorViewport
  只描述本帧需要什么渲染结果和编辑器交互数据

RenderGraph
  负责 pass 依赖分析
  负责资源生命周期
  负责 transient texture 创建与复用
  负责 Vulkan layout/barrier
  负责执行顺序、裁剪和调试输出

具体 RenderPass
  只负责本 pass 内部如何绘制
```

## 2. 核心设计原则

### 2.1 RenderGraphPass 与 RenderPass 的关系

`RenderGraphPass` 是调度节点，描述：

- pass 名称
- 读取哪些资源
- 写入哪些资源
- 执行回调

现有 `IRenderPass / RenderPassBase / SceneRenderPassBase` 是具体渲染实现，负责：

- Begin/End rendering
- 绑定 pipeline
- 绑定 descriptor
- draw call

短期关系：

```cpp
RenderGraphPass
    -> Execute callback
        -> 创建 RenderPassContext
        -> 调用 BasePass / DeferredLightingPass / ToneMappingPass 等现有 pass
```

长期关系：

```text
RenderGraphPass 替代 EditorRenderer 中的手写顺序
RenderGraph 接管资源依赖和 barrier
现有 RenderPass 保留为可被 graph 调度的执行单元
```

### 2.2 不要一次性推翻现有系统

RenderGraph 应渐进式接入：

1. 先接管执行顺序。
2. 再增加 Compile 和依赖分析。
3. 再引入逻辑资源描述。
4. 再引入 transient texture。
5. 最后接管 Vulkan 资源创建、layout 和 barrier。

这样可以保持现有渲染结果稳定，避免一次性修改过大。

## 3. 当前阶段

当前已进入 RenderGraph Shell + Compile 初期阶段。

已引入或计划中的核心文件：

```text
engine/src/render/graph/RenderGraphResource.h
engine/src/render/graph/RenderGraphContext.h
engine/src/render/graph/RenderGraphContext.cpp
engine/src/render/graph/RenderGraphCompiled.h
engine/src/render/graph/RenderGraph.h
engine/src/render/graph/RenderGraph.cpp
```

当前 RenderGraph 的职责：

- 导入已有 `VulkanRenderTarget`
- 添加 pass
- 记录 pass 的 read/write 声明
- Compile 生成稳定执行列表
- Execute 按 compiled pass list 执行

当前仍然由外部创建的内容：

- `EditorViewportSurface` 仍创建 GBuffer、Lighting、Final、Picking RenderTarget
- `EditorRenderer` 仍创建具体 pass 对象
- 现有 `VulkanRenderTarget::BeginRendering / EndRendering` 仍负责大部分 layout transition

这是正常状态。此阶段目标只是让 graph 成为“帧渲染描述与执行入口”。

## 4. 当前必须确认的实现细节

后续会话接手时，先检查这些点：

### 4.1 Execute 必须使用 Compile 结果

`RenderGraph::Execute()` 应该类似：

```cpp
void RenderGraph::Execute(VulkanContext& context, VkCommandBuffer commandBuffer)
{
	const RenderGraphCompileResult& compileResult = Compile();
	if (!compileResult.Valid)
		return;

	RenderGraphContext graphContext(context, commandBuffer, m_Resources);

	for (const RenderGraphCompiledPass& compiledPass : compileResult.Passes)
	{
		KITA_CORE_ASSERT(
			compiledPass.PassIndex < m_Passes.size(),
			"RenderGraph compiled pass index is invalid");

		m_Passes[compiledPass.PassIndex].Execute(graphContext);
	}
}
```

不要绕过 `Compile()` 直接遍历 `m_Passes`。

### 4.2 Reset / Import / AddPass 需要清空编译结果

修改 graph 内容后，旧的 compile result 不再可靠。

```cpp
void RenderGraph::Reset()
{
	m_Resources.clear();
	m_Passes.clear();
	m_CompileResult = {};
}
```

`ImportRenderTarget()` 与 `AddPass()` 中也应设置：

```cpp
m_CompileResult = {};
```

### 4.3 RenderGraphResource 字段名需要统一

建议 `RenderGraphResource` 使用：

```cpp
struct RenderGraphResource
{
	std::string Name;
	RenderGraphResourceType Type = RenderGraphResourceType::Unknown;
	VulkanRenderTarget* RT = nullptr;
	bool Imported = false;
};
```

`RenderGraphContext::GetRenderTarget()` 中应检查 `resource.RT`，不要出现 `ImportedRenderTarget` 与 `RT` 字段混用。

### 4.4 CopyDepth 必须在 graph 内执行

`CopyDepthAttachment` 的作用是把 Lighting RT 的 depth 复制到 Final RT 的 depth，使后续 EditorGrid/Gizmo/Overlay 能基于 final depth 正确做深度测试。

正确顺序：

```text
Lighting / Skybox 写完 lighting depth
  -> CopyDepth
  -> ToneMap 写 final color
  -> EditorGrid 使用 final depth
```

如果 `CopyDepthAttachment` 在 graph 执行前调用，可能会读取 layout 仍为 `VK_IMAGE_LAYOUT_UNDEFINED` 的 depth image，并触发 Vulkan validation warning。

### 4.5 Graph pass callback 尽量值捕获资源 ID

短期每帧 `Reset + Build + Execute` 时 `[&]` 通常能工作，但长期可能引入悬空引用风险。

推荐：

```cpp
.SetExecute([this, lightingID](RenderGraphContext& graphContext)
{
	...
});
```

## 5. 阶段路线

### 阶段 1：Graph Shell

目标：RenderGraph 成为执行顺序容器。

完成标准：

- `ImportRenderTarget()`
- `AddPass()`
- `Read() / Write()`
- `SetExecute()`
- `Execute()`
- `EditorRenderer::Render()` 中真实 pass 通过 graph 执行

这一阶段不做：

- 自动排序
- 自动 barrier
- transient texture
- RT 生命周期接管

### 阶段 2：Compile 与依赖分析

目标：声明阶段和执行阶段分离。

核心结构：

```cpp
struct RenderGraphCompiledPass
{
	uint32_t PassIndex;
	std::vector<RenderGraphResourceID> Reads;
	std::vector<RenderGraphResourceID> Writes;
};

struct RenderGraphCompileResult
{
	bool Valid;
	std::vector<RenderGraphCompiledPass> Passes;
};
```

完成标准：

- `RenderGraph::Compile()`
- `RenderGraph::Validate()`
- `RenderGraph::Execute()` 使用 compile result
- 检查 resource id 是否有效
- 检查 pass 是否有 execute callback
- 检查 pass name/resource name 是否为空

下一步可扩展：

- 检查读取未写入的 transient resource
- dump pass 顺序和资源读写
- 为拓扑排序准备数据

### 阶段 3：Graph Dump 与调试视图基础

目标：RenderGraph 能输出自身结构，方便排查。

建议接口：

```cpp
void RenderGraph::Dump() const;
void RenderGraph::SetDebugDumpEnabled(bool enabled);
```

Dump 内容：

```text
RenderGraph dump: N resources, M passes
  Pass[0] GBuffer
    Write [0] GBuffer
  Pass[1] Lighting
    Read  [0] GBuffer
    Write [1] Lighting
```

注意：不要默认每帧打印，避免日志刷屏。建议通过 debug flag 控制。

### 阶段 4：Transient Texture 描述

目标：RenderGraph 开始描述中间纹理资源，但暂时不真正创建 Vulkan image。

含义：

```text
ImportRenderTarget:
  外部创建、外部持有，例如 Final / Picking

CreateTexture:
  graph 内声明的临时纹理，例如 GBuffer.BaseColor / Lighting.Color
```

第一步只记录描述：

```cpp
struct RenderGraphTextureDesc
{
	std::string Name;

	uint32_t Width = 1;
	uint32_t Height = 1;
	VkFormat Format = VK_FORMAT_UNDEFINED;
	VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;

	VkImageUsageFlags Usage = 0;
	bool CreateSampler = false;
};
```

资源类型扩展：

```cpp
enum class RenderGraphResourceType
{
	Unknown = 0,
	ImportedRenderTarget,
	TransientTexture
};
```

`RenderGraphResource` 扩展：

```cpp
struct RenderGraphResource
{
	std::string Name;
	RenderGraphResourceType Type = RenderGraphResourceType::Unknown;

	bool Imported = false;

	VulkanRenderTarget* RT = nullptr;
	RenderGraphTextureDesc TextureDesc{};
};
```

新增接口：

```cpp
RenderGraphResourceID CreateTexture(const RenderGraphTextureDesc& desc);
const RenderGraphResource& GetResource(RenderGraphResourceID id) const;
```

这一阶段仍然不要求现有 pass 使用 transient texture。先让 graph 能表达“这里有一个逻辑临时纹理”。

### 阶段 5：Transient Resource 生命周期分析

目标：Compile 阶段知道每个资源首次写入、最后读取、生命周期范围。

建议新增：

```cpp
struct RenderGraphResourceLifetime
{
	uint32_t FirstPass = InvalidRenderGraphPassIndex;
	uint32_t LastPass = InvalidRenderGraphPassIndex;
};
```

Compile 后可以知道：

```text
GBuffer.BaseColor:
  first write: GBuffer
  last read: DeferredLighting

Lighting.Color:
  first write: Lighting
  last read: ToneMap
```

这一步是后续资源复用和 pass culling 的基础。

### 阶段 6：RenderGraphAllocator

目标：真正为 transient texture 创建 Vulkan 资源。

建议新增：

```text
engine/src/render/graph/RenderGraphAllocator.h
engine/src/render/graph/RenderGraphAllocator.cpp
```

职责：

- 根据 `RenderGraphTextureDesc` 创建实际 `VulkanImage` 或轻量 attachment wrapper
- 按 viewport size resize
- 按生命周期释放或复用
- 后续支持 aliasing

这一阶段开始逐步把 GBuffer/Lighting 从 `EditorViewportSurface` 迁移到 RenderGraph。

### 阶段 7：RenderGraph 接管 layout / barrier

目标：不再依赖每个 `VulkanRenderTarget::BeginRendering / EndRendering` 自己做所有 transition。

长期方向：

- Compile 根据资源读写生成 layout plan
- Execute 在 pass 前后插入 barrier
- RenderPass 只声明使用方式，不直接猜测最终 layout

示例：

```text
GBuffer.BaseColor:
  Undefined -> ColorAttachment
  ColorAttachment -> ShaderReadOnly

Lighting.Color:
  Undefined -> ColorAttachment
  ColorAttachment -> ShaderReadOnly

Final.Color:
  Undefined/ShaderReadOnly -> ColorAttachment
  ColorAttachment -> ShaderReadOnly
```

这一步属于重大渲染底层变化，修改前需要确认设计。

### 阶段 8：自动排序、裁剪与多队列

长期增强：

- 根据 read/write 做拓扑排序
- 未被最终输出依赖的 pass 自动裁剪
- async compute
- GPU profiling
- editor graph view
- shadow / SSAO / SSR / Bloom / TAA 接入

## 6. 下一步具体任务

如果当前 Compile 阶段已完成，下一步建议做：

### 任务 A：确保 Compile 真的被 Execute 使用

检查：

- `Execute()` 是否调用 `Compile()`
- `Reset()` 是否清空 `m_CompileResult`
- `ImportRenderTarget()` 和 `AddPass()` 是否清空 `m_CompileResult`

### 任务 B：添加 Graph Dump

给 `RenderGraph` 添加：

```cpp
void Dump() const;
void SetDebugDumpEnabled(bool enabled);
bool IsDebugDumpEnabled() const;
```

建议 `Dump()` 输出 pass 顺序和资源读写。

### 任务 C：添加 Transient Texture 描述

修改：

```text
engine/src/render/graph/RenderGraphResource.h
engine/src/render/graph/RenderGraph.h
engine/src/render/graph/RenderGraph.cpp
```

新增：

```cpp
RenderGraphTextureDesc
RenderGraphResourceType::TransientTexture
RenderGraph::CreateTexture()
RenderGraph::GetResource()
```

第一版只记录描述，不创建 Vulkan 资源。

### 任务 D：在 Compile 中分析资源生命周期

为后续 allocator 准备：

- first write pass
- last read pass
- imported resource 不参与自动释放
- transient resource 必须至少被写入一次

## 7. 当前 EditorRenderer 迁移方向

短期 `EditorRenderer` 仍负责：

- 创建 pass 对象
- 准备 scene data
- 收集 draw item
- 获取 pipeline
- 同步材质 descriptor

RenderGraph 负责：

- 声明 pass 顺序
- 声明资源 read/write
- 执行 pass callback

建议把 `EditorRenderer::Render()` 继续拆分：

```cpp
void PrepareScenePassData(...);
void CollectDrawItems(...);
void BuildRenderGraph(...);
```

目标结构：

```cpp
void EditorRenderer::Render(EditorViewportSurface& surface)
{
	if (!IsReady())
		return;

	VkCommandBuffer cmd = m_Context->GetCurrentCommandBuffer();
	if (cmd == VK_NULL_HANDLE)
		return;

	PrepareScenePassData(surface);
	CollectDrawItems(surface);
	BuildRenderGraph(surface);

	m_RenderGraph->Execute(*m_Context, cmd);
}
```

## 8. 验证要求

每完成一个阶段，至少验证：

- `Release|x64` 能成功编译
- 编辑器视口能正常显示
- Vulkan validation 不出现新的 layout/barrier 报警
- Grid 开关行为与迁移前一致
- Skybox 材质为空或无效时不会崩溃
- Picking 仍能正常工作

项目默认推荐使用：

```text
Kita.slnx
Release|x64
```

## 9. 注意事项

- 不要随意修改 `AGENTS.md`，除非开发者确认。
- RenderGraph 接管 Vulkan barrier/layout 属于重大底层变化，实施前应先确认设计。
- 不要直接把 `.vcxproj` 当长期维护入口，构建规则应回写 Premake。
- 新增重要类和函数时保持中文注释。
- Graphify 输出文件可能是 dirty 状态，不影响开发判断。

