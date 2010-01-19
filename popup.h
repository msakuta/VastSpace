#ifndef POPUP_H
#define POPUP_H
#include <stddef.h>
#include <cpplib/dstring.h>

struct PopupMenuItem{
	cpplib::dstring title;
	int key;
	cpplib::dstring cmd;
	PopupMenuItem *next;
	bool isSeparator()const{return key == separator_key;}
	static const int separator_key = 13564;
};

class PopupMenu{
	PopupMenuItem *list, **end;
public:
	PopupMenu();
	PopupMenu(const PopupMenu &o);
	~PopupMenu();
	PopupMenu &append(cpplib::dstring title, int key, cpplib::dstring cmd, bool unique = true); // appends an menu item to the last
	PopupMenu &appendSeparator(bool mergenext = true); // append an separator at the end
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

inline PopupMenu::~PopupMenu(){
	for(PopupMenuItem *mi = list; mi;){
		PopupMenuItem *minext = mi->next;
		delete mi;
		mi = minext;
	}
}

inline PopupMenu &PopupMenu::append(cpplib::dstring title, int key, cpplib::dstring cmd, bool unique){
	if(unique){
		// Returns unchanged if duplicate menu item is present. This determination is done by title, not command.
		for(PopupMenuItem *p = list; p; p = p->next) if(p->title == title)
			return *this;
	}
	PopupMenuItem *i = *end = new PopupMenuItem;
	i->title = title;
	i->key = key;
	i->cmd = cmd;
	i->next = NULL;
	end = &i->next;
	return *this;
}

inline PopupMenu &PopupMenu::appendSeparator(bool mergenext){
	// Returns unchanged if preceding menu item is a separator too.
	if(mergenext && list){
		PopupMenuItem *p;
		for(p = list; p->next; p = p->next);
		if(p->key == PopupMenuItem::separator_key)
			return *this;
	}
	PopupMenuItem *i = *end = new PopupMenuItem;
	i->key = PopupMenuItem::separator_key;
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
