#include "OakDemoWindow.h"

OakDemoWindow::OakDemoWindow() :
	BWindow(BRect(200, 200, 800, 400), "demo",
		B_FLOATING_WINDOW, B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE |
			B_AVOID_FRONT)
{
	BRect r;
	
	r = Bounds();
	r.bottom = r.top + 200;
	//r2 = r;
	//r2.InsetBy(6, 6);
	btv = new BTextControl(r, "btv", 0, "", NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW);
	//btv->SetViewColor(246, 246, 246);
	BFont font;
	//btv->SetStylable(true);
	btv->GetFont(&font);
	font.SetSize(16);
	btv->SetFont(&font);
	AddChild(btv);
	
	r = Bounds();
	r.top = btv->Frame().bottom + 1;
	ftv = new FTView(r, "about", B_FOLLOW_ALL_SIDES,
		false, false, false);
	AddChild(ftv);
	//ftv->SetFBorders();
	
	//ftv->LoadFile("about:blank");
	btv->SetText("<CENTER><u><font size=+4>FTView</font></u> Class "
		"<font color=red>Demo</font>. Type some tags...</CENTER>");
	btv->SetText("<TABLE border><TD width=200>KO</TD><TD width=200>KO</TD><TD>KO</TD></TABLE>");
	ftv->LoadHtml(btv->Text());
	
	btv->SetModificationMessage(new BMessage(DEMO_EDITED));
	
	SetSizeLimits(400, 40000, 150, 40000);
	Show();
}

void OakDemoWindow::MessageReceived( BMessage *m )
{
	switch (m->what)
	{
	case DEMO_EDITED:
		ftv->LoadHtml(btv->Text());
	}
}