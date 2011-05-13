#ifndef KEYBIND_H
#define KEYBIND_H

#ifdef __cplusplus
extern "C"{
#endif

#define KEYBIND_MOUSE_BASE 0x200
#define KEYBIND_JOYSTICK_BASE 0x400

int cmd_bind(int argc, char *argv[]);
int cmd_pushbind(int argc, char *argv[]);
int cmd_popbind(int argc, char *argv[]);
void BindExec(int key);
void BindExecSpecial(int key);
void BindKeyUp(int key);
void BindKillFocus(void);

#ifdef __cplusplus
}

struct input_t;
class JoyStick{
public:
	typedef unsigned  JOYSTATE;
	static const JOYSTATE JOY_NONE = 0x0000L,
        JOY_LEFT = 0x0001L,
        JOY_RIGHT = 0x0002L,
        JOY_UP  = 0x0004L,
        JOY_DOWN = 0x0008L,
        JOY_FIRE1 = 0x0010L,
        JOY_FIRE2 = 0x0020L,
        JOY_FIRE3 = 0x0040L,
        JOY_FIRE4 = 0x0080L,
        JOY_FIRE5 = 0x0100L,
        JOY_FIRE6 = 0x0200L,
        JOY_FIRE7 = 0x0400L;
	JoyStick(unsigned id) : m_uiJoystickID(id), init(false), m_joyState(0){}
	bool InitJoystick();
	double getX()const;
	double getY()const;
	void CaptureJoystick();
	void ReleaseJoystick();
	void CheckJoystick(input_t &input);
	struct RECT{
		long left, top, right, bottom;
	};
private:
	unsigned m_uiJoystickID;
	RECT m_rcJoystickTrip;
	RECT m_rcJoystickTrip2;
	JOYSTATE m_joyState;
	bool init;
};
#endif

#endif
