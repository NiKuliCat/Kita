#pragma once

#include <EngineCore.h>
#include "EditorSelectionContext.h"
#include "UIAttributeUtil.h"

namespace Kita {

	class InspectorPanel
	{
	public:
		InspectorPanel() = default;
		InspectorPanel(const Ref<EditorSelectionContext>& selectionContext)
			: m_SelectionContext(selectionContext) {}

		void SetSelectionContext(const Ref<EditorSelectionContext>& selectionContext) { m_SelectionContext = selectionContext; }
		void SetOpenAssetCallback(std::function<void(AssetHandle)> callback) { m_OpenAssetCallback = std::move(callback); }

		void OnImGuiRender();

	private:
		void DrawInspectorPanel();
		void DrawSelectedObject(Object& selectedObject);
		void DrawObjectHeader(Object& selectedObject);
		void DrawAddComponentMenu(Object& selectedObject);
		bool DrawPropertyGroupHeader(const char* label);

		void DrawTransformProperties(Transform& transform);
		void DrawStaticMeshProperties(MeshRenderer& meshRenderer);
		void DrawMaterialProperties(MeshRenderer& meshRenderer);
		void DrawLightProperties(LightComponent& lightComponent);

		void DrawVec3Row(const char* label, glm::vec3& value, float speed = 0.05f, const glm::vec3& defaultValue = glm::vec3(0.0f));
		void DrawFloatRow(const char* label, float& value, float speed = 0.05f, float minValue = 0.0f, float maxValue = 0.0f, float defaultValue = 0.0f);
		void DrawColorRow(const char* label, glm::vec4& value, const glm::vec4& defaultValue = glm::vec4(1.0f));
		void DrawStaticMeshSlotRow(const char* label, AssetHandle& meshHandle, const std::vector<AssetMetadata>& meshAssets, AssetHandle defaultValue = InvalidAssetHandle);
		void DrawMaterialSlotRow(const char* label, size_t slotIndex, AssetHandle& slotMaterialHandle, AssetHandle defaultMaterialHandle, const std::vector<AssetMetadata>& materialAssets);
		void OpenMaterialEditor(AssetHandle materialHandle);

	private:
		UIAttributeUtil::TableStyle m_TableStyle = UIAttributeUtil::CreateDefaultTableStyle();
		Ref<EditorSelectionContext> m_SelectionContext = nullptr;
		std::function<void(AssetHandle)> m_OpenAssetCallback = nullptr;
	};

}
