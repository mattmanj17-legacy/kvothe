using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

/*
	stat : list EOF | assign EOF
	assign : list '=' list
	list: '[' elements ']'
	elements: element (',' element)*
	element: NAME '=' NAME | NAME | list
*/

enum TOKK
{
    EOS,
    NAME,
    RBRACK,
    LBRACK,
    COMMA,
    EQUALS,
}

class CNestedNameListTokenizer
{
    public void SetInput(string strInput)
    {
        m_strInput = strInput;
        m_iChrCur = 0;
    }

    public TOKK TokkNext()
    {
        while (m_iChrCur < m_strInput.Length)
        {
            char chrCur = m_strInput[m_iChrCur];
            if(char.IsWhiteSpace(chrCur)) { ++m_iChrCur; } // ignore white space
            else if (chrCur == ',') { ++m_iChrCur; return TOKK.COMMA; }
            else if (chrCur == '[') { ++m_iChrCur; return TOKK.LBRACK; }
            else if (chrCur == ']') { ++m_iChrCur; return TOKK.RBRACK; }
            else if (chrCur == '=') { ++m_iChrCur; return TOKK.EQUALS; }
            else if (char.IsLetter(chrCur))
            {

                while (char.IsLetter(chrCur) && m_iChrCur < m_strInput.Length)
                {
                    ++m_iChrCur;
                    chrCur = m_strInput[m_iChrCur];
                }

                return TOKK.NAME;
            }
            else { throw new Exception(); }
        }
        return TOKK.EOS;
    }

    private string m_strInput;
    private int m_iChrCur;
}

class CNestedNameListParser
{
    void MatchTokk(TOKK tokk)
    {
        if (TokkLookAhead(0) != tokk) { throw new Exception(); }

        if(FIsSpeculating())
        {
            ++m_iBeginCur;
        }
        else
        {
            m_deqTokk.RemoveFront();
            ++m_cTokkConsumed;
        }
    }

    TOKK TokkLookAhead(int i)
    {
        i = Math.Max(i, 0);

        i += m_iBeginCur;

        while(m_deqTokk.Count < i + 1)
        {
            m_deqTokk.AddBack(m_tokenizer.TokkNext());
        }

        return m_deqTokk[i];
    }

    void BeginSpeculate()
    {
        m_stkIBegin.Push(m_iBeginCur);
    }

    void EndSpeculate()
    {
        m_iBeginCur = m_stkIBegin.Pop();
        if(!FIsSpeculating())
        {
            m_dictMemoAssign.Clear();
        }
    }

    public void Stat()
    {
        if (FSpeculateStatIsAssign())
        {
            Assign();
            MatchTokk(TOKK.EOS);
        }
        else if (TokkLookAhead(0) == TOKK.LBRACK)
        {
            List();
            MatchTokk(TOKK.EOS);
        }
        else
        {
            throw new Exception();
        }
    }

    bool FSpeculateStatIsAssign()
    {
        bool fSuccess = true;
        BeginSpeculate();
        try { Assign(); MatchTokk(TOKK.EOS); }
        catch { fSuccess = false; }
        EndSpeculate();
        return fSuccess;
    }

    bool FIsSpeculating()
    {
        return m_stkIBegin.Count != 0;
    }

    public void Assign()
    {
        bool fFailed = false;
        int iParseStart = m_iBeginCur + m_cTokkConsumed;

        if (FIsSpeculating() && m_dictMemoAssign.ContainsKey(iParseStart))
        {
            if(m_dictMemoAssign[iParseStart] >= 0)
            {
                m_iBeginCur = m_dictMemoAssign[iParseStart];
                return;
            }
            else
            {
                throw new Exception();
            }
        }

        try
        {
            List();
            MatchTokk(TOKK.EQUALS);
            List();
        }
        catch(Exception e)
        {
            fFailed = true;
            throw e;
        }
        finally
        {
            if(FIsSpeculating())
            {
                m_dictMemoAssign[iParseStart] = fFailed ? -1 : m_iBeginCur + m_cTokkConsumed;
            }
        }
    }

    public void List()
    {
        MatchTokk(TOKK.LBRACK);
        Elements();
        MatchTokk(TOKK.RBRACK);
    }

    public void Elements()
    {
        Element();

        while(TokkLookAhead(0) == TOKK.COMMA)
        {
            MatchTokk(TOKK.COMMA);
            Element();
        }
    }

    public void Element()
    {
        if (TokkLookAhead(0) == TOKK.NAME)
        {
            MatchTokk(TOKK.NAME);

            if (TokkLookAhead(0) == TOKK.EQUALS)
            {
                MatchTokk(TOKK.EQUALS);
                MatchTokk(TOKK.NAME);
            }
        }
        else if(TokkLookAhead(0) == TOKK.LBRACK)
        {
            List();
        }
        else
        {
            throw new Exception();
        }
    }

    public void SetInput(string strInput)
    {
        m_tokenizer.SetInput(strInput);
        m_deqTokk.Clear();
        m_stkIBegin.Clear();
        m_iBeginCur = 0;
    }

    Dictionary<int, int> m_dictMemoAssign = new Dictionary<int, int>();

    CNestedNameListTokenizer m_tokenizer = new CNestedNameListTokenizer();
    Deque<TOKK> m_deqTokk = new Deque<TOKK>();
    Stack<int> m_stkIBegin = new Stack<int>();
    int m_iBeginCur = 0;
    int m_cTokkConsumed = 0;
}

