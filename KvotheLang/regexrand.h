#pragma once

#include "regexparse.h"
#include <string>

class CRegexRandom
{
public:

							CRegexRandom()
							: m_pRegexRoot(nullptr)
							, m_cLengthCur(0)
							, m_nDepthCur(0)
							, m_poolRegexData()
							{
							}
	
	SRegexAstNode			RegexRandom();
	std::string				StrRandFromRegex(SRegexAstNode regexAst);
							
protected:
				
	SRegexAstNode			UnionRandom();					
	SRegexAstNode			ConcatRandom();					
	SRegexAstNode			QuantRandom();					
	SRegexAstNode			AtomRandom();					
	SRegexAstNode			SetRandom();

	SRegexAstNode			RegexCreate(REGEXK regexk);

	SRegexAstNode *			m_pRegexRoot;
	int						m_cLengthCur;
	int						m_nDepthCur;

	VoidPool				m_poolRegexData;
};