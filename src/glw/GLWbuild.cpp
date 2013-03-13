/** \file
 * \brief Implementation of GLWbuild class.
 */
#define NOMINMAX
#include "Builder.h"
#include "antiglut.h"

void GLWbuild::progress_bar(double f, int width, int *piy){
	GLWrect cr = clientRect();
	int iy = *piy;
	glColor4ub(0,255,0,255);
	glBegin(GL_QUADS);
	glVertex2d(cr.x0, cr.y0 + iy * 12);
	glVertex2d(cr.x0 + width * f, cr.y0 + iy * 12);
	glVertex2d(cr.x0 + width * f, cr.y0 + (iy + 1) * 12);
	glVertex2d(cr.x0 + 0, cr.y0 + (iy + 1) * 12);
	glEnd();
	glColor4ub(127,127,127,255);
	glBegin(GL_LINE_LOOP);
	glVertex2d(cr.x0, cr.y0 + iy * 12);
	glVertex2d(cr.x0 + width, cr.y0 + iy * 12);
	glVertex2d(cr.x0 + width, cr.y0 + (iy + 1) * 12);
	glVertex2d(cr.x0 + 0, cr.y0 + (iy + 1) * 12);
	glEnd();
	glColor4ub(255,0,0,255);
	glwpos2d(cr.x0, cr.y0 + (1 + (*piy)++) * 12);
	glwprintf("%3.0lf%%", 100. * f);
}

int GLWbuild::draw_tab(int ix, int iy, const char *s, int selected){
	GLWrect cr = clientRect();
	int ix0 = ix;
	glBegin(GL_LINE_STRIP);
	glVertex2d(cr.x0 + ix, cr.y0 + iy + 12);
	glVertex2d(cr.x0 + (ix += 3), cr.y0 + iy + 1);
	ix += 8 * strlen(s);
	glVertex2d(cr.x0 + ix, cr.y0 + iy + 1);
	glVertex2d(cr.x0 + (ix += 3), cr.y0 + iy + 12);
	if(!selected){
		glVertex2d(cr.x0 + ix0, cr.y0 + iy + 12);
	}
	glEnd();
	glwpos2d(cr.x0 + ix0 + 3, cr.y0 + iy + 12);
	glwprintf("%s", s);
	return ix;
}

void GLWbuild::draw(GLwindowState &ws, double t){
	int ix;
	GLWrect cr = clientRect();
	int fonth = getFontHeight();
	if(!builder)
		return;
	glColor4ub(255,255,255,255);
	glwpos2d(cr.x0, cr.y0 + fonth);
	glwprintf("Resource Units: %.0lf", std::max(0., builder->getRU())); // The max is to avoid -0.
	glColor4ub(255,255,0,255);
	ix = draw_tab(ix = 2, fonth, "Build", tabindex == 0);
	ix = draw_tab(ix, fonth, "Queue", tabindex == 1);
	glBegin(GL_LINES);
	glVertex2d(cr.x0 + ix, cr.y0 + 2 * fonth);
	glVertex2d(cr.x1, cr.y0 + 2 * fonth);
	glEnd();
	const Builder::BuildData &top = builder->buildque[0];
	int iy = 2;
	if(tabindex == 0){
		int *builderc = NULL, builderi, j;
		if(builder->nbuildque){
			builderc = new int[Builder::buildRecipes.size()];
			for(j = 0; j < Builder::buildRecipes.size(); j++){
				builderc[j] = 0;

				// Accumulate all build queue entries
				for(int i = 0; i < builder->nbuildque; i++){
					if(builder->buildque[i].st == &Builder::buildRecipes[j])
						builderc[j] += builder->buildque[i].num;
				}

				// Remember the top (currently building) build queue entry for progress bar.
				if(top.st == &Builder::buildRecipes[j])
					builderi = j;
			}
		}
		glColor4ub(255,255,255,255);
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * fonth);
		glwprintf("Buildable Items:");

		/* Mouse cursor highlights */
		int mx = ws.mx - cr.x0, my = ws.my - cr.y0;
		if(!modal && 0 < mx && mx < cr.width() && (iy) * fonth < my && my < (iy + Builder::buildRecipes.size()) * fonth){
			glColor4ub(0,0,255,127);
			glRecti(cr.x0, cr.y0 + (my / fonth) * fonth, cr.x1, cr.y0 + (my / fonth) * fonth);
		}

		/* Progress bar of currently building item */
		if(builder->nbuildque && builder->build){
			glColor4ub(0,127,0,255);
			glRecti(cr.x0, cr.y0 + (builderi + iy) * fonth + 8, cr.x0 + (1. - builder->build / top.st->buildtime) * cr.width(), cr.y0 + (builderi + iy) * fonth + fonth);
		}

		// Draw the buildable item's name and cost after the progress bar.
		glColor4ub(191,191,255,255);
		for(j = 0; j < Builder::buildRecipes.size(); j++){
			const Builder::BuildRecipe *sta = &Builder::buildRecipes[j];
			glwpos2d(cr.x0, cr.y0 + (1 + iy++) * fonth);
			glwprintf("%10s  %d  %lg RU", (const char*)sta->name, builderc ? builderc[j] : 0, sta->cost);
		}
/*		glwpos2d(xpos, ypos + (2 + iy++) * 12);
		glwprintf("Container    %d  %lg RU %lg dm^3", builderc[0], scontainer_build.cost, 1e-3 * scontainer_s.getHitRadius * scontainer_s.getHitRadius * scontainer_s.getHitRadius * 8 * 1e9);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Worker       %d  %lg RU %lg dm^3", builderc[1], worker_build.cost, 1e-3 * worker_s.getHitRadius * worker_s.getHitRadius * worker_s.getHitRadius * 8 * 1e9);*/
/*		glwpos2d(xpos, ypos + (2 + iy++) * fonth);
		glwprintf("Interceptor  %d  %lg RU %lg dm^3", builderc ? builderc[0] : 0, sceptor_build.cost, 1e-3 * ((Sceptor*)NULL)->Sceptor::getHitRadius() * ((Sceptor*)NULL)->Sceptor::getHitRadius() * ((Sceptor*)NULL)->Sceptor::getHitRadius() * 8 * 1e9);*/
/*		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Lancer Class %d  %lg RU %lg dm^3", builderc[3], beamer_build.cost, 1e-3 * beamer_s.getHitRadius * beamer_s.getHitRadius * beamer_s.getHitRadius * 8 * 1e9);
		glwpos2d(wnd->x, wnd->y + (2 + iy++) * 12);
		glwprintf("Sabre Class  %d  %lg RU %lg dm^3", builderc[4], assault_build.cost, 1e-3 * assault_s.getHitRadius * assault_s.getHitRadius * assault_s.getHitRadius * 8 * 1e9);*/
		if(builder->nbuildque)
			delete[] builderc;
	}
	else{
		glColor4ub(255,255,255,255);
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * fonth);
		glwprintf("Build Queue:");
		if(builder->nbuildque == 0)
			return;
		glwpos2d(cr.x0, cr.y0 + (1 + iy++) * fonth);
		glwprintf(top.st->name);
		progress_bar((1. - builder->build / top.st->buildtime), 200, &iy);
		for(int i = 0; i < builder->nbuildque; i++){
			if(cr.height() < (2 + iy) * fonth)
				return;
			glColor4ub(255,255,255,255);
			glwpos2d(cr.x0, cr.y0 + (1 + iy) * fonth);
			int widthChars = glwprintf("Cancel");
			int widthPixels = widthChars * getFontWidth();

			glColor4ub(0,255,255,255);
			glBegin(GL_LINE_LOOP);
			glVertex2i(cr.x0, cr.y0 + (iy) * fonth);
			glVertex2i(cr.x0 + widthPixels, cr.y0 + (iy) * fonth);
			glVertex2i(cr.x0 + widthPixels, cr.y0 + (1 + iy) * fonth - 1);
			glVertex2i(cr.x0, cr.y0 + (1 + iy) * fonth - 1);
			glEnd();

			glColor4ub(255,255,255,255);
			glwpos2d(cr.x0 + widthPixels + 8, cr.y0 + (1 + iy++) * fonth);
			glwprintf("%d X %s %lg RUs", builder->buildque[i].num, builder->buildque[i].st->name.c_str(), builder->buildque[i].num * builder->buildque[i].st->cost);
		}
	}
}

static int select_tab(GLwindow *wnd, int ix, int iy, const char *s, int *selected, int mx, int my){
	int ix0 = ix;
	ix += 3;
	ix += 8 * strlen(s);
	ix += 3;
	*selected = ix0 < mx && mx < ix && iy < my + 12 && my + 12 < iy + 12;
	return ix;
}

int GLWbuild::mouse(GLwindowState &ws, int mbutton, int state, int mx, int my){
	GLWbuild *p = this;
	int sel0, sel1, ix;
	int fonth = (int)getFontHeight();
	if(st::mouse(ws, mbutton, state, mx, my))
		return 1;
	if((mbutton == GLUT_RIGHT_BUTTON || mbutton == GLUT_LEFT_BUTTON) && state == GLUT_UP || mbutton == GLUT_WHEEL_UP || mbutton == GLUT_WHEEL_DOWN){
		int num = 1, i;
#ifdef _WIN32
		if(HIBYTE(GetKeyState(VK_SHIFT)))
			num = 10;
#endif
		ix = select_tab(this, ix = 2, 2 * fonth, "Build", &sel0, mx, my);
		select_tab(this, ix, 2 * fonth, "Queue", &sel1, mx, my);
		if(sel0) p->tabindex = 0;
		if(sel1) p->tabindex = 1;
		if(p->tabindex == 0){
#if 1
			int ind = (my - 3 * fonth) / fonth;
			if(0 <= ind && ind < Builder::buildRecipes.size()){
				builder->addBuild(&Builder::buildRecipes[ind]);
				if(!game->isServer()){
					BuildCommand com(Builder::buildRecipes[ind].name);
					CMEntityCommand::s.send(builder->toEntity(), com);
				}
			}
#else
			if(0 <= ind && ind < 1) for(i = 0; i < num; i++) switch(ind){
				case 0:
/*					add_buildque(p->p, &scontainer_build);
					break;
				case 1:
					add_buildque(p->p, &worker_build);
					break;
				case 2:*/
					builder->addBuild(&sceptor_build);
					break;
/*				case 3:
					add_buildque(p->p, &beamer_build);
					break;
				case 4:
					if(mbutton == GLUT_RIGHT_BUTTON){
						int j;
						glwindow *wnd2;
						for(j = 0; j < p->p->narmscustom; j++) if(p->p->armscustom[j].builder == &assault_build)
							break;
						if(j == p->p->narmscustom){
							p->p->armscustom = realloc(p->p->armscustom, ++p->p->narmscustom * sizeof *p->p->armscustom);
							p->p->armscustom[j].builder = &assault_build;
							p->p->armscustom[j].c = numof(assault_hardpoints);
							p->p->armscustom[j].a = malloc(numof(assault_hardpoints) * sizeof *p->p->armscustom[j].a);
							memcpy(p->p->armscustom[j].a, assault_arms, numof(assault_hardpoints) * sizeof *p->p->armscustom[j].a);
						}
						wnd2 = ArmsShowWindow(AssaultNew, assault_build.cost, offsetof(assault_t, arms), p->p->armscustom[j].a, assault_hardpoints, numof(assault_hardpoints));
						wnd->modal = wnd2;
					}
					else
						add_buildque(p->p, &assault_build);
					break;*/
			}
#endif
		}
		else{
			int ind = (my - 5 * fonth) / fonth;
			if(builder->build != 0. && 0 <= ind && ind < builder->nbuildque && 0 <= mx && mx < getFontWidth() * (sizeof "Cancel" - 1)){
				// Send to the server before delete locally
				if(!game->isServer()){
					BuildCancelCommand com(builder->buildque[ind].orderId);
					CMEntityCommand::s.send(builder->toEntity(), com);
				}
				builder->build = 0.;
				builder->cancelBuild(ind);
			}
/*			else if(2 <= ind && ind < pf->nbuildque + 2){
				int i;
				int ptr;
				ind -= 2;
				ptr = (pf->buildque0 + ind) % numof(pf->buildque);
				if(1 < pf->buildquenum[ptr])
					pf->buildquenum[ptr]--;
				else{
					for(i = ind; i < pf->nbuildque - 1; i++){
						int ptr = (pf->buildque0 + i) % numof(pf->buildque);
						int ptr1 = (pf->buildque0 + i + 1) % numof(pf->buildque);
						pf->buildque[ptr] = pf->buildque[ptr1];
						pf->buildquenum[ptr] = pf->buildquenum[ptr1];
					}
					pf->nbuildque--;
				}
			}*/
		}
		return 1;
	}
	return 0;
}

void GLWbuild::postframe(){
	if(builder && !builder->w)
		builder = NULL;
}

