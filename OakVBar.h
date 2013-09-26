#ifndef _OAKVBAR_H_
#define _OAKVBAR_H_

#include <ScrollBar.h>

class OakVBar : public BScrollBar
{
private:
public:
	OakVBar( BRect r );
	virtual void ValueChanged( float );
};

#endif