#include "OakAnchor.h"

#include <stdio.h>

OakAnchor::OakAnchor( const char *n, int p )
{
	name = new BString(n);
	pos = p;
	//printf("OakAnchor:: Added anchor ('%s')\n", n);
}

OakAnchor::~OakAnchor()
{
	delete name;
}

int OakAnchor::Pos()
{
	return pos;
}

BString *OakAnchor::Name()
{
	return name;
}