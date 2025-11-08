#pragma once
#include "Event.h"
#include "KeyCode.h"

namespace Kita {


	class KeyBoardEvent : public Event
	{
	public:
		KeyCode GetKeyCode() const { return m_KeyCode; }

		//重写获取事件类型 GetCategoryFlags()     
		EVENT_CLASS_CATEGORY(EventKeyboard | EventInput)
	protected:
		KeyCode m_KeyCode;


		KeyBoardEvent(const KeyCode keycode)
			:m_KeyCode(keycode) {
		}
	};

	class KeyPressedEvent : public KeyBoardEvent
	{
	public:
		KeyPressedEvent(const KeyCode keyCode, bool isRepeat = false)
			:KeyBoardEvent(keyCode), m_IsRepeat(isRepeat) {
		}
		bool IsRepeat() { return m_IsRepeat; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent:" << m_KeyCode << "(repeat = " << m_IsRepeat << ")";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)
	private:
		bool m_IsRepeat;
	};

	class KeyReleasedEvent : public KeyBoardEvent
	{
	public:
		KeyReleasedEvent(const KeyCode keyCode)
			:KeyBoardEvent(keyCode){ }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};

	class KeyTypedEvent : public KeyBoardEvent
	{
	public:
		KeyTypedEvent(const KeyCode keyCode)
			:KeyBoardEvent(keyCode){ }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyTypedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};

}