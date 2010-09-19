#ifndef GLWENTLIST_H
#define GLWENTLIST_H
#include "glwindow.h"
#include "GLWtip.h"
#include "antiglut.h"

/// \brief Window that shows entity listing.
class GLWentlist : public GLwindowSizeable{
	static void draw_int(const CoordSys *cs, int &n, const char *names[], int counts[], std::vector<Entity*> ents[]){
		if(!cs)
			return;
		for(const CoordSys *cs1 = cs->children; cs1; cs1 = cs1->next)
			draw_int(cs1, n, names, counts, ents);
		if(!cs->w)
			return;
		Entity *pt;
		for(pt = cs->w->el; pt; pt = pt->next){
			int i;
			for(i = 0; i < n; i++) if(!strcmp(names[i], pt->dispname())){
				ents[i].push_back(pt);
				break;
			}
			if(i == n/* || current_vft == vfts[i]*/){
				names[n] = pt->dispname();
				counts[n] = 1;
				ents[n].push_back(pt);
				if(++n == OV_COUNT)
					break;
			}
			else
				counts[i]++;
		}
	}
	static const int OV_COUNT = 32; ///< Overview count
public:
	typedef GLwindowSizeable st;

	Player &pl;
	enum ListMode{ Select, All, Universe } listmode;
	bool groupByClass;
	bool icons;
	bool switches;
	std::vector<Entity*> ents[OV_COUNT];
	int n;
	GLWentlist(Player &player) : st("Entity List"), pl(player), listmode(Select), groupByClass(true), icons(true), switches(false){}
	virtual void draw(GLwindowState &ws, double){
		const WarField *const w = pl.cs->w;
		const char *names[OV_COUNT];
		int counts[OV_COUNT];
		int i;

		n = 0;
		for(i = 0; i < OV_COUNT; i++)
			ents[i].clear();

		if(listmode == Select){
			Entity *pt;
			for(pt = pl.selected; pt; pt = pt->selectnext){

				/* show only member of current team */
/*				if(pl.chase && pl.race != pt->race)
					continue;*/

				for(i = 0; i < n; i++) if(!strcmp(names[i], pt->dispname())){
					ents[i].push_back(pt);
					break;
				}
				if(i == n/* || current_vft == vfts[i]*/){
					names[n] = pt->dispname();
					counts[n] = 1;
					ents[n].push_back(pt);
					if(++n == OV_COUNT)
						break;
				}
				else
					counts[i]++;
			}
		}
		else if(listmode == All && w){
			Entity *pt;
			for(pt = w->el; pt; pt = pt->next){

				for(i = 0; i < n; i++) if(!strcmp(names[i], pt->dispname())){
					ents[i].push_back(pt);
					break;
				}
				if(i == n/* || current_vft == vfts[i]*/){
					names[n] = pt->dispname();
					counts[n] = 1;
					ents[n].push_back(pt);
					if(++n == OV_COUNT)
						break;
				}
				else
					counts[i]++;
			}
		}
		else if(listmode == Universe){
			draw_int(pl.cs->findcspath("/"), n, names, counts, ents);
		}

		GLWrect r = clientRect();
		int wid = (r.x1 - r.x0) / getFontWidth() -  6;

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

		if(icons){
			int x = 0, y = switches * getFontHeight();
			for(i = 0; i < n; i++){
				std::vector<Entity*>::iterator it = ents[i].begin();
				for(int j = 0; j < (groupByClass ? 1 : counts[i]); j++, it++){
					Entity *pe;
					if(it != ents[i].end() && (pe = *it)){
						glPushMatrix();
						glTranslatef(r.x0 + x + 16, r.y0 + y + 16, 0);
						glScalef(12, 12, 1);
						Vec3f cols(pe->race % 2, 1, (pe->race + 1) % 2);
						if(listmode != Select){
							for(Entity *pe2 = pl.selected; pe2; pe2 = pe2->selectnext) if(pe2 == pe)
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
					if(groupByClass){
						glColor4f(1,1,1,1);
						glwpos2d(r.x0 + x + 8, r.y0 + y + 32);
						glwprintf("%3d", counts[i]);
					}
					else if(pe){
						double x2 = 2. * pe->health / pe->maxhealth() - 1.;
						const double h = .1;
						glPushMatrix();
						glTranslatef(r.x0 + x + 16, r.y0 + y + 16, 0);
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
					if(r.width() < x + 32){
						y += 32;
						x = 0;
					}
				}
			}
		}
		else{
			glColor4f(1,1,1,1);
			if(groupByClass){
				for(i = 0; i < n; i++){
					glwpos2d(r.x0, r.y0 + (switches + 1 + i) * getFontHeight());
					glwprintf("%*.*s x %-3d", wid, wid, names[i], counts[i]);
				}
			}
			else{
				int y = 0;
				for(i = 0; i < n; i++){
					for(int j = 0; j < counts[i]; j++){
						glwpos2d(r.x0, r.y0 + (switches + 1 + y) * getFontHeight());
						glwprintf("%*.*s", wid, wid, names[i]);
						y++;
					}
				}
			}
		}
	}

	virtual int mouse(GLwindowState &ws, int button, int state, int mx, int my){
		if(button == GLUT_LEFT_BUTTON && state == GLUT_UP && switches){
			GLWrect r = clientRect();
			r.y0 = 0;
			r.y1 = getFontHeight();
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
				.appendSeparator()
				.append(new PopupMenuItemFunction(APPENDER(switches, "Show quick switches"), this, &GLWentlist::menu_switches)));
		}
		if(button == GLUT_LEFT_BUTTON && (state == GLUT_KEEP_UP || state == GLUT_UP)){
			int x = 0, y = switches * getFontHeight();
			bool set = false;
			GLWrect r = clientRect();
			if(icons) for(int i = 0; i < n; i++){
				std::vector<Entity*>::iterator it = ents[i].begin();
				for(int j = 0; j < (groupByClass ? 1 : ents[i].size()); j++, it++){
					if(x < mx && mx < x + 32 && y < my && my < y + 32){
						Entity *pe;
						if(it != ents[i].end() && (pe = *it)){
							if(state == GLUT_UP){
								if(groupByClass){
									Entity **prev = &pl.selected;
									for(it = ents[i].begin(); it != ents[i].end(); it++){
										*prev = *it;
										prev = &(*it)->selectnext;
										(*it)->selectnext = NULL;
									}
								}
								else{
									pl.selected = pe;
									pe->selectnext = NULL;
								}
							}
							else{
								int xs, ys;
								const long margin = 4;
								cpplib::dstring str = cpplib::dstring(pe->dispname()) << "\nrace = " << pe->race;
								glwGetSizeStringML(str, GLwindow::getFontHeight(), &xs, &ys);
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
					x += 32;
					if(r.width() < x + 32){
						y += 32;
						x = 0;
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
	void menu_select(){listmode = Select;}
	void menu_all(){listmode = All;}
	void menu_universe(){listmode = Universe;}
	void menu_grouping(){groupByClass = !groupByClass;}
	void menu_icons(){icons = !icons;}
	void menu_switches(){switches = !switches;}
	virtual void mouseLeave(GLwindowState &ws){
		if(glwtip->parent == this){
			glwtip->tips = NULL;
			glwtip->parent = NULL;
			glwtip->setExtent(GLWrect(-10,-10,-10,-10));
		}
	}
};

#endif
