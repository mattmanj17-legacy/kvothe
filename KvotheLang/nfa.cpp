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
	printf("\n");
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
		printf("\n");
	}
}

void CNfaState::Patch(CNfaState * pNfas)
{
	for(size_t iEpsilon = 0; iEpsilon < m_arypStateTransitionEpsilon.size(); ++iEpsilon)
	{
		if(!m_arypStateTransitionEpsilon[iEpsilon])
		{
			m_arypStateTransitionEpsilon[iEpsilon] = pNfas;
		}
	}

	for(size_t iChr = 0; iChr < DIM(m_mpChrPStateTransition); ++iChr)
	{
		if(m_mpChrPStateTransition[iChr] == CNfa::PStateEmpty())
		{
			m_mpChrPStateTransition[iChr] = pNfas;
		}
	}
}

bool CNfaState::FIsUnpatched() const
{
	// BB (matthewd) cache this value instead?
	
	for(size_t iEpsilon = 0; iEpsilon < m_arypStateTransitionEpsilon.size(); ++iEpsilon)
	{
		if(!m_arypStateTransitionEpsilon[iEpsilon])
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
	
	queue<const CNfaState*> qpNfas;
	qpNfas.push(this);

	while(qpNfas.size() > 0)
	{
		const CNfaState* pNfas = qpNfas.front();
		qpNfas.pop();

		if(!m_baryEpsilonClosure.At(pNfas->m_nId))
		{
			m_baryEpsilonClosure.Set(pNfas->m_nId);
		
			for(const CNfaState* pStateEpsilon : pNfas->m_arypStateTransitionEpsilon)
			{
				qpNfas.push(pStateEpsilon);
			}
		}
	}
}

CNfa::SNfaFragment::SNfaFragment()
: m_pStateBegin(nullptr)
, m_arypStateUnpatched()
{
}

CNfa::SNfaFragment::SNfaFragment(CNfaState * pNfas, vector<CNfaState *> aryUnpatched)
: m_pStateBegin(pNfas)
, m_arypStateUnpatched(aryUnpatched)
{
}

void CNfa::SNfaFragment::Patch(CNfa::SNfaFragment nfa)
{
	for(CNfaState * Nfas : m_arypStateUnpatched)
	{
		Nfas->Patch(nfa.m_pStateBegin);
	}

	m_arypStateUnpatched.clear();

	// we cant just set m_arypStateUnpatched = nfa.m_arypStateUnpatched,
	// because m_arypStateUnpatched and nfa.m_arypStateUnpatched are not garenteed to be disjoint
	// so we may have already patched some of the nodes in nfa.m_arypStateUnpatched
	// BB (matthewd) hmm.... i dont like this
	
	for(CNfaState * Nfas : nfa.m_arypStateUnpatched)
	{
		if(Nfas->FIsUnpatched())
		{
			m_arypStateUnpatched.push_back(Nfas);
		}
	}
}

void CNfa::PrintDebug()
{
	// todo
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
		SNfaFragment nfaAlt = FragFromRegex(&pUnion->m_aryRegex[iRegex]);
		aryUnpatched.insert(aryUnpatched.end(), nfaAlt.m_arypStateUnpatched.begin(), nfaAlt.m_arypStateUnpatched.end());
		pNfasOr->AddEpsilonTransition(nfaAlt.m_pStateBegin);
	}

	return SNfaFragment(pNfasOr, aryUnpatched);
}

CNfa::SNfaFragment CNfa::FragFromConcat(const SConcatinationRegexData * pConcat)
{
	assert(pConcat->m_aryRegex.size() > 0);

	CNfa::SNfaFragment nfa = FragFromRegex(&pConcat->m_aryRegex[0]);

	for(size_t i = 1; i < pConcat->m_aryRegex.size(); ++i)
	{
		nfa.Patch(FragFromRegex(&pConcat->m_aryRegex[i]));
	}

	return nfa;
}

CNfa::SNfaFragment CNfa::FragFromQuant(const SQuantifierRegexData * pQuant)
{
	CNfa::SNfaFragment nfa;

	if(pQuant->m_cMic > 0)
	{
		nfa = FragCreateCount(&pQuant->m_regex, pQuant->m_cMic);
	}

	if(pQuant->m_cMac == -1)
	{
		if(nfa.m_pStateBegin)
		{
			nfa.Patch(FragCreateStar(&pQuant->m_regex));
		}
		else
		{
			nfa = FragCreateStar(&pQuant->m_regex);
		}

		return nfa;
	}

	if(pQuant->m_cMac <= pQuant->m_cMic)
	{
		assert(nfa.m_pStateBegin);
		return nfa;
	}

	if(nfa.m_pStateBegin)
	{
		nfa.Patch(FragCreateCountOptional(&pQuant->m_regex, pQuant->m_cMac - pQuant->m_cMic));
	}
	else
	{
		nfa = FragCreateCountOptional(&pQuant->m_regex, pQuant->m_cMac - pQuant->m_cMic);
	}

	return nfa;
}

CNfa::SNfaFragment CNfa::FragFromRange(const SRangeRegexData * pRange)
{
	CNfaState * pNfasOr = PStateCreate();

	for(int iChr = pRange->m_chrMic; iChr <= pRange->m_chrMac; ++iChr)
	{
		CNfaState * pNfasChr = PStateCreate();
		pNfasChr->AddChrTransition(iChr, CNfa::PStateEmpty());

		pNfasOr->AddEpsilonTransition(pNfasChr);
	}

	return SNfaFragment(pNfasOr, pNfasOr->m_arypStateTransitionEpsilon);
}

CNfa::SNfaFragment CNfa::FragFromChr(const SChrRegexData * pRegexchr)
{
	CNfaState * pNfasChr = PStateCreate();
	pNfasChr->AddChrTransition(pRegexchr->m_chr, CNfa::PStateEmpty());

	return SNfaFragment(pNfasChr, { pNfasChr });
}

CNfa::SNfaFragment CNfa::FragCreateCount(const SRegexAstNode * pRegex, int c)
{
	CNfa::SNfaFragment nfa = FragFromRegex(pRegex);

	for(size_t iC = 1; iC < c; ++iC)
	{
		nfa.Patch(FragFromRegex(pRegex));
	}

	return nfa;
}

CNfa::SNfaFragment CNfa::FragCreateStar(const SRegexAstNode * pRegex)
{
	// create a ?
	
	CNfaState * pNfasOr = PStateCreate();
	SNfaFragment nfa = FragFromRegex(pRegex);
	pNfasOr->AddEpsilonTransition(nfa.m_pStateBegin);
	pNfasOr->AddEpsilonTransition(nullptr);

	// the final nfa will only have one unpatched node, the starting node, 
	// because the nfa created from m_pregex is patched back to pOrState

	SNfaFragment nfaStar(pNfasOr, { pNfasOr });

	// loop back the nfa created from m_pregex to pOrState
	// turning a ? into a *

	nfa.Patch(nfaStar);

	return nfaStar;
}

CNfa::SNfaFragment CNfa::FragCreateQMark(const SRegexAstNode * pRegex)
{
	// create a ?
	
	CNfaState * pNfasOr = PStateCreate();
	SNfaFragment nfa = FragFromRegex(pRegex);
	pNfasOr->AddEpsilonTransition(nfa.m_pStateBegin);
	pNfasOr->AddEpsilonTransition(nullptr);

	// the unpatched states are pOrState and all the unpatched states of
	// the nfa created from m_pRegex

	vector<CNfaState *> aryUnpatched;
	aryUnpatched.push_back(pNfasOr);
	aryUnpatched.insert(aryUnpatched.end(), nfa.m_arypStateUnpatched.begin(), nfa.m_arypStateUnpatched.end());

	return SNfaFragment(pNfasOr, aryUnpatched);
}

CNfa::SNfaFragment CNfa::FragCreateCountOptional(const SRegexAstNode * pRegex, int c)
{
	assert(c > 0);

	// create nested ?'s, starting on the right and working
	// back to the left to the starting node
	
	SNfaFragment nfaLeftMost;

	for(int iC = c; iC > 0; --iC)
	{
		SNfaFragment nfaQMark = FragCreateQMark(pRegex);

		if(nfaLeftMost.m_pStateBegin)
		{
			nfaQMark.Patch(nfaLeftMost);
		}

		nfaLeftMost = nfaQMark;
	}

	return nfaLeftMost;
}

CNfaState * CNfa::PStateCreate()
{
	CNfaState * pNfas = m_poolState.PTNew<CNfaState>();

	pNfas->m_nId = m_poolState.m_arypT.size() - 1;

	return pNfas;
}

CNfaState CNfa::s_stateEmpty;

