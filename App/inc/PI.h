#ifndef PI_H
#define PI_H

#include "DCLF32.h"

typedef struct {
    DCL_DF22  df22;
    float     Kp;
    float     outMax;
    float     outMin;
} PI_Handle;

extern void PI_Init(PI_Handle *pi, float Kp, float Ki, float Ts);
extern void PI_SetClamp(PI_Handle *pi, float max, float min);
extern void PI_Reset(PI_Handle *pi);
extern float PI_Calc(PI_Handle *pi, float err);

#endif
