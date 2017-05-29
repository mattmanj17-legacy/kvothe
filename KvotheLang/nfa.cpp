#include "nfa.h"

#include "macros.h"

#include <queue>
using std::queue;

void CNfa::Build(const SRegexAstNode * pRegex)
{
	m_poolBary.Clear();
	m_poolNfas.Clear();
	
	SNfaFragment frag = FragFromRegex(pRegex);

	m_pStateEnd = PStateCreate();

	frag.Patch(SNfaFragment(m_pStateEnd, {}));

	m_pStateStart = frag.m_pStateBegin;

	Bake();
}

CNfaState::CNfaState()
: m_aryEpsilon()
, m_transitions()
, m_fBaked(false)
, m_eclosure()
, m_nId(-1)
{
	memset(m_transitions, 0, DIM(m_transitions) * sizeof(CNfaState *));
}

void CNfaState::PrintDebug() const
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
		for(const CNfaState * nfas : m_aryEpsilon)
		{
			assert(nfas);
			printf(" %d", nfas->m_nId);
		}
		printf("\n");
	}
}

void CNfaState::AddTransition(u8 chr, const CNfaState * pNfas)
{
	m_transitions[chr] = pNfas;
}

void CNfaState::AddEpsilon(CNfaState * pNfas)
{
	m_aryEpsilon.push_back(pNfas);
}

void CNfaState::Patch(CNfaState * pNfas)
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
		if(m_transitions[iChr] == CNfa::PStateEmpty())
		{
			m_transitions[iChr] = pNfas;
		}
	}
}

bool CNfaState::FUnpatched() const
{
	// BB (matthewd) cache this value instead?
	
	for(size_t iEpsilon = 0; iEpsilon < m_aryEpsilon.size(); ++iEpsilon)
	{
		if(!m_aryEpsilon[iEpsilon])
			return true;
	}

	for(size_t iChr = 0; iChr < DIM(m_transitions); ++iChr)
	{
		if(m_transitions[iChr] == CNfa::PStateEmpty())
			return true;
	}

	return false;
}

void CNfaState::Bake(int cState)
{
	assert(!m_fBaked);
	
	m_fBaked = true;

	m_eclosure.SetSize(cState);

	// breadth first search for nodes we can reach by 0 or more epsilon transitions
	
	queue<const CNfaState*> qpNfas;
	qpNfas.push(this);

	while(qpNfas.size() > 0)
	{
		const CNfaState* pNfas = qpNfas.front();
		qpNfas.pop();

		if(!m_eclosure.At(pNfas->m_nId))
		{
			m_eclosure.Set(pNfas->m_nId);
		
			for(const CNfaState* pStateEpsilon : pNfas->m_aryEpsilon)
			{
				qpNfas.push(pStateEpsilon);
			}
		}
	}
}

const CDynBitAry * CNfaState::EpsilonClosure() const
{
	assert(m_fBaked);
	
	return &m_eclosure;
}

CNfa::SNfaFragment::SNfaFragment()
: m_pStateBegin(nullptr)
, m_aryUnpatched()
{
}

CNfa::SNfaFragment::SNfaFragment(CNfaState * pNfas, vector<CNfaState *> aryUnpatched)
: m_pStateBegin(pNfas)
, m_aryUnpatched(aryUnpatched)
{
}

void CNfa::SNfaFragment::Patch(CNfa::SNfaFragment nfa)
{
	for(CNfaState * Nfas : m_aryUnpatched)
	{
		Nfas->Patch(nfa.m_pStateBegin);
	}

	m_aryUnpatched.clear();

	// we cant just set m_arypStateUnpatched = nfa.m_arypStateUnpatched,
	// because m_arypStateUnpatched and nfa.m_arypStateUnpatched are not garenteed to be disjoint
	// so we may have already patched some of the nodes in nfa.m_arypStateUnpatched
	// BB (matthewd) hmm.... i dont like this
	
	for(CNfaState * Nfas : nfa.m_aryUnpatched)
	{
		if(Nfas->FUnpatched())
		{
			m_aryUnpatched.push_back(Nfas);
		}
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
			return FragFromRegexchr(pRegex->m_pChrData);
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
		aryUnpatched.insert(aryUnpatched.end(), nfaAlt.m_aryUnpatched.begin(), nfaAlt.m_aryUnpatched.end());
		pNfasOr->AddEpsilon(nfaAlt.m_pStateBegin);
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
		nfa = FragCreateCount(pQuant, pQuant->m_cMic);
	}

	if(pQuant->m_cMac == -1)
	{
		if(nfa.m_pStateBegin)
		{
			nfa.Patch(FragCreateStar(pQuant));
		}
		else
		{
			nfa = FragCreateStar(pQuant);
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
		nfa.Patch(FragCreateCountOptional(pQuant, pQuant->m_cMac - pQuant->m_cMic));
	}
	else
	{
		nfa = FragCreateCountOptional(pQuant, pQuant->m_cMac - pQuant->m_cMic);
	}

	return nfa;
}

CNfa::SNfaFragment CNfa::FragFromRange(const SRangeRegexData * pRange)
{
	CNfaState * pNfasOr = PStateCreate();

	for(int iChr = pRange->m_chrMic; iChr <= pRange->m_chrMac; ++iChr)
	{
		CNfaState * pNfasChr = PStateCreate();
		pNfasChr->AddTransition(iChr, CNfa::PStateEmpty());

		pNfasOr->AddEpsilon(pNfasChr);
	}

	return SNfaFragment(pNfasOr, pNfasOr->m_aryEpsilon);
}

CNfa::SNfaFragment CNfa::FragFromRegexchr(const SChrRegexData * pRegexchr)
{
	CNfaState * pNfasChr = PStateCreate();
	pNfasChr->AddTransition(pRegexchr->m_chr, CNfa::PStateEmpty());

	return SNfaFragment(pNfasChr, { pNfasChr });
}

CNfa::SNfaFragment CNfa::FragCreateCount(const SQuantifierRegexData * pQuant, int c)
{
	SConcatinationRegexData concat;

	for(int iC = 0; iC < c; ++ iC)
	{
		concat.m_aryRegex.push_back(pQuant->m_regex);
	}

	return FragFromConcat(&concat);
}

CNfa::SNfaFragment CNfa::FragCreateStar(const SQuantifierRegexData * pQuant)
{
	// create a ?
	
	CNfaState * pNfasOr = PStateCreate();
	SNfaFragment nfa = FragFromRegex(&pQuant->m_regex);
	pNfasOr->AddEpsilon(nfa.m_pStateBegin);
	pNfasOr->AddEpsilon(nullptr);

	// the final nfa will only have one unpatched node, the starting node, 
	// because the nfa created from m_pregex is patched back to pOrState

	SNfaFragment nfaStar(pNfasOr, { pNfasOr });

	// loop back the nfa created from m_pregex to pOrState
	// turning a ? into a *

	nfa.Patch(nfaStar);

	return nfaStar;
}

CNfa::SNfaFragment CNfa::FragCreateQMark(const SQuantifierRegexData * pQuant)
{
	// create a ?
	
	CNfaState * pNfasOr = PStateCreate();
	SNfaFragment nfa = FragFromRegex(&pQuant->m_regex);
	pNfasOr->AddEpsilon(nfa.m_pStateBegin);
	pNfasOr->AddEpsilon(nullptr);

	// the unpatched states are pOrState and all the unpatched states of
	// the nfa created from m_pRegex

	vector<CNfaState *> aryUnpatched;
	aryUnpatched.push_back(pNfasOr);
	aryUnpatched.insert(aryUnpatched.end(), nfa.m_aryUnpatched.begin(), nfa.m_aryUnpatched.end());

	return SNfaFragment(pNfasOr, aryUnpatched);
}

CNfa::SNfaFragment CNfa::FragCreateCountOptional(const SQuantifierRegexData * pQuant, int c)
{
	assert(c > 0);

	// create nested ?'s, starting on the right and working
	// back to the left to the starting node
	
	SNfaFragment nfaLeftMost;

	for(int iC = c; iC > 0; --iC)
	{
		SNfaFragment nfaQMark = FragCreateQMark(pQuant);

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
	CNfaState * pNfas = m_poolNfas.PTNew<CNfaState>();

	pNfas->m_nId = m_poolNfas.m_arypT.size() - 1;

	return pNfas;
}

void CNfa::Bake()
{
	for(CNfaState * pNfas : m_poolNfas.m_arypT)
	{
		pNfas->Bake(m_poolNfas.m_arypT.size());
	}
}

const CNfaState * CNfa::PStateFromId(int nId) const
{
	return m_poolNfas.m_arypT[nId];
}

CDynBitAry * CNfa::PBAryCreate()
{
	CDynBitAry * pBary = m_poolBary.PTNew<CDynBitAry>();
	pBary->SetSize(m_poolNfas.m_arypT.size());
	return pBary;
}

CNfaState CNfa::s_stateEmpty;

