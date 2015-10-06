#include "glw/glwindow.h"
#include "GLWmenu.h"
#include "glw/PopupMenu.h"
#include "GLWtip.h"
#include "sqadapt.h"
#include "Autonomous.h"
#include "Inventory.h"
#include "antiglut.h"
#include "draw/material.h"

#include <algorithm>


class GLWinventory;

/// \brief Window that shows entity listing.
///
/// It can select view modes from two flavors, list and icons.
class GLWinventory : public GLwindowSizeable{
protected:
	static int ent_pred(const InventoryItem *a, const InventoryItem *b){
		return a->typeString() < b->typeString();
	}
	void menu_icons(){icons = !icons;}

	WeakPtr<Autonomous> container;
	std::vector<const InventoryItem*> items;
public:
	typedef GLwindowSizeable st;

	bool icons;
	bool summary;
	int scrollpos; ///< Current scroll position in the vertical scroll bar.
	GLWinventory(Game *game, Autonomous *container);
	virtual void draw(GLwindowState &ws, double);
	virtual int mouse(GLwindowState &ws, int button, int state, int mx, int my);
	virtual void mouseLeave(GLwindowState &ws);

	static SQInteger sqf_constructor(HSQUIRRELVM v);
	static bool sq_define(HSQUIRRELVM v);
};

static sqa::Initializer definer("GLWinventory", GLWinventory::sq_define);

GLWinventory::GLWinventory(Game *game, Autonomous *container) :
	st(game, "Inventory"),
	container(container),
	icons(false),
	summary(true),
	scrollpos(0)
{
	setCollapsable(true);
}


/// Definition of how items are located.
class InventoryItemLocator{
public:
	virtual int allHeight() = 0; ///< Returns height of all items, used by the vertical scroll bar.
	virtual bool begin() = 0; ///< Begins enumeration
	virtual GLWrect getRect() = 0; ///< Returns rectangle of current item
	virtual bool next() = 0; ///< Enumerate the next item
	GLWrect nextRect(){
		GLWrect ret = getRect();
		next();
		return ret;
	}
};

/// Locates items in a list. An item each line.
class InventoryListItemLocator : public InventoryItemLocator{
public:
	GLWinventory *p;
	int count;
	int i;
	InventoryListItemLocator(GLWinventory *p, int count) : p(p), count(count), i(0){}
	int allHeight()override{return count * int(GLwindow::getFontHeight());}
	bool begin()override{i = 0; return true;}
	GLWrect getRect()override{
		GLWrect ret = p->clientRect();
		ret.y0 += i * int(GLwindow::getFontHeight());
		ret.y1 = ret.y0 + int(GLwindow::getFontHeight());
		return ret;
	}
	bool next()override{return i++ < count;}
};

/// Icon matrix.
class InventoryIconItemLocator : public InventoryItemLocator{
public:
	GLWinventory *p;
	int count;
	int i;
	int iconSize;
	InventoryIconItemLocator(GLWinventory *p, int count, int iconSize = 64) : p(p), count(count), i(0), iconSize(iconSize){}
	int allHeight()override{
		return (count + (p->clientRect().width() - 10) / iconSize - 1)
			/ max(1, (p->clientRect().width() - 10) / iconSize) * iconSize;
	}
	bool begin()override{i = 0; return true;}
	GLWrect getRect()override{
		GLWrect ret = p->clientRect();
		int cols = max(1, (ret.width() - 10) / iconSize);
		ret.x0 += i % cols * iconSize;
		ret.y0 += i / cols * iconSize;
		ret.x1 = ret.x0 + iconSize;
		ret.y1 = ret.y0 + iconSize;
		return ret;
	}
	bool next()override{return i++ < count;}
};

void GLWinventory::draw(GLwindowState &ws, double){
	// Ensure the player is present. There can be no player in the game.
	if(!container)
		return;

	items.clear();
	double totalVolume = 0.;
	double totalMass = 0.;

	for(auto it : container->getInventory()){
		items.push_back(it);
		totalVolume += it->getVolume();
		totalMass += it->getMass();
	}

	GLWrect r = clientRect();
	int wid = int((r.width() - 10) / getFontWidth()) -  6;

	// Offset pixels at the top of the window for showing summary
	double offset = 0;
	if(summary){
		static const GLfloat color[4] = {1,1,1,1};
		glColor4fv(color);
		glwpos2d(r.x0, r.y0 + getFontHeight());
		glwprintf("Volume: %g m^3", totalVolume);
		glwpos2d(r.x0, r.y0 + 2 * getFontHeight());
		glwprintf("Mass: %g tons", totalMass);

		offset = 2 * getFontHeight();

		glBegin(GL_LINES);
		glVertex2d(r.x0, r.y0 + offset);
		glVertex2d(r.x1, r.y0 + offset);
		glEnd();
	}

#if PROFILE_SORT
	timemeas_t tm;
	TimeMeasStart(&tm);
#endif
	std::sort(items.begin(), items.end(), ent_pred);
#if PROFILE_SORT
	printf("sort %g\n", TimeMeasLap(&tm));
#endif

	int itemCount = int(items.size());

	InventoryListItemLocator lil(this, itemCount);
	InventoryIconItemLocator eil(this, itemCount);
	InventoryItemLocator &il = icons ? (InventoryItemLocator&)eil : (InventoryItemLocator&)lil;

	// Do not scroll too much
	if(il.allHeight() < r.height())
		scrollpos = 0;
	else if(il.allHeight() - r.height() <= scrollpos)
		scrollpos = il.allHeight() - r.height() - 1;

	int fontheight = int(getFontHeight());
	glPushAttrib(GL_SCISSOR_BIT);
	glScissor(r.x0, ws.h - r.y1, r.width(), r.height() - offset);
	glEnable(GL_SCISSOR_TEST);

	int x = 0, y = offset;
	glPushMatrix();
	glTranslated(0, -scrollpos + offset, 0);
	for(auto it : items){
		GLWrect iconRect = il.nextRect();
			
		if(iconRect.include(ws.mx, ws.my + scrollpos - offset)){
			glColor4f(0,0,1.,.5);
			glBegin(GL_QUADS);
			glVertex2i(iconRect.x0, iconRect.y0);
			glVertex2i(iconRect.x1, iconRect.y0);
			glVertex2i(iconRect.x1, iconRect.y1);
			glVertex2i(iconRect.x0, iconRect.y1);
			glEnd();
		}

		if(icons){
			GLWrect borderRect = iconRect.expand(-2);
			glBegin(GL_LINE_LOOP);
			glVertex2i(borderRect.x0, borderRect.y0);
			glVertex2i(borderRect.x1, borderRect.y0);
			glVertex2i(borderRect.x1, borderRect.y1);
			glVertex2i(borderRect.x0, borderRect.y1);
			glEnd();
			const InventoryItemClass *ic = it->getType();
			if(ic->textureFile != "" && ic->texture == 0){
				suftexparam_t stp;
				stp.flags = STP_MAGFIL | STP_ALPHA | STP_ALPHA_TEST;
				stp.magfil = GL_LINEAR;
				ic->texture = CallCacheBitmap(ic->typeString, ic->textureFile, &stp, NULL);
			}
			if(ic->texture){
				glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
				glCallList(ic->texture);
				// Texture Y-axis coordinate is upside down
				glBegin(GL_QUADS);
				glTexCoord2d(0, 1); glVertex2i(borderRect.x0, borderRect.y0);
				glTexCoord2d(1, 1); glVertex2i(borderRect.x1, borderRect.y0);
				glTexCoord2d(1, 0); glVertex2i(borderRect.x1, borderRect.y1);
				glTexCoord2d(0, 0); glVertex2i(borderRect.x0, borderRect.y1);
				glEnd();
				glPopAttrib();
			}
			if(it->isCountable()){
				glColor4f(1,1,1,1);
				glwpos2d(iconRect.x1 - 24, iconRect.y1);
				glwprintf("%-3g", it->getAmount());
			}
		}
		else{
//				if(ps.ret)
					glColor4f(1,1,1,1);
//				else
//					glColor4f(.75,.75,.75,1);
//			}
			glwpos2d(iconRect.x0, iconRect.y1);
			if(it->isCountable())
				glwprintf("%*.*s x %-3g", wid, wid, it->typeString().c_str(), it->getAmount());
			else
				glwprintf("%*.*s", wid, wid, it->typeString().c_str());
		}
	}
	glPopMatrix();

	if(r.height() < il.allHeight())
		glwVScrollBarDraw(this, r.x1 - 10, r.y0, 10, r.height(), il.allHeight() - r.height(), scrollpos);
	glPopAttrib();
}

/// @brief Wraps words in a string by inserting newlines (\n)
/// @param str The string to be processed
/// @param lineWidth Desired linew width in bytes (multibyte characters are assumed 2 bytes wide).
/// @returns String with newlines inserted
static gltestp::dstring wordWrap(const gltestp::dstring &str, int lineWidth){
	if(str.len() < lineWidth)
		return str;
	int lastspace = -1;
	int lastoutput = 0;
	int curLineWidth = 0;
	gltestp::dstring ret;
	for(int i = 0; i < str.len(); i++){
		// We have encoded strings in UTF-8, and isspace() may be screwed up with multibyte
		// string because it can have one of many character encodings.
		// We opt out multi-byte character from the check (always assume non-alnum) to avoid this.
		// Char type can be either signed or unsigned, so we cast to unsigned.
		unsigned char c = (unsigned)str[i];
		if(c < 128 && isspace(c))
			lastspace = i;

		// Read out multi-byte character without breaking
		if(128 <= c){
			// Count 1 bits from the most significant bits, which is the number of bytes
			// in this UTF-8 multibyte character sequence.  See:
			// https://ja.wikipedia.org/wiki/UTF-8
			for(int bit = 1; (c >> (7 - bit)) & 1; bit++)
				++i;
			// Whether multibyte character's width larger than singlebyte
			// depends on character systems and we cannot be certain without actually
			// measuring output string.  Here we assume it's 2 characters wide.
			curLineWidth++; 
		}

		curLineWidth++;
		if(lineWidth <= curLineWidth){
			if(lastoutput <= lastspace){
				ret.strncat(str.c_str() + lastoutput, lastspace - lastoutput);
				lastoutput = lastspace + 1;
			}
			else{
				ret.strncat(str.c_str() + lastoutput, i + 1 - lastoutput);
				lastoutput = i + 1;
			}
			ret << "\n";
			curLineWidth = 0;
		}
	}
	ret << (str.c_str() + lastoutput);
	return ret;
}

int GLWinventory::mouse(GLwindowState &ws, int button, int state, int mx, int my){
	// Ensure the container is present because it can be destroyed anytime.
	if(!container)
		return 0;
	Autonomous *pl = container;
	GLWrect r = clientRect();
	int itemCount = pl->getInventory().size();

	InventoryListItemLocator lil(this, itemCount);
	InventoryIconItemLocator eil(this, itemCount);
	InventoryItemLocator &il = icons ? (InventoryItemLocator&)eil : (InventoryItemLocator&)lil;

	if(r.width() - 10 < mx && button == GLUT_LEFT_BUTTON && (state == GLUT_DOWN || state == GLUT_KEEP_DOWN || state == GLUT_UP) && r.height() < il.allHeight()){
		int iy = glwVScrollBarMouse(this, mx, my, r.width() - 10, 0, 10, r.height(), il.allHeight() - r.height(), scrollpos);
		if(0 <= iy)
			scrollpos = iy;
		return 1;
	}
	if(button == GLUT_RIGHT_BUTTON && state == GLUT_UP){
		// Menu items to toggle options in this GLWinventory instance.
		// It could be done by almost std::functions only, but it cannot have a weak reference.
		// Weak referencing matters when this window is destroyed before the popup menu.
		class PopupMenuItemFunction : public PopupMenuItem{
		public:
			typedef PopupMenuItem st;
			WeakPtr<GLWinventory> p;
			void (*func)(GLWinventory *);
			PopupMenuItemFunction(gltestp::dstring title, GLWinventory *p, void (*func)(GLWinventory*)) : st(title), p(p), func(func){}
			PopupMenuItemFunction(const PopupMenuItemFunction &o) : st(o.title), p(o.p), func(o.func){next = NULL;}
			void execute()override{
				if(p)
					func(p);
			}
			PopupMenuItem *clone(void)const override{return new PopupMenuItemFunction(*this);}
		};

#define APPENDER(a,s) ((a) ? "* " s : "  " s)

		glwPopupMenu(game, ws, PopupMenu()
			.append(new PopupMenuItemFunction(APPENDER(icons, "Show icons"), this, [](GLWinventory *p){p->menu_icons();}))
			.append(new PopupMenuItemFunction(APPENDER(summary, "Show summary"), this, [](GLWinventory *p){p->summary = !p->summary;})));
	}
	if(button == GLUT_LEFT_BUTTON && (state == GLUT_KEEP_UP || state == GLUT_UP)){
		int i;
		bool set = false;
		double offset = summary ? 2 * getFontHeight() : 0;

		if(r.include(mx + r.x0, my + r.y0) && mx < r.width() - 10){
			il.begin();
			for(auto it : items){
				GLWrect itemRect = il.nextRect().rmove(0, -scrollpos + offset);
				if(itemRect.include(mx + r.x0, my + r.y0)){
					if(it){
						int xs, ys;
						const long margin = 4;
						gltestp::dstring str = it->typeString() + "\n"
							+ "amount: " + it->getAmount() + "\n"
							+ "volume: " + it->getVolume() + " m^3\n"
							+ "mass: " + it->getMass() + " tons";
						if(it->getDesc())
							str << "\n\n" << wordWrap(it->getDesc(), 40);
						glwGetSizeStringML(str, GLwindow::glwfontheight, &xs, &ys);
						GLWrect localrect = GLWrect(itemRect.x0, itemRect.y0 - ys - 3 * margin, itemRect.x0 + xs + 3 * margin, itemRect.y0);

						// Adjust rect to fit in the screen. No care is taken if tips window is larger than the screen.
						if(ws.w < localrect.x1)
							localrect.rmove(ws.w - localrect.x1, 0);
						if(ws.h < localrect.y1)
							localrect.rmove(0, ws.h - localrect.y1);

						glwtip->setExtent(localrect);
						glwtip->tips = str.c_str();
						glwtip->parent = this;
						glwtip->setVisible(true);
						glwActivate(glwFindPP(glwtip));
						set = true;
					}
					break;
				}
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

void GLWinventory::mouseLeave(GLwindowState &ws) {
	if (glwtip->parent == this) {
		glwtip->tips = NULL;
		glwtip->parent = NULL;
		glwtip->setExtent(GLWrect(-10, -10, -10, -10));
	}
}



// ------------------------------
// Squirrel Initializer Implementation
// ------------------------------

SQInteger GLWinventory::sqf_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	Game *game = (Game*)sq_getforeignptr(v);
	if(!game)
		return sq_throwerror(v, _SC("The game object is not assigned"));
	Autonomous *ent = dynamic_cast<Autonomous*>(Entity::sq_refobj(v, 2));
	if(!ent)
		return sq_throwerror(v, _SC("The argument is not a valid Entity"));
	GLWinventory *p = new GLWinventory(game, ent);
	sq_assignobj(v, p);
	glwAppend(p);
	return 0;
}


bool GLWinventory::sq_define(HSQUIRRELVM v){
	st::sq_define(v);
	// Define class GLWentlist
	sq_pushstring(v, _SC("GLWinventory"), -1);
	sq_pushstring(v, st::s_sqClassName(), -1);
	sq_get(v, -3);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, "GLWinventory");
	sq_setclassudsize(v, -1, sizeof(WeakPtr<GLwindow>));
	register_closure(v, _SC("constructor"), sqf_constructor);
	sq_createslot(v, -3);
	return true;
}
