#pragma once
#include <EngineCore.h>
#include "scene/EditorRenderer.h"
#include "ui/InspectorPanel.h"
#include "ui/IBLPreviewPanel.h"
#include "ui/SceneRenderSettingsPanel.h"
#include "ui/SceneHierarchyPanel.h"
#include "ui/EditorSelectionContext.h"
#include "ui/ContentBrowserPanel.h"
#include "ui/asset/AssetEditorManager.h"
#include "ui/SvgIconAtlas.h"
#include "ui/ThumbnailCache.h"
#include "ui/UIColorPanel.h"
#include "ui/viewport/ViewportInstance.h"
namespace Kita{

	struct EditorPreviewTextureHandle
	{
		Ref<VulkanTexture> Texture = nullptr;
		ImTextureID TextureID = 0;
		uint32_t Width = 0;
		uint32_t Height = 0;
	};

	struct EditorIBLPreviewResources
	{
		std::array<EditorPreviewTextureHandle, 6> EnvironmentFaces{};
		std::array<EditorPreviewTextureHandle, 6> IrradianceFaces{};
		std::vector<std::array<EditorPreviewTextureHandle, 6>> PrefilterMipFaces;
		EditorPreviewTextureHandle BrdfLut;
	};

	class EditorLayer :public Layer{

	public:
		EditorLayer();


		virtual ~EditorLayer() = default;


		virtual void OnCreate()  override;
		virtual void OnUpdate(Timestep ts)  override;
		virtual void OnDestroy()  override;

		virtual void OnRender()  override;
		virtual void OnImGuiRender()  override;
		virtual void OnEvent(Event& event)  override;

		// 手动烘焙入口，真正执行会延迟到安全时机。
		void RequestBakeSceneIBL();
		bool BakeSceneIBL();

		// 当前 editor 持有的 IBL 结果，供 viewport renderer 读取
		const Ref<ImageBasedLighting>& GetBakedIBL() const { return m_IBLSource; }
	private:
		bool OnKeyPressed(KeyPressedEvent& event);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
		std::string BuildNextViewportWindowName();
		void AddViewportPanel(std::string windowName);
		void RemoveClosedViewportPanels();
		void EnsureMainDockLayout(ImGuiID dockspaceId, const ImVec2& dockspaceSize);
		void RenderTimeSystemPanel();
		void RenderIBLPreviewPanel();
		void RenderSceneRenderSettingsPanel();
		bool BuildIBLPreviewResources(EditorIBLPreviewResources& outResources);
		static void ReleaseIBLPreviewResources(EditorIBLPreviewResources& resources);


	private:
		Ref<Scene> m_Scene = nullptr;
		SceneSerializer  m_SceneSerializer;

		//ui
		SceneHierarchyPanel m_SceneHierarchyPanel;
		InspectorPanel m_InspectorPanel;
		ContentBrowserPanel m_ContentBrowserPanel;
		AssetEditorManager m_AssetEditorManager;
		UIColorPanel m_UIColorPanel;
		IBLPreviewPanel m_IBLPreviewPanel;
		SceneRenderSettingsPanel m_SceneRenderSettingsPanel;
		std::vector<ViewportInstance> m_SceneViewportPanels{};
		int32_t m_ActiveViewportIndex = -1;
		uint32_t m_NextViewportSerial = 1;
		bool m_ShowTimeSystemPanel = true;
		bool m_MainDockLayoutInitialized = false;
		ImGuiID m_MainDockSpaceId = 0;
		ImGuiID m_AssetEditorDockNodeId = 0;

		Unique<IBLGenerator> m_IBLGenerator = nullptr;
		Ref<ImageBasedLighting>  m_IBLSource = nullptr;
		EditorIBLPreviewResources m_IBLPreviewResources{};
		std::string m_IBLPreviewStatusText = "Ready.";
		bool m_PendingBakeSceneIBL = false;

		Ref<EditorSelectionContext> m_EditorSelectionContext = nullptr;
		Unique<VulkanResourceFactory> m_EditorVulkanResourceFactory = nullptr;
		Unique<PipelineFactory> m_PipelineFactory = nullptr;
		Unique<ThumbnailCache> m_ContentBrowserThumbnailCache = nullptr;
		Unique<SvgIconAtlas> m_ContentBrowserIconAtlas = nullptr;
	};
}
