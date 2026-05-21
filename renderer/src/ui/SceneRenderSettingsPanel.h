#pragma once

#include <EngineCore.h>

#include "UIAttributeUtil.h"

namespace Kita {

	class SceneRenderSettingsPanel
	{
	public:
		SceneRenderSettingsPanel() = default;
		explicit SceneRenderSettingsPanel(const Ref<Scene>& scene)
			: m_Scene(scene) {}

		void SetScene(const Ref<Scene>& scene) { m_Scene = scene; }
		void SetOpen(bool isOpen) { m_IsOpen = isOpen; }
		bool IsOpen() const { return m_IsOpen; }
		void SetBakeCallback(std::function<void()> callback) { m_BakeCallback = std::move(callback); }
		void SetStatusText(std::string statusText) { m_StatusText = std::move(statusText); }

		void OnImGuiRender();

	private:
		void DrawEnvironmentSection(SceneRenderSettings& settings);
		void DrawTextureSourceRow(SceneRenderSettings& settings, const std::vector<AssetMetadata>& textureAssets);
		void DrawSourceTypeRow(SceneRenderSettings& settings);
		void DrawCubeSizeRow(const char* label, uint32_t& value, uint32_t defaultValue, uint32_t minValue, uint32_t maxValue);
		void DrawFloatRow(const char* label, float& value, float speed, float minValue, float maxValue, float defaultValue);

	private:
		UIAttributeUtil::TableStyle m_TableStyle = UIAttributeUtil::CreateDefaultTableStyle();
		Ref<Scene> m_Scene = nullptr;
		std::function<void()> m_BakeCallback = nullptr;
		std::string m_StatusText = "Ready.";
		bool m_IsOpen = true;
	};

}
