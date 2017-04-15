/*
	stat : list EOF | assign EOF
	assign : list '=' list
	list: '[' elements ']'
	elements: element (',' element)*
	element: NAME '=' NAME | NAME | list
*/

#include <deque>
#include <ctype.h>
#include <exception>
#include <string>
#include <stack>

using std::exception;
using std::string;
using std::deque;
using std::stack;

enum TOKK
{
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

	TOKK TokkNext()
	{
		while (m_iChrCur < m_strInput.length())
		{
			char chrCur = m_strInput[m_iChrCur];
			if (isspace(chrCur)) { ++m_iChrCur; } // ignore white space
			else if (chrCur == ',') { ++m_iChrCur; return TOKK_COMMA; }
			else if (chrCur == '[') { ++m_iChrCur; return TOKK_LBRACK; }
			else if (chrCur == ']') { ++m_iChrCur; return TOKK_RBRACK; }
			else if (chrCur == '=') { ++m_iChrCur; return TOKK_EQUALS; }
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
	void match(TOKK tokk)
	{
		if (m_deqTokk[m_iBeginCur] != tokk) { throw exception(); }

		if (m_stkIBegin.size() == 0)
		{
			m_deqTokk.pop_front();
		}
		else
		{

		}

		m_deqTokk.push_back(m_tokenizer.TokkNext());
	}

	void BeginSpeculate()
	{
		m_stkIBegin.push(m_iBeginCur);
	}

	void EndSpeculate()
	{
		m_iBeginCur = m_stkIBegin.top();
		m_stkIBegin.pop();
	}

	void stat()
	{
		if (FSpeculateStatIsAssign())
		{
			assign();
			match(TOKK_EOS);
		}
		else if (FSpeculateStatIsList())
		{
			list();
			match(TOKK_EOS);
		}
		else
		{
			throw exception();
		}
	}

	bool FSpeculateStatIsAssign()
	{
		bool fSuccess = true;
		BeginSpeculate();
		try { assign(); match(TOKK_EOS); }
		catch (...) { fSuccess = false; }
		EndSpeculate();
		return fSuccess;
	}

	bool FSpeculateStatIsList()
	{
		bool fSuccess = true;
		BeginSpeculate();
		try { list(); match(TOKK_EOS); }
		catch (...) {fSuccess = false; }
		EndSpeculate();
		return fSuccess;
	}

	void assign()
	{

	}

	void list()
	{

	}

	void elements()
	{

	}

	void element()
	{

	}

	void SetInput(string strInput)
	{
		m_tokenizer.SetInput(strInput);
	}

private:

	CNestedNameListTokenizer m_tokenizer;
	deque<TOKK> m_deqTokk;
	stack<int> m_stkIBegin;
	int m_iBeginCur;
};