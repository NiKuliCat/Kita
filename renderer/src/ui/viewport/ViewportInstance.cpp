#include "renderer_pch.h"
#include "ViewportInstance.h"
#include "component/Transform.h"
#include "core/Input.h"

namespace Kita {

	ViewportInstance::ViewportInstance(
		VulkanContext& context,
		VulkanResourceFactory& resFactory,
		PipelineFactory& pipelineFactory,
		const Ref<Scene>& scene,
		const Ref<EditorSelectionContext>& selectionContext,
		std::string windowName)
		: m_Context(&context)
		, m_SceneContext(scene)
		, m_SelectionContext(selectionContext)
	{
		m_PickRegistry = CreateUnique<EditorPickRegistry>();
		m_Panel = CreateUnique<EditorViewportPanel>(std::move(windowName));

		EditorViewportSurface::CreateInfo surfaceInfo{};
		if (m_Panel)
			surfaceInfo.Name = m_Panel->GetViewportCamera() ? "EditorViewportSurface" : "EditorViewportSurface";

		m_Surface = CreateUnique<EditorViewportSurface>(context, surfaceInfo);

		if (m_Panel && m_Surface && m_Panel->GetViewportCamera())
		{
			m_Renderer = CreateUnique<EditorRenderer>(
				context,
				m_Surface->GetRenderTarget(),
				m_Surface->GetPickingRenderTarget(),
				resFactory,
				pipelineFactory,
				scene,
				*m_Panel->GetViewportCamera(),
				*m_PickRegistry);
		}
	}

	ViewportInstance::~ViewportInstance()
	{
		if (m_Context)
			m_Context->WaitIdle();

		if (m_Renderer)
			m_Renderer->OnDestroy();

		m_Renderer.reset();
		m_Surface.reset();
		m_Panel.reset();
		m_PickRegistry.reset();
	}

	void ViewportInstance::OnUpdate(Timestep ts)
	{
		SyncViewportOverlaySettings();
		SyncCameraFocusTargetFromSelection();

		if (m_Panel &&
			m_Panel->IsWindowFocused() &&
			Input::IsKeyPressed(Key::F))
		{
			FocusCameraOnSelectedObject();
		}

		if (m_Panel)
			m_Panel->OnUpdata(ts);
	}

	void ViewportInstance::OnRender()
	{
		if (!m_Panel || !m_Surface || !m_Renderer)
			return;

		ProcessPendingPickRequest();

		const glm::vec2& desiredSize = m_Panel->GetDesiredViewportSize();
		m_Surface->EnsureSize(
			static_cast<uint32_t>(std::max(1.0f, desiredSize.x)),
			static_cast<uint32_t>(std::max(1.0f, desiredSize.y)));

		if (m_PickRegistry)
			m_PickRegistry->Clear();
		m_Renderer->Render(*m_Surface);
		m_Panel->SetDisplayTexture(m_Surface->GetTextureID());
	}

	void ViewportInstance::OnImGuiRender()
	{
		if (!m_Panel)
			return;

		m_Panel->OnImGuiRender();
	}

	void ViewportInstance::OnEvent(Event& event)
	{
		if (m_Panel)
			m_Panel->OnEvent(event);
	}

	void ViewportInstance::SetActive(bool isActive)
	{
		if (m_Panel)
			m_Panel->SetActive(isActive);
	}

	bool ViewportInstance::IsOpen() const
	{
		return m_Panel && m_Panel->IsOpen();
	}

	bool ViewportInstance::IsImageHovered() const
	{
		return m_Panel && m_Panel->IsImageHovered();
	}

	bool ViewportInstance::IsWindowFocused() const
	{
		return m_Panel && m_Panel->IsWindowFocused();
	}

	Object ViewportInstance::FindSceneObjectByUUID(UUID uuid) const
	{
		if (!m_SceneContext || uuid == UUID(0))
			return {};

		auto view = m_SceneContext->GetRegistry().view<IDComponent>();
		for (auto entity : view)
		{
			const IDComponent& idComponent = view.get<IDComponent>(entity);
			if (idComponent.ID != uuid)
				continue;

			return Object(entity, m_SceneContext.get(), "");
		}

		return {};
	}

	void ViewportInstance::ProcessPendingPickRequest()
	{
		if (!m_Panel || !m_Surface || !m_SelectionContext)
			return;

		if (!m_Panel->HasPendingPickRequest())
			return;

		const ViewportPickRequest request = m_Panel->ConsumePickRequest();
		const uint32_t pickId = m_Surface->ReadPickingPixel(request.PixelX, request.PixelY);
		ApplyPickResult(pickId);
	}

	void ViewportInstance::ApplyPickResult(uint32_t pickId)
	{
		if (!m_SelectionContext)
			return;

		EditorPickEntry entry{};
		if (!m_PickRegistry || !m_PickRegistry->TryGetEntry(pickId, entry))
		{
			m_SelectionContext->Clear();
			return;
		}

		switch (entry.SelectionType)
		{
		case EditorSelectionItemType::SceneObject:
		{
			Object selectedObject = FindSceneObjectByUUID(entry.SceneObjectUUID);
			if (!selectedObject)
			{
				m_SelectionContext->Clear();
				return;
			}

			m_SelectionContext->SetSelectionType(EditorSelectionItemType::SceneObject);
			m_SelectionContext->SetSelctionObject(selectedObject);
			SyncCameraFocusTargetFromSelection();
			return;
		}

		case EditorSelectionItemType::Asset:
		{
			if (!Asset::IsValidHandle(entry.SelectedAssetHandle))
			{
				m_SelectionContext->Clear();
				return;
			}

			m_SelectionContext->SetSelectionType(EditorSelectionItemType::Asset);
			m_SelectionContext->SetSelectionAsset(entry.SelectedAssetHandle);
			return;
		}

		case EditorSelectionItemType::None:
		default:
			m_SelectionContext->Clear();
			m_LastFocusSelectionUUID = 0;
			return;
		}
	}

	void ViewportInstance::SyncCameraFocusTargetFromSelection()
	{
		if (!m_Panel || !m_SelectionContext)
			return;

		if (m_SelectionContext->GetSelectionType() != EditorSelectionItemType::SceneObject)
		{
			m_LastFocusSelectionUUID = 0;
			return;
		}

		Object selectedObject = m_SelectionContext->GetSelectionItemHandle().m_SelectionObject;
		if (!selectedObject || !selectedObject.HasComponent<Transform>())
		{
			m_LastFocusSelectionUUID = 0;
			return;
		}

		const uint64_t selectedUUID = selectedObject.GetUUID();
		if (m_LastFocusSelectionUUID == selectedUUID)
			return;

		if (ViewportCamera* camera = m_Panel->GetViewportCamera())
			camera->SetFocusTarget(GetObjectFocusPoint(selectedObject));

		m_LastFocusSelectionUUID = selectedUUID;
	}

	void ViewportInstance::FocusCameraOnSelectedObject()
	{
		if (!m_Panel || !m_SelectionContext)
			return;

		if (m_SelectionContext->GetSelectionType() != EditorSelectionItemType::SceneObject)
			return;

		Object selectedObject = m_SelectionContext->GetSelectionItemHandle().m_SelectionObject;
		if (!selectedObject || !selectedObject.HasComponent<Transform>())
			return;

		if (ViewportCamera* camera = m_Panel->GetViewportCamera())
		{
			camera->FocusOnPoint(
				GetObjectFocusPoint(selectedObject),
				GetObjectFocusRadius(selectedObject));
		}
	}

	void ViewportInstance::SyncViewportOverlaySettings()
	{
		if (!m_Panel)
			return;

		const ViewportOverlaySettings& settings = m_Panel->GetOverlaySettings();

		if (ViewportCamera* camera = m_Panel->GetViewportCamera())
		{
			camera->SetFlightSpeedScale(settings.FlightSpeedScale);
			camera->SetRotationSpeedValue(settings.RotationSpeed);
			camera->SetZoomSpeedScale(settings.ZoomSpeedScale);
		}

		if (m_Renderer)
		{
			m_Renderer->SetGridEnabled(settings.ShowGrid);

			EditorGridPass::PushConstants pushConstants = m_Renderer->GetGridPushConstants();
			pushConstants.GridParams = glm::vec4(
				settings.MinorCellSize,
				std::max(settings.MajorCellSize, settings.MinorCellSize),
				settings.MinorLineWidth,
				std::max(settings.MajorLineWidth, settings.MinorLineWidth));
			pushConstants.FadeParams = glm::vec4(
				settings.FadeNear,
				std::max(settings.FadeFar, settings.FadeNear + 1.0f),
				settings.FadeAngleStart,
				settings.DepthBias);
			m_Renderer->SetGridPushConstants(pushConstants);
		}
	}

	glm::vec3 ViewportInstance::GetObjectFocusPoint(Object object)
	{
		return object.HasComponent<Transform>()
			? object.GetComponent<Transform>().GetPosition()
			: glm::vec3(0.0f);
	}

	float ViewportInstance::GetObjectFocusRadius(Object object)
	{
		if (!object.HasComponent<Transform>())
			return 1.0f;

		const glm::vec3 scale = object.GetComponent<Transform>().GetScale();
		return std::max(std::max(std::abs(scale.x), std::abs(scale.y)), std::abs(scale.z));
	}

}
