# Vulkan 渲染框架总览

本文档用于快速说明当前项目中 Vulkan 渲染部分的实际状态、现有设计、主要问题、后续演进方案，以及建议的改造优先级。

目标读者：

- 后续进入仓库继续开发渲染模块的开发者
- 需要快速理解当前 Vulkan 渲染链路的 AI agent
- 需要判断下一阶段是做前向、延迟、后处理、材质系统还是渲染图的人

本文档只描述当前仓库内已经存在或已明确规划的 Vulkan 渲染框架，不覆盖通用图形学理论。

---

## 1. 当前结论

当前仓库中的 Vulkan 接入已经不是“初始化演示”级别，而是已经形成了一条可运行的主渲染路径：

- 应用主循环已经完全走 Vulkan 帧流程
- 使用 `dynamic rendering`，不是传统 `VkRenderPass/VkFramebuffer`
- 编辑器视口已经走离屏渲染到 `VulkanRenderTarget`
- 离屏颜色附件再通过 ImGui 纹理显示在视口中
- 资产层已经能把 Shader / Mesh / Texture / Material 组织到 Vulkan runtime 资源

但当前框架仍然明显偏“单通路编辑器渲染原型”，还没有进入“正式可扩展渲染管线”的阶段。现状更准确地说是：

- 已经有 Vulkan 主路径
- 已经有基础资源抽象
- 已经有单个场景 pass 的执行链
- 还缺正式的 `RenderPass + SceneBindings + PipelineFactory + 可扩展 Material` 骨架

因此，后续推荐路线不是重做 Vulkan 接入，而是在现有基础上继续向可扩展渲染框架演进。

---

## 2. 当前渲染主流程

当前帧流程的实际入口在：

- `engine/src/core/Application.cpp`

主要顺序如下：

1. 初始化窗口
2. 初始化 `VulkanContext`
3. 初始化 ImGui Vulkan backend
4. 每帧调用 `VulkanContext::BeginFrame()`
5. 各 Layer 执行 `OnRender()`
6. ImGui 录制界面
7. 使用 swapchain image 进行最终 UI 合成
8. 调用 `VulkanContext::EndFrame()`

这意味着当前渲染框架的组织方式是：

- 场景渲染先写入离屏目标
- 编辑器 UI 和 viewport 展示在同一帧后续阶段完成
- 最终只把 ImGui 合成结果直接输出到 swapchain

这套流程对于编辑器场景预览、后处理接入、多视口扩展是合理的。

---

## 3. 当前核心模块

### 3.1 VulkanContext

相关文件：

- `engine/src/render/VulkanContext.h`
- `engine/src/render/VulkanContext.cpp`

职责：

- Vulkan 实例、设备、表面、交换链创建
- graphics/present queue 获取
- command pool / primary command buffer 创建
- 每帧同步对象管理
- `BeginFrame()` / `EndFrame()`
- swapchain resize / recreate

当前特点：

- API 版本目标为 Vulkan 1.3
- 已启用 `dynamic rendering`
- 每个 in-flight frame 使用一个 primary command buffer
- 当前所有渲染 pass 顺序录制到同一个 current command buffer

现状判断：

- 对当前阶段是足够的
- 对未来并行录制、多 command buffer、多 queue 来说还不够

---

### 3.2 VulkanRenderCommand

相关文件：

- `engine/src/render/VulkanRenderCommand.h`
- `engine/src/render/VulkanRenderCommand.cpp`

职责：

- swapchain image 的 begin/end rendering
- swapchain image layout transition
- viewport / scissor 设置
- geometry 绑定与 draw

当前特点：

- 仍然是偏底层 helper
- 对 scene pass 和 swapchain pass 都有支撑
- 目前还没有“按 pass 分类”的命令层抽象

现状判断：

- 可以保留
- 后续更适合作为底层命令辅助层，而不是上层渲染器核心

---

### 3.3 VulkanRenderTarget

相关文件：

- `engine/src/render/VulkanRenderTarget.h`
- `engine/src/render/VulkanRenderTarget.cpp`

职责：

- 管理离屏颜色附件与深度附件
- 支持多 color attachment
- 支持可采样颜色输出
- 支持 resize
- 负责 `vkCmdBeginRendering` / `vkCmdEndRendering`
- 负责 pass 前后 attachment layout transition

当前特点：

- 已支持多个 color attachment 的资源组织
- 已支持 depth attachment
- 已支持颜色附件作为 sampled image 暴露给后续 pass 或 ImGui

现状判断：

- 这是当前框架中最接近“正式渲染管线基础设施”的一层
- 后处理与延迟渲染都会直接依赖这一层
- 设计方向正确，应继续复用

---

### 3.4 VulkanGraphicsPipeline

相关文件：

- `engine/src/render/VulkanGraphicsPipeline.h`
- `engine/src/render/VulkanGraphicsPipeline.cpp`

职责：

- graphics pipeline layout 创建
- graphics pipeline 创建
- pipeline bind

当前特点：

- 已兼容 `dynamic rendering`
- 已支持 descriptor set layouts 和 push constant
- 当前输入依赖：
  - vertex shader
  - fragment shader
  - geometry layout
  - color format
  - depth format
  - topology / cull / blend / depth state

当前限制：

- 当前 pipeline 创建逻辑仍按单 color attachment 编写
- 还没有 pipeline cache / pipeline factory
- 还没有 shader variant / pass type / render state key 体系

现状判断：

- 可继续用
- 但必须引入 `PipelineKey + PipelineFactory` 才适合进入正式渲染框架阶段

---

### 3.5 VulkanRenderer

相关文件：

- `engine/src/render/VulkanRenderer.h`
- `engine/src/render/VulkanRenderer.cpp`

当前职责实际混合了三类内容：

1. Scene 级资源
   - `CameraUBO`
   - `DirectionLightUBO`
   - scene descriptor set
2. Scene begin/end
   - `BeginScene()`
   - `EndScene()`
3. Draw submit
   - `SubmitMesh()`
   - `PushObjectData()`

现状判断：

- 当前它帮助项目快速跑通了单个场景 pass
- 但它不是未来稳定架构中的理想中心类
- 一旦引入 `RenderPass` 层，这个类会被明显弱化

长期建议：

- 将 scene 级 UBO / descriptor set 拆到 `SceneBindings`
- 将 `BeginScene/EndScene` 职责转移给 `RenderPass`
- 将 `SubmitMesh/PushObjectData` 保留为 helper 或直接拆入 pass
- 最终可以移除 `VulkanRenderer` 作为核心对象

---

### 3.6 VulkanMaterial

相关文件：

- `engine/src/render/VulkanMaterial.h`
- `engine/src/render/VulkanMaterial.cpp`

当前职责：

- 保存 vertex shader / fragment shader 引用
- 保存 albedo texture
- 保存 base color
- 初始化 material descriptor set

当前限制非常明显：

- 只有 `AlbedoTexture`
- 只有 `BaseColor`
- descriptor set 只绑定一个 `combined image sampler`
- 没有参数表
- 没有贴图槽系统
- 没有材质状态描述
- 没有 shader 反射驱动的数据布局

现状判断：

- 当前材质系统只适合原型验证
- 不适合正式前向/延迟管线
- 必须尽早改成“可扩展参数结构”

---

### 3.7 VulkanResourceFactory

相关文件：

- `engine/src/render/VulkanResourceFactory.h`
- `engine/src/render/VulkanResourceFactory.cpp`

职责：

- 根据资产系统构建 runtime Vulkan 资源
- shader bundle 创建
- texture 创建
- material 创建
- mesh geometry 创建

优点：

- 已把 Asset 层和 Vulkan runtime 层打通
- 后续做 shader file、材质系统、pipeline factory 时仍可继续复用

不足：

- 目前 material 的构建仍建立在硬编码字段上
- 还没有和 pass / pipeline key / shader variant 体系结合

---

### 3.8 ShaderCompiler

相关文件：

- `engine/src/render/ShaderCompiler.h`
- `engine/src/render/ShaderCompiler.cpp`

当前能力：

- 基于 Slang 编译 shader
- 输出 SPIR-V
- 支持 include path、macro、profile、entry point

现状判断：

- 这是非常重要的基础
- 项目做“Unity 风格手写 shader 文件”已经有了起点
- 后续不应该绕开它重新做另一套 shader 编译通路

---

### 3.9 EditorRenderer 与 SceneViewportPanel

相关文件：

- `renderer/src/scene/EditorRenderer.h`
- `renderer/src/scene/EditorRenderer.cpp`
- `renderer/src/ui/SceneViewportPanel.h`
- `renderer/src/ui/SceneViewportPanel.cpp`

当前职责：

- `SceneViewportPanel`
  - 创建离屏 `VulkanRenderTarget`
  - 处理 viewport resize
  - 将 render target 的 sampled image 注册给 ImGui
- `EditorRenderer`
  - 读取场景
  - 构建 camera / main light 数据
  - 获取 mesh/material runtime 资源
  - 创建一个默认 opaque pipeline
  - 对所有对象执行 draw

现状判断：

- 当前编辑器视口渲染已经可用
- 但 `EditorRenderer` 仍是“单体式渲染流程”
- 后续应该拆成：
  - draw list 收集
  - pass 执行
  - pipeline factory 获取

---

### 3.10 ImGui Vulkan Backend

相关文件：

- `engine/src/third_party/imgui/imgui_layer.cpp`

当前特点：

- 使用 ImGui Vulkan backend
- 同样走 `dynamic rendering`
- 自建 ImGui descriptor pool
- 最终 UI 直接绘制到 swapchain image

现状判断：

- 设计方向正确
- 与编辑器视口离屏渲染配合良好
- 目前不是主要瓶颈

---

## 4. 当前已存在但未完成的 Pass 框架

当前仓库已经出现了初步的 pass 目录：

- `engine/src/render/pass/RenderPass.h`
- `engine/src/render/pass/RenderContext.h`
- `engine/src/render/pass/RenderDataStruct.h`

但实际状态仍是：

- `RenderPass.h` 只有空文件
- `RenderContext.h` 只有极简声明
- `RenderDataStruct.h` 已有初步类型定义
- 尚未与现有渲染主路径真正集成

这说明项目已经开始向 `RenderPass` 思路过渡，但目前仍处于“框架草图阶段”。

后续应继续沿这条路线推进，而不是重新换方向。

---

## 5. 当前框架的主要优点

### 5.1 已采用 dynamic rendering

这是当前项目 Vulkan 路线的一个正确选择。

优点：

- 更适合编辑器多视口
- 更适合离屏 RT + ImGui 混合
- 更适合后处理链和后续 render graph
- 不需要过早维护复杂的 `VkRenderPass/VkFramebuffer` 组合

结论：

- 后续不要回退到传统 Vulkan render pass 路线

---

### 5.2 离屏视口路径已经打通

当前 viewport 不是直接画 swapchain，而是：

- scene 渲染到 `VulkanRenderTarget`
- sampled color attachment 注册成 ImGui 纹理
- ImGui 中显示 viewport 图像

这对后续这些功能都非常有利：

- 后处理
- 多视口
- 选中高亮
- gizmo / overlay
- viewport debug 输出

---

### 5.3 资产层到 Vulkan runtime 已经打通

当前已经能从资产系统驱动：

- shader
- texture
- mesh
- material

这意味着后续开发重点不是“先让资源能进 Vulkan”，而是“让资源结构变得可扩展且可调度”。

---

## 6. 当前框架的主要问题

### 6.1 缺正式的 RenderPass 执行层

当前 `EditorRenderer` 自己承担了：

- 准备 scene data
- 创建 pipeline
- 遍历场景
- 绑定材质
- 调用 draw

这会导致：

- 难以拆 forward / deferred / postprocess
- 难以引入 render graph
- 难以形成统一 pass 输入输出协议

---

### 6.2 VulkanRenderer 职责混乱

当前 `VulkanRenderer` 混合了：

- scene bindings
- scene begin/end
- draw submit

这不利于后续的 pass 化和 render graph 化。

---

### 6.3 Material 系统过于固定

当前材质资产和运行时材质都建立在固定字段上：

- `ShaderHandle`
- `AlbedoTextureHandle`
- `BaseColor`

这不适合：

- normal / metallic / roughness / emissive
- 用户自定义参数
- 多贴图槽
- shader 文件暴露材质参数
- 将来 Shader Graph

---

### 6.4 缺 PipelineFactory / PipelineCache

当前 pipeline 仍然是“临时创建并长期手持”的模式。

问题：

- 没有统一 pipeline key
- 没有 pass + material + geometry + format 组合缓存
- 后续 forward / deferred / postprocess 会越来越混乱

---

### 6.5 Scene 级 GPU 资源没有独立层

当前 camera/light UBO 绑定在 `VulkanRenderer` 中，而不是独立的 `SceneBindings`。

这会导致：

- pass 之间难以共享 scene set
- postprocess / shadow / deferred lighting 的职责边界不清晰

---

### 6.6 当前 pipeline 还没有真正支撑 MRT

虽然 `VulkanRenderTarget` 已支持多个 color attachment，但 `VulkanGraphicsPipeline` 还没有真正按多 attachment 的 pipeline 创建路线组织。

这意味着：

- 延迟渲染的 GBuffer pass 还不能直接落地

---

## 7. 推荐的目标架构

后续推荐的方向不是立即做完整 render graph，而是向下面这个分层逐步收敛：

### 7.1 RenderGraph

职责：

- 管理 pass 顺序
- 管理资源依赖
- 管理 barrier / layout transition 策略
- 管理哪些 pass 在当前 frame 执行

注意：

- 这层不需要第一阶段就做完整
- 但设计时要给它留接口

---

### 7.2 RenderPass

职责：

- 一个完整的 dynamic rendering block
- begin / execute / end
- 绑定 render target
- 绑定 scene bindings
- 请求 pipeline
- 执行 draw calls 或 fullscreen pass

推荐的第一批 pass：

- `ForwardOpaquePass`
- `ForwardTransparentPass`
- `PostProcessPass`

后续再扩展：

- `GBufferPass`
- `DeferredLightingPass`
- `ShadowCasterPass`
- `UIPass`

---

### 7.3 RenderContext

职责：

- 提供本帧 CPU 侧输入
- camera / lights / viewport / draw list / frame index
- 供 pass 执行时读取

注意：

- 这层不应直接持有大量 GPU 资源生命周期

---

### 7.4 SceneBindings / FrameBindings

职责：

- 持有 scene 级 GPU 共享资源
- per-frame camera UBO
- per-frame light UBO
- scene descriptor set

这是后续必须补的一层。

---

### 7.5 PipelineFactory / PipelineCache

职责：

- 根据 `pass + shader variant + material state + geometry layout + RT format` 创建或复用 pipeline

建议引入：

- `PipelineKey`
- `MaterialPipelineDesc`
- `RenderPassDesc`
- `VertexLayoutHash`

---

## 8. 推荐渲染路线：先前向，后延迟

项目长期可以同时支持前向和延迟，但实际开发顺序建议明确如下：

### 第一阶段

先做前向渲染主路径：

- `ForwardOpaquePass`
- `ForwardTransparentPass`
- `PostProcessPass`

原因：

- 更适合先稳定 pass 层
- 更适合先稳定材质系统
- 更适合先稳定 shader 文件工作流
- 透明、编辑器视口、后处理都更自然

---

### 第二阶段

在前向稳定后，再接延迟：

- `GBufferPass`
- `DeferredLightingPass`
- 与 forward transparent 混合

原因：

- 延迟依赖 MRT、GBuffer 协议、lighting pass、材质输出规范
- 这些应该建立在已稳定的 pass / material / pipeline factory 之上

---

## 9. Shader 文件能力应尽早进入最小可用状态

当前项目已经有 Slang 编译到 SPIR-V 的基础，因此后续不建议继续把 shader 作为“固定资产二进制”看待，而应尽快让它进入“用户可编写 shader 文件”的状态。

推荐目标：

- 用户可以手写 shader 文件
- shader 资产可导入、编译、缓存
- shader 可声明用于哪些 pass
- material 可绑定 shader 暴露的参数和贴图槽

注意：

- 第一阶段不需要做完整 UE 式 Shader Graph
- 第一阶段优先做“Unity 风格手写 shader”

---

## 10. 需要修改的部分（按重要程度排序）

下面按优先级从高到低排序。

### P0：必须优先处理

#### 1. 正式落地 RenderPass 执行层

原因：

- 这是后续所有渲染演进的基础
- 不先把 `EditorRenderer` 的单体流程拆开，后面所有工作都会继续耦合

建议结果：

- `RenderPassDesc`
- `RenderPassContext`
- `SceneBindings`
- `ForwardOpaquePass`

---

#### 2. 将 scene 级 UBO / descriptor set 从 VulkanRenderer 中拆出

原因：

- `VulkanRenderer` 当前职责过重
- scene 级资源应作为共享 bindings 提供给多个 pass

建议结果：

- 新增 `SceneBindings`
- `VulkanRenderer` 被弱化或逐步淘汰

---

#### 3. 改造 MaterialAsset / VulkanMaterial 为可扩展结构

原因：

- 当前材质结构过于固定
- 再继续基于当前字段开发，会导致后续大规模返工

建议结果：

- 贴图槽系统
- 参数表系统
- 材质状态描述
- 与 shader 文件输入协议对齐

---

#### 4. 建立 PipelineFactory / PipelineCache

原因：

- pipeline 不能再由某个 renderer 临时创建并长期手持
- 后续 pass 化之后必须有统一的 pipeline 创建入口

建议结果：

- `PipelineKey`
- `MaterialPipelineDesc`
- `PipelineFactory`

---

### P1：高优先级

#### 5. 将 EditorRenderer 拆为“收集 + 执行”

原因：

- 目前 `EditorRenderer` 既收集场景数据，又创建 pipeline，又直接执行 draw
- 后续应拆成 draw list 收集和 pass 执行两阶段

---

#### 6. 支持完整的前向渲染路径

包括：

- opaque
- transparent
- postprocess

原因：

- 这是第一条正式渲染路径
- 同时也会验证 pass / material / pipeline factory 是否合理

---

#### 7. 打通最小可用 shader 文件工作流

包括：

- shader 文件导入
- 编译缓存
- 参数约定或反射
- 与 material 绑定

---

### P2：中优先级

#### 8. 扩展 VulkanGraphicsPipeline 支持多 color attachment

原因：

- 为后续延迟渲染做准备

---

#### 9. 引入 GBufferPass / DeferredLightingPass

前提：

- pass 系统稳定
- 材质系统支持更多 surface 输出
- pipeline factory 稳定

---

#### 10. 设计 RenderGraph 初版

建议第一版只解决：

- pass 顺序组织
- RT 输入输出关系
- 简单资源依赖

不要一开始就做复杂的自动资源别名和完整 barrier 推导。

---

### P3：后续增强

#### 11. Shader Graph / 节点材质系统

复杂度很高，建议非常靠后。

---

#### 12. 多线程 command buffer 录制

当前每帧一个 primary command buffer 的模式对现阶段完全够用，这项不是当前瓶颈。

---

#### 13. Async compute / async transfer

属于后续优化，不应前置。

---

## 11. 推荐开发顺序

推荐按以下顺序推进：

1. 完成 `RenderPass` 基础类型
2. 新增 `SceneBindings`
3. 将 `VulkanRenderer` 中的 scene UBO / descriptor set 拆出去
4. 做 `ForwardOpaquePass`
5. 改造 `MaterialAsset` / `VulkanMaterial` 为可扩展结构
6. 做 `PipelineFactory / PipelineCache`
7. 将 `EditorRenderer` 改造成 draw list 收集器
8. 补 `ForwardTransparentPass`
9. 补 `PostProcessPass`
10. 建立最小可用 shader 文件工作流
11. 扩展 MRT pipeline
12. 引入延迟渲染
13. 视情况引入 `RenderGraph`

---

## 12. 当前代码阅读顺序建议

如果要快速理解当前 Vulkan 渲染部分，建议按下面顺序读：

1. `engine/src/core/Application.cpp`
2. `engine/src/render/VulkanContext.*`
3. `engine/src/render/VulkanRenderCommand.*`
4. `engine/src/render/VulkanRenderTarget.*`
5. `engine/src/render/VulkanGraphicsPipeline.*`
6. `engine/src/render/VulkanDescriptorSet.*`
7. `engine/src/render/VulkanImage.*`
8. `engine/src/render/VulkanMaterial.*`
9. `engine/src/render/VulkanResourceFactory.*`
10. `renderer/src/ui/SceneViewportPanel.*`
11. `renderer/src/scene/EditorRenderer.*`
12. `engine/src/render/ShaderCompiler.*`

如果是继续开发新框架，则额外关注：

- `engine/src/render/pass/`

因为这里已经是后续演进方向的落点。

---

## 13. 最后的判断

当前项目的 Vulkan 渲染部分不需要推翻重做，方向总体是对的，尤其是：

- `dynamic rendering`
- 离屏 viewport
- Vulkan 资源封装
- shader 编译通路

真正需要做的是“从可运行原型，过渡到可扩展渲染框架”。

最关键的不是立即上延迟渲染，也不是立即做 Shader Graph，而是先把下面四件事做稳：

- `RenderPass`
- `SceneBindings`
- `可扩展 Material`
- `PipelineFactory`

这四件事完成后，前向、后处理、延迟、shader 文件、材质图系统都会变得顺理成章。

