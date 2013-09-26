#ifndef _OAKIMAGE_H_
#define _OAKIMAGE_H_

#include <Bitmap.h>
#include <TranslationUtils.h>
#include <String.h>
#include <stdio.h>

class OakImage
{
private:
	BBitmap *bb;
	BString *name;
public:
	OakImage( const char * );
	~OakImage();
	BString *Name();
	BBitmap *Bitmap();
};

#endif