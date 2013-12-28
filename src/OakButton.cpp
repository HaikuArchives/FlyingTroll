#include "OakButton.h"

#include <stdio.h>
#include <MenuItem.h>
#include <Looper.h>
#include <Message.h>

OakButton::OakButton( BRect r, const char *name,
	BBitmap *b1, BBitmap *b2, BBitmap *b3, BMessage *message,
	bool wm ) :
	BButton(r, name, "", message, B_FOLLOW_RIGHT | B_FOLLOW_TOP)
{
	off = b1;
	on = b2;
	dis = b3;
	
	withmenu = wm;
	if (withmenu)
	{
		menu = new BPopUpMenu(name);
	}
}

OakButton::~OakButton()
{
	if (off != NULL)
		delete off;
	if (on != NULL)
		delete on;
	if (dis != NULL)
		delete dis;
		
	if (withmenu)
	{
		delete menu;
	}
}

void OakButton::Draw( BRect ur )
{
	SetDrawingMode(B_OP_COPY);
	SetHighColor(216, 216, 216);
	//static BRect r;
	//r = Bounds();
	//r.right--;
	FillRect(Bounds());
	SetDrawingMode(B_OP_ALPHA);
	//MoveTo(Frame().LeftTop());
	if (Value() && on != NULL)
		DrawBitmap(on);
	else if (!IsEnabled() && dis != NULL)
		DrawBitmap(dis);
	else if (off != NULL)
		DrawBitmap(off);
	//printf("> Button Pressed? %d\n", (int)Value());
}

BPopUpMenu *OakButton::Menu()
{
	return menu;
}

void OakButton::MouseDown( BPoint p )
{
	if (withmenu && IsEnabled())
	{
		BMenuItem *selected;
		//BMessage *m;
	
		ConvertToScreen(&p);
		selected = menu->Go(p);
		if (selected)
		{
			BLooper *looper;
			BHandler *targ = selected->Target(&looper);
			looper->PostMessage(selected->Message(), targ);
		}
	}
	BButton::MouseDown(p);
}

void OakButton::MouseUp( BPoint p )
{
	BButton::MouseDown(p);
}
