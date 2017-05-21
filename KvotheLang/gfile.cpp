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
// for grammar of gfiles (written as a gfile), see gfile.gfile

// token kind

enum TOKK
{
	TOKK_KW_RULE,
	TOKK_KW_KEYWORD,

	TOKK_KW_ID,
	TOKK_KW_STRING,
	TOKK_KW_BTICK,
	TOKK_KW_TILDE,
	TOKK_KW_BANG,
	TOKK_KW_AT,
	TOKK_KW_HASH,
	TOKK_KW_DOLLAR,
	TOKK_KW_PERCENT,
	TOKK_KW_CARROT,
	TOKK_KW_AMP,
	TOKK_KW_STAR,
	TOKK_KW_LPAREN,
	TOKK_KW_RPAREN,
	TOKK_KW_PLUS,
	TOKK_KW_HBAR,
	TOKK_KW_EQUAL,
	TOKK_KW_BSLASH,
	TOKK_KW_VBAR,
	TOKK_KW_LBRACK,
	TOKK_KW_RBRACK,
	TOKK_KW_LBRACE,
	TOKK_KW_RBRACE,
	TOKK_KW_SEMICOLON,
	TOKK_KW_COLON,
	TOKK_KW_COMMA,
	TOKK_KW_LT,
	TOKK_KW_GT,
	TOKK_KW_DOT,
	TOKK_KW_FSLASH,
	TOKK_KW_QMARK,
	
	TOKK_ID,	
	TOKK_STRING,
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
	TOKK_PLUS,
	TOKK_HBAR,
	TOKK_EQUAL,
	TOKK_BSLASH,
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

	TOKK_Max,
	TOKK_Nil = -1,
	TOKK_Min = 0
};

// rule kind

enum RULEK
{
	RULEK_grammar,
	RULEK_rule,
	RULEK_ruleBody,
	RULEK_postfixOperator,
	RULEK_atom,
	RULEK_builtinToken,
	RULEK_keyword,

	RULEK_Max,
	RULEK_Nil = -1,
	RULEK_Min = 0
};

const char * PChzFromRulek(RULEK rulek)
{
	const char * aPchzRulek[] =
	{
		"RULEK_grammar",
		"RULEK_rule",
		"RULEK_ruleBody",
		"RULEK_postfixOperator",
		"RULEK_atom",
		"RULEK_builtinToken",
		"RULEK_keyword",
	};
	CASSERT(DIM(aPchzRulek) == RULEK_Max);

	assert(rulek >= 0 && rulek < RULEK_Max);

	return aPchzRulek[rulek];
}

// parse node kind

enum NODEK
{
	NODEK_Rule,
	NODEK_Token,

	NODEK_Max,
	NODEK_Nil = -1,
	NODEK_Min = 0
};

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
			printf("(%s", PChzFromRulek(m_rulek));

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
			if(m_str.size() > 0)
			{
				printf("'%s'", m_str.c_str());
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
	, m_pFile(nullptr)
	{
	}

	SParseTree::STokenNode * PTNodeNext()
	{
		static char s_mpTokkChr [] =
		{
			0,		// TOKK_KW_RULE,
			0,		// TOKK_KW_KEYWORD,
			
			0,		// TOKK_KW_ID,
			0,		// TOKK_KW_STRING,
			0,		// TOKK_KW_BTICK,
			0,		// TOKK_KW_TILDE,
			0,		// TOKK_KW_BANG,
			0,		// TOKK_KW_AT,
			0,		// TOKK_KW_HASH,
			0,		// TOKK_KW_DOLLAR,
			0,		// TOKK_KW_PERCENT,
			0,		// TOKK_KW_CARROT,
			0,		// TOKK_KW_AMP,
			0,		// TOKK_KW_STAR,
			0,		// TOKK_KW_LPAREN,
			0,		// TOKK_KW_RPAREN,
			0,		// TOKK_KW_PLUS,
			0,		// TOKK_KW_HBAR,
			0,		// TOKK_KW_EQUAL,
			0,		// TOKK_KW_BSLASH,
			0,		// TOKK_KW_VBAR,
			0,		// TOKK_KW_LBRACK,
			0,		// TOKK_KW_RBRACK,
			0,		// TOKK_KW_LBRACE,
			0,		// TOKK_KW_RBRACE,
			0,		// TOKK_KW_SEMICOLON,
			0,		// TOKK_KW_COLON,
			0,		// TOKK_KW_COMMA,
			0,		// TOKK_KW_LT,
			0,		// TOKK_KW_GT,
			0,		// TOKK_KW_DOT,
			0,		// TOKK_KW_FSLASH,
			0,		// TOKK_KW_QMARK,

			0,		// TOKK_ID,
			0,		// TOKK_STRING,

			'`',	// TOKK_BTICK,
			'~',	// TOKK_TILDE,
			'!',	// TOKK_BANG,
			'@',	// TOKK_AT,
			'#',	// TOKK_HASH,
			'$',	// TOKK_DOLLAR,
			'%',	// TOKK_PERCENT,
			'^',	// TOKK_CARROT,
			'&',	// TOKK_AMP,
			'*',	// TOKK_STAR,
			'(',	// TOKK_LPAREN,
			')',	// TOKK_RPAREN,
			'+',	// TOKK_PLUS,
			'-',	// TOKK_HBAR,
			'=',	// TOKK_EQUAL,
			'\\',	// TOKK_BSLASH
			'|',	// TOKK_VBAR,
			'[',	// TOKK_LBRACK,
			']',	// TOKK_RBRACK,
			'{',	// TOKK_LBRACE,
			'}',	// TOKK_RBRACE,
			';',	// TOKK_SEMICOLON,
			':',	// TOKK_COLON,
			',',	// TOKK_COMMA,
			'<',	// TOKK_LT,
			'>',	// TOKK_GT,
			'.',	// TOKK_DOT,
			'/',	// TOKK_FSLASH,
			'?',	// TOKK_QMARK
		};
		CASSERT(DIM(s_mpTokkChr) == TOKK_Max);

		static const char * s_mpTokkPChz [] =
		{
			"Rule",			// TOKK_KW_RULE,
			"Keyword",		// TOKK_KW_KEYWORD,
			
			"ID",			// TOKK_KW_ID,
			"STRING",		// TOKK_KW_STRING,
			"BTICK",		// TOKK_KW_BTICK,
			"TILDE",		// TOKK_KW_TILDE,
			"BANG",			// TOKK_KW_BANG,
			"AT",			// TOKK_KW_AT,
			"HASH",			// TOKK_KW_HASH,
			"DOLLAR",		// TOKK_KW_DOLLAR,
			"PERCENT",		// TOKK_KW_PERCENT,
			"CARROT",		// TOKK_KW_CARROT,
			"AMP",			// TOKK_KW_AMP,
			"STAR",			// TOKK_KW_STAR,
			"LPAREN",		// TOKK_KW_LPAREN,
			"RPAREN",		// TOKK_KW_RPAREN,
			"PLUS",			// TOKK_KW_PLUS,
			"HBAR",			// TOKK_KW_HBAR,
			"EQUAL",		// TOKK_KW_EQUAL,
			"VBAR",			// TOKK_KW_VBAR,
			"LBRACK",		// TOKK_KW_LBRACK,
			"RBRACK",		// TOKK_KW_RBRACK,
			"LBRACE",		// TOKK_KW_LBRACE,
			"RBRACE",		// TOKK_KW_RBRACE,
			"SEMICOLON",	// TOKK_KW_SEMICOLON,
			"COLON",		// TOKK_KW_COLON,
			"COMMA",		// TOKK_KW_COMMA,
			"LT",			// TOKK_KW_LT,
			"GT",			// TOKK_KW_GT,
			"DOT",			// TOKK_KW_DOT,
			"FSLASH",		// TOKK_KW_FSLASH,
			"QMARK",		// TOKK_KW_QMARK,

			nullptr,		// TOKK_ID,
			nullptr,		// TOKK_STRING,

			nullptr,		// TOKK_BTICK,
			nullptr,		// TOKK_TILDE,
			nullptr,		// TOKK_BANG,
			nullptr,		// TOKK_AT,
			nullptr,		// TOKK_HASH,
			nullptr,		// TOKK_DOLLAR,
			nullptr,		// TOKK_PERCENT,
			nullptr,		// TOKK_CARROT,
			nullptr,		// TOKK_AMP,
			nullptr,		// TOKK_STAR,
			nullptr,		// TOKK_LPAREN,
			nullptr,		// TOKK_RPAREN,
			nullptr,		// TOKK_PLUS,
			nullptr,		// TOKK_HBAR,
			nullptr,		// TOKK_EQUAL,
			nullptr,		// TOKK_VBAR,
			nullptr,		// TOKK_LBRACK,
			nullptr,		// TOKK_RBRACK,
			nullptr,		// TOKK_LBRACE,
			nullptr,		// TOKK_RBRACE,
			nullptr,		// TOKK_SEMICOLON,
			nullptr,		// TOKK_COLON,
			nullptr,		// TOKK_COMMA,
			nullptr,		// TOKK_LT,
			nullptr,		// TOKK_GT,
			nullptr,		// TOKK_DOT,
			nullptr,		// TOKK_FSLASH,
			nullptr,		// TOKK_QMARK
		};
		CASSERT(DIM(s_mpTokkChr) == TOKK_Max);

		// BB handle literals generally
		// int literals https://msdn.microsoft.com/en-us/library/aa664674(v=vs.71).aspx
		// float literals https://msdn.microsoft.com/en-us/library/aa691085(v=vs.71).aspx
		// char literals https://msdn.microsoft.com/en-us/library/aa691087(v=vs.71).aspx
		// string literals https://msdn.microsoft.com/en-us/library/aa691090(v=vs.71).aspx

		// BB handle comments
		
		while (ChrCur() != EOF)
		{
			// check for string
			
			if(ChrCur() == '"')
			{
				ConsumeChar();
				
				string str = "";

				while(ChrCur() != '"' && ChrCur() != '\n' && ChrCur() != '\r' && ChrCur() != '\v' && ChrCur() != '\f' )
				{
					str += ConsumeChar();
				}

				if(ChrCur() != '"')
				{
					assert(false);
				}

				ConsumeChar();

				return m_parsetree.PTNodeCreate(TOKK_STRING, str);
			}

			// skip white space

			if(isspace(ChrCur()))
			{
				ConsumeChar();
				continue;
			}

			// check for single symbol
			
			for(TOKK tokk = TOKK_Min; tokk < TOKK_Max; tokk = (TOKK)((int)tokk + 1))
			{
				char chrTest = s_mpTokkChr[tokk];
				if(chrTest != 0 && ChrCur() == chrTest)
				{
					return m_parsetree.PTNodeCreate(tokk, string(1, ConsumeChar()));
				}
			}
			
			if (ChrCur() == '_' || isdigit(ChrCur()) || isalpha(ChrCur()))
			{
				// check for keyword or ID
			
				string strTok = "";
				
				while(ChrCur() == '_' || isdigit(ChrCur()) || isalpha(ChrCur()))
				{
					strTok+=ConsumeChar();
				}

				for(TOKK tokk = TOKK_Min; tokk < TOKK_Max; tokk = (TOKK)((int)tokk + 1))
				{
					const char * pChzTest = s_mpTokkPChz[tokk];
					if(pChzTest && strcmp(pChzTest, strTok.c_str()) == 0)
						return m_parsetree.PTNodeCreate(tokk, string(pChzTest)); // keyword
				}

				return m_parsetree.PTNodeCreate(TOKK_ID, strTok); // ID
			}

			// unrecognized char

			assert(false);	
		}

		return nullptr; // BB return EOF/EOI token instead?
	}

	char ChrCur()
	{
		return m_chrCur;
	}

	char ConsumeChar()
	{
		char chrPrev = m_chrCur;
		m_chrCur = (char)fgetc(m_pFile);

		return chrPrev;
	}

	void SetInput(FILE * pFile)
	{
		m_pFile = pFile;
		fseek(m_pFile, 0, SEEK_SET);
		ConsumeChar();
	}

	char m_chrCur;
	FILE *			m_pFile;

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
		else if(TokkCur() == TOKK_KW_KEYWORD)
		{
			pRNode->AddChild(PRNodeKeyword());
		}

		while(TokkCur() == TOKK_KW_RULE || TokkCur() == TOKK_KW_KEYWORD)
		{
			if(TokkCur() == TOKK_KW_RULE)
			{
				pRNode->AddChild(PRNodeRule());
			}
			else if(TokkCur() == TOKK_KW_KEYWORD)
			{
				pRNode->AddChild(PRNodeKeyword());
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

		while(TokkCur() == TOKK_VBAR || TokkCur() == TOKK_ID || TokkCur() == TOKK_LPAREN || (TokkCur() < TOKK_ID && TokkCur() >= TOKK_KW_KEYWORD))
		{
			if(TokkCur() == TOKK_VBAR)
			{
				pRNode->AddChild(m_pTNodeCur);
				ReadToken();
			}

			pRNode->AddChild(PRNodeAtom());

			if(TokkCur() == TOKK_QMARK || TokkCur() == TOKK_PLUS || TokkCur() == TOKK_STAR)
			{
				pRNode->AddChild(PRNodePostfixOperator());
			}
		}

		return pRNode;
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
		TOKK aTokk[] = 
		{
			TOKK_KW_ID,
			TOKK_KW_STRING,
			TOKK_KW_BTICK,
			TOKK_KW_TILDE,
			TOKK_KW_BANG,
			TOKK_KW_AT,
			TOKK_KW_HASH,
			TOKK_KW_DOLLAR,
			TOKK_KW_PERCENT,
			TOKK_KW_CARROT,
			TOKK_KW_AMP,
			TOKK_KW_STAR,
			TOKK_KW_LPAREN,
			TOKK_KW_RPAREN,
			TOKK_KW_PLUS,
			TOKK_KW_HBAR,
			TOKK_KW_EQUAL,
			TOKK_KW_VBAR,
			TOKK_KW_LBRACK,
			TOKK_KW_RBRACK,
			TOKK_KW_LBRACE,
			TOKK_KW_RBRACE,
			TOKK_KW_SEMICOLON,
			TOKK_KW_COLON,
			TOKK_KW_COMMA,
			TOKK_KW_LT,
			TOKK_KW_GT,
			TOKK_KW_DOT,
			TOKK_KW_FSLASH,
			TOKK_KW_QMARK,
		};

		for(TOKK tokk : aTokk)
		{
			if(TokkCur() == tokk)
			{
				SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_builtinToken);
				pRNode->AddChild(m_pTNodeCur);
				ReadToken();

				return pRNode;
			}
		}
		
		assert(false);
		return nullptr;
	}

	SParseTree::SRuleNode * PRNodeKeyword()
	{
		SParseTree::SRuleNode * pRNode = PParseTree()->PRNodeCreate(RULEK_keyword);
		
		assert(TokkCur() == TOKK_KW_KEYWORD);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		assert(TokkCur() == TOKK_ID);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		assert(TokkCur() == TOKK_COLON);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		assert(TokkCur() == TOKK_STRING);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		assert(TokkCur() == TOKK_SEMICOLON);
		pRNode->AddChild(m_pTNodeCur);
		ReadToken();

		return pRNode;
	}

	void ReadToken()
	{
		m_pTNodeCur = m_tokenizer.PTNodeNext();
	}

	TOKK TokkCur()
	{
		return m_pTNodeCur ? m_pTNodeCur->m_tokk : TOKK_Nil;
	}
	
	SParseTree * PParseTree()
	{
		return &m_tokenizer.m_parsetree;
	}

	void SetInput(FILE * pFile)
	{
		m_tokenizer.SetInput(pFile);
		ReadToken();
	}

	SParseTree::STokenNode * m_pTNodeCur;
	STokenizer m_tokenizer;
};

int local_main()
{
	const char * pChzFileName = "gfile.gfile";

	FILE * pFile = fopen(pChzFileName, "r");

	SParser parser;

	parser.SetInput(pFile);

	SParseTree::SRuleNode * pRNodeGrammar = parser.PRNodeGrammar();

	pRNodeGrammar->PrintDebug();

	return 0;
}
