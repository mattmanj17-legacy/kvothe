#pragma once

#include <vector>
using std::vector;

#include "types.h"
#include "pool.h"

// REGular EXpression Kind

enum REGEXK
{
	REGEXK_Union,
	REGEXK_Concat,
	REGEXK_Quant,
	REGEXK_Range,
	REGEXK_Chr,
	REGEXK_Nil,
};

struct SUnionRegexData;
struct SConcatinationRegexData;
struct SQuantifierRegexData;
struct SRangeRegexData;
struct SChrRegexData;

struct SRegexAstNode
{
									SRegexAstNode()
									: m_pUnionData(nullptr)
									, m_regexk(REGEXK_Nil)
									{
									}
	
	void							PrintDebug() const;
	
	// regex data
	
	union
	{
		SUnionRegexData *			m_pUnionData;
		SConcatinationRegexData *	m_pConcatData;
		SQuantifierRegexData *		m_pQuantData;
		SRangeRegexData *			m_pRangeData;
		SChrRegexData *				m_pChrData;
	};

	REGEXK							m_regexk;		// what type of regex is this
};

struct SUnionRegexData
{
	vector<SRegexAstNode>	m_aryRegex; // list of regexs to union
};

struct SConcatinationRegexData
{
	vector<SRegexAstNode>	m_aryRegex; // list of regexs to concatinate
};

struct SQuantifierRegexData
{
	int				m_cMic;			// the fewest and the most times to match. m_cMac == -1 means can match unlimited number of times
	int				m_cMac;			// ...

	SRegexAstNode	m_regex;		// regex to match a given number of times
};

struct SRangeRegexData
{
	u8		m_chrMic;		// start and end of the range and chrs to match
	u8		m_chrMac;		// ...
};

struct SChrRegexData
{
	u8		m_chr;			// single chr this regex matches
};

class CRegexParser
{
public:

	void					ParseFile(FILE * pFile);	// parse a whole file as one regex
	const SRegexAstNode *	PRegexAstParsed();			// the regex parsed in ParseFile

private:

	SRegexAstNode			RegexParse();					
	SRegexAstNode			UnionParse();					
	SRegexAstNode			ConcatParse();					
	SRegexAstNode			QuantParse();					
	SRegexAstNode			AtomParse();					
	SRegexAstNode			SetParse();						

	u8						ChrConsume();				// return the current chr and read a new one
	u8						ChrConsumeHex();			// check that the current chr is hex, then consume it
	u8						ChrConsumeSet();			// check that the current chr is a valid set chr, then consume it
	u8						ChrConsumeEscaped();		// check that the current chr is a vlid escape chr, then consume it

	int						NConsume();					// consume a base ten integer from input
	
	u8						ChrPeek();					// return the current chr, but do not read a new one
	
	bool					FChrCanBeginAtom(u8 chr);	// is a chr a valid atom chr?
	bool					FChrCanBeginRange(u8 chr);	// is a chr a valid set chr?
	
	void					MatchChr(u8 chrMatch);		// check that the current char is a given one, then consume it

	SRegexAstNode			RegexCreate(REGEXK regexk);	// create a regex and its data (in m_poolRegex and m_poolRegexData) based on REGEXK
	
	u8						m_chrCur;						
	FILE *					m_pFile;					// the file we are consuming chrs from
	SRegexAstNode			m_regexAstParsed;			// the regex parsed in ParseFile
	
	VoidPool				m_poolRegexData;			// pool for allocating regex data			
};
