#pragma once

#include "PreviewSceneRenderer.h"

namespace Kita {

	// 资产编辑器内嵌的实时 LookDev 小视口。
	class LookDevPreviewViewport
	{
	public:
		enum class Mode : uint8_t
		{
			None = 0,
			MaterialDefinition,
			Material,
			Cubemap
		};

	public:
		LookDevPreviewViewport() = default;
		explicit LookDevPreviewViewport(PreviewSceneRenderer* previewRenderer);
		~LookDevPreviewViewport();

		void SetPreviewSceneRenderer(PreviewSceneRenderer* previewRenderer) { m_PreviewRenderer = previewRenderer; }
		void SetMaterialDefinitionAsset(AssetHandle handle);
		void SetMaterialAsset(AssetHandle handle);
		void SetCubemapAsset(AssetHandle handle);
		void ClearMode();

		void OnRender();
		void Draw(const char* id, const ImVec2& size);

		bool HasValidOutput() const;
		Mode GetMode() const { return m_Mode; }
		AssetHandle GetAssetHandle() const { return m_AssetHandle; }

	private:
		void EnsureSurface(uint32_t width, uint32_t height);
		void HandleInteraction();

	private:
		PreviewSceneRenderer* m_PreviewRenderer = nullptr;
		Unique<EditorViewportSurface> m_Surface = nullptr;
		PreviewOrbitCameraState m_CameraState{};
		Mode m_Mode = Mode::None;
		AssetHandle m_AssetHandle = InvalidAssetHandle;
		ImVec2 m_LastDrawSize = ImVec2(256.0f, 256.0f);
		std::string m_SurfaceName = "LookDevPreviewViewport";
	};

}
