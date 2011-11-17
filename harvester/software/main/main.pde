/*
This reads serial data from the harvester.
*/

import processing.serial.*;

Serial myPort;
PrintWriter output;

int[] Buff = new int[7];
int i, j, rcState;

void setup() {
  //println(Serial.list());

  myPort = new Serial(this, Serial.list()[0], 2400);
  
  rcState = 0;
  j = 0;
  
  output = createWriter("measurements.csv");
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
          
          output.println(gain_mode + ", " + red_reg + ", " + grn_reg + ", " + blu_reg);
        }
        break;
    }
  }  
}

void keyPressed() {
  output.flush();
  output.close();
  exit();
}
