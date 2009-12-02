#ifndef KEYBIND_H
#define KEYBIND_H

#ifdef __cplusplus
extern "C"{
#endif

#define KEYBIND_MOUSE_BASE 0x200

int cmd_bind(int argc, char *argv[]);
int cmd_pushbind(int argc, char *argv[]);
int cmd_popbind(int argc, char *argv[]);
void BindExec(int key);
void BindExecSpecial(int key);
void BindKeyUp(int key);
void BindKillFocus(void);

#ifdef __cplusplus
}
#endif
#endif
