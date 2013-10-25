#include "OakHistory.h"

OakHistory::OakHistory( )
{
	path = new BString();
	title = new BString();
}

OakHistory::~OakHistory()
{
	delete path;
	delete title;
}

OakHistory &OakHistory::operator=( OakHistory &h )
{
	path->SetTo(h.Path()->String());
	title->SetTo(h.Title()->String());
	
	return h;
}