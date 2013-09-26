#ifndef _OAKBUTTON_H_
#define _OAKBUTTON_H_

#include <Button.h>
#include <Bitmap.h>
#include <PopUpMenu.h>

class OakButton : public BButton
{
private:
	BBitmap *on, *off, *dis;
	BPopUpMenu *menu;
	bool withmenu;
public:
	OakButton(BRect r, const char *name,
		BBitmap *b1, BBitmap *b2, BBitmap *b3, BMessage *message,
		bool wm = false);
	~OakButton();
	virtual void Draw( BRect ur );
	virtual void MouseDown( BPoint p );
	virtual void MouseUp( BPoint p );
	BPopUpMenu *Menu();
};

#endif