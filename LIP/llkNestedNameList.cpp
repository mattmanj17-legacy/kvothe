#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string>

#include "circlebuffer.h"

using std::string;
using std::exception;

/*
	grammar NestedNameList;

	list : LBRACK elements RBRACK;
	elements : element (COMMA element)*;
	element : assignment
			| NAME 
			| list
	assignment: NAME EQUALS NAME

	NAME : ([a-z] | [A-Z])+
	RBRACK: ]
	LBRACK : [
	COMMA : ,
	EQUALS : =
*/

class CNestedNameListParser
{
public:

	void list()
	{
		if (m_cbufTok[0].m_tokk == TOKK_LBRACK) { m_cbufTok.Push(m_tokenizer.TokNext()); }
		else { throw exception(); }

		elements();

		if (m_cbufTok[0].m_tokk == TOKK_RBRACK) { m_cbufTok.Push(m_tokenizer.TokNext()); }
		else { throw exception(); }
	}

	void elements()
	{
		element();

		while (m_cbufTok[0].m_tokk == TOKK_COMMA)
		{
			m_cbufTok.Push(m_tokenizer.TokNext());
			element();
		}
	}

	void element()
	{
		if (m_cbufTok[0].m_tokk == TOKK_LBRACK)
		{
			list();
		}
		else if (m_cbufTok[0].m_tokk == TOKK_NAME)
		{
			if (m_cbufTok[1].m_tokk == TOKK_EQUALS) { assignment();}
			else { m_cbufTok.Push(m_tokenizer.TokNext()); }
		}
		else { throw exception(); }
	}

	void assignment()
	{
		if (m_cbufTok[0].m_tokk == TOKK_NAME) { m_cbufTok.Push(m_tokenizer.TokNext()); }
		else { throw exception(); }

		if (m_cbufTok[0].m_tokk == TOKK_EQUALS) { m_cbufTok.Push(m_tokenizer.TokNext()); }
		else { throw exception(); }

		if (m_cbufTok[0].m_tokk == TOKK_NAME) { m_cbufTok.Push(m_tokenizer.TokNext()); }
		else { throw exception(); }
	}

	void SetInput(string strInput)
	{
		m_tokenizer.SetInput(strInput);
		m_cbufTok[0] = m_tokenizer.TokNext();
		m_cbufTok[1] = m_tokenizer.TokNext();
	}

private:

	enum TOKK
	{
		TOKK_NIL,
		TOKK_EOS,
		TOKK_NAME,
		TOKK_RBRACK,
		TOKK_LBRACK,
		TOKK_COMMA,
		TOKK_EQUALS,
	};

	class CNestedNameListTokenizer
	{
	public:

		void SetInput(string strInput)
		{
			m_strInput = strInput;
			m_iChrCur = 0;
		}

		TOKK TokNext()
		{
			while (m_iChrCur < m_strInput.length())
			{
				char chrCur = m_strInput[m_iChrCur];
				if (isspace(chrCur)) { ++m_iChrCur; } // ignore white space
				else if (chrCur == ',') { ++m_iChrCur; return SToken{ TOKK_COMMA }; }
				else if (chrCur == '[') { ++m_iChrCur; return SToken{ TOKK_LBRACK }; }
				else if (chrCur == ']') { ++m_iChrCur; return SToken{ TOKK_RBRACK }; }
				else if (chrCur == '=') { ++m_iChrCur; return SToken{ TOKK_EQUALS }; }
				else if (isalpha(chrCur))
				{
					SToken tok{ TOKK_NAME };

					while (isalpha(chrCur) && m_iChrCur < m_strInput.length())
					{
						chrCur = m_strInput[m_iChrCur];
						tok.m_str += chrCur;
						++m_iChrCur;
					}

					return tok;
				}
				else { throw exception("invalid character"); }
			}
			return TOKK_EOS;
		}

	private:

		string m_strInput;
		int m_iChrCur;
	};

	CNestedNameListTokenizer m_tokenizer;
	CCircleBuffer<TOKK,2> m_cbufTok;
};