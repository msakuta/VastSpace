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
	scrollpos(0){}

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
		if(teamOnly && pl.race != pt->race)
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

void GLWentlist::draw(GLwindowState &ws, double){
	const WarField *const w = pl.cs->w;
//		const char *names[OV_COUNT];
//		int counts[OV_COUNT];
	int i;

	n = 0;
	for(i = 0; i < OV_COUNT; i++)
		ents[i].clear();

	if(listmode == Select){
		Entity *pt;
		for(pt = pl.selected; pt; pt = pt->selectnext){

			/* show only member of current team */
			if(teamOnly && pl.race != pt->race)
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
			if(teamOnly && pl.race != pt->race)
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

	// Do not scroll too much
	if(0 < itemCount && itemCount <= scrollpos)
		scrollpos = itemCount - 1;

	if(icons){
		int x = 0, y = switches * int(getFontHeight());
		for(is.begin(), i = 0; i < itemCount; i++, is.next()){
			Entity *pe = is.get();
//			std::vector<Entity*>::iterator it = pents[i]->begin();
/*			for(int j = 0; j < is.count(); j++, it++)*/{
//				Entity *pe;
/*				if(it != pents[i]->end() && (pe = *it))*/{
					glPushMatrix();
					glTranslated(r.x0 + x + 16, r.y0 + y + 16, 0);
					glScalef(12, 12, 1);
					Vec3f cols(float(pe->race % 2), 1, float((pe->race + 1) % 2));
					if(listmode != Select){
						Entity *pe2;
						for(pe2 = pl.selected; pe2; pe2 = pe2->selectnext) if(pe2 == pe)
							break;
						if(!pe2)
							cols *= .5;
					}
					glColor4f(cols[0], cols[1], cols[2], 1);
					pe->drawOverlay(NULL);
					glPopMatrix();
				}
				glBegin(GL_LINE_LOOP);
				glVertex2i(r.x0 + x + 2, r.y0 + y + 2);
				glVertex2i(r.x0 + x + 2, r.y0 + y + 30);
				glVertex2i(r.x0 + x + 30, r.y0 + y + 30);
				glVertex2i(r.x0 + x + 30, r.y0 + y + 2);
				glEnd();
				if(is.stack()){
					glColor4f(1,1,1,1);
					glwpos2d(r.x0 + x + 8, r.y0 + y + 32);
					glwprintf("%3d", pents[i]->size());
				}
				else if(pe){
					double x2 = 2. * pe->health / pe->maxhealth() - 1.;
					const double h = .1;
					glPushMatrix();
					glTranslated(r.x0 + x + 16, r.y0 + y + 16, 0);
					glScalef(14, 14, 1);
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
				x += 32;
				if(r.width() - 10 < x + 32){
					y += 32;
					x = 0;
				}
			}
		}
	}
	else{
		glColor4f(1,1,1,1);
		int fontheight = int(getFontHeight());
/*		if(groupByClass)*/{
			for(is.begin(), i = 0; i < itemCount; i++, is.next()) if(scrollpos <= i){
				Entity *pe = is.get();
				int y = i - scrollpos;
				int top = (switches + y) * fontheight + r.y0;
				if(r.x0 <= ws.mx && ws.mx < r.x1 - 10 && top < ws.my && ws.my < top + fontheight){
					glColor4f(0,0,1.,.5);
					glBegin(GL_QUADS);
					glVertex2i(r.x0, top);
					glVertex2i(r.x0, top + fontheight);
					glVertex2i(r.x1 - 10, top + fontheight);
					glVertex2i(r.x1 - 10, top);
					glEnd();
					glColor4f(1,1,1,1);
				}
				glwpos2d(r.x0, r.y0 + (switches + 1 + y) * getFontHeight());
				if(is.stack())
					glwprintf("%*.*s x %-3d", wid, wid, pe->dispname(), is.countInGroup());
				else
					glwprintf("%*.*s", wid, wid, pe->dispname());
			}
		}
/*		else{
			int y = 0;
			for(i = 0; i < n; i++){
				for(unsigned j = 0; j < pents[i]->size(); j++){
					glwpos2d(r.x0, r.y0 + (switches + 1 + y) * getFontHeight());
					glwprintf("%*.*s", wid, wid, (*pents[i])[j]->dispname());
					y++;
				}
			}
		}*/
	}
	glwVScrollBarDraw(this, r.x1 - 10, r.y0, 10, r.height(), itemCount, scrollpos);
}

int GLWentlist::mouse(GLwindowState &ws, int button, int state, int mx, int my){
	GLWrect r = clientRect();
	ClassItemSelector cis(this);
	ExpandItemSelector eis(this);
	ItemSelector &is = groupByClass ? (ItemSelector&)cis : (ItemSelector&)eis;
	int itemCount = is.count();
	if(r.width() - 10 < mx && button == GLUT_LEFT_BUTTON && (state == GLUT_DOWN || state == GLUT_KEEP_DOWN || state == GLUT_UP)){
		int iy = glwVScrollBarMouse(this, mx, my, r.width() - 10, 0, 10, r.height(), itemCount, scrollpos);
		if(0 <= iy)
			scrollpos = iy;
		return 1;
	}
	if(button == GLUT_LEFT_BUTTON && state == GLUT_UP && switches){
		r.y0 = 0;
		r.y1 = long(getFontHeight());
		if(r.include(mx + r.x0, my + r.y0)){
			int index = mx * 5 / r.width();
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
		/// Menu item that executes a member function of GLWentlist.
		struct PopupMenuItemFunction : public PopupMenuItem{
			typedef PopupMenuItem st;
			GLWentlist *p;
			void (GLWentlist::*func)();
			PopupMenuItemFunction(cpplib::dstring title, GLWentlist *p, void (GLWentlist::*func)()) : p(p), func(func){
				this->title = title;
			}
			virtual void execute(){
				(p->*func)();
			}
			virtual PopupMenuItem *clone(void)const{return new PopupMenuItemFunction(*this);}
		};
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
			.append(new PopupMenuItemFunction(APPENDER(switches, "Show quick switches"), this, &GLWentlist::menu_switches)));
	}
	if(button == GLUT_LEFT_BUTTON && (state == GLUT_KEEP_UP || state == GLUT_UP)){
		int x = 0, y = (switches - scrollpos) * int(getFontHeight());
		bool set = false;
		if(mx < r.width() - 10) for(int i = (is.begin(), 0); i < itemCount; i++, is.next()){
/*			std::vector<Entity*> &ent = *pents[i];
			std::vector<Entity*>::iterator it = ent.begin();
			for(unsigned j = 0; j < (groupByClass ? 1 : ent.size()); j++, it++)*/{
				if(icons ? x < mx && mx < x + 32 && y < my && my < y + 32 : y < my && my < y + getFontHeight()){
					Entity *pe = is.get();
/*					if(it != ent.end() && (pe = *it))*/{
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
/*							if(groupByClass){
								Entity **prev = &pl.selected;
								for(it = ent.begin(); it != ent.end(); it++){
								}
							}
							else{
								pl.selected = pe;
								pe->selectnext = NULL;
							}*/
						}
						else{
							int xs, ys;
							const long margin = 4;
							cpplib::dstring str = cpplib::dstring(pe->dispname()) << "\nrace = " << pe->race << "\nhealth = " << pe->health;
							glwGetSizeStringML(str, GLwindow::glwfontheight, &xs, &ys);
							GLWrect localrect = GLWrect(r.x0 + x, r.y0 + y - ys - 3 * margin, r.x0 + x + xs + 3 * margin, r.y0 + y);

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
				if(icons){
					x += 32;
					if(r.width() - 10 < x + 32){
						y += 32;
						x = 0;
					}
				}
				else{
					y += int(getFontHeight());
				}
			}
		}
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
