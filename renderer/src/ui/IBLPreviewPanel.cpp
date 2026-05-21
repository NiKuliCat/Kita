#include "renderer_pch.h"
#include "IBLPreviewPanel.h"

#include "imgui.h"
#include <imgui_internal.h>

namespace Kita {

	namespace
	{
		constexpr float kPreviewPadding = 8.0f;
		constexpr float kMinPreviewTileSize = 72.0f;
		constexpr float kMaxPreviewTileSize = 180.0f;
		constexpr float kCrossLabelOffset = 20.0f;
	}

	void IBLPreviewPanel::OnImGuiRender(const PreviewViewModel& viewModel, const std::function<void()>& onBakeClicked)
	{
		if (!m_IsOpen)
			return;

		ImGuiWindowClass viewportWindowClass{};
		viewportWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::SetNextWindowClass(&viewportWindowClass);

		if (!ImGui::Begin("IBL Preview", &m_IsOpen))
		{
			ImGui::End();
			return;
		}

		DrawSummary(viewModel, onBakeClicked);
		ImGui::Separator();

		if (!viewModel.HasBakedData)
		{
			ImGui::TextDisabled("No baked IBL preview data.");
			ImGui::End();
			return;
		}

		if (ImGui::BeginTabBar("##IBLPreviewTabs"))
		{
			if (ImGui::BeginTabItem("Environment"))
			{
				DrawCubemapSection("Environment Cubemap", viewModel.Environment, viewModel.EnvironmentSize);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Irradiance"))
			{
				DrawCubemapSection("Irradiance Cubemap", viewModel.Irradiance, viewModel.IrradianceSize);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Prefilter"))
			{
				DrawPrefilterSection(viewModel);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("BRDF LUT"))
			{
				DrawBrdfSection(viewModel);
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	void IBLPreviewPanel::DrawSummary(const PreviewViewModel& viewModel, const std::function<void()>& onBakeClicked)
	{
		if (ImGui::Button("Bake IBL"))
		{
			onBakeClicked();
		}

		ImGui::SameLine();
		ImGui::TextDisabled("%s", viewModel.StatusText.empty() ? "Ready." : viewModel.StatusText.c_str());

		ImGui::SeparatorText("Summary");
		ImGui::Text("Runtime IBL: %s", viewModel.HasRuntimeIBL ? "Ready" : "Not Ready");
		ImGui::Text("Environment: %u", viewModel.EnvironmentSize);
		ImGui::Text("Irradiance: %u", viewModel.IrradianceSize);
		ImGui::Text("Prefilter: %u (%zu mips)", viewModel.PrefilterBaseSize, viewModel.PrefilterMipCount);
		ImGui::Text("BRDF LUT: %u x %u", viewModel.BrdfWidth, viewModel.BrdfHeight);
	}

	void IBLPreviewPanel::DrawCubemapSection(const char* label, const CubemapView& cubemapView, uint32_t faceSize)
	{
		ImGui::SeparatorText(label);
		ImGui::Text("Face Size: %u x %u", faceSize, faceSize);
		DrawFaceCrossLayout(cubemapView, 0.0f);
	}

	void IBLPreviewPanel::DrawPrefilterSection(const PreviewViewModel& viewModel)
	{
		ImGui::SeparatorText("Specular Prefilter Cubemap");

		if (viewModel.PrefilterMips.empty())
		{
			ImGui::TextDisabled("No prefilter mip data.");
			return;
		}

		const int maxMipIndex = static_cast<int>(viewModel.PrefilterMips.size()) - 1;
		m_SelectedPrefilterMip = std::clamp(m_SelectedPrefilterMip, 0, maxMipIndex);
		ImGui::SliderInt("Mip", &m_SelectedPrefilterMip, 0, maxMipIndex);

		const uint32_t mipSize = std::max(1u, viewModel.PrefilterBaseSize >> static_cast<uint32_t>(m_SelectedPrefilterMip));
		const float roughness = maxMipIndex > 0
			? static_cast<float>(m_SelectedPrefilterMip) / static_cast<float>(maxMipIndex)
			: 0.0f;

		ImGui::Text("Mip Size: %u x %u", mipSize, mipSize);
		ImGui::Text("Approx Roughness: %.3f", roughness);
		DrawFaceCrossLayout(viewModel.PrefilterMips[static_cast<size_t>(m_SelectedPrefilterMip)], 0.0f);
	}

	void IBLPreviewPanel::DrawBrdfSection(const PreviewViewModel& viewModel)
	{
		ImGui::SeparatorText("Split-Sum BRDF LUT");

		if (!viewModel.BrdfLutTextureID)
		{
			ImGui::TextDisabled("No BRDF LUT preview texture.");
			return;
		}

		ImGui::Text("Resolution: %u x %u", viewModel.BrdfWidth, viewModel.BrdfHeight);
		const float availableWidth = ImGui::GetContentRegionAvail().x;
		const float previewSize = std::clamp(availableWidth, 180.0f, 360.0f);
		ImGui::Image(viewModel.BrdfLutTextureID, ImVec2(previewSize, previewSize));
	}

	void IBLPreviewPanel::DrawFaceCrossLayout(const CubemapView& cubemapView, float previewSize)
	{
		const float availableWidth = ImGui::GetContentRegionAvail().x;
		const float computedPreviewSize = previewSize > 0.0f
			? previewSize
			: std::clamp((availableWidth - kPreviewPadding * 3.0f) * 0.25f, kMinPreviewTileSize, kMaxPreviewTileSize);

		const float cellSize = computedPreviewSize;
		const float step = cellSize + kPreviewPadding;
		const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
		const ImVec2 canvasSize(step * 4.0f - kPreviewPadding, step * 3.0f - kPreviewPadding);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Border);
		const ImU32 labelColor = ImGui::GetColorU32(ImGuiCol_Text);
		const ImU32 dimLabelColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);

		struct FaceLayout
		{
			size_t FaceIndex;
			int GridX;
			int GridY;
		};

		// 十字展开布局：
		//         +Y
		// -X  +Z  +X  -Z
		//         -Y
		constexpr FaceLayout layouts[] =
		{
			{ 2, 1, 0 }, // +Y
			{ 1, 0, 1 }, // -X
			{ 4, 1, 1 }, // +Z
			{ 0, 2, 1 }, // +X
			{ 5, 3, 1 }, // -Z
			{ 3, 1, 2 }  // -Y
		};

		ImGui::Dummy(canvasSize);

		for (const FaceLayout& layout : layouts)
		{
			const CubemapFaceView& faceView = cubemapView.Faces[layout.FaceIndex];
			const ImVec2 faceMin(
				canvasOrigin.x + static_cast<float>(layout.GridX) * step,
				canvasOrigin.y + static_cast<float>(layout.GridY) * step);
			const ImVec2 faceMax(faceMin.x + cellSize, faceMin.y + cellSize);

			if (faceView.TextureID)
			{
				drawList->AddImage(faceView.TextureID, faceMin, faceMax);
			}
			else
			{
				drawList->AddRectFilled(faceMin, faceMax, ImGui::GetColorU32(ImGuiCol_FrameBg));
			}

			drawList->AddRect(faceMin, faceMax, borderColor);
			drawList->AddText(ImVec2(faceMin.x + 6.0f, faceMin.y + 4.0f), labelColor, GetFaceLabel(layout.FaceIndex));

			char resolutionText[32] = {};
			std::snprintf(resolutionText, sizeof(resolutionText), "%u x %u", faceView.Width, faceView.Height);
			drawList->AddText(
				ImVec2(faceMin.x + 6.0f, faceMax.y - kCrossLabelOffset),
				dimLabelColor,
				resolutionText);
		}
	}

	const char* IBLPreviewPanel::GetFaceLabel(size_t faceIndex)
	{
		static const char* labels[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
		return faceIndex < std::size(labels) ? labels[faceIndex] : "?";
	}

}
