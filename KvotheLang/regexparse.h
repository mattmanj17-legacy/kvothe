#pragma once

#include <assert.h>
#include <ctype.h>
#include <string>
using std::string;

#include <vector>
using std::vector;

#include "types.h"
#include "pool.h"

struct SUnionData;
struct SConcatinationData;
struct SQuantifierData;
struct SRangeData;
struct SRegexCharData;

enum REGEXK
{
	REGEXK_Union,
	REGEXK_Concat,
	REGEXK_Quant,
	REGEXK_Range,
	REGEXK_Chr,
};

struct SRegex // tag = regex
{
	void PrintDebug() {}
	REGEXK m_regexk;

	union
	{
		SUnionData * m_pUnion;
		SConcatinationData * m_pConcat;
		SQuantifierData * m_pQuant;
		SRangeData * m_pRange;
		SRegexCharData * m_pChr;
	};
};

struct SUnionData // tag = union
{
	vector<SRegex *> m_arypRegex;
	
	void PrintDebug();
};

struct SConcatinationData // tag = concat
{
	vector<SRegex *> m_arypRegex;
	
	void PrintDebug()
	{
		printf("(CONCAT");

		for(SRegex * pRegex : m_arypRegex)
		{
			printf(" ");
			pRegex->PrintDebug();
		}

		printf(")");
	}
};

struct SQuantifierData
{
	int m_cMic;
	int m_cMac;
	SRegex * m_pRegex;

	void PrintDebug()
	{
		printf("({%d, %d} ", m_cMic, m_cMac);

		m_pRegex->PrintDebug();

		printf(")");
	}
};

struct SRangeData
{
	u8 m_chrMic;
	u8 m_chrMac;

	void PrintDebug()
	{
		printf("(%d '%c' - %d '%c')", m_chrMic, m_chrMic, m_chrMac, m_chrMac);
	}	
};

struct SRegexCharData
{
	u8 m_chr;

	void PrintDebug()
	{
		printf("%d '%c'", m_chr, m_chr);
	}	
};

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
			SRegex * pRegex = PRegexCreate(REGEXK_Union);
			SUnionData * pUnion = pRegex->m_pUnion;
			pUnion->m_arypRegex.push_back(pConcat);
			
			while(ChrCur() == '|')
			{
				MatchChr('|');
				pUnion->m_arypRegex.push_back(ConcatParse());
			}

			return pRegex;
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
			SRegex * pRegex = PRegexCreate(REGEXK_Concat);
			SConcatinationData * pConcat = pRegex->m_pConcat;
			pConcat->m_arypRegex.push_back(pQuant);
			
			while(FChrCanBeginAtom(ChrCur()))
			{
				pConcat->m_arypRegex.push_back(QuantParse());
			}

			return pRegex;
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

	void ParseInitQuantExplicit(SQuantifierData * pQuant)
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

	void ParseInitQuant(SQuantifierData * pQuant)
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
			
			SRegex * pRegex = PRegexCreate(REGEXK_Quant);
			SQuantifierData * pQuant = pRegex->m_pQuant;
			pQuant->m_pRegex = pRegexCur;

			ParseInitQuant(pQuant);

			pRegexCur = pRegex;
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
			
			SRegex * pRegex = PRegexCreate(REGEXK_Range);
			SRangeData * pRange = pRegex->m_pRange;
			pRange->m_chrMic = 0;
			pRange->m_chrMac = 255;

			return pRegex;
		}
		else if(ChrCur() == '\\')
		{
			MatchChr('\\');

			SRegex * pRegex = PRegexCreate(REGEXK_Chr);
			SRegexCharData * pRegexchar = pRegex->m_pChr;
			pRegexchar->m_chr = ChrEscaped();

			return pRegex;
		}
		else
		{
			assert(FChrCanBeginAtom(ChrCur()));

			SRegex * pRegex = PRegexCreate(REGEXK_Chr);
			SRegexCharData * pRegexchar = pRegex->m_pChr;
			pRegexchar->m_chr = ChrConsume();

			return pRegex;
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

		SRegex * pRegex = PRegexCreate(REGEXK_Union);
		SUnionData * pUnion = pRegex->m_pUnion;
		
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
					SRegex * pRegex = PRegexCreate(REGEXK_Chr);
					SRegexCharData * pRegexchar = pRegex->m_pChr;
					pRegexchar->m_chr = chrBegin;
					pUnion->m_arypRegex.push_back(pRegex);
				}
				else
				{
					SRegex * pRegex = PRegexCreate(REGEXK_Range);
					SRangeData * pRange = pRegex->m_pRange;
					pRange->m_chrMic = chrBegin;
					pRange->m_chrMac = iChr;
					pUnion->m_arypRegex.push_back(pRegex);
				}
			}
		}

		// a set has to have at least one element

		assert(pUnion->m_arypRegex.size() > 0);

		return pRegex;
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

	SRegex* PRegexCreate(REGEXK regexk)
	{
		SRegex* pRegex = m_poolRegex.PTNew<SRegex>();

		pRegex->m_regexk = regexk;

		switch (regexk)
		{
			case REGEXK_Union:
				pRegex->m_pUnion = m_poolRegexData.PTNew<SUnionData>();
				break;

			case REGEXK_Concat:
				pRegex->m_pConcat = m_poolRegexData.PTNew<SConcatinationData>();
				break;

			case REGEXK_Quant:
				pRegex->m_pQuant = m_poolRegexData.PTNew<SQuantifierData>();
				break;

			case REGEXK_Range:
				pRegex->m_pRange = m_poolRegexData.PTNew<SRangeData>();
				break;

			case REGEXK_Chr:
				pRegex->m_pChr = m_poolRegexData.PTNew<SRegexCharData>();
				break;

			default:
				assert(false);
				break;
		}

		return pRegex;
	}

	u8	m_chrCur;
	FILE *			m_pFile;

	Pool<SRegex> m_poolRegex;

	VoidPool m_poolRegexData;
};
