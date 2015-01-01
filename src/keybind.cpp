#include "keybind.h"
#include "cmd.h"
#include "antiglut.h"
#include "war.h"
#include "motion.h"
#include "Entity.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <windows.h>

#define DELETEKEY 0x7f

#define FCONTROL 0x1
#define FALT 0x2
#define FSPECIFIC 0x4

struct binding{
	int key;
	int flags;
	char *cmd;
};

static struct binding *binds = NULL;
static int nbinds;

static struct bindset{
	struct binding *binds;
	int n;
} *bindstack = NULL;
static int nbindstack = 0;

static int name2key(const char *name){
	if(!name || !*name)
		return 0;
	if(!name[1] && ('0' <= name[0] && name[0] <= '9' || 'A' <= name[0] && name[0] <= 'Z' || 'a' <= name[0] && name[0] <= 'z'))
		return name[0];
	else if(!strncmp(name, "numpad", sizeof"numpad"-1) && '0' <= name[sizeof"numpad"-1] && name[sizeof"numpad"-1] <= '9' && name[sizeof"numpad"] == '\0')
		return 0x90 + name[sizeof"numpad"-1] - '0';
	else if(!strcmp(name, "enter"))
		return '\n';
	else if(!strcmp(name, "semicolon"))
		return ';';
	else if(!strcmp(name, "dquote"))
		return '"';
	else if(!strcmp(name, "space"))
		return ' ';
	else if(!strcmp(name, "delete"))
		return DELETEKEY;
	else if(!strcmp(name, "tab"))
		return '\t';
	else if(!strcmp(name, "shift"))
		return '\001';
	else if(!strcmp(name, "ctrl"))
		return '\002';
	else if(!strcmp(name, "alt"))
		return '\003';
	else if(!strcmp(name, "pageup"))
		return KEYBIND_MOUSE_BASE + GLUT_KEY_PAGE_UP;
	else if(!strcmp(name, "pagedown"))
		return KEYBIND_MOUSE_BASE + GLUT_KEY_PAGE_DOWN;
	else if(!strcmp(name, "home"))
		return KEYBIND_MOUSE_BASE + GLUT_KEY_HOME;
	else if(!strcmp(name, "end"))
		return KEYBIND_MOUSE_BASE + GLUT_KEY_END;
	else if(!strcmp(name, "insert"))
		return KEYBIND_MOUSE_BASE + GLUT_KEY_INSERT;
	else if(!strcmp(name, "lclick"))
		return KEYBIND_MOUSE_BASE + GLUT_LEFT_BUTTON;
	else if(!strcmp(name, "rclick"))
		return KEYBIND_MOUSE_BASE + GLUT_RIGHT_BUTTON;
	else if(!strcmp(name, "wheelup"))
		return KEYBIND_MOUSE_BASE + GLUT_WHEEL_UP;
	else if(!strcmp(name, "wheeldown"))
		return KEYBIND_MOUSE_BASE + GLUT_WHEEL_DOWN;
	else if(!strcmp(name, "joy1"))
		return KEYBIND_JOYSTICK_BASE;
	else if(!strcmp(name, "joy2"))
		return KEYBIND_JOYSTICK_BASE + 1;
	else if(!strcmp(name, "joy3"))
		return KEYBIND_JOYSTICK_BASE + 2;
	else if(!strcmp(name, "joy4"))
		return KEYBIND_JOYSTICK_BASE + 3;
	else if(!strcmp(name, "joy5"))
		return KEYBIND_JOYSTICK_BASE + 4;
	else if(!strcmp(name, "joy6"))
		return KEYBIND_JOYSTICK_BASE + 5;
	else if(!strcmp(name, "joy7"))
		return KEYBIND_JOYSTICK_BASE + 6;
	else if((name[0] == 'f' || name[0] == 'F')){
		if(name[1] == '1' && '0' <= name[2] && name[2] <= '2')
			return 0x80 + 10 + (name[2] - '0');
		else if('1' <= name[1] && name[1] <= '9')
			return 0x80 + (name[1] - '1');
		else
			return 0;
	}
	else
		return name[0];
}

static const char *key2name(int key){
	static char ret[16];
	if('0' <= key && key <= '9' || 'A' <= key && key <= 'Z' || 'a' <= key && key <= 'z'){
		ret[0] = key;
		ret[1] = '\0';
		return ret;
	}
	else if(0x80 <= key && key <= 0x88){
		ret[0] = 'F';
		ret[1] = key - 0x80 + '1';
		ret[2] = '\0';
		return ret;
	}
	else if(0x89 <= key && key <= 0x8c){
		ret[0] = 'F';
		ret[1] = '1';
		ret[2] = key - 0x8a + '0';
		ret[3] = '\0';
		return ret;
	}
	else if(0x90 <= key && key <= 0x90 + 9){
		strncpy(ret, "numpad", sizeof "numpad"-1);
		ret[sizeof "numpad"-1] = key - 0x90 + '0';
		ret[sizeof "numpad"] = '\0';
		return ret;
	}
	else switch(key){
		case '\n': return "enter";
		case ':': return "semicolon";
		case '"': return "dquote";
		case ' ': return "space";
		case DELETEKEY: return "delete";
		case '\t': return "tab";
		case '\001': return "shift";
		case '\002': return "ctrl";
		case '\003': return "alt";
		case KEYBIND_MOUSE_BASE + GLUT_KEY_PAGE_UP: return "pageup";
		case KEYBIND_MOUSE_BASE + GLUT_KEY_PAGE_DOWN: return "pagedown";
		case KEYBIND_MOUSE_BASE + GLUT_KEY_HOME: return "home";
		case KEYBIND_MOUSE_BASE + GLUT_KEY_END: return "end";
		case KEYBIND_MOUSE_BASE + GLUT_KEY_INSERT: return "insert";
		case KEYBIND_MOUSE_BASE + GLUT_LEFT_BUTTON: return "lclick";
		case KEYBIND_MOUSE_BASE + GLUT_RIGHT_BUTTON: return "rclick";
		case KEYBIND_MOUSE_BASE + GLUT_WHEEL_UP: return "wheelup";
		case KEYBIND_MOUSE_BASE + GLUT_WHEEL_DOWN: return "wheeldown";
		case KEYBIND_JOYSTICK_BASE: return "joy1";
		case KEYBIND_JOYSTICK_BASE + 1: return "joy2";
		case KEYBIND_JOYSTICK_BASE + 2: return "joy3";
		case KEYBIND_JOYSTICK_BASE + 3: return "joy4";
		case KEYBIND_JOYSTICK_BASE + 4: return "joy5";
		case KEYBIND_JOYSTICK_BASE + 5: return "joy6";
		case KEYBIND_JOYSTICK_BASE + 6: return "joy7";
		default:
		{
			ret[0] = key;
			ret[1] = '\0';
			return ret;
		}
	}
}


int cmd_bind(int argc, char *argv[]){
/*	char args[128];*/
	char *thekey, *thevalue;
	int i, argstart = 2;
	char buf[128];
	int key;
	int flags = 0;
	size_t valuelen;
	if(argc <= 1){
		for(i = 0; i < nbinds; i++){
			sprintf(buf, "%s%s: %s", key2name(binds[i].key), binds[i].flags & FCONTROL ? "+CTRL" : "", binds[i].cmd);
			CmdPrint(buf);
		}
		sprintf(buf, "%d binds bound. stack depth is %d", nbinds, nbindstack);
		CmdPrint(buf);
		return 0;
	}
	else if(argc == 2){
		thekey = argv[1];
/*	else if(strncpy(args, arg, sizeof args), (thekey = strtok(args, " \t")) && !(thevalue = strtok(NULL, ""))){*/
		key = name2key(thekey);
		for(i = 0; i < nbinds; i++) if(key == binds[i].key){
			sprintf(buf, "%s%s: %s", key2name(binds[i].key), binds[i].flags & FCONTROL ? "+CTRL" : "", binds[i].cmd);
			CmdPrint(buf);
			return 0;
		}
		sprintf(buf, "no binds for %s is found.", (thekey));
		CmdPrint(buf);
		return -1;
	}

	if(2 < argc && !stricmp("CTRL", argv[argstart])){
		flags |= FCONTROL | FSPECIFIC;
		argstart++;
	}
	thekey = argv[1];
	for(valuelen = 0, i = argstart; i < argc; i++)
		valuelen += strlen(argv[i]) + 1;
	thevalue = (char*)malloc(valuelen);
	for(valuelen = 0, i = argstart; i < argc; i++){
		strcpy(&thevalue[valuelen], argv[i]);
		valuelen += strlen(argv[i]) + 1;
		if(i != argc - 1)
			thevalue[valuelen-1] = ' ';
	}
	{
		char *p;
		char *buf;
		p = strchr(thekey, '+');
		if(p){
			if(!stricmp(p+1, "CTRL"))
				flags |= FCONTROL | FSPECIFIC;
			p[0] = '\0';
		}
		key = name2key(thekey);
	}
	for(i = 0; i < nbinds; i++) if(key == binds[i].key && flags == binds[i].flags){
		free(binds[i].cmd);
/*		binds[i].cmd = malloc(strlen(thevalue)+1);
		strcpy(binds[i].cmd, thevalue);*/
		binds[i].cmd = thevalue;
		return 1;
	}
	binds = (struct binding*)realloc(binds, ++nbinds * sizeof *binds);
	binds[nbinds - 1].key = name2key(thekey);
	binds[nbinds - 1].flags = flags;
/*	binds[nbinds - 1].cmd = malloc(strlen(thevalue)+1);
	strcpy(binds[nbinds - 1].cmd, thevalue);*/
	binds[nbinds - 1].cmd = thevalue;
	return 1;
}

int cmd_pushbind(int argc, char *argv[]){
	int i;
	bindstack = (bindset*)realloc(bindstack, (nbindstack + 1) * sizeof *bindstack);
	bindstack[nbindstack].binds = binds;
	bindstack[nbindstack].n = nbinds;
	binds = (binding*)malloc(nbinds * sizeof *binds);
	memcpy(binds, bindstack[nbindstack].binds, nbinds * sizeof *binds);
	for(i = 0; i < nbinds; i++){
		binds[i].cmd = (char*)malloc(strlen(bindstack[nbindstack].binds[i].cmd) + 1);
		strcpy(binds[i].cmd, bindstack[nbindstack].binds[i].cmd);
	}
	nbindstack++;
	return 0;
}

int cmd_popbind(int argc, char *argv[]){
	int i;
	if(!nbindstack){
		CmdPrint("Bind Stack Underflow!");
		return 1;
	}
	for(i = 0; i < nbinds; i++)
		free(binds[i].cmd);
	realloc(binds, 0);
	nbindstack--;
	binds = bindstack[nbindstack].binds;
	nbinds = bindstack[nbindstack].n;
	bindstack = (bindset*)realloc(bindstack, nbindstack * sizeof *bindstack);
	return 0;
}

void BindExec(int key){
	int i;
	int control;
	control = GetKeyState(VK_CONTROL) & 0x7000;
	for(i = 0; i < nbinds; i++) if(binds[i].key == key && (key == '\002' || !(binds[i].flags & FCONTROL) ^ !!control)){
		CmdExec(binds[i].cmd);
		return;
	}
}

void BindExecSpecial(int key){
	if(GLUT_KEY_F1 <= key && key <= GLUT_KEY_F12){
		int i;
		unsigned char keycode = key - GLUT_KEY_F1 + 0x80;
		for(i = 0; i < nbinds; i++) if(binds[i].key == keycode){
			CmdExec(binds[i].cmd);
			return;
		}
	}
	switch(key){
		int i;
		case GLUT_KEY_PAGE_UP:
		case GLUT_KEY_PAGE_DOWN:
		case GLUT_KEY_HOME:
		case GLUT_KEY_END:
		case GLUT_KEY_INSERT:
			for(i = 0; i < nbinds; i++) if(binds[i].key == key + KEYBIND_MOUSE_BASE){
				CmdExec(binds[i].cmd);
				return;
			}
			break;
	}
}

void BindKeyUp(int key){
	int i;
	for(i = 0; i < nbinds; i++) if(key == toupper(binds[i].key) && binds[i].cmd[0] == '+'){
		binds[i].cmd[0] = '-';
		CmdExec(binds[i].cmd);
		binds[i].cmd[0] = '+';
	}
}

void BindKillFocus(void){
	int i;
	for(i = 0; i < nbinds; i++) if(binds[i].cmd[0] == '+'){
		binds[i].cmd[0] = '-';
		CmdExec(binds[i].cmd);
		binds[i].cmd[0] = '+';
	}
}



bool JoyStick::InitJoystick()
{
	if(init)
		return true;
	// Make sure joystick driver is present
	UINT uiNumJoysticks;
	if ((uiNumJoysticks = joyGetNumDevs()) == 0)
		return false;

	// Make sure the joystick is attached
	JOYINFO jiInfo;
	unsigned i;
	for(i = m_uiJoystickID; i < uiNumJoysticks; i++){
		MMRESULT mr = joyGetPos(i, &jiInfo);
		if (mr == JOYERR_NOERROR){
			m_uiJoystickID = i;
			break;
		}
	}
	if(i == uiNumJoysticks){
		return false;
	}

	// Calculate the trip values
	JOYCAPS jcCaps;
	joyGetDevCaps(m_uiJoystickID, &jcCaps, sizeof(JOYCAPS));
	DWORD dwXCenter = ((DWORD)jcCaps.wXmin + jcCaps.wXmax) / 2;
	DWORD dwYCenter = ((DWORD)jcCaps.wYmin + jcCaps.wYmax) / 2;
	DWORD dwZCenter = ((DWORD)jcCaps.wZmin + jcCaps.wZmax) / 2;
	DWORD dwRCenter = ((DWORD)jcCaps.wRmin + jcCaps.wRmax) / 2;
	m_rcJoystickTrip.left = (jcCaps.wXmin + (WORD)dwXCenter) / 2;
	m_rcJoystickTrip.right = (jcCaps.wXmax + (WORD)dwXCenter) / 2;
	m_rcJoystickTrip.top = (jcCaps.wYmin + (WORD)dwYCenter) / 2;
	m_rcJoystickTrip.bottom = (jcCaps.wYmax + (WORD)dwYCenter) / 2;
	m_rcJoystickTrip2.left = (jcCaps.wZmin + (WORD)dwZCenter) / 2;
	m_rcJoystickTrip2.right = (jcCaps.wZmax + (WORD)dwZCenter) / 2;
	m_rcJoystickTrip2.top = (jcCaps.wRmin + (WORD)dwRCenter) / 2;
	m_rcJoystickTrip2.bottom = (jcCaps.wRmax + (WORD)dwRCenter) / 2;

	init = true;
	return true;
}

void JoyStick::CaptureJoystick()
{
	// Capture the joystick
//	if (m_uiJoystickID == JOYSTICKID1)
//		joySetCapture(hWndApp, m_uiJoystickID, NULL, TRUE);
}

void JoyStick::ReleaseJoystick()
{
	// Release the joystick
//	if (m_uiJoystickID == JOYSTICKID1)
//		joyReleaseCapture(m_uiJoystickID);
}

void JoyStick::CheckJoystick(input_t &input)
{
	if (m_uiJoystickID == JOYSTICKID1 || m_uiJoystickID == JOYSTICKID2){
		// Reserve old press state for taking differences
		int oldpress = input.press;
		JOYINFOEX jiInfo;
		JOYSTATE jsJoystickState = 0;

		// JOYINFOEX has members that need to be initialized before using.
		jiInfo.dwSize = sizeof jiInfo;
		jiInfo.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNR | JOY_RETURNBUTTONS;

		if(joyGetPosEx(m_uiJoystickID, &jiInfo) == JOYERR_NOERROR){
			// Check horizontal movement
			if (jiInfo.dwXpos < (WORD)m_rcJoystickTrip.left)
				jsJoystickState |= JOY_LEFT;
			else if (jiInfo.dwXpos > (WORD)m_rcJoystickTrip.right)
				jsJoystickState |= JOY_RIGHT;
			input.analog[0] = jiInfo.dwXpos / ((m_rcJoystickTrip.left + m_rcJoystickTrip.right) / 2.) - 1.;

			// Check vertical movement
			if (jiInfo.dwYpos < (WORD)m_rcJoystickTrip.top)
				jsJoystickState |= JOY_UP;
			else if (jiInfo.dwYpos > (WORD)m_rcJoystickTrip.bottom)
				jsJoystickState |= JOY_DOWN;
			input.analog[1] = jiInfo.dwYpos / ((m_rcJoystickTrip.top + m_rcJoystickTrip.bottom) / 2.) - 1.;

			// Check horizontal movement of right joystick
			if (jiInfo.dwZpos < (WORD)m_rcJoystickTrip2.left)
				jsJoystickState |= JOY_LEFT;
			else if (jiInfo.dwZpos > (WORD)m_rcJoystickTrip2.right)
				jsJoystickState |= JOY_RIGHT;
			input.analog[2] = jiInfo.dwZpos / ((m_rcJoystickTrip2.left + m_rcJoystickTrip2.right) / 2.) - 1.;

			// Check vertical movement of right joystick
			if (jiInfo.dwRpos < (WORD)m_rcJoystickTrip2.top)
				jsJoystickState |= JOY_UP;
			else if (jiInfo.dwRpos > (WORD)m_rcJoystickTrip2.bottom)
				jsJoystickState |= JOY_DOWN;
			input.analog[3] = jiInfo.dwRpos / ((m_rcJoystickTrip2.top + m_rcJoystickTrip2.bottom) / 2.) - 1.;

			// Check buttons
			if(jiInfo.dwButtons & JOY_BUTTON1)
				/*input.press |= PL_ENTER,*/ jsJoystickState |= JOY_FIRE1;
			if(jiInfo.dwButtons & JOY_BUTTON2)
				/*input.press |= PL_RCLICK,*/ jsJoystickState |= JOY_FIRE2;
			if(jiInfo.dwButtons & JOY_BUTTON3)
				jsJoystickState |= JOY_FIRE3;
			if(jiInfo.dwButtons & JOY_BUTTON4)
				jsJoystickState |= JOY_FIRE4;
			if(jiInfo.dwButtons & JOY_BUTTON5)
				jsJoystickState |= JOY_FIRE5;
			if(jiInfo.dwButtons & JOY_BUTTON6)
				jsJoystickState |= JOY_FIRE6;
			if(jiInfo.dwButtons & JOY_BUTTON7)
				jsJoystickState |= JOY_FIRE7;

			// Compare new and old state and indicate changed keys.
//			input.change ^= (input.press ^ oldpress) & (PL_ENTER | PL_RCLICK);
			if((m_joyState ^ jsJoystickState) & JOY_FIRE1)
				(jsJoystickState & JOY_FIRE1 ? BindExec : BindKeyUp)(KEYBIND_JOYSTICK_BASE);
			if((m_joyState ^ jsJoystickState) & JOY_FIRE2)
				(jsJoystickState & JOY_FIRE2 ? BindExec : BindKeyUp)(KEYBIND_JOYSTICK_BASE + 1);
			if((m_joyState ^ jsJoystickState) & JOY_FIRE3)
				(jsJoystickState & JOY_FIRE3 ? BindExec : BindKeyUp)(KEYBIND_JOYSTICK_BASE + 2);
			if((m_joyState ^ jsJoystickState) & JOY_FIRE4)
				(jsJoystickState & JOY_FIRE4 ? BindExec : BindKeyUp)(KEYBIND_JOYSTICK_BASE + 3);
			if((m_joyState ^ jsJoystickState) & JOY_FIRE5)
				(jsJoystickState & JOY_FIRE5 ? BindExec : BindKeyUp)(KEYBIND_JOYSTICK_BASE + 4);
			if((m_joyState ^ jsJoystickState) & JOY_FIRE6)
				(jsJoystickState & JOY_FIRE6 ? BindExec : BindKeyUp)(KEYBIND_JOYSTICK_BASE + 5);
			if((m_joyState ^ jsJoystickState) & JOY_FIRE7)
				(jsJoystickState & JOY_FIRE7 ? BindExec : BindKeyUp)(KEYBIND_JOYSTICK_BASE + 6);

			m_joyState = jsJoystickState;
		}
	}
}