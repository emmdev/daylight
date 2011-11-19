#ifndef CALC_H
#define CALC_H

#include "pic/pic16f88.h"

#define nop() \
_asm\
    nop\
_endasm

char *calc(int, int, int);

#endif