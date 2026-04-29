# Kita 引擎架构与渲染流程分析

## 一、项目概览

Kita 是一个基于 OpenGL + ImGui 的 Windows 桌面端游戏编辑器引擎，使用 Premake5 构建系统，C++17 标准，面向 Visual Studio 2022/2026。

项目采用 **引擎库 + 编辑器应用** 的经典分离架构：

```
Kita/
├── engine/                    # 引擎核心（静态库 lib）
│   └── src/
│       ├── core/              # 应用生命周期、窗口、层栈、日志、输入
│       ├── event/             # 事件系统
│       ├── render/            # 渲染抽象层 + 场景/网格/字体
│       ├── platform/          # 平台层（windows/*, opengl/*）
│       ├── component/         # ECS 组件系统（基于 EnTT）
│       ├── serialize/         # JSON 序列化
│       └── third_party/       # ImGui 胶水层
├── renderer/                  # 编辑器应用（可执行文件）
│   └── src/
│       ├── EditorApp.cpp      # 应用入口
│       ├── EditorLayer.cpp    # 主编编辑器层（DockSpace + 菜单栏）
│       ├── scene/             # 视口相机控制器
│       ├── ui/                # 场景层级面板、视口面板、内容浏览器
│       ├── file/              # 项目系统
│       └── utils/             # 文件对话框
└── premake/                   # Premake5 构建工具
```

---

## 二、底层框架架构

### 2.1 依赖关系图

```
┌──────────────────────────────────────────────────────┐
│                   Renderer (exe)                      │
│  EditorApp → EditorLayer → ViewportPanel/Hierarchy   │
└──────────────────────┬───────────────────────────────┘
                       │ 链接
┌──────────────────────▼───────────────────────────────┐
│                   Engine (lib)                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────┐   │
│  │   Core   │  │  Event   │  │     Render       │   │
│  │App/Window│  │Dispatch  │  │Renderer/Shader/   │   │
│  │Layer/Log │  │Keyboard  │  │Buffer/Texture/    │   │
│  │Input     │  │Mouse     │  │FrameBuffer/UBO    │   │
│  └──────────┘  └──────────┘  └────────┬─────────┘   │
│                                       │              │
│  ┌──────────┐  ┌──────────┐  ┌───────▼──────────┐  │
│  │Component │  │Serialize │  │    Platform       │  │
│  │ECS/EnTT  │  │JSON      │  │ OpenGL + GLFW     │  │
│  └──────────┘  └──────────┘  └──────────────────┘  │
└──────────────────────┬───────────────────────────────┘
                       │ 依赖
┌──────────────────────▼───────────────────────────────┐
│  第三方库                                             │
│  GLFW │ Glad │ ImGui │ ImGuizmo │ EnTT │ Assimp     │
│  spdlog │ GLM │ nlohmann_json │ stb_image           │
└──────────────────────────────────────────────────────┘
```

### 2.2 核心子系统

| 子系统 | 关键类 | 职责 |
|--------|--------|------|
| **应用生命周期** | `Application`, `EntryPoint` | 单例模式管理，`main()` 通过宏 `CreateApplication` 注入 |
| **窗口管理** | `Window`(抽象), `WindowsWindow` | GLFW 窗口创建、事件回调、SwapBuffers |
| **层栈系统** | `Layer`, `LayerStack` | 分层架构，支持普通层和覆盖层（Overlay），按栈序 `OnUpdate` / `OnImGuiRender` |
| **事件系统** | `Event`, `EventDispatcher` | 类型安全的事件分发，支持窗口/键盘/鼠标事件 |
| **渲染抽象** | `Renderer`, `RenderCommand`, `RendererAPI` | 命令模式：`RenderCommand` 是静态门面，委托给 `OpenGLRendererAPI` |
| **图形上下文** | `GraphicsContext`(抽象), `OpenGLContext` | GLFW + Glad 初始化，OpenGL 状态管理 |
| **ECS** | `Scene`, `Object`, `entt::registry` | 基于 EnTT，`Object` 是 `entt::entity` 的句柄封装 |
| **序列化** | `SceneSerializer`, `JsonUtils` | 场景 JSON 序列化（`.sce` 文件格式） |

### 2.3 应用入口与启动流程

```
main()
  │
  ├─ Kita::Log::Init()                    // spdlog 初始化
  ├─ Kita::CreateApplication(argc, argv)  // → EditorApp 构造函数
  │    ├─ Application::Application()
  │    │    ├─ InitWindow()               // GLFW 窗口创建 + 事件回调绑定
  │    │    │    └─ Window::Create() → WindowsWindow (GLFWwindow)
  │    │    ├─ InitImGuiLayer()           // 创建 ImGuiLayer 作为 Overlay
  │    │    │    ├─ ImGui::CreateContext() + Docking/Viewports 启用
  │    │    │    ├─ 暗色主题设置
  │    │    │    ├─ 字体加载 (Poppins + Fontello 图标)
  │    │    │    └─ ImGui_ImplGlfw_InitForOpenGL + ImGui_ImplOpenGL3_Init
  │    │    └─ InitRenderer()
  │    │         └─ Renderer::Init()
  │    │              ├─ UniformBuffer::Create × 3  // Camera(0), Light(1), Viewport(2)
  │    │              ├─ InitEditorGridData()        // 全屏三角形 VAO + Grid UBO(10)
  │    │              ├─ Gizmo::Init()               // Gizmo 点 VAO/VBO
  │    │              └─ ShaderLibrary::Load × 6     // 加载全部 GLSL Shader
  │    └─ PushLayer(new EditorLayer())   // 推入编辑层
  │
  └─ app->Run()
       │
       └─ MainLoop()  [每帧循环]
            ├─ for each Layer: OnUpdate(dt)         // 场景模拟 + 视口渲染
            ├─ ImGuiLayer::Begin()                   // NewFrame
            ├─ for each Layer: OnImGuiRender()       // ImGui 绘制
            ├─ ImGuiLayer::End()                     // RenderDrawData
            └─ m_Window->OnUpdate()                  // PollEvents + SwapBuffers
```

---

## 三、渲染管线详细分析

### 3.1 管线总览图

```
每帧渲染流程
═══════════════════════════════════════════════════════════════

  Application::MainLoop()
       │
       ▼
  ┌─────────────────────────────────────────────────────┐
  │  Phase 1: OnUpdate                                  │
  │  ┌───────────────────────────────────────────────┐  │
  │  │ EditorLayer::OnUpdate()                       │  │
  │  │   ├─ Scene::SimulateSceneEditor()             │  │
  │  │   │    └─ 遍历 LineRenderer，重建脏曲线 VAO    │  │
  │  │   └─ for each ViewportPanel::Render()  ◄──────┤  │
  │  │        │                                       │  │
  │  └────────┼───────────────────────────────────────┘  │
  └───────────┼──────────────────────────────────────────┘
              │
     ┌────────▼────────────────────────────────────────────┐
     │  SceneViewportPanel::Render()  (每个视口)            │
     │                                                      │
     │  ┌─────────────────────────────────────────────┐     │
     │  │  Pass 1: Picking (离屏渲染)                  │     │
     │  │  ┌───────────────────────────────────────┐  │     │
     │  │  │ m_PickingFrameBuffer->Bind()          │  │     │
     │  │  │   (RGBA16F + RED_INTEGER×2 + DEPTH)   │  │     │
     │  │  │ Renderer::BeginScene(VP, camPos, ...) │  │     │
     │  │  │   └─ 上传 UBO: Camera(0), Light(1),   │  │     │
     │  │  │      Viewport(2)                       │  │     │
     │  │  │ Clear + ClearIDBuffer × 2 (-1)        │  │     │
     │  │  │ Scene::RenderSceneEditor() ◄──────────┼──┼──┐  │
     │  │  │ Renderer::EndScene()                  │  │  │  │
     │  │  │ UnBind()                              │  │  │  │
     │  │  └───────────────────────────────────────┘  │  │  │
     │  └─────────────────────────────────────────────┘  │  │
     │                                                   │  │
     │  ┌─────────────────────────────────────────────┐  │  │
     │  │  Pass 2: MSAA 渲染                          │  │  │
     │  │  ┌───────────────────────────────────────┐  │  │  │
     │  │  │ m_SceneMSAAFrameBuffer->Bind()        │  │  │  │
     │  │  │   (RGBA16F + DEPTH, 4× MSAA)         │  │  │  │
     │  │  │ Renderer::BeginScene(VP, camPos, ...) │  │  │  │
     │  │  │ Clear                                 │  │  │  │
     │  │  │ Scene::RenderSceneEditor() ◄──────────┼──┼──┼──┤
     │  │  │ Renderer::EndScene()                  │  │  │  │
     │  │  │ UnBind()                              │  │  │  │
     │  │  └───────────────────────────────────────┘  │  │  │
     │  │                                             │  │  │
     │  │  ┌───────────────────────────────────────┐  │  │  │
     │  │  │ Blit: MSAA → Resolve                  │  │  │  │
     │  │  │ m_SceneMSAAFrameBuffer->BlitColorTo(  │  │  │  │
     │  │  │   m_SceneResolveFrameBuffer)          │  │  │  │
     │  │  │   (4× AA → 1×, GL_NEAREST)            │  │  │  │
     │  │  └───────────────────────────────────────┘  │  │  │
     │  └─────────────────────────────────────────────┘  │  │
     └────────────────────────────────────────────────────┘  │
              │                                              │
              ▼                                              │
  ┌─────────────────────────────────────────────────────┐    │
  │  Phase 2: OnImGuiRender                             │    │
  │  ┌───────────────────────────────────────────────┐  │    │
  │  │ ImGuiLayer::Begin()                           │  │    │
  │  │   ├─ ImGui_ImplOpenGL3_NewFrame()             │  │    │
  │  │   ├─ ImGui_ImplGlfw_NewFrame()                │  │    │
  │  │   ├─ ImGui::NewFrame()                        │  │    │
  │  │   └─ ImGuizmo::BeginFrame()                   │  │    │
  │  └───────────────────────────────────────────────┘  │    │
  │                                                      │    │
  │  ┌───────────────────────────────────────────────┐  │    │
  │  │ EditorLayer::OnImGuiRender()                  │  │    │
  │  │   ├─ DockSpace (全屏)                          │  │    │
  │  │   ├─ 菜单栏 (File/Save/Load, Window, ...)     │  │    │
  │  │   ├─ SceneHierarchyPanel::OnImGuiRender()     │  │    │
  │  │   ├─ ContentBrowserPanel::OnImGuiRender()     │  │    │
  │  │   └─ for each ViewportPanel::OnImGuiRender()  │  │    │
  │  │        ├─ ImGui::Image(ResolveTexID)          │  │    │
  │  │        └─ ImGuizmo::Manipulate()  (如果选中)   │  │    │
  │  └───────────────────────────────────────────────┘  │    │
  │                                                      │    │
  │  ┌───────────────────────────────────────────────┐  │    │
  │  │ ImGuiLayer::End()                             │  │    │
  │  │   ├─ ImGui::Render()                          │  │    │
  │  │   └─ ImGui_ImplOpenGL3_RenderDrawData()       │  │    │
  │  └───────────────────────────────────────────────┘  │    │
  └─────────────────────────────────────────────────────┘    │
              │                                              │
              ▼                                              │
  ┌─────────────────────────────────────────────────────┐    │
  │  m_Window->OnUpdate()                               │    │
  │    ├─ glfwPollEvents()                              │    │
  │    └─ glfwSwapBuffers()                             │    │
  └─────────────────────────────────────────────────────┘    │
              │                                              │
              ▼ 回到 MainLoop 顶部                            │
═════════════════════════════════════════════════════════════╝
```

### 3.2 Scene::RenderSceneEditor() 内部渲染顺序

```
Scene::RenderSceneEditor()
│
├─ 1. 渲染实体网格 (MeshRenderer)
│     遍历 entt::registry.group<Transform, MeshRenderer>
│     对每个实体:
│       ├─ 计算 model 矩阵 (Transform::GetTransformMatrix)
│       ├─ 设置 Shader Uniform: Model, id (entity handle)
│       ├─ 遍历 MeshRenderer 的子 Mesh 列表
│       └─ Renderer::Submit(mesh->GetVAO(), shader)
│            └─ RenderCommand::DrawIndexed(VAO, Shader)
│                 └─ glBindVertexArray → glDrawElements(GL_TRIANGLES, ...)
│
├─ 2. 渲染天空盒 (Skybox)
│     Renderer::DrawSkyBox(cubemap, slot=9)
│       ├─ SetDepthWrite(false), SetDepthTestMode(Lequal)
│       ├─ cubemap->Bind(slot)
│       ├─ skyboxShader->SetInt("SkyboxTex", slot)
│       └─ Renderer::Submit(FullScreenTriangle_VAO, skyboxShader)
│            └─ 全屏三角形 + 片元着色器采样 CubeMap
│
├─ 3. 渲染曲线 (LineRenderer)
│     遍历 entt::registry.view<Transform, LineRenderer>
│     对每个实体:
│       ├─ 设置 Uniform: Model, id, Color
│       ├─ Renderer::SubmitAsLine(curveVAO, lineShader, vertexCount, lineWidth)
│       │    └─ glLineWidth → glDrawArrays(GL_LINE_STRIP, ...)
│       ├─ 收集锚点控制点数据 → Gizmo::DrawPoints(points)
│       └─ Gizmo::FlushAllPoints(model, id)
│            └─ glEnable(GL_PROGRAM_POINT_SIZE) → glDrawArrays(GL_POINTS, ...)
│
└─ 4. 渲染编辑器网格 (Editor Grid)
      Renderer::DrawEditorGrids(settings)
        ├─ 上传 Grid UBO (binding=10): CellSize, MajorStep, Fade, 颜色
        ├─ SetCullMode(None), SetDepthWrite(false), SetBlend(true)
        ├─ SetColorAttachmentWriteMask({true, false, false})  // 只写 attachment 0
        └─ Renderer::Submit(FullScreenTriangle_VAO, gridShader)
             └─ 全屏三角形 → 片元着色器通过逆 VP 矩阵反算世界坐标绘制网格
```

### 3.3 UBO 布局与绑定点

| 绑定槽 | 名称 | 内容 | 大小 |
|--------|------|------|------|
| `binding=0` | CameraData | Matrix_V, Matrix_P, Matrix_VP, Matrix_I_V, Matrix_I_P, Matrix_I_VP, CameraPosWS | 6×mat4 + vec4 |
| `binding=1` | LightData | LightDirection, LightColor, LightIntensity | vec3 + vec4 + float |
| `binding=2` | ViewportData | ScreenSize | vec4 |
| `binding=10` | EditorGrid | CellSize, MajorStep, FadeStart/End, 线宽, 颜色 | 6×vec4 |

所有 UBO 使用 `std140` 布局，在所有 Shader 中共享。

### 3.4 多渲染目标 (MRT) 布局

Picking Pass 使用 MRT 同时输出到 3 个颜色附件：

| Attachment | 格式 | 用途 |
|------------|------|------|
| `location=0` | RGBA16F | 标准颜色输出 |
| `location=1` | RED_INTEGER (int) | 实体 ID（`entt::entity` 数值） |
| `location=2` | RED_INTEGER (int) | 控制点索引（曲线编辑用） |

鼠标拾取时，通过 `glReadPixels(GL_RED_INTEGER)` 读取 attachment 1 和 2 的值完成对象选择。

### 3.5 Shader 系统

所有 Shader 使用自定义单文件格式，通过 `#program vertex` / `#program fragment` 指令分隔着色器阶段：

```glsl
#program vertex
#version 450 core
// ... vertex shader code ...

#program fragment
#version 450 core
// ... fragment shader code ...
```

`OpenGLShader` 解析时按 `#program` 分割，分别编译顶点/片元着色器后链接。所有 Shader 使用 `#version 450 core`。

| Shader 文件 | 用途 | 顶点输入 |
|-------------|------|----------|
| `EditorDefaultShader.glsl` | 实体网格渲染 | Position/Normal/UV/Color → Lambert 漫反射 |
| `EditorLineShader.glsl` | 曲线线条渲染 | Position → 纯色输出 |
| `EditorGridShader.glsl` | 无限编辑器网格 | 全屏三角形 → 逆VP反算世界坐标 |
| `DefaultSkyBox.glsl` | 天空盒 | 全屏三角形 → CubeMap 采样 |
| `GizmoPoint.glsl` | Gizmo 控制点 | Position/Color/Radius/Index |
| `GizmoDiamond.glsl` | Gizmo 菱形点 | 同上 |

---

## 四、关键设计模式与架构特点

### 4.1 命令模式 (RenderCommand)

```
Renderer::Submit(VAO, Shader)
  └─ RenderCommand::DrawIndexed(VAO, Shader)     // 静态门面
       └─ s_RendererAPI->DrawIndexed(VAO, Shader) // 多态调用
            └─ OpenGLRendererAPI::DrawIndexed()    // OpenGL 实现
```

这种设计使得未来切换到其他图形 API（如 Vulkan/D3D12）时只需替换 `RendererAPI` 的实现。

### 4.2 层栈模式 (LayerStack)

```
LayerStack
├── Overlay: ImGuiLayer        ← 持久层，始终渲染
└── Layer:   EditorLayer       ← 业务层，按需推入/弹出
```

- `OnUpdate(dt)`: 从栈底到栈顶遍历（先 EditorLayer 后 ImGuiLayer）
- `OnImGuiRender()`: ImGuiLayer 提供 Begin/End 包装，EditorLayer 在之间绘制
- 事件分发: 从栈顶到栈底（反序），事件被消费后停止传递

### 4.3 ECS 组件系统 (基于 EnTT)

```
Scene (entt::registry)
├── entity_0
│   ├── Name: "sphere"
│   ├── IDComponent: UUID
│   ├── Transform: position/rotation/scale
│   └── MeshRenderer: mesh list + material
├── entity_1
│   ├── Name: "curve 1"
│   ├── Transform
│   └── LineRenderer: control points + curve VAO
└── ...
```

`Object` 是 `entt::entity` 的轻量句柄包装，提供 `AddComponent<T>()`, `GetComponent<T>()`, `HasComponent<T>()` 模板方法。

### 4.4 帧缓冲管线

```
  Scene Data (ECS)
       │
       ├──→ PickingFrameBuffer (1×, MRT: RGBA16F + RED_INT×2 + DEPTH)
       │    │   └─ 用于鼠标拾取 (glReadPixels)
       │
       └──→ SceneMSAAFrameBuffer (4× MSAA, RGBA16F + DEPTH)
            │   └─ 抗锯齿场景渲染
            │
            └── Blit (GL_NEAREST)
                 │
                 └──→ SceneResolveFrameBuffer (1×, RGBA16F + DEPTH)
                      │   └─ 作为 ImGui::Image 纹理显示
```

### 4.5 ImGuizmo 集成

视口面板中集成 ImGuizmo 实现 3D 变换操纵器：

- 快捷键 `W`→平移, `E`→旋转, `R`→缩放
- 按住 `Ctrl` 启用吸附（snap 0.1）
- 对象选中时使用 `WORLD` 空间模式操作 `modelMatrix`
- 控制点选中时使用 `TRANSLATE` 模式操作单个控制点相对于父模型的位置

---

## 五、数据流向总结

```
┌─────────────┐    ┌──────────────┐    ┌───────────────┐
│  GLFW 事件   │───▶│ EventDispatcher│───▶│  Layer::OnEvent│
│ (键盘/鼠标)  │    │ (类型安全分发) │    │ (视口相机控制) │
└─────────────┘    └──────────────┘    └───────────────┘
                                               │
                                               ▼
┌─────────────┐    ┌──────────────┐    ┌───────────────┐
│ Scene (ECS) │───▶│  Simulate()  │───▶│   Render()    │
│ Transform   │    │ (曲线重建)    │    │ (两次Pass渲染) │
│ MeshRenderer│    └──────────────┘    └───────┬───────┘
│ LineRenderer│                               │
└─────────────┘                               ▼
                                    ┌──────────────────┐
                                    │  Framebuffer 纹理  │
                                    │  (ResolveTexID)    │
                                    └────────┬─────────┘
                                             │
                                             ▼
                                    ┌──────────────────┐
                                    │ ImGui::Image()    │
                                    │ 显示在视口面板     │
                                    └──────────────────┘
```

此架构将一个场景的数据变化（Transform 变更、曲线编辑）自动反映到两遍离屏渲染（Picking + MSAA），最终通过 ImGui 纹理呈现在编辑器 UI 中，实现了编辑-预览的实时闭环。
