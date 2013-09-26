#ifndef _OAKHISTORY_H_
#define _OAKHISTORY_H_

#include <String.h>

class OakHistory
{
private:
	BString *path;
	BString *title;
public:
	OakHistory( );
	~OakHistory();
	BString *Path() { return path; }
	BString *Title() { return title; }
	void SetPath( const char *s ) { path->SetTo(s); }
	void SetTitle( const char *s ) { title->SetTo(s); }
	OakHistory &operator=( OakHistory & );
};

#endif