# Kita 资产系统实现方案

## 一、现状诊断

### 1.1 当前资产的硬编码问题清单

| 文件 | 位置 | 硬编码内容 |
|------|------|-----------|
| `MeshRenderer.cpp:89` | `ProcessMesh()` | `"packages/shaders/EditorDefaultShader.glsl"` |
| `MeshRenderer.cpp:90` | `ProcessMesh()` | `"content/textures/test.jpg"` |
| `EditorLayer.cpp:27` | `OnCreate()` | `"content/models/Sphere.fbx"` |
| `EditorLayer.cpp:44-49` | `OnCreate()` | 天空盒 6 个 JPG 路径 |
| `SceneSerializer.cpp` | 序列化 | 不保存 MeshRenderer/LineRenderer 组件数据 |

### 1.2 现有可用基础设施

| 已有设施 | 可复用方式 |
|---------|-----------|
| `UUID` 类 (`engine/src/core/UUID.h`) | 作为资产主键 |
| `Project` 类 (`renderer/src/file/Project.h`) | 提供 content root 路径 |
| `ContentBrowserPanel` | UI 骨架，需扩展资产视图 |
| `Material` 类 | 改造为 `MaterialAsset` 的运行时表示 |
| `nlohmann_json` | 资产注册表持久化 |
| EnTT | 非必须，资产系统更适合用简单 map |

---

## 二、架构设计

### 2.1 核心类关系

```
AssetRegistry (单例, 持久化到 asset_registry.json)
│
│  管理 UUID → AssetMetadata 的映射
│  提供: Register / Lookup / LookupByPath / Unregister / Save / Load
│
├── AssetMetadata (值类型)
│   ├── UUID uuid
│   ├── AssetType type        // Texture, Mesh, Material, Scene
│   ├── std::filesystem::path relativePath  // 相对于 content root
│   ├── std::filesystem::path sourcePath    // 导入源文件路径(可选)
│   └── std::vector<UUID> dependencies     // 引用的其他资产
│
│
AssetCache (单例)
│
│  管理 UUID → Ref<Asset> 的运行时缓存
│  提供: Get<T>(UUID) → Ref<T> / Preload / Unload / IsLoaded
│
│
Asset (基类, 引擎层 engine/src/asset/)
│
├── UUID GetUUID()
├── AssetType GetType()
├── const std::string& GetName()
├── virtual bool Load(const AssetMetadata&) = 0   // 从磁盘加载 GPU/CPU 资源
├── virtual void Unload() = 0                     // 释放 GPU/CPU 资源
├── virtual bool Serialize(const std::filesystem::path&) = 0
├── virtual bool Deserialize(const std::filesystem::path&) = 0
│
├── TextureAsset
│   ├── 持有 Ref<Texture>
│   ├── .ktex 格式 (可选，初期直接加载源文件)
│   │
├── MeshAsset  
│   ├── 持有 std::vector<Ref<Mesh>>
│   ├── .kmesh 格式 (可选，初期直接加载 FBX)
│   │
├── MaterialAsset
│   ├── 持有 Ref<Material>
│   ├── .kmat 格式 (JSON)
│   └── 属性: baseColor, shader UUID, texture UUIDs
│
└── SceneAsset
    ├── 持有 Ref<Scene>
    └── .sce 格式 (现有格式扩展)
```

### 2.2 文件布局

```
engine/src/asset/                    # 引擎层新目录
├── Asset.h                          # AssetType 枚举 + Asset 基类
├── Asset.cpp
├── AssetRegistry.h / .cpp           # 资产注册表
├── AssetCache.h / .cpp              # 运行时缓存
├── AssetImporter.h / .cpp           # 导入管线基类
├── TextureAsset.h / .cpp            # 纹理资产
├── MeshAsset.h / .cpp               # 网格资产
├── MaterialAsset.h / .cpp           # 材质资产
└── SceneAsset.h / .cpp              # 场景资产（轻量包装）

renderer/src/ui/                     # 编辑器层新文件
├── AssetInspectorPanel.h / .cpp     # 资产属性面板
└── (ContentBrowserPanel 扩展)
```

### 2.3 材质资产文件格式 (.kmat)

```json
{
  "uuid": 17345678901234567890,
  "type": "Material",
  "version": 1,
  "name": "DefaultMaterial",
  "shader": 0,
  "properties": {
    "baseColor": [1.0, 1.0, 1.0, 1.0],
    "metallic": 0.0,
    "roughness": 0.5
  },
  "textures": {
    "albedo": 0,
    "normal": 0,
    "metallic": 0,
    "roughness": 0
  }
}
```

UUID 值为 0 表示"未设置/使用默认值"。Shader UUID 为 0 表示使用引擎内置默认 Shader。

### 2.4 资产注册表文件格式 (asset_registry.json)

```json
{
  "version": 1,
  "assets": {
    "17345678901234567890": {
      "type": "Texture",
      "relativePath": "textures/test.jpg",
      "sourcePath": "",
      "dependencies": []
    },
    "17345678901234567891": {
      "type": "Material",
      "relativePath": "materials/default.kmat",
      "sourcePath": "",
      "dependencies": ["17345678901234567890"]
    }
  }
}
```

---

## 三、分阶段实现计划

### Phase 1: 地基——AssetType + Asset 基类 + AssetRegistry

**目标**: 定义资产系统的核心数据结构和注册表，不改变任何渲染行为。

**新建文件:**

```
engine/src/asset/Asset.h             # AssetType 枚举, Asset 抽象基类
engine/src/asset/Asset.cpp           # Asset 默认实现
engine/src/asset/AssetRegistry.h     # 注册表声明
engine/src/asset/AssetRegistry.cpp   # 注册表实现
```

**`Asset.h` 核心定义:**

```cpp
namespace Kita {

enum class AssetType : uint8_t {
    None = 0,
    Texture,
    Mesh,
    Material,
    Scene,
    COUNT
};

struct AssetMetadata {
    UUID uuid{ 0 };
    AssetType type = AssetType::None;
    std::filesystem::path relativePath;    // 相对 content root
    std::filesystem::path sourcePath;      // 导入源（可选）
    std::vector<UUID> dependencies;
};

class Asset {
public:
    Asset(UUID uuid, AssetType type, std::string name)
        : m_UUID(uuid), m_Type(type), m_Name(std::move(name)) {}

    virtual ~Asset() = default;

    UUID GetUUID()     const { return m_UUID; }
    AssetType GetType() const { return m_Type; }
    const std::string& GetName() const { return m_Name; }

    virtual bool Load() = 0;
    virtual void Unload() = 0;
    virtual bool Save() = 0;

private:
    UUID m_UUID;
    AssetType m_Type;
    std::string m_Name;
};

} // namespace Kita
```

**`AssetRegistry` 核心定义:**

```cpp
namespace Kita {

class AssetRegistry {
public:
    static AssetRegistry& Get();

    void Initialize(const std::filesystem::path& contentRoot);
    void Shutdown();

    // 注册/注销
    UUID Register(AssetMetadata metadata);
    void Unregister(UUID uuid);

    // 查询
    const AssetMetadata* Lookup(UUID uuid) const;
    UUID LookupByPath(const std::filesystem::path& relativePath) const;
    std::vector<const AssetMetadata*> GetAllOfType(AssetType type) const;

    // 持久化
    bool Save();
    bool Load();

    const std::filesystem::path& GetContentRoot() const { return m_ContentRoot; }

private:
    std::filesystem::path m_ContentRoot;
    std::filesystem::path m_DatabasePath;  // content/asset_registry.json
    std::unordered_map<UUID, AssetMetadata> m_Assets;
    std::unordered_map<std::string, UUID> m_PathToUUID; // normalized relative path → UUID
    bool m_Initialized = false;
};

} // namespace Kita
```

**与 Project 类的集成点:**

在 `Project::Load()` 末尾追加：

```cpp
// 在 Project::Load() 中追加
AssetRegistry::Get().Initialize(s_ActiveProject->GetContentDirectory());
```

在 `Project::Unload()` 中追加：

```cpp
AssetRegistry::Get().Save();
AssetRegistry::Get().Shutdown();
```

**完成标志:** 编辑器启动时自动加载 `asset_registry.json`（如果存在），日志输出已注册的资产数量。

---

### Phase 2: AssetCache + TextureAsset

**目标**: 第一个具体资产类型落地，Content Browser 可以识别纹理资产。

**新建文件:**

```
engine/src/asset/AssetCache.h / .cpp
engine/src/asset/TextureAsset.h / .cpp
engine/src/asset/AssetImporter.h / .cpp
```

**`AssetCache` 核心定义:**

```cpp
namespace Kita {

class AssetCache {
public:
    static AssetCache& Get();

    template<typename T>
    Ref<T> Get(UUID uuid)
    {
        static_assert(std::is_base_of_v<Asset, T>);

        // 已加载，直接返回
        auto it = m_Cache.find(uuid);
        if (it != m_Cache.end())
            return std::static_pointer_cast<T>(it->second);

        // 查注册表获取路径
        auto* meta = AssetRegistry::Get().Lookup(uuid);
        if (!meta) return nullptr;

        // 创建并加载
        auto asset = CreateRef<T>(uuid, meta->relativePath);
        if (!asset->Load())
            return nullptr;

        m_Cache[uuid] = asset;
        return asset;
    }

    void Unload(UUID uuid);
    bool IsLoaded(UUID uuid) const;
    void Clear();

private:
    std::unordered_map<UUID, Ref<Asset>> m_Cache;
};

} // namespace Kita
```

**`TextureAsset`:**

```cpp
namespace Kita {

class TextureAsset : public Asset {
public:
    TextureAsset(UUID uuid, const std::filesystem::path& relativePath);

    bool Load() override;    // stb_image 加载 → 创建 GPU Texture
    void Unload() override;  // 释放 GPU Texture
    bool Save() override;    // Texture 通常不需要 save（源文件即权威）

    Ref<Texture> GetTexture() const { return m_Texture; }

private:
    Ref<Texture> m_Texture;
};

} // namespace Kita
```

**`AssetImporter` (导入器框架):**

```cpp
namespace Kita {

struct ImportResult {
    UUID uuid;
    AssetMetadata metadata;
    bool success = false;
};

class AssetImporterBase {
public:
    virtual ~AssetImporterBase() = default;
    virtual AssetType GetType() const = 0;
    virtual ImportResult Import(const std::filesystem::path& sourcePath) = 0;
};

class AssetImporterRegistry {
public:
    static AssetImporterRegistry& Get();
    void RegisterImporter(std::unique_ptr<AssetImporterBase> importer);
    ImportResult Import(const std::filesystem::path& sourcePath); // 自动匹配类型

private:
    std::vector<std::unique_ptr<AssetImporterBase>> m_Importers;
};

} // namespace Kita
```

请注意，这个阶段实际的 `TextureImporter` 可以放在 `renderer/src/` 下（编辑器层），因为导入行为是编辑器功能，引擎层只需要 Asset 基类和注册表。

**ContentBrowserPanel 修改点:**

在 `DrawDirectoryContents()` 中，每个文件条目增加逻辑：
- 查询 `AssetRegistry::LookupByPath()` 判断是否为已注册资产
- 已注册资产显示对应类型图标（纹理→图片图标，材质→调色板图标）
- 右键菜单增加 "Import Asset" / "Reimport" 选项

**完成标志:** 将一张 JPG 放入 content/textures/，右键 Import，注册表更新，Content Browser 显示带纹理图标的条目。

---

### Phase 3: MaterialAsset —— 材质作为资产

**目标**: 材质成为独立资产，告别 MeshRenderer 中硬编码 Shader/Texture 路径。

**新建文件:**

```
engine/src/asset/MaterialAsset.h / .cpp    # 材质资产类
renderer/src/ui/MaterialEditorPanel.h/cpp  # 材质编辑器面板（可选，可后续）
```

**`MaterialAsset` 核心定义:**

```cpp
namespace Kita {

struct MaterialPropertyBlock {
    glm::vec4 baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float metallic = 0.0f;
    float roughness = 0.5f;

    UUID albedoTexture = 0;
    UUID normalTexture = 0;
    UUID metallicTexture = 0;
    UUID roughnessTexture = 0;
};

class MaterialAsset : public Asset {
public:
    MaterialAsset(UUID uuid, const std::filesystem::path& relativePath);

    bool Load() override;    // 解析 .kmat JSON → 设置 MaterialPropertyBlock
    void Unload() override;  // 释放 Material 引用
    bool Save() override;    // 序列化到 .kmat JSON

    // 运行时接口 —— 替换当前 Material 类的职责
    void Bind();             // Shader + 纹理绑定

    MaterialPropertyBlock& GetProperties() { return m_Properties; }
    Ref<Material> GetMaterial() const { return m_Material; }

private:
    MaterialPropertyBlock m_Properties;
    Ref<Material> m_Material; // 当前的运行时 Material
};

} // namespace Kita
```

**MeshRenderer 修改——这是关键重构:**

当前 `MeshRenderer::ProcessMesh()` 硬编码了 Shader 和纹理。需要改为：

```cpp
// 修改前 (MeshRenderer.cpp:89-90)
mat->SetShader("packages/shaders/EditorDefaultShader.glsl");
mat->SetAlbedoTexture("content/textures/test.jpg");

// 修改后
void MeshRenderer::SetMaterialAsset(UUID materialAssetUUID)
{
    m_MaterialAssetUUID = materialAssetUUID;
}

void MeshRenderer::BindMaterial()
{
    auto materialAsset = AssetCache::Get().Get<MaterialAsset>(m_MaterialAssetUUID);
    if (materialAsset)
        materialAsset->Bind();
}
```

`MeshRenderer` 新增字段：`UUID m_MaterialAssetUUID = 0;`

同时，`EditorLayer::OnCreate()` 中创建实体的代码改为：

```cpp
// 修改前
meshrenderer.LoadMeshs("content/models/Sphere.fbx");

// 修改后
// 1. 先创建/获取 MeshAsset
UUID meshUUID = AssetRegistry::Get().LookupByPath("models/Sphere.fbx");
if (meshUUID == 0) {
    // 如果未注册，导入它
    auto result = AssetImporterRegistry::Get().Import("content/models/Sphere.fbx");
    meshUUID = result.uuid;
}
meshrenderer.SetMeshAsset(meshUUID);

// 2. 创建/获取 MaterialAsset
UUID materialUUID = AssetRegistry::Get().LookupByPath("materials/default.kmat");
if (materialUUID == 0) {
    // 创建默认材质
    auto materialAsset = CreateRef<MaterialAsset>(UUID(), "materials/default.kmat");
    materialAsset->GetProperties().baseColor = {1,1,1,1};
    materialAsset->Save();
    UUID uuid = materialAsset->GetUUID();
    AssetRegistry::Get().Register({ uuid, AssetType::Material, "materials/default.kmat" });
    materialUUID = uuid;
}
meshrenderer.SetMaterialAsset(materialUUID);
```

实际上，可以封装一个辅助函数简化这个流程。

**完成标志:** 创建 .kmat 文件 → 在 Content Browser 中可见 → 场景中的物体引用材质 UUID → 修改 .kmat 的 baseColor 后场景中反映变化。

---

### Phase 4: MeshAsset —— 网格作为资产

**目标**: FBX 等原始模型文件通过导入管线生成 MeshAsset，解耦文件格式与运行时。

**新建文件:**

```
engine/src/asset/MeshAsset.h / .cpp
renderer/src/importers/MeshImporter.h / .cpp   # 编辑器层导入器
```

**`MeshAsset` 核心定义:**

```cpp
namespace Kita {

struct SubMeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

class MeshAsset : public Asset {
public:
    MeshAsset(UUID uuid, const std::filesystem::path& relativePath);

    bool Load() override;    // 加载 .kmesh 或 FBX → 创建 GPU VAO
    void Unload() override;  // 释放 GPU 资源

    const std::vector<Ref<Mesh>>& GetSubMeshes() const { return m_SubMeshes; }
    size_t GetSubMeshCount() const { return m_SubMeshes.size(); }
    BoundingBox GetBoundingBox() const { return m_BoundingBox; }

    bool Save() override;    // 可选：将 Assimp 解析结果存为 .kmesh 二进制快照

private:
    std::vector<Ref<Mesh>> m_SubMeshes;
    BoundingBox m_BoundingBox;
    std::vector<SubMeshData> m_SourceData;  // CPU 端数据（导入时填充）
};

} // namespace Kita
```

初期 `.kmesh` 可以不做——`Load()` 直接用 Assimp 解析 FBX，后续再加 .kmesh 二进制缓存加速加载。

**`MeshRenderer` 进一步修改:**

```cpp
class MeshRenderer {
public:
    void SetMeshAsset(UUID uuid) { m_MeshAssetUUID = uuid; }
    UUID GetMeshAssetUUID() const { return m_MeshAssetUUID; }

    const std::vector<Ref<Mesh>>& GetMeshs() const {
        auto asset = AssetCache::Get().Get<MeshAsset>(m_MeshAssetUUID);
        return asset ? asset->GetSubMeshes() : m_EmptyMeshes;
    }

private:
    UUID m_MeshAssetUUID = 0;
    UUID m_MaterialAssetUUID = 0;
    static const std::vector<Ref<Mesh>> m_EmptyMeshes;
};
```

**完成标志:** FBX 文件拖入 Content Browser → 自动导入为 MeshAsset → 拖拽 MeshAsset 到 Hierarchy 面板创建实体 → 实体正确渲染。

---

### Phase 5: SceneAsset + 序列化完善 + 场景引用资产

**目标**: 场景文件完整序列化所有组件数据，场景中所有引用走 UUID。

**SceneSerializer 扩展:**

序列化 MeshRenderer：

```cpp
void SceneSerializer::SerializeObject(json& objectJson, Object object) {
    objectJson["uuid"] = JsonUtils::SerializeUUID(object.GetUUID());
    objectJson["name"] = object.GetName();
    objectJson["transform"] = JsonUtils::SerializeTransform(object.GetComponent<Transform>());

    // 新增: MeshRenderer
    if (object.HasComponent<MeshRenderer>()) {
        auto& mr = object.GetComponent<MeshRenderer>();
        json mrJson;
        mrJson["meshAsset"] = (uint64_t)mr.GetMeshAssetUUID();
        mrJson["materialAsset"] = (uint64_t)mr.GetMaterialAssetUUID();
        objectJson["meshRenderer"] = mrJson;
    }

    // 新增: LineRenderer
    if (object.HasComponent<LineRenderer>()) {
        auto& lr = object.GetComponent<LineRenderer>();
        json lrJson;
        lrJson["lineWidth"] = lr.GetLineWidth();
        lrJson["lineColor"] = { lr.GetLineColor().r, lr.GetLineColor().g,
                                 lr.GetLineColor().b, lr.GetLineColor().a };
        // 序列化控制点
        json points = json::array();
        for (const auto& cp : lr.GetControlPoints()) {
            points.push_back({
                {"id", cp.id},
                {"position", {cp.position.x, cp.position.y, cp.position.z}},
                {"color", {cp.color.r, cp.color.g, cp.color.b, cp.color.a}}
            });
        }
        lrJson["controlPoints"] = points;
        objectJson["lineRenderer"] = lrJson;
    }
}
```

**SceneAsset:**

```cpp
namespace Kita {

class SceneAsset : public Asset {
public:
    SceneAsset(UUID uuid, const std::filesystem::path& relativePath);

    bool Load() override;    // 调用 SceneSerializer::Deserialize()
    void Unload() override;  // 释放 Scene
    bool Save() override;    // 调用 SceneSerializer::Serialize()

    Ref<Scene> GetScene() const { return m_Scene; }

private:
    Ref<Scene> m_Scene;
};

} // namespace Kita
```

**完成标志:** Save Scene → .sce 文件包含完整的 MeshRenderer/LineRenderer 数据（UUID 引用）→ Load Scene → 场景完全还原。

---

### Phase 6: 集成与打磨

这是锦上添花的阶段，包括但不限于：

- Content Browser 中拖拽 MeshAsset 到 Viewport → 创建实体
- Content Browser 中拖拽 MaterialAsset 到 Viewport 中的物体 → 赋值材质
- Asset Inspector 面板：选中 Content Browser 中的资产后显示属性
- 缩略图生成（渲染资产到小 FBO → 保存为缓存纹理）
- 热重载：源文件变更时自动重新导入
- 未引用资产检测和清理

---

## 四、实现顺序依赖图

```
Phase 1: AssetType + Asset + AssetRegistry
   │
   ▼
Phase 2: AssetCache + TextureAsset + Importer 框架
   │
   ▼
Phase 3: MaterialAsset + MeshRenderer 重构
   │
   ▼
Phase 4: MeshAsset + FBX 导入器
   │
   ▼
Phase 5: SceneSerializer 扩展 + SceneAsset
   │
   ▼
Phase 6: 编辑器交互（拖拽、面板、缩略图）
```

每个 Phase 完成后都可以独立编译运行，不会破坏现有功能——Phase 1 只添加代码不做行为变更，Phase 2 起逐步替换硬编码路径。

---

## 五、关键风险与对策

| 风险 | 对策 |
|------|------|
| 大量硬编码路径分散在各处，改动容易遗漏 | Phase 1-2 不删任何旧代码，新系统与旧系统并行；Phase 3-4 逐处替换，每处替换后验证 |
| Assimp 加载的 FBX 每次都要重新解析，性能差 | Phase 4 初期保持和现在一致（Assimp 实时加载），后续加 .kmesh 二进制缓存 |
| 资产 UUID 引用断裂（文件被移动/删除） | 资产注册表通过相对路径建立反向索引；缺失资产在日志中警告而非崩溃，显示粉红色替代纹理 |
| 现有 .sce 文件与新格式不兼容 | 加 version 字段；Phase 5 迁移时旧文件自动补全缺失字段（meshAsset=0 表示未设置） |

---

## 六、第一个 PR 的建议范围

一个合理的最小可合并 PR 应该是 **Phase 1 + Phase 2 的前半部分**：

1. `Asset.h/cpp` — 枚举 + 基类
2. `AssetRegistry.h/cpp` — 注册表 + asset_registry.json 读写
3. `AssetCache.h/cpp` — 运行时缓存
4. `AssetImporter.h/cpp` — 导入器框架（不含具体导入器）
5. `TextureAsset.h/cpp` — 纹理资产（第一个具体类型）
6. 在 `Project::Load()` 中接入 AssetRegistry 初始化
7. 单元验证：启动编辑器，手动调用 `AssetRegistry::Register()` 注册一张纹理，日志确认

大约 8-10 个新文件，对现有代码的修改仅限于 `Project.cpp` 中加 3-4 行初始化调用。
