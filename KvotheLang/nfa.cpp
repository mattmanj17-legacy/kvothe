#include "nfa.h"

#include "macros.h"

#include <queue>
using std::queue;

CNfaState::CNfaState()
: m_arypStateTransitionEpsilon()
, m_mpChrPStateTransition()
, m_fWasEpsilonClosureComputed(false)
, m_baryEpsilonClosure()
, m_nId(-1)
{
	memset(m_mpChrPStateTransition, 0, DIM(m_mpChrPStateTransition) * sizeof(CNfaState *));
}

void CNfaState::PrintDebug() const
{
	printf("state %d:\n", m_nId);

	printf("transitions:");

	for(size_t iChr = 0; iChr < DIM(m_mpChrPStateTransition); ++iChr)
	{
		if(m_mpChrPStateTransition[iChr])
		{
			printf(" ");
			printf("%d '%c' -> %d", iChr, iChr, m_mpChrPStateTransition[iChr]->m_nId);
		}
	}

	printf("\n");

	if(m_arypStateTransitionEpsilon.size() > 0)
	{
		printf("epsilon:");
		for(const CNfaState * nfas : m_arypStateTransitionEpsilon)
		{
			assert(nfas);
			printf(" %d", nfas->m_nId);
		}
	}
}

void CNfaState::Patch(CNfaState * pState)
{
	for(size_t iEpsilonTransition = 0; iEpsilonTransition < m_arypStateTransitionEpsilon.size(); ++iEpsilonTransition)
	{
		if(!m_arypStateTransitionEpsilon[iEpsilonTransition])
		{
			m_arypStateTransitionEpsilon[iEpsilonTransition] = pState;
		}
	}

	for(size_t iChr = 0; iChr < DIM(m_mpChrPStateTransition); ++iChr)
	{
		if(m_mpChrPStateTransition[iChr] == CNfa::PStateEmpty())
		{
			m_mpChrPStateTransition[iChr] = pState;
		}
	}
}

bool CNfaState::FIsUnpatched() const
{
	// BB (matthewd) cache this value instead?
	
	for(size_t iEpsilonTransition = 0; iEpsilonTransition < m_arypStateTransitionEpsilon.size(); ++iEpsilonTransition)
	{
		if(!m_arypStateTransitionEpsilon[iEpsilonTransition])
			return true;
	}

	for(size_t iChr = 0; iChr < DIM(m_mpChrPStateTransition); ++iChr)
	{
		if(m_mpChrPStateTransition[iChr] == CNfa::PStateEmpty())
			return true;
	}

	return false;
}

void CNfaState::ComputeEpsilonClosure(int cState)
{
	assert(!m_fWasEpsilonClosureComputed);
	
	m_fWasEpsilonClosureComputed = true;

	m_baryEpsilonClosure.SetSize(cState);

	// breadth first search for nodes we can reach by 0 or more epsilon transitions
	
	queue<const CNfaState*> qpState;
	qpState.push(this);

	while(qpState.size() > 0)
	{
		const CNfaState* pNfas = qpState.front();
		qpState.pop();

		if(!m_baryEpsilonClosure.At(pNfas->m_nId))
		{
			m_baryEpsilonClosure.Set(pNfas->m_nId);
		
			for(const CNfaState* pStateEpsilon : pNfas->m_arypStateTransitionEpsilon)
			{
				qpState.push(pStateEpsilon);
			}
		}
	}

	m_baryEpsilonClosure.ComputeHash();
}

CNfa::SNfaFragment::SNfaFragment()
: m_pStateBegin(nullptr)
, m_arypStateUnpatched()
{
}

CNfa::SNfaFragment::SNfaFragment(CNfaState * pStateBegin, vector<CNfaState *> arypStateUnpatched)
: m_pStateBegin(pStateBegin)
, m_arypStateUnpatched(arypStateUnpatched)
{
}

void CNfa::SNfaFragment::Patch(SNfaFragment frag)
{
	for(CNfaState * pState : m_arypStateUnpatched)
	{
		pState->Patch(frag.m_pStateBegin);
	}

	m_arypStateUnpatched.clear();

	// we cant just set m_arypStateUnpatched = nfa.m_arypStateUnpatched,
	// because m_arypStateUnpatched and nfa.m_arypStateUnpatched are not garenteed to be disjoint
	// so we may have already patched some of the nodes in nfa.m_arypStateUnpatched
	// BB (matthewd) hmm.... i dont like this
	
	for(CNfaState * pState : frag.m_arypStateUnpatched)
	{
		if(pState->FIsUnpatched())
		{
			m_arypStateUnpatched.push_back(pState);
		}
	}
}

void CNfa::PrintDebug() const
{
	printf("start state: %d\n", m_pStateStart->m_nId);

	for(CNfaState * pState : m_poolState.m_arypT)
	{
		printf("\n");
		pState->PrintDebug();
	}
}

void CNfa::Build(const SRegexAstNode * pRegex)
{
	m_poolState.Clear();
	
	SNfaFragment frag = FragFromRegex(pRegex);

	m_pStateEnd = PStateCreate();

	frag.Patch(SNfaFragment(m_pStateEnd, {}));

	m_pStateStart = frag.m_pStateBegin;

	for(CNfaState * pNfas : m_poolState.m_arypT)
	{
		pNfas->ComputeEpsilonClosure(m_poolState.m_arypT.size());
	}
}

CNfa::SNfaFragment CNfa::FragFromRegex(const SRegexAstNode * pRegex)
{
	switch (pRegex->m_regexk)
	{
		case REGEXK_Union:
			return FragFromUnion(pRegex->m_pUnionData);
			break;

		case REGEXK_Concat:
			return FragFromConcat(pRegex->m_pConcatData);
			break;

		case REGEXK_Quant:
			return FragFromQuant(pRegex->m_pQuantData);
			break;

		case REGEXK_Range:
			return FragFromRange(pRegex->m_pRangeData);
			break;

		case REGEXK_Chr:
			return FragFromChr(pRegex->m_pChrData);
			break;

		default:
			assert(false);
			return SNfaFragment();
			break;
	}
}

CNfa::SNfaFragment CNfa::FragFromUnion(const SUnionRegexData * pUnion)
{
	assert(pUnion->m_aryRegex.size() > 0);

	CNfaState * pNfasOr = PStateCreate();

	vector<CNfaState *> aryUnpatched;

	for(size_t iRegex = 0; iRegex < pUnion->m_aryRegex.size(); ++iRegex)
	{
		SNfaFragment frag = FragFromRegex(&pUnion->m_aryRegex[iRegex]);
		aryUnpatched.insert(aryUnpatched.end(), frag.m_arypStateUnpatched.begin(), frag.m_arypStateUnpatched.end());
		pNfasOr->AddEpsilonTransition(frag.m_pStateBegin);
	}

	return SNfaFragment(pNfasOr, aryUnpatched);
}

CNfa::SNfaFragment CNfa::FragFromConcat(const SConcatinationRegexData * pConcat)
{
	assert(pConcat->m_aryRegex.size() > 0);

	CNfa::SNfaFragment frag = FragFromRegex(&pConcat->m_aryRegex[0]);

	for(size_t i = 1; i < pConcat->m_aryRegex.size(); ++i)
	{
		frag.Patch(FragFromRegex(&pConcat->m_aryRegex[i]));
	}

	return frag;
}

CNfa::SNfaFragment CNfa::FragFromQuant(const SQuantifierRegexData * pQuant)
{
	CNfa::SNfaFragment frag;

	if(pQuant->m_cMic > 0)
	{
		frag = FragCreateCount(&pQuant->m_regex, pQuant->m_cMic);
	}

	if(pQuant->m_cMac == -1)
	{
		if(frag.m_pStateBegin)
		{
			frag.Patch(FragCreateStar(&pQuant->m_regex));
		}
		else
		{
			frag = FragCreateStar(&pQuant->m_regex);
		}

		return frag;
	}

	if(pQuant->m_cMac <= pQuant->m_cMic)
	{
		assert(frag.m_pStateBegin);
		return frag;
	}

	if(frag.m_pStateBegin)
	{
		frag.Patch(FragCreateCountOptional(&pQuant->m_regex, pQuant->m_cMac - pQuant->m_cMic));
	}
	else
	{
		frag = FragCreateCountOptional(&pQuant->m_regex, pQuant->m_cMac - pQuant->m_cMic);
	}

	return frag;
}

CNfa::SNfaFragment CNfa::FragFromRange(const SRangeRegexData * pRange)
{
	CNfaState * pState = PStateCreate();

	for(int iChr = pRange->m_chrMic; iChr <= pRange->m_chrMac; ++iChr)
	{
		pState->AddChrTransition(iChr, CNfa::PStateEmpty());
	}

	return SNfaFragment(pState, { pState });
}

CNfa::SNfaFragment CNfa::FragFromChr(const SChrRegexData * pChrRegex)
{
	CNfaState * pState = PStateCreate();
	pState->AddChrTransition(pChrRegex->m_chr, CNfa::PStateEmpty());

	return SNfaFragment(pState, { pState });
}

CNfa::SNfaFragment CNfa::FragCreateCount(const SRegexAstNode * pRegex, size_t c)
{
	CNfa::SNfaFragment frag = FragFromRegex(pRegex);

	for(size_t iC = 1; iC < c; ++iC)
	{
		frag.Patch(FragFromRegex(pRegex));
	}

	return frag;
}

CNfa::SNfaFragment CNfa::FragCreateStar(const SRegexAstNode * pRegex)
{
	// create a ?
	
	CNfaState * pState = PStateCreate();
	SNfaFragment frag = FragFromRegex(pRegex);
	pState->AddEpsilonTransition(frag.m_pStateBegin);
	pState->AddEpsilonTransition(nullptr);

	// the final nfa will only have one unpatched node, the starting node, 
	// because the nfa created from m_pregex is patched back to pOrState

	SNfaFragment fragStar(pState, { pState });

	// loop back the nfa created from m_pregex to pOrState
	// turning a ? into a *

	frag.Patch(fragStar);

	return fragStar;
}

CNfa::SNfaFragment CNfa::FragCreateQMark(const SRegexAstNode * pRegex)
{
	// create a ?
	
	CNfaState * pState = PStateCreate();
	SNfaFragment frag = FragFromRegex(pRegex);
	pState->AddEpsilonTransition(frag.m_pStateBegin);
	pState->AddEpsilonTransition(nullptr);

	// the unpatched states are pOrState and all the unpatched states of
	// the nfa created from m_pRegex

	vector<CNfaState *> arypStateUnpatched;
	arypStateUnpatched.push_back(pState);
	arypStateUnpatched.insert(arypStateUnpatched.end(), frag.m_arypStateUnpatched.begin(), frag.m_arypStateUnpatched.end());

	return SNfaFragment(pState, arypStateUnpatched);
}

CNfa::SNfaFragment CNfa::FragCreateCountOptional(const SRegexAstNode * pRegex, size_t c)
{
	assert(c > 0);

	// create nested ?'s, starting on the right and working
	// back to the left to the starting node
	
	SNfaFragment fragLeftMost;

	for(size_t iC = c; iC > 0; --iC)
	{
		SNfaFragment fragQMark = FragCreateQMark(pRegex);

		if(fragLeftMost.m_pStateBegin)
		{
			fragQMark.Patch(fragLeftMost);
		}

		fragLeftMost = fragQMark;
	}

	return fragLeftMost;
}

CNfaState * CNfa::PStateCreate()
{
	CNfaState * pState = m_poolState.PTNew<CNfaState>();

	pState->m_nId = m_poolState.m_arypT.size() - 1;

	return pState;
}

CNfaState CNfa::s_stateEmpty;

