#ifndef _OAKTABVIEW_H_
#define _OAKTABVIEW_H

#include <TabView.h>

class OakTabView : public BTabView
{
public:
	OakTabView( BRect r );
	virtual void Draw ( BRect ur );
};

#endif