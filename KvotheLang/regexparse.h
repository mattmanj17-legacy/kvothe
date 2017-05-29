#pragma once

#include <assert.h>
#include <ctype.h>
#include <string>
using std::string;

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
