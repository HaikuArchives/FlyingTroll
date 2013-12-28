#ifndef _OAKVIEW_H_
#define _OAKVIEW_H_

#include <Application.h>
#include <Messenger.h>
#include <Message.h>
#include <View.h>
#include <String.h>
#include <File.h>
#include <stdlib.h>
#include <regex.h>
#include <Font.h>
#include <Bitmap.h>
#include <Entry.h>
#include <Path.h>
#include <TranslationUtils.h>
#include <stdio.h>
#include <TextControl.h>
#include <TextView.h>
#include <Cursor.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>

#include "Constants.h"
#include "FTConstants.h"
#include "OakImage.h"
#include "OakLink.h"
#include "OakAnchor.h"
#include "OakWord.h"
#include "OakHistory.h"

void ParseTag( BString str, BString *lex, BString *value, int *col );

bool SetFamily( BFont *font, const char *s, bool prim = true );

class OakTD
{
public:
	int col;
	int row;
	int colspan;
	int rowspan;
	bool canresize;
	bool is_bgcolor;
	rgb_color bgcolor;
	int width;
	bool widthp;
	bool lines;
	uint32 align;
	//BRect r;
	OakTD()
	{
		col = 0; row = 0; colspan = 1; rowspan = 1; canresize = true;
		is_bgcolor = false; width = -1; widthp = false;
		lines = false; // multiline col (for resizing)
		align = ALIGN_LEFT;
	}
};

class OakTable
{
public:
	int cols;
	int td_pos;
	int rows;
	int tr_pos;
	bool border;
	int tdwidths[MAX_COLS];
	OakTD *td[3000];
	int tds;
	int offset;
	int aoffset;
	int line;
	int right;
	int width;
	bool widthp;
	uint32 align;
	int cellpadding, cellp;
	rgb_color bgcolor, pbgcolor;
	bool is_bgcolor;
	
	//new version:
	int colposes[MAX_COLS + 1];
	bool tdcanresize[MAX_COLS];
	int rowposes[MAX_ROWS + 1];
	int colmaxes[MAX_COLS + 1];
	//int rowmaxes[MAX_ROWS + 1];

	bool created;
	
	OakTable() { tds = 0; cols = 0; rows = 0; border = false; td_pos = -1; tr_pos = -1;
		width = 0; is_bgcolor = false; canresize = true; created = false; }
	bool canresize;
	
	BRect rect( int pos = -1 )
	{
		if (pos == -1)
			return BRect(colposes[0], rowposes[0], colposes[cols], rowposes[rows]);
			
		OakTD *TD = td[pos];
		return BRect(colposes[TD->col], rowposes[TD->row],
			colposes[TD->col + TD->colspan], rowposes[TD->row + TD->rowspan]);
	}
	
	void ResizeRow( int j )
	{
		if (tr_pos != -1)
		{
			int tdpos = td_pos == -1 ? 0 : td_pos;
			int dy = j - rowposes[td[tdpos]->row + td[tdpos]->rowspan];
			if (dy > 0)
				for (int i = td[tdpos]->row + td[tdpos]->rowspan; i <= rows; i++)
					rowposes[i] += dy;
		}
	}
	
	void CheckWidth( int i, int *ri, int d = 0, int right = -1 )
	{
		//printf(">> checkwidth\n");
		if (td_pos != -1)
		{
			int pos = td[td_pos]->col + td[td_pos]->colspan;
			if (pos <= cols)
			{
				if (colmaxes[pos] < i - d)
					colmaxes[pos] = i - d;
				int dx = i - d - colposes[pos];
				if (dx > 0)
				{
					for (int j = pos; j <= cols; j++)
					{
						colposes[j] += dx;
						if (right != -1 && j != pos && colposes[j] > right)
							colposes[j] = right;
					}
					*ri += dx;
				}
			}
		}
	}
	
	void CheckMax( int tdpos = -1 )
	{
		//printf(">> checkmax\n");
		int dx;
		int cl;
		if (canresize)
			cl = cols + 1;
		else
			cl = cols;
		if (tdpos != -1)
		{
			if (canresize)
				cl = tdpos + 1;
			else
				cl = tdpos;
		}
		//printf("CL:%d\n", cl);
		//if (canresize)
		{
			for (int i = 1; i <= cl; i++)
			{
				dx = colposes[i] - colmaxes[i];
				if (dx > 0 && tdcanresize[i - 1])
				{
					if (i != cols || canresize)
					{
						colposes[i] -= dx;
					}
					if (i < cols)
					{
						colmaxes[i + 1] -= dx;
						//colposes[i + 1] -= dx;
					}
					if (i == cols && !canresize)
					{
					}
				}
			}
		}
	}
};

class OakLexem : public BString
{
public:
	OakLexem ( const char *s ) :
		BString(s)
	{
		//printf("RECEIVED (%s)\n", s);

		if (s[0] == '<')
			is_tag = true;
		else
			is_tag = false;
		
		if (is_tag)
		{
			lex = new BString[20];
			val = new BString[20];
			if (FindFirst("<!--") == -1)
			{
				ParseTag(BString(s), lex, val, &col);
				RemoveFirst("<");
				if (FindFirst(">") != -1)
					Truncate(FindFirst(">"));
				if (FindFirst(" ") != -1)
					Truncate(FindFirst(" "));
			}
			else
			{
				SetTo("");
			}
		}
		strw = 0;
	}
	~OakLexem()
	{
		if (is_tag)
		{
			delete [] lex;
			delete [] val;
		}
	}
	bool is_tag;
	BString *lex;
	BString *val;
	int col;
	int strw;
};

class OakView : public BView
{
private:
	BString *html;
	BString *body;
	rgb_color bgcolor;
	rgb_color pcolor;
	rgb_color lcolor;
	BString *title;
	BString *filename;
	BString *path;
	int vmax, hmax;
	int vpos;
	OakImage *images[255];
	int n_images;
	OakLink *links[MAX_LINKS];
	int n_links;
	OakAnchor *anchors[MAX_ANCHORS];
	int n_anchors;
	OakWord *words[MAX_WORDS];
	int32 n_words;
	BBitmap *cache;
	OakImage *bg;
	OakHistory history[MAX_HISTORY];
	int n_history;
	int pos_history;
	class FTView *papa;
	// for select:
	bool select_is, select_processed, overlink;
	int32 select_from, select_to;
	int32 buttonsdown;
	BPoint mousedown;
	bool mousedown_is;
	int64 lastselectscroll;
	int32 searched;
	// TABLES! :-(
	OakTable *tables[MAX_TABLES];
	int n_tables;
	OakLexem *lexems[MAX_LEXEMS];
	int32 n_lexems;
	char lines[MAX_LINES];
	int linews[MAX_LINES];
	int32 n_lines;
	BFont sysfont;
	bool hscrolled;
	int maxr;
	bool fborders;
	BPopUpMenu *default_menu, *select_menu, *link_menu;
	int vl_tables;
	uint32 charset;
	bool do_words, do_links;
	bool do_buffer, do_pictures;
	BCursor *hand_cursor;
	BMessageRunner *scrolldownmessrunner, *scrollupmessrunner;
public:
	OakView( BRect r,  const char *name, class FTView *daddy );
	~OakView();
	virtual void Draw( BRect ur );
	virtual void MouseUp( BPoint p );
	virtual void MouseDown( BPoint p );
	virtual void MouseMoved( BPoint p, uint32 transit, const BMessage *m );
	virtual void MessageReceived( BMessage *message );
	void MyDraw( uint32 );
	status_t LoadFile( BMessage *message, const char *fname2 = NULL,
		bool his = false );
	const char *Title();
	const char *FileName();
	const char *Path();
	OakHistory *History( int n ) { return &history[n]; }
	int VMax();
	int VPos();
	int HMax() { return hmax; }
	int CountHistory();
	int HistoryPos();
	void SetVPos( int );
	void Back( int to = 0 );
	void Forward();
	void InitBlank();
	void InitAbout();
	void InitHome();
	void SelectAll();
	void Search( const char *s, bool again = false );
	void ToClipboard();
	void SetFontFamily( const char *s, bool prim = true ) {	SetFamily(&sysfont, s, prim); }
	void GetFontFamily( font_family *f ) {
		font_style fs;
		sysfont.GetFamilyAndStyle(f, &fs);
	}
	void SetDefaultMenu(BPopUpMenu *m) { default_menu = m; }
	void SetSelectMenu(BPopUpMenu *m) { select_menu = m; }
	void SetLinkMenu(BPopUpMenu *m) { link_menu = m; }
	void SetFBorders() { fborders = !fborders; MyDraw(OAK_REDRAW); }
	void Process();
	void LoadHtml( const char *str )
	{
		html->SetTo(str);
		Process();
	}
	uint32 Charset() { return charset; }
	void SetCharset( uint32 ch ) { charset = ch; };
	void Reload();
	void SetDoWords( bool dw ) { do_words = dw; }
	void SetDoLinks( bool dl ) { do_links = dl; }
	void SetDoBuffer( bool db )
	{
		do_buffer = db;
	}
	void SetDoPictures( bool pc )
	{
		do_pictures = pc;
	}
	const char *Address()
	{
		BString *s = new BString();
		s->SetTo(*path);
		s->Append(*filename);
		printf("RETURN:%s\n", s->String());
		return s->String();
	}
	status_t LoadURL( const char *url );
};

#endif
