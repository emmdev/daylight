/*
This reads serial data from the harvester.
*/

import processing.serial.*;

Serial myPort;

int[] Buff = new int[4];
int i, j, rcState;

void setup() {
  //println(Serial.list());

  myPort = new Serial(this, Serial.list()[0], 2400);
  
  rcState = 0;
  j = 0;
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
        if (j >= 4) {
          j = 0;
          rcState = 0;
          println("new data:");
          println(Buff);
          int value = Buff[0] + 256 * Buff[1];
          println(value);
        }
        break;
    }
  }  
}

