/** \file
 * \brief Implementation of GLWentlist class.
 */
#include "GLWentlist.h"
#include "GLWtip.h"
#include "GLWmenu.h"
#include "antiglut.h"
#include "Player.h"
#include "war.h"
#include "Game.h"
#include "sqadapt.h"
extern "C"{
#include <clib/timemeas.h>
}
#include <string.h>
#include <algorithm>

/// Set nonzero to measure sorting speed
#define PROFILE_SORT 0

static sqa::Initializer definer("GLWentlist", GLWentlist::sq_define);

/// Class that represents a criterion to determine if given Entity is included in the Entity Listing.
class ItemCriterion{
public:
	GLWentlist &entlist;
	ItemCriterion(GLWentlist &entlist) : entlist(entlist){}
	virtual ClassId id()const = 0; ///< Define distinguishable identifier in derived class.
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

	template<typename T> void insertNode();
};



static bool pents_pred(std::vector<const Entity*> *a, std::vector<const Entity*> *b){
	// Comparing returned pointers themselves by dispname() would be enough for sorting, but it's better
	// that the player can see items sorted alphabetically.
	return strcmp((*a)[0]->dispname(), (*b)[0]->dispname()) < 0;
}


GLWentlist::GLWentlist(Game *game) :
	st(game, "Entity List"),
	player(NULL),
	listmode(Select),
	groupByClass(true),
	icons(true),
	switches(false),
	teamOnly(false),
	scrollpos(0),
	crtRoot(NULL)
{
}

Player *GLWentlist::getPlayer(){
	if(player)
		return player;
	if(game && game->player)
		return player = game->player;
}

void GLWentlist::draw_int(const CoordSys *cs, int &n, std::vector<const Entity*> ents[]){
	if(!cs)
		return;
	for(const CoordSys *cs1 = cs->children; cs1; cs1 = cs1->next)
		draw_int(cs1, n, ents);
	if(!cs->w)
		return;
	WarField::EntityList::iterator it = cs->w->el.begin();
	for(; it != cs->w->el.end(); it++) if(*it){
		Entity *pt = *it;

		/* show only member of current team */
//		if(teamOnly && pl.race != pt->race)
//			continue;
		if(crtRoot && !crtRoot->match(pt))
			continue;

		int i;
		for(i = 0; i < n; i++) if(!strcmp(ents[i][0]->dispname(), pt->dispname())){
			ents[i].push_back(pt);
			break;
		}
		if(i == n){
			ents[n].push_back(pt);
			if(++n == OV_COUNT)
				break;
		}
	}
}

/// Class that defines how GLWentlist items are grouped.
typedef class GLWentlist::ItemSelector{
public:
	std::vector<const Entity*> *(&pents)[GLWentlist::OV_COUNT];
	int n;
	ItemSelector(GLWentlist *p) : pents(p->pents), n(p->n){}
	virtual int count() = 0; ///< Returns count of displayed items
	virtual bool begin() = 0; ///< Starts enumeration of displayed items
	virtual const Entity *get() = 0; ///< Gets current item, something like STL's iterator::operator* (unary)
	virtual bool next() = 0; ///< Returns true if there are more items to go
	virtual bool stack() = 0; ///< If items are stackable
	virtual int countInGroup() = 0; ///< Count stacked items

	/// Callback class that is to be derived to define procedure for each item. Used with foreach().
	class ForEachProc{
	public:
		virtual void proc(const Entity*) = 0;
	};
	virtual void foreach(ForEachProc&) = 0; ///< Call ForEachProc::proc for each of all items, regardless of how grouping works.
} ItemSelector;

/// Grouped by class (dispname)
class ClassItemSelector : public ItemSelector{
public:
	int i;
	ClassItemSelector(GLWentlist *p) : ItemSelector(p){}
	virtual int count(){return n;}
	virtual bool begin(){i = 0; return true;}
	virtual const Entity *get(){return (*pents[i])[0];}
	virtual bool next(){return ++i < n;}
	virtual bool stack(){return true;}
	virtual int countInGroup(){return pents[i]->size();}
	virtual void foreach(ForEachProc &proc){
		std::vector<const Entity*>::iterator it = pents[i]->begin();
		for(; it != pents[i]->end(); it++)
			proc.proc(*it);
	}
};

/// No grouping whatsoever
class ExpandItemSelector : public ItemSelector{
public:
	int i;
	std::vector<const Entity*>::iterator it;
	ExpandItemSelector(GLWentlist *p) : ItemSelector(p){}
	virtual int count(){
		int ret = 0;
		for(int i = 0; i < n; i++)
			ret += pents[i]->size();
		return ret;
	}
	virtual bool begin(){
		i = 0;
		if(n)
			it = pents[0]->begin();
		return true;
	}
	virtual const Entity *get(){
		return it == pents[i]->end() ? NULL : *it;
	}
	virtual bool next(){
		it++;
		if(it == pents[i]->end()){
			if(n <= i + 1)
				return false;
			it = pents[++i]->begin();
		}
		return true;
	}
	virtual bool stack(){return false;}
	virtual int countInGroup(){return 1;}
	virtual void foreach(ForEachProc &proc){
		proc.proc(*it);
	}
};

/// Definition of how items are located.
class ItemLocator{
public:
	virtual int allHeight() = 0; ///< Returns height of all items, used by the vertical scroll bar.
	virtual bool begin() = 0; ///< Begins enumeration
	virtual GLWrect getRect() = 0; ///< Returns rectangle of current item
	virtual bool next() = 0; ///< Enumerate the next item
};

/// Locates items in a list. An item each line.
class ListItemLocator : public ItemLocator{
public:
	GLWentlist *p;
	int count;
	int i;
	ListItemLocator(GLWentlist *p, int count) : p(p), count(count), i(0){}
	virtual int allHeight(){return count * int(GLwindow::getFontHeight());}
	virtual bool begin(){i = 0; return true;}
	virtual GLWrect getRect(){
		GLWrect ret = p->clientRect();
		ret.y0 += i * int(GLwindow::getFontHeight());
		ret.y1 = ret.y0 + int(GLwindow::getFontHeight());
		return ret;
	}
	virtual bool next(){return i++ < count;}
};

/// Icon matrix.
class IconItemLocator : public ItemLocator{
public:
	GLWentlist *p;
	int count;
	int i;
	int iconSize;
	IconItemLocator(GLWentlist *p, int count, int iconSize = 32) : p(p), count(count), i(0), iconSize(iconSize){}
	virtual int allHeight(){
		return (count + (p->clientRect().width() - 10) / iconSize - 1)
			/ max(1, (p->clientRect().width() - 10) / iconSize) * iconSize;
	}
	virtual bool begin(){i = 0; return true;}
	virtual GLWrect getRect(){
		GLWrect ret = p->clientRect();
		int cols = max(1, (ret.width() - 10) / iconSize);
		ret.x0 += i % cols * iconSize;
		ret.y0 += i / cols * iconSize;
		ret.x1 = ret.x0 + iconSize;
		ret.y1 = ret.y0 + iconSize;
		return ret;
	}
	virtual bool next(){return i++ < count;}
};

void GLWentlist::draw(GLwindowState &ws, double){
	// Ensure the player is present. There can be no player in the game.
	Player *player = getPlayer();
	if(!player)
		return;
	Player &pl = *player;
	const WarField *const w = pl.cs->w;
	int i;

	n = 0;
	for(i = 0; i < OV_COUNT; i++)
		ents[i].clear();

	if(listmode == Select){
		Player::SelectSet::iterator it;
		for(it = player->selected.begin(); it != player->selected.end(); it++){
			Entity *pt = *it;

			// It is possible that a weak pointer has lost a reference to deleted Entity.
			if(!pt)
				continue;

			/* show only member of current team */
//			if(teamOnly && pl.race != pt->race)
//				continue;
			if(crtRoot && !crtRoot->match(pt))
				continue;

			for(i = 0; i < n; i++) if(!strcmp(ents[i][0]->dispname(), pt->dispname())){
				ents[i].push_back(pt);
				break;
			}
			if(i == n/* || current_vft == vfts[i]*/){
				ents[n].push_back(pt);
				if(++n == OV_COUNT)
					break;
			}
		}
	}
	else if(listmode == All && w){
		WarField::EntityList::const_iterator it = w->el.begin();
		for(; it != w->el.end(); it++) if(*it){
			const Entity *pt = *it;

			// It is possible that a weak pointer has lost a reference to deleted Entity.
			if(!pt)
				continue;

			/* show only member of current team */
//			if(teamOnly && pl.race != pt->race)
//				continue;
			if(crtRoot && !crtRoot->match(pt))
				continue;

			for(i = 0; i < n; i++) if(!strcmp(ents[i][0]->dispname(), pt->dispname())){
				ents[i].push_back(pt);
				break;
			}
			if(i == n/* || current_vft == vfts[i]*/){
				ents[n].push_back(pt);
				if(++n == OV_COUNT)
					break;
			}
		}
	}
	else if(listmode == Universe){
		draw_int(player->cs->findcspath("/"), n, ents);
	}

	GLWrect r = clientRect();
	int wid = int((r.width() - 10) / getFontWidth()) -  6;

	if(switches){
		static const GLfloat colors[2][4] = {{1,1,1,1},{0,1,1,1}};
		glColor4fv(colors[listmode == Select]);
		glwpos2d(r.x0, r.y0 + getFontHeight());
		glwprintf("Select");
		glColor4fv(colors[listmode == All]);
		glwpos2d(r.x0 + r.width() / 5, r.y0 + getFontHeight());
		glwprintf("All");
		glColor4fv(colors[listmode == Universe]);
		glwpos2d(r.x0 + r.width() * 2 / 5, r.y0 + getFontHeight());
		glwprintf("Universe");
		glColor4fv(colors[groupByClass]);
		glwpos2d(r.x0 + r.width() * 3 / 5, r.y0 + getFontHeight());
		glwprintf("Grouping");
		glColor4fv(colors[icons]);
		glwpos2d(r.x0 + r.width() * 4 / 5, r.y0 + getFontHeight());
		glwprintf("Icons");
	}

#if PROFILE_SORT
	timemeas_t tm;
	TimeMeasStart(&tm);
#endif
	for(i = 0; i < n; i++){
		std::sort(ents[i].begin(), ents[i].end(), ent_pred);
		pents[i] = &ents[i];
	}
#if PROFILE_SORT
	printf("sort %g\n", TimeMeasLap(&tm));
#endif

#if PROFILE_SORT
	TimeMeasStart(&tm);
#endif
	if(1 < n)
		std::sort(&pents[0], &pents[n-1], pents_pred);
#if PROFILE_SORT
	printf("sort2 %g\n", TimeMeasLap(&tm));
#endif

	ClassItemSelector cis(this);
	ExpandItemSelector eis(this);
	ItemSelector &is = groupByClass ? (ItemSelector&)cis : (ItemSelector&)eis;
	int itemCount = is.count();

	ListItemLocator lil(this, itemCount);
	IconItemLocator eil(this, itemCount);
	ItemLocator &il = icons ? (ItemLocator&)eil : (ItemLocator&)lil;

	// Do not scroll too much
	if(il.allHeight() < r.height())
		scrollpos = 0;
	else if(il.allHeight() - r.height() <= scrollpos)
		scrollpos = il.allHeight() - r.height() - 1;

	int fontheight = int(getFontHeight());
	glPushAttrib(GL_SCISSOR_BIT);
	glScissor(r.x0, ws.h - r.y1, r.width(), r.height() - switches * fontheight);
	glEnable(GL_SCISSOR_TEST);

	int x = 0, y = switches * int(getFontHeight());
	glPushMatrix();
	glTranslated(0, -scrollpos + switches * fontheight, 0);
	for(is.begin(), il.begin(), i = 0; i < itemCount; i++, is.next(), il.next()){
		const Entity *pe = is.get();
		GLWrect iconRect = il.getRect();
			
		if(iconRect.include(ws.mx, ws.my + scrollpos + switches * fontheight)){
			glColor4f(0,0,1.,.5);
			glBegin(GL_QUADS);
			glVertex2i(iconRect.x0, iconRect.y0);
			glVertex2i(iconRect.x1, iconRect.y0);
			glVertex2i(iconRect.x1, iconRect.y1);
			glVertex2i(iconRect.x0, iconRect.y1);
			glEnd();
		}

		/// Local class that aquires whether any of given group of Entities are selected by the Player.
		class PlayerSelection : public ItemSelector::ForEachProc{
		public:
			Player &pl;
			bool ret;
			PlayerSelection(GLWentlist *p) : ret(false), pl(*p->player){}
			virtual void proc(const Entity *pe){
				for(Player::SelectSet::iterator it = pl.selected.begin(); it != pl.selected.end(); it++) if(*it == pe){
					ret = true;
					return;
				}
			}
		};

		if(icons){
			if(pe){
				glPushMatrix();
				glTranslated(iconRect.hcenter(), iconRect.vcenter(), 0);
				glScalef(iconRect.width() * 6 / 16.f, iconRect.height() * 6 / 16.f, 1);
				Vec3f cols(float(pe->race % 2), 1, float((pe->race + 1) % 2));
				if(listmode != Select){
					PlayerSelection ps(this);
					is.foreach(ps);
					if(!ps.ret)
						cols *= .5;
				}
				glColor4f(cols[0], cols[1], cols[2], 1);
				const_cast<Entity*>(pe)->drawOverlay(NULL);
				glPopMatrix();
			}
			GLWrect borderRect = iconRect.expand(-2);
			glBegin(GL_LINE_LOOP);
			glVertex2i(borderRect.x0, borderRect.y0);
			glVertex2i(borderRect.x1, borderRect.y0);
			glVertex2i(borderRect.x1, borderRect.y1);
			glVertex2i(borderRect.x0, borderRect.y1);
			glEnd();
			if(is.stack()){
				glColor4f(1,1,1,1);
				glwpos2d(iconRect.x1 - 24, iconRect.y1);
				glwprintf("%3d", pents[i]->size());
			}
			else if(pe){
				double x2 = 2. * pe->health / pe->maxhealth() - 1.;
				const double h = .1;
				glPushMatrix();
				glTranslated(iconRect.hcenter(), iconRect.vcenter(), 0);
				glScalef(borderRect.width() / 2.f, borderRect.height() / 2.f, 1);
				glBegin(GL_QUADS);
				glColor4ub(0,255,0,255);
				glVertex3d(-1., -1., 0.);
				glVertex3d( x2, -1., 0.);
				glVertex3d( x2, -1. + h, 0.);
				glVertex3d(-1., -1. + h, 0.);
				glColor4ub(255,0,0,255);
				glVertex3d( x2, -1., 0.);
				glVertex3d( 1., -1., 0.);
				glVertex3d( 1., -1. + h, 0.);
				glVertex3d( x2, -1. + h, 0.);
				glEnd();
				glPopMatrix();
			}
		}
		else{
			if(listmode == Select)
				glColor4f(1,1,1,1);
			else{
				PlayerSelection ps(this);
				is.foreach(ps);
				if(ps.ret)
					glColor4f(1,1,1,1);
				else
					glColor4f(.75,.75,.75,1);
			}
			glwpos2d(iconRect.x0, iconRect.y1);
			if(is.stack())
				glwprintf("%*.*s x %-3d", wid, wid, pe->dispname(), is.countInGroup());
			else
				glwprintf("%*.*s", wid, wid, pe->dispname());
		}
	}
	glPopMatrix();

	if(r.height() < il.allHeight())
		glwVScrollBarDraw(this, r.x1 - 10, r.y0, 10, r.height(), il.allHeight() - r.height(), scrollpos);
	glPopAttrib();
}

/// Menu item that executes a member function of some class.
template<typename T>
struct PopupMenuItemFunctionT : public PopupMenuItem{
	typedef PopupMenuItem st;
	T *p;
	void (T::*func)();
	PopupMenuItemFunctionT(gltestp::dstring title, T *p, void (T::*func)()) : st(title), p(p), func(func){}
	virtual void execute(){
		(p->*func)();
	}
	virtual PopupMenuItem *clone(void)const{return new PopupMenuItemFunctionT(*this);}
};

template<typename T, typename TA>
struct PopupMenuItemFunctionArgT : public PopupMenuItem{
	typedef PopupMenuItem st;
	T *p;
	TA arg;
	void (T::*func)(TA);
	PopupMenuItemFunctionArgT(cpplib::dstring title, T *p, void (T::*func)(TA), TA arg) : st(title), p(p), func(func), arg(arg){}
	virtual void execute(){
		(p->*func)(arg);
	}
	virtual PopupMenuItem *clone(void)const{return new PopupMenuItemFunctionArgT(*this);}
};

int GLWentlist::mouse(GLwindowState &ws, int button, int state, int mx, int my){
	// Ensure the player is present. There can be no player in the game.
	Player *player = getPlayer();
	if(!player)
		return 0;
	Player &pl = *player;
	GLWrect r = clientRect();
	ClassItemSelector cis(this);
	ExpandItemSelector eis(this);
	ItemSelector &is = groupByClass ? (ItemSelector&)cis : (ItemSelector&)eis;
	int itemCount = is.count();

	ListItemLocator lil(this, itemCount);
	IconItemLocator eil(this, itemCount);
	ItemLocator &il = icons ? (ItemLocator&)eil : (ItemLocator&)lil;

	if(r.width() - 10 < mx && button == GLUT_LEFT_BUTTON && (state == GLUT_DOWN || state == GLUT_KEEP_DOWN || state == GLUT_UP) && r.height() < il.allHeight()){
		int iy = glwVScrollBarMouse(this, mx, my, r.width() - 10, 0, 10, r.height(), il.allHeight() - r.height(), scrollpos);
		if(0 <= iy)
			scrollpos = iy;
		return 1;
	}
	if(button == GLUT_LEFT_BUTTON && state == GLUT_UP && switches){
		GLWrect r2 = r;
		r2.y0 = 0;
		r2.y1 = long(getFontHeight());
		if(r2.include(mx + r.x0, my + r.y0)){
			int index = mx * 5 / r2.width();
			if(index < 3)
				listmode = MIN(Universe, MAX(Select, ListMode(index)));
			else if(index == 3)
				groupByClass = !groupByClass;
			else
				icons = !icons;
			return 1;
		}
	}
	else if(button == GLUT_RIGHT_BUTTON && state == GLUT_UP){
		typedef PopupMenuItemFunctionT<GLWentlist> PopupMenuItemFunction;
#define APPENDER(a,s) ((a) ? "* " s : "  " s)

		glwPopupMenu(ws, PopupMenu()
			.append(new PopupMenuItemFunction(APPENDER(listmode == Select, "Show Selected"), this, &GLWentlist::menu_select))
			.append(new PopupMenuItemFunction(APPENDER(listmode == All, "Show All"), this, &GLWentlist::menu_all))
			.append(new PopupMenuItemFunction(APPENDER(listmode == Universe, "Show Universally"), this, &GLWentlist::menu_universe))
			.appendSeparator()
			.append(new PopupMenuItemFunction(APPENDER(groupByClass, "Group by class"), this, &GLWentlist::menu_grouping))
			.append(new PopupMenuItemFunction(APPENDER(icons, "Show icons"), this, &GLWentlist::menu_icons))
			.append(new PopupMenuItemFunction(APPENDER(teamOnly, "Show allies only"), this, &GLWentlist::menu_team))
			.appendSeparator()
			.append(new PopupMenuItemFunction(APPENDER(switches, "Show quick switches"), this, &GLWentlist::menu_switches))
			.appendSeparator()
			.append(new PopupMenuItemFunction("Edit search criteria...", this, &GLWentlist::menu_criteria)));
	}
	if(button == GLUT_LEFT_BUTTON && (state == GLUT_KEEP_UP || state == GLUT_UP)){
		int i;
		bool set = false;
		if(r.include(mx + r.x0, my + r.y0) && mx < r.width() - 10) for(is.begin(), il.begin(), i = 0; i < itemCount; i++, is.next(), il.next()){
			GLWrect itemRect = il.getRect().rmove(0, -scrollpos + switches * int(getFontHeight()));
			if(itemRect.include(mx + r.x0, my + r.y0)){
				const Entity *pe = is.get();
				if(pe){
					if(state == GLUT_UP){
						class SelectorProc : public ItemSelector::ForEachProc{
						public:
							Player::SelectSet &set;
//							Entity **prev;
//							SelectorProc(Entity **prev) : prev(prev){}
							SelectorProc(Player::SelectSet &set) : set(set){}
							virtual void proc(const Entity *pe){
								set.insert(const_cast<Entity*>(pe));
/*								*prev = pe;
								prev = &pe->selectnext;
								pe->selectnext = NULL;*/
							}
						};
						is.foreach(SelectorProc(pl.selected));
					}
					else{
						int xs, ys;
						const long margin = 4;
						cpplib::dstring str = cpplib::dstring(pe->dispname()) << "\nrace = " << pe->race << "\nhealth = " << pe->health;
						glwGetSizeStringML(str, GLwindow::glwfontheight, &xs, &ys);
						GLWrect localrect = GLWrect(itemRect.x0, itemRect.y0 - ys - 3 * margin, itemRect.x0 + xs + 3 * margin, itemRect.y0);

						// Adjust rect to fit in the screen. No care is taken if tips window is larger than the screen.
						if(ws.w < localrect.x1)
							localrect.rmove(ws.w - localrect.x1, 0);
						if(ws.h < localrect.y1)
							localrect.rmove(0, ws.h - localrect.y1);

						glwtip->setExtent(localrect);
						glwtip->tips = str;
						glwtip->parent = this;
						glwtip->setVisible(true);
						glwActivate(glwFindPP(glwtip));
						set = true;
					}
				}
				i = n;
				break;
			}
		}
		else
			mouseLeave(ws);
		if(!set){
			if(glwtip->parent == this){
				glwtip->tips = NULL;
				glwtip->parent = NULL;
				glwtip->setVisible(false);
			}
		}
	}
	return st::mouse(ws, button, state, mx, my);
}

void GLWentlist::mouseLeave(GLwindowState &ws){
	if(glwtip->parent == this){
		glwtip->tips = NULL;
		glwtip->parent = NULL;
		glwtip->setExtent(GLWrect(-10,-10,-10,-10));
	}
}



struct ItemCriterion::DrawParams{
	int x;
	int y;
};

struct ItemCriterion::MouseParams{
	int x;
	int y;
	int key;
	int state;
	int mx;
	int my;
};

class BinaryOpItemCriterion : public ItemCriterion{
public:
	ItemCriterion *left, *right;
	enum Op{And, Or, Xor, NumOp} op;

	BinaryOpItemCriterion(GLWentlist &entlist) : ItemCriterion(entlist), left(NULL), right(NULL), op(And){}
	static const ClassId sid;
	virtual ClassId id()const{return "BinaryOpItemCriterion";}
	bool match(const Entity *e)const{
		if(op == And)
			return left->match(e) && right->match(e);
		else if(op == Or)
			return left->match(e) || right->match(e);
		else
			return left->match(e) ^ right->match(e);
	}

	int mouse(GLwindowState &ws, GLWentlistCriteria *p, MouseParams &mp);
	void draw(GLwindowState &ws, GLWentlistCriteria *p, DrawParams &dp);
	virtual ItemCriterion *deleteNode(const ItemCriterion *node);
	void alterNode(const ItemCriterion *from, ItemCriterion *to);
};
const ClassId BinaryOpItemCriterion::sid = "ItemCriterionNot";

class GLWentlistCriteria : public GLwindowSizeable{
public:
	GLWentlist *p;
	GLWentlistCriteria(GLWentlist *p) : p(p), GLwindowSizeable("Entity List Criteria"){
		width = 450;
		height = 300;
		setClosable(true);
	}
	void draw(GLwindowState &ws, double t){
		ItemCriterion::DrawParams dp;
		dp.x = 0;
		dp.y = 0;
		if(p->crtRoot){
			p->crtRoot->draw(ws, this, dp);
		}
		GLWrect cr = clientRect();

		glColor4f(1,1,1,1);
		glwpos2d(cr.x0, cr.y0 + dp.y + lineHeight());
		glwprintf("Add criterion...");
	}

	int mouse(GLwindowState &ws, int key, int state, int mx, int my);
	int lineHeight()const{return int(getFontHeight() * 1.5);}
	template<typename T> void addCriterion(){
		if(!p->crtRoot)
			p->crtRoot = new T(*p);
		else{
			BinaryOpItemCriterion *node = new BinaryOpItemCriterion(*p);
			node->left = p->crtRoot;
			node->right = new T(*p);
			p->crtRoot = node;
		}
	}
	/// Deriving PopupMenuItem directly to create a class like this looks more straightforward than PopupMenuItemFunctionT,
	/// but it certainly eats more code size through templating.
	template<typename T> class MenuItemInsertCriterion : public PopupMenuItem{
	public:
		typedef PopupMenuItem st;
		ItemCriterion *node;
		MenuItemInsertCriterion(cpplib::dstring title, ItemCriterion *p) : st(title), node(p){}
		T *creator(){return new T(node->entlist);}
		virtual void execute(){
			if(!node->entlist.crtRoot)
				return;
			BinaryOpItemCriterion *newnode = new BinaryOpItemCriterion(node->entlist);
			newnode->left = node;
			newnode->right = creator();
			if(node->entlist.crtRoot == node)
				node->entlist.crtRoot = newnode;
			else
				node->entlist.crtRoot->alterNode(node, newnode);
		}
		virtual PopupMenuItem *clone(void)const{return new MenuItemInsertCriterion(*this);}
	};
};

void GLWentlist::menu_criteria(){
	glwAppend(new GLWentlistCriteria(this));
}

/// Negation operator.
class ItemCriterionNot : public ItemCriterion{
public:
	ItemCriterion *crt;
	ItemCriterionNot(GLWentlist &entlist, ItemCriterion *crt) : ItemCriterion(entlist), crt(crt){}
	static const ClassId sid;
	virtual ClassId id()const{return sid;}
	bool match(const Entity *e)const{
		return !crt->match(e);
	}
	void draw(GLwindowState &ws, GLWentlistCriteria *p, DrawParams &dp){
		GLWrect cr = p->clientRect();
		glColor4f(1,1,1,1);
		glwpos2d(cr.x0 + dp.x, cr.y0 + dp.y + p->lineHeight());
		glwprintf("! Not");
		dp.y += p->lineHeight();

		DrawParams dp2 = dp;
		dp2.x += p->getFontWidth();
		crt->draw(ws, p, dp2);

		glColor4f(1,1,1,1);
		glBegin(GL_LINE_STRIP);
		glVertex2i(cr.x0 + dp.x + p->getFontWidth() / 2, cr.y0 + dp.y);
		glVertex2i(cr.x0 + dp.x + p->getFontWidth() / 2, cr.y0 + dp2.y);
		glVertex2i(cr.x0 + dp.x + p->getFontWidth(), cr.y0 + dp2.y);
		glEnd();

		dp.y = dp2.y;
	}
	int mouse(GLwindowState &ws, GLWentlistCriteria *p, MouseParams &mp){
		if(int ret = ItemCriterion::mouse(ws, p, mp))
			return ret;
//		mp.y += p->lineHeight();
		MouseParams mp2 = mp;
		mp2.x += p->getFontWidth();
		int ret = crt->mouse(ws, p, mp2);
		mp.y = mp2.y;
		return ret;
	}
	ItemCriterion *deleteNode(const ItemCriterion *node){
		crt = crt->deleteNode(node);
		if(!crt){
			delete this;
			return NULL;
		}
		else if(node == this){ // Return subtree if this node is to be deleted.
			ItemCriterion *ret = crt;   // Reserve pointer to subnode before deleting this.
			delete this;
			return ret;
		}
		return this;
	}
	void alterNode(const ItemCriterion *from, ItemCriterion *to){
		if(crt == from)
			crt = to;
		else
			crt->alterNode(from, to);
	}
};
const ClassId ItemCriterionNot::sid = "ItemCriterionNot";

/// Criterion with no sub-criterions like binary and unary operators, meaning a leaf in the tree.
class LeafItemCriterion : public ItemCriterion{
public:
	LeafItemCriterion(GLWentlist &entlist) : ItemCriterion(entlist){}
	static const ClassId sid;
	virtual ClassId id()const{return sid;}
	virtual cpplib::dstring text()const = 0;
	virtual void draw(GLwindowState &ws, GLWentlistCriteria *p, DrawParams &dp){
		GLWrect cr = p->clientRect();
		glColor4f(.25, .25, .25, .5);
		glBegin(GL_QUADS);
		glVertex2i(cr.x0 + dp.x + p->getFontWidth() + 4, cr.y0 + dp.y + p->lineHeight() - p->getFontHeight());
		glVertex2i(cr.x0 + dp.x + p->getFontWidth() + 4, cr.y0 + dp.y + p->lineHeight());
		glVertex2i(cr.x1 - p->getFontWidth() - 4, cr.y0 + dp.y + p->lineHeight());
		glVertex2i(cr.x1 - p->getFontWidth() - 4, cr.y0 + dp.y + p->lineHeight() - p->getFontHeight());
		glEnd();

		glColor4f(1,1,1,1);
		glwpos2d(cr.x0 + dp.x + p->getFontWidth() + 4, cr.y0 + dp.y + p->lineHeight());
		glwprintf(text());
		ItemCriterion::draw(ws, p, dp);
	}
	template<typename Criterion, typename T, T Criterion::*member> class PopupMenuItemCriterion : public PopupMenuItem{
	public:
		typedef PopupMenuItem st;
		Criterion *p;
		T src;
		PopupMenuItemCriterion(gltestp::dstring title, Criterion *p, T src) : p(p), src(src){
			this->title = title;
		}
		virtual void execute(){
			p->*member = src;
		}
		virtual PopupMenuItem *clone(void)const{return new PopupMenuItemCriterion<Criterion, T, member>(*this);}
	};
};
const ClassId LeafItemCriterion::sid = "LeafItemCriterion";

/// Matches if a given Entity is selected by the player.
class SelectedItemCriterion : public LeafItemCriterion{
public:
	Player &pl;
	SelectedItemCriterion(GLWentlist &entlist) : pl(*entlist.getPlayer()), LeafItemCriterion(entlist){}
	virtual bool match(const Entity *e)const{
		Player::SelectSet::iterator it = pl.selected.begin();
		for(; it != pl.selected.end(); it++) if(*it == e)
			 return true;
		return false;
	}
	virtual cpplib::dstring text()const{return "Selected";}
};

/// Matches an Entity class.
class ClassItemCriterion : public LeafItemCriterion{
public:
	typedef ClassItemCriterion tt;
	const char *cid;
	ClassItemCriterion(GLWentlist &entlist) : cid(NULL), LeafItemCriterion(entlist){}
	virtual bool match(const Entity *e)const{
		return !cid || cid == e->classname();
	}
	virtual cpplib::dstring text()const{return cpplib::dstring() << "ClassName: " << (cid ? cid : "Unspecified");}
	virtual int mouse(GLwindowState &ws, GLWentlistCriteria *p, MouseParams &mp){
		int y0 = mp.y;
		int y1 = mp.y + p->lineHeight();
		if(int ret = ItemCriterion::mouse(ws, p, mp))
			return ret;
		GLWrect cr = p->clientRect();
		if(mp.key == GLUT_LEFT_BUTTON && mp.state == GLUT_UP && y0 < mp.my && mp.my < y1){
			PopupMenu pm;
			for(Entity::EntityCtorMap::const_iterator it = Entity::constEntityCtorMap().begin(); it != Entity::constEntityCtorMap().end(); it++)
				pm.append(new PopupMenuItemCriterion<tt, const char *, &tt::cid>(it->first, this, it->first));
			glwPopupMenu(ws, pm);
			return 1;
		}
		return 0;
	}
};

/// Matches if given Entity belongs to a team.
class TeamItemCriterion : public LeafItemCriterion{
public:
	typedef TeamItemCriterion tt;
	int team;
	TeamItemCriterion(GLWentlist &entlist) : team(-1), LeafItemCriterion(entlist){}
	virtual bool match(const Entity *e)const{
		return team == -1 || team == e->race;
	}
	virtual cpplib::dstring text()const{return
		team == -1 ? "Team: Unspecified" :
		cpplib::dstring() << "Team: " << team;
	}
	virtual int mouse(GLwindowState &ws, GLWentlistCriteria *p, MouseParams &mp){
		int y0 = mp.y;
		int y1 = mp.y + p->lineHeight();
		if(int ret = ItemCriterion::mouse(ws, p, mp))
			return ret;
		if(mp.key == GLUT_LEFT_BUTTON && mp.state == GLUT_UP && y0 < mp.my && mp.my < y1){
			PopupMenu pm;
			pm.append(new PopupMenuItemCriterion<tt, int, &tt::team>(gltestp::dstring() << "Unspecified", this, -1));
			for(int i = 0; i < 4; i++)
				pm.append(new PopupMenuItemCriterion<tt, int, &tt::team>(gltestp::dstring() << "Team " << i, this, i));
			glwPopupMenu(ws, pm);
			return 1;
		}
		return 0;
	}
};

/// Matches if given Entity belongs to a CoordSys.
class CoordSysItemCriterion : public LeafItemCriterion{
public:
	typedef CoordSysItemCriterion tt;
	const CoordSys *cs;
	CoordSysItemCriterion(GLWentlist &entlist) : cs(NULL), LeafItemCriterion(entlist){}
	virtual bool match(const Entity *e)const{
		return !cs || e->w->cs == cs;
	}
	virtual cpplib::dstring text()const{return cpplib::dstring() << "CoordSys: " << (cs ? cs->getpath() : "Unspecified");}
	virtual int mouse(GLwindowState &ws, GLWentlistCriteria *p, MouseParams &mp){
		int y0 = mp.y;
		int y1 = mp.y + p->lineHeight();
		if(int ret = ItemCriterion::mouse(ws, p, mp))
			return ret;
		if(mp.key == GLUT_LEFT_BUTTON && mp.state == GLUT_UP && y0 < mp.my && mp.my < y1){
			PopupMenu pm;
			const CoordSys *root = p->p->getPlayer()->cs->findcspath("/");
			mouseint(root, pm);
			glwPopupMenu(ws, pm);
			return 1;
		}
		return 0;
	}
protected:
	/// Searches through all CoordSys that has a WarSpace.
	void mouseint(const CoordSys *cs, PopupMenu &pm){
		typedef PopupMenuItemCriterion<tt, const CoordSys *, &tt::cs> PopupMenuItemCS;
		if(!cs) return;
		if(cs->w)
			pm.append(new PopupMenuItemCS((const char*)cs->getpath(), this, cs));
		mouseint(cs->next, pm);
		mouseint(cs->children, pm);
	}
};


// ------------------------------
// Implementation
// ------------------------------


void ItemCriterion::draw(GLwindowState &ws, GLWentlistCriteria *p, DrawParams &dp){
	GLWrect cr = p->clientRect();
	int y0 = cr.y0 + dp.y + p->lineHeight() - p->getFontHeight();

	glColor4f(1,1,1,1);
	glBegin(GL_LINES);
	glVertex2i(cr.x1 - p->getFontHeight() * 3 / 2, y0 + 2);
	glVertex2i(cr.x1 - p->getFontHeight() * 3 / 2, y0 + p->getFontHeight() - 1);
	glVertex2i(cr.x1 - p->getFontHeight() * 2 + 1, y0 + p->getFontHeight() / 2);
	glVertex2i(cr.x1 - p->getFontHeight() * 1 - 2, y0 + p->getFontHeight() / 2);

	glVertex2i(cr.x1 - p->getFontHeight() + 2, y0 + 2);
	glVertex2i(cr.x1 - 2, y0 + p->getFontHeight() - 2);
	glVertex2i(cr.x1 - p->getFontHeight() + 2, y0 + p->getFontHeight() - 2);
	glVertex2i(cr.x1 - 2, y0 + 2);
	glEnd();

	dp.y += p->lineHeight();
}

int ItemCriterion::mouse(GLwindowState &ws, GLWentlistCriteria *p, MouseParams &mp){
	GLWrect cr = p->clientRect().move(0, 0);

	cr.y0 = mp.y;
	cr.y1 = mp.y += p->lineHeight();
	if(cr.include(mp.mx, mp.my)){
		if(mp.key == GLUT_LEFT_BUTTON && mp.state == GLUT_UP){
			if(mp.mx < mp.x + p->getFontHeight()){
				// Nested NOT operators will automatically collapse to nothing.
				if(this->id() == ItemCriterionNot::sid)
					p->p->crtRoot = p->p->crtRoot->deleteNode(this);
				else{
					ItemCriterionNot *not = new ItemCriterionNot(entlist, this);
					if(p->p->crtRoot == this)
						p->p->crtRoot = not;
					else
						p->p->crtRoot->alterNode(this, not);
				}
				return 1; // Be sure to return 1 here or access violation will occur
			}
			if(cr.x1 - p->getFontHeight() * 2 < mp.mx && mp.mx < cr.x1 - p->getFontHeight()){
				glwPopupMenu(ws, PopupMenu()
/*					.append(new GLWentlistCriteria::MenuItemInsertCriterion<SelectedItemCriterion>("Selected", this))
					.append(new GLWentlistCriteria::MenuItemInsertCriterion<ClassItemCriterion>("Class Name", this))
					.append(new GLWentlistCriteria::MenuItemInsertCriterion<TeamItemCriterion>("Team", this))
					.append(new GLWentlistCriteria::MenuItemInsertCriterion<CoordSysItemCriterion>("CoordSys", this))*/
					.append(new PopupMenuItemFunctionT<ItemCriterion>("Selected", this, &ItemCriterion::insertNode<SelectedItemCriterion>))
					.append(new PopupMenuItemFunctionT<ItemCriterion>("Class Name", this, &ItemCriterion::insertNode<ClassItemCriterion>))
					.append(new PopupMenuItemFunctionT<ItemCriterion>("Team", this, &ItemCriterion::insertNode<TeamItemCriterion>))
					.append(new PopupMenuItemFunctionT<ItemCriterion>("CoordSys", this, &ItemCriterion::insertNode<CoordSysItemCriterion>))
				);
				return 1;
			}
			else if(cr.x1 - p->getFontHeight() < mp.mx){
				p->p->crtRoot = p->p->crtRoot->deleteNode(this);
				return 1; // Be sure to return 1 here or access violation will occur
			}
		}
		return ItemCriterion::mouse(ws, p, mp);
	}
	return 0;
}

template<typename T> void ItemCriterion::insertNode(){
	if(!entlist.crtRoot)
		return;
	BinaryOpItemCriterion *newnode = new BinaryOpItemCriterion(entlist);
	newnode->left = this;
	newnode->right = new T(entlist);
	if(entlist.crtRoot == this)
		entlist.crtRoot = newnode;
	else
		entlist.crtRoot->alterNode(this, newnode);
}



int GLWentlistCriteria::mouse(GLwindowState &ws, int key, int state, int mx, int my){
	ItemCriterion::MouseParams mp;
	mp.x = 0;
	mp.y = 0;
	mp.key = key;
	mp.state = state;
	mp.mx = mx;
	mp.my = my;

	if(p->crtRoot){
		int ret = p->crtRoot->mouse(ws, this, mp);
		if(ret)
			return ret;
	}
	GLWrect cr = clientRect().move(0, 0);
	cr.y0 = mp.y;
	cr.y1 = cr.y0 + lineHeight();
	if(cr.include(mx, my) && key == GLUT_LEFT_BUTTON && state == GLUT_UP){
		glwPopupMenu(ws, PopupMenu()
			.append(new PopupMenuItemFunctionT<GLWentlistCriteria>("Selected", this, &GLWentlistCriteria::addCriterion<SelectedItemCriterion>))
			.append(new PopupMenuItemFunctionT<GLWentlistCriteria>("Class Name", this, &GLWentlistCriteria::addCriterion<ClassItemCriterion>))
			.append(new PopupMenuItemFunctionT<GLWentlistCriteria>("Team", this, &GLWentlistCriteria::addCriterion<TeamItemCriterion>))
			.append(new PopupMenuItemFunctionT<GLWentlistCriteria>("CoordSys", this, &GLWentlistCriteria::addCriterion<CoordSysItemCriterion>))
		);
		return 1;
	}
	return 0;
}

void BinaryOpItemCriterion::draw(GLwindowState &ws, GLWentlistCriteria *p, DrawParams &dp){
	GLWrect cr = p->clientRect();
	DrawParams dp2 = dp;
	dp2.x += p->getFontWidth();

	glColor4f(1,1,1,1);
	glwpos2d(cr.x0 + dp.x, cr.y0 + dp2.y + p->lineHeight());
	glwprintf(op == And ? "& And" : op == Or ? "| Or" : op == Xor ? "^ Xor" : "?");
	ItemCriterion::draw(ws, p, dp2);

	left->draw(ws, p, dp2);

	right->draw(ws, p, dp2);

	glColor4f(1,1,1,1);
	glBegin(GL_LINE_STRIP);
	glVertex2i(cr.x0 + dp.x + p->getFontWidth() / 2, cr.y0 + dp.y + p->lineHeight());
	glVertex2i(cr.x0 + dp.x + p->getFontWidth() / 2, cr.y0 + dp2.y);
	glVertex2i(cr.x0 + dp.x + p->getFontWidth(), cr.y0 + dp2.y);
	glEnd();

	dp.y = dp2.y;
}

int BinaryOpItemCriterion::mouse(GLwindowState &ws, GLWentlistCriteria *p, MouseParams &mp){
	MouseParams mp2 = mp;
	mp2.x += p->getFontWidth();
	GLWrect cr = p->clientRect().move(0, 0);
	cr.y0 = mp2.y;
	cr.y1 = mp2.y + p->lineHeight();
	int ret = ItemCriterion::mouse(ws, p, mp2);
	if(ret)
		return ret;
	if(cr.include(mp.mx, mp.my)){
		if(mp.key == GLUT_LEFT_BUTTON && mp.state == GLUT_UP/* && mp.mx < mp.x + 2 * p->getFontHeight()*/){
			op = Op((op + 1) % NumOp);
			return 1;
		}
	}

	ret = left->mouse(ws, p, mp2);
	if(ret)
		return ret;

	ret = right->mouse(ws, p, mp2);
	if(ret)
		return ret;

	mp.y = mp2.y;
	return 0;
}

ItemCriterion *BinaryOpItemCriterion::deleteNode(const ItemCriterion *node){
	if(node == this){
		left->deleteNode(left);   // Avoid
		right->deleteNode(right); // memory
		delete this;              // leaks
		return NULL;
	}
	left = left->deleteNode(node);
	right = right->deleteNode(node);

	// Wonder if this case exist
	if(!left && !right){
		delete this;
		return NULL;
	}

	// If either one of operands are deleted, the binary operator is unnecessary too.
	if(!left || !right){
		ItemCriterion *ret = left ? left : right;
		delete this;
		return ret;
	}
	return this;
}

void BinaryOpItemCriterion::alterNode(const ItemCriterion *from, ItemCriterion *to){
	if(left == from)
		left = to;
	else
		left->alterNode(from, to);
	if(right == from)
		right = to;
	else
		right->alterNode(from, to);
}




// ------------------------------
// Squirrel Initializer Implementation
// ------------------------------

static SQInteger sqf_GLWentlist_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	Game *game = (Game*)sq_getforeignptr(v);
	if(!game)
		return sq_throwerror(v, _SC("The game object is not assigned"));
	GLWentlist *p = new GLWentlist(game);
	if(!sqa_newobj(v, p, 1))
		return SQ_ERROR;
	glwAppend(p);
	return 0;
}

static SQInteger sqf_screenwidth(HSQUIRRELVM v){
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	sq_pushinteger(v, vp[2] - vp[0]);
	return 1;
}

static SQInteger sqf_screenheight(HSQUIRRELVM v){
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	sq_pushinteger(v, vp[3] - vp[1]);
	return 1;
}


bool GLWentlist::sq_define(HSQUIRRELVM v){
	// Define class GLWentlist
	sq_pushstring(v, _SC("GLWentlist"), -1);
	sq_pushstring(v, _SC("GLwindow"), -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	register_closure(v, _SC("constructor"), sqf_GLWentlist_constructor);
	sq_createslot(v, -3);

	sq_pushstring(v, _SC("screenwidth"), -1);
	sq_newclosure(v, sqf_screenwidth, 0);
	sq_createslot(v, 1);
	sq_pushstring(v, _SC("screenheight"), -1);
	sq_newclosure(v, sqf_screenheight, 0);
	sq_createslot(v, 1);
	return true;
}
