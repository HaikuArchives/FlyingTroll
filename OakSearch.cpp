#include "OakSearch.h"

OakSearch::OakSearch( BRect r, class OakView *daddy ) :
	 BTextControl(r, "searchbar",
		"search:", "", new BMessage(SEARCH_CHANGE),
		B_FOLLOW_RIGHT, B_WILL_DRAW)
{
	papa = daddy;
}

void OakSearch::KeyUp( const char *bytes, int32 numbytes )
{
	printf("Ko-Ko\n");
	papa->Search(Text());
}

void OakSearch::MouseDown( BPoint p )
{
	printf("Ko-Ko\n");
	papa->Search(Text());
}