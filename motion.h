/** \file
 * \brief Definiton of functions for monitoring keystrokes to implement toggle or momentary keys. */
#ifndef MOTION_H
#define MOTION_H


/* Bitmask for buttons, moved from war.h.
 * I'm not sure this is right place. Probably a new header is the right place. */
#define PL_W 0x01
#define PL_S 0x02
#define PL_A 0x04
#define PL_D 0x08
#define PL_Q 0x10
#define PL_Z 0x20
#define PL_E 0x8000
#define PL_8 0x40
#define PL_2 0x80
#define PL_4 0x100
#define PL_6 0x200
#define PL_7 0x400
#define PL_9 0x800
#define PL_SPACE 0x1000
#define PL_ENTER 0x2000
#define PL_TAB 0x4000
#define PL_LCLICK 0x10000 /* left mouse button */
#define PL_RCLICK 0x20000
#define PL_B 0x40000
#define PL_G 0x80000
#define PL_R   0x100000
#define PL_MWU 0x200000 /* mouse wheel up */
#define PL_MWD 0x400000 /* mouse wheel down */
#define PL_F   0x800000
#define PL_SHIFT 0x1000000
#define PL_CTRL  0x2000000
#define PL_ALT  0x4000000


void MotionInit();
int MotionFrame(double dt);
void MotionAnim(class Player &pl, double dt, double accel);
int MotionGet();
int MotionSet(int mask, int state);
int MotionGetChange();
int MotionGetToggle();
int MotionSetToggle(int mask, int state);

#endif
