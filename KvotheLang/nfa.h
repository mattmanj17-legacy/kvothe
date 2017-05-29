#pragma once

#include <queue>
using std::queue;

#include "bitary.h"
#include "macros.h"

struct SNfaState;

SNfaState * nfasEmpty;

struct SNfaState
{	
	SNfaState()
	: m_aryEpsilon()
	, m_transitions()
	, m_fBaked(false)
	, m_eclosure()
	, m_nId(-1)
	{
		memset(m_transitions, 0, DIM(m_transitions) * sizeof(SNfaState *));
	}

	void PrintDebug()
	{
		printf("\n");
		printf("state %d:\n", m_nId);
	
		printf("transitions:");

		for(size_t iChr = 0; iChr < DIM(m_transitions); ++iChr)
		{
			if(m_transitions[iChr])
			{
				printf(" ");
				printf("%d '%c' -> %d", iChr, iChr, m_transitions[iChr]->m_nId);
			}
		}
		printf("\n");

		if(m_aryEpsilon.size() > 0)
		{
			printf("epsilon:");
			for(SNfaState * nfas : m_aryEpsilon)
			{
				assert(nfas);
				printf(" %d", nfas->m_nId);
			}
			printf("\n");
		}
	}

	void AddTransition(u8 chr, SNfaState * pNfas)
	{
		m_transitions[chr] = pNfas;
	}

	void AddEpsilon(SNfaState * pNfas)
	{
		m_aryEpsilon.push_back(pNfas);
	}

	void Patch(SNfaState * pNfas)
	{
		for(size_t iEpsilon = 0; iEpsilon < m_aryEpsilon.size(); ++iEpsilon)
		{
			if(!m_aryEpsilon[iEpsilon])
			{
				m_aryEpsilon[iEpsilon] = pNfas;
			}
		}

		for(size_t iChr = 0; iChr < DIM(m_transitions); ++iChr)
		{
			if(m_transitions[iChr] == nfasEmpty)
			{
				m_transitions[iChr] = pNfas;
			}
		}
	}

	bool FUnpatched()
	{
		// BB (matthewd) cache this value instead?
		
		for(size_t iEpsilon = 0; iEpsilon < m_aryEpsilon.size(); ++iEpsilon)
		{
			if(!m_aryEpsilon[iEpsilon])
				return true;
		}

		for(size_t iChr = 0; iChr < DIM(m_transitions); ++iChr)
		{
			if(m_transitions[iChr] == nfasEmpty)
				return true;
		}

		return false;
	}

	void Bake(int cState)
	{
		assert(!m_fBaked);
		
		m_fBaked = true;

		m_eclosure.SetSize(cState);

		// breadth first search for nodes we can reach by 0 or more epsilon transitions
		
		queue<SNfaState*> qpNfas;
		qpNfas.push(this);

		while(qpNfas.size() > 0)
		{
			SNfaState* pNfas = qpNfas.front();
			qpNfas.pop();

			if(!m_eclosure.At(pNfas->m_nId))
			{
				m_eclosure.Set(pNfas->m_nId);
			
				for(SNfaState* pStateEpsilon : pNfas->m_aryEpsilon)
				{
					qpNfas.push(pStateEpsilon);
				}
			}
		}
	}

	CDynBitAry * EClosure()
	{
		assert(m_fBaked);
		
		return &m_eclosure;
	}

	vector<SNfaState*> m_aryEpsilon;
	SNfaState* m_transitions [256];
	bool m_fBaked;
	CDynBitAry m_eclosure;
	int m_nId;
};

struct SNfaFragment
{
	SNfaState * m_pStateBegin;
	vector<SNfaState *> m_aryUnpatched;

	void Patch(SNfaFragment nfa)
	{
		for(SNfaState * Nfas : m_aryUnpatched)
		{
			Nfas->Patch(nfa.m_pStateBegin);
		}

		m_aryUnpatched.clear();

		// we cant just set m_arypStateUnpatched = nfa.m_arypStateUnpatched,
		// because m_arypStateUnpatched and nfa.m_arypStateUnpatched are not garenteed to be disjoint
		// so we may have already patched some of the nodes in nfa.m_arypStateUnpatched
		// BB (matthewd) hmm.... i dont like this
		
		for(SNfaState * Nfas : nfa.m_aryUnpatched)
		{
			if(Nfas->FUnpatched())
			{
				m_aryUnpatched.push_back(Nfas);
			}
		}
	}

	SNfaFragment()
	: m_pStateBegin(nullptr)
	, m_aryUnpatched()
	{
	}

	SNfaFragment(SNfaState * pNfas, vector<SNfaState *> aryUnpatched)
	: m_pStateBegin(pNfas)
	, m_aryUnpatched(aryUnpatched)
	{
	}
};

struct SNfaBuilder
{	
	SNfaState * PNfasCreate()
	{
		SNfaState * pNfas = m_poolNfas.PTNew<SNfaState>();

		pNfas->m_nId = m_poolNfas.m_arypT.size() - 1;

		return pNfas;
	}

	void Bake()
	{
		for(SNfaState * pNfas : m_poolNfas.m_arypT)
		{
			pNfas->Bake(m_poolNfas.m_arypT.size());
		}
	}

	SNfaState * PNfasFromId(int nId)
	{
		return m_poolNfas.m_arypT[nId];
	}

	CDynBitAry * PBAryCreate()
	{
		CDynBitAry * pBary = m_poolBary.PTNew<CDynBitAry>();
		pBary->SetSize(m_poolNfas.m_arypT.size());
		return pBary;
	}

	CDynBitAry * PBAryCreateUnpooled()
	{
		CDynBitAry * pBary = new CDynBitAry();
		pBary->SetSize(m_poolNfas.m_arypT.size());
		return pBary;
	}

	void AddToPool(CDynBitAry * pBary)
	{
		m_poolBary.m_arypT.push_back(pBary);
	}

	Pool<CDynBitAry> m_poolBary;
	Pool<SNfaState> m_poolNfas;
};
