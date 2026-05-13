#include "renderer_pch.h"
#include "ViewportInstance.h"

namespace Kita {

	ViewportInstance::ViewportInstance(VulkanContext& context, const Ref<Scene>& scene, std::string windowName)
	{
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
				scene,
				*m_Panel->GetViewportCamera());

			m_Panel->SetDisplayTexture(m_Surface->GetTextureID());
		}
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

		const glm::vec2& desiredSize = m_Panel->GetDesiredViewportSize();
		m_Surface->EnsureSize(
			static_cast<uint32_t>(std::max(1.0f, desiredSize.x)),
			static_cast<uint32_t>(std::max(1.0f, desiredSize.y)));

		m_Panel->SetDisplayTexture(m_Surface->GetTextureID());
		m_Renderer->Render(m_Surface->GetRenderTarget());
	}

	void ViewportInstance::OnImGuiRender()
	{
		if (!m_Panel)
			return;

		if (m_Surface)
			m_Panel->SetDisplayTexture(m_Surface->GetTextureID());

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

}
