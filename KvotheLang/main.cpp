#include <string>
#include <ctype.h>
#include <stdio.h>
#include <stack>
#include <vector>
#include <assert.h>
#include <memory>
#include <stdio.h>
#include <functional>

using std::string;
using std::stack;
using std::vector;
using std::shared_ptr;
using std::function;

// http://stackoverflow.com/a/6256085

#define DIM(arr) ( \
   0 * sizeof(reinterpret_cast<const ::Bad_arg_to_DIM*>(arr)) + \
   0 * sizeof(::Bad_arg_to_DIM::check_type((arr), &(arr))) + \
   sizeof(arr) / sizeof((arr)[0]) )

struct Bad_arg_to_DIM {
   class Is_pointer; // incomplete
   class Is_array {};
   template <typename T>
   static Is_pointer check_type(const T*, const T* const*);
   static Is_array check_type(const void*, const void*);
};

#define ARY(NAME, TYPE, ...) TYPE NAME ## _underlying_array [] = {__VA_ARGS__}; const SAry<TYPE> NAME = {NAME ## _underlying_array, DIM(NAME ## _underlying_array)}
#define CASSERT(predicate) static_assert(predicate, #predicate)

template <typename T>
struct SAry
{
	const T*	m_a;
	const int	m_c;
};

// Tiny C
// http://www.iro.umontreal.ca/~felipe/IFT2030-Automne2002/Complements/tinyc.c

// grammar
// factored to remove left recursion (because we are an LL parser)
// factored so that the sets of starting tokens of all alts of a rule are disjoint (because we are an LL(1) parser)
// in the case where an alt begins with an optional rule, the set of starting tokens includes the starting tokens of the next rule in the alt if there is one (which could also be an optional, etc)
// factored to not use +, * or | within an alt. ? is supported with OPTIONAL.
// OPTIONAL means the rule can go the the empty string (i.e. if we cant go into one of the other alts, just skip this rule)
// CammelCase is a rule, UPPERCASE is a token
// text in '' means an implicet token (i.e. 'while' implies a token WHILE : 'while')
/*
	program
		: statement statement_list
		;
		
	statement_list
		: OPTIONAL
		| statement statement_list
		;

	statement
		: 'if' paren_expr statement if_rhs
		| 'while' paren_expr statement
		| 'do' statement 'while' paren_expr ';'
		| '{' statement_list '}'
		| ID '=' testExpr ';'
		;

	if_rhs
		: OPTIONAL
		| 'else' statement
		;

	parenExpr
		: '(' test_expr ')'
		;

	testExpr
		: sum testExpr_rhs
		;
		
	testExpr_rhs
		: OPTIONAL
		| '<' sum
		; 

	sum
		: atom sum_rhs
		;
		
	sum_rhs
		: OPTIONAL
		| '+' sum
		| '-' sum
		;

	atom
		: ID
		| INT
		| parenExpr
		;

	ID	: [a-z]
	INT : [0-9]+
*/

// parse node kind (both tokens and rules)

enum NODEK
{
	NODEK_NIL,
	NODEK_EOI,
	NODEK_INT,
	NODEK_ID,
	NODEK_PLUS,
	NODEK_SUB,
	NODEK_LT,
	NODEK_LPAREN,
	NODEK_RPAREN,
	NODEK_EQUALS,
	NODEK_LBRACE,
	NODEK_RBRACE,
	NODEK_DO,
	NODEK_WHILE,
	NODEK_IF,
	NODEK_ELSE,
	NODEK_SEMICOLON,

	NODEK_TOKMAX,

	NODEK_Atom = NODEK_TOKMAX,
	NODEK_Sum,
	NODEK_SumRHS,
	NODEK_TestExpr,
	NODEK_TestExprRHS,
	NODEK_ParenExpr,
	NODEK_Statement,
	NODEK_IfRHS,
	NODEK_StatementList,
	NODEK_Program,

	NODEK_Max,
};

typedef SAry<const NODEK> Alt;
typedef SAry<const Alt*> Rule;

ARY(g_altOptional, const NODEK, 
	NODEK_NIL);

ARY(g_altAtom1, const NODEK, 
	NODEK_ID);

ARY(g_altAtom2, const NODEK, 
	NODEK_INT);

ARY(g_altAtom3, const NODEK, 
	NODEK_ParenExpr);

ARY(g_ruleAtom, const Alt*, 
	&g_altAtom1, 
	&g_altAtom2, 
	&g_altAtom3);

ARY(g_altSum1, const NODEK, 
	NODEK_Atom, NODEK_SumRHS);

ARY(g_ruleSum, const Alt*, 
	&g_altSum1);

ARY(g_altSumRHS1, const NODEK, 
	NODEK_PLUS, NODEK_Sum);

ARY(g_altSumRHS2, const NODEK, 
	NODEK_SUB, NODEK_Sum);

ARY(g_ruleSumRHS, const Alt*, 
	&g_altOptional, 
	&g_altSumRHS1, 
	&g_altSumRHS2);

ARY(g_altTestExpr1, const NODEK, 
	NODEK_Sum, NODEK_TestExprRHS);

ARY(g_ruleTestExpr, const Alt*, 
	&g_altTestExpr1);

ARY(g_altTestExprRHS1, const NODEK, 
	NODEK_LT, NODEK_Sum);

ARY(g_ruleTestExprRHS, const Alt*, 
	&g_altOptional,
	&g_altTestExprRHS1);

ARY(g_altParenExpr1, const NODEK, 
	NODEK_LPAREN, NODEK_TestExpr, NODEK_RPAREN);

ARY(g_ruleParenExpr, const Alt*, 
	&g_altParenExpr1);

ARY(g_altStatement1, const NODEK, 
	NODEK_IF, NODEK_ParenExpr, NODEK_Statement, NODEK_IfRHS);

ARY(g_altStatement2, const NODEK, 
	NODEK_WHILE, NODEK_ParenExpr, NODEK_Statement);

ARY(g_altStatement3, const NODEK, 
	NODEK_DO, NODEK_Statement, NODEK_WHILE, NODEK_ParenExpr, NODEK_SEMICOLON);

ARY(g_altStatement4, const NODEK, 
	NODEK_LBRACE, NODEK_StatementList, NODEK_RBRACE);

ARY(g_altStatement5, const NODEK, 
	NODEK_ID, NODEK_EQUALS, NODEK_TestExpr, NODEK_SEMICOLON);

ARY(g_ruleStatement, const Alt*, 
	&g_altStatement1,
	&g_altStatement2,
	&g_altStatement3,
	&g_altStatement4,
	&g_altStatement5);

ARY(g_altIfRHS1, const NODEK, 
	NODEK_ELSE, NODEK_Statement);

ARY(g_ruleIfRHS, const Alt*, 
	&g_altOptional,
	&g_altIfRHS1);

ARY(g_altStatementList1, const NODEK, 
	NODEK_Statement, NODEK_StatementList);

ARY(g_ruleStatementList, const Alt*, 
	&g_altOptional,
	&g_altStatementList1);

ARY(g_altProgram1, const NODEK, 
	NODEK_Statement, NODEK_StatementList);

ARY(g_ruleProgram, const Alt*, 
	&g_altProgram1);

ARY(g_mpRulekRule, const Rule*, 
	&g_ruleAtom, 
	&g_ruleSum, 
	&g_ruleSumRHS, 
	&g_ruleTestExpr, 
	&g_ruleTestExprRHS, 
	&g_ruleParenExpr, 
	&g_ruleStatement, 
	&g_ruleIfRHS, 
	&g_ruleStatementList, 
	&g_ruleProgram);

// node in the parse tree

struct SParseNode // tag = node
{
	SParseNode(NODEK nodek, string str = "")
	: m_aryPNodeChild() 
	, m_nodek(nodek)
	, m_str(str)
	{
		assert(m_nodek < NODEK_TOKMAX || m_str.size() == 0);
	}

	void AddChild(SParseNode * pNodeChild)
	{
		assert(m_nodek >= NODEK_TOKMAX);
		m_aryPNodeChild.push_back(pNodeChild);
	}

	SParseNode * PNodeChild(unsigned int nChild)
	{
		assert(nChild < m_aryPNodeChild.size() && nChild >= 0);
		return m_aryPNodeChild[nChild];
	}

	int CChild()
	{
		return m_aryPNodeChild.size();
	}

	void PrintDebug()
	{
		assert(m_nodek != NODEK_NIL);

		if(m_nodek < NODEK_TOKMAX)
		{
			assert(m_str.size() == 0);

			printf("(");

			switch (m_nodek)
			{
				case NODEK_Atom:
				printf("Atom");
				break;
				case NODEK_Sum:
				printf("Sum");
				break;
				case NODEK_TestExpr:
				printf("TestExpr");
				break;
				case NODEK_ParenExpr:
				printf("ParenExpr");
				break;
				case NODEK_Statement:
				printf("Statement");
				break;
				case NODEK_Program:
				printf("Program");
				break;
				default:
				assert(false);
				break;
			}

			for(SParseNode * pNode : m_aryPNodeChild)
			{
				printf(" ");
				pNode->PrintDebug();
			}

			printf(")");
		}
		else
		{
			assert(m_aryPNodeChild.size() == 0);
			
			switch (m_nodek)
			{
				case NODEK_INT:
				printf("INT");
				break;
				case NODEK_ID:
				printf("ID");
				break;
				case NODEK_PLUS:
				printf("PLUS");
				break;
				case NODEK_SUB:
				printf("SUB");
				break;
				case NODEK_LT:
				printf("LT");
				break;
				case NODEK_LPAREN:
				printf("LPAREN");
				break;
				case NODEK_RPAREN:
				printf("RPAREN");
				break;
				case NODEK_EQUALS:
				printf("EQUALS");
				break;
				case NODEK_LBRACE:
				printf("LBRACE");
				break;
				case NODEK_RBRACE:
				printf("RBRACE");
				break;
				case NODEK_DO:
				printf("DO");
				break;
				case NODEK_WHILE:
				printf("WHILE");
				break;
				case NODEK_IF:
				printf("IF");
				break;
				case NODEK_ELSE:
				printf("ELSE");
				break;
				case NODEK_SEMICOLON:
				printf("SEMICOLON");
				break;
				default:
				assert(false);
				break;
			}

			if(m_str.size() > 0)
			{
				printf(" '%s'", m_str.c_str());
			}
		}
	}
	
	vector<SParseNode *>			m_aryPNodeChild;

	NODEK							m_nodek;
	string							m_str;
};

// parse tree

struct SParseTree // tag = parsetree
{
	SParseNode *	PNodeCreate(NODEK nodek, string str = "")
	{
		shared_ptr<SParseNode> pNode(new SParseNode(nodek, str));
		m_aryPNode.push_back(pNode);
		return pNode.get();
	}

	vector<shared_ptr<SParseNode>> m_aryPNode;
};

// tokenizer

struct STokenizer // tag = tokenizer
{
	STokenizer()
	: m_parsetree()
	, m_iChrCur(0)
	, m_strInput("")
	{
	}

	SParseNode * PNodeTokNext()
	{
		while (m_iChrCur < m_strInput.length())
		{
			char chrCur = m_strInput[m_iChrCur];
			if (isspace(chrCur)) { ++m_iChrCur; } // skip whitespace
			else if (isdigit(chrCur)) // NODEK_INT
			{
				string strInt = "";

				do
				{
					strInt += chrCur;
					++m_iChrCur;
					chrCur = m_strInput[m_iChrCur];
				} while (isdigit(chrCur) && m_iChrCur < m_strInput.length());

				return m_parsetree.PNodeCreate(NODEK_INT, strInt);
			}
			else if (isalpha(chrCur)) // keyword or ID
			{
				string strToken = "";

				do
				{
					strToken += chrCur;
					++m_iChrCur;
					chrCur = m_strInput[m_iChrCur];
				} while (isalpha(chrCur) && m_iChrCur < m_strInput.length());

				NODEK nodekTokKeyword = NodekTokKeywordFromString(strToken);
				if (nodekTokKeyword != NODEK_NIL) // keyword
				{
					return m_parsetree.PNodeCreate(nodekTokKeyword, strToken);
				}
				else if (strToken.length() == 1) // NODEK_Id
				{
					if (isupper(chrCur))
					{
						assert(false);
					}
					else
					{
						return m_parsetree.PNodeCreate(NODEK_ID, strToken);
					}
				}
				else
				{
					assert(false);
				}
			}
			else if (chrCur == '+') { ++m_iChrCur; return m_parsetree.PNodeCreate(NODEK_PLUS); }
			else if (chrCur == '-') { ++m_iChrCur; return m_parsetree.PNodeCreate(NODEK_SUB); }
			else if (chrCur == '<') { ++m_iChrCur; return m_parsetree.PNodeCreate(NODEK_LT); }
			else if (chrCur == '(') { ++m_iChrCur; return m_parsetree.PNodeCreate(NODEK_LPAREN); }
			else if (chrCur == ')') { ++m_iChrCur; return m_parsetree.PNodeCreate(NODEK_RPAREN); }
			else if (chrCur == '=') { ++m_iChrCur; return m_parsetree.PNodeCreate(NODEK_EQUALS); }
			else if (chrCur == '{') { ++m_iChrCur; return m_parsetree.PNodeCreate(NODEK_LBRACE); }
			else if (chrCur == '}') { ++m_iChrCur; return m_parsetree.PNodeCreate(NODEK_RBRACE); }
			else if (chrCur == ';') { ++m_iChrCur; return m_parsetree.PNodeCreate(NODEK_SEMICOLON); }
			else
			{
				assert(false);
			}
		}
		return  m_parsetree.PNodeCreate(NODEK_EOI);
	}

	void SetInput(string str)
	{
		m_strInput = str;
		m_iChrCur = 0;
	}

	NODEK NodekTokKeywordFromString(string str)
	{
		if(str.compare("do") == 0)
		{
			return NODEK_DO;
		}
		else if(str.compare("while") == 0)
		{
			return NODEK_WHILE;
		}
		else if(str.compare("if") == 0)
		{
			return NODEK_IF;
		}
		else if(str.compare("else") == 0)
		{
			return NODEK_ELSE;
		}
		else
		{
			return NODEK_NIL;
		}
	}

	string			m_strInput;
	unsigned int	m_iChrCur;

	SParseTree		m_parsetree;
};

struct SParser
{
	typedef vector<NODEK> alt;
	typedef vector<alt> rule;
	
	// parse a program
	
	SParseNode * PNodeProgNext()
	{
		/*
			program
				: statement +
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(NODEK_Program);

		while (NodekTokCur() != NODEK_EOI)
		{
			pNode->AddChild(PNodeStatNext()); // statement
		}

		return pNode;
	}

	// parse a statement

	SParseNode * PNodeStatNext()
	{
		/*
			statement
				: 'if' paren_expr statement
				| 'if' paren_expr statement 'else' statement
				| 'while' paren_expr statement
				| 'do' statement 'while' paren_expr ';'
				| '{' statement* '}'
				| ID '=' test_expr ';'
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(NODEK_Statement);

		if (NodekTokCur() == NODEK_IF)
		{
			// : 'if' paren_expr statement
			// | 'if' paren_expr statement 'else' statement
			
			pNode->AddChild(PNodeTokConsume(NODEK_IF)); // 'if'

			pNode->AddChild(PNodeParenExprNext()); // paren_expr 

			pNode->AddChild(PNodeStatNext()); // statement

			if (NodekTokCur() == NODEK_ELSE)
			{
				pNode->AddChild(PNodeTokConsume(NODEK_ELSE)); // 'else'

				pNode->AddChild(PNodeStatNext()); // statement
			}
		}
		else if (NodekTokCur() == NODEK_WHILE)
		{
			// 'while' paren_expr statement
			
			pNode->AddChild(PNodeTokConsume(NODEK_WHILE)); // 'while'

			pNode->AddChild(PNodeParenExprNext()); // paren_expr

			pNode->AddChild(PNodeStatNext()); // statement
		}
		else if (NodekTokCur() == NODEK_DO)
		{
			// 'do' statement 'while' paren_expr ';'
			
			pNode->AddChild(PNodeTokConsume(NODEK_DO)); // 'do'

			pNode->AddChild(PNodeStatNext()); // statement

			pNode->AddChild(PNodeTokConsume(NODEK_WHILE)); // 'while'

			pNode->AddChild(PNodeParenExprNext()); // paren_expr

			pNode->AddChild(PNodeTokConsume(NODEK_SEMICOLON)); // ';'
		}
		else if (NodekTokCur() == NODEK_LBRACE)
		{
			// '{' statement* '}'
			
			pNode->AddChild(PNodeTokConsume(NODEK_LBRACE)); // '{'

			while (NodekTokCur() != NODEK_RBRACE)
			{
				pNode->AddChild(PNodeStatNext()); // statement
			}

			pNode->AddChild(PNodeTokConsume(NODEK_RBRACE)); // '}'
		}
		else if (NodekTokCur() == NODEK_ID)
		{
			// ID '=' test_expr ';'
			
			pNode->AddChild(PNodeTokConsume(NODEK_ID)); // ID

			pNode->AddChild(PNodeTokConsume(NODEK_EQUALS)); // '='

			pNode->AddChild(PNodeTestExprNext()); // test_expr

			pNode->AddChild(PNodeTokConsume(NODEK_SEMICOLON)); // ';'
		}
		else
		{
			assert(false);
		}

		return pNode;
	}

	// parse a parenExpr

	SParseNode * PNodeParenExprNext()
	{
		/*
			parenExpr
				: '(' test_expr ')'
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(NODEK_ParenExpr);

		pNode->AddChild(PNodeTokConsume(NODEK_LPAREN)); // '('

		pNode->AddChild(PNodeTestExprNext()); // test_expr

		pNode->AddChild(PNodeTokConsume(NODEK_RPAREN)); // ')'

		return pNode;
	}

	// parse a testExpr

	SParseNode * PNodeTestExprNext()
	{
		/*
			testExpr
				: sum
				| sum '<' sum
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(NODEK_TestExpr);

		pNode->AddChild(PNodeSumNext()); // sum

		if (NodekTokCur() == NODEK_LT)
		{
			pNode->AddChild(PNodeTokConsume(NODEK_LT)); // '<'

			pNode->AddChild(PNodeSumNext()); // sum
		}
		return pNode;
	}

	// parse a sum

	SParseNode * PNodeSumNext()
	{
		/*
			sum
				: atom
				| atom '+' sum
				| atom '-' sum
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(NODEK_Sum);

		pNode->AddChild(PNodeAtomNext()); // atom

		if (NodekTokCur() == NODEK_PLUS)
		{
			pNode->AddChild(PNodeTokConsume(NODEK_PLUS)); // '+'

			pNode->AddChild(PNodeSumNext()); //  sum
		}
		else if (NodekTokCur() == NODEK_SUB)
		{
			pNode->AddChild(PNodeTokConsume(NODEK_SUB)); // '-'

			pNode->AddChild(PNodeSumNext()); // sum
		}

		return pNode;
	}

	// parse an atom

	SParseNode * PNodeAtomNext()
	{
		/*
			atom
				: ID
				| INT
				| parenExpr
				;
		*/
		
		SParseNode * pNode = PParseTree()->PNodeCreate(NODEK_Atom);

		if (NodekTokCur() == NODEK_INT) { pNode->AddChild(PNodeTokConsume(NODEK_INT)); }  // ID
		else if (NodekTokCur() == NODEK_ID) { pNode->AddChild(PNodeTokConsume(NODEK_ID)); } // INT
		else if (NodekTokCur() == NODEK_LPAREN) 
		{ 
			pNode->AddChild(PNodeParenExprNext()); // parenExpr
		} 
		else
		{
			assert(false);
		}

		return pNode;
	}

	void SetInput(string strInput)
	{
		m_tokenizer.SetInput(strInput);
		m_pNodeTokCur = m_tokenizer.PNodeTokNext();
	}

	NODEK NodekTokCur()
	{
		return m_pNodeTokCur->m_nodek;
	}

	SParseNode * PNodeTokConsume(NODEK nodek)
	{
		assert(nodek < NODEK_TOKMAX);
		assert(m_pNodeTokCur->m_nodek == nodek);
		
		// return current token and get next token

		SParseNode * pNodeTokMatch = m_pNodeTokCur;
		m_pNodeTokCur = m_tokenizer.PNodeTokNext();

		return pNodeTokMatch;
	}

	SParseTree * PParseTree()
	{
		return &m_tokenizer.m_parsetree;
	}

	SParseNode * m_pNodeTokCur;
	STokenizer m_tokenizer;
};

// binary operation kind

enum BINOPK
{
	BINOPK_Nil,
	BINOPK_LT,
	BINOPK_PLUS,
	BINOPK_SUB
};

// expression kind

enum EXPRK
{
	EXPRK_Nil,
	EXPRK_Binop,
	EXPRK_Int,
	EXPRK_Id
};

// an AST expression

struct SExpression 
{
	SExpression(EXPRK exprk)
	: m_exprk(exprk)
	{
	}
	
	EXPRK m_exprk;

	// EXPRK_Binop

	BINOPK m_binopk;
	SExpression * m_pExprLeft = nullptr;
	SExpression * m_pExprRight = nullptr;

	// EXPRK_Int

	unsigned int m_n;

	// EXPRK_Id

	unsigned int m_iGlobal;

	void PrintDebug()
	{
		assert(m_exprk != EXPRK_Nil);
		
		switch (m_exprk)
		{
			case EXPRK_Binop:
				assert(m_binopk != BINOPK_Nil);
				printf("( ");
				switch (m_binopk)
				{
					case BINOPK_LT: 
						printf("<");
						break;
					case BINOPK_PLUS: 
						printf("+");
						break;
					case BINOPK_SUB:
						printf("-");
						break;
				}
				printf(" ");
				m_pExprLeft->PrintDebug();
				printf(" ");
				m_pExprRight->PrintDebug();
				printf(" )");
				break;
			case EXPRK_Int: 
				printf("%d", m_n);
				break;
			case EXPRK_Id: 
				printf("%c", (char)(m_iGlobal + 'a'));
				break;
			default:
				break;
		}
	}
};

// statement kind

enum STATK
{
	STATK_Nil,
	STATK_Statements,
	STATK_If,
	STATK_While,
	STATK_DoWhile,
	STATK_Assign,
};

// an ast statement

struct SStatement 
{
	SStatement(STATK statk)
	: m_statk(statk)
	{
	}
	
	STATK m_statk;

	// STATK_Statements
		
	vector<SStatement *> m_aryPStat;

	// STATK_If
	
	SStatement * m_pStatElse = nullptr;

	// STATK_If / STATK_While / STATK_DoWhile

	SExpression * m_pExprCondition = nullptr;
	SStatement * m_pStatBody = nullptr;

	// STATK_Assign
		
	SExpression * m_pId = nullptr; // must be EXPRK_Id
	SExpression * m_pExpr = nullptr;

	void PrintDebug()
	{
		assert(m_statk != STATK_Nil);
		
		switch (m_statk)
		{
			case STATK_Statements:
				printf("( ");
				for(unsigned int i = 0; i < m_aryPStat.size(); ++i)
				{
					m_aryPStat[i]->PrintDebug();
					printf(" ");
				}
				printf(")");
				break;
			case STATK_If:
				printf("( ifelse ");
				m_pExprCondition->PrintDebug();
				printf(" ");
				m_pStatBody->PrintDebug();
				printf(" ");
				if(m_pStatElse)
				{
					m_pStatElse->PrintDebug();
					printf(" ");
				}
				printf(")");
				break;
			case STATK_While:
				printf("( while ");
				m_pExprCondition->PrintDebug();
				printf(" ");
				m_pStatBody->PrintDebug();
				printf(" )");
				break;
			case STATK_DoWhile:
				printf("( dowhile ");
				m_pStatBody->PrintDebug();
				printf(" ");
				m_pExprCondition->PrintDebug();
				printf(" )");
				break;
			case STATK_Assign:
				printf("( = ");
				m_pId->PrintDebug();
				printf(" ");
				m_pExpr->PrintDebug();
				printf(" )");
				break;
		}
	}
};

// the AST

struct SAst
{
	SExpression * PExprCreate(EXPRK exprk)
	{
		shared_ptr<SExpression> pExpr(new SExpression(exprk));
		m_aryPExpr.push_back(pExpr);
		return pExpr.get();
	}

	SExpression * PExprFromPNode(SParseNode * pNode)
	{
		SExpression * pExpr = nullptr;
		
		if (pNode->m_nodek == NODEK_INT)
		{
			// INT
			
			pExpr = PExprCreate(EXPRK_Int);

			assert(pNode->m_str.size() > 0);
			pExpr->m_n = atoi(&pNode->m_str[0]);
		}
		else if (pNode->m_nodek == NODEK_ID)
		{
			// ID
			
			pExpr = PExprCreate(EXPRK_Id);

			assert(pNode->m_str.size() == 1);
			pExpr->m_iGlobal = pNode->m_str[0] - 'a';
		}
		else if (pNode->m_nodek == NODEK_Atom)
		{
			/*
				atom
					: ID
					| INT
					| parenExpr
					;
			*/
			
			assert(pNode->CChild() == 1);
			pExpr = PExprFromPNode(pNode->PNodeChild(0)); // ID | INT | parenExpr
		}
		else if (pNode->m_nodek == NODEK_ParenExpr)
		{
			/*
				parenExpr
					: '(' test_expr ')'
					;
			*/
			
			assert(pNode->CChild() == 3);
			pExpr = PExprFromPNode(pNode->PNodeChild(1)); // test_expr
		}
		else if (pNode->m_nodek == NODEK_Sum || pNode->m_nodek == NODEK_TestExpr)
		{
			/*
				testExpr
					: sum
					| sum '<' sum
					;

				sum
					: atom
					| atom '+' sum
					| atom '-' sum
					;
			*/
			
			if (pNode->CChild() == 3)
			{
				pExpr = PExprCreate(EXPRK_Binop);

				pExpr->m_pExprLeft = PExprFromPNode(pNode->PNodeChild(0)); // sum | atom

				pExpr->m_binopk = BinopkFromNodekTok(pNode->PNodeChild(1)->m_nodek); // '<' | '+' | '-'
				assert(pExpr->m_binopk != BINOPK_Nil);

				pExpr->m_pExprRight = PExprFromPNode(pNode->PNodeChild(2)); // sum
			}
			else
			{
				assert(pNode->CChild() == 1);

				pExpr = PExprFromPNode(pNode->PNodeChild(0)); // sum | atom
			}
		}
		else
		{
			assert(false);
		}

		return pExpr;
	}

	SStatement * PStatCreate(STATK statk)
	{
		shared_ptr<SStatement> pStat(new SStatement(statk));
		m_aryPStat.push_back(pStat);
		return pStat.get();
	}

	SStatement * PStatFromPNode(SParseNode * pNode)
	{
		SStatement * pStat = nullptr;

		// a program or statement

		if (pNode->m_nodek == NODEK_Program)
		{
			/*
				program
					: statement +
					;
			*/

			pStat = PStatCreate(STATK_Statements);

			for (int iChild = 0; iChild < pNode->CChild(); ++iChild)
			{
				pStat->m_aryPStat.push_back(PStatFromPNode(pNode->PNodeChild(iChild))); // statement
			}
		}
		else if (pNode->m_nodek == NODEK_Statement)
		{
			/*
				statement
					: 'if' paren_expr statement
					| 'if' paren_expr statement 'else' statement
					| 'while' paren_expr statement
					| 'do' statement 'while' paren_expr ';'
					| '{' statement* '}'
					| ID '=' atom ';'
					;
			*/

			assert(pNode->CChild() != 0);

			NODEK nodekChildFirst = pNode->PNodeChild(0)->m_nodek;

			if (nodekChildFirst == NODEK_IF)
			{
				// : 'if' paren_expr statement
				// | 'if' paren_expr statement 'else' statement

				pStat = PStatCreate(STATK_If);

				assert(pNode->CChild() == 3 || pNode->CChild() == 5);
				
				pStat->m_pExprCondition = PExprFromPNode(pNode->PNodeChild(1)); // paren_expr
				pStat->m_pStatBody = PStatFromPNode(pNode->PNodeChild(2)); // statement
				
				if(pNode->CChild() == 5)
				{
					pStat->m_pStatBody = PStatFromPNode(pNode->PNodeChild(4)); // statement
				}
			}
			else if (nodekChildFirst == NODEK_WHILE)
			{
				// | 'while' paren_expr statement
				
				assert(pNode->CChild() == 3);
				
				pStat = PStatCreate(STATK_While);
				pStat->m_pExprCondition = PExprFromPNode(pNode->PNodeChild(1)); // paren_expr
				pStat->m_pStatBody = PStatFromPNode(pNode->PNodeChild(2)); // statement
			}
			else if (nodekChildFirst == NODEK_DO)
			{
				// | 'do' statement 'while' paren_expr ';'
				
				assert(pNode->CChild() == 5);
				
				pStat = PStatCreate(STATK_DoWhile);
				pStat->m_pStatBody = PStatFromPNode(pNode->PNodeChild(1)); // statement
				pStat->m_pExprCondition = PExprFromPNode(pNode->PNodeChild(3)); // paren_expr
			}
			else if (nodekChildFirst == NODEK_LBRACE)
			{
				// | '{' statement* '}'

				pStat = PStatCreate(STATK_Statements);
				
				assert(pNode->CChild() >= 2);
				assert(pNode->PNodeChild(pNode->CChild() - 1)->m_nodek == NODEK_RBRACE);

				for(int iChild = 1; iChild < pNode->CChild() - 1; ++iChild)
				{
					pStat->m_aryPStat.push_back(PStatFromPNode(pNode->PNodeChild(iChild))); // statement
				}
			}
			else if (nodekChildFirst == NODEK_ID)
			{
				// | ID '=' atom ';'
				
				assert(pNode->CChild() == 4);

				pStat = PStatCreate(STATK_Assign);
				pStat->m_pId = PExprFromPNode(pNode->PNodeChild(0)); // ID
				pStat->m_pExpr = PExprFromPNode(pNode->PNodeChild(2)); // atom
			}
			else
			{
				assert(false); // no viable alt
			}
		}
		else
		{
			assert(false); // no viable alt
		}

		return pStat;
	}

	BINOPK BinopkFromNodekTok(NODEK nodek)
	{
		switch (nodek)
		{
			case NODEK_PLUS:	return BINOPK_PLUS;
			case NODEK_SUB:		return BINOPK_SUB;
			case NODEK_LT:		return BINOPK_LT;
			default:			return BINOPK_Nil;
		}
	}

	vector<shared_ptr<SExpression>> m_aryPExpr;
	vector<shared_ptr<SStatement>> m_aryPStat;
};

int main()
{
	string strInput = "{ i=1; while ((i=i+10)<50) ; }";

	SParser parser;

	parser.SetInput(strInput);

	SParseNode * pNode = parser.PNodeProgNext();

	pNode->PrintDebug();

	SAst ast;

	SStatement * pStat = ast.PStatFromPNode(pNode);

	printf("\n\n\n");

	pStat->PrintDebug();

	return 0;
}