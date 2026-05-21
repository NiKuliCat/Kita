#pragma once

#include <EngineCore.h>
#include <EngineRender.h>

namespace Kita {

	class IBLPreviewPanel
	{
	public:
		struct CubemapFaceView
		{
			ImTextureID TextureID = 0;
			uint32_t Width = 0;
			uint32_t Height = 0;
		};

		struct CubemapView
		{
			std::array<CubemapFaceView, 6> Faces{};
		};

		struct PreviewViewModel
		{
			bool HasBakedData = false;
			bool HasRuntimeIBL = false;
			uint32_t EnvironmentSize = 0;
			uint32_t IrradianceSize = 0;
			uint32_t PrefilterBaseSize = 0;
			uint32_t BrdfWidth = 0;
			uint32_t BrdfHeight = 0;
			size_t PrefilterMipCount = 0;
			std::string StatusText;
			CubemapView Environment;
			CubemapView Irradiance;
			std::vector<CubemapView> PrefilterMips;
			ImTextureID BrdfLutTextureID = 0;
		};

		void SetOpen(bool isOpen) { m_IsOpen = isOpen; }
		bool IsOpen() const { return m_IsOpen; }

		void OnImGuiRender(const PreviewViewModel& viewModel, const std::function<void()>& onBakeClicked);

	private:
		void DrawSummary(const PreviewViewModel& viewModel, const std::function<void()>& onBakeClicked);
		void DrawCubemapSection(const char* label, const CubemapView& cubemapView, uint32_t faceSize);
		void DrawPrefilterSection(const PreviewViewModel& viewModel);
		void DrawBrdfSection(const PreviewViewModel& viewModel);
		void DrawFaceCrossLayout(const CubemapView& cubemapView, float previewSize);
		static const char* GetFaceLabel(size_t faceIndex);

	private:
		bool m_IsOpen = true;
		int m_SelectedPrefilterMip = 0;
	};

}
