#include "keybind.h"
#include "cmd.h"
#include "antiglut.h"
#include <string.h>

#define DELETEKEY 0x7f

struct binding{
	unsigned char key;
	char *cmd;
};

static struct binding *binds = NULL;
static int nbinds;

static struct bindset{
	struct binding *binds;
	int n;
} *bindstack = NULL;
static int nbindstack = 0;

static unsigned char name2key(const char *name){
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
	else if(!strcmp(name, "lclick"))
		return '\004';
	else if(!strcmp(name, "rclick"))
		return '\005';
	else if((name[0] == 'f' || name[0] == 'F')){
		if(name[1] == '1' && '0' <= name[2] && name[2] <= '2')
			return 0x80 + 10 + (name[2] - '0');
		else if('1' <= name[1] && name[1] <= '9')
			return 0x80 + (name[1] - '1');
	}
	else
		return name[0];
}

static const char *key2name(unsigned char key){
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
		case '\004': return "lclick";
		case '\005': return "rclick";
		default:
		{
			ret[0] = key;
			ret[1] = '\0';
			return ret;
		}
	}
}


int cmd_bind(int argc, char *argv[]){
	char args[128], *thekey, *thevalue;
	int i;
	char buf[128];
	unsigned char key;
	size_t valuelen;
	if(argc <= 1){
		for(i = 0; i < nbinds; i++){
			sprintf(buf, "%s: %s", key2name(binds[i].key), binds[i].cmd);
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
			sprintf(buf, "%s: %s", key2name(binds[i].key), binds[i].cmd);
			CmdPrint(buf);
			return 0;
		}
		sprintf(buf, "no binds for %s is found.", (thekey));
		CmdPrint(buf);
		return -1;
	}
	thekey = argv[1];
	for(valuelen = 0, i = 2; i < argc; i++)
		valuelen += strlen(argv[i]) + 1;
	thevalue = malloc(valuelen);
	for(valuelen = 0, i = 2; i < argc; i++){
		strcpy(&thevalue[valuelen], argv[i]);
		valuelen += strlen(argv[i]) + 1;
		if(i != argc - 1)
			thevalue[valuelen-1] = ' ';
	}
	key = name2key(thekey);
	for(i = 0; i < nbinds; i++) if(key == binds[i].key){
		free(binds[i].cmd);
/*		binds[i].cmd = malloc(strlen(thevalue)+1);
		strcpy(binds[i].cmd, thevalue);*/
		binds[i].cmd = thevalue;
		return 1;
	}
	binds = (struct binding*)realloc(binds, ++nbinds * sizeof *binds);
	binds[nbinds - 1].key = name2key(thekey);
/*	binds[nbinds - 1].cmd = malloc(strlen(thevalue)+1);
	strcpy(binds[nbinds - 1].cmd, thevalue);*/
	binds[nbinds - 1].cmd = thevalue;
	return 1;
}

int cmd_pushbind(int argc, char *argv[]){
	int i;
	bindstack = realloc(bindstack, (nbindstack + 1) * sizeof *bindstack);
	bindstack[nbindstack].binds = binds;
	bindstack[nbindstack].n = nbinds;
	binds = malloc(nbinds * sizeof *binds);
	memcpy(binds, bindstack[nbindstack].binds, nbinds * sizeof *binds);
	for(i = 0; i < nbinds; i++){
		binds[i].cmd = malloc(strlen(bindstack[nbindstack].binds[i].cmd) + 1);
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
	bindstack = realloc(bindstack, nbindstack * sizeof *bindstack);
	return 0;
}

void BindExec(int key){
	int i;
	for(i = 0; i < nbinds; i++) if(binds[i].key == key){
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
