#include "regexparse.h"

void SUnionData::PrintDebug()
{
	printf("(UNION");

	for(SRegex * pRegex : m_arypRegex)
	{
		printf(" ");
		pRegex->PrintDebug();
	}

	printf(")");
}