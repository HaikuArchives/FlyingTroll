#include "OakAboutWindow.h"
#include "FTView.h"

OakAboutWindow::OakAboutWindow() :
	BWindow(BRect(200, 200, 600, 450), "",
		B_MODAL_WINDOW, B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE |
			B_NOT_RESIZABLE | B_AVOID_FRONT)
{
	BRect r;
	r = Bounds();
	
	BView *v = new BView(r, "v", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	v->SetViewColor(55, 102, 152);
	AddChild(v);
	
	r = Bounds();
	r.bottom -= 50;
	FTView *ftv = new FTView(r, "about", B_FOLLOW_ALL_SIDES, false, false, false);
	v->AddChild(ftv);
	//ftv->SetFBorders();
	
	r.bottom += 40;
	r.top = r.bottom - 30;
	r.left += 130;
	r.right -= 130;
	BButton *bok = new BButton(r, "bok", "Ok, close me", new BMessage(ABOUT_OK));
	//bok->SetLowColor(55, 102, 152);
	v->AddChild(bok);
	ftv->SetFontFamily("Dutch", true);
	
	ftv->LoadFile("about:about");
	
	Show();
}

void OakAboutWindow::MessageReceived( BMessage *m )
{
	switch (m->what)
	{
	case ABOUT_OK:
		printf("CLOSE\n");
		Close();
	}
}