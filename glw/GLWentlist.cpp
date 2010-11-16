/** \file
 * \brief Implementation of GLWentlist class.
 */
#include "GLWentlist.h"
#include "GLWtip.h"
#include "GLWmenu.h"
#include "../antiglut.h"
#include "../Player.h"
#include "../war.h"
extern "C"{
#include <clib/timemeas.h>
}
#include <string.h>
#include <algorithm>

/// Set nonzero to measure sorting speed
#define PROFILE_SORT 0




static bool pents_pred(std::vector<Entity*> *a, std::vector<Entity*> *b){
	// Comparing returned pointers themselves by dispname() would be enough for sorting, but it's better
	// that the player can see items sorted alphabetically.
	return strcmp((*a)[0]->dispname(), (*b)[0]->dispname()) < 0;
}


GLWentlist::GLWentlist(Player &player) :
	st("Entity List"),
	pl(player),
	listmode(Select),
	groupByClass(true),
	icons(true),
	switches(false),
	teamOnly(false),
	scrollpos(0),
	crtRoot(NULL){}

void GLWentlist::draw_int(const CoordSys *cs, int &n, std::vector<Entity*> ents[]){
	if(!cs)
		return;
	for(const CoordSys *cs1 = cs->children; cs1; cs1 = cs1->next)
		draw_int(cs1, n, ents);
	if(!cs->w)
		return;
	Entity *pt;
	for(pt = cs->w->el; pt; pt = pt->next){

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
	std::vector<Entity*> *(&pents)[GLWentlist::OV_COUNT];
	int n;
	ItemSelector(GLWentlist *p) : pents(p->pents), n(p->n){}
	virtual int count() = 0; ///< Returns count of displayed items
	virtual bool begin() = 0; ///< Starts enumeration of displayed items
	virtual Entity *get() = 0; ///< Gets current item, something like STL's iterator::operator* (unary)
	virtual bool next() = 0; ///< Returns true if there are more items to go
	virtual bool stack() = 0; ///< If items are stackable
	virtual int countInGroup() = 0; ///< Count stacked items

	/// Callback class that is to be derived to define procedure for each item. Used with foreach().
	class ForEachProc{
	public:
		virtual void proc(Entity*) = 0;
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
	virtual Entity *get(){return (*pents[i])[0];}
	virtual bool next(){return ++i < n;}
	virtual bool stack(){return true;}
	virtual int countInGroup(){return pents[i]->size();}
	virtual void foreach(ForEachProc &proc){
		std::vector<Entity*>::iterator it = pents[i]->begin();
		for(; it != pents[i]->end(); it++)
			proc.proc(*it);
	}
};

/// No grouping whatsoever
class ExpandItemSelector : public ItemSelector{
public:
	int i;
	std::vector<Entity*>::iterator it;
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
	virtual Entity *get(){
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
	const WarField *const w = pl.cs->w;
	int i;

	n = 0;
	for(i = 0; i < OV_COUNT; i++)
		ents[i].clear();

	if(listmode == Select){
		Entity *pt;
		for(pt = pl.selected; pt; pt = pt->selectnext){

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
		Entity *pt;
		for(pt = w->el; pt; pt = pt->next){

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
		draw_int(pl.cs->findcspath("/"), n, ents);
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
		Entity *pe = is.get();
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
			PlayerSelection(GLWentlist *p) : ret(false), pl(p->pl){}
			virtual void proc(Entity *pe){
				for(Entity *pe2 = pl.selected; pe2; pe2 = pe2->selectnext) if(pe2 == pe){
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
				pe->drawOverlay(NULL);
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
	PopupMenuItemFunctionT(cpplib::dstring title, T *p, void (T::*func)()) : p(p), func(func){
		this->title = title;
	}
	virtual void execute(){
		(p->*func)();
	}
	virtual PopupMenuItem *clone(void)const{return new PopupMenuItemFunctionT(*this);}
};

int GLWentlist::mouse(GLwindowState &ws, int button, int state, int mx, int my){
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
				Entity *pe = is.get();
				if(pe){
					if(state == GLUT_UP){
						class SelectorProc : public ItemSelector::ForEachProc{
						public:
							Entity **prev;
							SelectorProc(Entity **prev) : prev(prev){}
							virtual void proc(Entity *pe){
								*prev = pe;
								prev = &pe->selectnext;
								pe->selectnext = NULL;
							}
						};
						is.foreach(SelectorProc(&pl.selected));
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

class GLWentlistCriteria : public GLwindowSizeable{
public:
	typedef std::vector<ItemCriterion*> ItemCriterionList;
	GLWentlist *p;
//	ItemCriterionList crt;
	GLWentlistCriteria(GLWentlist *p) : p(p), GLwindowSizeable("Entity List Criteria"){
		width = 450;
		height = 300;
		setClosable(true);
	}
	~GLWentlistCriteria(){
//		for(int i = 0; i < crt.size(); i++)
//			delete crt[i];
	}
	void drawint(GLwindowState &ws, CriterionNode *crt, int *py){
		if(crt->crt){
			crt->crt->draw(ws, this, *py);
			*py += lineHeight();
		}
		else{
			drawint(ws, crt->left, py);
			drawint(ws, crt->right, py);
		}
	}
	void draw(GLwindowState &ws, double t){
		int y = 0;
		if(p->crtRoot)
			p->crtRoot->draw(ws, this, y);
		GLWrect cr = clientRect();

		glColor4f(1,1,1,1);
		glwpos2d(cr.x0, cr.y0 + y + getFontHeight());
		glwprintf("Add criterion...");
	}
	int mouseint(GLwindowState &ws, CriterionNode *crt, int *py, int key, int state, int mx, int my){
		if(crt->crt){
			GLWrect ir = clientRect();
			ir.y0 = *py;
			ir.y1 = *py += lineHeight();
			if(ir.include(mx, my))
				return crt->crt->mouse(ws, this, key, state, mx, my);
		}
		else{
			int ret = mouseint(ws, crt->left, py, key, state, mx, my);
			if(ret)
				return ret;
			ret = mouseint(ws, crt->right, py, key, state, mx, my);
			if(ret)
				return ret;
		}
		return 0;
	}

	template<typename T>
	void foreachCriterionInt(void (*proc)(T), T param, CriterionNode *crt){
		if(crt->crt){
			proc(crt->crt);
		}
		else{
			foreachCriterionInt(proc, param, crt->left);
			foreachCriterionInt(proc, param, crt->right);
		}
	}

	template<typename T>
	void foreachCriterion(void (*proc)(T), T param){
		foreachCriterionInt(proc, param, p->crtRoot);
	}

	int mouse(GLwindowState &ws, int key, int state, int mx, int my);
	int lineHeight()const{return int(getFontHeight() * 1.2);}
	template<typename T> void addCriterion(){
		if(!p->crtRoot){
			p->crtRoot = new CriterionNode();
			p->crtRoot->crt = new T(*p);
			p->crtRoot->left = p->crtRoot->right = NULL;
		}
		else{
			CriterionNode *rightmost = p->crtRoot;
			while(rightmost->right)
				rightmost = rightmost->right;
			CriterionNode *left = new CriterionNode(*rightmost);
			rightmost->crt = NULL;
			rightmost->left = left;
			rightmost->right = new CriterionNode();
			rightmost->right->crt = new T(*p);
			rightmost->right->left = rightmost->right->right = NULL;
		}
	}
};

void GLWentlist::menu_criteria(){
	glwAppend(new GLWentlistCriteria(this));
}

class SelectedItemCriterion : public ItemCriterion{
public:
	Player &pl;
	SelectedItemCriterion(GLWentlist &entlist) : pl(entlist.pl), ItemCriterion(entlist){}
	virtual bool match(const Entity *e)const{
		for(Entity *pt = pl.selected; pt; pt = pt->selectnext) if(pt == e)
			 return true;
		return false;
	}
	virtual void draw(GLwindowState &ws, GLWentlistCriteria *p, int y){
		GLWrect cr = p->clientRect();
		glColor4f(.25, .25, .25, .5);
		glBegin(GL_QUADS);
		glVertex2i(cr.x0 + p->getFontHeight() + 4, cr.y0 + y + p->lineHeight() - p->getFontHeight());
		glVertex2i(cr.x0 + p->getFontHeight() + 4, cr.y0 + y + p->getFontHeight());
		glVertex2i(cr.x1 - p->getFontHeight() - 4, cr.y0 + y + p->getFontHeight());
		glVertex2i(cr.x1 - p->getFontHeight() - 4, cr.y0 + y + p->lineHeight() - p->getFontHeight());
		glEnd();

		glColor4f(1,1,1,1);
		glwpos2d(cr.x0 + p->getFontHeight() + 4, cr.y0 + y + p->lineHeight());
		glwprintf("Selected");
		ItemCriterion::draw(ws, p, y);
	}
};

class ClassItemCriterion : public ItemCriterion{
public:
	const char *cid;
	ClassItemCriterion(GLWentlist &entlist) : cid(NULL), ItemCriterion(entlist){}
	virtual bool match(const Entity *e)const{
		return cid == e->classname();
	}
	virtual void draw(GLwindowState &ws, GLWentlistCriteria *p, int y){
		GLWrect cr = p->clientRect();
		glColor4f(.25, .25, .25, .5);
		glBegin(GL_QUADS);
		glVertex2i(cr.x0 + p->getFontHeight() + 4, cr.y0 + y + p->lineHeight() - p->getFontHeight());
		glVertex2i(cr.x0 + p->getFontHeight() + 4, cr.y0 + y + p->getFontHeight());
		glVertex2i(cr.x1 - p->getFontHeight() - 4, cr.y0 + y + p->getFontHeight());
		glVertex2i(cr.x1 - p->getFontHeight() - 4, cr.y0 + y + p->lineHeight() - p->getFontHeight());
		glEnd();

		glColor4f(1,1,1,1);
		glwpos2d(cr.x0 + p->getFontHeight() + 4, cr.y0 + y + p->lineHeight());
		glwprintf(cpplib::dstring() << "ClassName: " << (cid ? cid : "Unspecified"));
		ItemCriterion::draw(ws, p, y);
	}
	virtual int mouse(GLwindowState &ws, GLWentlistCriteria *p, int key, int state, int mx, int my){
		if(int ret = ItemCriterion::mouse(ws, p, key, state, mx, my))
			return ret;
		if(key == GLUT_LEFT_BUTTON && state == GLUT_UP){
			struct PopupMenuItemClass : public PopupMenuItem{
				typedef PopupMenuItem st;
				ClassItemCriterion *p;
				const char *cname;
				PopupMenuItemClass(cpplib::dstring title, ClassItemCriterion *p, const char *cname) : p(p), cname(cname){
					this->title = title;
				}
				virtual void execute(){
					p->cid = cname;
				}
				virtual PopupMenuItem *clone(void)const{return new PopupMenuItemClass(*this);}
			};

			PopupMenu pm;
			for(Entity::EntityCtorMap::const_iterator it = Entity::constEntityCtorMap().begin(); it != Entity::constEntityCtorMap().end(); it++)
				pm.append(new PopupMenuItemClass(it->first, this, it->first));
			glwPopupMenu(ws, pm);
			return 1;
		}
		return 0;
	}
};

class TeamItemCriterion : public ItemCriterion{
public:
	int team;
	TeamItemCriterion(GLWentlist &entlist) : team(-1), ItemCriterion(entlist){}
	virtual bool match(const Entity *e)const{
		return team == e->race;
	}
	virtual void draw(GLwindowState &ws, GLWentlistCriteria *p, int y){
		GLWrect cr = p->clientRect();
		glColor4f(.25, .25, .25, .5);
		glBegin(GL_QUADS);
		glVertex2i(cr.x0 + p->getFontHeight() + 4, cr.y0 + y + p->lineHeight() - p->getFontHeight());
		glVertex2i(cr.x0 + p->getFontHeight() + 4, cr.y0 + y + p->getFontHeight());
		glVertex2i(cr.x1 - p->getFontHeight() - 4, cr.y0 + y + p->getFontHeight());
		glVertex2i(cr.x1 - p->getFontHeight() - 4, cr.y0 + y + p->lineHeight() - p->getFontHeight());
		glEnd();

		glColor4f(1,1,1,1);
		glwpos2d(cr.x0 + p->getFontHeight() + 4, cr.y0 + y + p->lineHeight());
		glwprintf(cpplib::dstring() << "Team: " << team);
		ItemCriterion::draw(ws, p, y);
	}
	virtual int mouse(GLwindowState &ws, GLWentlistCriteria *p, int key, int state, int mx, int my){
		if(int ret = ItemCriterion::mouse(ws, p, key, state, mx, my))
			return ret;
		if(key == GLUT_LEFT_BUTTON && state == GLUT_UP){
			struct PopupMenuItemTeam : public PopupMenuItem{
				typedef PopupMenuItem st;
				TeamItemCriterion *p;
				int team;
				PopupMenuItemTeam(cpplib::dstring title, TeamItemCriterion *p, int team) : p(p), team(team){
					this->title = title;
				}
				virtual void execute(){
					p->team = team;
				}
				virtual PopupMenuItem *clone(void)const{return new PopupMenuItemTeam(*this);}
			};

			PopupMenu pm;
			for(int i = 0; i < 4; i++)
				pm.append(new PopupMenuItemTeam(cpplib::dstring() << "Team " << i, this, i));
			glwPopupMenu(ws, pm);
			return 1;
		}
		return 0;
	}
};


int GLWentlistCriteria::mouse(GLwindowState &ws, int key, int state, int mx, int my){
	int y = 0;
	GLWrect cr = clientRect();
	if(p->crtRoot){
		int ret = p->crtRoot->mouse(ws, this, y, key, state, mx, my);
		if(ret)
			return ret;
	}
/*	for(int i = 0; i < crt.size(); i++){
		GLWrect ir = cr;
		ir.y0 = y;
		ir.y1 = y += lineHeight();
		if(ir.include(mx, my))
			return crt[i]->mouse(ws, this, key, state, mx, my);
	}*/
	cr.y0 = y;
	cr.y1 = cr.y0 + lineHeight();
	if(cr.include(mx, my) && key == GLUT_LEFT_BUTTON && state == GLUT_UP){
		glwPopupMenu(ws, PopupMenu()
			.append(new PopupMenuItemFunctionT<GLWentlistCriteria>("Selected", this, &GLWentlistCriteria::addCriterion<SelectedItemCriterion>))
			.append(new PopupMenuItemFunctionT<GLWentlistCriteria>("Class Name", this, &GLWentlistCriteria::addCriterion<ClassItemCriterion>))
			.append(new PopupMenuItemFunctionT<GLWentlistCriteria>("Team", this, &GLWentlistCriteria::addCriterion<TeamItemCriterion>))
		);
		return 1;
	}
	return 0;
}

void CriterionNode::draw(GLwindowState &ws, GLWentlistCriteria *p, int &y){
	GLWrect cr = p->clientRect();
	if(!crt){
		left->draw(ws, p, y);

		glwpos2d(cr.x0, cr.y0 + y + p->getFontHeight());
		glwprintf(op == And ? "&" : op == Or ? "|" : op == Xor ? "^" : "?");
		y += p->lineHeight();

		right->draw(ws, p, y);
		return;
	}
	crt->draw(ws, p, y);

	glColor4f(1,1,1,1);
	glBegin(GL_LINES);
	glVertex2i(cr.x1 - p->getFontHeight() + 2, cr.y0 + y + 2);
	glVertex2i(cr.x1 - 2, cr.y0 + y + p->getFontHeight() - 2);
	glVertex2i(cr.x1 - p->getFontHeight() + 2, cr.y0 + y + p->getFontHeight() - 2);
	glVertex2i(cr.x1 - 2, cr.y0 + y + 2);
	glEnd();
	y += p->lineHeight();
}

int CriterionNode::mouse(GLwindowState &ws, GLWentlistCriteria *p, int &y, int key, int state, int mx, int my){
	GLWrect cr = p->clientRect().move(0, 0);
	if(!crt){
		int ret = left->mouse(ws, p, y, key, state, mx, my);
		if(ret)
			return ret;

		cr.y0 = y;
		cr.y1 = y += p->lineHeight();
		if(cr.include(mx, my)){
			if(key == GLUT_LEFT_BUTTON && state == GLUT_UP && mx < p->getFontHeight()){
				op = Op((op + 1) % NumOp);
				return 1;
			}
		}

		ret = right->mouse(ws, p, y, key, state, mx, my);
		if(ret)
			return ret;
		return 0;
	}

	cr.y0 = y;
	cr.y1 = y += p->lineHeight();
	if(cr.include(mx, my)){
		if(key == GLUT_LEFT_BUTTON && state == GLUT_UP && cr.x1 - p->getFontHeight() < mx + cr.x0){
			CriterionNode::deleteNode(&p->p->crtRoot, crt);
			return 1; // Be sure to return 1 here or access violation will occur
		}
		return crt->mouse(ws, p, key, state, mx, my);
	}
	return 0;
}
