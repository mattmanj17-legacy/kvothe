#pragma once

#include <vector>
using std::vector;

template<typename T>
struct Pool
{
	template<typename TDerived>
	TDerived* PTNew()
	{
		TDerived* pT = new TDerived();
		m_arypT.push_back(pT);

		return pT;
	}

	~Pool()
	{
		Clear();
	}

	void Clear()
	{
		for(T* pT : m_arypT)
		{
			delete pT;
		}

		m_arypT.clear();
	}
	
	vector<T*> m_arypT;
};
