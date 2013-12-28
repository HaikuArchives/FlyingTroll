#ifndef _FTVIEW_H_
#define _FTVIEW_H_

#include "OakView.h"
#include "OakButton.h"
#include "FTConstants.h"

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

class FTView : public BView
{
private:
	BView *panel;
	BTextControl *address;
	BTextControl *searchbar;
	BStringView *statusbar;
	OakView *oakview;
	BScrollView *scrollview;
	OakButton *button_search, *button_back, *button_forward, *button_home;
	OakButton *button_hback;
	bool show_panel, show_statusbar, show_scroll;
public:
	FTView( BRect rect, const char *name, uint32 rmode,
		bool pan = false, bool st = false,
		bool scroll = false );
	virtual void FrameResized( float w, float h );
	void SetOakTitle( const char * );
	void SetAddress( const char * );
	const char *Address();
	void SetStatus( const char *s, int num = 0 );
	void SetBars( const int vmax, const int hmax,
		bool init = false, int pos = 0 );
	void UpdateButtons();
	int GetScrollBarValue();
	void SetScrollBarValue( int newvalue );
	int GetScrollViewBottom();
	//BScrollBar *HScrollBar() { return hscrollbar; }
	status_t LoadFile( const char *s ) { return oakview->LoadFile(NULL, s); }
	status_t LoadFileMessage( BMessage *m ) { return oakview->LoadFile(m); }
	void Back( int to = 0 ) { oakview->Back(to); }
	void Forward() { oakview->Forward(); }
	void SelectAll() { oakview->SelectAll(); }
	void ToClipboard() { oakview->ToClipboard(); }
	void SetFontFamily( const char *s, bool prim = true ) {
		oakview->SetFontFamily(s, prim);
		oakview->Process();
		oakview->MyDraw(OAK_REDRAW);
	}
	void GetFontFamily( font_family *f ) {
		oakview->GetFontFamily(f);
	}
	virtual void AttachedToWindow();
	bool IsScroll() { return show_scroll; }
	bool IsPanel() { return show_panel; }
	bool IsStatus() { return show_statusbar; }
	virtual void MessageReceived( BMessage *m );
	void SetDefaultMenu(BPopUpMenu *m) { oakview->SetDefaultMenu(m); }
	void SetSelectMenu(BPopUpMenu *m) { oakview->SetSelectMenu(m); }
	void SetLinkMenu(BPopUpMenu *m) { oakview->SetLinkMenu(m); }
	void SetFBorders() { oakview->SetFBorders(); }
	void LoadHtml( const char *str ) {
		oakview->LoadHtml(str);
		oakview->MyDraw(OAK_REDRAW);
	}
	const char *Title() { return oakview->Title(); }
	uint32 Charset() { return oakview->Charset(); }
	void SetCharset( uint32 ch ) {oakview->SetCharset(ch); }
	void Reload() { oakview->Reload(); }
	void EnableSelect( bool dw = true ) { oakview->SetDoWords(dw); }
	void EnableLinks( bool dl = true ) { oakview->SetDoLinks(dl); }
	void DisableBuffer( bool buf = true )
	{
		oakview->SetDoBuffer(!buf);
		oakview->MyDraw(OAK_REDRAW);
	}
	void DisablePictures( bool pic = true )
	{
		oakview->SetDoPictures(!pic);
		oakview->Process();
		oakview->MyDraw(OAK_REDRAW);
	}
	const char *Url()
	{
		printf("address:%s\n", oakview->Address());
		BString *s = new BString(oakview->Address());
		printf("address2:%s\n", s->String());
		return s->String();
	}
};

#endif
