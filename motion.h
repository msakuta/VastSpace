#ifndef MOTION_H
#define MOTION_H

void MotionInit();
int MotionFrame(double dt);
void MotionAnim(class Player &pl, double dt);

#endif
