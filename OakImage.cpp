#include "OakImage.h"

OakImage::OakImage( const char *s )
{
	BString ss(s);
	ss.ReplaceAll("%20", " ");
	ss.ReplaceAll("%21", "!");
	bb = BTranslationUtils::GetBitmapFile(ss.String());
	if(bb == NULL)
	{
		printf("OakImage:: Image load failed ('%s').\n", s);
	}
	name = new BString();
	name->SetTo(s);
}

OakImage::~OakImage()
{
	if (bb != NULL)
		delete bb;
	delete name;
}

BString *OakImage::Name()
{
	return name;
}

BBitmap *OakImage::Bitmap()
{
	return bb;
}