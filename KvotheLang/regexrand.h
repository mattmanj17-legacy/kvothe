#pragma once

#include "regexparse.h"
#include <string>

class CRegexRandom
{
public:

	SRegexAstNode			RegexRandom();
	std::string				StrRandFromRegex(SRegexAstNode regexAst);
							
protected:
				
	SRegexAstNode			UnionRandom();					
	SRegexAstNode			ConcatRandom();					
	SRegexAstNode			QuantRandom();					
	SRegexAstNode			AtomRandom();					
	SRegexAstNode			SetRandom();

	SRegexAstNode			RegexCreate(REGEXK regexk);

	VoidPool				m_poolRegexData;
};