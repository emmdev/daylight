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


typedef unsigned int config;
config at 0x2007 __CONFIG = _CP_OFF &
 _WDT_OFF &
 _BODEN_OFF &
 _PWRTE_OFF &
 _INTRC_IO &
 _MCLR_ON &
 _LVP_OFF;


#define LED_PIN RB4
#define TEST_PIN RB6

#define CS_PIN RA1
#define SCK_PIN RA0
#define SDI_PIN RA7

#define nop() \
_asm\
    nop\
_endasm


//will not be part of lcd library:
unsigned char Lcd_Ready;

unsigned char TX_Buf[16];
unsigned char pwm_ch0, pwm_ch1, pwm_ch2, pwm_ch3, pwm_ch4, pwm_ch6, pwm_ch7;

void write_spi(unsigned char);
void write_int(unsigned int, unsigned char, unsigned char);


void intHand(void) __interrupt 0
{
    static unsigned char led_count;
    
    static char i;

    if (TMR0IE && TMR0IF) {
        TMR0 = 0xff - 180; //11.6 ms
        Lcd_Ready = 1;
        if (led_count > 49) {
            LED_PIN = !LED_PIN;
            led_count = 0;
        } else {
            led_count++;
        }

        TEST_PIN = 1;

write_spi(pwm_ch0);
write_spi(pwm_ch1);
write_spi(pwm_ch2);
write_spi(pwm_ch3);
write_spi(pwm_ch4);
write_spi(pwm_ch6);
write_spi(pwm_ch7);


        TEST_PIN = 0;
        TMR0IF = 0;
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
    SPBRG = 51;
    RCSTA = 0b10000000;
    TXSTA = 0b00100000;
    
}


void main(void) {
    unsigned char i, j;//, temp;

    setup();

    //clear TX Buff
    for (j = 0; j < 16; j++) {
        TX_Buf[j] = ' ';
    }
    
    //IT DOESNT LIKE 255?!
    pwm_ch2 = 254; //blue
    pwm_ch3 = 0; //green
    pwm_ch4 = 0; //red
    
            
    while (1) {
/*        if (TXIF) // ready for new word
        {
            j++;
            if (j > 15)
                j = 0;
            TXREG = TX_Buf[j];
        }*/

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