#include "glw/glwindow.h"
#include "sqadapt.h"
#include "Autonomous.h"
#include "Inventory.h"

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
public:
	typedef GLwindowSizeable st;

	bool icons;
	int scrollpos; ///< Current scroll position in the vertical scroll bar.
	GLWinventory(Game *game, Autonomous *container);
	virtual void draw(GLwindowState &ws, double);
//	virtual int mouse(GLwindowState &ws, int button, int state, int mx, int my);
//	virtual void mouseLeave(GLwindowState &ws);

	static SQInteger sqf_constructor(HSQUIRRELVM v);
	static bool sq_define(HSQUIRRELVM v);
};

static sqa::Initializer definer("GLWinventory", GLWinventory::sq_define);

GLWinventory::GLWinventory(Game *game, Autonomous *container) :
	st(game, "Inventory"),
	container(container),
	icons(false),
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
	InventoryIconItemLocator(GLWinventory *p, int count, int iconSize = 32) : p(p), count(count), i(0), iconSize(iconSize){}
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

	std::vector<const InventoryItem*> items;

	for(auto it : container->getInventory())
		items.push_back(it);

	GLWrect r = clientRect();
	int wid = int((r.width() - 10) / getFontWidth()) -  6;

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
	glScissor(r.x0, ws.h - r.y1, r.width(), r.height());
	glEnable(GL_SCISSOR_TEST);

	int x = 0, y = 0;
	glPushMatrix();
	glTranslated(0, -scrollpos, 0);
	for(auto it : items){
		GLWrect iconRect = il.nextRect();
			
		if(iconRect.include(ws.mx, ws.my + scrollpos)){
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
