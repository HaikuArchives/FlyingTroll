#ifndef _OAKLINK_H_
#define _OAKLINK_H_

#include <String.h>
#include <Rect.h>

class OakLink
{
private:
	BString *href;
	BRect rect;
public:
	OakLink( BRect r, const char *hr );
	~OakLink();
	BRect Bounds();
	const char *Href();
};

#endif