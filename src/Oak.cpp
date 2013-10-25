#include "Oak.h"

#include <stdio.h>

OakApp::OakApp() : BApplication("application/x-vnd.Be-FlyingTroll")
{
	BRect r;
	BScreen scr;
	r = scr.Frame();
	r.InsetBy(100, 100);
	theWindow = new OakWindow(r);
	//if (s != NULL)
	//	theWindow->LoadFile(s);
	param = false;
}

void OakApp::ArgvReceived( int32 argc, char **argv )
{
	if (argc > 1)
	{
		s = argv[1];
		//printf("blya...\n");
		theWindow->LockLooper();
		theWindow->LoadFile(argv[1]);
		theWindow->UnlockLooper();
	}
	param = true;
}

void OakApp::AppActivated( bool active )
{
}

void OakApp::ReadyToRun()
{
	if (!param)
	{
		theWindow->LockLooper();
		theWindow->LoadFile("about:home");
		theWindow->UnlockLooper();
	}
}

void OakApp::RefsReceived( BMessage *m )
{
	//printf("REF RECEIVED\n");

	theWindow->LockLooper();
	theWindow->LoadFileMessage(m);
	theWindow->UnlockLooper();
	param = true;
}

int main( void )
{
	BDirectory dir;
	
	mkdir("/boot/home/config/settings/FlyingTroll", 0777);
	mkdir("/boot/home/config/settings/FlyingTroll/Bookmarks", 0777);
	
	OakApp *app = new OakApp();
	app->Run();
	delete app;
	return 0;
}

/*void OakApp::RefsReceived( BMessage *message ) 
{ 
    entry_ref ref;
    
    if ( message->FindRef("refs", &ref) == B_OK )
    {
        BMessage aMessage(B_SIMPLE_DATA);
        aMessage.AddRef("refs", &ref);
		aMessage.PrintToStream();
        dropWindow->Lock();
        BView *view = dropWindow->ChildAt(0);
        dropWindow->Unlock();
       	dropWindow->PostMessage(&aMessage, view);
    }
}*/