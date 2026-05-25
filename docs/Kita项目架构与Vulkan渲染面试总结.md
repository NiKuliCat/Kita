# Kita 项目架构与 Vulkan 渲染面试总结

本文档用于总结 Kita 当前项目的整体框架、底层渲染架构、项目中实际用到的 Vulkan 知识点，以及围绕该项目可以准备的高频面试问题。

> 说明：本文档基于当前仓库代码结构整理，重点面向项目复盘、面试表达和后续进入项目时的快速理解。

---

## 1. 项目整体定位

Kita 是一个基于 Vulkan、ImGui 和若干第三方库构建的 Windows 桌面渲染引擎项目，当前形态更接近“带编辑器的实时渲染引擎原型”，后续可以继续演进为 3D 游戏引擎。

项目主要特点：

- 使用 Vulkan 作为底层图形 API。
- 使用 ImGui 构建编辑器界面和视口 UI。
- 使用 `Renderer` 可执行程序 + `Engine` 静态库的双层结构。
- 当前已具备场景、组件、资产、材质、网格、纹理、Shader、离屏渲染、延迟光照、天空盒、Tonemap、拾取和编辑器网格等能力。
- 构建系统以 Premake 为源头，生成 Visual Studio 解决方案。

---

## 2. 项目目录与模块分层

### 2.1 顶层目录

| 路径 | 职责 |
| --- | --- |
| `engine/` | 引擎静态库，封装底层能力，包括 Vulkan 渲染、资源、场景、组件、窗口、事件、序列化等。 |
| `renderer/` | 编辑器可执行程序，负责编辑器界面、视口、面板、项目加载、调试渲染和用户交互。 |
| `projects/` | 示例项目与项目资源，例如测试场景、材质、纹理、Shader、模型等。 |
| `docs/` | 项目文档与截图资源。 |
| `premake/` | Premake 可执行文件与构建辅助内容。 |
| `Build.lua` | Premake 主构建脚本。 |
| `Setup.bat` | 生成 Visual Studio 工程的入口。 |
| `Kita.slnx` | 当前推荐使用的 Visual Studio 解决方案文件。 |

### 2.2 Engine 静态库

`engine/` 是项目的可复用底层，核心职责包括：

- `core/`：应用框架、窗口抽象、日志、Layer 系统、输入等。
- `platform/windows/`：Windows 平台窗口和输入实现。
- `event/`：窗口、键盘、鼠标事件。
- `component/`：场景 ECS 相关组件，例如 `Scene`、`Object`、`Transform`、`MeshRenderer`、`LightComponent`。
- `asset/`：资产基础类型和资产管理，例如材质、Shader、纹理、Mesh。
- `serialize/`：场景、材质等序列化。
- `render/`：Vulkan 渲染封装和渲染管线。
- `system/`：时间系统等运行时基础服务。
- `third-party/`：第三方库代码或头文件。

### 2.3 Renderer 可执行程序

`renderer/` 是编辑器层，主要负责：

- 编辑器应用入口。
- EditorLayer 生命周期。
- ImGui 面板，例如场景层级、Inspector、Content Browser、材质/纹理/Mesh 编辑器。
- 编辑器视口和多视口管理。
- 编辑器相机、Gizmo、拾取、网格、视口渲染。
- 项目启动和预加载资源管理。

### 2.4 分层边界

项目采用比较清晰的“上层编辑器 + 底层引擎”边界：

- `engine` 不应直接依赖编辑器 UI 逻辑。
- `renderer` 可以调用 `engine` 提供的资源、场景、渲染接口。
- Vulkan 资源、渲染 pass、材质、Pipeline、IBL 等底层能力应优先放在 `engine`。
- 视口交互、面板、编辑器选择、拾取注册、调试可视化应优先放在 `renderer`。

---

## 3. 构建系统

当前构建系统以 Premake 为源头：

- `Build.lua`：workspace、全局 include/library、第三方依赖和项目引入。
- `engine/engine_core_build.lua`：`Engine` 静态库构建规则。
- `renderer/renderer_build.lua`：`Renderer` 可执行程序构建规则。
- `Setup.bat`：调用 Premake 生成 VS 工程。

工程约定：

- Workspace 名称：`Kita`
- 架构：`x86_64`
- 配置：`Debug`、`Release`、`Dist`
- 默认启动项目：`Renderer`
- C++ 标准：C++20
- 主要外部库：Vulkan SDK、GLFW、ImGui、glm、spdlog、assimp、entt、nlohmann json、ImGuizmo、Slang

依赖接入方式：

- Vulkan 与 Slang 来自 `VULKAN_SDK` 环境变量。
- assimp 使用本地二进制库，按 Debug/Release 分别链接和复制 DLL。
- GLFW、ImGui 通过 Premake 子项目构建。

---

## 4. 应用生命周期

核心入口在 `engine/src/core/Application.cpp`。

启动流程：

1. 创建窗口。
2. 初始化 `VulkanContext`。
3. 初始化 ImGui Layer。
4. 初始化 Renderer 相关层。
5. 初始化 pending layers。
6. 进入主循环。
7. 退出时等待 GPU idle，销毁 Layer、资产、VulkanContext 和窗口。

每帧主循环大致是：

1. `TimeSystem::Tick()` 更新时间。
2. 调用各 Layer 的 `OnUpdate()`。
3. `VulkanContext::BeginFrame()` 获取 swapchain image 并开始 command buffer。
4. 调用各 Layer 的 `OnRender()`，此阶段通常录制离屏渲染命令。
5. ImGui `Begin()`。
6. 调用各 Layer 的 `OnImGuiRender()`，录制 UI。
7. `VulkanRenderCommand::BeginSwapchainRendering()` 开始对 swapchain image 动态渲染。
8. ImGui `End()` 将 UI 绘制到 swapchain image。
9. `VulkanRenderCommand::EndRendering()` 结束 swapchain rendering 并转为 present layout。
10. `VulkanContext::EndFrame()` 提交 command buffer 并 present。
11. `Window::OnUpdate()` 处理窗口事件。

这个设计意味着：场景通常先渲染到离屏 render target，最终作为 ImGui texture 显示在编辑器视口中，swapchain 主要承载最终 UI 合成结果。

---

## 5. 底层渲染架构总览

当前渲染架构可以拆成以下层次：

```text
Application / Layer
    |
    v
EditorRenderer / EditorViewportSurface
    |
    v
RenderPass / SceneBindings / PipelineFactory
    |
    v
VulkanMaterial / VulkanGeometry / VulkanTexture / VulkanShader
    |
    v
VulkanRenderTarget / VulkanDescriptorSet / VulkanBuffer / VulkanImage
    |
    v
VulkanContext / VulkanRenderCommand
    |
    v
Vulkan API
```

### 5.1 VulkanContext

`VulkanContext` 是 Vulkan 运行环境的核心封装，负责：

- 创建 Vulkan instance。
- 启用 validation layer 和 debug messenger。
- 创建 GLFW window surface。
- 选择 physical device。
- 创建 logical device。
- 获取 graphics queue 和 present queue。
- 创建 swapchain 和 image view。
- 创建 command pool 和每帧 command buffer。
- 创建同步对象：image available semaphore、render finished semaphore、in-flight fence。
- 管理 `BeginFrame()` / `EndFrame()`。
- 处理窗口 resize 和 swapchain recreate。

当前特征：

- 目标 Vulkan API 版本为 Vulkan 1.3。
- 启用了 `dynamicRendering` 特性。
- 启用了 `shaderDrawParameters` 特性。
- 默认 frames in flight 为 2。
- 每个 in-flight frame 使用一个 primary command buffer。
- 当前主要使用 graphics queue 完成绘制和资源上传。

### 5.2 VulkanRenderCommand

`VulkanRenderCommand` 是轻量命令辅助层，负责：

- swapchain image 的 layout transition。
- 使用 dynamic rendering 开始/结束 swapchain 渲染。
- 设置 viewport 和 scissor。
- 绑定 pipeline。
- 绑定 geometry。
- 发起 draw / draw indexed。

它不是完整渲染器，更像是 Vulkan 命令录制的便捷封装。

### 5.3 VulkanImage / VulkanBuffer / VulkanTexture

资源封装层负责把 Vulkan 原始对象包装成 C++ 对象：

- `VulkanBuffer`：封装 vertex/index/uniform/storage/staging/readback buffer。
- `VulkanImage`：封装 `VkImage`、`VkDeviceMemory`、`VkImageView`、可选 `VkSampler`。
- `VulkanTexture`：基于 `VulkanImage` 组织 2D 纹理、Cube 纹理、mipmap、采样器和像素上传。

其中 `VulkanImage` 支持：

- 自动创建 image / memory / image view / sampler。
- staging buffer 上传初始数据。
- layout transition。
- descriptor image info 获取。
- depth/color/texture 常用 create info helper。

### 5.4 VulkanRenderTarget

`VulkanRenderTarget` 是离屏渲染目标封装，负责：

- 管理一个或多个 color attachment。
- 管理可选 depth attachment。
- 支持 MSAA resolve image。
- 支持 attachment resize。
- 使用 `vkCmdBeginRendering` / `vkCmdEndRendering`。
- 在 pass 前后自动处理 attachment layout transition。
- 将 color/depth attachment 暴露为 sampled image 给后续 pass 或 ImGui。

这是当前渲染管线的关键基础设施。编辑器视口、GBuffer、Lighting、Final、Picking 都建立在它之上。

### 5.5 VulkanGraphicsPipeline

`VulkanGraphicsPipeline` 封装 graphics pipeline 和 pipeline layout。

当前支持：

- vertex shader / fragment shader。
- vertex input 或 fullscreen pass 无 vertex input。
- descriptor set layouts。
- push constants。
- dynamic viewport/scissor。
- topology、polygon mode、cull mode、front face。
- depth test/write/compare。
- blending。
- sample count。
- dynamic rendering 下的 color/depth attachment format。
- 多 color attachment 的 pipeline color format 配置。

注意点：

- dynamic rendering 模式下，pipeline 创建时不绑定传统 `VkRenderPass`。
- 通过 `VkPipelineRenderingCreateInfo` 指定 color/depth format。

### 5.6 VulkanDescriptorSet

Descriptor 封装承担资源绑定职责：

- 创建 descriptor set layout。
- 创建 descriptor pool。
- 分配 descriptor set。
- 写入 uniform buffer。
- 写入 combined image sampler。
- 绑定 descriptor set 到 pipeline layout。

当前项目里常见 set 约定：

- set 0：场景级资源，例如 camera UBO、directional light UBO。
- set 1：材质或 pass 输入资源，例如材质贴图、GBuffer、IBL、Tonemap source。

### 5.7 SceneBindings

`SceneBindings` 是场景级 GPU 资源绑定层，负责：

- 每帧 camera UBO。
- 每帧 main directional light UBO。
- 每帧 scene descriptor set。
- 将 scene data 更新到当前 frame 的 UBO。

它把原先散在渲染器里的 scene-level descriptor 逻辑抽离出来，使多个 pass 可以共享相同场景数据。

### 5.8 RenderPass 框架

当前已经形成基础 RenderPass 抽象：

- `IRenderPass`：统一接口，提供 `GetDesc()` 和 `Execute()`。
- `RenderPassBase`：负责 render target 校验、begin pass、end pass。
- `SceneRenderPassBase`：增加 `SceneBindings` 和 scene data 更新能力。
- `FullscreenPassBase`：用于 fullscreen triangle/quad 类 pass，例如 DeferredLighting、Tonemap。

相关数据结构：

- `PassType`
- `RenderPassDesc`
- `RenderPassBeginInfo`
- `SceneCameraData`
- `SceneDirectionalLightData`
- `ScenePassData`
- `ObjectData`

当前已有或正在使用的 pass：

- `BasePass`：写入 GBuffer。
- `DeferredLightingPass`：读取 GBuffer、IBL，输出 lighting RT。
- `SkyboxPass`：绘制天空盒。
- `ToneMappingPass`：将 HDR lighting 结果转换到 final RT。
- `EditorGridPass`：绘制编辑器网格 overlay。
- `ViewportPickingPass`：绘制对象拾取 ID。
- `ForwardOpaquePass`：前向不透明 pass 基础。

### 5.9 PipelineFactory

`PipelineFactory` 是 pipeline 缓存和复用层。

它通过 `PipelineRequest` 构建 `PipelineKey`，然后查找或创建 `VulkanGraphicsPipeline`。

PipelineKey 包含：

- pass 类型。
- color attachment formats 和数量。
- depth format。
- sample count。
- topology / polygon / cull / front face。
- depth state。
- blending state。
- vertex shader module。
- fragment shader module。
- descriptor set layout hash。
- vertex layout hash。
- push constant 信息。

这个设计解决了“每个 renderer 临时创建 pipeline”的问题，是后续扩展更多 pass 和材质变体的基础。

### 5.10 VulkanMaterial

`VulkanMaterial` 当前已经从早期单 albedo 材质扩展为更完整的 PBR 参数和贴图结构。

材质 GPU 参数包括：

- BaseColor
- Emissive
- Metallic
- Roughness
- AmbientOcclusion
- Opacity
- NormalScale
- AlphaCutoff
- TextureFlags

材质贴图槽包括：

- Albedo
- Normal
- MetallicRoughness
- AmbientOcclusion
- Emissive
- Opacity

每个材质会维护 per-frame 参数 UBO 和 descriptor set，并支持 fallback texture，避免 shader 采样无效 descriptor。

### 5.11 VulkanResourceFactory

`VulkanResourceFactory` 是资产层到 Vulkan runtime 对象的桥：

- ShaderAsset -> VulkanShader bundle。
- TextureAsset -> VulkanTexture。
- MaterialAsset -> VulkanMaterial。
- MeshAsset -> VulkanGeometry。

它负责缓存 runtime 资源，并提供 invalidate / refresh 机制。

### 5.12 EditorViewportSurface

`EditorViewportSurface` 负责编辑器视口所需的离屏资源。

它创建多个 render target：

- GBuffer RT：
  - BaseColor：`R8G8B8A8_UNORM`
  - Normal：`R16G16B16A16_SFLOAT`
  - Material：`R8G8B8A8_UNORM`
  - Emissive：`R16G16B16A16_SFLOAT`
  - Depth：`D32_SFLOAT`
- Lighting RT：
  - HDR color：`R16G16B16A16_SFLOAT`
  - Depth
- Final RT：
  - 最终视口颜色
  - Depth
- Picking RT：
  - `R32_UINT` 对象 ID
  - Depth

Final RT 的 sampled color attachment 会注册给 ImGui：

```cpp
ImGui_ImplVulkan_AddTexture(
    sampledImage.GetSampler(),
    sampledImage.GetView(),
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
```

这样编辑器视口本质上显示的是一个 Vulkan 离屏渲染结果。

### 5.13 EditorRenderer 当前渲染链路

`EditorRenderer::Render()` 是当前编辑器视口渲染的核心组织者。

当前主要渲染顺序：

1. 收集 scene camera、main light 等 scene data。
2. 遍历 `Transform + MeshRenderer` 组件组。
3. 为每个 mesh primitive 获取 VulkanGeometry。
4. 获取材质并刷新材质当前帧 descriptor。
5. 通过 PipelineFactory 获取 GBuffer pipeline。
6. 向 BasePass 添加 draw item。
7. 同时为 PickingPass 添加 picking draw item。
8. 执行 BasePass，输出 GBuffer。
9. 执行 DeferredLightingPass，读取 GBuffer + IBL，输出 Lighting RT。
10. 执行 SkyboxPass，叠加天空盒。
11. 将 lighting depth 拷贝到 final depth。
12. 执行 ToneMappingPass，输出 Final RT。
13. 执行 EditorGridPass，叠加编辑器网格。
14. 执行 ViewportPickingPass，输出 picking ID RT。
15. Final RT 被 ImGui 作为视口纹理显示。

可以概括为：

```text
Scene Meshes
    |
    v
BasePass -> GBuffer
    |
    v
DeferredLightingPass + IBL -> Lighting RT
    |
    v
SkyboxPass -> Lighting RT
    |
    v
ToneMappingPass -> Final RT
    |
    v
EditorGridPass -> Final RT
    |
    v
ImGui Viewport Image

Parallel debug path:
Scene Meshes -> ViewportPickingPass -> Picking RT -> CPU readback on click
```

### 5.14 IBL 系统

`IBLGenerator` 根据场景环境设置生成 IBL 资源：

- EnvironmentCube
- IrradianceCube
- PrefilteredSpecularCube
- BrdfLut

当前 IBL 生成以 CPU 计算为主：

- 从 equirectangular 环境图采样生成 cubemap。
- 半球积分生成 irradiance cube。
- GGX importance sampling 生成 prefilter mip chain。
- split-sum 方式生成 BRDF LUT。
- 使用 staging buffer 上传到 VulkanTexture。

DeferredLightingPass 会读取这些 IBL 纹理参与 PBR 光照。

---

## 6. 当前使用到的 Vulkan 知识点

### 6.1 Instance / Validation Layer / Debug Messenger

项目创建 `VkInstance` 时：

- 设置 `VkApplicationInfo`。
- 请求 GLFW 所需 instance extensions。
- Debug 模式下启用 `VK_LAYER_KHRONOS_validation`。
- 使用 `VK_EXT_debug_utils` 创建 debug messenger。

面试表达重点：

- Instance 是 Vulkan 应用和驱动之间的入口。
- Validation layer 不参与最终渲染，但能在开发阶段捕获 API 使用错误。
- Debug messenger 用于接收 validation/performance/general 消息。

### 6.2 Physical Device / Logical Device / Queue Family

项目会枚举 physical device，并检查：

- 是否存在 graphics queue。
- 是否存在 present queue。
- 是否支持 swapchain extension。
- swapchain format 和 present mode 是否可用。
- 是否支持 Vulkan 1.3 dynamic rendering。
- 是否支持 shader draw parameters。

之后创建 logical device，并获取 graphics queue 和 present queue。

面试表达重点：

- Physical device 是实际 GPU。
- Logical device 是应用访问 GPU 的逻辑接口。
- Queue family 决定队列能力，例如 graphics、compute、transfer、present。

### 6.3 Swapchain

Swapchain 负责把渲染结果提交给窗口系统显示。

项目中 swapchain 创建关注：

- surface format。
- present mode。
- extent。
- image count。
- image usage。
- image sharing mode。
- image view。
- resize/recreate。

当前 swapchain image usage 是 color attachment，最终 ImGui UI 会直接绘制到 swapchain image。

面试表达重点：

- Swapchain image 不是自己创建的普通 image，而是由 WSI 管理。
- 窗口 resize、最小化、surface 变化会导致 swapchain out-of-date，需要重建。
- FIFO present mode 保证可用，Mailbox 延迟更低但不一定支持。

### 6.4 Command Pool / Command Buffer

项目为 graphics queue family 创建 command pool，并按 frames in flight 分配 primary command buffer。

每帧：

- 等待当前 frame fence。
- acquire swapchain image。
- reset command buffer。
- begin command buffer。
- 录制场景、UI、swapchain rendering。
- end command buffer。
- queue submit。
- queue present。

面试表达重点：

- Vulkan 是显式 API，渲染命令需要录制到 command buffer。
- Command pool 决定 command buffer 的分配来源和 queue family。
- 多线程录制通常会使用多个 command pool 和 secondary command buffer。

### 6.5 同步对象

项目使用：

- `VkSemaphore`：GPU-GPU 同步。
  - image available semaphore：等待 swapchain image 可用。
  - render finished semaphore：等待渲染完成后 present。
- `VkFence`：CPU-GPU 同步。
  - in-flight fence：CPU 等待某一帧 GPU 完成，避免覆盖仍在使用的 command buffer 和 per-frame 资源。

面试表达重点：

- Semaphore 用于 queue submit 和 present 之间的 GPU 同步。
- Fence 用于 CPU 等待 GPU。
- Frames in flight 可以提高 CPU/GPU 并行度，但需要 per-frame 资源隔离。

### 6.6 Dynamic Rendering

项目使用 Vulkan 1.3 dynamic rendering，而不是传统 `VkRenderPass + VkFramebuffer`。

使用方式：

- pass 开始时填写 `VkRenderingInfo`。
- color/depth attachment 用 `VkRenderingAttachmentInfo` 描述。
- 调用 `vkCmdBeginRendering`。
- 结束时调用 `vkCmdEndRendering`。
- 创建 pipeline 时通过 `VkPipelineRenderingCreateInfo` 指定 attachment formats。

优点：

- 更适合编辑器多视口。
- 更适合离屏 render target。
- 更适合后处理链。
- 减少传统 render pass/framebuffer 组合数量。

### 6.7 Pipeline State

项目创建 graphics pipeline 时配置：

- shader stages。
- vertex input。
- input assembly。
- viewport/scissor dynamic state。
- rasterization。
- multisample。
- depth stencil。
- color blend。
- pipeline layout。
- dynamic rendering formats。

面试表达重点：

- Vulkan pipeline 是大部分 GPU 状态的预编译组合。
- Pipeline 创建成本较高，应缓存复用。
- Pipeline layout 必须和 descriptor set / push constant 使用方式匹配。

### 6.8 Descriptor Set

项目使用 descriptor set 绑定：

- UBO：camera、light、material params。
- combined image sampler：材质贴图、GBuffer、IBL、Tonemap source。

面试表达重点：

- Descriptor set 是 shader 访问资源的绑定表。
- Descriptor set layout 是绑定协议。
- Pipeline layout 包含 descriptor set layout 和 push constant range。
- per-frame descriptor/UBO 能避免 CPU 覆盖 GPU 正在读取的数据。

### 6.9 Uniform Buffer / Push Constant

项目中：

- Camera、light、material params 使用 UBO。
- 每个 object 的 model matrix、inverse model matrix 使用 push constant。
- Grid、Skybox、Picking 也使用 push constant 传递小块频繁变化数据。

面试表达重点：

- UBO 适合中小块、多个 shader 共享、每帧或每材质更新的数据。
- Push constant 适合非常小、频繁变化的数据，更新开销低。
- Push constant 大小受硬件限制，不能滥用。

### 6.10 Image Layout / Pipeline Barrier

项目显式管理 image layout：

- swapchain：`UNDEFINED/PRESENT_SRC_KHR -> COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR`
- render target color：`UNDEFINED/SHADER_READ_ONLY_OPTIMAL -> COLOR_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL`
- depth：`DEPTH_STENCIL_ATTACHMENT_OPTIMAL`、`TRANSFER_SRC/DST_OPTIMAL`
- texture upload：`UNDEFINED -> TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL`
- picking readback：`COLOR_ATTACHMENT_OPTIMAL -> TRANSFER_SRC_OPTIMAL`

面试表达重点：

- Vulkan 不会自动推断资源状态，layout 和访问同步需要开发者显式管理。
- Barrier 同时解决执行依赖和内存可见性问题。
- oldLayout/newLayout、src/dst stage、src/dst access mask 必须匹配资源使用场景。

### 6.11 Staging Buffer 上传

纹理和 IBL 资源上传使用 staging buffer：

1. 创建 host visible / host coherent buffer。
2. 写入 CPU 数据。
3. image transition 到 transfer dst。
4. `vkCmdCopyBufferToImage`。
5. image transition 到 shader read only。

面试表达重点：

- Device local image 通常不能直接 CPU 写入。
- Staging buffer 是 CPU 到 GPU device local 资源的常用桥梁。

### 6.12 MRT / GBuffer

BasePass 输出多个 color attachment：

- base color
- normal
- material
- emissive

DeferredLightingPass 再读取这些 attachment 做延迟光照。

面试表达重点：

- MRT 指一个 fragment shader 同时输出多个 render target。
- Deferred rendering 把几何阶段和光照阶段拆开。
- 优点是多光源时可以减少重复材质计算。
- 缺点是带宽和显存占用更高，透明物体处理更复杂。

### 6.13 HDR / Tonemapping

Lighting RT 使用 `R16G16B16A16_SFLOAT`，适合保留 HDR 光照结果。

ToneMappingPass 将 HDR lighting 结果转换到最终视口输出。

面试表达重点：

- HDR buffer 可以存储超过 1.0 的亮度。
- Tonemapping 把 HDR 映射到显示设备可呈现的 LDR 范围。

### 6.14 ImGui Vulkan 集成

项目中 ImGui：

- 使用 ImGui Vulkan backend。
- UI 最终绘制到 swapchain image。
- 编辑器视口通过 `ImGui_ImplVulkan_AddTexture` 显示 final render target。

面试表达重点：

- ImGui 的 Vulkan backend 也需要 descriptor pool、rendering 信息和 command buffer。
- 离屏图像显示到 ImGui 前，需要 image layout 处于 shader read only。

### 6.15 Picking Readback

项目使用 `R32_UINT` picking render target：

1. 每个对象绘制唯一 PickID。
2. 鼠标点击时把指定像素从 image copy 到 readback buffer。
3. CPU map buffer 读取 uint32。
4. 根据 PickID 找回对象。

面试表达重点：

- GPU -> CPU readback 需要同步，否则 CPU 可能读到未完成的数据。
- 项目当前直接 `WaitIdle()`，实现简单但性能不是最优。
- 后续可用异步 readback、延迟一帧读取、fence 管理优化。

---

## 7. 可以在面试中强调的项目亮点

### 7.1 使用 Vulkan 1.3 Dynamic Rendering

相比传统 render pass，dynamic rendering 更现代，也更适合编辑器多视口和后处理链路。

### 7.2 已具备 Pass 化渲染框架

项目不是单个硬编码 draw loop，而是已经有：

- RenderPass 抽象。
- SceneBindings。
- FullscreenPassBase。
- BasePass。
- DeferredLightingPass。
- ToneMappingPass。
- PickingPass。

这说明项目正在向可扩展渲染管线演进。

### 7.3 离屏编辑器视口

视口不是直接渲染到 swapchain，而是渲染到独立 render target，再交给 ImGui 显示。这对编辑器非常关键。

支持能力包括：

- 多视口。
- 后处理。
- debug view。
- picking。
- overlay。
- viewport resize。

### 7.4 PipelineFactory 缓存

PipelineFactory 使用 PipelineKey 对 pipeline 进行复用，已经开始解决 Vulkan pipeline 数量和状态组合管理问题。

### 7.5 延迟渲染与 PBR/IBL 基础

项目已有：

- GBuffer。
- Deferred lighting。
- PBR 材质参数。
- IBL cubemap。
- irradiance。
- prefilter。
- BRDF LUT。
- HDR lighting。
- Tonemap。

这是一套比较完整的现代实时渲染基础链路。

---

## 8. 当前架构的不足与可优化方向

### 8.1 RenderGraph 尚未形成

当前 pass 执行顺序仍主要由 `EditorRenderer` 手动组织。

后续可以引入 RenderGraph 管理：

- pass 依赖。
- transient resource。
- resource lifetime。
- layout transition。
- barrier 推导。
- debug 可视化。

### 8.2 Barrier 管理仍偏手动

当前 layout transition 分散在 `VulkanRenderTarget`、`VulkanImage`、`VulkanRenderCommand` 和部分 pass 逻辑中。

后续可收敛为统一资源状态追踪系统。

### 8.3 资源上传大量使用 queue wait idle

单次上传和 IBL 生成中使用 `vkQueueWaitIdle`，实现简单但会阻塞 GPU。

后续可改为：

- upload queue。
- transfer command ring。
- fence 回收 staging buffer。
- frame delayed deletion。

### 8.4 Picking readback 同步方式较重

当前读 picking pixel 会等待设备或队列空闲，适合编辑器原型，但性能不优。

可优化为：

- 延迟一帧 readback。
- 使用 fence 判断 readback 是否完成。
- 批量 copy 更大区域。

### 8.5 材质系统仍可继续泛化

虽然已经有 PBR 常用参数和贴图槽，但还不是 shader reflection 驱动的通用材质系统。

未来可加入：

- shader 参数反射。
- 材质参数表。
- texture slot 自动绑定。
- shader variant。
- material pipeline state。

### 8.6 多线程录制与多队列尚未展开

当前每帧主要使用一个 primary command buffer，后续可考虑：

- 多线程构建 draw list。
- secondary command buffer。
- async compute。
- async transfer。

---

## 9. 高频 Vulkan 面试问题与回答

### Q1：Vulkan 和 OpenGL 最大区别是什么？

Vulkan 是显式图形 API，开发者需要显式管理设备、内存、同步、资源状态、command buffer 和 pipeline。OpenGL 更偏隐式状态机，驱动替开发者做了大量调度。Vulkan 的优势是可控性强、CPU 开销低、多线程友好；代价是代码复杂度和同步复杂度更高。

### Q2：Vulkan 初始化大致流程是什么？

典型流程是：创建 instance，启用 validation layer，创建 surface，选择 physical device，检查 queue family 和 device extension，创建 logical device，获取 queue，创建 swapchain，创建 image view，创建 command pool 和 command buffer，创建同步对象，然后进入每帧 acquire-record-submit-present 流程。

### Q3：PhysicalDevice 和 Device 有什么区别？

PhysicalDevice 是物理 GPU 的抽象，用来查询能力、格式、内存类型和队列族。Device 是 logical device，是应用实际创建资源、提交命令、访问队列的对象。

### Q4：Queue family 是什么？

Queue family 表示一组具备相同能力的队列，例如 graphics、compute、transfer。创建 logical device 时需要指定要从哪些 queue family 创建 queue。渲染窗口还要检查某个 queue family 是否支持 present。

### Q5：Swapchain 的作用是什么？

Swapchain 是窗口系统和 Vulkan 之间的显示图像队列。应用 acquire 一张 swapchain image，渲染到它，最后 present 给窗口系统显示。窗口 resize 或 surface 变化时，swapchain 可能 out of date，需要重建。

### Q6：为什么需要 frames in flight？

Frames in flight 允许 CPU 在 GPU 处理上一帧时准备下一帧，提高并行度。代价是每帧会有独立 command buffer、fence、UBO、descriptor 等资源，避免 CPU 覆盖 GPU 正在使用的数据。

### Q7：Semaphore 和 Fence 有什么区别？

Semaphore 主要用于 GPU 队列之间或 queue submit/present 之间的同步，CPU 一般不直接等待 binary semaphore。Fence 用于 CPU 等待 GPU 完成某个提交。项目里 acquire image 后等待 image available semaphore，渲染完成 signal render finished semaphore，CPU 用 in-flight fence 控制帧资源复用。

### Q8：Command buffer 的作用是什么？

Command buffer 用于记录 GPU 命令，例如 begin rendering、bind pipeline、bind descriptor、draw、copy、barrier。Vulkan 中命令不是直接立即执行，而是先录制，再提交到 queue。

### Q9：Command pool 为什么和 queue family 相关？

Command buffer 从 command pool 分配，而 command pool 创建时绑定某个 queue family。分配出的 command buffer 应提交到兼容该 queue family 的 queue。

### Q10：什么是 Dynamic Rendering？

Dynamic Rendering 是 Vulkan 1.3 核心特性，可以不用预先创建 `VkRenderPass` 和 `VkFramebuffer`，而是在录制命令时通过 `VkRenderingInfo` 直接指定 color/depth attachment。Pipeline 创建时通过 `VkPipelineRenderingCreateInfo` 指定 attachment format。

### Q11：Dynamic Rendering 的优点是什么？

它减少了 render pass/framebuffer 对象组合，适合离屏渲染、多视口、后处理链和动态 render target。项目中编辑器视口、GBuffer、Lighting、Final RT 都适合这种模式。

### Q12：传统 RenderPass 和项目里的 RenderPass 抽象是一回事吗？

不是。Vulkan 传统 `VkRenderPass` 是 API 对象；项目里的 `RenderPass` 是引擎层抽象，用来组织一次渲染阶段的 begin、resource binding、draw 和 end。项目底层实际使用的是 Vulkan dynamic rendering。

### Q13：Pipeline layout 是什么？

Pipeline layout 描述 shader 可访问的 descriptor set layout 和 push constant range。绑定 descriptor set 或 push constant 时必须与当前 pipeline layout 兼容。

### Q14：为什么 Vulkan pipeline 需要缓存？

Graphics pipeline 包含大量固定状态和 shader 编译链接信息，创建成本高。频繁创建会造成卡顿。项目用 `PipelineFactory` 根据 shader、render target format、vertex layout、descriptor set layout、渲染状态等生成 key，并缓存 pipeline。

### Q15：Descriptor set 是什么？

Descriptor set 是 shader 访问 buffer、image、sampler 等资源的绑定集合。Descriptor set layout 定义绑定槽位、类型、数量和 shader stage。项目中 set 0 通常是 scene UBO，set 1 通常是 material 或 pass 输入。

### Q16：UBO 和 Push Constant 怎么选择？

UBO 适合中小块数据、跨 draw 或跨 shader 共享的数据，例如 camera、light、material params。Push constant 适合很小且频繁变化的数据，例如每个对象的矩阵、picking ID、grid 参数。Push constant 更新快，但容量有限。

### Q17：为什么要做 image layout transition？

Vulkan 要求 image 在不同用途下处于合适 layout，例如 color attachment、shader read、transfer dst、present。layout transition 同时配合 memory barrier，确保前一次写入对后续读取可见。

### Q18：Pipeline barrier 解决什么问题？

Pipeline barrier 解决执行顺序和内存可见性问题。它指定 src stage/access 和 dst stage/access，让 GPU 知道前一个阶段的哪些访问必须完成并对后一个阶段可见。

### Q19：Staging buffer 为什么常用于纹理上传？

GPU device local image 性能好，但 CPU 通常不能直接写。做法是先写入 host visible staging buffer，再通过 copy command 拷贝到 device local image，最后转为 shader read layout。

### Q20：什么是 GBuffer？

GBuffer 是延迟渲染中保存几何阶段结果的一组 render target，通常包括 base color、normal、roughness/metallic、emissive、depth 等。后续 lighting pass 读取 GBuffer 计算光照。

### Q21：延迟渲染的优缺点是什么？

优点是几何和光照解耦，多光源时可以减少重复绘制和材质计算。缺点是 GBuffer 带宽和显存开销大，透明物体处理麻烦，MSAA 支持更复杂。

### Q22：项目里的 BasePass 做什么？

BasePass 遍历场景 draw item，把材质、法线、PBR 参数等输出到 GBuffer，同时写 depth。它是延迟渲染的几何阶段。

### Q23：项目里的 DeferredLightingPass 做什么？

DeferredLightingPass 读取 GBuffer 的多个 attachment、depth、IBL 纹理，结合 scene camera/light 数据，在 fullscreen pass 中计算 lighting 结果，输出到 HDR lighting render target。

### Q24：为什么 Lighting RT 使用 `R16G16B16A16_SFLOAT`？

因为 lighting 结果可能超过 0 到 1，需要 HDR 格式保留高亮信息。半浮点 RGBA 在质量和带宽之间比较平衡。

### Q25：Tonemapping 是什么？

Tonemapping 把 HDR 光照结果映射到显示设备可显示的 LDR 范围。项目中 ToneMappingPass 读取 lighting RT，输出 final RT 给编辑器视口显示。

### Q26：IBL 里的 irradiance、prefilter、BRDF LUT 分别是什么？

Irradiance cube 表示漫反射环境光卷积结果。Prefiltered specular cube 表示不同 roughness 下的镜面反射预过滤结果。BRDF LUT 存储 split-sum 近似中和视角、roughness 相关的积分结果。

### Q27：为什么编辑器视口要离屏渲染？

离屏渲染能让场景结果作为 ImGui texture 显示，并支持独立 resize、多视口、后处理、拾取、debug overlay、viewport gizmo 等编辑器功能。直接渲染到 swapchain 不适合复杂编辑器 UI。

### Q28：Picking 是怎么实现的？

项目为 picking 创建 `R32_UINT` render target。每个对象绘制时输出唯一 PickID。鼠标点击时把对应像素 copy 到 CPU 可读 buffer，再根据 ID 查找对象。

### Q29：为什么 readback 会影响性能？

GPU 到 CPU readback 需要同步。如果直接 wait idle，会强制 CPU 等待 GPU 完成，破坏并行。优化方式是异步 readback、延迟一帧读取、使用 fence 判断结果是否可用。

### Q30：为什么项目使用 per-frame UBO 和 descriptor？

因为 frames in flight 下 GPU 可能还在读取上一帧 UBO。如果 CPU 直接覆盖同一个 buffer，会产生数据竞争。per-frame 资源可以让每帧写入独立区域或独立对象，避免同步复杂度。

### Q31：Viewport 为什么要设置负高度？

Vulkan 的 NDC/Y 方向和一些上层坐标习惯不同。项目通过设置 viewport 的 `y = height`、`height = -height` 实现 Y 翻转，使最终画面方向符合预期。

### Q32：为什么要有 fallback texture？

Vulkan shader 采样的 descriptor 必须有效。材质某个贴图槽为空时，如果不绑定有效 fallback texture，shader 采样可能产生 validation error 或未定义行为。项目用 white/black/normal fallback 保证 descriptor 完整。

### Q33：什么是 PipelineKey 中的 vertex layout hash？

同一个 shader 和 render target format 下，如果 vertex buffer layout 不同，pipeline 的 vertex input state 也不同。因此 PipelineKey 需要包含 vertex layout hash，避免错误复用 pipeline。

### Q34：RenderGraph 能解决什么问题？

RenderGraph 可以集中管理 pass 依赖和资源读写关系，自动或半自动处理资源生命周期、layout transition、barrier 和执行顺序。当前项目已有 pass 层，但 pass 顺序和资源状态仍主要手动组织。

### Q35：如果让你继续优化这个 Vulkan 渲染器，你会先做什么？

可以按优先级回答：先收敛 RenderGraph/资源状态管理，再优化上传和 readback 同步，随后完善材质反射和 shader variant，最后再做多线程 command buffer 录制、多队列和异步 compute/transfer。

---

## 10. 面试中推荐的项目讲述顺序

可以按下面顺序讲，逻辑会比较清楚：

1. 先讲整体结构：`Renderer` 是编辑器可执行程序，`Engine` 是静态库。
2. 再讲应用主循环：update、BeginFrame、Layer render、ImGui、swapchain present。
3. 再讲 VulkanContext：instance、device、queue、swapchain、command buffer、sync。
4. 再讲离屏视口：EditorViewportSurface 创建 GBuffer/Lighting/Final/Picking RT。
5. 再讲 pass 链：BasePass -> DeferredLighting -> Skybox -> Tonemap -> Grid -> Picking。
6. 再讲资源系统：Asset -> VulkanResourceFactory -> VulkanShader/Texture/Material/Geometry。
7. 再讲 PipelineFactory：用 key 缓存 pipeline。
8. 再讲亮点：dynamic rendering、deferred、IBL、ImGui viewport、picking。
9. 最后讲不足和后续优化：RenderGraph、barrier 统一、异步上传/readback、多线程录制。

一句话版本：

> Kita 当前是一个 Vulkan + ImGui 的编辑器型渲染引擎，底层用 Vulkan 1.3 dynamic rendering 构建离屏视口，渲染链路采用 GBuffer + Deferred Lighting + IBL + Tonemap，并通过 RenderPass、SceneBindings、PipelineFactory、VulkanResourceFactory 把 pass、场景绑定、pipeline 缓存和资产到 runtime 资源的转换拆开。

---

## 11. 代码阅读建议

如果想快速理解当前底层渲染，建议按下面顺序读：

1. `engine/src/core/Application.cpp`
2. `engine/src/render/VulkanContext.h/.cpp`
3. `engine/src/render/VulkanRenderCommand.h/.cpp`
4. `engine/src/render/VulkanImage.h/.cpp`
5. `engine/src/render/VulkanRenderTarget.h/.cpp`
6. `engine/src/render/VulkanGraphicsPipeline.h/.cpp`
7. `engine/src/render/VulkanDescriptorSet.h/.cpp`
8. `engine/src/render/pass/RenderPass.h/.cpp`
9. `engine/src/render/pass/SceneBindings.h/.cpp`
10. `engine/src/render/deferred/BasePass.h/.cpp`
11. `engine/src/render/deferred/DeferredLightingPass.h/.cpp`
12. `engine/src/render/pipeline/PipelineFactory.h/.cpp`
13. `engine/src/render/VulkanMaterial.h/.cpp`
14. `engine/src/render/VulkanResourceFactory.h/.cpp`
15. `renderer/src/ui/viewport/EditorViewportSurface.h/.cpp`
16. `renderer/src/scene/EditorRenderer.h/.cpp`
17. `engine/src/render/ibl/IBLGenerator.h/.cpp`

---

## 12. 总结

Kita 当前已经不是简单的 Vulkan 初始化 demo，而是具备编辑器视口、资产系统、材质系统、pass 框架、延迟渲染、IBL、Tonemap 和 picking 的实时渲染引擎原型。

底层渲染架构最核心的关键词是：

- Vulkan 1.3
- Dynamic Rendering
- Offscreen Render Target
- GBuffer
- Deferred Lighting
- SceneBindings
- PipelineFactory
- PBR Material
- IBL
- ImGui Viewport
- Picking Readback

从面试角度看，这个项目可以重点体现三个能力：

1. 能把 Vulkan 复杂底层概念封装成可维护的引擎模块。
2. 能设计编辑器所需的离屏渲染和多 pass 管线。
3. 能理解现代实时渲染中的 GBuffer、PBR、IBL、HDR、Tonemapping 和资源同步问题。

