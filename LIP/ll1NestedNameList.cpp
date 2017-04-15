#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string>

using std::string;
using std::exception;

/*
	grammar NestedNameList;

	list : LBRACK elements RBRACK;
	elements : element (COMMA element)*;
	element : NAME | list

	NAME : ([a-z] | [A-Z])+
	RBRACK: ]
	LBRACK : [
	COMMA : ,
*/

enum TOKK
{
	TOKK_NIL,
	TOKK_EOS,
	TOKK_NAME,
	TOKK_RBRACK,
	TOKK_LBRACK,
	TOKK_COMMA,
};

class CNestedNameListTokenizer
{
public:

	void SetInput(string strInput)
	{
		m_strInput = strInput;
		m_iChrCur = 0;
	}

	TOKK TokkNext()
	{
		while (m_iChrCur < m_strInput.length())
		{
			char chrCur = m_strInput[m_iChrCur];
			if (isspace(chrCur)) { ++m_iChrCur; } // ignore white space
			else if (chrCur == ',') { ++m_iChrCur; return TOKK_COMMA; }
			else if (chrCur == '[') { ++m_iChrCur; return TOKK_LBRACK; }
			else if (chrCur == ']') { ++m_iChrCur; return TOKK_RBRACK; }
			else if (isalpha(chrCur))
			{

				while (isalpha(chrCur) && m_iChrCur < m_strInput.length())
				{
					++m_iChrCur;
					chrCur = m_strInput[m_iChrCur];
				}

				return TOKK_NAME;
			}
			else { throw exception(); }
		}
		return TOKK_EOS;
	}

private:

	string m_strInput;
	int m_iChrCur;
};

class CNestedNameListParser
{
public:
	
	void list()
	{
		if (m_tokkCur == TOKK_LBRACK) { m_tokkCur = m_tokenizer.TokkNext(); }
		else { throw exception(); }

		elements();

		if (m_tokkCur == TOKK_RBRACK) { m_tokkCur = m_tokenizer.TokkNext(); }
		else { throw exception(); }
	}

	void elements()
	{
		element();

		while (m_tokkCur == TOKK_COMMA)
		{
			m_tokkCur = m_tokenizer.TokkNext();
			element();
		}
	}

	void element()
	{
		if (m_tokkCur == TOKK_LBRACK) { list(); }
		else if (m_tokkCur == TOKK_NAME) { m_tokkCur = m_tokenizer.TokkNext(); }
		else { throw exception(); }
	}

	void SetInput(string strInput)
	{
		m_tokenizer.SetInput(strInput);
		m_tokkCur = m_tokenizer.TokkNext();
	}
	
private:

	CNestedNameListTokenizer m_tokenizer;
	TOKK m_tokkCur;
};







