#include "OakBookmark.h"

OakBookmark::OakBookmark( const char *n, const char *p )
{
	name = new BString(n);
	path = new BString(p);
}

OakBookmark::~OakBookmark()
{
	delete name;
	delete path;
}

BString *OakBookmark::Name()
{
	return name;
}

BString *OakBookmark::Path()
{
	return path;
}