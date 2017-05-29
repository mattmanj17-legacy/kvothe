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
	size_t operator()(const CDynBitAry * const pBary) const
	{
		return pBary->NHash();
	}
};

struct NEqualsPDynBitAry
{
	bool operator()(const CDynBitAry * const lhs, const CDynBitAry * const rhs ) const
	{
		return lhs->FEquals(rhs);
	}
};

SDfaState * DfaFromNfa(const CNfa * pBuilder)
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
		const CDynBitAry * m_transitions [256];
	};

	Pool<TempState> poolTempstate;

	Pool<CDynBitAry> poolBary;

	unordered_map<const CDynBitAry *, TempState *, NHashPDynBitAry, NEqualsPDynBitAry> Move;

	struct Foo
	{
		const CDynBitAry * m_pBary;
		TempState * m_pTempState;
	};
	
	queue<Foo> EClosureQ;

	const CDynBitAry * eclosureBegin = pBuilder->PStateStart()->EpsilonClosure();

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
				const CNfaState * pNfas = pBuilder->PStateFromId(iBit);

				for(size_t iChr = 0; iChr < 256; ++iChr)
				{
					u8 chr = (u8)iChr;
					
					assert(pNfas->PStateTransition(chr) != CNfa::PStateEmpty());

					if(pNfas->PStateTransition(chr))
					{
						if(!transitions[iChr])
						{
							CDynBitAry * pBary = poolBary.PTNew<CDynBitAry>();
							pBary->SetSize(pBuilder->CState());
							transitions[iChr] = pBary;
						}
					
						transitions[iChr]->Set(pNfas->PStateTransition(chr)->NId());
					}
				}
			}
		}

		TempState * pTempState = T.m_pTempState;

		for(int iChr = 0; iChr < DIM(transitions); ++iChr)
		{
			if(transitions[iChr])
			{
				CDynBitAry * S = new CDynBitAry();
				S->SetSize(pBuilder->CState());
				
				for(size_t iBit = 0; iBit < transitions[iChr]->C(); ++ iBit)
				{
					if(transitions[iChr]->At(iBit))
					{
						const CNfaState * pNfasTran = pBuilder->PStateFromId(iBit);

						S->Union(pNfasTran->EpsilonClosure());
					}
				}

				typedef decltype(Move) MapType;

				MapType::iterator lb = Move.lower_bound(S);

				if(lb != Move.end() && S->FEquals(lb->first))
				{
					// key already exists

					delete S;

					pTempState->m_transitions[iChr] = lb->first;
				}
				else
				{
					// the key does not exist in the map
					// Use lb as a hint to insert, so we can avoid another lookup

					poolBary.m_arypT.push_back(S);

					TempState * pTempState = poolTempstate.PTNew<TempState>();

					Move.insert(lb, MapType::value_type(S, pTempState));

					EClosureQ.push(Foo { S,  pTempState });

					pTempState->m_transitions[iChr] = S;
				}
			}
		}
	}

	for(pair<const CDynBitAry *, TempState *> move : Move)
	{
		const CDynBitAry * eclosure = move.first;
		TempState * pTempState = move.second;
		
		pTempState->m_pDfas = g_poolDfas.PTNew<SDfaState>();

		for(size_t iBit = 0; iBit < eclosure->C(); ++iBit)
		{
			if(eclosure->At(iBit))
			{
				const CNfaState * pNfas = pBuilder->PStateFromId(iBit);

				if(pNfas == pBuilder->PStateAccept())
				{
					pTempState->m_pDfas->m_fIsFinal = true;
					break;
				}
			}
		}
	}

	for(pair<const CDynBitAry *, TempState *> move : Move)
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
