#ifndef _OAKSEARCH_H_
#define _OAKSEARCH_H_

#include <TextControl.h>
#include <View.h>
#include "OakView.h"

class OakSearch : public BTextControl
{
private:
	OakView *papa;
public:
	OakSearch( BRect r, class OakView *daddy );
	virtual void KeyUp( const char *bytes, int32 numbytes );
	virtual void MouseDown( BPoint p );
};

#endif