#include "OakWindow.h"
#include "OakVBar.h"
#include "OakAboutWindow.h"
#include "OakDemoWindow.h"

#include <fs_attr.h>
#include <Looper.h>
#include <Alert.h>
#include <UTF8.h>
#include <Node.h>
#include <NodeInfo.h>

status_t WriteAttributes(BFile& file,
	const BMessage& attrData)
{
	status_t status;

	// Iterate through all of the bits of data in the message.
	// For each component, get its name, type, data, and size, and
	// create a corresponding attribute in the file.
	for (int i = 0; i < attrData.CountNames(B_ANY_TYPE); i++) {
		char *name;
		type_code type;
		if ((status = attrData.GetInfo(B_ANY_TYPE, i, &name, &type)))
			return status;
		const void *data;
		ssize_t size;
		if ((status = attrData.FindData(name, type, &data, &size)))
			return status;
		if (file.WriteAttr(name, type, 0, data, size) != size)
			return status;
	}
	return B_NO_ERROR;
}

status_t ReadAttributes(const char *path,
	BMessage &attrData)
{
	status_t status;
	void *data = 0;
	
	
	BNode node(path);

	if ((status = node.InitCheck()))
		return status;
		
	// Iterate through all of the attributes in the node.
	// For each attribute, get its name, type, data, and size, and
	// create a corresponding entry in the message;
	if ((status = node.RewindAttrs()))
		return status;
	do {
		char attrName[B_ATTR_NAME_LENGTH];
		if (node.GetNextAttrName(attrName))
			break;

		attr_info attrInfo;
		if (node.GetAttrInfo(attrName, &attrInfo))
			break;
			
		if (data)
			free(data);
		data = malloc(attrInfo.size);
		if (!data)
			break;

		if (node.ReadAttr(attrName, attrInfo.type, 0, data, attrInfo.size) != attrInfo.size)
			break;
		
		if (attrData.AddData(attrName, attrInfo.type, data, attrInfo.size, false))
			break;
	} while (true);

	if (data)
		free(data);
	return B_NO_ERROR;
}

const char *GetUrlFromLink( const char *fname )
{
	BMessage bm;
	const char *url;
	
	ReadAttributes(fname, bm);
	
	if (bm.FindString("META:url", &url) == B_OK)
		return url;
	else
		return "";
}

OakWindow::OakWindow( BRect rect ) 
	: BWindow(rect , "", B_DOCUMENT_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
{
	//n_bookmarks = 0;

	BRect r;
	BMenu *menuFile = new BMenu("File");
	//BMenuItem *menuFileNewTab = new BMenuItem("New tab",
	//	new BMessage(MENU_FILE_NEWTAB), 'T');
	//menuFile->AddItem(menuFileNewTab);
	BMenuItem *menuFileOpenFile = new BMenuItem("Open file"
		B_UTF8_ELLIPSIS, new BMessage(MENU_FILE_OPENFILE), 'O');
	menuFile->AddItem(menuFileOpenFile);
	menuFile->AddSeparatorItem();
	BMenuItem *menuFileAbout = new BMenuItem("About Flying Troll"
		B_UTF8_ELLIPSIS, new BMessage(FILE_ABOUT), 0);
	menuFile->AddItem(menuFileAbout);
	BMenuItem *menuFileDemo = new BMenuItem("Some demo"
		B_UTF8_ELLIPSIS, new BMessage(FILE_DEMO), 0);
	menuFile->AddItem(menuFileDemo);
	BMenuItem *menuFileClose = new BMenuItem("Close", new BMessage(FILE_CLOSE), 'Q');
	menuFile->AddItem(menuFileClose);
	BMenu *menuEdit = new BMenu("Edit");
	BMenu *menuGo = new BMenu("Go");
	menuGo->AddItem(new BMenuItem("Back", new BMessage(HISTORY_BACK), B_LEFT_ARROW));
	menuGo->AddItem(new BMenuItem("Forward", new BMessage(HISTORY_FORWARD), B_RIGHT_ARROW));
	menuGo->AddSeparatorItem();
	menuGo->AddItem(new BMenuItem("Home", new BMessage(GO_HOME)));
	menuBookmarks = new BMenu("Bookmarks");
	menuView = new BMenu("View");
	
	r = Bounds();
	r.top = 0;
	r.bottom = 19;
	menubar = new BMenuBar(r, "menu_bar");
	menubar->AddItem(menuFile);
	menubar->AddItem(menuEdit);
	BMenuItem *menuEditCopy = new BMenuItem("Copy",
		new BMessage(EDIT_COPY), 0);
	menuEdit->AddItem(menuEditCopy);
	menuEdit->AddSeparatorItem();
	BMenuItem *menuEditSelectAll = new BMenuItem("Select All",
		new BMessage(EDIT_SELECTALL), 'A');
	menuEdit->AddItem(menuEditSelectAll);
	BMenuItem *menuEditSearch = new BMenuItem("Find",
		new BMessage(EDIT_SEARCH), 'F');
	menuEdit->AddItem(menuEditSearch);
	BMenuItem *menuEditSearchNext = new BMenuItem("Find next",
		new BMessage(EDIT_SEARCHNEXT), 'G');
	menuEdit->AddItem(menuEditSearchNext);
	menubar->AddItem(menuGo);
	menubar->AddItem(menuBookmarks);
	UpdateBookmarks();
	menubar->AddItem(menuView);
	BMenuItem *menuViewFullScreen = new BMenuItem("Fullscreen",
		new BMessage(VIEW_FULLSCREEN), B_ENTER);
	menuView->AddItem(menuViewFullScreen);
	menuView->AddSeparatorItem();
	menuViewFonts = new BMenu("Font");
	menuView->AddItem(menuViewFonts);
	
	BMenu *menuViewCharset = new BMenu("Charset");
	BMenuItem *ch;
	BMessage *chm;

	chm = new BMessage(VIEW_CHARSET);
	chm->AddInt32("charset", OAK_AUTOCONV);
	ch = new BMenuItem("Auto", chm, 0);
	menuViewCharset->AddItem(ch);

	chm = new BMessage(VIEW_CHARSET);
	chm->AddInt32("charset", B_ISO_8859_1);
	ch = new BMenuItem("ISO-8859-1", chm, 0);
	menuViewCharset->AddItem(ch);

	chm = new BMessage(VIEW_CHARSET);
	chm->AddInt32("charset", B_MS_WINDOWS_CONVERSION);
	ch = new BMenuItem("Windows-1251", chm, 0);
	menuViewCharset->AddItem(ch);

	chm = new BMessage(VIEW_CHARSET);
	chm->AddInt32("charset", B_KOI8R_CONVERSION);
	ch = new BMenuItem("KOI8-R", chm, 0);
	menuViewCharset->AddItem(ch);
	
	chm = new BMessage(VIEW_CHARSET);
	chm->AddInt32("charset", B_MS_DOS_866_CONVERSION);
	ch = new BMenuItem("MS-DOS 866", chm, 0);
	menuViewCharset->AddItem(ch);
	
	chm = new BMessage(VIEW_CHARSET);
	chm->AddInt32("charset", B_UNICODE_CONVERSION);
	ch = new BMenuItem("Unicode", chm, 0);
	menuViewCharset->AddItem(ch);

	menuView->AddItem(menuViewCharset);
	
	menuDebug = new BMenu("Debug");
	menuDebug->AddItem(new BMenuItem("Show borders", new BMessage(DEBUG_BORDERS)));
	menuDebug->AddItem(new BMenuItem("Don't use buffer", new BMessage(DEBUG_DONTUSEBUFFER)));
	menuDebug->AddItem(new BMenuItem("Use buffer", new BMessage(DEBUG_USEBUFFER)));
	menuDebug->AddItem(new BMenuItem("Hide pictures", new BMessage(DEBUG_HIDEPICTURES)));
	menubar->AddItem(menuDebug);
	
	AddChild(menubar);

	r = Bounds();
	r.top = menubar->Frame().bottom + 1;
	
	//btv = new OakTabView(r);
	//AddChild(btv);
	
	//r = btv->Bounds();
	//r.bottom -= btv->TabHeight() + 2;
	//r.right -= 2;
	ftview = new FTView(r, "my", B_FOLLOW_ALL_SIDES, true, false, true);

	//BTab *bt = new BTab();
	//btv->AddTab(ftview, bt);
	//bt->SetLabel("KO-KO-KO");
	//AddChild(btv);
	
	AddChild(ftview);
	ftview->SetFontFamily("Dutch", true);
	ftview->EnableLinks();
	ftview->EnableSelect();
	//ftview->DisableBuffer();

	ScanFontsToMenu();
	
	BMenuItem *mi;
	
	BPopUpMenu *pmenu1 = new BPopUpMenu("pmenu1");
	mi = new BMenuItem("Back", new BMessage(HISTORY_BACK), B_LEFT_ARROW);
	mi->SetTarget(BMessenger(ftview));
	pmenu1->AddItem(mi);
	mi = new BMenuItem("Forward", new BMessage(HISTORY_FORWARD), B_RIGHT_ARROW);
	mi->SetTarget(BMessenger(ftview));
	pmenu1->AddItem(mi);
	pmenu1->AddSeparatorItem();
	mi = new BMenuItem("Select All", new BMessage(EDIT_SELECTALL), 'A');
	mi->SetTarget(BMessenger(ftview));
	pmenu1->AddItem(mi);

	ftview->SetDefaultMenu(pmenu1);

	
	BPopUpMenu *pmenu2 = new BPopUpMenu("pmenu2");
	mi = new BMenuItem("Back", new BMessage(HISTORY_BACK), B_LEFT_ARROW);
	mi->SetTarget(BMessenger(ftview));
	pmenu2->AddItem(mi);
	mi = new BMenuItem("Forward", new BMessage(HISTORY_FORWARD), B_RIGHT_ARROW);
	mi->SetTarget(BMessenger(ftview));
	pmenu2->AddItem(mi);
	pmenu2->AddSeparatorItem();
	mi = new BMenuItem("Copy", new BMessage(EDIT_COPY), 0);
	mi->SetTarget(BMessenger(ftview));
	pmenu2->AddItem(mi);
	mi = new BMenuItem("Select All", new BMessage(EDIT_SELECTALL), 'A');
	mi->SetTarget(BMessenger(ftview));
	pmenu2->AddItem(mi);

	ftview->SetSelectMenu(pmenu2);

	fpanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false,
		new BMessage(FILEPANEL_OPEN));
	fpanel->SetPanelDirectory("/boot/beos/documentation");
	
	SetSizeLimits(400, 40000, 200, 40000);
	
	fullscreen = false;
	
	//ftview->SetCharset(B_ISO1_CONVERSION);
	//ftview->LoadFile("about:home");
	//ftview->LoadHtml("<BODY bgcolor=#fedf60><table><td bgcolor=#f0f0f0 align=center> dg drgdrg drg drg drg dr sefjseiop fjisej fisefo seiof jseiofj iosejoifffffffffj fjsei jfisejfi<br><img src=\"images/haikulogo\"></td></table></BODY>");
	//ftview->LoadHtml("<BODY bgcolor=#fedf60><CENTER>dg drgdrg drg drg drg dr sefjseiop fjisej fisefo seiof jseiofj iosejoifffffffffj fjsei jfisejfi<br><img src=\"images/haikulogo\"></CENTER></BODY>");
	//btv->FocusTab();
	//btv->TabAt(0)->SetLabel(ftview->Title());

	//oakview->SetMouseEventMask(0, 0);
	
	Show();
}

bool OakWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void OakWindow::MessageReceived( BMessage *message )
{
	const char *bmname, *bmpath, *family;
	BString url, url2;
	OakAboutWindow *oaw;
	int i;
	BRect r;
	uint32 charset;
	BFile bfile;
	BMessage attrData;
	BNode node;
	BNodeInfo nodeinfo;
	FILE *f;
	char append[10];

	switch (message->what)
	{
	case B_MOUSE_WHEEL_CHANGED:
		ftview->MessageReceived(message);
		break;
	case MENU_FILE_OPENFILE:
		fpanel->Show();
		break;
	case MENU_FILE_NEWTAB:
		r = btv->Bounds();
		r.bottom -= btv->TabHeight();
		//r.right -= 2;
		btv->AddTab(ftview = new FTView(r, "my", B_FOLLOW_ALL_SIDES));
		btv->Draw(btv->Frame());
		break;
	case FILEPANEL_OPEN:
		if (ftview->LoadFileMessage(message) != B_OK)
			printf("> Error!\n");
		break;
	case MENU_BOOKMARK_ADD:
		url.SetTo(ftview->Title());
		if (url.Length() == 0)
			url.SetTo("Bookmark");
		url.Prepend("/boot/home/config/settings/FlyingTroll/Bookmarks/");
		i = 0;
		url2 = url;
		while ((f = fopen(url2.String(), "rt")) != NULL)
		{
			i++;
			url2 = url;
			sprintf(append, " %d", i + 1);
			url2.Append(append);
		}
		if (i != 0)
			url = url2;
		bfile.SetTo(url.String(), B_CREATE_FILE);
		node.SetTo(url.String());
		nodeinfo.SetTo(&node);
		nodeinfo.SetType("application/x-vnd.Be-bookmark");
		printf("Adding bookmark '%s'\n", url.String());
		if (bfile.InitCheck() == B_OK)
		{
			url.SetTo("file://");
			printf("url: %s\n", ftview->Url());
			url.Append(ftview->Url());
			printf("url2: %s\n", url.String());
			attrData.AddData("META:url", B_STRING_TYPE, url.String(), url.Length() + 1, false);
			attrData.AddData("META:keywords", B_STRING_TYPE, "", 0, false);
			attrData.AddData("META:title", B_STRING_TYPE, "", 0, false);
			WriteAttributes(bfile, attrData);
			bfile.Unset();
			UpdateBookmarks();
		}
		else
			printf("Cannot add bookmark '%s' :(\n", url.String());
		break;
	case MENU_BOOKMARK_OPEN:
		message->FindString("name", &bmname);
		message->FindString("path", &bmpath);
		
		// attributtes!!! Oi, Mama!!!
		
		url.SetTo(GetUrlFromLink(bmpath));
		
		if (url.Compare("file://", 7) == 0)
			url.RemoveFirst("file://");
		
		ftview->LoadFile(url.String());
		//SetOakTitle(bmpath);
		break;
	case VIEW_CHARSET:
		message->FindInt32("charset", (int32 *)&charset);
		ftview->SetCharset(charset);
		ftview->Reload();
		break;
	case VIEW_FULLSCREEN:
		if (!fullscreen)
		{
			SetLook(B_NO_BORDER_WINDOW_LOOK);
			oldframe = Frame();
			MoveTo(BPoint(0, 0));
			ResizeTo(BScreen().Frame().right -1, BScreen().Frame().bottom -1);
			menuView->ItemAt(0)->SetMarked(true);
		}
		else
		{
			SetLook(B_DOCUMENT_WINDOW_LOOK);
			MoveTo(BPoint(oldframe.left, oldframe.top));
			ResizeTo(oldframe.right - oldframe.left -1,
				oldframe.bottom - oldframe.top -1);
			menuView->ItemAt(0)->SetMarked(false);
		}
		fullscreen = !fullscreen;
		break;
	case FILE_ABOUT:
		oaw = new OakAboutWindow();
		break;
	case FILE_DEMO:
		new OakDemoWindow();
		break;
	case FILE_CLOSE:
		Quit();
		break;
	case MENU_BOOKMARK_MANAGE:
		system("/boot/beos/system/Tracker ~/config/settings/FlyingTroll/Bookmarks");
		break;
	case MENU_BOOKMARK_RESCAN:
		UpdateBookmarks();
		break;
	case SET_FONT:
		message->FindString("family", &family);
		static font_family f2;
		ftview->GetFontFamily(&f2);
		if (strcmp(family, f2) != 0)
		{
			for (i = 0; i < menuViewFonts->CountItems(); i++)
			{
				printf("--- Item: %s\n", menuViewFonts->ItemAt(i)->Label());
			
				if (strcmp(menuViewFonts->ItemAt(i)->Label(), f2) == 0)
					menuViewFonts->ItemAt(i)->SetMarked(false);
				else if (strcmp(menuViewFonts->ItemAt(i)->Label(), family) == 0)
					menuViewFonts->ItemAt(i)->SetMarked(true);
			}
			ftview->SetFontFamily(family);
		}
		//ftview->MyDraw(OAK_REDRAW);
		break;
	case DEBUG_BORDERS:
		ftview->SetFBorders();
		break;
	case DEBUG_DONTUSEBUFFER:
		ftview->DisableBuffer();
		break;
	case DEBUG_USEBUFFER:
		ftview->DisableBuffer(false);
		break;
	case DEBUG_HIDEPICTURES:
		ftview->DisablePictures();
		break;
	default:
		ftview->MessageReceived(message);
		break;
	}
}

void ScanAndAdd( const char *dirname, BMenu *menu, int level = 0 )
{
	BDirectory dir, dir2;
	entry_ref ref, ref2;
	BEntry entry, entry2;
	BPath path;
	BString d, d2, s;
	char fname[150];
	BMenu *m;
	BMessage *bm;

	if (level == 3)
		return;
	
	for (int i = 0; i < 2; i++)
		if (dir.SetTo(dirname) == B_OK)
		{
			while (dir.GetNextRef(&ref) == B_OK)
			{
				entry.SetTo(&ref);
				entry.GetName(fname);
	
				//printf("> Bookmarks fname: %s\n", fname);
				d.SetTo(dirname);
				if (d.FindLast("/") != d.Length() - 1)
					d.Append("/");
				d.Append(fname);
				if (dir2.SetTo(d.String()) == B_OK && i == 0)
				{
					m = new BMenu(fname);
					menu->AddItem(m);
	
					ScanAndAdd(d.String(), m, level + 1);
					//printf("> Bookmarks dir: %s\n", d.String());
				}
				else if (dir2.SetTo(d.String()) != B_OK && i == 1)
				{
					bm = new BMessage(MENU_BOOKMARK_OPEN);
					bm->AddString("name", fname);
					bm->AddString("path", d.String());
					s.SetTo(fname);
					//s.Prepend("  ");
					menu->AddItem(new BMenuItem(s.String(), bm));
				}
			}
		}
		else
			printf("> Error! Cannot open bookmarks\n");
}

void OakWindow::UpdateBookmarks()
{
	
	//for (int i = 0; i < n_bookmarks; i++)
	//	delete bookmarks[i];
	//n_bookmarks = 0;
	
	//printf("> Bookmarks directory: '%s'\n", B_USER_DIRECTORY "/HelpMeBookmarks/");

	menuBookmarks->RemoveItems(0, menuBookmarks->CountItems(), true);
	menuBookmarks->AddItem(new BMenuItem("Add to bookmarks", new BMessage(MENU_BOOKMARK_ADD), 'B'));
	menuBookmarks->AddItem(new BMenuItem("Manage bookmarks"
		B_UTF8_ELLIPSIS, new BMessage(MENU_BOOKMARK_MANAGE)));
	menuBookmarks->AddSeparatorItem();
	ScanAndAdd("/boot/home/config/settings/FlyingTroll/Bookmarks/", menuBookmarks);
	menuBookmarks->AddSeparatorItem();
	menuBookmarks->AddItem(new BMenuItem("Rescan bookmarks", new BMessage(MENU_BOOKMARK_RESCAN)));
}

void OakWindow::ScanFontsToMenu()
{
	BMenuItem *m;
	BMessage *bm;


	font_family family, f2;
	font_style style;
	int n_families = count_font_families();
	uint32 flags;
	update_font_families(false);

	ftview->GetFontFamily(&f2);
	for (int i = 0; i < n_families; i++)
	{
		if (get_font_family(i, &family, &flags) == B_OK)
		{
			//printf("> Family: '%s'\n", family);
			if (get_font_style(family, 0, &style, &flags) == B_OK)
			{
				bm = new BMessage(SET_FONT);
				bm->AddString("family", family);
				m = new BMenuItem(family, bm);
				if (strcmp(family, f2) == 0)
				{
					m->SetMarked(true);
				}
				menuViewFonts->AddItem(m);
			}
		}
	}

}

void OakWindow::FrameResized( float w, float h )
{
	//ftview->FrameResized(w, h);
}
