/* main.c

2010-11-22
    added i2c code; resolved issue with code pages?
2010-11-29
    fixed i2c code to avoid read-modify-write issue!
2011-11-04
    start afresh!
    led blinking
    serial port sending 'y' repeatedly
    serial port sending "hello"
2011-11-5
    serial port sending 16-bit integers
*/

#define __16f88
#include "pic/pic16f88.h"

#include "bitop.h"
//include "colours.h"
#include "calc.h"

typedef unsigned int config;
config at 0x2007 __CONFIG = _CP_OFF &
 _WDT_OFF &
 _BODEN_OFF &
 _PWRTE_OFF &
 _INTRC_IO &
 _MCLR_ON &
 _LVP_OFF;


#define LED_PIN RB4

#define CS_PIN RA1
#define SCK_PIN RA0
#define SDI_PIN RA7

#define nop() \
_asm\
    nop\
_endasm

unsigned char TX_Buf[16];
unsigned char pwm_ch0, pwm_ch1, pwm_ch2, pwm_ch3, pwm_ch4, pwm_ch6, pwm_ch7;
unsigned char rx_Buf[7];
unsigned char rcState, inByte; //Why do these need to be global?
char nextColour;

unsigned int red_reg, grn_reg, blu_reg;


void write_spi(unsigned char);
void write_int(unsigned int, unsigned char, unsigned char);


void intHand(void) __interrupt 0
{
    static unsigned char led_count;
    static unsigned char j;
    
    if (TMR0IE && TMR0IF) {
        TMR0 = 0xff - 180; //11.6 ms

        if (led_count > 99) {
            LED_PIN = !LED_PIN;
            led_count = 0;
            
            nextColour = 1;
        } else {
            led_count++;
        }

        write_spi(pwm_ch0);
        write_spi(pwm_ch1);
        write_spi(pwm_ch2);
        write_spi(pwm_ch3);
        write_spi(pwm_ch4);
        write_spi(pwm_ch6);
        write_spi(pwm_ch7);

        TMR0IF = 0;
    }
    
    if (RCIE && RCIF) { //Why is RCIE always TRUE?
        inByte = RCREG;    
        
        if (OERR) {
            CREN = 0;
            CREN = 1;
        }
        
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
                rx_Buf[j] = inByte;
                j++;
                if (j >= 7) {
                    j = 0;
                    rcState = 0;
                    blu_reg = rx_Buf[5] + 256*rx_Buf[6]; //blue
                    grn_reg = rx_Buf[3] + 256*rx_Buf[4]; //green
                    red_reg = rx_Buf[1] + 256*rx_Buf[2]; //red
                }
                break;
        }
    }
}

void setup(void) {
    OSCCON=0b01110000; //8MHz

    //Ports
    TRISA=0b00000000;
    PORTA=0b00000000;
    TRISB=0b00100100;
    PORTB=0b00000000;

    CMCON=0b00000111; //Turn off comparator on RA port
    //shouldn't be needed due to POR defaults
    
    ANSEL=0;//This is needed! PORTA defaults to analog input!


    GIE = 1; //Enable interrupts

    //Initialize Timer0 - used for LCD refresh rate and long-term timebase
    OPTION_REG = 6; // 1:32 prescaler, giving XLCD 4.1ms for Cmd cycles
    TMR0IE = 1;
    TMR0 = 0;
    
    //serial port (TRISB{5,2} are set above)
    // 8 bits, ASYNC, 2400 baud, TX only
    RCIE = 1;
    PEIE = 1;
    SPBRG = 51;
    RCSTA = 0b10010000;
    TXSTA = 0b00100000;
}


void main(void) {
    unsigned char i, j;
//char *retbuf;

    setup();

    //clear TX Buff
    for (j = 0; j < 16; j++) {
        TX_Buf[j] = ' ';
    }
    
    rcState = 0;
    while (1) {
/*
        if (TXIF) // ready for new word
        {
            j++;
            if (j > 15)
                j = 0;
            TXREG = TX_Buf[j];
        }*/
/*
        if (nextColour) {
            nextColour = 0;
            pwm_ch4 = colours[i][0];
            pwm_ch3 = colours[i][1];
            pwm_ch2 = colours[i][2];
            i++;
            if (i >= COLOURS_SIZE)
                i = 0;
        }*/

unsigned int Ru, Gu, Bu;
unsigned int max_val1;
unsigned int Yu, Zu;
int Rc, Gc, Bc;

__code int m[3][3] = {
  {29, -2, 1},
  {-4, 127, -17},
  {-1, -26, 56}
};

Ru = red_reg;
Gu = grn_reg;
Bu = blu_reg;

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

//Rc = 53;
//Gc = 153;
//Bc = 47;

pwm_ch4 = (unsigned char)Rc;
pwm_ch3 = (unsigned char)Gc;
pwm_ch2 = (unsigned char)Bc;

/*
pwm_ch4 = (unsigned char)(red_reg >> 8);
pwm_ch3 = (unsigned char)(grn_reg >> 8);
pwm_ch2 = (unsigned char)(blu_reg >> 8);*/
    }
}


void write_spi(unsigned char spi_word)
{
    unsigned char x;
    
    CS_PIN = 0; //select it
    for(x = 8; x; x--) {
        SCK_PIN = 0;
        if (spi_word & 0x80) //setup data
            SDI_PIN = 1;
        else
            SDI_PIN = 0;
        SCK_PIN = 1; //clock it in
        spi_word <<= 1;
    }
    CS_PIN = 1;
}

void write_int(unsigned int num, unsigned char col, unsigned char num_digits)
// writes the ascii representation of an integer to the TX Buffer
{
    unsigned int digit, s;

    switch (num_digits) {
        case 4: goto four_digits;
        case 3: goto three_digits;
        case 2: goto two_digits;
        case 1: goto one_digit;
    }
    
//five_digits:
    digit = num / 10000;
    s = digit * 10000;
    num = num - s;
    TX_Buf[col] = '0' + digit;
    col++;
    
four_digits:
    digit = num / 1000;
    s = digit * 1000;
    num = num - s;
    TX_Buf[col] = '0' + digit;
    col++;

three_digits:
    digit = num / 100;
    s = 100 * digit;
    num = num - s;
    TX_Buf[col] = '0' + digit;
    col++;

two_digits:
    digit = num / 10;
    s = digit * 10;
    num = num - s;
    TX_Buf[col] = '0' + digit;
    col++;

one_digit:
    TX_Buf[col] = '0' + num;
}