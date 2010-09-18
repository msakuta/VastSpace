#ifndef GLWENTLIST_H
#define GLWENTLIST_H
#include "glwindow.h"
#include "antiglut.h"

/// \brief Window that shows entity listing.
class GLWentlist : public GLwindowSizeable{
	static void draw_int(const CoordSys *cs, int &n, const char *names[], int counts[]){
		if(!cs)
			return;
		for(const CoordSys *cs1 = cs->children; cs1; cs1 = cs1->next)
			draw_int(cs1, n, names, counts);
		if(!cs->w)
			return;
		const Entity *pt;
		for(pt = cs->w->el; pt; pt = pt->next){
			int i;
			for(i = 0; i < n; i++) if(!strcmp(names[i], pt->dispname()))
				break;
			if(i == n/* || current_vft == vfts[i]*/){
				names[n] = pt->dispname();
				counts[n] = 1;
//				tanks[n] = pt;
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
	GLWentlist(Player &player) : st("Entity List"), pl(player), listmode(Select), groupByClass(true){}
	virtual void draw(GLwindowState &ws, double){
		const WarField *const w = pl.cs->w;
		const char *names[OV_COUNT];
//		const Entity *tanks[OV_COUNT] = {NULL};
		int counts[OV_COUNT];
		int i, n = 0;

		if(listmode == Select){
			const Entity *pt;
			for(pt = pl.selected; pt; pt = pt->selectnext){

				/* show only member of current team */
/*				if(pl.chase && pl.race != pt->race)
					continue;*/

				for(i = 0; i < n; i++) if(!strcmp(names[i], pt->dispname()))
					break;
				if(i == n/* || current_vft == vfts[i]*/){
					names[n] = pt->dispname();
					counts[n] = 1;
//					tanks[n] = pt;
					if(++n == OV_COUNT)
						break;
				}
				else
					counts[i]++;
			}
		}
		else if(listmode == All && w){
			const Entity *pt;
			for(pt = w->el; pt; pt = pt->next){

				for(i = 0; i < n; i++) if(!strcmp(names[i], pt->dispname()))
					break;
				if(i == n/* || current_vft == vfts[i]*/){
					names[n] = pt->dispname();
					counts[n] = 1;
//					tanks[n] = pt;
					if(++n == OV_COUNT)
						break;
				}
				else
					counts[i]++;
			}
		}
		else if(listmode == Universe){
			draw_int(pl.cs->findcspath("/"), n, names, counts);
		}

		GLWrect r = clientRect();
		int wid = (r.x1 - r.x0) / getFontWidth() -  6;

		static const GLfloat colors[2][4] = {{1,1,1,1},{0,1,1,1}};
		glColor4fv(colors[listmode == Select]);
		glwpos2d(r.x0, r.y0 + getFontHeight());
		glwprintf("Select");
		glColor4fv(colors[listmode == All]);
		glwpos2d(r.x0 + r.width() / 4, r.y0 + getFontHeight());
		glwprintf("All");
		glColor4fv(colors[listmode == Universe]);
		glwpos2d(r.x0 + r.width() * 2 / 4, r.y0 + getFontHeight());
		glwprintf("Universe");
		glColor4fv(colors[groupByClass]);
		glwpos2d(r.x0 + r.width() * 3 / 4, r.y0 + getFontHeight());
		glwprintf("Grouping");

		glColor4f(1,1,1,1);

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

	virtual int mouse(GLwindowState &ws, int button, int state, int x, int y){
		if(button == GLUT_LEFT_BUTTON && state == GLUT_UP){
			GLWrect r = clientRect();
			r.y0 = 0;
			r.y1 = getFontHeight();
			if(r.include(x + r.x0, y + r.y0)){
				int index = x * 4 / r.width();
				if(index < 3)
					listmode = MIN(Universe, MAX(Select, ListMode(index)));
				else
					groupByClass = !groupByClass;
				return 1;
			}
		}
		return st::mouse(ws, button, state, x, y);
	}
};

#endif
