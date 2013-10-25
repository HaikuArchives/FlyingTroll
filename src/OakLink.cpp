#include "OakLink.h"
#include "OakView.h"

#include <Message.h>

OakLink::OakLink( BRect r, const char *hr )
{
	href = new BString(hr);
	rect = r;
	//printf("OakLink:: Added link to '%s'\n", hr);
}

OakLink::~OakLink()
{
	delete href;
}

BRect OakLink::Bounds()
{
	return rect;
}

const char *OakLink::Href()
{
	return href->String();
}