/** \file
 * \brief Definition of GLWentlist class.
 */
#ifndef GLW_GLWENTLIST_H
#define GLW_GLWENTLIST_H
#include "glwindow.h"
#include "../CoordSys.h"
#include "../Entity.h"


class GLWentlist;
class GLWentlistCriteria;

/// Class that represents a criterion to determine if given Entity is included in the Entity Listing.
class ItemCriterion{
public:
	GLWentlist &entlist;
	ItemCriterion(GLWentlist &entlist) : entlist(entlist){}
	virtual bool match(const Entity *)const = 0; ///< Override to define criterion

	struct DrawParams;
	virtual void draw(GLwindowState &ws, GLWentlistCriteria *p, DrawParams &dp); ///< Drawing method in GLWentlist.

	struct MouseParams;
	virtual int mouse(GLwindowState &ws, GLWentlistCriteria *p, MouseParams &mp); ///< Mouse response method in GLWentlist.

	/// Delete method has a convention that delete itself and return a pointer that referrer should have instead.
	/// This convention propergates to complex nested tree nodes properly.
	virtual ItemCriterion *deleteNode(const ItemCriterion *node){
		if(node == this){
			delete this;
			return NULL;
		}
		return this;
	}

	/// Alter all pointers in the tree that point to given item.
	virtual void alterNode(const ItemCriterion *from, ItemCriterion *to){}

	/// Only not operator must return true.
	virtual bool isNotOperator()const{return false;}
};

/// \brief Window that shows entity listing.
///
/// It can select view modes from two flavors, list and icons.
class GLWentlist : public GLwindowSizeable{
protected:
	void draw_int(const CoordSys *cs, int &n, std::vector<Entity*> ents[]);
	static const int OV_COUNT = 32; ///< Overview count
	static int ent_pred(Entity *a, Entity *b){
		return a->race < b->race;
	}
	void menu_select(){listmode = Select;}
	void menu_all(){listmode = All;}
	void menu_universe(){listmode = Universe;}
	void menu_grouping(){groupByClass = !groupByClass;}
	void menu_icons(){icons = !icons;}
	void menu_switches(){switches = !switches;}
	void menu_team(){teamOnly = !teamOnly;}
	void menu_criteria();
	class ItemSelector;
public:
	typedef GLwindowSizeable st;

	Player &pl;
	enum ListMode{ Select, All, Universe } listmode;
	bool groupByClass;
	bool icons;
	bool switches;
	bool teamOnly;
	std::vector<Entity*> ents[OV_COUNT], *pents[OV_COUNT];
	int n; ///< Count of ents.
	int scrollpos; ///< Current scroll position in the vertical scroll bar.
	GLWentlist(Player &player);
	virtual void draw(GLwindowState &ws, double);
	virtual int mouse(GLwindowState &ws, int button, int state, int mx, int my);
	virtual void mouseLeave(GLwindowState &ws);

	ItemCriterion *crtRoot;
};

#endif
