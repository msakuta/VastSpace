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
	virtual void draw(GLwindowState &ws, GLWentlistCriteria *p, int y){}
	virtual int mouse(GLwindowState &ws, GLWentlistCriteria *p, int key, int state, int mx, int my){return 0;}
};

class CriterionNode{
public:
	ItemCriterion *crt;
	CriterionNode *left, *right;
	enum Op{And, Or, Xor, NumOp} op;

	bool match(Entity *e)const{
		if(crt)
			return crt->match(e);
		if(op == And)
			return left->match(e) && right->match(e);
		else
			return left->match(e) || right->match(e);
	}

	int mouse(GLwindowState &ws, GLWentlistCriteria *p, int &y, int key, int state, int mx, int my);
	void draw(GLwindowState &ws, GLWentlistCriteria *p, int &y);

	static bool deleteNode(CriterionNode **ppcrt, const ItemCriterion *node){
		if(!*ppcrt)
			return false;
		if((*ppcrt)->crt){
			if((*ppcrt)->crt == node){
				delete (*ppcrt)->crt;
				delete *ppcrt;
				*ppcrt = NULL;
				return true;
			}
			else
				return false;
		}
		else
			return deleteNodeInt(ppcrt, node);
	}

private:
	/// The assumption is that *ppparent is not NULL and (*ppparent)->crt is NULL.
	static bool deleteNodeInt(CriterionNode **ppparent, const ItemCriterion *node){
		CriterionNode *parent = *ppparent;
		if(parent->left->crt){
			if(parent->left->crt == node){
				delete parent->left->crt;
				*ppparent = parent->right;
				delete parent;
				return true;
			}
		}
		else{
			bool ret = deleteNodeInt(&parent->left, node);
			if(ret)
				return ret;
		}

		if(parent->right->crt){
			if(parent->right->crt == node){
				delete parent->right->crt;
				*ppparent = parent->left;
				delete parent;
				return true;
			}
		}
		else{
			bool ret = deleteNodeInt(&parent->right, node);
			if(ret)
				return ret;
		}

		return false;
	}
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

	CriterionNode *crtRoot;
};

#endif
