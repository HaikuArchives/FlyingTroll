#ifndef _OAKWINDOW_H_
#define _OAKWINDOW_H_

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <TextControl.h>
#include <ScrollBar.h>
#include <StatusBar.h>
#include <TextView.h>
#include <Screen.h>
#include <Message.h>
#include <FilePanel.h>
#include <TranslationUtils.h>
#include <ScrollView.h>
#include <Directory.h>
#include <StringView.h>
#include <PopUpMenu.h>
#include <Alert.h>
#include <TabView.h>

#include "FTView.h"
#include "OakTabView.h"
#include "OakBookmark.h"
#include "Constants.h"

class OakWindow : public BWindow
{
private:
	FTView *ftview;
	OakTabView *btv;
	BMenuBar *menubar;
	BFilePanel *fpanel;
	BMenu *menuBookmarks, *menuView, *menuViewFonts, *menuDebug;
	bool fullscreen;
	BRect oldframe;
	BAlert *alert;
public:
	OakWindow( BRect r );
	virtual bool QuitRequested();
	void UpdateBookmarks();
	virtual void MessageReceived( BMessage *message );
	void ScanFontsToMenu();
	void LoadFile( char *s ) { ftview->LoadFile(s); }
	void LoadFileMessage( BMessage *m ) { ftview->LoadFileMessage(m); }
	virtual void FrameResized( float w, float h );
};

#endif