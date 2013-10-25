#ifndef _OAKBOOKMARK_H_
#define _OAKBOOKMARK_H_

#include <String.h>

class OakBookmark
{
private:
	BString *name;
	BString *path;
public:
	OakBookmark( const char *n, const char *p );
	~OakBookmark();
	BString *Name();
	BString *Path();
};

#endif