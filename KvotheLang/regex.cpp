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
struct Pool // tag = pool
{
	template<typename TDerived>
	TDerived* PTNew()
	{
		TDerived* pT = new TDerived();
		m_arypT.push_back(pT);

		return pT;
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

struct SNfaTransition // tag = nfat
{
	SNfaState * m_pNfasNext = nullptr;
	unsigned char m_chr;
};

struct SNfaState // tag = nfas
{
	static int s_nIdNext;
	int m_nId;
	
	SNfaState()
	{
		m_nId = s_nIdNext++;
	}

	void PrintDebug()
	{
		printf("\n");
		printf("state %d:\n", m_nId);
		
		if(m_aryNfat.size() > 0)
		{
			printf("transitions:");
			for(SNfaTransition nfat : m_aryNfat)
			{
				assert(nfat.m_pNfasNext);
				printf(" ");
				printf("%d '%c' -> %d", nfat.m_chr, nfat.m_chr, nfat.m_pNfasNext->m_nId);
			}
			printf("\n");
		}

		if(m_arypNfasEpsilon.size() > 0)
		{
			printf("epsilon:");
			for(SNfaState * nfas : m_arypNfasEpsilon)
			{
				assert(nfas);
				printf(" %d", nfas->m_nId);
			}
			printf("\n");
		}
	}

	void AddTransition(unsigned char chr, SNfaState * pNfas)
	{
		m_aryNfat.push_back({pNfas, chr});
	}

	void AddEpsilon(SNfaState * pNfas)
	{
		m_arypNfasEpsilon.push_back(pNfas);
	}

	void Patch(SNfaState * pNfas)
	{
		for(size_t ipNfasEpsilon = 0; ipNfasEpsilon < m_arypNfasEpsilon.size(); ++ipNfasEpsilon)
		{
			if(!m_arypNfasEpsilon[ipNfasEpsilon])
			{
				m_arypNfasEpsilon[ipNfasEpsilon] = pNfas;
			}
		}

		for(size_t iNfat = 0; iNfat < m_aryNfat.size(); ++iNfat)
		{
			if(!m_aryNfat[iNfat].m_pNfasNext)
			{
				m_aryNfat[iNfat].m_pNfasNext = pNfas;
			}
		}
	}

	bool FUnpatched()
	{
		// BB (matthewd) cache this value instead?
		
		for(size_t ipNfasEpsilon = 0; ipNfasEpsilon < m_arypNfasEpsilon.size(); ++ipNfasEpsilon)
		{
			if(!m_arypNfasEpsilon[ipNfasEpsilon])
				return true;
		}

		for(size_t iNfat = 0; iNfat < m_aryNfat.size(); ++iNfat)
		{
			if(!m_aryNfat[iNfat].m_pNfasNext)
				return true;
		}

		return false;
	}

	vector<SNfaState*> AryPStateEpsilonClosure()
	{
		// breadth first search for nodes we can reach by 0 or more epsilon transitions
		
		vector<SNfaState*> arypNfasEpsilon;
		queue<SNfaState*> qpNfas;
		qpNfas.push(this);

		while(qpNfas.size() > 0)
		{
			SNfaState* pState = qpNfas.front();
			qpNfas.pop();

			if(std::find(arypNfasEpsilon.begin(), arypNfasEpsilon.end(), pState) == arypNfasEpsilon.end())
			{
				for(SNfaState* pStateEpsilon : pState->m_arypNfasEpsilon)
				{
					qpNfas.push(pStateEpsilon);
				}
			}
		}

		return arypNfasEpsilon;
	}

	vector<SNfaState*> m_arypNfasEpsilon;
	vector<SNfaTransition> m_aryNfat;
};
int SNfaState::s_nIdNext = 0;

Pool<SNfaState> g_poolNfas;

struct SNfa // tag = nfa
{
	SNfaState * m_pNfasBegin;
	vector<SNfaState *> m_arypNfasUnpatched;

	void Patch(SNfa nfa)
	{
		for(SNfaState * Nfas : m_arypNfasUnpatched)
		{
			Nfas->Patch(nfa.m_pNfasBegin);
		}

		m_arypNfasUnpatched.clear();

		// we cant just set m_arypStateUnpatched = nfa.m_arypStateUnpatched,
		// because m_arypStateUnpatched and nfa.m_arypStateUnpatched are not garenteed to be disjoint
		// so we may have already patched some of the nodes in nfa.m_arypStateUnpatched
		// BB (matthewd) hmm.... i dont like this
		
		for(SNfaState * Nfas : nfa.m_arypNfasUnpatched)
		{
			if(Nfas->FUnpatched())
			{
				m_arypNfasUnpatched.push_back(Nfas);
			}
		}
	}

	SNfa()
	: m_pNfasBegin(nullptr)
	, m_arypNfasUnpatched()
	{
	}

	SNfa(SNfaState * pNfas, vector<SNfaState *> arypNfasUnpatched)
	: m_pNfasBegin(pNfas)
	, m_arypNfasUnpatched(arypNfasUnpatched)
	{
	}
};

struct SDfaState;

struct SDfaTransition // tag = dfat
{
	SDfaState * m_pDfasNext = nullptr;
	unsigned char m_chr;
};

struct SDfaState // tag = dfas
{
	static int s_nIdNext;
	int m_nId;

	vector<SDfaTransition> m_aryTran;
};
int SDfaState::s_nIdNext = 0;

Pool<SDfaState> g_poolDfas;

//SDfaState * PState

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

		SNfaState * pNfasOr = g_poolNfas.PTNew<SNfaState>();

		vector<SNfaState *> arypNfasUnpatched;

		for(SRegex * pRegex : m_arypRegex)
		{
			SNfa nfaAlt = pRegex->NfaCreate();
			arypNfasUnpatched.insert(arypNfasUnpatched.end(), nfaAlt.m_arypNfasUnpatched.begin(), nfaAlt.m_arypNfasUnpatched.end());
			pNfasOr->AddEpsilon(nfaAlt.m_pNfasBegin);
		}

		return SNfa(pNfasOr, arypNfasUnpatched);
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
		
		SNfaState * pNfasOr = g_poolNfas.PTNew<SNfaState>();
		SNfa nfa = m_pRegex->NfaCreate();
		pNfasOr->AddEpsilon(nfa.m_pNfasBegin);
		pNfasOr->AddEpsilon(nullptr);

		// the final nfa will only have one unpatched node, the starting node, 
		// because the nfa created from m_pregex is patched back to pOrState

		SNfa nfaStar(pNfasOr, { pNfasOr });

		// loop back the nfa created from m_pregex to pOrState
		// turning a ? into a *

		nfa.Patch(nfaStar);

		return nfaStar;
	}

	SNfa NfaCreateQMark()
	{
		// create a ?
		
		SNfaState * pNfasOr = g_poolNfas.PTNew<SNfaState>();
		SNfa nfa = m_pRegex->NfaCreate();
		pNfasOr->AddEpsilon(nfa.m_pNfasBegin);
		pNfasOr->AddEpsilon(nullptr);

		// the unpatched states are pOrState and all the unpatched states of
		// the nfa created from m_pRegex

		vector<SNfaState *> arypNfasUnpatched;
		arypNfasUnpatched.push_back(pNfasOr);
		arypNfasUnpatched.insert(arypNfasUnpatched.end(), nfa.m_arypNfasUnpatched.begin(), nfa.m_arypNfasUnpatched.end());

		return SNfa(pNfasOr, arypNfasUnpatched);
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

			if(nfaLeftMost.m_pNfasBegin)
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
			if(nfa.m_pNfasBegin)
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
			assert(nfa.m_pNfasBegin);
			return nfa;
		}

		if(nfa.m_pNfasBegin)
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
		SNfaState * pNfasOr = g_poolNfas.PTNew<SNfaState>();

		for(int iChr = m_chrMic; iChr <= m_chrMac; ++iChr)
		{
			SNfaState * pNfasChr = g_poolNfas.PTNew<SNfaState>();
			pNfasChr->AddTransition(iChr, nullptr);

			pNfasOr->AddEpsilon(pNfasChr);
		}

		return SNfa(pNfasOr, pNfasOr->m_arypNfasEpsilon);
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
		SNfaState * pNfasChr = g_poolNfas.PTNew<SNfaState>();
		pNfasChr->AddTransition(m_chr, nullptr);

		return SNfa(pNfasChr, { pNfasChr });
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

		SUnion * pUnion = g_poolRegex.PTNew<SUnion>();
		
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

	SNfa nfa;
	
	{
		SRegex * pRegex = parser.RegexFileParse();

		nfa = pRegex->NfaCreate();

		// dont need the regex anymore
		
		g_poolRegex.Clear();
	}

	SNfaState stateAccept;
	stateAccept.m_nId = -1;

	nfa.Patch(SNfa(&stateAccept, {}));

	g_poolNfas.Clear();

	return 0;
}