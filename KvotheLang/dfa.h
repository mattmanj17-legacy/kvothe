#pragma once

#include "macros.h"

#include <unordered_map>
using std::unordered_map;
using std::pair;

#include <queue>
using std::queue;

struct SDfaState // tag = dfas
{
	SDfaState()
	{
		m_nId = ++s_nIdNext;
		memset(m_transitions, 0, DIM(m_transitions) * sizeof(SDfaState*));
	}

	void PrintDebug()
	{
		printf("state %d:\n", m_nId);

		if(m_fIsFinal)
		{
			printf("accepting state!\n");
		}
		
		printf("transitions:\n");

		for(size_t iChr = 0; iChr < DIM(m_transitions); ++iChr)
		{
			if(m_transitions[iChr])
			{
				printf("%d '%c' -> %d", iChr, iChr, m_transitions[iChr]->m_nId);
				printf("\n");
			}
		}
	}

	SDfaState* m_transitions [256];
	static int s_nIdNext;
	int m_nId;
	bool m_fIsFinal = false;
};
int SDfaState::s_nIdNext = 0;

Pool<SDfaState> g_poolDfas;

struct NHashPDynBitAry
{
	size_t operator()(CDynBitAry * const tohash) const
	{
		return tohash->Hash();
	}
};

struct NEqualsPDynBitAry
{
	bool operator()( CDynBitAry * const lhs, CDynBitAry * const rhs ) const
	{
		return lhs->FEquals(rhs);
	}
};

SDfaState * DfaFromNfa(SNfaBuilder * pBuilder, SNfaState * pNfasBegin, SNfaState * pNfasEnd)
{
	struct TempState
	{
		TempState()
		: m_pDfas(nullptr)
		, m_transitions()
		{
			memset(m_transitions, 0, DIM(m_transitions) * sizeof(CDynBitAry *));
		}
		
		SDfaState * m_pDfas;
		CDynBitAry * m_transitions [256];
	};

	Pool<TempState> poolTempstate;

	unordered_map<CDynBitAry *, TempState *, NHashPDynBitAry, NEqualsPDynBitAry> Move;

	struct Foo
	{
		CDynBitAry * m_pBary;
		TempState * m_pTempState;
	};
	
	queue<Foo> EClosureQ;

	CDynBitAry * eclosureBegin = pNfasBegin->EClosure();

	TempState * pTempStateBegin = poolTempstate.PTNew<TempState>();
	Move[eclosureBegin] = pTempStateBegin;

	EClosureQ.push(Foo { eclosureBegin, pTempStateBegin });

	CDynBitAry * transitions [256];

	while(EClosureQ.size() > 0)
	{
		Foo T = EClosureQ.front();
		EClosureQ.pop();
		
		memset(transitions, 0, DIM(transitions) * sizeof(CDynBitAry *));

		for(size_t iBit = 0; iBit < T.m_pBary->C(); ++iBit)
		{
			if(T.m_pBary->At(iBit))
			{
				SNfaState * pNfas = pBuilder->PNfasFromId(iBit);

				for(size_t iChr = 0; iChr < DIM(pNfas->m_transitions); ++iChr)
				{
					assert(pNfas->m_transitions[iChr] != nfasEmpty);

					if(pNfas->m_transitions[iChr])
					{
						if(!transitions[iChr])
						{
							transitions[iChr] = pBuilder->PBAryCreate();
						}
					
						transitions[iChr]->Set(pNfas->m_transitions[iChr]->m_nId);
					}
				}
			}
		}

		TempState * pTempState = T.m_pTempState;

		for(int iChr = 0; iChr < DIM(transitions); ++iChr)
		{
			if(transitions[iChr])
			{
				CDynBitAry * S = pBuilder->PBAryCreateUnpooled();
				
				for(size_t iBit = 0; iBit < transitions[iChr]->C(); ++ iBit)
				{
					if(transitions[iChr]->At(iBit))
					{
						SNfaState * pNfasTran = pBuilder->PNfasFromId(iBit);

						S->Union(pNfasTran->EClosure());
					}
				}

				typedef decltype(Move) MapType;

				MapType::iterator lb = Move.lower_bound(S);

				if(lb != Move.end() && S->FEquals(lb->first))
				{
					// key already exists

					CDynBitAry * SOld = S;
					S = lb->first;
					delete SOld;
				}
				else
				{
					// the key does not exist in the map
					// Use lb as a hint to insert, so we can avoid another lookup

					pBuilder->AddToPool(S);

					TempState * pTempState = poolTempstate.PTNew<TempState>();

					Move.insert(lb, MapType::value_type(S, pTempState));

					EClosureQ.push(Foo { S,  pTempState });
				}

				pTempState->m_transitions[iChr] = S;
			}
		}
	}

	for(pair<CDynBitAry *, TempState *> move : Move)
	{
		CDynBitAry * eclosure = move.first;
		TempState * pTempState = move.second;
		
		pTempState->m_pDfas = g_poolDfas.PTNew<SDfaState>();

		for(size_t iBit = 0; iBit < eclosure->C(); ++iBit)
		{
			if(eclosure->At(iBit))
			{
				SNfaState * pNfas = pBuilder->PNfasFromId(iBit);

				if(pNfas == pNfasEnd)
				{
					pTempState->m_pDfas->m_fIsFinal = true;
					break;
				}
			}
		}
	}

	for(pair<CDynBitAry *, TempState *> move : Move)
	{
		for(size_t iChr = 0; iChr < DIM(move.second->m_transitions); ++iChr)
		{
			if(move.second->m_transitions[iChr])
			{
				move.second->m_pDfas->m_transitions[iChr] = Move[move.second->m_transitions[iChr]]->m_pDfas;
			}
		}
	}

	return pTempStateBegin->m_pDfas;
}
