#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#include <vector>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <string>

#include "bitary.h"
#include "macros.h"

using std::vector;
using std::queue;
using std::unordered_set;
using std::unordered_map;
using std::pair;
using std::string;

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

struct SDfaState // tag = dfas
{
	SDfaState()
	{
		m_nId = ++s_nIdNext;
	}

	void PrintDebug()
	{
		printf("state %d:\n", m_nId);

		if(m_fIsFinal)
		{
			printf("accepting state!\n");
		}
		
		if(m_transitions.size() > 0)
		{
			printf("transitions:\n");

			for(pair<u8, SDfaState*> transition : m_transitions)
			{
				assert(transition.second);
				printf("%d '%c' -> %d", transition.first, transition.first, transition.second->m_nId);
				printf("\n");
			}
		}
	}

	unordered_map<u8, SDfaState *> m_transitions;
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
	queue<CDynBitAry *> EClosureQ;
	unordered_set<CDynBitAry *, NHashPDynBitAry, NEqualsPDynBitAry> EClosureEverInQ;
	unordered_map<CDynBitAry *, unordered_map<u8, CDynBitAry *>, NHashPDynBitAry, NEqualsPDynBitAry> Move;

	CDynBitAry * eclosureBegin = pNfasBegin->EClosure();

	EClosureQ.push(eclosureBegin);
	EClosureEverInQ.insert(eclosureBegin);

	while(EClosureQ.size() > 0)
	{
		CDynBitAry * T = EClosureQ.front();
		EClosureQ.pop();

		Move[T] = unordered_map<u8, CDynBitAry *>();

		unordered_map<u8, CDynBitAry *> transitions;

		for(size_t iBit = 0; iBit < T->C(); ++iBit)
		{
			if(T->At(iBit))
			{
				SNfaState * pNfas = pBuilder->PNfasFromId(iBit);

				for(size_t iChr = 0; iChr < DIM(pNfas->m_transitions); ++iChr)
				{
					assert(pNfas->m_transitions[iChr] != nfasEmpty);

					if(pNfas->m_transitions[iChr])
					{
						if(transitions.count(iChr) == 0)
						{
							transitions[iChr] = pBuilder->PBAryCreate();
						}
					
						transitions[iChr]->Set(pNfas->m_transitions[iChr]->m_nId);
					}
				}
			}
		}

		for(pair<u8, CDynBitAry *> tran : transitions)
		{
			CDynBitAry * S = pBuilder->PBAryCreateUnpooled();
			
			for(size_t iBit = 0; iBit < tran.second->C(); ++ iBit)
			{
				if(tran.second->At(iBit))
				{
					SNfaState * pNfasTran = pBuilder->PNfasFromId(iBit);

					S->Union(pNfasTran->EClosure());
				}
			}

			if(EClosureEverInQ.count(S) == 0)
			{
				EClosureQ.push(S);
				EClosureEverInQ.insert(S);
				pBuilder->AddToPool(S);
			}
			else
			{
				CDynBitAry * SOld = S;
				S = *EClosureEverInQ.find(SOld);
				delete SOld;
			}

			Move[T][tran.first] = S;
		}
	}

	unordered_map<CDynBitAry *, SDfaState *, NHashPDynBitAry, NEqualsPDynBitAry> dfass;

	for(pair<CDynBitAry *, unordered_map<u8, CDynBitAry *>> move : Move)
	{
		dfass[move.first] = g_poolDfas.PTNew<SDfaState>();

		CDynBitAry * eclosure = move.first;

		for(size_t iBit = 0; iBit < eclosure->C(); ++iBit)
		{
			if(eclosure->At(iBit))
			{
				SNfaState * pNfas = pBuilder->PNfasFromId(iBit);

				if(pNfas == pNfasEnd)
				{
					dfass[move.first]->m_fIsFinal = true;
					break;
				}
			}
		}
	}

	for(pair<CDynBitAry *, unordered_map<u8, CDynBitAry *>> move : Move)
	{
		for(pair<u8, CDynBitAry *> tran : move.second)
		{
			assert(dfass.count(move.first));
			assert(dfass.count(tran.second));
			dfass[move.first]->m_transitions[tran.first] = dfass[tran.second];
		}
	}

	return dfass[eclosureBegin];
}

struct SRegex // tag = regex
{
	virtual void PrintDebug() = 0;

	virtual ~SRegex(){};

	virtual SNfaFragment NfafragCreate(SNfaBuilder * pBuilder) = 0;
};

struct SRegexList : public SRegex // tag = regexlist
{
	vector<SRegex *> m_arypRegex;
};

struct SUnion : public SRegexList // tag = union
{
	void PrintDebug() override
	{
		printf("(UNION");

		for(SRegex * pRegex : m_arypRegex)
		{
			printf(" ");
			pRegex->PrintDebug();
		}

		printf(")");
	}

	SNfaFragment NfafragCreate(SNfaBuilder * pBuilder) override
	{
		assert(m_arypRegex.size() > 0);

		SNfaState * pNfasOr = pBuilder->PNfasCreate();

		vector<SNfaState *> aryUnpatched;

		for(SRegex * pRegex : m_arypRegex)
		{
			SNfaFragment nfaAlt = pRegex->NfafragCreate(pBuilder);
			aryUnpatched.insert(aryUnpatched.end(), nfaAlt.m_aryUnpatched.begin(), nfaAlt.m_aryUnpatched.end());
			pNfasOr->AddEpsilon(nfaAlt.m_pStateBegin);
		}

		return SNfaFragment(pNfasOr, aryUnpatched);
	}
};

struct SConcatination : public SRegexList // tag = concat
{
	void PrintDebug() override
	{
		printf("(CONCAT");

		for(SRegex * pRegex : m_arypRegex)
		{
			printf(" ");
			pRegex->PrintDebug();
		}

		printf(")");
	}

	SNfaFragment NfafragCreate(SNfaBuilder * pBuilder) override
	{
		assert(m_arypRegex.size() > 0);

		SNfaFragment nfa = m_arypRegex[0]->NfafragCreate(pBuilder);

		for(size_t i = 1; i < m_arypRegex.size(); ++i)
		{
			nfa.Patch(m_arypRegex[i]->NfafragCreate(pBuilder));
		}

		return nfa;
	}
};

struct SQuantifier : public SRegex // tag = quant
{
	int m_cMic;
	int m_cMac;
	SRegex * m_pRegex;

	void PrintDebug() override
	{
		printf("({%d, %d} ", m_cMic, m_cMac);

		m_pRegex->PrintDebug();

		printf(")");
	}

	SNfaFragment NfaCreateCount(SNfaBuilder * pBuilder, int c)
	{
		SConcatination concat;

		for(int iC = 0; iC < c; ++ iC)
		{
			concat.m_arypRegex.push_back(m_pRegex);
		}

		return concat.NfafragCreate(pBuilder);
	}

	SNfaFragment NfaCreateStar(SNfaBuilder * pBuilder)
	{
		// create a ?
		
		SNfaState * pNfasOr = pBuilder->PNfasCreate();
		SNfaFragment nfa = m_pRegex->NfafragCreate(pBuilder);
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

	SNfaFragment NfaCreateQMark(SNfaBuilder * pBuilder)
	{
		// create a ?
		
		SNfaState * pNfasOr = pBuilder->PNfasCreate();
		SNfaFragment nfa = m_pRegex->NfafragCreate(pBuilder);
		pNfasOr->AddEpsilon(nfa.m_pStateBegin);
		pNfasOr->AddEpsilon(nullptr);

		// the unpatched states are pOrState and all the unpatched states of
		// the nfa created from m_pRegex

		vector<SNfaState *> aryUnpatched;
		aryUnpatched.push_back(pNfasOr);
		aryUnpatched.insert(aryUnpatched.end(), nfa.m_aryUnpatched.begin(), nfa.m_aryUnpatched.end());

		return SNfaFragment(pNfasOr, aryUnpatched);
	}

	SNfaFragment NfaCreateCountOptional(SNfaBuilder * pBuilder, int c)
	{
		assert(c > 0);

		// create nested ?'s, starting on the right and working
		// back to the left to the starting node
		
		SNfaFragment nfaLeftMost;

		for(int iC = c; iC > 0; --iC)
		{
			SNfaFragment nfaQMark = NfaCreateQMark(pBuilder);

			if(nfaLeftMost.m_pStateBegin)
			{
				nfaQMark.Patch(nfaLeftMost);
			}

			nfaLeftMost = nfaQMark;
		}

		return nfaLeftMost;
	}

	SNfaFragment NfafragCreate(SNfaBuilder * pBuilder) override
	{
		SNfaFragment nfa;

		if(m_cMic > 0)
		{
			nfa = NfaCreateCount(pBuilder, m_cMic);
		}

		if(m_cMac == -1)
		{
			if(nfa.m_pStateBegin)
			{
				nfa.Patch(NfaCreateStar(pBuilder));
			}
			else
			{
				nfa = NfaCreateStar(pBuilder);
			}

			return nfa;
		}

		if(m_cMac <= m_cMic)
		{
			assert(nfa.m_pStateBegin);
			return nfa;
		}

		if(nfa.m_pStateBegin)
		{
			nfa.Patch(NfaCreateCountOptional(pBuilder, m_cMac - m_cMic));
		}
		else
		{
			nfa = NfaCreateCountOptional(pBuilder, m_cMac - m_cMic);
		}

		return nfa;
	}
};

struct SRange : SRegex // tag = range
{
	u8 m_chrMic;
	u8 m_chrMac;

	void PrintDebug() override
	{
		printf("(%d '%c' - %d '%c')", m_chrMic, m_chrMic, m_chrMac, m_chrMac);
	}

	SNfaFragment NfafragCreate(SNfaBuilder * pBuilder) override
	{
		SNfaState * pNfasOr = pBuilder->PNfasCreate();

		for(int iChr = m_chrMic; iChr <= m_chrMac; ++iChr)
		{
			SNfaState * pNfasChr = pBuilder->PNfasCreate();
			pNfasChr->AddTransition(iChr, nfasEmpty);

			pNfasOr->AddEpsilon(pNfasChr);
		}

		return SNfaFragment(pNfasOr, pNfasOr->m_aryEpsilon);
	}
};

struct SRegexChar : SRegex // tag = regexchr
{
	u8 m_chr;

	void PrintDebug() override
	{
		printf("%d '%c'", m_chr, m_chr);
	}

	SNfaFragment NfafragCreate(SNfaBuilder * pBuilder) override
	{
		SNfaState * pNfasChr = pBuilder->PNfasCreate();
		pNfasChr->AddTransition(m_chr, nfasEmpty);

		return SNfaFragment(pNfasChr, { pNfasChr });
	}
};

Pool<SRegex> g_poolRegex;

struct SParser
{
	SRegex * RegexFileParse()
	{
		SRegex * pRegex = RegexParse();

		assert(fgetc(m_pFile) == EOF);

		return pRegex;
	}
	
	SRegex * RegexParse()
	{
		MatchChr('(');

		SRegex * pUnion = UnionParse();

		MatchChr(')');

		return pUnion;
	}

	SRegex * UnionParse()
	{
		SRegex * pConcat = ConcatParse();

		// if this is a union (has a '|' operator), create a union, add all the concats, and return it
		
		if(ChrCur() == '|')
		{
			SUnion * pUnion = g_poolRegex.PTNew<SUnion>();
			pUnion->m_arypRegex.push_back(pConcat);
			
			while(ChrCur() == '|')
			{
				MatchChr('|');
				pUnion->m_arypRegex.push_back(ConcatParse());
			}

			return pUnion;
		}

		// otherwise, just return the concat we parsed

		return pConcat;
	}

	bool FChrCanBeginAtom(u8 chr)
	{
		return 
			ChrCur() != ')' &&
			ChrCur() != '|' &&
			ChrCur() != '*' &&
			ChrCur() != '+' &&
			ChrCur() != '?' &&
			ChrCur() != '{' &&
			ChrCur() != '}' &&
			ChrCur() != ']' &&
			!iscntrl(ChrCur());
	}

	SRegex * ConcatParse()
	{
		SRegex * pQuant = QuantParse();

		// if this is a concat (has a more than one quant in a row), create a concat, add all the quants, and return it

		if(FChrCanBeginAtom(ChrCur()))
		{
			SConcatination * pConcat = g_poolRegex.PTNew<SConcatination>();
			pConcat->m_arypRegex.push_back(pQuant);
			
			while(FChrCanBeginAtom(ChrCur()))
			{
				pConcat->m_arypRegex.push_back(QuantParse());
			}

			return pConcat;
		}

		// otherwise, just return the quant we parsed

		return pQuant;
	}

	int NParse()
	{
		assert(isdigit(ChrCur()));
		
		string strNum = "";

		while(isdigit(ChrCur()))
		{
			strNum += ChrConsume();
		}

		return atoi(strNum.c_str());
	}

	void ParseInitQuantExplicit(SQuantifier * pQuant)
	{
		// handle {,} syntax
		
		MatchChr('{');

		// three valid versions
		// {N} exactly N times. N > 0
		// {N,} at least N times. N >= 0
		// {N1, N2} N1 to N2 times. N1 >= 0 and N2 > N1

		assert(isdigit(ChrCur()));

		pQuant->m_cMic = NParse();

		assert(pQuant->m_cMic > 0 || ChrCur() == ',');

		if(ChrCur() == ',')
		{
			MatchChr(',');

			if(isdigit(ChrCur()))
			{
				pQuant->m_cMac = NParse();
				assert(pQuant->m_cMac > pQuant->m_cMic);
			}
			else
			{
				// {N,} case

				pQuant->m_cMac = -1;
			}
		}
		else
		{
			// {N} case

			pQuant->m_cMac = pQuant->m_cMic;
		}
		
		MatchChr('}');
	}

	void ParseInitQuant(SQuantifier * pQuant)
	{
		// set cMic and cMac based on the current quantifier
		
		if(ChrCur() == '*')
		{
			MatchChr('*');
			pQuant->m_cMic = 0;
			pQuant->m_cMac = -1;
		}	
		else if(ChrCur() == '+')
		{
			MatchChr('+');
			pQuant->m_cMic = 1;
			pQuant->m_cMac = -1;
		}
		else if(ChrCur() == '?')
		{
			MatchChr('?');
			pQuant->m_cMic = 0;
			pQuant->m_cMac = 1;
		}
		else
		{
			// {,} case
			
			ParseInitQuantExplicit(pQuant);
		}
	}

	SRegex * QuantParse()
	{
		SRegex * pRegexCur = AtomParse();
			
		while(
			ChrCur() == '*' ||
			ChrCur() == '+' ||
			ChrCur() == '?' ||
			ChrCur() == '{' 
		)
		{
			// while there is a chr that begins a quantification, 
			// make a new quantifier quantifieing the regex to the left
			
			SQuantifier * pQuant = g_poolRegex.PTNew<SQuantifier>();
			pQuant->m_pRegex = pRegexCur;

			ParseInitQuant(pQuant);

			pRegexCur = pQuant;
		}

		// return the quantified regex (either the original atom, or nested quantifiers ending with the atom)

		return pRegexCur;
	}

	u8 ChrConsumeHex()
	{
		u8 chrHex = ChrConsume();

		assert(
			(chrHex >= '0' && chrHex <= '9') ||
			(chrHex >= 'a' && chrHex <= 'f') ||
			(chrHex >= 'A' && chrHex <= 'F')
		);

		return chrHex;
	}

	u8 ChrEscaped()
	{
		u8 chrEscape = ChrConsume();

		if(chrEscape == 'a')		return '\a';
		else if(chrEscape == 'b')	return '\b';
		else if(chrEscape == 'e')	return 27; // '\e'
		else if(chrEscape == 'f')	return '\f';
		else if(chrEscape == 'n')	return '\n';
		else if(chrEscape == 'r')	return '\r';
		else if(chrEscape == 't')	return '\t';
		else if(chrEscape == 'v')	return '\v';
		else if(chrEscape == 'c')
		{
			// \cA control character
			
			assert(isalpha(ChrCur()));
			u8 chr = tolower(ChrConsume());
			return chr - 'a' + 1;
		}
		else if(chrEscape == 'x')
		{
			// hex byte
			
			string strHex = "";
			
			strHex += ChrConsumeHex();
			strHex += ChrConsumeHex();
			
			return (u8) strtol(strHex.c_str(), nullptr, 16);
		}
		else if(isdigit(chrEscape))
		{
			// octal byte
			
			string strOctal = "";
			strOctal += chrEscape;

			int cDigit = 0;

			while(isdigit(ChrCur()) && cDigit < 2)
			{
				strOctal += ChrConsume();
				++cDigit;
			}

			return (u8) strtol(strOctal.c_str(), nullptr, 8);
		}
		else
		{
			assert(
				chrEscape == '.' ||
				chrEscape == '\\' ||
				chrEscape == '(' ||
				chrEscape == ')' ||
				chrEscape == '|' ||
				chrEscape == '*' ||
				chrEscape == '+' ||
				chrEscape == '?' ||
				chrEscape == '{' ||
				chrEscape == '}' || 
				chrEscape == '[' ||
				chrEscape == ']'
			);

			return chrEscape;
		}
	}

	SRegex * AtomParse()
	{
		if(ChrCur() == '(')
		{
			return RegexParse();
		}
		else if(ChrCur() == '[')
		{
			return SetParse();
		}
		else if(ChrCur() == '.')
		{
			MatchChr('.');
			
			SRange * pRange = g_poolRegex.PTNew<SRange>();
			pRange->m_chrMic = 0;
			pRange->m_chrMac = 255;

			return pRange;
		}
		else if(ChrCur() == '\\')
		{
			MatchChr('\\');

			SRegexChar * pRegexchar = g_poolRegex.PTNew<SRegexChar>();
			pRegexchar->m_chr = ChrEscaped();

			return pRegexchar;
		}
		else
		{
			assert(FChrCanBeginAtom(ChrCur()));

			SRegexChar * pRegexchar = g_poolRegex.PTNew<SRegexChar>();
			pRegexchar->m_chr = ChrConsume();

			return pRegexchar;
		}
	}

	bool FChrIsValidSetChrStart()
	{
		return 
			ChrCur() != '[' && 
			ChrCur() != ']' && 
			ChrCur() != '-' && 
			!iscntrl(ChrCur());
	}

	u8 ChrConsumeSet()
	{
		assert(FChrIsValidSetChrStart());
		
		if(ChrCur() == '\\')
		{
			MatchChr('\\');

			// '-' is an escape char in a set
			
			return ChrCur() == '-' ? ChrConsume() : ChrEscaped();
		}
			
		return ChrConsume();
	}

	SRegex * SetParse()
	{
		MatchChr('[');

		// check if this set is a negation
		
		bool fNegate = false;

		if(ChrCur() == '^')
		{
			MatchChr('^');
			fNegate = true;
		}

		// store elements of this set in a map from chr to bool (to convert to ranges later)
		
		bool mpChrFIncluded[256];

		for(int iChr = 0; iChr < 256; ++iChr)
		{
			mpChrFIncluded[iChr] = fNegate;
		}

		assert(FChrIsValidSetChrStart());
		while(FChrIsValidSetChrStart())
		{
			u8 chrBegin = ChrConsumeSet();
			u8 chrEnd = chrBegin;

			if(ChrCur() == '-')
			{
				MatchChr('-');
				chrEnd = ChrConsumeSet();
				assert(chrEnd > chrBegin);
			}

			for(; chrBegin <= chrEnd; ++chrBegin)
			{
				mpChrFIncluded[chrBegin] = !fNegate;
			}
		}

		MatchChr(']');

		// convert mpChrF to a union of ranges

		SUnion * pUnion = g_poolRegex.PTNew<SUnion>();
		
		for(int iChr = 0; iChr < 256; ++iChr)
		{
			if(mpChrFIncluded[iChr])
			{
				// save the current chr, which is the begining or a range

				u8 chrBegin = iChr;

				// advance to the end of the range
				
				while(mpChrFIncluded[iChr + 1] && iChr + 1 < 256)
				{
					++iChr;
				}
				
				// add either a signle chr or or range to the union
				
				if(chrBegin == iChr)
				{
					SRegexChar * pRegexchar = g_poolRegex.PTNew<SRegexChar>();
					pRegexchar->m_chr = chrBegin;
					pUnion->m_arypRegex.push_back(pRegexchar);
				}
				else
				{
					SRange * pRange = g_poolRegex.PTNew<SRange>();
					pRange->m_chrMic = chrBegin;
					pRange->m_chrMac = iChr;
					pUnion->m_arypRegex.push_back(pRange);
				}
			}
		}

		// a set has to have at least one element

		assert(pUnion->m_arypRegex.size() > 0);

		return pUnion;
	}
	
	u8 ChrCur()
	{
		return m_chrCur;
	}

	u8 ChrConsume()
	{
		u8 chrPrev = m_chrCur;
		m_chrCur = (u8)fgetc(m_pFile);

		return chrPrev;
	}

	void MatchChr(u8 chrMatch)
	{
		assert(ChrCur() == chrMatch);

		(void) ChrConsume();
	}

	void SetInput(FILE * pFile)
	{
		m_pFile = pFile;
		fseek(m_pFile, 0, SEEK_SET);
		(void) ChrConsume();
	}

	u8	m_chrCur;
	FILE *			m_pFile;
};

int main()
{
	SNfaState s;
	nfasEmpty = &s;
	
	SDfaState * pDfas = nullptr;
	
	SNfaBuilder nfaBuilder;

	{
		SNfaFragment nfa;
	
		{
			const char * pChzFileName = "example.regex";

			FILE * pFile = fopen(pChzFileName, "r");
			
			SParser parser;

			parser.SetInput(pFile);
			
			SRegex * pRegex = parser.RegexFileParse();

			fclose(pFile);

			nfa = pRegex->NfafragCreate(&nfaBuilder);

			// dont need the regex anymore
		
			g_poolRegex.Clear();
		}

		SNfaState * pStateAccept = nfaBuilder.PNfasCreate();

		nfa.Patch(SNfaFragment(pStateAccept, {}));

		nfaBuilder.Bake();

		pDfas = DfaFromNfa(&nfaBuilder, nfa.m_pStateBegin, pStateAccept);
	}

	//printf("start state: %d", pDfas->m_nId);

	//for(SDfaState * pDfas : g_poolDfas.m_arypT)
	//{
		//printf("\n");
		//pDfas->PrintDebug();
		//printf("\n");
	//}

	//g_poolDfas.Clear();

	return 0;
}