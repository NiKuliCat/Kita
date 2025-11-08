#pragma once
#include "Event.h"
#include "KeyCode.h"
namespace Kita {

	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(const float x, const float y)
			:m_MouseX(x), m_MouseY(y) {}

		float GetX() const { return m_MouseX; }
		float GetY() const { return m_MouseY; }


		std::string ToString()const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventMouse | EventInput)
	private:
		float m_MouseX,m_MouseY;
	};


	class MouseButtonEvent : public Event
	{
	public:
		MouseCode GetMouseButton() const { return m_MouseCode; }

		EVENT_CLASS_CATEGORY(EventMouse | EventInput | EventMouseButton)

	protected:
		MouseButtonEvent(const MouseCode mouseCode)
			:m_MouseCode(mouseCode){ }

		MouseCode m_MouseCode;
	};

	class MouseButtonPressedButton : public MouseButtonEvent
	{
	public:
		MouseButtonPressedButton(const MouseCode mouseCode)
			:MouseButtonEvent(mouseCode){ }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_MouseCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	class MouseButtonReleaseEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleaseEvent(const MouseCode mouseCode)
			:MouseButtonEvent(mouseCode){ }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_MouseCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};

	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(const float offsetX,const float offsetY)
			:m_OffsetX(offsetX),m_OffsetY(offsetY){ }

		float GetOffsetX() const { return m_OffsetX; }
		float GetOffsetY() const { return m_OffsetY; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << GetOffsetX() << ", " << GetOffsetY();
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventMouse | EventInput)
	private:
		float m_OffsetX, m_OffsetY;
	};
}