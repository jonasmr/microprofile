#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//minimal tool to embed src html into microprofile.h

char* ReadFile(const char* pFile)
{
	FILE* F = fopen(pFile, "r");

	if(!F) return 0;
	fseek(F, 0, SEEK_END);
	long size = ftell(F);
	char* pData = (char*)malloc(size + 1);
	fseek(F, 0, SEEK_SET);	
	size = fread(pData, 1, size, F);
	fclose(F);

	pData[size] = '\0';
	return pData;
}
void DumpFile(FILE* pOut, const char* pTest)
{
	size_t len = strlen(pTest);
	for(size_t i = 0; i < len; ++i)
	{
		char c = pTest[i];
		switch(c)
		{
			case '\n':
				fprintf(pOut, "\\n\"\n\""); 
				break;
			case '\\':
				fprintf(pOut, "\\"); 
				break;
			case '\"':
				fprintf(pOut, "\\\""); 
				break;
			case '\'':
				fprintf(pOut, "\\\'"); 
				break;
			default:
				fprintf(pOut, "%c", c);
		}
	}
}


int main(int argc, char* argv[])
{
	if(7 != argc)
	{
		printf("Syntax: %s dest source embedsource pattern symbol define\n", argv[0]);
		return 1;
	}
	const char* pDestArg = argv[1];
	const char* pSourceArg = argv[2];
	const char* pEmbedSourceArg = argv[3];
	const char* pPatternArg = argv[4];
	const char* pSymbolArg = argv[5];
	const char* pDefineArg = argv[6];

	char* pSrc = ReadFile(pSourceArg);
	char* pEmbedSrc = ReadFile(pEmbedSourceArg);
	
	if(!pSrc || !pEmbedSrc)
	{
		return 1;
	}

	char* pEmbedStart = pEmbedSrc;
	char* pEmbedStartEnd = strstr(pEmbedStart, pPatternArg);
	char* pEmbedEnd = pEmbedStartEnd + strlen(pPatternArg);
	*pEmbedStartEnd = '\0';

	FILE* pOut = fopen(pDestArg, "w");
	if(!pOut)
	{
		free(pSrc);
		free(pEmbedSrc);
		return 1;		
	}
	fwrite(pSrc, strlen(pSrc), 1, pOut);
	fprintf(pOut, "\n\n///start embedded file from %s\n", pEmbedSourceArg);
	fprintf(pOut, "#ifdef %s\n", pDefineArg);
	fprintf(pOut, "const char %s_begin[] =\n\"", pSymbolArg);
	DumpFile(pOut, pEmbedStart);
	fprintf(pOut, "\";\n\n");
	fprintf(pOut, "const size_t %s_begin_size = sizeof(%s_begin);\n", pSymbolArg, pSymbolArg);
	fprintf(pOut, "const char %s_end[] =\n\"", pSymbolArg);
	DumpFile(pOut, pEmbedEnd);
	fprintf(pOut, "\";\n\n");
	fprintf(pOut, "const size_t %s_end_size = sizeof(%s_end);\n", pSymbolArg, pSymbolArg);

	fprintf(pOut, "#endif //%s\n", pDefineArg);
	fprintf(pOut, "\n\n///end embedded file from %s\n", pEmbedSourceArg);

	fclose(pOut);
	free(pSrc);
	free(pEmbedSrc);

	return 0;
}