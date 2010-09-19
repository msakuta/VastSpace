/** \file
 * \brief Definition of GLWentlist class.
 */
#ifndef GLW_GLWENTLIST_H
#define GLW_GLWENTLIST_H
#include "glwindow.h"
#include "../CoordSys.h"
#include "../Entity.h"

/// \brief Window that shows entity listing.
///
/// It can select view modes from two flavors, list and icons.
class GLWentlist : public GLwindowSizeable{
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
public:
	typedef GLwindowSizeable st;

	Player &pl;
	enum ListMode{ Select, All, Universe } listmode;
	bool groupByClass;
	bool icons;
	bool switches;
	bool teamOnly;
	std::vector<Entity*> ents[OV_COUNT], *pents[OV_COUNT];
	int n;
	GLWentlist(Player &player) : st("Entity List"), pl(player), listmode(Select), groupByClass(true), icons(true), switches(false), teamOnly(false){}
	virtual void draw(GLwindowState &ws, double);
	virtual int mouse(GLwindowState &ws, int button, int state, int mx, int my);
	virtual void mouseLeave(GLwindowState &ws);
};

#endif
