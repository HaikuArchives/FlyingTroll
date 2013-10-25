#ifndef _OAKWORD_H_
#define _OAKWORD_H_

#include <String.h>
#include <Rect.h>
#include <stdlib.h>
#include <string.h>

class OakWord
{
private:
	char *word;
	BRect rect;
public:
	OakWord( BRect r, const char *w );
	~OakWord();
	BRect Bounds();
	const char *Word();
};

#endif