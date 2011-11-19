/*
This reads serial data from the harvester.
*/

import processing.serial.*;

Serial myPort;
PrintWriter output;

int[] Buff = new int[7];
int i, j, rcState;
float X;

void setup() {
  //println(Serial.list());

  myPort = new Serial(this, Serial.list()[0], 2400);
  
  rcState = 0;
  j = 0;
  
  X = 100000;
  
//  output = createWriter("measurements.csv");
}


void draw() {
  while (myPort.available() > 0) {
    int inByte = myPort.read();
    
    // receive state machine
    switch (rcState) {
      case 0: //start from scratch
        if (inByte == 'n') {
          rcState = 1;
        }
        break;
      case 1: //received at least 1 'n'
        if (inByte == 'y') {
          rcState = 2;
        } else if (inByte != 'n') {
          rcState = 0;
        }
        break;
      case 2: //receive array
        Buff[j] = inByte;
        j++;
        if (j >= 7) {
          j = 0;
          rcState = 0;
          println("new data:");
//          println(Buff);
          int gain_mode = Buff[0];
          int red_reg = Buff[1] + 256 * Buff[2];
          int grn_reg = Buff[3] + 256 * Buff[4];
          int blu_reg = Buff[5] + 256 * Buff[6];
          println(gain_mode);
          println(red_reg);
          println(grn_reg);
          println(blu_reg);
          
//          output.println(gain_mode + ", " + red_reg + ", " + grn_reg + ", " + blu_reg);

int max_val1;
int Ru = red_reg;
int Gu = grn_reg;
int Bu = blu_reg;
int Yu, Zu;
int Rc, Gc, Bc;

int m[][] = {
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

println(Ru + " " + Gu + " " + Bu);

Rc = (Ru*m[0][0] + Gu*m[1][0] + Bu*m[2][0]) >> 7;
Gc = (Ru*m[0][1] + Gu*m[1][1] + Bu*m[2][1]) >> 7;
Bc = (Ru*m[0][2] + Gu*m[1][2] + Bu*m[2][2]) >> 7;

println(Rc + " " + Gc + " " + Bc);
        }
        break;
    }
  }  
}

void keyPressed() {
//  output.flush();
//  output.close();
  exit();
}
