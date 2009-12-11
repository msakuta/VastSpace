#ifndef POPUP_H
#define POPUP_H
#include <stddef.h>
#include <cpplib/dstring.h>

struct PopupMenuItem{
	cpplib::dstring title;
	int key;
	cpplib::dstring cmd;
	PopupMenuItem *next;
};

class PopupMenu{
	PopupMenuItem *list, **end;
public:
	PopupMenu();
	PopupMenu(const PopupMenu &o);
	~PopupMenu();
	PopupMenu &append(cpplib::dstring title, int key, cpplib::dstring cmd); // appends an menu item to the last
	PopupMenu &appendSeparator(); // append an separator at the end
	const PopupMenuItem *get()const;
	PopupMenuItem *get();
	int count()const;
};


//--------------------------------------------------------------------------
// Implementation
//--------------------------------------------------------------------------

inline PopupMenu::PopupMenu() : list(NULL), end(&list){}

inline PopupMenu::PopupMenu(const PopupMenu &o) : list(NULL), end(&list){
	const PopupMenuItem *src = o.list;
	int count;
	for(count = 0; src; count++, src = src->next){
		PopupMenuItem *dst = *end = new PopupMenuItem(*src);
		end = &dst->next;
	}
}

inline PopupMenu &PopupMenu::append(cpplib::dstring title, int key, cpplib::dstring cmd){
	PopupMenuItem *i = *end = new PopupMenuItem;
	i->title = title;
	i->key = key;
	i->cmd = cmd;
	i->next = NULL;
	end = &i->next;
	return *this;
}

inline PopupMenu::~PopupMenu(){
	for(PopupMenuItem *mi = list; mi;){
		PopupMenuItem *minext = mi->next;
		delete mi;
		mi = minext;
	}
}

inline PopupMenu &PopupMenu::appendSeparator(){
	PopupMenuItem *i = *end = new PopupMenuItem;
	i->key = 0;
	i->next = NULL;
	end = &i->next;
	return *this;
}

inline const PopupMenuItem *PopupMenu::get()const{ return list; }
inline PopupMenuItem *PopupMenu::get(){ return list; }

inline int PopupMenu::count()const{
	int ret = 0;
	for(const PopupMenuItem *item = list; item; item = item->next)
		ret++;
	return ret;
}

#endif
