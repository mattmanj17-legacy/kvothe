#include <stdio.h>
#include <ctype.h>
#include <vector>
#include <string>
#include <stdio.h>
#include <assert.h>

using std::vector;
using std::string;

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

struct State;

struct StateTransitions
{
	State* m_mpChrState [256] = { nullptr };
	vector<State*> m_arypStateEpsilon;
};

struct State
{
	void Patch(State * pState)
	{
		if(m_fPatched)
			return;

		m_fPatched = true;

		PatchImpl(pState);
	}

	virtual void PatchImpl(State * pState) = 0;

	StateTransitions * PStateTransitions()
	{
		if(!m_statetransitions)
		{
			m_statetransitions = m_fAccepting ? new StateTransitions() : PStateTransitionsImpl();
		}

		return m_statetransitions;
	}

	virtual StateTransitions * PStateTransitionsImpl() = 0;

	~State()
	{
		if(m_statetransitions)
			delete m_statetransitions;
	}

	StateTransitions * m_statetransitions = nullptr;

	bool m_fPatched = false;
	bool m_fAccepting = false;
};

Pool<State> m_poolState;

struct OrState : State
{
	State * m_pState1 = nullptr;
	State * m_pState2 = nullptr;
	
	void PatchImpl(State * pState) override
	{
		assert(pState);
		
		if(m_pState1) m_pState1->Patch(pState); else m_pState1 = pState;
		if(m_pState2) m_pState2->Patch(pState); else m_pState2 = pState;
	}

	StateTransitions * PStateTransitionsImpl() override
	{
		assert(m_pState1);
		assert(m_pState2);
		
		StateTransitions * pSt = new StateTransitions();

		pSt->m_arypStateEpsilon.push_back(m_pState1);
		pSt->m_arypStateEpsilon.push_back(m_pState2);

		return pSt;
	}
};

struct MatchState : State
{
	State * m_pStateNext = nullptr;
	unsigned char m_chr;

	void PatchImpl(State * pState) override
	{
		assert(pState != nullptr);
		
		if(m_pStateNext)
		{
			m_pStateNext->Patch(pState);
		}
		else
		{
			m_pStateNext = pState;
		}
	}

	StateTransitions * PStateTransitionsImpl() override
	{
		assert(m_pStateNext);
		
		StateTransitions * pSt = new StateTransitions();

		pSt->m_mpChrState[m_chr] = m_pStateNext;

		return pSt;
	}
};

struct SRegex // tag = regex
{
	virtual void PrintDebug() = 0;

	virtual ~SRegex() {};

	virtual State * PStateCreate() = 0;
};

struct SRegexList : public SRegex // tag = regexlist
{
	vector<SRegex *> m_arypRegex;
};

struct SUnion : public SRegexList // tag = union
{
	void PrintDebug() override
	{
		printf("(UNION ");

		for(SRegex * pRegex : m_arypRegex)
		{
			pRegex->PrintDebug();
		}

		printf(")");
	}

	State * PStateCreateImpl(unsigned int iMic)
	{
		assert(iMic >= 0 && iMic < m_arypRegex.size());

		if(iMic == m_arypRegex.size() - 1)
			return m_arypRegex[iMic]->PStateCreate();

		OrState * pOrState = m_poolState.PCreate<OrState>();

		pOrState->m_pState1 = m_arypRegex[iMic]->PStateCreate();

		pOrState->m_pState2 = PStateCreateImpl(iMic + 1);

		return pOrState;
	}

	State * PStateCreate() override
	{
		assert(m_arypRegex.size() > 0);

		return PStateCreateImpl(0);
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

	State * PStateCreateImpl(unsigned int iMic)
	{
		assert(iMic >= 0 && iMic < m_arypRegex.size());

		if(iMic == m_arypRegex.size() - 1)
			return m_arypRegex[iMic]->PStateCreate();
		
		State * pStateStart = m_arypRegex[iMic]->PStateCreate();

		pStateStart->Patch(PStateCreateImpl(iMic + 1));

		return pStateStart;
	}

	State * PStateCreate() override
	{
		assert(m_arypRegex.size() > 0);

		return PStateCreateImpl(0);
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

	State * PStateCreateCount(int c)
	{
		SConcatination concat;

		for(int iC = 0; iC < c; ++ iC)
		{
			concat.m_arypRegex.push_back(m_pRegex);
		}

		State * pState = concat.PStateCreate();

		concat.m_arypRegex.clear();

		return pState;
	}

	State * PStateCreateStar()
	{
		State * pState;

		State * pStateQMark = PStateCreateQMark(&pState);

		pState->Patch(pStateQMark);

		return pStateQMark;
	}

	State * PStateCreateQMark(State ** ppState = nullptr)
	{
		OrState * pState = m_poolState.PCreate<OrState>();
		pState->m_pState1 = m_pRegex->PStateCreate();

		if(ppState)
			(*ppState) = pState->m_pState1;

		return pState;
	}

	State * PStateCreateCountOptional(int c)
	{
		assert(c > 0);
		
		State * pState;

		State * pStateQMark = PStateCreateQMark(&pState);

		if(c > 1)
		{
			pState->Patch(PStateCreateCountOptional(c-1));
		}

		return pStateQMark;
	}

	State * PStateCreate() override
	{
		State * pState = nullptr;

		if(m_cMic > 0)
		{
			pState = PStateCreateCount(m_cMic);
		}

		if(m_cMac == -1)
		{
			if(pState)
			{
				pState->Patch(PStateCreateStar());
			}
			else
			{
				pState = PStateCreateStar();
			}

			return pState;
		}

		if(m_cMac <= m_cMic)
		{
			assert(pState);
			return pState;
		}

		if(pState)
		{
			pState->Patch(PStateCreateCountOptional(m_cMac - m_cMic));
		}
		else
		{
			pState = PStateCreateCountOptional(m_cMac - m_cMic);
		}

		return pState;
	}
};

struct SRange : SRegex // tag = range
{
	unsigned char m_chrMic;
	unsigned char m_chrMac;

	void PrintDebug() override
	{
		printf("('%c' - '%c')", m_chrMic, m_chrMac);
	}

	OrState * PStateCreateImpl(unsigned char chrMic, unsigned char chrMac)
	{
		assert(chrMic < chrMac);

		OrState * pOrState = m_poolState.PCreate<OrState>();

		MatchState * pMatch = m_poolState.PCreate<MatchState>();
		pMatch->m_chr = chrMic;

		pOrState->m_pState1 = pMatch;

		if(chrMac == chrMic + 1)
		{
			pMatch = m_poolState.PCreate<MatchState>();
			pMatch->m_chr = chrMac;

			pOrState->m_pState2 = pMatch;
		}
		else
		{
			pOrState->m_pState2 = PStateCreateImpl(chrMic + 1, chrMac);
		}

		return pOrState;
	}

	State * PStateCreate() override
	{
		return PStateCreateImpl(m_chrMic, m_chrMac);
	}
};

struct SRegexChar : SRegex // tag = regexchr
{
	unsigned char m_chr;

	void PrintDebug() override
	{
		printf("'%c'", m_chr);
	}

	State * PStateCreate() override
	{
		MatchState * pMatch = m_poolState.PCreate<MatchState>();
		pMatch->m_chr = m_chr;

		return pMatch;
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

		pQuant->m_cMic = 0;

		// three valid versions
		// {N} exactly N times. N > 0
		// {N1,N2} N1 to N2 times. N1 > 0 and N2 > N1
		// {,N} 0 to N times. N > 0
		
		if(isdigit(ChrCur()))
		{
			// {N} or {N1,N2} case
			
			pQuant->m_cMic = NParse();
			assert(pQuant->m_cMic > 0);
		}

		if(pQuant->m_cMic == 0 || ChrCur() == ',')
		{
			// {N1,N2} or {,N} case
			
			MatchChr(',');
			pQuant->m_cMac = NParse();
			assert(pQuant->m_cMac > pQuant->m_cMic);
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
		char chrEscape = ChrConsume();

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
			char chr = tolower(ChrConsume());
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
				ChrCur() == '.' ||
				ChrCur() == '\\' ||
				ChrCur() == '(' ||
				ChrCur() == ')' ||
				ChrCur() == '|' ||
				ChrCur() == '*' ||
				ChrCur() == '+' ||
				ChrCur() == '?' ||
				ChrCur() == '{' ||
				ChrCur() == '}' || 
				ChrCur() == '[' ||
				ChrCur() == ']'
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
		
		bool mpChrFIncluded[256] = { fNegate };

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
		char chrPrev = m_chrCur;
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

	pRegex->PrintDebug();

	State * pState = pRegex->PStateCreate();

	MatchState * pMatch = m_poolState.PCreate<MatchState>();
	pMatch->m_fAccepting = true;

	pState->Patch(pMatch);

	vector<StateTransitions *> arypTran;

	m_poolRegex.Clear();
	m_poolState.Clear();

	return 0;
}