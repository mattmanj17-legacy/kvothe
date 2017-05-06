#pragma once

/* Copyright (C) 2017 Matthew Draper
|
| Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
| documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
| rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
| persons to whom the Software is furnished to do so, subject to the following conditions:
|
| The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
| Software.
|
| THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
| WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
| COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
| OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "string.h"
#include "ary.h"

// TOKen Kind

enum TOKK
{
	TOKK_NIL,
	TOKK_EOI,
	TOKK_INT,
	TOKK_ID,
	TOKK_PLUS,
	TOKK_SUB,
	TOKK_LT,
	TOKK_LPAREN,
	TOKK_RPAREN,
	TOKK_EQUALS,
	TOKK_LBRACE,
	TOKK_RBRACE,
	TOKK_DO,
	TOKK_WHILE,
	TOKK_IF,
	TOKK_ELSE,
	TOKK_SEMICOLON,
};

// RULE Kind

enum RULEK
{
	RULEK_NIL,
	RULEK_Atom,
	RULEK_Sum,
	RULEK_TestExpr,
	RULEK_ParenExpr,
	RULEK_Statement,
	RULEK_Program,
};

// a node in the parse tree

class CParseNode // tag = node
{
	friend class CParseTree;

public:

	void			AddChild(int iNode);

	RULEK			Rulek() const;
	TOKK			Tokk() const;

	CParseNode		NodeChild(int iChild) const;
	int				CChild() const;

	CString			Str() const;

private:
					CParseNode();

	RULEK			m_rulek;
	TOKK			m_tokk;
	CString			m_str;

	CDynAry<int>	m_daryINodeChild;
};

// the actual parse tree (mostly just a place to store all of the nodes)

class CParseTree // tag = parsetree
{
public:
	int	INodeCreate(RULEK rulek);
	int	INodeCreate(TOKK tokk);
	int	INodeCreate(TOKK tokk, CString str);

	CParseNode * PNodeFromINode(int iNode);

private:
	CDynAry<CParseNode> m_daryNode;
};

// tokenizer

class CTokenizer // tag = tokenizer
{
public:
					CTokenizer(CParseTree * pParsetree);

	int				INodeNext();

	void			SetInput(CString str);

private:
	int				INodeKeywordFromString(CString strToken);

	CString			m_strInput;
	int				m_iChrCur;

	CParseTree *	m_pParsetree;
};

// parser

class CParser // tag = parser
{
public:
	int		INodeParseProgram();
	int		INodeParseStatment();
	int		INodeParseParenExpr();
	int		INodeParseTestExpr();
	int		INodeParseSum();
	int		INodeParseAtom();

	void	SetInput(CString strInput);

private:

	TOKK	TokkCur();

	int		INodeMatch(TOKK tokk);
};

// BINary OPeration Kind

enum BINOPK
{
	BINOPK_NIL,
	BINOPK_LT,
	BINOPK_PLUS,
	BINOPK_SUB
};

// EXPRession Kind

enum EXPRK
{
	EXPRK_Nil,
	EXPRK_Binop,
	EXPRK_Int,
	EXPRK_Id
};

// AST expression

struct SExpression // tag = expr
{
	EXPRK m_exprk;

	union
	{
		// EXPRK_Binop

		struct
		{
			BINOPK m_binopk;
			int m_iExprLeft;
			int m_iExprRight;
		};

		// EXPRK_Int

		int m_n;

		// EXPRK_Id

		int m_iGlobal;
	};
};

// STATement Kind

enum STATK
{
	STATK_Nil,
	STATK_Statements,
	STATK_If,
	STATK_While,
	STATK_DoWhile,
	STATK_Assign,
};

// AST Statement

struct SStatement // tag = stat
{
	STATK				m_statk;

	union
	{
		// STATK_Statements
		
		CDynAry<int>	m_aryIStat;

		// STATK_If
		
		struct 
		{
			int			m_iExprCondition;
			int			m_iStatBody;
			bool		m_fHasElse;
			int			m_iStatElse;
		};

		// STATK_While / STATK_DoWhile

		struct
		{
			int			m_iExprCondition;
			int			m_iStatBody;
		};

		// STATK_Assign

		struct
		{
			int			m_iExprId; // must be EXPRK_Id
			int			m_iExprValue;
		};
	};
};

// The Ast

class CAst // tag = ast
{
public:

	int						IStatCreate(const CParseNode & pnode);
	int						IExprCreate(const CParseNode & pnode);

private:

	BINOPK					BinopkFromTokk(TOKK tokk);

	CDynAry<SExpression>	m_daryPExpr;
	CDynAry<SStatement>		m_daryPStat;
};

// virtual machine OPCODE

enum OPCODE : int
{
	OPCODE_IFETCH,
	OPCODE_ISTORE,
	OPCODE_IPUSH,
	OPCODE_IPOP,
	OPCODE_IADD,
	OPCODE_ISUB,
	OPCODE_ILT,
	OPCODE_JZ,
	OPCODE_JNZ,
	OPCODE_JMP,
	OPCODE_HALT
};