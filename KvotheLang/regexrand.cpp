#include "regexrand.h"
#include <assert.h>
#include <limits>

bool FProb(float uProb, bool fAllowTrue)
{
	if(!fAllowTrue)
		return false;
	
	int nRand = rand();
	float uRand = float(nRand) / float(RAND_MAX);

	return uRand <= uProb;
}

template<typename T>
T TRand()
{
	int nRand = rand();
	nRand %= std::numeric_limits<T>::max();
	return T(nRand);
}

template<typename T>
T TRand(T tMin, T tMax)
{
	assert(tMax > tMin);
	int nRand = rand();
	nRand %= (tMax - tMin);
	return T(nRand) + tMin;
}

inline int max ( int a, int b ) { return a > b ? a : b; }
inline int min ( int a, int b ) { return a < b ? a : b; }

int ISegProb(vector<float> aryProb)
{
	int nRand = rand();
	float uRand = float(nRand) / float(RAND_MAX);

	float probSum = 0.0f;

	for(float prob : aryProb)
	{
		probSum += prob;
	}

	for(float & prob : aryProb)
	{
		prob /= probSum;
	}

	probSum = 0.0f;

	for(size_t iProb = 0; iProb < aryProb.size() - 1; ++iProb)
	{
		probSum += aryProb[iProb];
		
		if(uRand < probSum)
			return iProb;
	}

	return aryProb.size() - 1;
}

const int g_cRegexDataMax = 15;
const int g_nRegexDeapthMax = 200;
static const int s_cMaxQuant = 10;

int g_nRegexDeapth = 0;

#define FALLOWTRUE m_poolRegexData.m_arypv.size() < g_cRegexDataMax && g_nRegexDeapth < g_nRegexDeapthMax 

static const float s_uProbUnionContinue = 0.2f;
static const float s_uProbConcatContinue = 0.2f;
static const float s_uProbQuantContinue = 0.1f;
static const float s_uProbNegate = 0.5f;
static const float s_uProbSetContinue = 0.2f;
static const float s_uProbRange = 0.5f;

SRegexAstNode CRegexRandom::RegexRandom()
{
	g_nRegexDeapth++;
	SRegexAstNode unionAst = UnionRandom();

	g_nRegexDeapth--;
	return unionAst;
}

std::string CRegexRandom::StrRandFromRegex(SRegexAstNode regexAst)
{
	static const int s_nDeapthMax = 1000;

	static const int s_cLengthMax = 2000;

	if(m_nDepthCur > s_nDeapthMax)
	{
		m_cLengthCur = s_cLengthMax;
		return "!SOE!";
	}
	
	if(m_pRegexRoot == nullptr)
	{
		m_pRegexRoot = &regexAst;
	}

	std::string strRet = "";

	m_nDepthCur++;
	
	switch (regexAst.m_regexk)
	{
		case REGEXK_Union:
			{
				int iRegexRand = TRand<int>(0, regexAst.m_pUnionData->m_aryRegex.size());

				strRet = StrRandFromRegex(regexAst.m_pUnionData->m_aryRegex[iRegexRand]);
			}
			break;

		case REGEXK_Concat:
			{
				for(SRegexAstNode regex : regexAst.m_pConcatData->m_aryRegex)
				{
					strRet += StrRandFromRegex(regex);

					if(m_nDepthCur > s_nDeapthMax)
					{
						break;
					}
				}
			}
			break;

		case REGEXK_Quant:
			{
				SRegexAstNode regex = regexAst.m_pQuantData->m_regex;
			
				int cMic = regexAst.m_pQuantData->m_cMic;
				int cMac = regexAst.m_pQuantData->m_cMac;

				int cQuant = 0;

				while(cQuant < cMic)
				{
					cQuant++;
					strRet += StrRandFromRegex(regex);

					if(m_nDepthCur > s_nDeapthMax)
					{
						break;
					}
				}

				static const float s_uProbQuantStrContinue = 0.5;

				while((cMac == -1 || cQuant < cMac) && FProb(s_uProbQuantStrContinue, m_cLengthCur < s_cLengthMax))
				{
					cQuant++;
					strRet += StrRandFromRegex(regex);

					if(m_nDepthCur > s_nDeapthMax)
					{
						break;
					}
				}
			}
			break;

		case REGEXK_Range:
			{
				u8 chrRand = TRand<u8>(regexAst.m_pRangeData->m_chrMic, regexAst.m_pRangeData->m_chrMac);
				strRet = std::string(1, chrRand);
				m_cLengthCur++;
			}
			break;

		case REGEXK_Chr:
			strRet = std::string(1, regexAst.m_pChrData->m_chr);
			m_cLengthCur++;
			break;

		case REGEXK_Self:
			strRet = StrRandFromRegex(*m_pRegexRoot);
			break;

		default:
			assert(false);
			break;
	}

	if(m_nDepthCur <= s_nDeapthMax)
	{
		m_nDepthCur--;
	}
	
	if(m_pRegexRoot == &regexAst)
	{
		m_pRegexRoot = nullptr;
		m_cLengthCur = 0;
	}

	return strRet;
}

SRegexAstNode CRegexRandom::UnionRandom()
{
	g_nRegexDeapth++;
	SRegexAstNode regexConcat = ConcatRandom();
	
	if(FProb(s_uProbUnionContinue, FALLOWTRUE))
	{
		SRegexAstNode regexUnion = RegexCreate(REGEXK_Union);
		SUnionRegexData * pUnionData = regexUnion.m_pUnionData;
		pUnionData->m_aryRegex.push_back(regexConcat);
		
		do
		{
			pUnionData->m_aryRegex.push_back(ConcatRandom());
		} while(FProb(s_uProbUnionContinue, FALLOWTRUE));

		g_nRegexDeapth--;
		return regexUnion;
	}

	// otherwise, just return the concat we parsed

	g_nRegexDeapth--;
	return regexConcat;
}

SRegexAstNode CRegexRandom::ConcatRandom()
{
	g_nRegexDeapth++;
	SRegexAstNode regexQuant = QuantRandom();

	if(FProb(s_uProbConcatContinue, FALLOWTRUE))
	{
		SRegexAstNode regexConcat = RegexCreate(REGEXK_Concat);
		SConcatinationRegexData * pConcatData = regexConcat.m_pConcatData;
		pConcatData->m_aryRegex.push_back(regexQuant);
		
		do
		{
			pConcatData->m_aryRegex.push_back(QuantRandom());
		} while(FProb(s_uProbConcatContinue, FALLOWTRUE));

		g_nRegexDeapth--;
		return regexConcat;
	}

	// otherwise, just return the quant we parsed

	g_nRegexDeapth--;
	return regexQuant;
}

SRegexAstNode CRegexRandom::QuantRandom()
{
	g_nRegexDeapth++;
	SRegexAstNode regexCur = AtomRandom();
	
	while(FProb(s_uProbQuantContinue, FALLOWTRUE))
	{
		// while there is a chr that begins a quantification, 
		// make a new quantifier quantifieing the regex to the left
		
		SRegexAstNode regexQuant = RegexCreate(REGEXK_Quant);

		SQuantifierRegexData * pQuantData = regexQuant.m_pQuantData;
		pQuantData->m_regex = regexCur;

		regexCur = regexQuant;

		int iSeg = ISegProb({0.18f, 0.16f, 0.16f, 0.50f});
	
		if(iSeg == 0)
		{
			pQuantData->m_cMic = 0;
			pQuantData->m_cMac = -1;
		}	
		else if(iSeg == 1)
		{
			pQuantData->m_cMic = 1;
			pQuantData->m_cMac = -1;
		}
		else if(iSeg == 2)
		{
			pQuantData->m_cMic = 0;
			pQuantData->m_cMac = 1;
		}
		else
		{
			// {,} case

			// three valid versions
			// {N} exactly N times. N > 0
			// {N,} at least N times. N >= 0
			// {N1, N2} N1 to N2 times. N1 >= 0 and N2 > N1

			pQuantData->m_cMic = TRand<int>(0, s_cMaxQuant);

			int iSeg = ISegProb({0.20f, 0.40f, 0.40f});

			if(iSeg > 0)
			{
				if(iSeg == 2)
				{
					// {N1, N2} case

					int cOther = TRand<int>(0, s_cMaxQuant);

					pQuantData->m_cMac = max(pQuantData->m_cMic + 1, cOther);
					pQuantData->m_cMic = min(pQuantData->m_cMic, cOther);
					
					assert(pQuantData->m_cMac > pQuantData->m_cMic);
				}
				else
				{
					// {N,} case

					pQuantData->m_cMac = -1;

					assert(pQuantData->m_cMic >= 0);
				}
			}
			else
			{
				// {N} case

				pQuantData->m_cMic = max(pQuantData->m_cMic, 1);
				pQuantData->m_cMac = pQuantData->m_cMic;

				assert(pQuantData->m_cMic > 0);
			}
		}
	}

	// return the quantified regex (either the original atom, or nested quantifiers ending with the atom)

	g_nRegexDeapth--;
	return regexCur;
}

SRegexAstNode CRegexRandom::AtomRandom()
{
	g_nRegexDeapth++;
	int iSeg = ISegProb({0.5f, 0.2f, 0.01f, 0.26f, 0.03f});
	
	if(iSeg == 0 && FALLOWTRUE)
	{
		g_nRegexDeapth--;
		return RegexRandom();
	}
	else if(iSeg == 1 && FALLOWTRUE)
	{
		g_nRegexDeapth--;
		return SetRandom();
	}
	else if(iSeg == 2)
	{	
		SRegexAstNode regexRange = RegexCreate(REGEXK_Range);

		SRangeRegexData * pRangeData = regexRange.m_pRangeData;
		pRangeData->m_chrMic = 'a';
		pRangeData->m_chrMac = 'z';

		g_nRegexDeapth--;
		return regexRange;
	}
	else if(iSeg == 3)
	{	
		g_nRegexDeapth--;
		return RegexCreate(REGEXK_Self);
	}
	else
	{
		SRegexAstNode regexChr = RegexCreate(REGEXK_Chr);
		regexChr.m_pChrData->m_chr = TRand<u8>();

		g_nRegexDeapth--;
		return regexChr;
	}
}

SRegexAstNode CRegexRandom::SetRandom()
{
	g_nRegexDeapth++;

	// check if this set is a negation
	
	bool fNegate = false;

	if(FProb(s_uProbNegate, true))
	{
		fNegate = true;
	}

	// store elements of this set in a map from chr to bool (to convert to ranges later)
	
	bool mpChrFIncluded[256];

	for(int iChr = 0; iChr < 256; ++iChr)
	{
		mpChrFIncluded[iChr] = fNegate;
	}

	do
	{
		u8 chrBegin = TRand<u8>();
		u8 chrEnd = chrBegin;

		if(FProb(s_uProbRange, true))
		{
			u8 chrOther = TRand<u8>();

			chrEnd = max(chrBegin, chrOther);
			chrBegin = min(chrBegin, chrOther);
		}

		for(; chrBegin <= chrEnd; ++chrBegin)
		{
			mpChrFIncluded[chrBegin] = !fNegate;
		}

	} while(FProb(s_uProbSetContinue, true));

	// convert mpChrF to a union of ranges

	SRegexAstNode regexUnion = RegexCreate(REGEXK_Union);
	SUnionRegexData * pUnionData = regexUnion.m_pUnionData;
	
	for(int iChr = 0; iChr < 256; ++iChr)
	{
		if(mpChrFIncluded[iChr])
		{
			// save the current chr, which is the begining or a range

			u8 chrBegin = iChr;

			// advance to the end of the range
			
			while(mpChrFIncluded[iChr + 1] && iChr + 1 < 256)
			{
				++iChr;
			}
			
			// add either a signle chr or or range to the union
			
			if(chrBegin == iChr)
			{
				SRegexAstNode regexChr = RegexCreate(REGEXK_Chr);

				regexChr.m_pChrData->m_chr = chrBegin;

				pUnionData->m_aryRegex.push_back(regexChr);
			}
			else
			{
				SRegexAstNode regexRange = RegexCreate(REGEXK_Range);

				SRangeRegexData * pRangeData = regexRange.m_pRangeData;
				pRangeData->m_chrMic = chrBegin;
				pRangeData->m_chrMac = iChr;

				pUnionData->m_aryRegex.push_back(regexRange);
			}
		}
	}

	// a set has to have at least one element

	assert(pUnionData->m_aryRegex.size() > 0);

	g_nRegexDeapth--;
	
	if(pUnionData->m_aryRegex.size() == 1)
		return pUnionData->m_aryRegex[0];
	else
		return regexUnion;
}

SRegexAstNode CRegexRandom::RegexCreate(REGEXK regexk)
{
	SRegexAstNode regex;

	regex.m_regexk = regexk;

	switch (regexk)
	{
		case REGEXK_Union:
			regex.m_pUnionData = m_poolRegexData.PTNew<SUnionRegexData>();
			break;

		case REGEXK_Concat:
			regex.m_pConcatData = m_poolRegexData.PTNew<SConcatinationRegexData>();
			break;

		case REGEXK_Quant:
			regex.m_pQuantData = m_poolRegexData.PTNew<SQuantifierRegexData>();
			break;

		case REGEXK_Range:
			regex.m_pRangeData = m_poolRegexData.PTNew<SRangeRegexData>();
			break;

		case REGEXK_Chr:
			regex.m_pChrData = m_poolRegexData.PTNew<SChrRegexData>();
			break;

		case REGEXK_Self:
			break;

		default:
			assert(false);
			break;
	}

	return regex;
}