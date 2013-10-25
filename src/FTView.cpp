#include "FTView.h"

#include <Box.h>

FTView::FTView( BRect r, const char *name, uint32 rmode,
	bool pan = false, bool st = false,
	bool scroll = false ) :
	BView(r, name, rmode, B_WILL_DRAW | B_FRAME_EVENTS)
{
	show_panel = pan;
	show_statusbar = st;
	show_scroll = scroll;
	
	BRect r;
	
	if (show_statusbar)
	{
		r = Bounds();
		r.top = r.bottom - 13;
		r.bottom += 2;
		r.right++;
		r.right -= 13;
		r.left += 200;
		
		statusbar = new BStringView(r, "ftstbar", "",
			B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
		BFont f;
		f = be_plain_font;
		f.SetSize(9);
		statusbar->SetFont(&f);
		AddChild(statusbar);

		//BBox *bbox = new BBox(r, "bbox", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
		//bbox->SetViewColor(216, 216, 216);
		//AddChild(bbox);

	}
	
	if (show_panel)
	{
		r = Bounds();
		r.top = 0;
		r.bottom = 20;
		panel = new BView(r, "myPanel", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
		panel->SetViewColor(216, 216, 216);
		
		r = panel->Bounds();
		r.right -= 32 * 4 + 9 + 102;
		r.top += 2;
		r.left += 2;
		r.right -= 2;
		r.bottom -= 2;
		address = new BTextControl(r, "address",
			0, 0, NULL, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
		panel->AddChild(address);
		
		panel->ResizeTo(panel->Bounds().right, address->Bounds().bottom + 3);
	
		r.top += 1;
		r.bottom -= 1;
	
	
		r.right += 32;
		r.left = r.right - 30;
		button_back = new OakButton(r, "back",
			BTranslationUtils::GetBitmapFile("images/button_back"),
			BTranslationUtils::GetBitmapFile("images/button_back_on"),
			BTranslationUtils::GetBitmapFile("images/button_back_dis"),
			new BMessage(HISTORY_BACK));
		button_back->SetEnabled(false);
		panel->AddChild(button_back);
	
		r.right += 10;
		r.left = r.right - 9;
		button_hback = new OakButton(r, "hback",
			BTranslationUtils::GetBitmapFile("images/button_down"),
			BTranslationUtils::GetBitmapFile("images/button_down_on"),
			BTranslationUtils::GetBitmapFile("images/button_down_dis"),
			NULL, true);
		button_hback->SetEnabled(false);
		panel->AddChild(button_hback);
	
		r.right += 32;
		r.left = r.right - 30;
		button_forward = new OakButton(r, "forward",
			BTranslationUtils::GetBitmapFile("images/button_forward"),
			BTranslationUtils::GetBitmapFile("images/button_forward_on"),
			BTranslationUtils::GetBitmapFile("images/button_forward_dis"),
			new BMessage(HISTORY_FORWARD));
		button_forward->SetEnabled(false);
		panel->AddChild(button_forward);
	
		r.right += 32;
		r.left = r.right - 30;
		button_home = new OakButton(r, "home",
			BTranslationUtils::GetBitmapFile("images/button_home"),
			BTranslationUtils::GetBitmapFile("images/button_home_on"),
			BTranslationUtils::GetBitmapFile("images/button_home_dis"),
			new BMessage(GO_HOME));
		panel->AddChild(button_home);
	
		r.right += 102;
		r.left = r.right - 100;
		searchbar = new BTextControl(r, "searchbar", 0, "", NULL, B_FOLLOW_RIGHT, B_WILL_DRAW);
		searchbar->SetModificationMessage(new BMessage(SEARCH_CHANGE));
		searchbar->SetHighColor(116, 116, 176);
		panel->AddChild(searchbar);

		r.right += 32;
		r.left = r.right - 30;
		button_search = new OakButton(r, "search",
			BTranslationUtils::GetBitmapFile("images/button_search"),
			BTranslationUtils::GetBitmapFile("images/button_search_on"),
			BTranslationUtils::GetBitmapFile("images/button_search_dis"),
			new BMessage(SEARCH_AGAIN));
		button_search->SetEnabled(false);
		panel->AddChild(button_search);
	
		AddChild(panel);
	}

	r = Bounds();
	if (show_panel)
		r.top = panel->Frame().bottom + 1;
//	if (show_statusbar)
//		r.bottom = statusbar->Frame().top;
	if (show_scroll)
		r.right -= B_V_SCROLL_BAR_WIDTH;
	if (show_scroll || show_statusbar)
		r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	oakview = new OakView(r, "oakview", this);

	// ScrollView
	
	if (show_scroll)
	{
		scrollview = new BScrollView("scroll", oakview, B_FOLLOW_ALL, 0, true,
			true, B_NO_BORDER);
		if (show_statusbar)
		{
			scrollview->ScrollBar(B_HORIZONTAL)->ResizeTo(200, 14);
			scrollview->ScrollBar(B_HORIZONTAL)->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		}
		AddChild(scrollview);
		
		scrollview->ScrollBar(B_VERTICAL)->SetProportion(0.3);
		scrollview->ScrollBar(B_VERTICAL)->SetRange(0, 0);	
	
		scrollview->ScrollBar(B_HORIZONTAL)->SetProportion(0.3);
		scrollview->ScrollBar(B_HORIZONTAL)->SetRange(0, 0);	
	}
	else
	{
		scrollview = new BScrollView("scroll", oakview, B_FOLLOW_ALL, 0, false,
			false, B_NO_BORDER);
		AddChild(scrollview);
	}
	
	if (show_statusbar)
	{
		statusbar->SetHighColor(116, 116, 116);
		statusbar->SetViewColor(216, 216, 216);
	}

	UpdateButtons();
}

void FTView::AttachedToWindow()
{
	LoadFile("about:blank");
}

void FTView::FrameResized( float w, float h )
{
	printf("(%d:%d)", (int)w, (int)h);
	//SetOakTitle(s);
	
	oakview->MyDraw(OAK_REDRAW);
	oakview->MyDraw(OAK_DRAW);
	SetBars(oakview->VMax(), oakview->HMax(), false, 0);
}

void FTView::SetOakTitle( const char *s )
{
	BString tl;
	tl.SetTo(s);
	tl.Prepend("");
	tl.Append("");
	Window()->SetTitle(tl.String());
}

void FTView::SetAddress( const char* s )
{
	if (show_panel)
		address->SetText(s);
}

void FTView::SetStatus( const char* s, int num = 0 )
{
	if (show_statusbar && num == 0)
	{
		BString ss(s);
		ss.Prepend(" ");
		statusbar->SetText(ss.String());
	}
}

const char *FTView::Address( )
{
	if (show_panel)
		return address->Text();
	else
		return "";
}

void FTView::SetBars( const int vmax, const int hmax, bool init = false, int pos = 0 )
{
	int k, k2;
	float pr1, pr2;
	
	if (show_scroll)
	{
		//printf("Trying to set bars: k=%d, vmax=%d\n", k, vmax);
	
		k = (int)Bounds().bottom - (int)B_H_SCROLL_BAR_HEIGHT + 1;
		k2 = (int)Bounds().right - (int)B_V_SCROLL_BAR_WIDTH + 1;
		if (show_panel)
			k -= (int)panel->Frame().bottom;
	
		pr1 = (1000.0 - vmax + k) / 1000.0;
		if (pr1 < 0)
			pr1 = 0.1;
		if (pr1 > 1)
			pr1 = 1;
		pr2 = (1000.0 - hmax + k2) / 1000.0;
		if (pr2 < 0)
			pr2 = 0.1;
		if (pr2 > 1)
			pr2 = 1;
		
		scrollview->ScrollBar(B_VERTICAL)->SetRange(0, (vmax - k > 0 ? vmax - k : 0));
		scrollview->ScrollBar(B_VERTICAL)->SetProportion(pr1);
		scrollview->ScrollBar(B_HORIZONTAL)->SetRange(0, (hmax - k2 > 0 ? hmax - k2 : 0));
		scrollview->ScrollBar(B_HORIZONTAL)->SetProportion(pr2);
		if (init)
		{
			scrollview->ScrollBar(B_VERTICAL)->SetValue(pos);
			scrollview->ScrollBar(B_HORIZONTAL)->SetValue(0);
		}
			
		//printf("SetBars: k=%d, vmax=%d\n", k, vmax);
	}
}

void FTView::UpdateButtons()
{
	
	if (show_panel)
	{
		int i;
		BMenu *m;
		m = button_hback->Menu();
		
		m->RemoveItems(0, m->CountItems(), true);
		if (oakview->CountHistory() - oakview->HistoryPos() > 1)
		{
			button_back->SetEnabled(true);
	
			/*BMenuItem *mi;
			mi = new BMenuItem("Back", new BMessage(HISTORY_BACK));
			mi->SetTarget(BMessenger(this));
			m->AddItem(mi);*/
	
			//m->AddSeparatorItem();
			
			//int j = 0;
			//char ss[10];
			BMessage *mes;
			int q = oakview->CountHistory() - 2 - oakview->HistoryPos();
			int q2 = oakview->CountHistory() - 1;
			for (i = q; i >= 0; i--)
			{
				BMenuItem *mi;
				BString s = oakview->History(i)->Title()->String();
				if (s.Compare(" ", 1) == 0)
					s.RemoveFirst(" ");
				if (s.Length() > 40)
				{
					s.Truncate(40);
					s.Append("...");
				}
				
				//sprintf(ss, "%d. ", j);
				//s.Prepend(ss);
				mes = new BMessage(HISTORY_BACK_MENU);
				mes->AddInt32("back_to", q2 - i);
				mi = new BMenuItem(s.String(), mes);
				mi->SetTarget(BMessenger(this));
				m->AddItem(mi);
				//j++;
			}
			if (q >= 0)
				button_hback->SetEnabled(true);
			else
				button_hback->SetEnabled(false);
		}
		else
		{
			button_back->SetEnabled(false);
			button_hback->SetEnabled(false);
			for (i = 0; i <= m->CountItems(); i++)
				delete m->RemoveItem(i);
		}
		if (oakview->HistoryPos() > 0)
			button_forward->SetEnabled(true);
		else
			button_forward->SetEnabled(false);
	}
}

int FTView::GetScrollBarValue()
{
	if (show_scroll)
		return (int)scrollview->ScrollBar(B_VERTICAL)->Value();
	else
		return 0;
}

void FTView::SetScrollBarValue( int newvalue )
{
	if (show_scroll)
		scrollview->ScrollBar(B_VERTICAL)->SetValue(newvalue);
}

int FTView::GetScrollViewBottom()
{
	if (show_scroll)
		return (int)scrollview->Bounds().bottom;
	else
		return (int)Bounds().bottom;
}

void FTView::MessageReceived( BMessage *message )
{
	switch (message->what)
	{
	case HISTORY_BACK:
		Back();
		break;
	case HISTORY_BACK_MENU:
		int32 to;
		message->FindInt32("back_to", &to);
		Back(to);
		break;
	case HISTORY_FORWARD:
		Forward();
		break;
	case GO_HOME:
		LoadFile("about:home");
		break;
	case EDIT_SELECTALL:
		SelectAll();
		break;
	case EDIT_COPY:
		ToClipboard();
		break;
	case SEARCH_CHANGE:
		if (show_panel)
		{
			oakview->Search(searchbar->Text());
			if (searchbar->Text() != "")
				button_search->SetEnabled(true);
			else
				button_search->SetEnabled(false);
		}
		break;
	case SEARCH_AGAIN:
		if (show_panel)
		{
			oakview->Search(searchbar->Text(), true);
		}
		break;
	case EDIT_SEARCH:
		if (show_panel)
			searchbar->MakeFocus();
		break;
	case EDIT_SEARCHNEXT:
		if (show_panel)
			oakview->Search(searchbar->Text(), true);
		break;
	case B_MOUSE_WHEEL_CHANGED:
		if (show_scroll)
		{
			static float y;
			message->FindFloat("be:wheel_delta_y", &y);
			static int newvalue;
			newvalue = (int)scrollview->ScrollBar(B_VERTICAL)->Value() + (int)y * 50;
			if (newvalue < 0)
				newvalue = 0;
			else if (newvalue > oakview->VMax())
				newvalue = oakview->VMax();
			scrollview->ScrollBar(B_VERTICAL)->SetValue(newvalue);
		}
		break;
	default:
		oakview->MessageReceived(message);
		break;
	}
}