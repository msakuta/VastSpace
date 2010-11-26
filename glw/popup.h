/** \file
 * \brief Defines PopupMenu and its items.
 */
#ifndef POPUP_H
#define POPUP_H
#include "../cmd.h"
#include <stddef.h>
#include <squirrel.h>
#include <cpplib/dstring.h>

/// Base class for all menu items.
struct PopupMenuItem{
	cpplib::dstring title; ///< String shown to identify the menu item.
	int key; ///< Shortcut key.
	PopupMenuItem *next; ///< Link to next node in the linked list.
	PopupMenuItem(cpplib::dstring title = "") : title(title){}
	virtual bool isSeparator()const{return false;}
	virtual void execute(){};
	virtual PopupMenuItem *clone()const = 0;
	virtual ~PopupMenuItem(){}
	static const int separator_key = 13564;
};

/// A separator.
struct PopupMenuItemSeparator : public PopupMenuItem{
	virtual bool isSeparator()const{return true;}
	virtual PopupMenuItem *clone()const{return new PopupMenuItemSeparator(*this);}
};

/// Menu item that executes a console command.
struct PopupMenuItemCmd : public PopupMenuItem{
	cpplib::dstring cmd;
	virtual void execute(){CmdExec(cmd);}
	virtual PopupMenuItem *clone()const{return new PopupMenuItemCmd(*this);}
};

/// Menu item that executes a Squirrel closure.
struct PopupMenuItemClosure : public PopupMenuItem{
	HSQUIRRELVM v; ///< It's redundant to store the Squirrel VM handle here, but it's required to add reference for ho member anyway.
	HSQOBJECT ho; ///< Squirrel closure that will be executed. Reference count must be managed.
	PopupMenuItemClosure(HSQOBJECT ho, HSQUIRRELVM v) : ho(ho), v(v){sq_addref(v, const_cast<HSQOBJECT*>(&ho));}
	PopupMenuItemClosure(const PopupMenuItemClosure &o) : PopupMenuItem(o), ho(o.ho), v(o.v){ sq_addref(v, const_cast<HSQOBJECT*>(&ho));}
	virtual void execute(){
		sq_pushobject(v, ho);
		sq_pushroottable(v); // The this pointer could be altered to realize method pointer.
		sq_call(v, 1, 0, SQTrue);
		sq_pop(v, 1);
	}
	virtual PopupMenuItem *clone()const{return new PopupMenuItemClosure(*this);}
	virtual ~PopupMenuItemClosure(){sq_release(v, &ho);}
};

/// List of menu items that is commonly used by GLWmenu. Aggrigates PopupMenuItems.
class PopupMenu{
	PopupMenuItem *list, **end;
public:
	PopupMenu();
	PopupMenu(const PopupMenu &o);
	~PopupMenu();
	PopupMenu &append(cpplib::dstring title, int key, cpplib::dstring cmd, bool unique = true); // appends an menu item to the last
	PopupMenu &appendSeparator(bool mergenext = true); // append an separator at the end
	PopupMenu &append(PopupMenuItem *item, bool unique = true);
	const PopupMenuItem *get()const;
	PopupMenuItem *get();
	int count()const;
};


//--------------------------------------------------------------------------
// Implementation
//--------------------------------------------------------------------------

inline PopupMenu::PopupMenu() : list(NULL), end(&list){}
inline const PopupMenuItem *PopupMenu::get()const{ return list; }
inline PopupMenuItem *PopupMenu::get(){ return list; }

#endif
