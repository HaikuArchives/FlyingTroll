#include "OakVBar.h"
#include "OakView.h"

OakVBar::OakVBar( BRect r ) :
	BScrollBar(r, "vbar", NULL, 0, 0, B_VERTICAL )
{
}

void OakVBar::ValueChanged( float newvalue )
{
	//SetRange(0, 50);

	//oakview->SetVPos(newvalue);
}