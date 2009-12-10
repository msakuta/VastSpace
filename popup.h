#ifndef POPUP_H
#define POPUP_H
#include <cpplib/dstring.h>

struct PopupMenuItem{
	cpplib::dstring title;
	int key;
	cpplib::dstring cmd;
	PopupMenuItem *next;
};

#endif
