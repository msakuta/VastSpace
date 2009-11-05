#ifndef MOTION_H
#define MOTION_H

void MotionInit();
int MotionFrame(double dt);
void MotionAnim(class Player &pl, double dt, double accel);
int MotionGet();
int MotionSet(int mask, int state);
int MotionGetChange();
int MotionGetToggle();
int MotionSetToggle(int mask, int state);

#endif
