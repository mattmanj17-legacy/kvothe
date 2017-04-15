#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string>
#include <exception>
#include <assert.h>

using std::string;
using std::exception;

/*
union
: concatenation (VBAR union)?
;

concatenation
: quantifier concatenation?
;

quantifier
: expression (QMARK|STAR|PLUS)?
;

expression
: lPAREN union RPAREN
| DOT
| group
| character
;

group
: LBRACK CARROT? groupItems RBRACK
;

groupItems
: groupItem groupItems?
;

groupItem
: character (DASH character)?
;

character
: CHAR
| HEX
;
*/

class CRegexParser
{
public:
	
	void Union()
	{
		concatenation();

		if (m_tokCur.m_tokk == TOKK_VBAR)
		{
			m_tokCur = m_tokenizer.TokNext();
			Union();
		}
	}

	void concatenation()
	{
		quantifier();

		if (m_tokCur.m_tokk == TOKK_LPAREN ||
			m_tokCur.m_tokk == TOKK_DOT ||
			m_tokCur.m_tokk == TOKK_LBRACK ||
			m_tokCur.m_tokk == TOKK_CHAR ||
			m_tokCur.m_tokk == TOKK_HEX)
		{
			concatenation();
		}
	}

	void quantifier()
	{
		expression();

		if (m_tokCur.m_tokk == TOKK_QMARK ||
			m_tokCur.m_tokk == TOKK_STAR ||
			m_tokCur.m_tokk == TOKK_PLUS)
		{
			m_tokCur = m_tokenizer.TokNext();
		}
	}

	void expression()
	{
		if (m_tokCur.m_tokk == TOKK_LPAREN) 
		{
			m_tokCur = m_tokenizer.TokNext();

			Union();

			if (m_tokCur.m_tokk != TOKK_RPAREN) { throw exception(); }

			m_tokCur = m_tokenizer.TokNext();
		}
		else if (m_tokCur.m_tokk == TOKK_DOT) { m_tokCur = m_tokenizer.TokNext(); }
		else if (m_tokCur.m_tokk == TOKK_LBRACK) { group(); }
		else if (m_tokCur.m_tokk == TOKK_CHAR || m_tokCur.m_tokk == TOKK_HEX) { character(); }
		else { throw exception(); }
	}

	void group()
	{
		if (m_tokCur.m_tokk == TOKK_LBRACK) { m_tokCur = m_tokenizer.TokNext(); }
		else { throw exception(); }

		if (m_tokCur.m_tokk == TOKK_CARROT) { m_tokCur = m_tokenizer.TokNext(); }

		groupItems();

		if (m_tokCur.m_tokk == TOKK_RBRACK) { m_tokCur = m_tokenizer.TokNext(); }
		else { throw exception(); }
	}

	void groupItems()
	{
		groupItem();

		if (m_tokCur.m_tokk == TOKK_CHAR || m_tokCur.m_tokk == TOKK_HEX)
		{
			groupItems();
		}
	}

	void groupItem()
	{
		character();

		if (m_tokCur.m_tokk == TOKK_DASH)
		{
			m_tokCur = m_tokenizer.TokNext();

			character();
		}
	}

	void character()
	{
		if (m_tokCur.m_tokk == TOKK_CHAR || m_tokCur.m_tokk == TOKK_HEX) { m_tokCur = m_tokenizer.TokNext(); }
		else { throw exception(); }
	}

	void SetInput(string strInput)
	{
		m_tokenizer.SetInput(strInput);
		m_tokCur = m_tokenizer.TokNext();
	}

private:

	enum TOKK
	{
		// invalid token

		TOKK_NIL,

		// end of stream

		TOKK_EOS,

		// a character (non meta token or escaped meta token)

		TOKK_CHAR,

		// hex literal

		TOKK_HEX,

		// meta tokens (does not include back slash because the parser does not care about it)

		TOKK_STAR,
		TOKK_PLUS,
		TOKK_QMARK,
		TOKK_LPAREN,
		TOKK_RPAREN,
		TOKK_LBRACK,
		TOKK_RBRACK,
		TOKK_CARROT,
		TOKK_DASH,
		TOKK_DOT,
		TOKK_VBAR,
	};

	struct SToken
	{
		TOKK m_tokk = TOKK_NIL;
		string m_str = "";
	};

	class CRegexTokenizer
	{
	public:
		void SetInput(string strInput)
		{
			m_strInput = strInput;
			m_iChrCur = 0;
		}

		SToken TokNext()
		{
			while (m_iChrCur < m_strInput.length())
			{
				char chrCur = m_strInput[m_iChrCur];

				if (isspace(chrCur)) { ++m_iChrCur; } // ignore white space
				else if (chrCur == '\\') // escaped character
				{
					++m_iChrCur;
					chrCur = m_strInput[m_iChrCur];

					if (chrCur == 'x') // hex literal
					{
						SToken tok{ TOKK_HEX };

						++m_iChrCur;
						chrCur = m_strInput[m_iChrCur];

						while (isdigit(chrCur))
						{
							tok.m_str += chrCur;
							++m_iChrCur;
							chrCur = m_strInput[m_iChrCur];
						}

						if (tok.m_str.length() == 0) { throw exception("hex literal needs at least one digit"); }
						if (tok.m_str.length() > 4) { throw exception("hex literal can not have more than 4 digits"); }
						else { return tok; }
					}
					else if (FIsMetaChar(chrCur) || chrCur == '\\') { ++m_iChrCur; return SToken{ TOKK_CHAR, string(1, chrCur) }; } // escaped meta character (or backslash)
					else { throw exception("invalid escape sesuence"); }
				}
				else if (FIsMetaChar(chrCur)) { ++m_iChrCur; return SToken{ TokkFromMetaChar(chrCur) }; } // meta character
				else { ++m_iChrCur; return SToken{ TOKK_CHAR, string(1, chrCur) }; } // any other character
			}
			return SToken{ TOKK_EOS };
		}

	private:

		bool FIsMetaChar(char chr)
		{
			switch (chr)
			{
			case '*':
			case '+':
			case '?':
			case '(':
			case ')':
			case '[':
			case ']':
			case '^':
			case '-':
			case '.':
			case '|': return true;
			default: return false;
			}
		}

		TOKK TokkFromMetaChar(char chr)
		{
			switch (chr)
			{
			case '*': return TOKK_STAR;
			case '+': return TOKK_PLUS;
			case '?': return TOKK_QMARK;
			case '(': return TOKK_LPAREN;
			case ')': return TOKK_RPAREN;
			case '[': return TOKK_LBRACK;
			case ']': return TOKK_RBRACK;
			case '^': return TOKK_CARROT;
			case '-': return TOKK_DASH;
			case '.': return TOKK_DOT;
			case '|': return TOKK_VBAR;
			default: throw exception("can not get TOKK from non meta char");
			}
		}

		string m_strInput;
		int m_iChrCur;
	};

	CRegexTokenizer m_tokenizer;
	SToken m_tokCur;
};



