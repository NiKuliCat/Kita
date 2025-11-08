#include "kita_pch.h"
#include "Layer.h"
namespace Kita {

	Layer::Layer(const std::string& name)
		:m_name(name)
	{
	}

	LayerStack::LayerStack()
	{
	}

	LayerStack::~LayerStack()
	{
		for (Layer* layer : m_Layer)
		{
			delete layer;
		}
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		m_Layer.emplace(m_Layer.begin() + m_LayerInsertIndex, layer);
		m_LayerInsertIndex++;
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::find(m_Layer.begin(), m_Layer.end(), layer);
		if (it != m_Layer.end())
		{
			m_Layer.erase(it);
			m_LayerInsertIndex--;
		}
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		m_Layer.push_back(overlay);
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(m_Layer.begin(), m_Layer.end(), overlay);
		if (it != m_Layer.end())
		{
			m_Layer.erase(it);
		}
	}


}