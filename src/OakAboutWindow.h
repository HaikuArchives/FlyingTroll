#ifndef _OAKABOUTWINDOW_H_
#define _OAKABOUTWINDOW_H_

#include <Window.h>

class OakAboutWindow : public BWindow
{
private:
public:
	OakAboutWindow();
	virtual void MessageReceived( BMessage *m );
};

#endif