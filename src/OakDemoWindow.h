#ifndef _OAKDEMOWINDOW_H_
#define _OAKDEMOWINDOW_H_

#include "FTView.h"

#include <Window.h>
#include <TextControl.h>

class OakDemoWindow : public BWindow
{
private:
	FTView *ftv;
	BTextControl *btv;
public:
	OakDemoWindow();
	virtual void MessageReceived( BMessage *m );
};

#endif