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


#define LED_PIN RB3

#define nop() \
_asm\
    nop\
_endasm


unsigned char pwm_ch0, pwm_ch1, pwm_ch2, pwm_ch3, pwm_ch4, pwm_ch6, pwm_ch7;
unsigned char rx_buf[7];

void write_int(unsigned int, unsigned char, unsigned char);


void intHand(void) __interrupt 0
{
    static unsigned char led_count;
    
    static char i;

    if (TMR0IE && TMR0IF) {

        TMR0IF = 0;
    }
}

void setup(void) {
    OSCCON=0b01110000; //8MHz

    //Ports
    TRISA=0b00000000;
    PORTA=0b00000000;
    TRISB=0b00110010;
    PORTB=0b00000000;

    CMCON=0b00000111; //Turn off comparator on RA port
    //shouldn't be needed due to POR defaults
    
    ANSEL=0;//This is needed! PORTA defaults to analog input!

    //Enable SPI slave mode
    SSPSTAT = 0b00000000; //datasheet is wrong. CKE must = 0
    SSPCON = 0b00110100;
    SSPIE = 0;

    GIE = 1; //Enable interrupts

    //Initialize Timer0 - used for LCD refresh rate and long-term timebase
    OPTION_REG = 0; // 1:32 prescaler, giving XLCD 4.1ms for Cmd cycles
    TMR0IE = 0;
    TMR0 = 0; //disable
}


void main(void) {
    unsigned char i;
    unsigned char pwm_count, pwm_reg;
    unsigned char led_count = 0;

    setup();
    
    pwm_count = 0;
    while (1) {
        //wait for next batch of data     
        if (pwm_count == 0) {
            SSPOV = 0;
            SSPIF = 0;
            for (i = 0; i < 7; i++) {
                while (!SSPIF)
                    nop();
                rx_buf[i] = SSPBUF;
                SSPIF = 0;
                
                WCOL = 0;//needed?
                
                pwm_ch0 = rx_buf[0];
                pwm_ch1 = rx_buf[1];
                pwm_ch2 = rx_buf[2]; //blue
                pwm_ch3 = rx_buf[3]; //green
                pwm_ch4 = rx_buf[4]; //red
                pwm_ch6 = rx_buf[5];
                pwm_ch7 = rx_buf[6];
            }

            if (led_count > 49) {
                LED_PIN = !LED_PIN;
                led_count = 0;
            } else {
                led_count++;
            }
        }
        
        pwm_reg = 0xff;
        if (pwm_count >= pwm_ch0)   //why doesn't > work but >= does??
        	clear_bit(pwm_reg,0);
        if (pwm_count >= pwm_ch1)
        	clear_bit(pwm_reg,1);
        if (pwm_count >= pwm_ch2)
        	clear_bit(pwm_reg,2);
        if (pwm_count >= pwm_ch3)
        	clear_bit(pwm_reg,3);
        if (pwm_count >= pwm_ch4)
        	clear_bit(pwm_reg,4);
        if (pwm_count >= pwm_ch6)
        	clear_bit(pwm_reg,6);
        if (pwm_count >= pwm_ch7)
        	clear_bit(pwm_reg,7);

        PORTA = pwm_reg;
        	
        pwm_count++;
    }
}
