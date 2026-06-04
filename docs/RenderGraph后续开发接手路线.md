# RenderGraph 后续开发接手路线

本文用于给下一次会话快速接手 RenderGraph 开发。重点记录当前状态、下一步顺序、模块边界和暂时不要提前做的内容。

## 1. 当前状态

当前 RenderGraph 已经完成基础接入：

- `EditorRenderer` 每帧通过 `RenderGraph` 声明并执行视口渲染流程。
- 已有 pass 顺序：
  - `GBuffer`
  - `Lighting`
  - `Skybox`
  - `CopyDepth`
  - `ToneMap`
  - `EditorGrid`
  - `Picking`
- `RenderGraph` 已支持：
  - `ImportRenderTarget`
  - `CreateTexture`
  - `AddPass`
  - `ReadTexture / WriteColor / TransferRead / TransferWrite`
  - `Compile`
  - resource lifetime / access summary
  - transient allocator
  - dump
- `Debug.TransientColor` 测试 pass 和测试窗口已经移除。
- 当前预览能力已经接到真实管线 RT：
  - `GBuffer.Color0..n`
  - `Lighting.Color0`
  - `Final.Color0`
- 当前中间 RT 仍然由 `EditorViewportSurface` 外部创建：
  - `GBuffer`
  - `Lighting`
  - `Final`
  - `Picking`

这意味着当前 RenderGraph 主要负责“声明、编译和执行编排”，还没有真正接管主渲染链路的中间 RT 生命周期。

## 2. 最终理想状态

RenderGraph 最终应该成为一帧渲染资源和 pass 的中心调度器。

外部只提供：

- swapchain / viewport final output
- picking readback 需要的外部资源
- scene data / camera / draw list / light data
- asset texture / material / mesh / shader
- history buffer、shadow cache、IBL 等跨帧资源

RenderGraph 内部负责：

- transient render target 创建与复用
- resource lifetime 分析
- usage / layout / barrier 推导
- pass 执行顺序
- pass culling
- frame debugger / RT debug 数据
- transient resource preview

目标边界：

```text
EditorViewportSurface
  只持有 viewport final output、picking readback、UI texture handle

EditorRenderer
  负责收集场景数据、维护 pass/feature、声明 graph

RenderGraph
  负责中间 RT、资源生命周期、barrier、调试数据

具体 RenderPass
  负责真正的 draw/dispatch，不负责资源生命周期
```

## 3. 推荐开发顺序

### 阶段 1：新增 VulkanRenderTargetView

目标：把“拥有资源的 RT”和“引用已有 image 的 RT view”拆开。

当前 `VulkanRenderTarget` 拥有 image，并提供 `BeginRendering / EndRendering`。RenderGraph transient image 由 allocator 持有，不能再让 `VulkanRenderTarget` 重复拥有。

建议新增：

```cpp
class VulkanRenderTargetView
{
public:
    struct ColorAttachment
    {
        VulkanImage* Image = nullptr;
        VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        VkClearValue ClearValue{};
    };

    std::string Name;
    uint32_t Width = 1;
    uint32_t Height = 1;
    std::vector<ColorAttachment> ColorAttachments;
    VulkanImage* DepthAttachment = nullptr;

    void BeginRendering(VkCommandBuffer commandBuffer);
    void EndRendering(VkCommandBuffer commandBuffer);
};
```

同时让 `VulkanRenderTarget` 可以生成 view：

```cpp
VulkanRenderTargetView VulkanRenderTarget::CreateView();
```

完成标准：

- 外部已有 `VulkanRenderTarget` 仍可正常渲染。
- `VulkanRenderTargetView` 可以引用外部 RT 的 image。
- 后续 transient image 可以组装成 view。

### 阶段 2：RenderPassContext 改为使用 RT View

目标：让现有 pass 不再强依赖 `VulkanRenderTarget&`。

当前代码类似：

```cpp
RenderPassContext passContext(
    context,
    commandBuffer,
    renderTarget);
```

建议改为：

```cpp
RenderPassContext passContext(
    context,
    commandBuffer,
    renderTargetView);
```

需要逐步修改：

- `BasePass`
- `DeferredLightingPass`
- `ToneMappingPass`
- `SkyboxPass`
- `EditorGridPass`
- `ViewportPickingPass`
- `PreviewSpherePass` 等依赖 `RenderPassContext` 的 pass

完成标准：

- 现有 viewport 渲染结果不变。
- `Release|x64` 编译通过。
- viewport、grid、skybox、picking 正常。

### 阶段 3：先迁移 Lighting 为 transient RT

目标：让 `Lighting` 从外部 RT 迁移为 RenderGraph 内部 transient texture。

推荐先做 Lighting，而不是 GBuffer，因为 Lighting 通常只有一张 color，风险更小。

大致形态：

```cpp
RenderGraphTextureDesc lightingDesc{};
lightingDesc.Name = "Lighting.Color";
lightingDesc.Width = viewportWidth;
lightingDesc.Height = viewportHeight;
lightingDesc.Format = lightingFormat;
lightingDesc.Usage =
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT |
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
lightingDesc.CreateSampler = true;

RenderGraphResourceID lightingID = graph.CreateTexture(lightingDesc);
```

Lighting pass 写 transient lighting，ToneMap / Skybox / CopyDepth 读取或写入 graph resource。

完成标准：

- `Lighting` 不再由 `EditorViewportSurface` 创建。
- `ToneMap` 能读取 graph 内部 lighting image。
- preview 列表能显示 transient lighting。
- viewport 最终画面不变。

### 阶段 4：迁移 GBuffer 为 transient 多附件

目标：把 GBuffer 的 base color、normal、material、emissive、depth 迁移进 RenderGraph。

建议定义：

```cpp
struct GBufferResources
{
    RenderGraphResourceID BaseColor = InvalidRenderGraphResourceID;
    RenderGraphResourceID Normal = InvalidRenderGraphResourceID;
    RenderGraphResourceID Material = InvalidRenderGraphResourceID;
    RenderGraphResourceID Emissive = InvalidRenderGraphResourceID;
    RenderGraphResourceID Depth = InvalidRenderGraphResourceID;
};
```

完成标准：

- `EditorViewportSurface` 不再创建 GBuffer RT。
- `BasePass` 写 graph transient GBuffer。
- `DeferredLightingPass` 读取 graph transient GBuffer。
- frame debugger / preview 可以看到每张 GBuffer。

### 阶段 5：RenderGraph 接管 layout / barrier

目标：不再依赖每个 `VulkanRenderTarget::BeginRendering / EndRendering` 自己做全部 transition。

RenderGraph compile 阶段根据 access 生成 layout plan：

```text
GBuffer.BaseColor:
  Undefined -> ColorAttachment
  ColorAttachment -> ShaderReadOnly

Lighting.Color:
  Undefined -> ColorAttachment
  ColorAttachment -> ShaderReadOnly

Final.Color:
  ShaderReadOnly/Undefined -> ColorAttachment
  ColorAttachment -> ShaderReadOnly
```

完成标准：

- 每个 pass 前后由 RenderGraph 插入必要 barrier。
- RenderPass 只表达读写语义，不自己猜测最终 layout。
- Vulkan validation 不出现新的 layout/barrier warning。

这是底层重大变化，做之前应先确认设计。

### 阶段 6：抽离 SetExecute，走 Pass Executor / RenderFeature

目标：从 lambda 过渡到类似 Unity RenderFeature 的模块化结构。

保留 `SetExecute(lambda)` 作为兼容入口，同时新增：

```cpp
class IRenderGraphPassExecutor
{
public:
    virtual ~IRenderGraphPassExecutor() = default;
    virtual void Execute(RenderGraphContext& context) = 0;
};
```

`RenderGraphPass` 支持：

```cpp
RenderGraphPass& SetExecutor(Unique<IRenderGraphPassExecutor> executor);
```

再引入：

```cpp
class RenderGraphFeature
{
public:
    virtual ~RenderGraphFeature() = default;
    virtual void AddPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard) = 0;
};
```

目标使用方式：

```cpp
for (RenderGraphFeature* feature : m_Features)
{
    feature->AddPasses(*m_RenderGraph, blackboard);
}
```

完成标准：

- 至少一个 pass 从 lambda 迁移到 executor。
- 至少一个 feature 能独立添加 pass。
- `EditorRenderer::BuildRenderGraph()` 开始从“堆 pass”变成“组织 feature”。

### 阶段 7：材质和 shader 自定义化

目标：减少硬编码 shader/material，让 pipeline request 从材质和 shader asset 推导。

当前硬编码示例：

```cpp
EditorProjectBootstrap::GetPreLoadShaderHandle("deferredLighting");
EditorProjectBootstrap::GetPreLoadShaderHandle("tonemap");
EditorProjectBootstrap::GetPreLoadMaterialHandle("skybox");
```

目标方向：

```cpp
struct MaterialAsset
{
    AssetHandle Shader;
    BlendState Blend;
    DepthState Depth;
    RasterState Raster;
    std::vector<TextureBinding> Textures;
    std::vector<MaterialParameter> Parameters;
};

struct ShaderAsset
{
    std::string VertexPath;
    std::string FragmentPath;
    std::vector<ShaderProperty> ReflectedProperties;
    std::vector<DescriptorBinding> ReflectedBindings;
    std::vector<ShaderVariantKeyword> Keywords;
};
```

Pipeline request 目标：

```cpp
PipelineRequest request = PipelineRequest::FromMaterial(
    material,
    renderTargetView,
    passType,
    vertexLayout);
```

完成标准：

- 材质可以选择 shader。
- descriptor layout 可以从 shader/material 反射或描述生成。
- pipeline state 不再大量写死在 `EditorRenderer`。

### 阶段 8：PipelineFactory 完整化 + VkPipelineCache

目标：在材质/shader 自定义化后，再做 Vulkan 驱动级 pipeline cache。

不要太早做 `VkPipelineCache`，因为材质系统稳定前，`PipelineRequest` 和 cache key 会频繁变化。

建议分两层：

```text
PipelineFactory
  按 PipelineRequest 做运行时 pipeline 对象缓存

VkPipelineCache
  Vulkan 驱动级编译缓存，可序列化到磁盘
```

完成标准：

- `PipelineRequest` 字段稳定。
- hash / equality 完整覆盖 shader、render target format、blend/depth/raster、descriptor layout 等。
- Vulkan pipeline cache 可以保存和加载。

### 阶段 9：Command Pool / 多 Command Buffer / Async Compute

当前不需要让 RenderGraph 管 command pool。

短期保持：

```cpp
graph.Execute(context, context.GetCurrentCommandBuffer());
```

等以下能力需要时再做：

- secondary command buffer
- parallel command recording
- async compute
- multi-queue scheduling
- per-pass GPU profiling query
- pass merge / renderpass batching

未来可以引入：

```cpp
struct RenderGraphCommandContext
{
    VkCommandBuffer Primary = VK_NULL_HANDLE;
    VkCommandPool GraphicsPool = VK_NULL_HANDLE;
    VkCommandPool ComputePool = VK_NULL_HANDLE;
    uint32_t FrameIndex = 0;
};
```

## 4. 不要提前做的事

当前阶段不要急着做：

- RenderGraph 自己管理 command pool。
- async compute。
- secondary command buffer。
- Vulkan `VkPipelineCache` 持久化。
- 完整材质编辑器。
- 大规模重写所有 pass。
- 一次性迁移全部 GBuffer / Lighting / Final / Picking。

优先级应该是：

```text
RT View
  -> PassContext 使用 View
  -> Lighting transient
  -> GBuffer transient
  -> barrier/layout
  -> executor/feature
  -> material/shader custom
  -> pipeline cache
  -> command pool / async compute
```

## 5. 当前模块边界

### RenderGraph 应该管理

- resource id
- imported / transient resource 描述
- pass read/write
- compile result
- lifetime
- access summary
- transient image allocator
- preview/debug metadata

### RenderGraph 暂时不应该管理

- pipeline 创建
- material descriptor 更新
- shader asset 加载
- mesh draw item 收集
- command pool
- swapchain
- editor UI

### Pipeline 归属

pipeline 推荐继续由 `PipelineFactory` 和具体 pass / renderer 管理。

RenderGraph 可以记录 debug 信息，例如 pass 使用了哪个 pipeline 名称，但不要拥有 pipeline。

## 6. 下一次接手建议从哪里开始

建议下一次直接从阶段 1 开始：

1. 新增 `VulkanRenderTargetView`。
2. 让 `VulkanRenderTarget` 生成 view。
3. 修改 `RenderPassContext` 支持 view。
4. 先让现有外部 RT 通过 view 渲染，确保画面不变。

不要一上来就迁移 Lighting/GBuffer。先把 pass 和 RT ownership 解耦，这一步是后续所有 graph transient 接入的地基。

## 7. 验证要求

每个阶段完成后至少验证：

- `Kita.slnx`
- `Release|x64`
- viewport 正常显示
- grid 开关正常
- skybox 正常
- picking 正常
- frame debugger / RT preview 数据不为空
- Vulkan validation 没有新增 layout/barrier warning

如果只改文档，不需要构建。

