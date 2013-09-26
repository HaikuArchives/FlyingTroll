#include "OakWord.h"

OakWord::OakWord( BRect r, const char *w )
{
	static uint32 sw;
	
	sw = strlen(w);
	word = (char *)malloc(sw + 1);
	memcpy(word, w, sw);
	word[sw] = 0;
	rect = r;
}

OakWord::~OakWord()
{
	free(word);
}

BRect OakWord::Bounds()
{
	return rect;
}

const char *OakWord::Word()
{
	return word;
}