#include <stdio.h>
#include <ctype.h>
#include <vector>
#include <string>
#include <stdio.h>
#include <assert.h>
#include <queue>

using std::vector;
using std::string;
using std::queue;

template<typename T>
struct Pool
{
	template<typename TDerived>
	TDerived* PCreate()
	{
		TDerived* p = new TDerived();
		m_aryp.push_back(p);

		return p;
	}

	void Clear()
	{
		for(T* p : m_aryp)
		{
			delete p;
		}

		m_aryp.clear();
	}
	
	vector<T*> m_aryp;
};

struct SNfaState;

struct SNfaTransition
{
	SNfaState * m_pStateNext = nullptr;
	unsigned char m_chr;
};

struct SNfaState
{
	static int idNext;
	int m_id;
	
	SNfaState()
	{
		m_id = idNext++;
	}

	void PrintDebug()
	{
		printf("\n");
		printf("state %d:\n", m_id);
		
		if(m_aryTran.size() > 0)
		{
			printf("transitions:");
			for(SNfaTransition tran : m_aryTran)
			{
				assert(tran.m_pStateNext);
				printf(" ");
				printf("%d '%c' -> %d", tran.m_chr, tran.m_chr, tran.m_pStateNext->m_id);
			}
			printf("\n");
		}

		if(m_aryPStateEpsilon.size() > 0)
		{
			printf("epsilon:");
			for(SNfaState * pState : m_aryPStateEpsilon)
			{
				assert(pState);
				printf(" %d", pState->m_id);
			}
			printf("\n");
		}
	}

	void AddTransition(unsigned char chr, SNfaState * pStateNext)
	{
		m_aryTran.push_back({pStateNext, chr});
	}

	void AddEpsilon(SNfaState * pStateNext)
	{
		m_aryPStateEpsilon.push_back(pStateNext);
	}

	void Patch(SNfaState * pState)
	{
		for(size_t iEpsilon = 0; iEpsilon < m_aryPStateEpsilon.size(); ++iEpsilon)
		{
			if(!m_aryPStateEpsilon[iEpsilon])
			{
				m_aryPStateEpsilon[iEpsilon] = pState;
			}
		}

		for(size_t iTran = 0; iTran < m_aryTran.size(); ++iTran)
		{
			if(!m_aryTran[iTran].m_pStateNext)
			{
				m_aryTran[iTran].m_pStateNext = pState;
			}
		}
	}

	bool FUnpatched()
	{
		// BB (matthewd) cache this value instead?
		
		for(size_t iEpsilon = 0; iEpsilon < m_aryPStateEpsilon.size(); ++iEpsilon)
		{
			if(!m_aryPStateEpsilon[iEpsilon])
				return true;
		}

		for(size_t iTran = 0; iTran < m_aryTran.size(); ++iTran)
		{
			if(!m_aryTran[iTran].m_pStateNext)
				return true;
		}

		return false;
	}

	bool m_fPatching = false;
	vector<SNfaState*> m_aryPStateEpsilon;
	vector<SNfaTransition> m_aryTran;
};
int SNfaState::idNext = 0;

Pool<SNfaState> m_poolState;

struct SNfa
{
	SNfaState * m_pStateBegin;

	vector<SNfaState *> m_arypStateUnpatched;

	void Patch(SNfa nfa)
	{
		for(SNfaState * pState : m_arypStateUnpatched)
		{
			pState->Patch(nfa.m_pStateBegin);
		}

		m_arypStateUnpatched.clear();

		// we cant just set m_arypStateUnpatched = nfa.m_arypStateUnpatched,
		// because m_arypStateUnpatched and nfa.m_arypStateUnpatched are not garenteed to be disjoint
		// so we may have already patched some of the nodes in nfa.m_arypStateUnpatched
		// BB (matthewd) hmm.... i dont like this
		
		for(SNfaState * pState : nfa.m_arypStateUnpatched)
		{
			if(pState->FUnpatched())
			{
				m_arypStateUnpatched.push_back(pState);
			}
		}
	}

	SNfa()
	: m_pStateBegin(nullptr)
	, m_arypStateUnpatched()
	{
	}

	SNfa(SNfaState * pStateBegin, vector<SNfaState *> arypStateUnpatched)
	: m_pStateBegin(pStateBegin)
	, m_arypStateUnpatched(arypStateUnpatched)
	{
	}
};

struct SRegex // tag = regex
{
	virtual void PrintDebug() = 0;

	virtual ~SRegex(){};

	virtual SNfa NfaCreate() = 0;
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

	SNfa NfaCreate() override
	{
		assert(m_arypRegex.size() > 0);

		SNfaState * pOrState = m_poolState.PCreate<SNfaState>();

		vector<SNfaState *> arypStateUnpatched;

		for(SRegex * pRegex : m_arypRegex)
		{
			SNfa nfaAlt = pRegex->NfaCreate();
			arypStateUnpatched.insert(arypStateUnpatched.end(), nfaAlt.m_arypStateUnpatched.begin(), nfaAlt.m_arypStateUnpatched.end());
			pOrState->AddEpsilon(nfaAlt.m_pStateBegin);
		}

		return SNfa(pOrState, arypStateUnpatched);
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

	SNfa NfaCreate() override
	{
		assert(m_arypRegex.size() > 0);

		SNfa nfa = m_arypRegex[0]->NfaCreate();

		for(size_t i = 1; i < m_arypRegex.size(); ++i)
		{
			nfa.Patch(m_arypRegex[i]->NfaCreate());
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

	SNfa NfaCreateCount(int c)
	{
		SConcatination concat;

		for(int iC = 0; iC < c; ++ iC)
		{
			concat.m_arypRegex.push_back(m_pRegex);
		}

		return concat.NfaCreate();
	}

	SNfa NfaCreateStar()
	{
		// create a ?
		
		SNfaState * pOrState = m_poolState.PCreate<SNfaState>();
		SNfa nfa = m_pRegex->NfaCreate();
		pOrState->AddEpsilon(nfa.m_pStateBegin);
		pOrState->AddEpsilon(nullptr);

		// the final nfa will only have one unpatched node, the starting node, 
		// because the nfa created from m_pregex is patched back to pOrState

		SNfa nfaStar(pOrState, { pOrState });

		// loop back the nfa created from m_pregex to pOrState
		// turning a ? into a *

		nfa.Patch(nfaStar);

		return nfaStar;
	}

	SNfa NfaCreateQMark()
	{
		// create a ?
		
		SNfaState * pOrState = m_poolState.PCreate<SNfaState>();
		SNfa nfa = m_pRegex->NfaCreate();
		pOrState->AddEpsilon(nfa.m_pStateBegin);
		pOrState->AddEpsilon(nullptr);

		// the unpatched states are pOrState and all the unpatched states of
		// the nfa created from m_pRegex

		vector<SNfaState *> arypStateUnpatched;
		arypStateUnpatched.push_back(pOrState);
		arypStateUnpatched.insert(arypStateUnpatched.end(), nfa.m_arypStateUnpatched.begin(), nfa.m_arypStateUnpatched.end());

		return SNfa(pOrState, arypStateUnpatched);
	}

	SNfa NfaCreateCountOptional(int c)
	{
		assert(c > 0);

		// create nested ?'s, starting on the right and working
		// back to the left to the starting node
		
		SNfa nfaLeftMost;

		for(int iC = c; iC > 0; --iC)
		{
			SNfa nfaQMark = NfaCreateQMark();

			if(nfaLeftMost.m_pStateBegin)
			{
				nfaQMark.Patch(nfaLeftMost);
			}

			nfaLeftMost = nfaQMark;
		}

		return nfaLeftMost;
	}

	SNfa NfaCreate() override
	{
		SNfa nfa;

		if(m_cMic > 0)
		{
			nfa = NfaCreateCount(m_cMic);
		}

		if(m_cMac == -1)
		{
			if(nfa.m_pStateBegin)
			{
				nfa.Patch(NfaCreateStar());
			}
			else
			{
				nfa = NfaCreateStar();
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
			nfa.Patch(NfaCreateCountOptional(m_cMac - m_cMic));
		}
		else
		{
			nfa = NfaCreateCountOptional(m_cMac - m_cMic);
		}

		return nfa;
	}
};

struct SRange : SRegex // tag = range
{
	unsigned char m_chrMic;
	unsigned char m_chrMac;

	void PrintDebug() override
	{
		printf("(%d '%c' - %d '%c')", m_chrMic, m_chrMic, m_chrMac, m_chrMac);
	}

	SNfa NfaCreate() override
	{
		SNfaState * pOrState = m_poolState.PCreate<SNfaState>();

		for(int iChr = m_chrMic; iChr <= m_chrMac; ++iChr)
		{
			SNfaState * pMatchState = m_poolState.PCreate<SNfaState>();
			pMatchState->AddTransition(iChr, nullptr);

			pOrState->AddEpsilon(pMatchState);
		}

		return SNfa(pOrState, pOrState->m_aryPStateEpsilon);
	}
};

struct SRegexChar : SRegex // tag = regexchr
{
	unsigned char m_chr;

	void PrintDebug() override
	{
		printf("%d '%c'", m_chr, m_chr);
	}

	SNfa NfaCreate() override
	{
		SNfaState * pMatchState = m_poolState.PCreate<SNfaState>();
		pMatchState->AddTransition(m_chr, nullptr);

		return SNfa(pMatchState, { pMatchState });
	}
};

Pool<SRegex> m_poolRegex;

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
			SUnion * pUnion = m_poolRegex.PCreate<SUnion>();
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

	bool FChrCanBeginAtom(unsigned char chr)
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
			SConcatination * pConcat = m_poolRegex.PCreate<SConcatination>();
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
			
			SQuantifier * pQuant = m_poolRegex.PCreate<SQuantifier>();
			pQuant->m_pRegex = pRegexCur;

			ParseInitQuant(pQuant);

			pRegexCur = pQuant;
		}

		// return the quantified regex (either the original atom, or nested quantifiers ending with the atom)

		return pRegexCur;
	}

	unsigned char ChrConsumeHex()
	{
		unsigned char chrHex = ChrConsume();

		assert(
			(chrHex >= '0' && chrHex <= '9') ||
			(chrHex >= 'a' && chrHex <= 'f') ||
			(chrHex >= 'A' && chrHex <= 'F')
		);

		return chrHex;
	}

	unsigned char ChrEscaped()
	{
		unsigned char chrEscape = ChrConsume();

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
			unsigned char chr = tolower(ChrConsume());
			return chr - 'a' + 1;
		}
		else if(chrEscape == 'x')
		{
			// hex byte
			
			string strHex = "";
			
			strHex += ChrConsumeHex();
			strHex += ChrConsumeHex();
			
			return (unsigned char) strtol(strHex.c_str(), nullptr, 16);
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

			return (unsigned char) strtol(strOctal.c_str(), nullptr, 8);
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
			
			SRange * pRange = m_poolRegex.PCreate<SRange>();
			pRange->m_chrMic = 0;
			pRange->m_chrMac = 255;

			return pRange;
		}
		else if(ChrCur() == '\\')
		{
			MatchChr('\\');

			SRegexChar * pRegexchar = m_poolRegex.PCreate<SRegexChar>();
			pRegexchar->m_chr = ChrEscaped();

			return pRegexchar;
		}
		else
		{
			assert(FChrCanBeginAtom(ChrCur()));

			SRegexChar * pRegexchar = m_poolRegex.PCreate<SRegexChar>();
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

	unsigned char ChrConsumeSet()
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
			unsigned char chrBegin = ChrConsumeSet();
			unsigned char chrEnd = chrBegin;

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

		SUnion * pUnion = m_poolRegex.PCreate<SUnion>();
		
		for(int iChr = 0; iChr < 256; ++iChr)
		{
			if(mpChrFIncluded[iChr])
			{
				// save the current chr, which is the begining or a range

				unsigned char chrBegin = iChr;

				// advance to the end of the range
				
				while(mpChrFIncluded[iChr + 1] && iChr + 1 < 256)
				{
					++iChr;
				}
				
				// add either a signle chr or or range to the union
				
				if(chrBegin == iChr)
				{
					SRegexChar * pRegexchar = m_poolRegex.PCreate<SRegexChar>();
					pRegexchar->m_chr = chrBegin;
					pUnion->m_arypRegex.push_back(pRegexchar);
				}
				else
				{
					SRange * pRange = m_poolRegex.PCreate<SRange>();
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
	
	unsigned char ChrCur()
	{
		return m_chrCur;
	}

	unsigned char ChrConsume()
	{
		unsigned char chrPrev = m_chrCur;
		m_chrCur = (unsigned char)fgetc(m_pFile);

		return chrPrev;
	}

	void MatchChr(char chrMatch)
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

	unsigned char	m_chrCur;
	FILE *			m_pFile;
};

int main()
{
	const char * pChzFileName = "example.regex";

	FILE * pFile = fopen(pChzFileName, "r");

	SParser parser;

	parser.SetInput(pFile);

	SRegex * pRegex = parser.RegexFileParse();

	printf("regex print debug:\n\n");

	pRegex->PrintDebug();

	SNfa nfa = pRegex->NfaCreate();

	SNfaState stateAccept;
	stateAccept.m_id = -1;

	nfa.Patch(SNfa(&stateAccept, {}));

	printf("\n\nNFA print debug:\n");

	for(SNfaState* pState : m_poolState.m_aryp)
	{
		pState->PrintDebug();
	}

	m_poolRegex.Clear();
	m_poolState.Clear();

	return 0;
}