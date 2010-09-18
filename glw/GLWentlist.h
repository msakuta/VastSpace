#ifndef GLWENTLIST_H
#define GLWENTLIST_H
#include "glwindow.h"
#include "antiglut.h"

/// \brief Window that shows entity listing.
class GLWentlist : public GLwindowSizeable{
	static void draw_int(const CoordSys *cs, int &n, const char *names[], int counts[], Entity *ents[]){
		if(!cs)
			return;
		for(const CoordSys *cs1 = cs->children; cs1; cs1 = cs1->next)
			draw_int(cs1, n, names, counts, ents);
		if(!cs->w)
			return;
		Entity *pt;
		for(pt = cs->w->el; pt; pt = pt->next){
			int i;
			for(i = 0; i < n; i++) if(!strcmp(names[i], pt->dispname()))
				break;
			if(i == n/* || current_vft == vfts[i]*/){
				names[n] = pt->dispname();
				counts[n] = 1;
				ents[n] = pt;
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
	GLWentlist(Player &player) : st("Entity List"), pl(player), listmode(Select), groupByClass(true), icons(true){}
	virtual void draw(GLwindowState &ws, double){
		const WarField *const w = pl.cs->w;
		const char *names[OV_COUNT];
		Entity *ents[OV_COUNT] = {NULL};
		int counts[OV_COUNT];
		int i, n = 0;

		if(listmode == Select){
			Entity *pt;
			for(pt = pl.selected; pt; pt = pt->selectnext){

				/* show only member of current team */
/*				if(pl.chase && pl.race != pt->race)
					continue;*/

				for(i = 0; i < n; i++) if(!strcmp(names[i], pt->dispname()))
					break;
				if(i == n/* || current_vft == vfts[i]*/){
					names[n] = pt->dispname();
					counts[n] = 1;
					ents[n] = pt;
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

				for(i = 0; i < n; i++) if(!strcmp(names[i], pt->dispname()))
					break;
				if(i == n/* || current_vft == vfts[i]*/){
					names[n] = pt->dispname();
					counts[n] = 1;
					ents[n] = pt;
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

		glColor4f(1,1,1,1);

		if(icons){
			int x = 0, y = 2 * getFontHeight();
			for(i = 0; i < n; i++) for(int j = 0; j < (groupByClass ? 1 : counts[i]); j++){
				glPushMatrix();
				glTranslatef(r.x0 + x + 16, r.y0 + y + 16, 0);
				glScalef(12, 12, 1);
				glColor4f(1,1,1,1);
				ents[i]->drawOverlay(NULL);
				glPopMatrix();
				glColor4f(0,0,1,.5);
				glBegin(GL_LINE_LOOP);
				glVertex2i(r.x0 + x + 2, r.y0 + y + 2);
				glVertex2i(r.x0 + x + 2, r.y0 + y + 30);
				glVertex2i(r.x0 + x + 30, r.y0 + y + 30);
				glVertex2i(r.x0 + x + 30, r.y0 + y + 2);
				glEnd();
				if(groupByClass){
					glColor4f(1,1,0,1);
					glwpos2d(r.x0 + x + 16, r.y0 + y + 32);
					glwprintf("%-3d", counts[i]);
				}
				x += 32;
				if(r.width() < x + 32){
					y += 32;
					x = 0;
				}
			}
		}
		else{
			if(groupByClass){
				for(i = 0; i < n; i++){
					glwpos2d(r.x0, r.y0 + (2 + i) * getFontHeight());
					glwprintf("%*.*s x %-3d", wid, wid, names[i], counts[i]);
				}
			}
			else{
				int y = 0;
				for(i = 0; i < n; i++){
					for(int j = 0; j < counts[i]; j++){
						glwpos2d(r.x0, r.y0 + (2 + y) * getFontHeight());
						glwprintf("%*.*s", wid, wid, names[i]);
						y++;
					}
				}
			}
		}
	}

	virtual int mouse(GLwindowState &ws, int button, int state, int x, int y){
		if(button == GLUT_LEFT_BUTTON && state == GLUT_UP){
			GLWrect r = clientRect();
			r.y0 = 0;
			r.y1 = getFontHeight();
			if(r.include(x + r.x0, y + r.y0)){
				int index = x * 5 / r.width();
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
			/// Menu item that executes a Squirrel closure.
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
				.append(new PopupMenuItemFunction(APPENDER(icons, "Show icons"), this, &GLWentlist::menu_icons)));
		}
		return st::mouse(ws, button, state, x, y);
	}
	void menu_select(){listmode = Select;}
	void menu_all(){listmode = All;}
	void menu_universe(){listmode = Universe;}
	void menu_grouping(){groupByClass = !groupByClass;}
	void menu_icons(){icons = !icons;}
};

#endif
