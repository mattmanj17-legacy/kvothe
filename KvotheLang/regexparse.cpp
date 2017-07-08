#include "regexparse.h"

#include <assert.h>
#include <ctype.h>

#include <sstream>
using std::ostringstream;

void SRegexAstNode::PrintDebug() const
{
	switch (m_regexk)
	{
		case REGEXK_Union:
			{
				printf("(|");

				for(SRegexAstNode regex : m_pUnionData->m_aryRegex)
				{
					printf(" ");
					regex.PrintDebug();
				}

				printf(")");
			}
			break;

		case REGEXK_Concat:
			{
				printf("(+");

				for(SRegexAstNode regex : m_pConcatData->m_aryRegex)
				{
					printf(" ");
					regex.PrintDebug();
				}

				printf(")");
			}
			break;

		case REGEXK_Quant:
			{
				printf("({%d, %d} ", m_pQuantData->m_cMic, m_pQuantData->m_cMac);

				m_pQuantData->m_regex.PrintDebug();

				printf(")");
			}
			break;

		case REGEXK_Range:
			printf("(%d - %d)", m_pRangeData->m_chrMic, m_pRangeData->m_chrMac);
			break;

		case REGEXK_Chr:
			printf("%d", m_pChrData->m_chr, m_pChrData->m_chr);
			break;

		case REGEXK_Self:
			printf("@");
			break;

		default:
			assert(false);
			break;
	}
}

string StrUnescape(u8 chr)
{
	if(chr == '\a')
	{
		return "\\a";
	}
	else if(chr == '\b')
	{
		return "\\b";
	}
	else if(chr == 27)
	{
		return "\\e";
	}
	else if(chr == '\f')
	{
		return "\\f";
	}
	else if(chr == '\n')
	{
		return "\\n";
	}
	else if(chr == '\r')
	{
		return "\\r";
	}
	else if(chr == '\t')
	{
		return "\\t";
	}
	else if(chr == '\v')
	{
		return "\\v";
	}
	else if(chr == '.')
	{
		return "\\.";
	}
	else if(chr == '\\')
	{
		return "\\\\";
	}
	else if(chr == '(')
	{
		return "\\(";
	}
	else if(chr == ')')
	{
		return "\\)";
	}
	else if(chr == '|')
	{
		return "\\|";
	}
	else if(chr == '*')
	{
		return "\\*";
	}
	else if(chr == '+')
	{
		return "\\+";
	}
	else if(chr == '?')
	{
		return "\\?";
	}
	else if(chr == '{')
	{
		return "\\{";
	}
	else if(chr == '}')
	{
		return "\\}";
	}
	else if(chr == '[')
	{
		return "\\[";
	}
	else if(chr == ']')
	{
		return "\\]";
	}
	else if(chr == '@')
	{
		return "\\@";
	}
	else if(chr >= 1 && chr <= 26)
	{
		ostringstream strStreamRet;
		strStreamRet << "\\c";
		u8 chrUpper = chr + 'A' - 1;
		strStreamRet << chrUpper;
		return strStreamRet.str();
	}
	else if(chr >= ' ' && chr <= '~')
	{
		return string(1, chr);
	}
	else
	{
		ostringstream strStreamRet;
		strStreamRet << "\\x" << std::hex << std::uppercase << (u32)chr;
		return strStreamRet.str();
	}
}

string StrUnescapeSet(u8 chr)
{
	if(chr == '-')
		return "\\-";

	return StrUnescape(chr);
}

string SRegexAstNode::StrPretty() const
{
	ostringstream strStreamRet;
	
	switch (m_regexk)
	{
		case REGEXK_Union:
			{
				for(int iRegex = 0; iRegex < m_pUnionData->m_aryRegex.size(); ++iRegex)
				{
					SRegexAstNode regex = m_pUnionData->m_aryRegex[iRegex];

					if(iRegex > 0)
					{
						strStreamRet << "|";
					}

					strStreamRet << regex.StrPretty();
				}
			}
			break;

		case REGEXK_Concat:
			{
				for(SRegexAstNode regex : m_pConcatData->m_aryRegex)
				{
					if(regex.m_regexk == REGEXK_Union)
					{
						strStreamRet << '(';
					}
					
					strStreamRet << regex.StrPretty();

					if(regex.m_regexk == REGEXK_Union)
					{
						strStreamRet << ')';
					}
				}
			}
			break;

		case REGEXK_Quant:
			{
				SRegexAstNode regex = m_pQuantData->m_regex;

				if(regex.m_regexk == REGEXK_Union || regex.m_regexk == REGEXK_Concat)
				{
					strStreamRet << '(';
				}

				strStreamRet << m_pQuantData->m_regex.StrPretty();

				if(regex.m_regexk == REGEXK_Union || regex.m_regexk == REGEXK_Concat)
				{
					strStreamRet << ')';
				}

				strStreamRet << "{" << m_pQuantData->m_cMic;
				if(m_pQuantData->m_cMac != m_pQuantData->m_cMic)
				{
					strStreamRet << ",";
					if(m_pQuantData->m_cMac != -1)
					{
						strStreamRet << m_pQuantData->m_cMac;
					}
				}
				strStreamRet << "}";
			}
			break;

		case REGEXK_Range:
			strStreamRet << "[" << StrUnescapeSet(m_pRangeData->m_chrMic) << "-" << StrUnescapeSet(m_pRangeData->m_chrMac) << "]";
			break;

		case REGEXK_Chr:
			strStreamRet << StrUnescape(m_pChrData->m_chr);
			break;

		case REGEXK_Self:
			strStreamRet << '@';
			break;

		default:
			assert(false);
			break;
	}

	return strStreamRet.str();
}

void CRegexParser::ParseFile(FILE * pFile)
{
	m_poolRegexData.Clear();
	
	m_pFile = pFile;

	m_chrCur = (u8)fgetc(m_pFile);	
	
	m_regexAstParsed = UnionParse();

	assert(fgetc(m_pFile) == EOF);
}

const SRegexAstNode * CRegexParser::PRegexAstParsed()
{
	if(m_regexAstParsed.m_regexk != REGEXK_Nil)
		return &m_regexAstParsed;
	else
		return nullptr;
}

SRegexAstNode CRegexParser::UnionParse()
{
	SRegexAstNode regexConcat = ConcatParse();

	// if this is a union (has a '|' operator), create a union, add all the concats, and return it
	
	if(ChrPeek() == '|')
	{
		SRegexAstNode regexUnion = RegexCreate(REGEXK_Union);
		SUnionRegexData * pUnionData = regexUnion.m_pUnionData;
		pUnionData->m_aryRegex.push_back(regexConcat);
		
		while(ChrPeek() == '|')
		{
			MatchChr('|');
			pUnionData->m_aryRegex.push_back(ConcatParse());
		}

		return regexUnion;
	}

	// otherwise, just return the concat we parsed

	return regexConcat;
}

SRegexAstNode CRegexParser::ConcatParse()
{
	SRegexAstNode regexQuant = QuantParse();

	// if this is a concat (has a more than one quant in a row), create a concat, add all the quants, and return it

	if(FChrCanBeginAtom(ChrPeek()))
	{
		SRegexAstNode regexConcat = RegexCreate(REGEXK_Concat);
		SConcatinationRegexData * pConcatData = regexConcat.m_pConcatData;
		pConcatData->m_aryRegex.push_back(regexQuant);
		
		while(FChrCanBeginAtom(ChrPeek()))
		{
			pConcatData->m_aryRegex.push_back(QuantParse());
		}

		return regexConcat;
	}

	// otherwise, just return the quant we parsed

	return regexQuant;
}

SRegexAstNode CRegexParser::QuantParse()
{
	SRegexAstNode regexCur = AtomParse();
			
	while(
		ChrPeek() == '*' ||
		ChrPeek() == '+' ||
		ChrPeek() == '?' ||
		ChrPeek() == '{' 
	)
	{
		// while there is a chr that begins a quantification, 
		// make a new quantifier quantifieing the regex to the left
		
		SRegexAstNode regexQuant = RegexCreate(REGEXK_Quant);

		SQuantifierRegexData * pQuantData = regexQuant.m_pQuantData;
		pQuantData->m_regex = regexCur;

		regexCur = regexQuant;

		// set cMic and cMac based on the current quantifier
	
		if(ChrPeek() == '*')
		{
			MatchChr('*');
			pQuantData->m_cMic = 0;
			pQuantData->m_cMac = -1;
		}	
		else if(ChrPeek() == '+')
		{
			MatchChr('+');
			pQuantData->m_cMic = 1;
			pQuantData->m_cMac = -1;
		}
		else if(ChrPeek() == '?')
		{
			MatchChr('?');
			pQuantData->m_cMic = 0;
			pQuantData->m_cMac = 1;
		}
		else
		{
			// {,} case
	
			MatchChr('{');

			// three valid versions
			// {N} exactly N times. N > 0
			// {N,} at least N times. N >= 0
			// {N1, N2} N1 to N2 times. N1 >= 0 and N2 > N1

			assert(isdigit(ChrPeek()));

			pQuantData->m_cMic = NConsume();

			assert(pQuantData->m_cMic > 0 || ChrPeek() == ',');

			if(ChrPeek() == ',')
			{
				MatchChr(',');

				if(isdigit(ChrPeek()))
				{
					pQuantData->m_cMac = NConsume();
					assert(pQuantData->m_cMac > pQuantData->m_cMic);
				}
				else
				{
					// {N,} case

					pQuantData->m_cMac = -1;
				}
			}
			else
			{
				// {N} case

				pQuantData->m_cMac = pQuantData->m_cMic;
			}
	
			MatchChr('}');
		}
	}

	// return the quantified regex (either the original atom, or nested quantifiers ending with the atom)

	return regexCur;
}

SRegexAstNode CRegexParser::AtomParse()
{
	if(ChrPeek() == '(')
	{
		MatchChr('(');

		SRegexAstNode unionAst = UnionParse();

		MatchChr(')');

		return unionAst;
	}
	else if(ChrPeek() == '[')
	{
		return SetParse();
	}
	else if(ChrPeek() == '.')
	{
		MatchChr('.');
		
		SRegexAstNode regexRange = RegexCreate(REGEXK_Range);

		SRangeRegexData * pRangeData = regexRange.m_pRangeData;
		pRangeData->m_chrMic = 0;
		pRangeData->m_chrMac = 255;

		return regexRange;
	}
	else if(ChrPeek() == '@')
	{
		MatchChr('@');

		return RegexCreate(REGEXK_Self);
	}
	else if(ChrPeek() == '\\')
	{
		MatchChr('\\');

		SRegexAstNode regexChr = RegexCreate(REGEXK_Chr);
		regexChr.m_pChrData->m_chr = ChrConsumeEscaped();

		return regexChr;
	}
	else
	{
		assert(FChrCanBeginAtom(ChrPeek()));

		SRegexAstNode regexChr = RegexCreate(REGEXK_Chr);
		regexChr.m_pChrData->m_chr = ChrConsume();

		return regexChr;
	}
}

SRegexAstNode CRegexParser::SetParse()
{
	MatchChr('[');

	// check if this set is a negation
	
	bool fNegate = false;

	if(ChrPeek() == '^')
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

	assert(FChrCanBeginRange(ChrPeek()));
	while(FChrCanBeginRange(ChrPeek()))
	{
		u8 chrBegin = ChrConsumeSet();
		u8 chrEnd = chrBegin;

		if(ChrPeek() == '-')
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

	SRegexAstNode regexUnion = RegexCreate(REGEXK_Union);
	SUnionRegexData * pUnionData = regexUnion.m_pUnionData;
	
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
				SRegexAstNode regexChr = RegexCreate(REGEXK_Chr);

				regexChr.m_pChrData->m_chr = chrBegin;

				pUnionData->m_aryRegex.push_back(regexChr);
			}
			else
			{
				SRegexAstNode regexRange = RegexCreate(REGEXK_Range);

				SRangeRegexData * pRangeData = regexRange.m_pRangeData;
				pRangeData->m_chrMic = chrBegin;
				pRangeData->m_chrMac = iChr;

				pUnionData->m_aryRegex.push_back(regexRange);
			}
		}
	}

	// a set has to have at least one element

	assert(pUnionData->m_aryRegex.size() > 0);

	if(pUnionData->m_aryRegex.size() == 1)
		return pUnionData->m_aryRegex[0];
	else
		return regexUnion;
}

u8 CRegexParser::ChrConsume()
{
	u8 chrPrev = m_chrCur;
	m_chrCur = (u8)fgetc(m_pFile);

	return chrPrev;
}

u8 CRegexParser::ChrConsumeHex()
{
	u8 chrHex = ChrConsume();

	assert(
		(chrHex >= '0' && chrHex <= '9') ||
		(chrHex >= 'a' && chrHex <= 'f') ||
		(chrHex >= 'A' && chrHex <= 'F')
	);

	return chrHex;
}

u8 CRegexParser::ChrConsumeSet()
{
	assert(FChrCanBeginRange(ChrPeek()));
	
	if(ChrPeek() == '\\')
	{
		MatchChr('\\');

		// '-' is an escape char in a set
		
		return ChrPeek() == '-' ? ChrConsume() : ChrConsumeEscaped();
	}
		
	return ChrConsume();
}

u8 CRegexParser::ChrConsumeEscaped()
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
		
		assert(isalpha(ChrPeek()));
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

		while(isdigit(ChrPeek()) && cDigit < 2)
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
			chrEscape == ']' ||
			chrEscape == '@'
		);

		return chrEscape;
	}
}

int CRegexParser::NConsume()
{
	assert(isdigit(ChrPeek()));
	
	string strNum = "";

	while(isdigit(ChrPeek()))
	{
		strNum += ChrConsume();
	}

	return atoi(strNum.c_str());
}

u8 CRegexParser::ChrPeek()
{
	return m_chrCur;
}

bool CRegexParser::FChrCanBeginAtom(u8 chr)
{
	return	ChrPeek() != ')' &&
			ChrPeek() != '|' &&
			ChrPeek() != '*' &&
			ChrPeek() != '+' &&
			ChrPeek() != '?' &&
			ChrPeek() != '{' &&
			ChrPeek() != '}' &&
			ChrPeek() != ']' &&
			!iscntrl(ChrPeek());
}

bool CRegexParser::FChrCanBeginRange(u8 chr)
{
	return 
		chr != '[' && 
		chr != ']' && 
		chr != '-' && 
		!iscntrl(chr);
}

void CRegexParser::MatchChr(u8 chrMatch)
{
	assert(ChrPeek() == chrMatch);

	(void) ChrConsume();
}

SRegexAstNode CRegexParser::RegexCreate(REGEXK regexk)
{
	SRegexAstNode regex;

	regex.m_regexk = regexk;

	switch (regexk)
	{
		case REGEXK_Union:
			regex.m_pUnionData = m_poolRegexData.PTNew<SUnionRegexData>();
			break;

		case REGEXK_Concat:
			regex.m_pConcatData = m_poolRegexData.PTNew<SConcatinationRegexData>();
			break;

		case REGEXK_Quant:
			regex.m_pQuantData = m_poolRegexData.PTNew<SQuantifierRegexData>();
			break;

		case REGEXK_Range:
			regex.m_pRangeData = m_poolRegexData.PTNew<SRangeRegexData>();
			break;

		case REGEXK_Chr:
			regex.m_pChrData = m_poolRegexData.PTNew<SChrRegexData>();
			break;

		case REGEXK_Self:
			regex.m_pRegexRoot = &m_regexAstParsed;
			break;

		default:
			assert(false);
			break;
	}

	return regex;
}