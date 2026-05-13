#include "renderer_pch.h"
#include "ViewportInstance.h"

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
			return;
		}
	}

}
