#include "dfa.h"

#include "macros.h"

#include <unordered_map>
using std::unordered_map;
using std::pair;

#include <queue>
using std::queue;

SDfaState::SDfaState()
: m_nId(-1)
, m_fIsFinal(false)
{
	memset(m_transitions, 0, DIM(m_transitions) * sizeof(SDfaState*));
}

void SDfaState::PrintDebug()
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

CDfa::CDfa()
: m_poolState()
, m_pStateStart(nullptr)
{
}

void CDfa::PrintDebug() const
{
	printf("start state: %d\n", m_pStateStart->m_nId);

	for(SDfaState * pState : m_poolState.m_arypT)
	{
		printf("\n");
		pState->PrintDebug();
	}
}

// struct used to hold data as we are converting from sets of nfa states to dfa states

struct SDfaStateTemp
{
	SDfaStateTemp()
	: m_pBary(nullptr)
	, m_pDfaState(nullptr)
	, m_transitions()
	, m_nId(-1)
	{
		memset(m_transitions, 0, DIM(m_transitions) * sizeof(CDynBitAry *));
	}

	const CDynBitAry *	m_pBary;
	SDfaState *			m_pDfaState;
	const CDynBitAry *	m_transitions [256];
	int					m_nId;
};

// hash struct for unordered_map

struct NHashPDynBitAry
{
	size_t operator()(const CDynBitAry * const pBary) const
	{
		return pBary->NHash();
	}
};

// equals struct for unordered_map

struct NEqualsPDynBitAry
{
	bool operator()(const CDynBitAry * const lhs, const CDynBitAry * const rhs ) const
	{
		return lhs->FEquals(rhs);
	}
};

void CDfa::Build(const CNfa * pNfa)
{
	m_poolState.Clear();
	
	// pools for all of the Temp states and bit arrays we will need
	
	Pool<SDfaStateTemp>		poolTempstate;
	Pool<CDynBitAry>		poolBaryEClosure;

	// map from epsilon closure to temp state
	
	typedef unordered_map<
		const CDynBitAry *, 
		SDfaStateTemp *, 
		NHashPDynBitAry, 
		NEqualsPDynBitAry> 
							TempStateMap;

	TempStateMap			tempStateMap;

	// queue of temp states to process

	queue<SDfaStateTemp *>	tempStatesToProcess;

	// create a temp state from the starting nfa state

	const CDynBitAry * baryEClosureBegin = pNfa->PStateStart()->EpsilonClosure();

	SDfaStateTemp * pTempStateBegin = poolTempstate.PTNew<SDfaStateTemp>();
	pTempStateBegin->m_pBary = baryEClosureBegin;
	pTempStateBegin->m_nId = poolTempstate.m_arypT.size() - 1;

	// add initial state to the map

	tempStateMap[baryEClosureBegin] = pTempStateBegin;

	// add initial state to processing queue

	tempStatesToProcess.push(pTempStateBegin);

	// scratch space for computing transitions
	
	CDynBitAry * transitions [256];
	for(int iTran = 0; iTran < DIM(transitions); ++ iTran)
	{
		CDynBitAry * pBary = new CDynBitAry();
		pBary->SetSize(pNfa->CState());
		transitions[iTran] = pBary;
	}

	CDynBitAry baryHasTran;
	baryHasTran.SetSize(256);

	while(tempStatesToProcess.size() > 0)
	{
		// pop off the next temp state to process
		
		SDfaStateTemp * pTempState = tempStatesToProcess.front();
		tempStatesToProcess.pop();
	
		// clear the transitions array
		
		for (CDynBitAry * pBary : transitions)
		{
			pBary->Clear();
		}

		baryHasTran.Clear();

		// compute the transitions for this temp state
		
		for(size_t iBit = 0; iBit < pTempState->m_pBary->C(); ++iBit)
		{
			// if nfa state with id iBit is in the current temp state's epsilon closure, get all of its transitions
			
			if(pTempState->m_pBary->At(iBit))
			{
				const CNfaState * pNfaState = pNfa->PStateFromId(iBit);

				// get all of its transitions
				
				for(size_t iChr = 0; iChr < 256; ++iChr)
				{
					u8 chr = (u8)iChr;
				
					assert(pNfaState->PStateTransition(chr) != CNfa::PStateEmpty());

					if(pNfaState->PStateTransition(chr))
					{
						if(!baryHasTran.At(iChr))
						{
							baryHasTran.Set(iChr);
						}
				
						transitions[iChr]->Set(pNfaState->PStateTransition(chr)->NId());
					}
				}
			}
		}

		for(int iChr = 0; iChr < DIM(transitions); ++iChr)
		{
			// for each char that this temp state transitions on
			
			if(baryHasTran.At(iChr))
			{
				// construct the epsilon closure of the node we are transitioning to
				// say we are transioning on the char 'a'. we are looking for aE*, or
				// the set of states we can get to by an a followed by zero or more epsilons
				
				CDynBitAry * aEStar = new CDynBitAry();
				aEStar->SetSize(pNfa->CState());
				
				for(size_t iBit = 0; iBit < transitions[iChr]->C(); ++ iBit)
				{
					// for each state we can reach on 'a' transition

					if(transitions[iChr]->At(iBit))
					{
						// union in the eClosure of this state
						
						const CNfaState * pNfaStateTran = pNfa->PStateFromId(iBit);

						aEStar->Union(pNfaStateTran->EpsilonClosure());
					}
				}

				// compute hash before interacting with tempStateMap

				aEStar->ComputeHash();

				// check if there is already a temp state for this set of nfa states

				TempStateMap::iterator lb = tempStateMap.lower_bound(aEStar);

				if(lb != tempStateMap.end() && aEStar->FEquals(lb->first))
				{
					// key already exists

					delete aEStar;

					// set transition on 'a' to the existing set of nfa states

					pTempState->m_transitions[iChr] = lb->first;
				}
				else
				{
					// the key does not exist in the map
					// Use lb as a hint to insert, so we can avoid another lookup

					// create a new temp state for this set of nfa states

					poolBaryEClosure.m_arypT.push_back(aEStar);

					SDfaStateTemp * pTempStateNew = poolTempstate.PTNew<SDfaStateTemp>();
					pTempStateNew->m_pBary = aEStar;
					pTempStateNew->m_nId = poolTempstate.m_arypT.size() - 1;

					tempStateMap.insert(lb, TempStateMap::value_type(aEStar, pTempStateNew));

					// set transition on 'a' to the new set of nfa states

					pTempState->m_transitions[iChr] = aEStar;

					// add the new temp state to the processing queue  

					tempStatesToProcess.push(pTempStateNew);
				}
			}
		}
	}

	for(int iTran = 0; iTran < DIM(transitions); ++ iTran)
	{
		delete transitions[iTran];
	}

	for(pair<const CDynBitAry *, SDfaStateTemp *> kvp : tempStateMap)
	{
		const CDynBitAry * eclosure = kvp.first;
		SDfaStateTemp * pTempState = kvp.second;

		// actually create dfa states for each temp state
	
		pTempState->m_pDfaState = m_poolState.PTNew<SDfaState>();

		pTempState->m_pDfaState->m_nId = pTempState->m_nId;

		for(size_t iBit = 0; iBit < eclosure->C(); ++iBit)
		{
			if(eclosure->At(iBit))
			{
				const CNfaState * pNfas = pNfa->PStateFromId(iBit);

				// mark this dfa state as final if the subset of nfa states contains the accepting nfa state
				
				if(pNfas == pNfa->PStateAccept())
				{
					pTempState->m_pDfaState->m_fIsFinal = true;
					break;
				}
			}
		}
	}

	for(pair<const CDynBitAry *, SDfaStateTemp *> kvp : tempStateMap)
	{
		const CDynBitAry * eclosure = kvp.first;
		SDfaStateTemp * pTempState = kvp.second;

		// set up all of the transisions of the dfa states
		
		for(size_t iChr = 0; iChr < DIM(pTempState->m_transitions); ++iChr)
		{
			if(pTempState->m_transitions[iChr])
			{
				pTempState->m_pDfaState->m_transitions[iChr] = tempStateMap[pTempState->m_transitions[iChr]]->m_pDfaState;
			}
		}
	}

	std::sort(
		m_poolState.m_arypT.begin(), 
		m_poolState.m_arypT.end(), 
		[](SDfaState * a, SDfaState * b){ return a->m_nId < b->m_nId; }
	);

	m_pStateStart = pTempStateBegin->m_pDfaState;

	poolTempstate.Clear();
	poolBaryEClosure.Clear();
}