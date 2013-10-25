#include "OakView.h"
#include "Oak.h"
#include "OakLink.h"
#include "OakWord.h"

#include <fs_attr.h>
#include <Clipboard.h>
#include <NodeInfo.h>
#include <UTF8.h>
#include <string.h>

status_t ReadAttributes(const char *path,
	BMessage& attrData);

bool SetFamily( BFont *font, const char *s, bool prim = true )
{
	//return false; // Poka chto ne ponyal, kak ih vybirat' :-(

	font_family family;
	font_style style;
	int n_families = count_font_families();
	uint32 flags;
	update_font_families(false);
	for (int i = 0; i < n_families; i++)
	{
		if (get_font_family(i, &family, &flags) == B_OK)
		{
			//printf("> Family: '%s'\n", family);
			if (get_font_style(family, 0, &style, &flags) == B_OK &&
				(!prim && (strcmp(family, s) == 0) ||
				(prim && (strstr(family, s)))))
			{
				font->SetFamilyAndStyle(family, style);
				return true;
			}
		}
	}
	return false;
}

void ParseTag( BString str, BString *lex, BString *value, int *col )
{
	int i = 0;
	bool quote;
	
	if (str.ByteAt(0) == '<')
	{
		while (str.ByteAt(0) != ' ' && str.ByteAt(0) != '>' &&
			str.Length() > 0)
			str.Remove(0, 1);

		while (str.ByteAt(0) == ' ' && str.Length() > 0)
			str.Remove(0, 1);

		while (str.ByteAt(0) != '>' && str.Length() > 0)
		{
			lex[i].SetTo("");
			value[i].SetTo("");
			while (str.ByteAt(0) != '>' && str.ByteAt(0) != ' ' && str.Length() > 0)
			{
				if (str.ByteAt(0) == '=')
				{
					str.Remove(0, 1);
					while (str.ByteAt(0) == ' ')
						str.Remove(0, 1);
					quote = false;
					while ((!quote && str.ByteAt(0) != '>' &&
						str.ByteAt(0) != ' ' &&	str.Length() > 0) ||
						(quote && str.ByteAt(0) != '"' &&
						str.Length() > 0))
					{
						if (str.ByteAt(0) == '"')
							quote = true;
						else
							value[i].Append(str.ByteAt(0), 1);
						str.Remove(0, 1);
					}
					if (quote && str.ByteAt(0) == '"')
						str.Remove(0, 1);
						//printf("> Parse:: '%s'\n", value[i].String());
				}
				else if (str.ByteAt(0) != ' ')
				{
					lex[i].Append(str.ByteAt(0), 1);
					str.Remove(0, 1);

					if (str.ByteAt(0) == ' ' && str.ByteAt(1) == '=')
					while (str.ByteAt(0) == ' ')
						str.Remove(0, 1);
				}
				else
				{
					str.Remove(0, 1);
				}
			}
			while (str.ByteAt(0) != '>' && str.ByteAt(0) == ' ' && str.Length() > 0)
				str.Remove(0, 1);
			i++;
		}
	}
	*col = i;
}

void GetRgbFromString( BString s, rgb_color *rgb )
{
	static int r, g, b;

	//printf("> color: %s\n", s.String());
	
	if (s.Compare("white") == 0)
	{
		rgb->red = 255;
		rgb->green = 255;
		rgb->blue = 255;
	}
	else if (s.Compare("red") == 0)
	{
		rgb->red = 255;
		rgb->green = 0;
		rgb->blue = 0;
	}
	else if (s.Compare("green") == 0)
	{
		rgb->red = 0;
		rgb->green = 255;
		rgb->blue = 0;
	}
	else if (s.Compare("gray") == 0)
	{
		rgb->red = 200;
		rgb->green = 200;
		rgb->blue = 200;
	}
	else if (s.Compare("yellow") == 0)
	{
		rgb->red = 255;
		rgb->green = 255;
		rgb->blue = 0;
	}
	else if (s.Compare("blue") == 0)
	{
		rgb->red = 0;
		rgb->green = 0;
		rgb->blue = 255;
	}
	else if ((s.Compare("#", 1) == 0 && s.Length() == 7) ||
		(s.Length() == 6))
	{
		s.RemoveFirst("#");
		
		sscanf(s.String(), "%02x", &r);
		s.Remove(0, 2);
//		rgb->red *= 16;
//		sscanf(s.String(), "%1x", &rgb->red);
//		s.Remove(0, 1);

		sscanf(s.String(), "%02x", &g);
		s.Remove(0, 2);

		sscanf(s.String(), "%02x", &b);
		
		rgb->red = r;
		rgb->green = g;
		rgb->blue = b;
	}
}

OakView::OakView( BRect r,  const char *name, class FTView *daddy) : BView(r, name,
	B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	html = new BString("");
	title = new BString("");
	body = new BString("");
	filename = new BString("");
	path = new BString("");

	//bgcolor = {216, 216, 216, 255};
	bgcolor.red = 240;
	bgcolor.green = 240;
	bgcolor.blue = 240;
	bgcolor.alpha = 255;
	
	papa = daddy;
	
	vmax = 0;
	
	n_images = 0;
	n_links = 0;
	n_anchors = 0;
	n_words = 0;
	n_history = 0;
	pos_history = 0;
	
	cache = NULL;
	bg = NULL;
	
	select_is = false;
	select_processed = false;
	select_from = 0;
	select_to = 0;
	lastselectscroll = 0;
	
	mousedown_is = false;

	sysfont = be_plain_font;
	
	hscrolled = false;
	
	default_menu = NULL;
	select_menu = NULL;
	link_menu = NULL;
	
	overlink = false;
	
	fborders = false;
	
	vl_tables = -1;

	charset = B_ISO1_CONVERSION;
	
	do_words = false;
	do_links = false;
	do_buffer = true;
	do_pictures = true;
	
	scrolldownmessrunner = NULL;
	scrollupmessrunner = NULL;
	
	unsigned char *mas = (unsigned char *)malloc(16 * 2 + 4 + 16 * 2);
	mas[0] = 16;
	mas[1] = 1;
	mas[2] = 0;
	mas[3] = 0;
	
	unsigned char cur[16 * 16] =
	{
		2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 1, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 1, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 1, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
		2, 2, 2, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 2, 2,
		2, 2, 2, 2, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 2,
		2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		2, 1, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1,
		2, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1,
		2, 2, 2, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1,
		2, 2, 2, 2, 2, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1,
		2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2,
		2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
	};
	
	int i, j, d;
	for (i = 0; i < 16 * 16; i += 8)
	{
		d = 128;
		mas[4 + i / 8] = 0;
		for (j = 0; j < 8; j++, d /= 2)
			if (cur[i + j] == 1)
				mas[4 + i / 8] += d;
	}
	for (i = 0; i < 16 * 16; i += 8)
	{
		d = 128;
		mas[36 + i / 8] = 0;
		for (j = 0; j < 8; j++, d /= 2)
			if (cur[i + j] != 2)
				mas[36 + i / 8] += d;
	}
	
	hand_cursor = new BCursor(mas);
	free(mas);
}

OakView::~OakView()
{
	delete html;
	delete title;
	delete body;
	delete filename;
	delete path;
	
	delete hand_cursor;
	
	int i;
	int32 i32;
	for (i = 0; i < n_images; i++)
		delete images[i];
	for (i = 0; i < n_links; i++)
		delete links[i];
	for (i = 0; i < n_anchors; i++)
		delete anchors[i];
	for (i32 = 0; i32 < n_words; i32++)
		delete words[i32];

	if (cache != NULL)
		delete cache;
	if (bg != NULL)
		delete bg;
}

void OakView::MyDraw( uint32 p = OAK_DRAW )
{
	if (p == OAK_DRAW && cache != NULL)
	{
		//printf("> OakView:: Draw from buffer...\n");
		DrawBitmap(cache, BPoint(0, 0));
		if (select_is)
		{
			static int32 i32;
			
			SetHighColor(0, 0, 0);
			SetDrawingMode(B_OP_INVERT);
			if (select_from > select_to)
				for (i32 = select_to; i32 <= select_from; i32++)
				{
					if (!strstr("\n", words[i32]->Word()))
						FillRect(BRect((int)words[i32]->Bounds().left,
							(int)words[i32]->Bounds().top,
						(int)words[i32]->Bounds().right - 1,
							(int)words[i32]->Bounds().bottom + 1));
				}
			else
				for (i32 = select_from; i32 <= select_to; i32++)
				{
					if (!strstr("\n", words[i32]->Word()))
						FillRect(BRect((int)words[i32]->Bounds().left,
							(int)words[i32]->Bounds().top,
						(int)words[i32]->Bounds().right - 1,
							(int)words[i32]->Bounds().bottom + 1));
				}
			SetDrawingMode(B_OP_COPY);
		}
		if (searched != -1)
		{
			SetHighColor(200, 20, 40);
			SetPenSize(3);
			MovePenTo(BPoint((int)words[searched]->Bounds().left,
				(int)words[searched]->Bounds().bottom));
			StrokeLine(BPoint((int)words[searched]->Bounds().right,
				(int)words[searched]->Bounds().bottom));
		}
	}
	else
	{
		//char *mas, *m, *catch_m;
		int i, j = 0, k = 0, q = 0;
		OakLexem *str;
		BFont font, bfont, hfonts[10], prefont, ttfont;
		bool istag, pre = false, link = false, tt = false;
		int ul_level = 0, h_level = 0, bq_level = 0;
		uint16 face;
		rgb_color bcolor[20];
		BBitmap *bb = NULL;
		BString fname, s, link_href;
		BView *bv = NULL;
		//BRect r;
		//OakLink *oaklink;
		int32 i32;
		int offset = I_START;
		int strw;
		int table_pos = 0, table_pos2 = -1;
		static BRect table_rects[MAX_TABLES];
		int tdw = 0, trh;
		int table_number[20];
		bool td_is = false;
		int right = (int)Bounds().right;
		bool br = false, nobr = false;
		int cellpadding = CELLPADDING;
		int32 lexpos = 0;
		int32 line_pos;
		bool nextlex, nextlex2;
		int br_left = 0, br_left_to = 0;
		int br_right = 0, br_right_to = 0;
		bool newp = true, autobr = false;
		uint32 align = ALIGN_LEFT;
		int color_level;
		int link_color_level = -1;
		bool underline = false;
		BString ss;
		bool code = false;
		OakTable *TABLE = NULL;
		OakTD *TD = NULL;
		bool sup = false;
		bool br_reset = false;
		
		if (p == OAK_TABLES)
			printf("> OakView:: Calcucating tables...\n");
		else if (p == OAK_FINDVMAX)
			printf("> OakView:: Find VMax...\n");
		else
			printf("> OakView:: Redraw graphics...\n");
		if (p == OAK_REDRAW)
		{
			if (cache != NULL)
				delete cache;
			printf("> OakView:: Cache is deleted...\n");
			printf("> OakView:: Trying to delete links(%d)...\n", n_links);
			for (i32 = 0; i32 < n_links; i32++)
				delete links[i32];
			n_links = 0;
			printf("> OakView:: Links are deleted...\n");
			printf("> OakView:: Trying to delete anchors(%d)...\n", n_anchors);
			for (i = 0; i < n_anchors; i++)
				delete anchors[i];
			n_anchors = 0;
			printf("> OakView:: Anchors are deleted...\n");
			printf("> OakView:: Trying to delete words(%ld)...\n", n_words);
			for (i32 = 0; i32 < n_words; i32++)
				delete words[i32];
			n_words = 0;
			printf("> OakView:: Words are deleted...\n");
		}
		
		printf("> OakView:: Old data is deleted...\n");
		

		if (p == OAK_FINDVMAX)
		{
			n_lines = 0;
			for (i32 = 0; i32 < MAX_LINES; i32++)
				lines[i32] = FONT_SIZE;
			for (i32 = 0; i32 < MAX_LINES; i32++)
				linews[i32] = 0;
			//MyDraw(OAK_LINES);
		}

		if (p == OAK_FINDVMAX && n_tables > 0)
		{
			MyDraw(OAK_TABLES);
			//MyDraw(OAK_TABLES);
		}

		if (p == OAK_FINDVMAX)
		{
			MyDraw(OAK_LINES);
		}
		
		if (p == OAK_REDRAW)
		{
			hmax = 0;

			maxr = (int)Bounds().right - cellpadding;
			
			//LockLooper();
			MyDraw(OAK_FINDVMAX);
			//for (k = 0; k < vl_tables; k++)
			//	MyDraw(OAK_FINDVMAX);
			//UnlockLooper();
			
			BRect bvr = papa->Bounds();
			if (bvr.bottom < vmax)
				bvr.bottom = vmax;
			if (bvr.right < hmax)
			{
				//if (!hscrolled)
				{
					hscrolled = true;
					//Parent()->ResizeBy(0, -14);
					//papa->HScrollBar()->SetRange(0, hmax - bvr.right);
				}
				
				if (hmax > 10000)
					hmax = 1000;
				bvr.right = hmax;
				//ResizeTo(bvr.right - B_V_SCROLL_BAR_WIDTH - 1,
				//	bvr.bottom);
			}
			else
			{
				if (hscrolled)
				{
					hscrolled = false;
					//papa->HScrollBar()->SetEnabled(true);
				}
			}
			
			printf("> Trying to create BView(%d, %d, %d, %d)\n",
				(int)bvr.left, (int)bvr.top, (int)bvr.right, (int)bvr.bottom);
			
			if (do_buffer)
			{
				bv = new BView(bvr, "cache", B_FOLLOW_NONE, B_WILL_DRAW);
				cache = new BBitmap(bvr, B_RGB32, true);
			}
			else
			{
				bv = this;
				cache = NULL;
				//bv->ResizeTo(bvr.Width(), bvr.Height());
			}

			printf("> BView created\n");
			
			if (do_buffer)
			{
				cache->Lock();
				cache->AddChild(bv);
				cache->Unlock();
			}
			
			bv->LockLooper();
			
			bv->SetHighColor(bgcolor);
			bv->FillRect(bvr);
			if (bg != NULL && bg->Bitmap() != NULL)
			{
				int iw = (int)bg->Bitmap()->Bounds().right;
				int ih = (int)bg->Bitmap()->Bounds().bottom;
			
				if (iw != 0 && ih != 0)
				{
					//printf("bg?\n");
				
					printf("> MyDraw:: Texturing with '%s'...\n", bg->Name()->String());
				
					for (i = 0; i < bvr.bottom; i += ih)
						for (j = 0; j < bvr.right; j += iw)
							bv->DrawBitmap(bg->Bitmap(), BPoint(j, i));
				}
			}
		
			for (k = 0; k < n_tables; k++)
			{
				if (tables[k]->is_bgcolor)
				{
					bv->SetHighColor(tables[k]->bgcolor);
					bv->FillRect(tables[k]->rect());
				}
				for (q = 0; q < tables[k]->tds; q++)
				{
					TD = tables[k]->td[q];
					if (TD->is_bgcolor)
					{
						bv->SetHighColor(TD->bgcolor);
						bv->FillRect(tables[k]->rect(q));
					}
				}

				if (tables[k]->border || fborders)
				{
					for (q = 0; q < tables[k]->tds; q++)
					{
						TD = tables[k]->td[q];
						
						//if (!TD->lines)
							bv->SetHighColor(150, 150, 150);
						//else
						//	bv->SetHighColor(150, 0, 0);
						bv->StrokeRect(tables[k]->rect(q));
					}
					TD = NULL;
				}
			}

			TD = NULL;
		
			bv->SetHighColor(pcolor);
			bv->SetLowColor(bgcolor);
		}
		
		cellpadding = CELLPADDING;

		font = sysfont;
		font.SetSize(FONT_SIZE);
		ttfont = font;
		bfont = font;
		
		table_pos = 0;
		table_pos2 = -1;
		
		right = (int)Bounds().right - cellpadding;//maxr;
		//printf("---------------------RIGHT:%d\n", right);
		offset = I_START + cellpadding;
		
		lexpos = 0;
		line_pos = 0;
		
		nextlex2 = true;
		
		br_left = -1;
		br_right = -1;
		br_left_to = -1;
		br_right_to = -1;
		
		bq_level = 0;
		
		int aoffset = 0;
		
		bcolor[0] = pcolor;
		
		//j = (int)font.Size() + 20;
		
		j = cellpadding;
		
		hmax = 0;
		
		color_level = -1;

		bool t_end = false; // eto chtoby... eee.... blya. chtoby posle tablic <br> stavilsya
		
		//align = ALIGN_CENTER;
		
		while (lexpos < n_lexems)
		{
			//catch_m = m;
			//while (*m == ' ' || *m == '\n' || *m == '\r' || *m == '\t')
			//	m++;
			
			if (right <= offset)
				right = offset + 1;
			

			i = offset;
			br = false;
			nobr = false;
			
			aoffset = offset;
			
			if (p != OAK_LINES)
			{ // aligns
				if (align == ALIGN_CENTER)
				{
					//printf("line width: %d\n", linews[line_pos]);
					aoffset += (right - offset - linews[line_pos]) / 2;
					i = aoffset;
				}
				else if (00&&align == ALIGN_RIGHT && linews[line_pos] > 0)
				{
					aoffset = right - linews[line_pos];
					i = aoffset;
				}
			}
			
			if (aoffset < 0)
			{
				aoffset = offset;
				i = aoffset;
			}
			
			//aoffset = offset;
			//i = aoffset;

			if (br_reset)
			{
				if (br_left != -1)
				{
					if (j < br_left_to)
						j = br_left_to;
					br_left = -1;
					br_left_to = -1;
				}
				else if (br_right != -1)
				{
					if (j < br_right_to)
						j = br_right_to;
					br_right = -1;
					br_right_to = -1;
				}
				br_reset = false;
			}
			if (br_left != -1)
			{
				if (br_left_to > j)
				{
					i += br_left;
					aoffset += br_left;
				}
				else
				{
					br_left = -1;
					br_left_to = -1;
				}
			}

			if (br_right != -1)
			{
				if (br_right_to > j)
				{
				}
				else
				{
					br_right = -1;
					br_right_to = -1;
				}
			}

			
			while (i <= right + cellpadding - (br_right == -1 ? 0 : br_right) &&
				lexpos < n_lexems && !br)
			{
				nextlex = true;
			
				newp = true;

				str = lexems[lexpos];
				
				/*printf("[ ");
				for (k = 0; k <= table_pos2; k++)
					printf("%d ", table_number[k]);
				printf("]\n");*/
				printf("LXM:%s\n", str->String());
				
				if (table_pos2 > -1)
					TABLE = tables[table_number[table_pos2]];

				istag = str->is_tag;
				
				if (p == OAK_REDRAW)
					bv->SetHighColor(pcolor);
				if (istag && (code == false || str->ICompare("/code") == 0 ||
					str->ICompare("br") == 0 || str->ICompare("b") == 0 ||
					str->ICompare("/b") == 0 || str->ICompare("tt") == 0 ||
					str->ICompare("/tt") == 0))
				{
				
#ifdef SHOW_TAGS
					if (p == OAK_REDRAW)
					{
						bv->MovePenTo(BPoint(i, j));
						bv->SetHighColor(0, 0, 150);
						bv->DrawString(str->String());
					}
					i += (int)font.StringWidth(str->String()) + 2;
#endif

//					str->ToLower();
					
					// for first: bold, italic etc.
					face = font.Face();
					if (face == B_REGULAR_FACE)
						face = 0;


					// for tables
					if (table_pos2 <= -1)
						td_is = false;



					if (str->ICompare("b") == 0)
					{
						face |= B_BOLD_FACE;
					}
					else if (str->ICompare("/b") == 0)
					{
						face &= !B_BOLD_FACE;
					}
					else if (str->ICompare("i") == 0)
					{
						face |= B_ITALIC_FACE;
					}
					else if (str->ICompare("/i") == 0)
					{
						face &= !B_ITALIC_FACE;
					}
					else if (str->ICompare("u") == 0)
					{
						underline = true;
						face |= B_UNDERSCORE_FACE;
					}
					else if (str->ICompare("/u") == 0)
					{
						face &= !B_UNDERSCORE_FACE;
						underline = false;
					}
					
					else if (str->ICompare("p") == 0)
					{
						if (i != aoffset)
						{
							//i = aoffset;
							//j += (int)font.Size() + 2;
							br = true;
							nextlex = false;
							//br2 = true;
						}
						else
						{
							br = true;
							/*
							while (bq_level > 0)
							{
								aoffset -= 50;
								offset -= 50;
								i -= 50;
								bq_level--;
							}*/
						}
					}

					
					// now breakin'
					else if (str->ICompare("br") == 0 ||
						 str->ICompare("br/") == 0 ||
						 str->ICompare("/br") == 0)
					{
						// adding '\n' to words
						if (n_words < MAX_WORDS)
						{
							words[n_words++] = new OakWord(BRect(i, j,
								i + 1, j + lines[line_pos]),
								"\n");
						}
						
						br = true;
					}

					// aligns: <center>, e.t.c
					else if (str->ICompare("center") == 0)
					{
						align = ALIGN_CENTER;
						br = true;
						nobr = true;
					}
					else if (str->ICompare("/center") == 0)
					{
						align = ALIGN_LEFT;
						br = true;
						//nobr = true;
					}

					// <ft:pre_tab> tag
					else if (str->ICompare("ft:pre_tab") == 0)
					{
						i += 50;
						i /= 50;
						i *= 50;
					}

					// <SCRIPT> tag
					else if (str->ICompare("script") == 0)
					{
						do
						{
							str = lexems[++lexpos];
							//lexpos++;
						} while (str->ICompare("/script") != 0 && lexpos < n_lexems);
					}

					// <IFRAME> tag
					else if (str->ICompare("iframe") == 0)
					{
						do
						{
							str = lexems[++lexpos];
							//lexpos++;
						} while (str->ICompare("/iframe") != 0 && lexpos < n_lexems);
					}


					// <ul> tag
					else if (str->ICompare("ul") == 0)
					{
						if (i != aoffset)
						{
							br = true;
							nextlex = false;
						}
						else
						{
							ul_level++;
							offset += 50;
							aoffset += 50;
							nextlex = true;
						}
					}
					else if (str->ICompare("/ul") == 0)
					{
						if (ul_level > 0)
						{
							ul_level--;
							offset -= 50;
							aoffset -= 50;
						}
					}
					else if (str->ICompare("li") == 0)
					{
						if (i != aoffset)
						{
							br = true;
							nextlex = false;
						}
						else
						{
							if (ul_level == 0 && i == aoffset)
							{
								i += 15;
							}
							if (p == OAK_REDRAW)
							{
								if (ul_level == 2)
								{
									bv->StrokeLine(BPoint(i - 10, j + lines[line_pos] / 2 - 1),
										BPoint(i - 6, j + lines[line_pos] / 2 - 1));
									bv->StrokeLine(BPoint(i - 6, j + lines[line_pos] / 2 - 1),
										BPoint(i - 8, j + lines[line_pos] / 2 + 3));
									bv->StrokeLine(BPoint(i - 10, j + lines[line_pos] / 2 - 1),
										BPoint(i - 8, j + lines[line_pos] / 2 + 3));
								}
								else if (ul_level == 3)
								{
									bv->StrokeLine(BPoint(i - 10, j + lines[line_pos] / 2 - 1),
										BPoint(i - 10, j + lines[line_pos] / 2 + 3));
									bv->StrokeLine(BPoint(i - 10, j + lines[line_pos] / 2 - 1),
										BPoint(i - 6, j + lines[line_pos] / 2 + 1));
									bv->StrokeLine(BPoint(i - 10, j + lines[line_pos] / 2 + 3),
										BPoint(i - 6, j + lines[line_pos] / 2 + 1));
								}
								else
									bv->StrokeEllipse(BRect(i - 10, j + lines[line_pos] / 2 - 1,
										i - 6, j + lines[line_pos] / 2 + 3));
							}
							nextlex = true;
						}
						//i += 10;
					}
					
					// formatting (<font> tag)
					else if (str->ICompare("font") == 0)
					{
						bfont = font;

						k = 0;
						while (k < str->col)
						{

							if (str->lex[k].ICompare("size") == 0)
							{
								if (str->val[k].ICompare("+1") == 0)
									font.SetSize(font.Size() + 2);
								else if (str->val[k].ICompare("+2") == 0)
									font.SetSize(font.Size() + 4);
								else if (str->val[k].ICompare("+3") == 0)
									font.SetSize(font.Size() + 6);
								else if (str->val[k].ICompare("+4") == 0)
									font.SetSize(font.Size() + 8);
								else if (str->val[k].ICompare("+5") == 0)
									font.SetSize(font.Size() + 10);
								else if (str->val[k].ICompare("-1") == 0)
									font.SetSize(font.Size() - 2);
								else if (str->val[k].ICompare("-2") == 0)
									font.SetSize(font.Size() - 4);
								else if (str->val[k].ICompare("-3") == 0)
									font.SetSize(font.Size() - 6);
								else if (str->val[k].ICompare("-4") == 0)
									font.SetSize(font.Size() - 8);
								else if (str->val[k].ICompare("-5") == 0)
									font.SetSize(font.Size() - 10);
								else if (str->val[k].ICompare("2") == 0)
									font.SetSize(sysfont.Size());
								else if (str->val[k].ICompare("3") == 0)
									font.SetSize(sysfont.Size() + 5);
								else if (str->val[k].ICompare("4") == 0)
									font.SetSize(sysfont.Size() + 8);
								else if (str->val[k].ICompare("5") == 0)
									font.SetSize(sysfont.Size() + 10);
								else if (str->val[k].ICompare("6") == 0)
									font.SetSize(sysfont.Size() + 14);
							}
							if (str->lex[k].ICompare("color") == 0)
							{
								if (color_level < 19)
								{
									color_level++;
									bcolor[color_level] = pcolor;
									GetRgbFromString(str->val[k], &pcolor);
								}
							}
							if (str->lex[k].ICompare("face") == 0)
							{
								if (!SetFamily(&font, str->val[k].String()))
								{
									//printf("> Error! Cannot find '%s' font :(\n",
									//	val[k].String());
								}
							}
							k++;
						}

						
						// lex: ready, value: ready;
						
					}

					else if (str->ICompare("/font") == 0)
					{
						font = bfont;
						
						if (color_level > -1)
						{ // vosstanavlivaem cvet
							pcolor = bcolor[color_level];
							color_level--;
						}
					}


					
					// formatting (<img> tag)
					else if (str->ICompare("img") == 0)
					{
						if (br_left == -1 && i != aoffset)
						{
							br = true;
							nextlex = false;
						}
						else
						{
							fname.SetTo("");
							int img_width = -1;
							int img_height = -1;
							bool border = true;
							BString alt("");
							//OakImage *oaim;
							uint32 img_align = 0;
						
							k = 0;
							while (k < str->col)
							{
								if (str->lex[k].ICompare("src") == 0)
								{
									fname = str->val[k];
								}
								else if (str->lex[k].ICompare("width") == 0)
								{
									sscanf(str->val[k].String(), "%d", &img_width);
								}
								else if (str->lex[k].ICompare("height") == 0)
								{
									sscanf(str->val[k].String(), "%d", &img_height);
								}
								else if (str->lex[k].ICompare("align") == 0)
								{
									if (str->val[k].ICompare("left") == 0)
										img_align = ALIGN_LEFT;
									else if (str->val[k].ICompare("right") == 0)
										img_align = ALIGN_RIGHT;
									//else if (str->val[k].ICompare("center") == 0)
									//	img_align = ALIGN_CENTER;
								}
								else if (str->lex[k].ICompare("alt") == 0)
								{
									alt.SetTo(str->val[k].String());
								}
								else if (str->lex[k].ICompare("border") == 0 &&
									str->val[k].ICompare("0") == 0)
								{
									border = false;
								}
								k++;
							}
							
							if (fname.Compare("/", 1) != 0)
								s.SetTo(path->String());
							else
								s.SetTo("");
							s.Append(fname);
							
							bb = NULL;
							for (k = 0; k < n_images; k++)
								if (images[k]->Name()->Compare(s.String()) == 0)
								{
									bb = images[k]->Bitmap();
									//oaim = images[k];
								}
							
							if (img_width == 0)
								img_width = -1;
							if (img_height == 0)
								img_height = -1;
							
							BRect irect;
							
							if (bb == NULL)
							{
								//printf("Image load failed ('%s').\n", s.String());
								if (img_width == -1)
									img_width = 20;
								if (img_height == -1)
									img_height = 20;


								if (img_align == ALIGN_RIGHT)
									irect = BRect(right - img_width, j,
										right - 1, j + img_height - 1);
								else if (img_align == ALIGN_CENTER)
									irect = BRect((right - offset) / 2 -
										img_width / 2, j,
										(right - offset) / 2 +
										img_width / 2 - 1, j + img_height - 1);
								else
									irect = BRect(i, j,
										i + img_width - 1, j + img_height - 1);

								
								if (alt.Length() != 0)
								{
									if (str->strw == 0)
										str->strw = (int)sysfont.StringWidth(alt.String());
									img_width += str->strw;
								}
								
								if (i != aoffset && i + img_width > right)
								{
									br = true;
									nextlex = false;
								}
								else
								{
									if (p == OAK_REDRAW)
									{
										bv->SetHighColor(255, 255, 255);
										bv->FillRect(irect);
										bv->SetHighColor(100, 100, 100);
										bv->StrokeRect(irect);
										bv->SetHighColor(200, 0, 0);
										bv->StrokeLine(BPoint(i + 2, j + 2),
											BPoint(i + 8, j + 8));
										bv->StrokeLine(BPoint(i + 8, j + 2),
											BPoint(i + 2, j + 8));
										if (alt.Length() != 0)
										{
											bv->SetHighColor(50, 50, 50);
											bv->SetLowColor(255, 255, 255);
											bv->SetFont(&sysfont);
											bv->MovePenTo(BPoint(i + 14, j + 2 + (int)sysfont.Size()));
											bv->DrawString(alt.String());
										}
									}
									nextlex = true;
								}
							}
							else
							{
							
								// ARE YOU READY?? ;-)
								if (img_width == -1)
									img_width = (int)bb->Bounds().right + 1;
								if (img_height == -1)
									img_height = (int)bb->Bounds().bottom + 1;

								if (img_align == ALIGN_RIGHT)
									irect = BRect(right - img_width, j,
										right - 1, j + img_height - 1);
								else if (img_align == ALIGN_CENTER)
									irect = BRect((right - offset) / 2 -
										img_width / 2, j,
										(right - offset) / 2 +
										img_width / 2 - 1, j + img_height - 1);
								else
									irect = BRect(i, j,
										i + img_width - 1, j + img_height - 1);

								if (i != aoffset && i + img_width > right)
								{
									br = true;
									nextlex = false;
								}
								else
								{
									if (p == OAK_REDRAW)
									{
										bv->SetDrawingMode(B_OP_ALPHA);
										bv->DrawBitmap(bb, irect);
										bv->SetDrawingMode(B_OP_COPY);
									}
									nextlex = true;
								}
							}
	
	
							if (link)
							{
								if (p == OAK_REDRAW)
								{
									if (do_links && n_links < MAX_LINKS)
									{
										links[n_links++] = new OakLink(BRect(i, j,
											i + img_width, j + img_height),
											link_href.String());
									}
									if (border)
									{
										bv->SetHighColor(lcolor);
										bv->StrokeRect(irect);
										irect.InsetBy(-1, -1);
										bv->StrokeRect(irect);
									}
								}
							}
							
							//br_left = offset;
							//offset = i;
							//aoffset = offset;
							//j += img_height;
							if (img_align == ALIGN_LEFT)
							{
								i += img_width - 1;
								br_left_to = j + img_height - 1;
								br_left = i;
							}
							else if (img_align == ALIGN_RIGHT)
							{
								br_right_to = j + img_height - 1;
								br_right = img_width;
							}
							else
							{
								i += img_width - 1;
								br_left_to = j + img_height - 1;
								br_left = i;
								br_right = -1;
								br_right_to = -1;
								br_reset = true;
								//i += img_width;
								//t_img_height = img_height;
								//j += img_height;
								//t_end = true;
							}
							
							/*if (img_height > lines[line_pos] + 2 &&
								img_height > font.Size() + 2)
							{
								t_end = true;
							}
							else
								j -= lines[line_pos];*/
							
							//br = true;
							//nobr = true;
							nextlex = true;
						}
					}



					// formatting (<spacer> tag)
					else if (str->ICompare("spacer") == 0)
					{
						k = 0;
						int spwidth = 0;
						while (k < str->col)
						{
							if (str->lex[k].ICompare("width") == 0)
							{
								sscanf(str->val[k].String(), "%d", &spwidth);
							}
							k++;
						}
						i += spwidth;
					}


					// formatting (<a> tag)
					else if (str->ICompare("a") == 0)
					{
						link_href.SetTo("");
					
						k = 0;
						while (k < str->col)
						{
							if (str->lex[k].ICompare("href") == 0)
							{
								link = true;
								link_href = str->val[k];
								link_color_level = color_level;
							}
							else if (str->lex[k].ICompare("name") == 0)
							{
								if (n_anchors < MAX_ANCHORS)
								{
									str->val[k].ReplaceAll("%20", " ");
									anchors[n_anchors++] = new OakAnchor(
										str->val[k].String(),
										j);
								}
							}
							k++;
						}
						
						if (link_href.Compare("http://", 7) != 0 &&
							link_href.Compare("/", 1) != 0 &&
							link_href.Compare("#", 1) != 0)
							link_href.Prepend(path->String());
					}
					else if (str->ICompare("/a") == 0 && link)
					{
						link = false;
					}
					
					
					// TABLES
					
					else if (str->ICompare("table") == 0)
					{
						br_left = -1;
						br_right = -1;
						br_left_to = -1;
						br_right_to = -1;
						br_reset = false;
						if (i != aoffset)
						{
							br = true;
							nextlex = false;
						}
						else
						{	
							table_pos2++;
						
							table_number[table_pos2] = table_pos++;

							if (table_pos2 > -1)
								TABLE = tables[table_number[table_pos2]];
							
							TABLE->offset = offset;
							TABLE->aoffset = aoffset;
							TABLE->right = right;
							TABLE->align = align;
							
							if (table_pos2 > 0)
							{
								OakTable *TT = tables[table_number[table_pos2 - 1]];
								if (TT->td[TT->td_pos]->is_bgcolor)
								{
									TABLE->pbgcolor.red = TT->td[TT->td_pos]->bgcolor.red;
									TABLE->pbgcolor.green = TT->td[TT->td_pos]->bgcolor.green;
									TABLE->pbgcolor.blue = TT->td[TT->td_pos]->bgcolor.blue;
								}
								else if (TT->is_bgcolor)
								{
									TABLE->pbgcolor.red = TT->bgcolor.red;
									TABLE->pbgcolor.green = TT->bgcolor.green;
									TABLE->pbgcolor.blue = TT->bgcolor.blue;
								}
								else
								{
									TABLE->pbgcolor.red = bgcolor.red;
									TABLE->pbgcolor.green = bgcolor.green;
									TABLE->pbgcolor.blue = bgcolor.blue;
								}
							}
							else
							{
								TABLE->pbgcolor.red = bgcolor.red;
								TABLE->pbgcolor.green = bgcolor.green;
								TABLE->pbgcolor.blue = bgcolor.blue;
							}
							
							if (p == OAK_LINES)
							{
								TABLE->line = n_lines;
								if (lines[n_lines] <= 0)
									lines[n_lines] = (int)font.Size();
							}
							else
								;
							
							br = true;
							
							TABLE->cellpadding = cellpadding;
							TABLE->td_pos = -1;
							TABLE->tr_pos = -1;
							
							cellpadding = TABLE->cellp;
							if (cellpadding < 0 || cellpadding > 50)
								cellpadding = CELLPADDING;
							
							//printf("> Building table with offset '%d'...\n", aoffset);
							
							printf("TABLE #%d:.................\n", table_number[table_pos2]);
							printf(" - - cols:%d, rows:%d\n", TABLE->cols, TABLE->rows);
							for (k = 0; k < TABLE->tds; k++)
							{
								TD = TABLE->td[k];
								printf("TD: col=%d, row=%d, colspan=%d, rowspan=%d\n",
									TD->col, TD->row, TD->colspan, TD->rowspan);
							}
							printf("\n\n\n");
							
							
							static int twidth;
							
							if (p == OAK_TABLES)
							{
								TABLE->created = false;
							}
							
							if (p != OAK_REDRAW && !TABLE->created)
							{
								if (TABLE->width == 0)
								{
									twidth = right - aoffset;
								}
								else
								{
									twidth = TABLE->width;
									if (TABLE->widthp)
									{
										twidth *= (right - offset);
										twidth /= 100;
									}
									if (twidth > right - aoffset)
										twidth = right - aoffset;
								}
								
								/*if (TABLE->cols > 0)
									tdw = (twidth) /
										TABLE->cols;
								else
									tdw = twidth;*/
									
								trh = cellpadding * 2;
								
								
								for (k = 0; k <= TABLE->cols; k++)
									TABLE->colposes[k] = 0;


								TABLE->colposes[0] = i;
								int tdwidths[MAX_COLS + 1];
								for (k = 1; k <= TABLE->cols; k++)
									tdwidths[k] = 0;

								if (TABLE->cols > 1)
									for (k = 0; k < TABLE->tds; k++)
									{
										TD = TABLE->td[k];
										if (TD->widthp && TABLE->cols > TD->colspan &&
											TD->width == 100)
										{
											for (q = 0; q < TABLE->tds; q++)
												if (TABLE->td[q]->width == 100 &&
													TABLE->cols > TABLE->td[q]->colspan &&
													TABLE->td[q]->widthp)
													TABLE->td[q]->width = -1;
												else
													TABLE->td[q]->width = 0;
											k = TABLE->tds;
										}
									}


								for (k = 0; k < TABLE->tds; k++)
								{
									TD = TABLE->td[k];
									if (TD->widthp && TABLE->cols > TD->colspan &&
										TD->width == 100)
										TD->width = -1;
									if (TD->width != -1)
									{
										if (TD->widthp)
										{
											TABLE->colposes[TD->col + TD->colspan] =
												TABLE->colposes[TD->col] + TD->width * twidth / 100;
											tdwidths[TD->col + 1] = TD->width * twidth / 100;
										}
										else
										{
											TABLE->colposes[TD->col + TD->colspan] =
												TABLE->colposes[TD->col] + TD->width;
											tdwidths[TD->col + 1] = TD->width;
										}
									}
								}
								
								static int col_autos;
								col_autos = 0;
								
								int mytwidth = twidth;
								
								for (k = 1; k <= TABLE->cols; k++)
									if (TABLE->colposes[k] == 0)
										col_autos++;
									else
										twidth -= tdwidths[k];
										
								if (twidth < 0)
									twidth = 0;

								if (!TABLE->canresize)
									TABLE->colposes[TABLE->cols] = i + mytwidth;
										
								tdw = 0;
								if (col_autos > 0)
									tdw = twidth / col_autos;
								printf("TDW:%d\n", tdw);

								printf("COL_AUTOS:%d\n", col_autos);
									
								for (k = 1; k <= TABLE->cols; k++)
									if (TABLE->colposes[k] == 0)
										TABLE->colposes[k] =
											TABLE->colposes[k - 1] + tdw;

								printf("T: ");
								for (k = 0; k <= TABLE->cols; k++)
									printf("%d ", TABLE->colposes[k]);
								printf("\n");
								
								for (k = 0; k <= TABLE->rows; k++)
									TABLE->rowposes[k] = j + trh * k;
								for (k = 0; k <= TABLE->cols; k++)
									TABLE->colmaxes[k] = 0;
								//printf("TOP:%d\n", TABLE->rowposes[0]);
								
								TABLE->created = true;
							}
							else if (p != OAK_REDRAW)
							{
								int dx = (int)TABLE->rect().left - i;
								if (dx != 0)
									for (k = 0; k <= TABLE->cols; k++)
										TABLE->colposes[k] -= dx;

								trh = cellpadding * 2;
								for (k = 0; k <= TABLE->rows; k++)
									TABLE->rowposes[k] = j + trh * k;

							}

							nextlex = true;
						}
					}
					else if (table_pos2 > -1 &&
						str->ICompare("tr") == 0)
					{
						br_left = -1;
						br_right = -1;
						br_left_to = -1;
						br_right_to = -1;
						TABLE->tr_pos++;
					}
					else if (table_pos2 > -1 &&
						str->ICompare("td") == 0)
					{
						TABLE->td_pos++;
						
						br_left = -1;
						br_right = -1;
						br_left_to = -1;
						br_right_to = -1;
						
						if (p == OAK_LINES)
						{
							n_lines++;
						}
						else
							line_pos++;
							
						if (TABLE->tr_pos == -1)
							TABLE->tr_pos = 0;

						if (TABLE->td_pos > 0)
						{
							TD = TABLE->td[TABLE->td_pos - 1];
							if (p == OAK_TABLES && TD->canresize &&
								TABLE->td_pos != -1 &&
								TD->row + TD->rowspan == TABLE->rows)
								TABLE->CheckMax(TD->col);
						}
						font = sysfont;
						font.SetSize(FONT_SIZE);

						TD = TABLE->td[TABLE->td_pos];
						
						td_is = true;

						offset = (int)TABLE->rect(TABLE->td_pos).left + cellpadding;
						aoffset = offset;
							
						right = (int)TABLE->rect(TABLE->td_pos).right - cellpadding;
						if (right > 10000)
							right = 1000;
							
						i = aoffset;
						j = (int)TABLE->rect(TABLE->td_pos).top + cellpadding;
						
						align = TD->align;
						br = true;
						nobr = true;
					}
					else if (table_pos2 > -1 &&
						str->ICompare("/td") == 0)
					{
					}
					else if (table_pos2 > -1 &&
						str->ICompare("/tr") == 0)
					{
					}
					else if (table_pos2 > -1 &&
						str->ICompare("/table") == 0)
					{

						if (p == OAK_LINES)
						{
							if ((int)font.Size() > lines[n_lines])
								lines[n_lines] = (int)font.Size();
							if (i - offset > linews[n_lines])
								linews[n_lines] = i - offset;
							n_lines++;
						}
						else
							line_pos++;

						if (TABLE->tds > 0)
						{
							TD = TABLE->td[TABLE->tds - 1];
							if (p == OAK_TABLES && TD->canresize &&
								TABLE->td_pos != -1 &&
								TD->row + TD->rowspan == TABLE->rows)
								TABLE->CheckMax(TD->col);
						}

						/*if (p == OAK_TABLES && TABLE->tds > 0 &&
							TABLE->td[TABLE->tds - 1]->canresize)
							TABLE->CheckMax(TABLE->cols);*/

						i = (int)TABLE->rect().right;
						j = (int)TABLE->rect().bottom;
						
						if (p == OAK_LINES)
							linews[TABLE->line] = i;
						
						offset = TABLE->offset;
						aoffset = TABLE->aoffset;
						right = TABLE->right;
						align = TABLE->align;
						cellpadding = TABLE->cellpadding;
						
						if (table_pos2 > -1)
							table_pos2--;

						if (table_pos2 > -1)
						{
							TABLE = tables[table_number[table_pos2]];
							TD = TABLE->td[TABLE->td_pos];

						}
						
						t_end = true;
					}
					
					// <hr>
					else if (str->ICompare("hr") == 0)
					{
						rgb_color hrcolor = pcolor;
						
						if (i != aoffset)
						{
							br = true;
							nextlex = false;
						}
						else
						{
							k = 0;
							while (k < str->col)
							{
	
								if (str->lex[k].ICompare("color") == 0)
								{
									GetRgbFromString(str->val[k], &hrcolor);
								}
								k++;
							}
							
							if (p == OAK_REDRAW)
							{
								bv->SetHighColor(hrcolor);
								bv->StrokeLine(BPoint(offset, j + 5),
									BPoint(right, j + 5));
								bv->StrokeLine(BPoint(offset, j + 6),
									BPoint(right, j + 6));
							}
							i = aoffset;
							br = true;
							t_end = true;
							//nextlex = true;
						}
					}
					
					// <h1>
					else if (str->ICompare("h1") == 0)
					{
						hfonts[h_level++] = font;
						font.SetSize(24);
						br = true;
					}
					else if (str->ICompare("h2") == 0)
					{
						hfonts[h_level++] = font;
						font.SetSize(19);
						br = true;
					}
					else if (str->ICompare("h3") == 0)
					{
						hfonts[h_level++] = font;
						font.SetSize(16);
						br = true;
					}
					else if (str->ICompare("h4") == 0)
					{
						hfonts[h_level++] = font;
						font.SetSize(14);
						br = true;
					}
					else if (str->ICompare("/h1") == 0 || str->ICompare("/h2") == 0 ||
						str->ICompare("/h3") == 0 || str->ICompare("/h4") == 0)
					{
						if (h_level > 0)
						{
							h_level--;
							font = hfonts[h_level];
							//printf("> h_Size = %d;\n", (int)hfonts[h_level].Size());
							//font.SetSize(12);
						}
						br = true;
					}

					// <pre>
					else if (str->ICompare("pre") == 0)
					{
						pre = true;
						prefont = font;
						font = be_fixed_font;
					}
					else if (str->ICompare("/pre") == 0)
					{
						pre = false;
						font = prefont;
					}

					else if (str->ICompare("sup") == 0)
					{
						sup = true;
					}
					else if (str->ICompare("/sup") == 0)
					{
						sup = false;
					}

					// <tt>
					else if (str->ICompare("tt") == 0)
					{
						tt = true;
						ttfont = font;
						int ttsize = (int)font.Size();
						//printf("> TT Font Size: %d\n", ttsize);
						font = be_fixed_font;
						font.SetSize(ttsize);
						//printf("> TT Font Size after: %d\n", (int)font.Size());
					}
					else if (str->ICompare("/tt") == 0)
					{
						tt = false;
						font = ttfont;
						if (font.Face() == be_fixed_font->Face())
							font = sysfont;
					}

					else if (str->ICompare("blockquote") == 0)
					{
						if (i != aoffset)
						{
							br = true;
							nextlex = false;
						}
						else
						{
							bq_level++;
							offset += 50;
							aoffset += 50;
							i += 50;
							nextlex = true;
						}
					}
					else if (str->ICompare("/blockquote") == 0)
					{
						if (bq_level > 0)
						{
							offset -= 50;
							aoffset -= 50;
							i -= 50;
							bq_level--;
						}
					}

					// oh uzh eti Be Inc. BLYA
					if (!pre && !tt && font.Spacing() == be_fixed_font->Spacing())
					{
						font = sysfont;
						//font.SetSize(FONT_SIZE);
					}

					// it is for fonts
					if (face == 0)
						face = B_REGULAR_FACE;
					font.SetFace(face);


					if (p == OAK_REDRAW)
						bv->SetFont(&font);
				}
				else
				{
					ss.SetTo(*str);
					
					if (i == aoffset && ss.ByteAt(0) == ' ')
						ss.RemoveFirst(" ");
						
					ss.ReplaceAll("&amp;", "&");
					ss.ReplaceAll("&nbsp;", " ");
					ss.ReplaceAll("&nbsp", " ");
					ss.ReplaceAll("&lt;", "<");
					ss.ReplaceAll("&gt;", ">");
					ss.ReplaceAll("&quot;", "\"");
					ss.ReplaceAll("&ndash;", " ");
					ss.ReplaceAll("&oslash;", "ø");
					ss.ReplaceAll("&copy;", "©");
					ss.ReplaceAll("&reg;", "®");
					ss.ReplaceAll("&raquo;", "»");
					ss.ReplaceAll("&mdash;", "—");
					
					/*static char s_symbols[7], s_with[2];
					for (k = 0; k < 256; k++)
					{
						sprintf(s_symbols, "&#%d;", k);
						sprintf(s_with, "%c", (char)k);
						ss.ReplaceAll(s_symbols, s_with);
					}*/
					
					// udalyaem probely v nachale stroki
					//if (i == aoffset && str->ByteAt(0) == ' ')
					//	str->ReplaceFirst(" ", "");
					
					
					if (str->strw == 0)
					{
						strw = str->strw = (int)font.StringWidth(ss.String());
					}
					else
						strw = str->strw;
					
					autobr = false;

					if (strw != 0 && (i + strw >
						right - (br_right == -1 ? 0 : br_right)) && nextlex2 && i != aoffset)
					{
						br = true;
						autobr = true;
						nextlex = false;
						newp = false;
					}
					else if (t_end && i != aoffset)
					{
						br = true;
						nextlex = false;
					}
					else
					{
						if (link)
						{
							if (p == OAK_REDRAW)
							{
								if (color_level == link_color_level)
									bv->SetHighColor(lcolor);
								if (do_links && n_links < MAX_LINKS)
								{
									links[n_links++] = new OakLink(BRect(i, j,
										i + strw, j + lines[line_pos]),
										link_href.String());
								}
							}
						}
						if (p == OAK_REDRAW)
						{
							if (do_words && n_words < MAX_WORDS)
							{
								words[n_words++] = new OakWord(BRect(i, j,
									i + strw, j + lines[line_pos]),
									ss.String());
							}
						}

						if (p == OAK_REDRAW)
						{
							bv->SetLowColor(bgcolor);
							if (table_pos2 != -1)
							{
								bv->SetLowColor(tables[table_number[table_pos2]]->pbgcolor);
								if (tables[table_number[table_pos2]]->is_bgcolor)
								{
									bv->SetLowColor(tables[table_number[table_pos2]]->bgcolor);
								}
							}
									
							if (table_pos2 != -1 && TD != NULL && TD->is_bgcolor)
								bv->SetLowColor(TD->bgcolor);
						
							if (!sup)
							{
								bv->MovePenTo(BPoint(i, j + lines[line_pos]));
								bv->DrawString(ss.String());
							}
							else
							{
								float fsize = font.Size();
								font.SetSize(font.Size() / 2 + 2);
								bv->SetFont(&font);
								bv->MovePenTo(BPoint(i, j + lines[line_pos] / 2));
								bv->DrawString(ss.String());
								font.SetSize(fsize);
							}

							if (link || (font.Face() & B_UNDERSCORE_FACE) != 0 || underline)
							{
								bv->MovePenTo(BPoint(i, j + lines[line_pos] + 1));
								bv->StrokeLine(
									BPoint(i + strw,
									j + lines[line_pos] + 1));
							}
						}
						i += strw;
						nextlex = true;

						if (p == OAK_LINES)
						{
							if (i - offset > linews[n_lines])
								linews[n_lines] = i - offset;
						}

					}

					t_end = false;
				}
				//m++;

				
				if (p != OAK_REDRAW && table_pos2 != -1)
				{
					TABLE->CheckWidth(i + cellpadding, &right, aoffset - offset,
						TABLE->right);

				}

				if (p != OAK_REDRAW && table_pos2 != -1)
				{
					TABLE->ResizeRow(j + cellpadding);
					if (br_left_to != -1)
						TABLE->ResizeRow(br_left_to + cellpadding);
					if (!t_end && i != aoffset)
						TABLE->ResizeRow(j + lines[line_pos] + cellpadding);
				}
		
				if (p == OAK_LINES)
				{
					if ((int)font.Size() > lines[n_lines])
						lines[n_lines] = (int)font.Size();
					if (i - offset > linews[n_lines])
						linews[n_lines] = i - offset;
				}
				

				/*if (br_offset != -1 && j >= br_height)
				{
					offset = br_offset;
					br_offset = -1;
					br_height = 0;
				}*/
				
				if (i + cellpadding > hmax)
				{
					//printf("i: %d\n", i);
					hmax = i + cellpadding;
				}
				
				if (i > maxr)
					maxr = i + cellpadding;

				if (nextlex || (!nextlex && !nextlex2))
					lexpos++;
					
				nextlex2 = nextlex; // chtoby ne bylo podvisona!
			}

			if (!nobr)
			{
	
				
				/*if (!autobr && br_offset != -1)
				{
					offset = br_offset;
					br_offset = -1;
					j = br_height;
				}*/
				
				if (p != OAK_REDRAW)
				{
					j += lines[line_pos] + 2;
					//j += (int)font.Size() + 2;
				}
				else
				{
					//j += (int)font.Size() + 2;
					j += lines[line_pos] + 2;
					
				}
	
				if (p == OAK_LINES)
				{
					if (lines[n_lines] <= 0)
						lines[n_lines] = (int)font.Size();
					//printf("> LINEZ[%d]=%d\n", n_lines, lines[n_lines]);
					n_lines++;
				}
				else
				{
					line_pos++;
				}
				
				//j += 200;
				
				if (p != OAK_REDRAW && table_pos2 > -1 && TABLE->tr_pos > -1)
				{
					TABLE->ResizeRow(j + cellpadding);
					if (TABLE->tds > 0)
						TABLE->td[TABLE->td_pos]->lines = true;
				}
	
				//m++;
				//while (*m == '\n' || *m == '\r' || *m == '\t')
				//	m++;
				//if (catch_m == m)
				//	m++;
			}
		}
		
		j += cellpadding;
		
		if (p == OAK_FINDVMAX)
		{
			vmax = j;
			if (vmax < (int)Parent()->Bounds().bottom &&
				(papa->IsScroll() || papa->IsStatus()))
				vmax = (int)Parent()->Bounds().bottom - (int)B_H_SCROLL_BAR_HEIGHT;
			else if (vmax < (int)Parent()->Bounds().bottom)
			{
				vmax = (int)Parent()->Bounds().bottom;
			}
			printf("> OakView:: VMax found!\n");
		}

		if (p == OAK_REDRAW)
		{
			printf("trying to unlock\n");
			bv->UnlockLooper();
			printf("unlocked\n");
			if (do_buffer)
			{
				cache->RemoveChild(bv);
				delete bv;
		
				MovePenTo(BPoint(0, 0));
				DrawBitmap(cache, BPoint(0, 0));
			}
		}
	}
	//delete bb;
}

void OakView::Draw( BRect ur )
{
	if (cache == NULL)
		MyDraw(OAK_REDRAW);
	else
		MyDraw();
}

void OakView::Process()
{
	char *mas, *m, *catch_m;
	bool istag, settitle = false, setbody = false, pre = false, isme = false;
	BString str, flname, lex[10], val[10], s, anchor;
	//BDirectory dir;
	BPath pth;
	int i, k, col;
	BString fn2, type;
	bool tdbegin = false;
	OakTable *TABLE = NULL;
	int pred_cols = 0;

	bool sethead = false;

	// selecting:
	select_is = false;
	select_processed = false;
	select_from = 0;
	select_to = 0;
	
	searched = -1;
	
	n_tables = 0;
	int table_pos2 = -1;
	int table_number[20];
	
	vl_tables = -1;
	
	for (i = 0; i < n_lexems; i++)
		delete lexems[i];
	n_lexems = 0;
	
	title->SetTo("");
	body->SetTo("");
	GetRgbFromString("#0000ff", &lcolor);
	GetRgbFromString("#000000", &pcolor);
	GetRgbFromString("#ffffff", &bgcolor);
	
	n_lines = 0;
	
	for (i = 0; i < n_tables; i++)
		delete tables[i];
	n_tables = 0;
	
	if (!isme)
	{
		// delete all images
		for (i = 0; i < n_images; i++)
			delete images[i];
		n_images = 0;
		if (bg != NULL)
		{
			delete bg;
			bg = NULL;
		}
	}

	if (html->Length() > 0)
	{
		// koe-chto izmenim :)
		html->IReplaceAll("<th>", "<td>");
		html->IReplaceAll("<th ", "<td ");
		html->IReplaceAll("</th>", "</td>");
	
		mas = (char *)malloc(html->Length() + 1);
		memcpy(mas, html->String(), html->Length());
		mas[html->Length()] = 0;
		m = mas;
		
		void *mpos = m + html->Length() - 1;
		
		bool wasprobel = false;
		bool saveprobel = false;
		
		while (*m != 0 && m <= mpos)
		{
			catch_m = m;
			str.SetTo("");

			/*if (*m == ' ')
			{
				str.Append(" ");
				m++;
			}*/
			
			wasprobel = false;
			if (!pre)
			{
				while (*m == ' ')
				{
					if (wasprobel == false)
					{
						str.Append(" ");
						wasprobel = true;
					}
					m++;
				}
			}
			else
				while (*m == ' ')
				{
					str.Append(" ");
					m++;
				}
				
			if (str.Compare(" ") == 0 && n_lexems != 0 && lexems[n_lexems - 1]->is_tag &&
				lexems[n_lexems - 1]->ICompare("table") != 0 &&
				lexems[n_lexems - 1]->ICompare("/table") != 0 &&
				lexems[n_lexems - 1]->ICompare("td") != 0 &&
				lexems[n_lexems - 1]->ICompare("/td") != 0 &&
				lexems[n_lexems - 1]->ICompare("/tr") != 0 &&
				lexems[n_lexems - 1]->ICompare("tr") != 0)
				saveprobel = true;
			else
				saveprobel = false;
				
			if (*m == '<')
			{
				int nl = n_lexems - 1;
				while (nl >= 0 && lexems[nl]->ByteAt(0) == '<')
					nl--;
				
				//if (nl >= 0 && lexems[nl]->ByteAt(lexems[nl]->Length() - 1) != ' ')
				//	lexems[nl]->Append(" ");
				
				//if (n_lexems < MAX_LEXEMS)
				//	lexems[n_lexems++] = new BString(str);
				//str.ReplaceFirst(" ", "");
			}


			while ((*m == '\n' && !pre) || *m == '\r' || *m == '\t')
				m++;

			if (saveprobel == false && *m == '<')
			{
				str.SetTo("");
				while (m <= mpos && *m && *m != '>')
				{
					if (*m == '\n' || *m == '\r')
						*m = ' ';
					str.Append(*m, 1);
					m++;
				}
				if (*m == '>')
				{
					str.Append(*m, 1);
					m++;
				}
				istag = true;
			}
			else
			{
				while (*m && *m != '\n' && *m != '\r' && *m != ' ' &&
					*m != '\t' && *m != '<' && m <= mpos)
				{
					str.Append(*m, 1);
					m++;
				}
				if (*m == ' ' && m <= mpos)
				{
					str.Append(*m, 1);
					m++;
				}
				if ((*m == '\n' || *m == '\t') && pre && m <= mpos)
				{
					str.Append(*m, 1);
					m++;
				}
				istag = false;
			}
			
			//printf("parsing (%s) lexem\n", str.String());
			
			if (istag)
			{
				//str.ToLower();
				
				if (str.ICompare("<!--", 4) == 0)
				{
					if (str.FindLast("-->") == -1)
					{
						while (*m && !(*m == '>' && *(m - 1) == '-' && *(m - 2) == '-'))
							m++;
						m++;
					}
				}

				if (str.ICompare("<title>") == 0)
				{
					settitle = true;
				}
				else if (str.ICompare("</title>") == 0)
				{
					settitle = false;
				}

				if (str.ICompare("<pre>") == 0)
				{
					pre = true;
				}
				else if (str.ICompare("</pre>") == 0)
				{
					pre = false;
				}


				if (str.ICompare("<body", 5) == 0)
				{
					setbody = true;
				}
				else if (str.ICompare("</body>") == 0)
				{
					setbody = false;
				}


				// <head> tag
				if (str.ICompare("<head ", 6) == 0 || str.ICompare("<head>") == 0)
				{
					sethead = true;
				}
				else if (str.ICompare("</head>") == 0)
				{
					sethead = false;
				}


				// parsing <BODY> tag
				if (str.ICompare("<body ", 6) == 0 || str.ICompare("<body>", 6) == 0)
				{
					sethead = false;
					ParseTag(str, lex, val, &col);
					GetRgbFromString("#c0c0c0", &bgcolor);
					k = 0;
					while (k < col)
					{
						if (do_pictures && !isme && lex[k].ICompare("background") == 0)
						{
							if (val[k].Compare("/", 1) != 0 &&
								val[k].Compare("http://", 7) != 0)
								val[k].Prepend(path->String());
							printf("> Add Image: %s\n", val[k].String());
							bg = new OakImage(val[k].String());
							if (bg == NULL || bg->Bitmap() == NULL)
							{
								//delete bg;
							}
						}
						if (lex[k].ICompare("bgcolor") == 0)
						{
							GetRgbFromString(val[k], &bgcolor);
						}
						if (lex[k].ICompare("text") == 0)
						{
							GetRgbFromString(val[k], &pcolor);
						}
						if (lex[k].ICompare("link") == 0)
						{
							GetRgbFromString(val[k], &lcolor);
						}
						k++;
					}
				}


				// preformatting (<img> tag)
				if (str.ICompare("<img", 4) == 0)
				{
					flname.SetTo("");
				
					ParseTag(str, lex, val, &col);
					k = 0;
					while (k < col)
					{
						if (lex[k].ICompare("src") == 0)
						{
							flname = val[k];
						}
						k++;
					}
					
					if (!isme)
					{
						if (flname.Compare("/", 1) != 0)
							s.SetTo(path->String());
						else
							s.SetTo("");
						s.Append(flname);
						
						if (do_pictures)
						{
							images[n_images] = new OakImage(s.String());
							//printf("new...\n");
							if (images[n_images]->Bitmap() == NULL)
							{
								delete images[n_images];
							}
							else
								n_images++;
						}
					}
				}

				// TABLES
				
				if (table_pos2 > -1)
					TABLE = tables[table_number[table_pos2]];
				
				if (str.ICompare("<table ", 7) == 0 || str.ICompare("<table>") == 0)
				{
					pred_cols = 0;
					
					if (n_tables > MAX_TABLES)
						printf("> Karaul! tablic ne hvataet :(\n");
					table_pos2++;
	
					table_number[table_pos2] = n_tables++;
					tables[table_number[table_pos2]] = new OakTable();

					if (table_pos2 > -1)
						TABLE = tables[table_number[table_pos2]];

					TABLE->cellp = CELLPADDING;
					for (k = 0; k < MAX_COLS; k++)
					{
						TABLE->tdwidths[k] = 0;
						TABLE->tdcanresize[k] = true;
					}

					ParseTag(str, lex, val, &col);
					k = 0;
					while (k < col)
					{
						if (lex[k].ICompare("width") == 0)
						{
							if (val[k].ByteAt(val[k].Length() - 1) != '%')
							{
								TABLE->widthp = false;
							}
							else
							{
								TABLE->widthp = true;
								val[k].ReplaceLast("%", "");
							}
							sscanf(val[k].String(), "%d", 
								&TABLE->width);
							TABLE->canresize = false;
						}
						if (lex[k].ICompare("border") == 0)
						{
							if (val[k].Compare("0") != 0)
								TABLE->border = true;
						}
						else if (lex[k].ICompare("cellpadding") == 0)
						{
							sscanf(val[k].String(), "%d", 
								&TABLE->cellp);
						}
						else if (lex[k].ICompare("bgcolor") == 0)
						{
							printf("> c: %s\n", val[k].String());
							GetRgbFromString(val[k],
								&TABLE->bgcolor);
							TABLE->is_bgcolor = true;
						}
						k++;
					}
					
					tdbegin = false;
				}
				else if (str.ICompare("<tr ", 3) == 0 || str.ICompare("<tr>") == 0 ||
					str.ICompare("<th ", 3) == 0 || str.ICompare("<th>") == 0)
				{

					if (tdbegin)
					{
						if (n_lexems < MAX_LEXEMS)
							lexems[n_lexems] = new OakLexem("</td>");
						n_lexems++;
					}

					if (TABLE->rows > -1 && pred_cols < TABLE->cols)
						pred_cols = TABLE->cols;
					TABLE->cols = 0;
					TABLE->rows++;
				}
				else if (str.ICompare("<td ", 3) == 0 || str.ICompare("<td>") == 0)
				{
					if (TABLE->rows == 0)
						TABLE->rows++;
					
					if (tdbegin)
					{
						if (n_lexems < MAX_LEXEMS)
							lexems[n_lexems] = new OakLexem("</td>");
						n_lexems++;
					}
					
					TABLE->td[TABLE->tds] = new OakTD();
					
					ParseTag(str, lex, val, &col);
					k = 0;
					while (k < col)
					{
						if (lex[k].ICompare("width") == 0)
						{
							if (val[k].ByteAt(val[k].Length() - 1) != '%')
							{
								TABLE->td[TABLE->tds]->widthp = false;
							}
							else
							{
								TABLE->td[TABLE->tds]->widthp = true;
								val[k].ReplaceLast("%", "");
							}
							TABLE->td[TABLE->tds]->width = -1;
							sscanf(val[k].String(), "%d", 
								&TABLE->td[TABLE->tds]->width);
							TABLE->td[TABLE->tds]->canresize = false;
						}
						else if (lex[k].ICompare("colspan") == 0)
						{
							sscanf(val[k].String(), "%d", 
								&TABLE->td[TABLE->tds]->colspan);
							if (TABLE->td[TABLE->tds]->colspan <= 0)
								TABLE->td[TABLE->tds]->colspan = 1;
						}
						else if (lex[k].ICompare("rowspan") == 0)
						{
							sscanf(val[k].String(), "%d", 
								&TABLE->td[TABLE->tds]->rowspan);
							if (TABLE->td[TABLE->tds]->rowspan <= 0)
								TABLE->td[TABLE->tds]->rowspan = 1;
						}
						else if (lex[k].ICompare("align") == 0)
						{
							if (val[k].ICompare("left") == 0)
								TABLE->td[TABLE->tds]->align = ALIGN_LEFT;
							else if (val[k].ICompare("right") == 0)
								TABLE->td[TABLE->tds]->align = ALIGN_RIGHT;
							else if (val[k].ICompare("center") == 0)
								TABLE->td[TABLE->tds]->align = ALIGN_CENTER;
						}
						else if (lex[k].ICompare("bgcolor") == 0)
						{
							GetRgbFromString(val[k],
								&TABLE->td[TABLE->tds]->bgcolor);
							TABLE->td[TABLE->tds]->is_bgcolor = true;
						}
						k++;
					}
					
					tdbegin = true;
					
					// for ROWSPAN
					static bool ismycol;
					k = TABLE->tds - 1;
					while (k >= 0)
					{
						//printf("K: %d\n", k);
						ismycol = true;
						for (k = TABLE->tds - 1; k >= 0 && ismycol; k--)
						{
							//printf("K2: %d\n", k);

							if (TABLE->td[k]->col == TABLE->cols &&
								TABLE->td[k]->rowspan >= TABLE->rows - TABLE->td[k]->row)
								{
									ismycol = false;
									TABLE->cols++;
								}
						}
					}
					
					for (k = 0; k < TABLE->tds; k++)
						if (TABLE->td[k]->col == TABLE->cols && TABLE->td[k]->width != -1)
							TABLE->td[TABLE->tds]->canresize = false;
	
	
					TABLE->td[TABLE->tds]->col = TABLE->cols;
					TABLE->td[TABLE->tds]->row = TABLE->rows - 1;

					TABLE->cols += TABLE->td[TABLE->tds]->colspan;

					if (!TABLE->td[TABLE->tds]->canresize)
						TABLE->tdcanresize[TABLE->cols - 1] = false;

					TABLE->tds++;
				}
				else if (str.ICompare("</td>", 4) == 0)
				{
					tdbegin = false;
				}
				else if (str.ICompare("</tr>", 4) == 0)
				{
					if (tdbegin)
					{
						if (n_lexems < MAX_LEXEMS)
						{
							lexems[n_lexems] = new OakLexem("</td>");
							tdbegin = false;
						}
						n_lexems++;
					}
				}
				else if (table_pos2 > -1 && str.ICompare("</table>") == 0)
				{
					if (tdbegin)
					{
						if (n_lexems < MAX_LEXEMS)
						{
							lexems[n_lexems] = new OakLexem("</td>");
							tdbegin = false;
						}
						n_lexems++;
					}
				
					if (TABLE->cols == 0)
						TABLE->cols = 1;
						
					if (pred_cols > TABLE->cols)
						TABLE->cols = pred_cols;

					//printf("> cols: %d\n", TABLE->cols);
					//n_tables++;

					table_pos2--;

					if (table_pos2 > -1)
						TABLE = tables[table_number[table_pos2]];

				}

				
				if (table_pos2 > vl_tables)
					vl_tables = table_pos2;

				

			}
			else
			{
				if (settitle)
				{
					title->Append(str);
				}

			}

			if (!sethead && str.Length() != 0)
			{
				if (!pre || str.ICompare("<pre>") == 0)
				{
					str.ReplaceAll("\n", " ");
					str.ReplaceAll("\r", " ");
					str.ReplaceAll("\t", " ");
					if (n_lexems < MAX_LEXEMS)
						lexems[n_lexems++] = new OakLexem(str.String());
				}
				else
				{
					//if (str.FindFirst("\n") == 0);
					//	lexems[n_lexems++] = new BString("<br>");
					if (n_lexems < MAX_LEXEMS)
						lexems[n_lexems] = new OakLexem(str.String());
					if (str.FindFirst("\n") == str.Length() - 1)
					{
						lexems[n_lexems]->ReplaceAll("\n", "");
						n_lexems++;
						if (n_lexems < MAX_LEXEMS)
						{
							lexems[n_lexems] = new OakLexem("<br>");
							n_lexems++;
						}
					}
					else if (str.FindLast("\t") == str.Length() - 1)
					{
						lexems[n_lexems]->ReplaceAll("\t", "");
						n_lexems++;
						if (n_lexems < MAX_LEXEMS)
						{
							lexems[n_lexems] = new OakLexem("<ft:pre_tab>");
							n_lexems++;
						}
					}
					else
					{
						lexems[n_lexems]->ReplaceAll("\n", "");
						lexems[n_lexems]->ReplaceAll("\t", "");
						n_lexems++;
					}
					
					//if (str.FindLast("\n") == str.Length() - 1);
						//lexems[n_lexems++] = new BString("<br>");
				}
				//body->Append(str);
				//if (pre && *m == '\n')
				//{
				//	lexems[n_lexems++] = new BString("<br>");
				//	printf("] <BR>\n");
				//	m++;
				//}
				//{
				//	lexems[n_lexems++] = new BString("<br>");
					//body->Append("\n");
				//	m++;
				//}
			}
		
			if (!pre)
			{
				if (*m == '\n')
					*m = ' ';
				
				while (*m == '\n' || *m == '\r' && *m != '\t')
					m++;
			}
			//m++;
			if (catch_m == m)
				m++;
		}
		free(mas);
	}
}

status_t OakView::LoadURL( const char *url )
{
	if (strstr(url, "file://") == 0)
	{
		printf("FILE!\n");
	}
	else if (strstr(url, "http://") == 0)
	{
		printf("HTTP!\n");
	}
	else
	{
		printf("UNKNOWN!\n");
		return B_ERROR;
	}
	return B_OK;
}

status_t OakView::LoadFile( BMessage *message, const char *fname2 = NULL,
	bool his = false )
{
	entry_ref ref;
	status_t err;
	BFile file;
	char *mas, *mas2, fname[255];
	off_t length;
	bool isme = false;
	BString str, flname, lex[10], val[10], s, anchor;
	//BDirectory dir;
	BPath pth;
	int i;
	status_t ok = false;
	bool system = false;
	BString fn2, type;
	
	if (fname2 != NULL)
	{
		if (strcmp(fname2, "about:blank") == 0)
			system = true;
		else if (strcmp(fname2, "about:about") == 0)
			system = true;
		else if (strcmp(fname2, "about:home") == 0)
			system = true;
	}

	if (fname2 != NULL)
		fn2.SetTo(fname2);
	
	if (!system)
	{
		if (message == NULL && fname2 == NULL)
		  return B_ERROR;
	
		anchor.SetTo("");
	
		if (fname2 != NULL)
		{
			fn2.ReplaceAll("%20", " ");
			
			if (fn2.FindFirst("#") == 0)
				isme = true;
			if (fn2.FindFirst("#") != -1)
			{
				anchor.SetTo(fn2);
				anchor.Remove(0, anchor.FindFirst("#") + 1);
				printf("> Anchor: '%s'\n", anchor.String());
				fn2.Truncate(fn2.FindFirst("#"));
				if (fn2.Length() == 0)
				{
					fn2.SetTo(FileName());
				}
			}
			if ((fn2.Compare("/", 1) != 0) && fn2.Compare("http://", 7) != 0)
			{
				fn2.Prepend(path->String());
			}
			if (fn2.ByteAt(fn2.Length() - 1) == '/')
				fn2.Append("index.html");
			//fn2.
			printf("> LoadFile:: Trying to open '%s'\n", fn2.String());
		}
	
		if (fname2 == NULL && (err = message->FindRef("refs", 0, &ref)) != B_OK)
		  return err;
		
		if (fname2 == NULL)
			ok = file.SetTo(&ref, B_READ_ONLY);	
		else if (fn2.Length() != 0)
			ok = file.SetTo(fn2.String(), B_READ_ONLY);
	}
	
	if (ok == B_OK || system)
	{
		papa->SetStatus("Loading...");
		
		if (fn2.Compare("about:blank") == 0)
		{
			InitBlank();
			filename->SetTo("about:blank");
			path->SetTo("");
		}
		else if (fn2.Compare("about:about") == 0)
		{
			InitAbout();
			filename->SetTo("about:about");
			path->SetTo("");
		}
		else if (fn2.Compare("about:home") == 0)
		{
			InitHome();
			filename->SetTo("about:home");
			path->SetTo("");
		}
		else
		{
			BNode node;
			if (fname2 == NULL)
				node.SetTo(&ref);
			else
				node.SetTo(fn2.String());
			BNodeInfo nodeinfo(&node);
			
			char tp[100];
			nodeinfo.GetType(tp);
			type.SetTo(tp);
			
			printf("Type: %s\n", type.String());
		
			if (type.ICompare("application/x-vnd.Be-bookmark") == 0)
			{
				status_t status;
				void *data = 0;
				const char *url;
				
				BNode nodd;
				if (fname2 == NULL)
					nodd.SetTo(&ref);
				else
					nodd.SetTo(fn2.String());
				BMessage attrData;
			
				//if ((status = nodd.InitCheck()))
				//	return status;
				status = nodd.InitCheck();
					
				// Iterate through all of the attributes in the node.
				// For each attribute, get its name, type, data, and size, and
				// create a corresponding entry in the message;
				status = nodd.RewindAttrs();
				do {
					char attrName[B_ATTR_NAME_LENGTH];
					if (nodd.GetNextAttrName(attrName))
						break;
			
					attr_info attrInfo;
					if (nodd.GetAttrInfo(attrName, &attrInfo))
						break;
						
					if (data)
						free(data);
					data = malloc(attrInfo.size);
					if (!data)
						break;
			
					if (nodd.ReadAttr(attrName, attrInfo.type, 0, data, attrInfo.size) != attrInfo.size)
						break;
					
					if (attrData.AddData(attrName, attrInfo.type, data, attrInfo.size, false))
						break;
				} while (true);
			
				if (data)
					free(data);
					
				if (attrData.FindString("META:url", &url) == B_OK)
				{
					BString urr(url);
					printf("URL:%s\n", url);
					if (urr.Compare("file://", 7) == 0)
						urr.RemoveFirst("file://");
					urr.ReplaceAll("%20", " ");
					file.SetTo(urr.String(), B_READ_ONLY);
					
					node.SetTo(urr.String());
					fname2 = urr.String();
					fn2.SetTo(urr.String());
					nodeinfo.SetTo(&node);

					nodeinfo.GetType(tp);
					type.SetTo(tp);
					
					printf("Type2: %s\n", type.String());
				}
			}
			
			if (
				type.ICompare("text/html") == 0 ||
				type.ICompare("text/plain") == 0 ||
				type.ICompare("image/", 6) == 0 ||
				0
			)
			{
				if (type.ICompare("image/", 6) != 0)
				{
					// kodirovki:
					
					if (charset == B_UNICODE_CONVERSION)
					{
						file.GetSize(&length);
						mas = (char *)malloc(length);
						file.Read(mas, length);
						html->SetTo(mas);
					}
					else
					{
						file.GetSize(&length);

						int32 state, len, slen;
						slen = 65536;
						status_t conv_res = B_ERROR;
						mas2 = (char *)malloc(length * 3);
						mas = (char *)malloc(65536);
						
						int32 shag = 0;
						
						while ((len = file.Read(mas, 65536)))
						{	
							if (charset == B_UNICODE_CONVERSION)
							{
							}
							else
								conv_res = convert_to_utf8(charset,
									mas, &len, (char *)(mas2 + shag), &slen, &state);
							shag += slen;
						}
						
						mas2[shag] = '\0';

						if (conv_res == B_OK)
						{
							html->SetTo(mas2);
						}
						free(mas2);
					}
					
					free(mas);
				}
				
				// ooops...
			
				if (fname2 == NULL)
				{
					BEntry entry(&ref, true);
					entry.GetName(fname);
					filename->SetTo(fname);
					entry.GetPath(&pth);
					path->SetTo(pth.Path());		
					path->RemoveLast(*filename);
				}
				else
				{
					BString s1, s2;
					
					s1.SetTo(fn2);
					if (fn2.FindLast("/") > 0)
						s1.Remove(0, fn2.FindLast("/") + 1);
					s2.SetTo(fn2);
					s2.RemoveLast(s1);
					filename->SetTo(s1);
					path->SetTo(s2);
				}
			}
		}

		if (type.ICompare("text/plain") == 0)
		{
			html->Prepend("</title></head><body bgcolor=#d0d0d0 text=#000000><pre>");
			html->Prepend(FileName());
			html->Prepend("<html><head><title>");
			html->Append("</pre></body></html>");
		}

		if (type.ICompare("image/", 6) == 0)
		{
			html->SetTo("");
			html->Append("<html><head><title>");
			html->Append(FileName());
			html->Append("</title></head><body><img src=\"");
			html->Append(Path());
			html->Append(FileName());
			html->Append("\"></body></html>");
		}
		
		// hmmmm....
		//html->ReplaceAll("\n", " ");
		
		Process();
		
		printf("> LoadFile:: File '%s' has opened successfully\n", fn2.String());
		
		// history:
		if (!his && filename->Compare("about:blank") != 0)
		{
			if (pos_history != 0)
			{
				history[n_history - pos_history].Path()->SetTo(fname2);
				history[n_history - pos_history].Title()->SetTo(title->String());
				n_history -= pos_history;
				n_history++;
				pos_history = 0;
			}
			else
			{
				if (n_history >= MAX_HISTORY)
				{
					for (i = 0; i < n_history - 1; i++)
						history[i] = history[i + 1];
					n_history--;
				}
				if (fname2 != NULL)
					history[n_history].Path()->SetTo(fname2);
				else
				{
					BString s(path->String());
					s.Append(filename->String());
					history[n_history].Path()->SetTo(s.String());
					//printf("> history: %s\n", s.String());
				}
				history[n_history++].Title()->SetTo(title->String());
			}
		}
		papa->UpdateButtons();
		
		
		papa->SetOakTitle(Title());
		s.SetTo(Path());
		s.Append("");
		s.Append(FileName());
		papa->SetAddress(s.String());
		if (!isme)
			MyDraw(OAK_REDRAW);
		if (anchor.Length() == 0)
			papa->SetBars(vmax, hmax, true);
		else
		{
			int pos = 0;
			for (i = 0; i < n_anchors; i++)
				if (anchors[i]->Name()->Compare(anchor) == 0)
				{
					pos = anchors[i]->Pos();
				}
			if (pos > VMax())
				pos = VMax();
			papa->SetBars(vmax, hmax, true, pos);
		}
		papa->SetStatus("");

		//regex("/<title>(.*)<\\/title>/", html->String());
		//title->SetTo("Title");
	}
	else
		return ok;
	
	//if ((err = message->FindString("name", &filename)) != B_OK)
	//  ;//return err;
	  

	return B_OK;
}

void OakView::MouseUp( BPoint p )
{
	static int32 i32;

	if (!mousedown_is)
		return;
	mousedown_is = false;

	//printf("> mouseup\n");

	if (buttonsdown == B_PRIMARY_MOUSE_BUTTON)
	{
		if (p == mousedown)
		{
			select_is = 0;
			searched = -1;
			MyDraw();
		}
		if (select_processed)
		{
			select_processed = false;
			MyDraw();
		}
		else
		{
			for (i32 = 0; i32 < n_links; i32++)
			{
				if (links[i32]->Bounds().Contains(p))
				{
					printf("> OakLink:: Click to '%s'\n", links[i32]->Href());
					LoadFile((BMessage *)NULL, links[i32]->Href());
					return;
				}
			}
		}
	}
	//BView::MouseUp(p);
}

void OakView::MouseDown( BPoint p )
{
	static int32 i32, buts;
	static BMessage *m;
	bool is_link = false;

	mousedown_is = true;
	
	MakeFocus();
	MakeFocus(false);
	
	//printf("> mousedown\n");
	
	m = Window()->CurrentMessage();
	m->FindInt32("buttons", &buts);	
	if (buts == B_PRIMARY_MOUSE_BUTTON)
	{
		mousedown = p;
		buttonsdown = B_PRIMARY_MOUSE_BUTTON;

		for (i32 = 0; i32 < n_links; i32++)
		{
			if (links[i32]->Bounds().Contains(p))
			{
				printf("> OakLink:: Click to '%s'\n", links[i32]->Href());
				is_link = true;
			}
		}
		
		if (!is_link)
		{
			for (i32 = 0; i32 < n_words; i32++)
			{
				if (words[i32]->Bounds().Contains(p))
				{
					printf("> OakLink:: Click to '%s'\n", words[i32]->Word());
					select_processed = true;
					select_from = i32;
					select_to = i32;
				}
			}
			MyDraw();
		}
	}
	if (buts == B_SECONDARY_MOUSE_BUTTON)
	{
		if (overlink && link_menu != NULL)
		{
			BMenuItem *selected;
		
			ConvertToScreen(&p);
			selected = link_menu->Go(p);
			if (selected)
			{
				BLooper *looper;
				BHandler *targ = selected->Target(&looper);
				looper->PostMessage(selected->Message(), targ);
			}
		}
		else if (select_is && select_menu != NULL)
		{
			BMenuItem *selected;
		
			ConvertToScreen(&p);
			selected = select_menu->Go(p);
			if (selected)
			{
				BLooper *looper;
				BHandler *targ = selected->Target(&looper);
				looper->PostMessage(selected->Message(), targ);
			}
		}
		else if (default_menu != NULL)
		{
			BMenuItem *selected;
		
			ConvertToScreen(&p);
			selected = default_menu->Go(p);
			if (selected)
			{
				BLooper *looper;
				BHandler *targ = selected->Target(&looper);
				looper->PostMessage(selected->Message(), targ);
			}
		}
	}
	//BView::MouseDown(p);
}

void OakView::MouseMoved( BPoint p, uint32 transit, const BMessage *m )
{
	static int32 i32;

	//static char s[20];
	//sprintf(s, "i:%d", (int)p.x);
	//papa->SetStatus(s);
	
	if (select_processed)
	{
		for (i32 = 0; i32 < n_words; i32++)
		{
			if (words[i32]->Bounds().Contains(p))
			{
				//printf("> OakLink:: Click to '%s'\n", words[i32]->Word());
				select_is = true;
				if (select_to != i32)
				{
					select_to = i32;
					MyDraw();
				}
			}
		}
		static BRect r;
		r = Bounds();
		r.bottom = r.top + 30;
		//printf("Bounds(%d,%d - %d,%d)\n", (int)r.left, (int)r.top,
		//	(int)r.right, (int)r.bottom);
		//printf("Point(%d,%d)\n", (int)p.x, (int)p.y);
		if (r.Contains(p))
		{
			//mes = Window()->CurrentMessage();
			//mes->FindInt64("when", &when);
			//if (when >= lastselectscroll + 10000)
			//{
			if (scrollupmessrunner == NULL)
			{
				scrollupmessrunner = new BMessageRunner(this,
					new BMessage(OAK_SCROLLUP), 1000);
			}
			//	lastselectscroll = when;
			//}
		}
		else if (scrollupmessrunner != NULL)
		{
			delete scrollupmessrunner;
			scrollupmessrunner = NULL;
		}

		r = Bounds();
		r.bottom = r.top + papa->GetScrollViewBottom();
		r.top = r.bottom - 30;
		if (r.Contains(p))
		{
			if (scrolldownmessrunner == NULL)
			{
				scrolldownmessrunner = new BMessageRunner(this,
					new BMessage(OAK_SCROLLDOWN), 1000);
			}
/*
			mes = Window()->CurrentMessage();
			mes->FindInt64("when", &when);
			if (when >= lastselectscroll + 10000)
			{
				lastselectscroll = when;
			}*/
		}
		else if (scrolldownmessrunner != NULL)
		{
			delete scrolldownmessrunner;
			scrolldownmessrunner = NULL;
		}
	}
	else
	{
		if (scrollupmessrunner != NULL)
		{
			delete scrollupmessrunner;
			scrollupmessrunner = NULL;
		}
		if (scrolldownmessrunner != NULL)
		{
			delete scrolldownmessrunner;
			scrolldownmessrunner = NULL;
		}


		bool link = false;
		
		for (i32 = 0; i32 < n_links; i32++)
		{
			if (links[i32]->Bounds().Contains(p))
			{
				papa->SetStatus(links[i32]->Href());
				link = true;
				be_app->SetCursor(hand_cursor);
				overlink = true;
			}
		}
		if (!link)
		{
			papa->SetStatus("");
			overlink = false;
			be_app->SetCursor(B_HAND_CURSOR);
		}
	}
	//BView::MouseMoved(p, transit, m);
}

const char *OakView::Title()
{
	return title->String();
}

const char *OakView::FileName()
{
	return filename->String();
}

const char *OakView::Path()
{
	return path->String();
}

int OakView::VMax()
{
	return vmax;
}

int OakView::CountHistory()
{
	return n_history;
}

int OakView::HistoryPos()
{
	return pos_history;
}

void OakView::Back( int to = 0 )
{
	if (to == 0 && n_history - pos_history > 1)
	{
		pos_history++;
		LoadFile(NULL, history[n_history - 1 - pos_history].Path()->String(), true);
	}
	else
	{
		pos_history = to;
		LoadFile(NULL, history[n_history - 1 - pos_history].Path()->String(), true);
	}
}

void OakView::Forward()
{
	if (pos_history > 0)
	{
		pos_history--;
		LoadFile(NULL, history[n_history - pos_history - 1].Path()->String(), true);
	}
}

bool ScanAndAddHtml( const char *dirname, BString *html, int level = 0, bool count = false )
{
	BDirectory dir, dir2;
	entry_ref ref, ref2;
	BEntry entry, entry2;
	BPath path;
	BString d, d2, s, p;
	char fname[150];

	if (level == 4)
		return false;
	
	for (int i = 0; i < 2; i++)
		if (dir.SetTo(dirname) == B_OK)
		{
			if (!count && i == 0)
			{
				if (level == 0)
					html->Append("<table cellpadding=4 border=0>");
				else
					html->Append("<table cellpadding=4 border=0>");
			}
			while (dir.GetNextRef(&ref) == B_OK)
			{
				entry.SetTo(&ref);
				entry.GetName(fname);
	
				d.SetTo(dirname);
				if (d.FindLast("/") != d.Length() - 1)
					d.Append("/");
				d.Append(fname);
				if (dir2.SetTo(d.String()) == B_OK && i == 0)
				{
					if (count)
					{
						return true;
					}

					html->Append("<tr><td><font size=+2>");
					html->Append(fname);
					html->Append(":</font></td></tr>\n");
	
					if (ScanAndAddHtml(d.String(), html, level + 1, true))
					{
						html->Append("<tr><td>");
						ScanAndAddHtml(d.String(), html, level + 1);
						html->Append("</td></tr>");
					}
				}
				else if (dir2.SetTo(d.String()) != B_OK && i == 1)
				{

					BMessage bm;
					const char *url;
					ReadAttributes(d.String(), bm);
					if (bm.FindString("META:url", &url) == B_OK)
					{
						if (count)
						{
							return true;
						}

						p.SetTo(url);
						p.RemoveFirst("file://");
						if (level > 0)
							html->Append("<tr><td bgcolor=\"#4376a8\">");
						else
							html->Append("<tr><td>");
						for (int j = 0; j < level; j++)
							html->Append("<spacer width=\"30\">");
						if (level == 0)
							html->Append("<font size=+1>");
						html->Append("<a href=\"");
						html->Append(p.String());
						html->Append("\">");
						html->Append(fname);
						html->Append("</a>");
						if (level == 0)
							html->Append("</font>");
						html->Append("</td></tr>\n");
					}
				}
			}
			if (!count && i == 1)
				html->Append("</table>");
		}
		else
			printf("> Error! Cannot open bookmarks\n");
	return false;
}

void OakView::InitBlank()
{
	html->SetTo("<html><head><title>Flying Troll</title></head><body>");
	html->Append("</body></html>");
}

void OakView::InitAbout()
{
	html->SetTo("<html><head><title>About Flying Troll</title></head><body bgcolor=\"#336698\" "
		"text=\"#ffffff\" link=\"#000080\">");
	html->Append("<table width=\"100%\"><tr><td align=center><img src=\"images/haikulogo\"></td></tr>"
				 "<tr><td><hr color=#d0d0d0><font size=\"+5\">Flying Troll DR<b>4.<font size=-2>1</font></b></font><br>");
	html->Append("<font color=#f0f0f0>2007. Written by Makitka (zaljotov@mail.ru).</font>");
	html->Append("</td></tr>");
	html->Append("</table>");
	html->Append("</body></html>");
}

void OakView::InitHome()
{
	html->SetTo("<html><head><title>About Flying Troll</title></head><body bgcolor=\"#336698\" "
		"text=\"#ffffff\" link=\"#c0c0c0\">");
	html->Append("<table width=\"100%\"><tr><td align=\"right\"><img src=\"images/haikulogo\"></td></tr>"
				 "<tr><td bgcolor=\"#4376a8\" align=\"right\"><hr color=#336698><font size=\"+5\">Flying Troll DR<b>4.<font size=-2>1</font></b></font><br>");
	html->Append("<font color=#f0f0f0 size=3>Mini browser for Haiku-OS.</font>");
	html->Append("</td></tr>");
	html->Append("</table>");
	
	ScanAndAddHtml("/boot/home/config/settings/FlyingTroll/Bookmarks/", html);
	
	html->Append("");
	html->Append("</body></html>");
}

void OakView::MessageReceived( BMessage *message )
{
	int newvalue = 0;
	switch (message->what)
	{
	case B_SIMPLE_DATA:
		//printf("Voila!!!!!\n");
		LoadFile(message);
		break;
	case OAK_SCROLLUP:
		if (papa->IsScroll())
		{
			newvalue = papa->GetScrollBarValue() - 10;
			if (newvalue < 0)
				newvalue = 0;
			papa->SetScrollBarValue(newvalue);
		}
		break;
	case OAK_SCROLLDOWN:
		if (papa->IsScroll())
		{
			newvalue = papa->GetScrollBarValue() + 10;
			if (newvalue < 0)
				newvalue = 0;
			papa->SetScrollBarValue(newvalue);
		}
		break;
	default:
		//BView::MessageReceived(message);
		break;
	}
}

void OakView::SelectAll()
{
	select_from = 0;
	select_to = n_words - 1;
	select_is = true;
	if (select_to < 0)
	{
		select_to = 0;
		select_is = false;
	}
	select_processed = false;
	MyDraw();
}

void OakView::Search( const char *s, bool again = false )
{
	int32 i32;
	int newvalue = -1;
	
	if (s == "")
	{
		searched = -1;
		return;
	}
	if (!again)
		searched = -1;
	for (i32 = searched + 1; i32 < n_words; i32++)
	{
		if ((new BString(words[i32]->Word()))->IFindFirst(s) != -1)
		{
			searched = i32;
			newvalue = (int)words[i32]->Bounds().top - 100;
			if (newvalue < 0)
				newvalue = 0;
			if (newvalue > vmax)
				newvalue = vmax;
			papa->SetScrollBarValue(newvalue);
			MyDraw();
			i32 = n_words;
		}
	}
	if (again && newvalue == -1)
	{
	
		for (i32 = 0; i32 < n_words; i32++)
		{
			if ((new BString(words[i32]->Word()))->IFindFirst(s) != -1)
			{
				searched = i32;
				newvalue = (int)words[i32]->Bounds().top - 100;
				if (newvalue < 0)
					newvalue = 0;
				if (newvalue > vmax)
					newvalue = vmax;
				papa->SetScrollBarValue(newvalue);
				MyDraw();
				i32 = n_words;
			}
		}
	}
}

void OakView::ToClipboard()
{
	if (select_is)
	{
		int32 i;
		int32 r;
		BString s;
		
		if (select_to < select_from)
		{
			r = select_to;
			select_to = select_from;
			select_from = r;
		}
		
		s.SetTo("");
		for (i = select_from; i <= select_to; i++)
		{
			s.Append(words[i]->Word());
		}
		
		BMessage *clip = (BMessage *)NULL;
		if (be_clipboard->Lock())
		{
			be_clipboard->Clear();
			if ((clip = be_clipboard->Data()))
			{
				clip->AddData("text/plain", B_MIME_TYPE, s.String(), s.Length());
				be_clipboard->Commit();
			}
			be_clipboard->Unlock();
		}
	}
}

void OakView::Reload()
{
	LoadFile(NULL, filename->String(), true);
}