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
	GLWentlist(Player &player) : st("Entity List"), pl(player), listmode(Select){}
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
		int wid = (r.r - r.l) / getFontWidth() -  6;

		static const GLfloat colors[2][4] = {{1,1,1,1},{0,1,1,1}};
		glColor4fv(colors[listmode == Select]);
		glwpos2d(r.l, r.t + getFontHeight());
		glwprintf("Select");
		glColor4fv(colors[listmode == All]);
		glwpos2d(r.l + 100, r.t + getFontHeight());
		glwprintf("All");
		glColor4fv(colors[listmode == Universe]);
		glwpos2d(r.l + 200, r.t + getFontHeight());
		glwprintf("Universe");

		glColor4f(1,1,1,1);

		for(i = 0; i < n; i++){
			glwpos2d(r.l, r.t + (2 + i) * getFontHeight());
			glwprintf("%*.*s x %-3d", wid, wid, names[i], counts[i]);
		}
	}

	virtual int mouse(GLwindowState &ws, int button, int state, int x, int y){
		if(button == GLUT_LEFT_BUTTON && state == GLUT_UP){
			GLWrect r = clientRect();
			r.t = 0;
			r.b = getFontHeight();
			if(r.include(x + r.l, y + r.t)){
				listmode = MIN(Universe, MAX(Select, ListMode(x / 100)));
				return 1;
			}
		}
		return st::mouse(ws, button, state, x, y);
	}
};

#endif
