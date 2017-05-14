#include <string>
#include <ctype.h>
#include <stdio.h>
#include <vector>
#include <assert.h>
#include <memory>

using std::string;
using std::vector;
using std::shared_ptr;

#include "macros.h"

// parser for gfiles grammar, files that specify a grammar

/* grammar for gfile (written as a gfile)
Rule grammar
	: (rule | token) +
	;

Rule rule
	: RULE _ID COLON ruleBody SEMICOLON
	;
	
Rule ruleBody
	: atom postfixOperator? (VBAR? atom postfixOperator?)* 
	;

Rule postfixOperator
	: QMARK
	| PLUS
	| STAR
	;
	
Rule atom 
	: builtinToken
	| _ID
	| LPAREN ruleBody RPAREN
	;
	
Rule builtinToken 
	: ID
	| NUM
	;
	
Rule token
	: TOKEN _ID COLON tokenLiteral SEMICOLON
	;
	
Rule tokenLiteral
	: TICK (_ID | symbol) TICK 
	;
	
Rule symbol
	: (escapedSymbolChar | unescapedSymbolChar) +
	;
	
Rule escapedSymbolChar
	: ESC_BSLASH | ESC_TICK
	;
	
Rule unescapedSymbolChar
	: BTICK		
	| TILDE		
	| BANG		
	| AT		
	| HASH		
	| DOLLAR	
	| PERCENT	
	| CARROT	
	| AMP		
	| STAR		
	| LPAREN	
	| RPAREN	
	| UBAR		
	| PLUS		
	| HBAR		
	| EQUAL		
	| VBAR		
	| LBRACK	
	| RBRACK	
	| LBRACE	
	| RBRACE	
	| SEMICOLON	
	| COLON		
	| COMMA		
	| LT		
	| GT		
	| DOT		
	| FSLASH	
	| QMARK		
	;
	
Token RULE 		: 'Rule';
Token TOKEN 	: 'Token';
Token ID 		: '_ID';
Token NUM 		: '_NUM';

Token ESC_BSLASH: '\\\\';
Token ESC_TICK	: '\\\'';
Token TICK 		: '\'';
Token BTICK		: '`' ;
Token TILDE		: '~' ;
Token BANG		: '!' ;
Token AT		: '@' ;
Token HASH		: '#' ;
Token DOLLAR	: '$' ;
Token PERCENT	: '%' ;
Token CARROT	: '^' ;
Token AMP		: '&' ;
Token STAR		: '*' ;
Token LPAREN	: '(' ;
Token RPAREN	: ')' ;
Token UBAR		: '_' ;
Token PLUS		: '+' ;
Token HBAR		: '-' ;
Token EQUAL		: '=' ;
Token VBAR		: '|' ;
Token LBRACK	: '[' ;
Token RBRACK	: ']' ;
Token LBRACE	: '{' ;
Token RBRACE	: '}' ;
Token SEMICOLON	: ';' ;
Token COLON		: ':' ;
Token COMMA		: ',' ;
Token LT		: '<' ;
Token GT		: '>' ;
Token DOT		: '.' ;
Token FSLASH	: '/' ;
Token QMARK		: '?' ;
*/

ENUM(TOKK, 
	TOKK_ID,	
	TOKK_NUM,
	TOKK_KW_RULE, 	
	TOKK_KW_TOKEN,
	TOKK_KW_ID,	
	TOKK_KW_NUM, 	
	TOKK_ESC_BSLASH,
	TOKK_ESC_TICK,
	TOKK_TICK,
	TOKK_BTICK,
	TOKK_TILDE,
	TOKK_BANG,
	TOKK_AT,
	TOKK_HASH,
	TOKK_DOLLAR,
	TOKK_PERCENT,
	TOKK_CARROT,
	TOKK_AMP,
	TOKK_STAR,
	TOKK_LPAREN,
	TOKK_RPAREN,
	TOKK_UBAR,
	TOKK_PLUS,
	TOKK_HBAR,
	TOKK_EQUAL,
	TOKK_VBAR,
	TOKK_LBRACK,
	TOKK_RBRACK,
	TOKK_LBRACE,
	TOKK_RBRACE,
	TOKK_SEMICOLON,
	TOKK_COLON,
	TOKK_COMMA,
	TOKK_LT,
	TOKK_GT,
	TOKK_DOT,
	TOKK_FSLASH,
	TOKK_QMARK
);

ENUM(RULEK, 
	RULEK_grammar,
	RULEK_rule,
	RULEK_ruleBody,
	RULEK_postfixOperator,
	RULEK_atom,
	RULEK_builtinToken,
	RULEK_token,
	RULEK_tokenLiteral,
	RULEK_symbol,
	RULEK_escapedSymbolChar,
	RULEK_unescapedSymbolChar
);

ENUM(NODEK,
	NODEK_Rule,
	NODEK_Token
);

// node in the parse tree

// parse tree

struct SParseTree // tag = parsetree
{
	struct SParseNode // tag = node
	{
		SParseNode(NODEK nodek)
		: m_nodek(nodek)
		{
		}

		virtual void PrintDebug() = 0;
		virtual TOKK Tokk() { return TOKK_Nil; }
		virtual RULEK Rulek() { return RULEK_Nil; }
		
		NODEK m_nodek;
	};

	struct SRuleNode // tag = rnode
	: public SParseNode 
	{
		SRuleNode(RULEK rulek)
		: SParseNode(NODEK_Rule)
		, m_aryPNodeChild() 
		, m_rulek(rulek)
		{
		}

		void AddChild(SParseNode * pNodeChild)
		{
			m_aryPNodeChild.push_back(pNodeChild);
		}

		SParseNode * PNodeChild(unsigned int nChild)
		{
			assert(nChild >= 0);
			assert(nChild < m_aryPNodeChild.size());

			return m_aryPNodeChild[nChild];
		}

		int CChild()
		{
			return m_aryPNodeChild.size();
		}

		void PrintDebug() override
		{
			printf("(%s", PChzFromRULEK(m_rulek));

			for(SParseNode * pNodeChild : m_aryPNodeChild)
			{
				printf(" ");
				pNodeChild->PrintDebug();
			}

			printf(")");
		}

		RULEK Rulek() override { return m_rulek; }
	
		vector<SParseNode *> m_aryPNodeChild;

		RULEK m_rulek;
	};

	struct STokenNode // tag = tnode
	: public SParseNode
	{
		STokenNode(TOKK tokk, string str = "")
		: SParseNode(NODEK_Token)
		, m_tokk(tokk)
		, m_str(str)
		{
		}
		
		void PrintDebug() override
		{
			printf("%s", PChzFromTOKK(m_tokk));

			if(m_str.size() > 0)
			{
				printf(" '%s'", m_str.c_str());
			}
		}

		TOKK Tokk() override { return m_tokk; }

		TOKK m_tokk;
		string m_str;
	};
	
	SRuleNode* PRNodeCreate(RULEK rulek)
	{
		shared_ptr<SParseNode> pNode(new SRuleNode(rulek));
		m_arypNode.push_back(pNode);
		return (SRuleNode*)pNode.get();
	}

	STokenNode* PTNodeCreate(TOKK tokk, string str = "")
	{
		shared_ptr<SParseNode> pNode(new STokenNode(tokk, str));
		m_arypNode.push_back(pNode);
		return (STokenNode*)pNode.get();
	}

	vector<shared_ptr<SParseNode>> m_arypNode;
};

struct STokenizer // tag = tokenizer
{
	STokenizer()
	: m_parsetree()
	, m_iChrCur(0)
	, m_strInput("")
	{
	}

	SParseTree::STokenNode * PTNodeNext()
	{
		static char s_mpTokkChr [] =
		{
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			'\'',
			'`',
			'~',
			'!',
			'@',
			'#',
			'$',
			'%',
			'^',
			'&',
			'*',
			'(',
			')',
			'_',
			'+',
			'-',
			'=',
			'|',
			'[',
			']',
			'{',
			'}',
			';',
			':',
			',',
			'<',
			'>',
			'.',
			'/',
			'?',
		};
		CASSERT(DIM(s_mpTokkChr) == TOKK_Max);

		static const char * s_mpTokkPChz [] =
		{
			nullptr,
			nullptr,
			"Rule",
			"Token",
			"__ID",
			"__NUM",
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
		};
		CASSERT(DIM(s_mpTokkChr) == TOKK_Max);

		while (m_iChrCur < m_strInput.length())
		{
			// skip white space

			if(isspace(ChrCur()))
			{
				ReadChr();
				continue;
			}
			
			// check for escape sequence
			
			if(ChrCur() == '\\' && m_iChrCur != m_strInput.length())
			{
				if(m_iChrCur != m_strInput.length())
				{
					ReadChr();

					if(ChrCur() == '\\')
						return m_parsetree.PTNodeCreate(TOKK_ESC_BSLASH);
					else if(ChrCur() == '\'')
						return m_parsetree.PTNodeCreate(TOKK_ESC_TICK);
				}

				assert(false);
			}
			
			// check for single symbol
			
			for(TOKK tokk = TOKK_Min; tokk < TOKK_Max; ++tokk)
			{
				char chrTest = s_mpTokkChr[tokk];
				if(chrTest != 0 && ChrCur() == chrTest)
					return m_parsetree.PTNodeCreate(tokk, string(1, ChrCur()));
			}

			// check for num
			
			if(isdigit(ChrCur()))
			{
				string strNum = "";

				while(isdigit(ChrCur()))
				{
					strNum+=ChrCur();
					ReadChr();
				}

				return m_parsetree.PTNodeCreate(TOKK_NUM, strNum);
			}
			
			// check for keyword or ID

			bool fHasNum = false;
			if(isalpha(ChrCur()))
			{
				string strTok = "";
				
				while(ChrCur() == '_' || isdigit(ChrCur()) || isalpha(ChrCur()))
				{
					if(isdigit(ChrCur()))
					{
						fHasNum = true;
					}

					strTok+=ChrCur();
					ReadChr();
				}

				if(!fHasNum)
				{
					for(TOKK tokk = TOKK_Min; tokk < TOKK_Max; ++tokk)
					{
						const char * pChzTest = s_mpTokkPChz[tokk];
						if(pChzTest && strcmp(pChzTest, strTok.c_str()))
							return m_parsetree.PTNodeCreate(tokk, string(pChzTest)); // keyword
					}
				}

				return m_parsetree.PTNodeCreate(TOKK_ID, strTok); // ID
			}

			// unrecognized char

			assert(false);
		}

		return nullptr;
	}

	char ChrCur()
	{
		return m_chrCur;
	}

	void ReadChr()
	{
		m_chrCur = m_strInput[m_iChrCur];
		m_iChrCur++;
	}

	void SetInput(string str)
	{
		m_strInput = str;
		m_iChrCur = 0;
		ReadChr();
	}

	char m_chrCur;
	string			m_strInput;
	unsigned int	m_iChrCur;

	SParseTree		m_parsetree;
};

struct SParser
{
	SParseTree::SRuleNode * PRNodeGrammar()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_grammar);
		
		if(TokkCur() == TOKK_KW_RULE)
		{
			pRNode->AddChild(PRNodeRule());
		}
		else if(TokkCur() == TOKK_KW_TOKEN)
		{
			pRNode->AddChild(PRNodeToken());
		}

		while(TokkCur() == TOKK_KW_RULE || TokkCur() == TOKK_KW_TOKEN)
		{
			if(TokkCur() == TOKK_KW_RULE)
			{
				pRNode->AddChild(PRNodeRule());
			}
			else if(TokkCur() == TOKK_KW_TOKEN)
			{
				pRNode->AddChild(PRNodeToken());
			}
		}

		return pRNode;
	}

	SParseTree::SRuleNode * PRNodeRule()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_rule);
		
		assert(TokkCur() == TOKK_KW_RULE);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		assert(TokkCur() == TOKK_ID);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		assert(TokkCur() == TOKK_COLON);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		pRNode->AddChild(PRNodeRuleBody());

		assert(TokkCur() == TOKK_SEMICOLON);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		return pRNode;
	}

	SParseTree::SRuleNode * PRNodeRuleBody()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_ruleBody);

		pRNode->AddChild(PRNodeAtom());

		if(TokkCur() == TOKK_QMARK || TokkCur() == TOKK_PLUS || TokkCur() == TOKK_STAR)
		{
			pRNode->AddChild(PRNodePostfixOperator());
		}

		while(TokkCur() == TOKK_VBAR)
		{
			pRNode->AddChild(m_pTNodeCur);
			ReadToken();

			pRNode->AddChild(PRNodeAtom());

			if(TokkCur() == TOKK_QMARK || TokkCur() == TOKK_PLUS || TokkCur() == TOKK_STAR)
			{
				pRNode->AddChild(PRNodePostfixOperator());
			}
		}
	}

	SParseTree::SRuleNode * PRNodePostfixOperator()
	{
		if(TokkCur() == TOKK_QMARK || TokkCur() == TOKK_PLUS || TokkCur() == TOKK_STAR)
		{
			SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_postfixOperator);

			pRNode->AddChild(m_pTNodeCur);

			ReadToken();

			return pRNode;
		}

		assert(false);
		return nullptr;
	}

	SParseTree::SRuleNode * PRNodeAtom()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_atom);

		if(TokkCur() == TOKK_ID)
		{
			pRNode->AddChild(m_pTNodeCur);
			ReadToken();
		}
		else if(TokkCur() == TOKK_LPAREN)
		{
			pRNode->AddChild(m_pTNodeCur);
			ReadToken();

			pRNode->AddChild(PRNodeRuleBody());

			assert(TokkCur() == TOKK_RPAREN);
			pRNode->AddChild(m_pTNodeCur);
			ReadToken();
		}
		else
		{
			pRNode->AddChild(PRNodeBuiltInToken());
		}

		return pRNode;
	}

	SParseTree::SRuleNode * PRNodeBuiltInToken()
	{
		if(TokkCur() == TOKK_KW_ID || TokkCur() == TOKK_KW_NUM)
		{
			SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_builtinToken);

			pRNode->AddChild(m_pTNodeCur);

			ReadToken();

			return pRNode;
		}

		assert(false);
		return nullptr;
	}

	SParseTree::SRuleNode * PRNodeToken()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_token);
		
		assert(TokkCur() == TOKK_KW_TOKEN);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		assert(TokkCur() == TOKK_ID);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		assert(TokkCur() == TOKK_COLON);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		pRNode->AddChild(PRNodeTokenLiteral());

		assert(TokkCur() == TOKK_SEMICOLON);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		return pRNode;
	}

	SParseTree::SRuleNode * PRNodeTokenLiteral()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_tokenLiteral);
		
		assert(TokkCur() == TOKK_TICK);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		if(TokkCur() == TOKK_ID)
		{
			pRNode->AddChild(m_pTNodeCur);
			ReadToken();
		}
		else
		{
			pRNode->AddChild(PRNodeTokenSymbol());
		}

		assert(TokkCur() == TOKK_TICK);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		return pRNode;
	}

	SParseTree::SRuleNode * PRNodeTokenSymbol()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_symbol);
		
		TOKK aTokk[] =
		{
			TOKK_BTICK,
			TOKK_TILDE,
			TOKK_BANG,
			TOKK_AT,
			TOKK_HASH,
			TOKK_DOLLAR,
			TOKK_PERCENT,
			TOKK_CARROT,
			TOKK_AMP,
			TOKK_STAR,
			TOKK_LPAREN,
			TOKK_RPAREN,
			TOKK_UBAR,
			TOKK_PLUS,
			TOKK_HBAR,
			TOKK_EQUAL,
			TOKK_VBAR,
			TOKK_LBRACK,
			TOKK_RBRACK,
			TOKK_LBRACE,
			TOKK_RBRACE,
			TOKK_SEMICOLON,
			TOKK_COLON,
			TOKK_COMMA,
			TOKK_LT,
			TOKK_GT,
			TOKK_DOT,
			TOKK_FSLASH,
			TOKK_QMARK,
		};
		
		if(TokkCur() == TOKK_ESC_BSLASH || TokkCur() == TOKK_ESC_TICK)
		{
			pRNode->AddChild(PRNodeTokenEscapedSymbolChar());
		}
		else
		{
			pRNode->AddChild(PRNodeTokenUnescapedSymbolChar());
		}

		while(true)
		{
			if(TokkCur() == TOKK_ESC_BSLASH || TokkCur() == TOKK_ESC_TICK)
			{
				pRNode->AddChild(PRNodeTokenEscapedSymbolChar());
				continue;
			}

			bool fIsSymbolChar = false;
			for(TOKK tokk : aTokk)
			{
				if(tokk == TokkCur())
				{
					fIsSymbolChar = true;
					break;
				}
			}

			if(fIsSymbolChar)
			{
				pRNode->AddChild(PRNodeTokenUnescapedSymbolChar());
				continue;
			}

			break;
		}
	}

	SParseTree::SRuleNode * PRNodeTokenEscapedSymbolChar()
	{
		if(TokkCur() == TOKK_ESC_BSLASH || TokkCur() == TOKK_ESC_TICK)
		{
			SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_escapedSymbolChar);

			pRNode->AddChild(m_pTNodeCur);

			ReadToken();

			return pRNode;
		}

		assert(false);
		return nullptr;
	}

	SParseTree::SRuleNode * PRNodeTokenUnescapedSymbolChar()
	{
		TOKK aTokk[] =
		{
			TOKK_BTICK,
			TOKK_TILDE,
			TOKK_BANG,
			TOKK_AT,
			TOKK_HASH,
			TOKK_DOLLAR,
			TOKK_PERCENT,
			TOKK_CARROT,
			TOKK_AMP,
			TOKK_STAR,
			TOKK_LPAREN,
			TOKK_RPAREN,
			TOKK_UBAR,
			TOKK_PLUS,
			TOKK_HBAR,
			TOKK_EQUAL,
			TOKK_VBAR,
			TOKK_LBRACK,
			TOKK_RBRACK,
			TOKK_LBRACE,
			TOKK_RBRACE,
			TOKK_SEMICOLON,
			TOKK_COLON,
			TOKK_COMMA,
			TOKK_LT,
			TOKK_GT,
			TOKK_DOT,
			TOKK_FSLASH,
			TOKK_QMARK,
		};

		for(TOKK tokk : aTokk)
		{
			if(tokk == TokkCur())
			{
				SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_unescapedSymbolChar);

				pRNode->AddChild(m_pTNodeCur);

				ReadToken();

				return pRNode;
			}
		}

		assert(false);
		return nullptr;
	}

	void ReadToken()
	{
		m_pTNodeCur = m_tokenizer.PTNodeNext();
	}

	TOKK TokkCur()
	{
		return m_pTNodeCur->m_tokk;
	}
	
	SParseTree * PParseTree()
	{
		return &m_tokenizer.m_parsetree;
	}

	void SetInput(string strInput)
	{
		m_tokenizer.SetInput(strInput);
		ReadToken();
	}

	SParseTree::STokenNode * m_pTNodeCur;
	STokenizer m_tokenizer;
};

int local_main()
{
	return 0;
}
