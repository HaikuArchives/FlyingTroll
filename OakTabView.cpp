#include "OakTabView.h"

OakTabView::OakTabView( BRect r ) : BTabView( r, "oaktabview" )
{
	
}

void OakTabView::Draw( BRect ur )
{
	DrawTabs();
}