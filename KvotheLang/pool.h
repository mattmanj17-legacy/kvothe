#pragma once

#include <vector>
using std::vector;

struct VoidPool
{
	template<typename T>
	T* PTNew()
	{
		T* pT = new T();
		m_arypv.push_back(pT);

		return pT;
	}

	~VoidPool()
	{
		Clear();
	}

	void Clear()
	{
		for(void* pT : m_arypv)
		{
			delete pT;
		}

		m_arypv.clear();
	}
	
	vector<void*> m_arypv;
};

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
