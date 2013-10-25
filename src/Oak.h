#ifndef _OAK_H_
#define _OAK_H_

#include <Application.h>
#include "OakWindow.h"

class OakApp : public BApplication
{
private:
	OakWindow *theWindow;
	char *s;
	bool param;
public:
	OakApp();
	virtual void ArgvReceived( int32, char ** );
	virtual void RefsReceived( BMessage * );
	virtual void AppActivated( bool active );
	virtual void ReadyToRun();
};

#endif