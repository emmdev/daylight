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
#include "i2c.h"

typedef unsigned int config;
config at 0x2007 __CONFIG = _CP_OFF & 
 _WDT_OFF & 
 _BODEN_OFF & 
 _PWRTE_OFF & 
 _INTRC_IO & 
 _MCLR_ON & 
 _LVP_OFF;


#define LED_PIN RB4

#define nop() \
        _asm\
        nop\
        _endasm


//will not be part of lcd library:
unsigned char Lcd_Ready;

unsigned char TX_Buf[16];

void write_int(unsigned int, unsigned char, unsigned char);


void intHand(void) __interrupt 0
{
    static unsigned char led_count;

    if (TMR0IE && TMR0IF) {
        Lcd_Ready = 1;
        if (led_count > 49) {
            LED_PIN = !LED_PIN;
            led_count = 0;
        } else {
            led_count++;
        }

        TMR0IF=0;
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
    OPTION_REG = 4; // 1:32 prescaler, giving XLCD 4.1ms for Cmd cycles
    TMR0IE = 1;
    TMR0 = 0;
    
    //serial port (TRISB{5,2} are set above)
    // 8 bits, ASYNC, 2400 baud, TX only
    SPBRG = 51;
    RCSTA = 0b10000000;
    TXSTA = 0b00100000;
    
}


void main(void) {
    unsigned char i, j;//, temp;
    unsigned int sen_long, sen_medm, sen_shrt;
    setup();

    //clear TX Buff
    for (j = 0; j < 16; j++) {
        TX_Buf[j] = ' ';
    }

    //turn on
    i2c_start();
    i2c_tx(0b01110010);
    i2c_tx(0x00|0b10100000);
    i2c_tx(0b00000011);
    i2c_stop();
    
    //set timing
    i2c_start();
    i2c_tx(0b01110010);
    i2c_tx(0x01|0b10100000);
    i2c_tx(0b00000010);
    i2c_stop();
    //set gain
    i2c_start();
    i2c_tx(0b01110010);
    i2c_tx(0x07|0b10100000);
    i2c_tx(0b00100000);
    i2c_stop();

            
    while (1) {
        if (TXIF) // ready for new word
        {
            j++;
            if (j > 15)
                j = 0;
            TXREG = TX_Buf[j];
        }

        if (Lcd_Ready) {
            Lcd_Ready = 0;
            
            i2c_start();
            i = i2c_tx(0b01110010);
            i2c_tx(0x10|0b10100000);
            i2c_start();
            i2c_tx(0b01110011);
            
            sen_medm = i2c_rx(1);
            sen_medm += 256 * i2c_rx(1);
            sen_long = i2c_rx(1);
            sen_long += 256 * i2c_rx(1);
            sen_shrt = i2c_rx(1);
            sen_shrt += 256 * i2c_rx(0);
            i2c_stop();
            
/*
            lcd_write_int(i, 1, 0, 1);
            lcd_write_int(sen_long, 0, 0, 5);
            lcd_write_int(sen_medm, 0, 6, 5);
            lcd_write_int(sen_shrt, 1, 2, 5);*/
            TX_Buf[0] = 'h';
            TX_Buf[1] = 'e';
            TX_Buf[2] = 'l';
            TX_Buf[3] = 'l';
            TX_Buf[4] = 'o';
            TX_Buf[14] = 13;
            TX_Buf[15] = 10;

            write_int(0xffff, 6, 5);

        }
    }
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
