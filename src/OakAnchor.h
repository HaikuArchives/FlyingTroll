#ifndef _OAKANCHOR_H_
#define _OAKANCHOR_H_

#include <String.h>

class OakAnchor
{
private:
	int pos;
	BString *name;
public:
	OakAnchor( const char *n, int p );
	~OakAnchor();
	int Pos();
	BString *Name();
};

#endif