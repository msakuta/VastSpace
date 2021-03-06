/** \file
 * \brief Definition of GLWentlist class.
 */
#ifndef GLW_GLWENTLIST_H
#define GLW_GLWENTLIST_H
#include "glw/glwindow.h"
#include "CoordSys.h"
#include "Entity.h"


class GLWentlist;
class GLWentlistCriteria;

class ItemCriterion;

/// \brief Window that shows entity listing.
///
/// It can select view modes from two flavors, list and icons.
class GLWentlist : public GLwindowSizeable{
protected:
	void draw_int(const CoordSys *cs, int &n, std::vector<const Entity*> ents[]);
	static const int OV_COUNT = 32; ///< Overview count
	static int ent_pred(const Entity *a, const Entity *b){
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

	WeakPtr<Player> player;
public:
	typedef GLwindowSizeable st;

	enum ListMode{ Select, All, Universe } listmode;
	bool groupByClass;
	bool icons;
	bool switches;
	bool teamOnly;
	std::vector<const Entity*> ents[OV_COUNT], *pents[OV_COUNT];
	int n; ///< Count of ents.
	int scrollpos; ///< Current scroll position in the vertical scroll bar.
	GLWentlist(Game *game);
	virtual void draw(GLwindowState &ws, double);
	virtual int mouse(GLwindowState &ws, int button, int state, int mx, int my);
	virtual void mouseLeave(GLwindowState &ws);
	Player *getPlayer();

	ItemCriterion *crtRoot;

	static SQInteger sqf_constructor(HSQUIRRELVM v);
	static bool sq_define(HSQUIRRELVM v);
};

#endif
