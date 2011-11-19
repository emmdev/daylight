
#include "calc.h"

char *calc(int Ru, int Gu, int Bu)
{
static char retbuf[3];
/*
int max_val1;
int Yu, Zu;
int Rc, Gc, Bc;

__code int m[3][3] = {
  {29, -2, 1},
  {-4, 127, -17},
  {-1, -26, 56}
};

//get R, G, B into 8 bit range
//find max_val1
max_val1 = Ru;
if (Gu > max_val1)
	max_val1 = Gu;
if (Bu > max_val1)
	max_val1 = Bu;
//shift down as necessary
while (max_val1 > 0x00ff) {
	max_val1 >>= 1;
	Ru >>= 1;
	Gu >>= 1;
	Bu >>= 1;
}
//shift up as necessary
while (max_val1 < 0x007f) {
	max_val1 <<= 1;
	Ru <<= 1;
	Gu <<= 1;
	Bu <<= 1;
}

Yu = Bu + 3*Gu + Ru;
	//worst case (R,G,B)/Yu is 1

Zu = 0xffff/Yu;

Ru = (Ru*Zu) >> 6;
Gu = (Gu*Zu) >> 6;
Bu = (Bu*Zu) >> 6;

Rc = (Ru*m[0][0] + Gu*m[1][0] + Bu*m[2][0]) >> 7;
Gc = (Ru*m[0][1] + Gu*m[1][1] + Bu*m[2][1]) >> 7;
Bc = (Ru*m[0][2] + Gu*m[1][2] + Bu*m[2][2]) >> 7;

//Rc = 57;
//Gc = 151;
//Bc = 48;

retbuf[0] = (unsigned char)Rc;
retbuf[1] = (unsigned char)Gc;
retbuf[2] = (unsigned char)Bc;
*/
char i, j, k;
for (i=0; i<2; i++) {
    for (j=0; j<2; j++) {
        nop();nop();nop();nop();nop();
        nop();nop();nop();nop();nop();
        nop();nop();nop();nop();nop();
        nop();nop();nop();nop();nop();
        nop();nop();nop();nop();nop();
        nop();nop();nop();nop();nop();
        nop();nop();nop();nop();nop();
        nop();nop();nop();nop();nop();
    }
}

return retbuf;
}