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
2011-11-05
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

#define INTR_PIN RB3
#define SYNC_PIN RA2
#define SYNC_TRIS TRISA2
#define TX_BUFF_SIZE 9

#define nop() \
        _asm\
        nop\
        _endasm


unsigned char Converting, Sending;

unsigned char tx_buffer[TX_BUFF_SIZE];
unsigned char tx_buffer_copy[TX_BUFF_SIZE];


void WriteByte(char, char, char);
void write_int(unsigned int, unsigned char, unsigned char);


void intHand(void) __interrupt 0
{
    static unsigned char led_count;

    if (TMR0IE && TMR0IF) {
		if (Converting == 0) {
	        Sending = 1;
    	    Converting = 1;
		}
        if (led_count >= 30) { // 32ms * 30 ~ 1s
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
    TRISB=0b00101100;
    PORTB=0b00000000;

    CMCON=0b00000111; //Turn off comparator on RA port
    //shouldn't be needed due to POR defaults
    
    ANSEL=0;//This is needed! PORTA defaults to analog input!


    GIE = 1; //Enable interrupts

    //Initialize Timer0 - used for LCD refresh rate and long-term timebase
    OPTION_REG = 0b111; // 1:256 prescaler -> 32.8 ms
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
    char send_state, send_index;
    char conv_state;
    char Address, Command, Data;
    char timing_reg, gain_reg, gain_mode;
    unsigned int red_reg, grn_reg, blu_reg;
    unsigned int greatest_val;
    
    setup();

    //turn on
    Address = 0x39; // slave address
    Command = 0x00|0x80; // Control register
    Data = 0b00000011; // power on, enable ADC
    WriteByte(Address, Command, Data);
    
    //set timing
    Command = 0x01|0x80; // Timing register
    Data = 0b00000010; // free-run at 400 ms
    WriteByte(Address, Command, Data);
    
    //set gain
    Command = 0x07|0x80; // Gain register
    Data = 0b00100000; // 16X, no pre-scaler
    WriteByte(Address, Command, Data);

    tx_buffer[0] = 'n';
    tx_buffer[1] = 'y';
/*    tx_buffer[2] = 123;
    tx_buffer[3] = 64;
    tx_buffer[4] = 231;
    tx_buffer[5] = 1022;*/
    
    send_state = 0;
    send_index = 0;
    conv_state = 0;
    Converting = 0;
    
    while (1) {
		if (Sending == 1) { // sending in progress
			switch (send_state) {
				case 0: //start
					//copy buffer
					for (send_index = 0; send_index < TX_BUFF_SIZE; send_index++) {
						tx_buffer_copy[send_index] = tx_buffer[send_index];
					}
					send_index = 0;
					send_state = 1;
				case 1: //send buffer, 1 byte at a time
					if (TXIF) { //ready for next byte
						TXREG = tx_buffer_copy[send_index];
						send_index++;
						if (send_index >= TX_BUFF_SIZE) { // we are done
							send_state = 0;
							Sending = 0;
						}
					}
			}
		} else {
			if (TXIF) { //send 'n' as a filler
				TXREG = 'n';
			}
		}

		if (Converting == 1) { // conversion in progress
			switch (conv_state) {
				case 0: //start conv1
				    //mode 0 settings:
            	    timing_reg = 0b00100000; // one-shot at 12ms
            	    gain_reg = 0b00010000; // 4X, no pre-scaler

				    //set ADC_EN
				    Command = 0x00|0x80; // Control register
				    Data = 0b00000011; // enable ADC
				    WriteByte(Address, Command, Data);
				    
				    //set timing
				    Command = 0x01|0x80; // Timing register
				    Data = timing_reg;
				    WriteByte(Address, Command, Data);
				    
				    //set gain
				    Command = 0x07|0x80; // Gain register
				    Data = gain_reg;
				    WriteByte(Address, Command, Data);

				    //set interrupt control
				    Command = 0x02|0x80; // Interrupt Control register
				    Data = 0b00010000; // 16X, no pre-scaler
				    WriteByte(Address, Command, Data);
		    
				    SYNC_TRIS = 1; // let SYNC_PIN go high
				    
				    for (i = 0; i <= 100; i++) //delay
				    	nop();
				    
				    SYNC_TRIS = 0;
				    
				    conv_state = 1;
				    break;
			    case 1: //get results, choose start conv2
			    	//test INTR_PIN
			    	if (INTR_PIN) {
			    		break;
			    	}
			    	
			    	//clear INTR (SendByte format)
				    i2c_start();
				    i2c_tx(0b01110010); // Address + write (0)
				    i2c_tx(0b11100000);
				    i2c_stop();
				    			    	
			    	//clear ADC_EN
				    Command = 0x00|0x80; // Control register
				    Data = 0b00000001; // disable ADC
				    WriteByte(Address, Command, Data);
				    
			    	//read data
		            i2c_start();
		            i2c_tx(0b01110010); // Address + write (0)
		            i2c_tx(0x10|0x80);  // DATA1LOW register
		            i2c_start();
		            i2c_tx(0b01110011); // Address + read (1)
		            
                    grn_reg = i2c_rx(1);
                    grn_reg += 256 * i2c_rx(1);
                    red_reg = i2c_rx(1);
                    red_reg += 256 * i2c_rx(1);
                    blu_reg = i2c_rx(1);
                    blu_reg += 256 * i2c_rx(0);
                    i2c_stop();
                    
                    //find greatest value
                    greatest_val = red_reg;
                    if (grn_reg > greatest_val)
                        greatest_val = grn_reg;
                    if (blu_reg > greatest_val)
                        greatest_val = blu_reg;
                    
                    //calc conv2 gain_mode based on greatest value
		            gain_mode = 1;
		            if (greatest_val < 3276)
                        gain_mode = 2;
                    if (greatest_val < 395)
                        gain_mode = 3;
                    if (greatest_val < 98)
                        gain_mode = 4;
		            
		            //set gain mode
                    switch (gain_mode) {
                        case 1:
                    	    timing_reg = 0b00100000; // one-shot at 12ms
                    	    gain_reg = 0b00010000; // 4X, no pre-scaler
                            break;
                        case 2:
                    	    timing_reg = 0b00100000; // one-shot at 12ms
                    	    gain_reg = 0b00110000; // 64X, no pre-scaler
                            break;
                        case 3:
                    	    timing_reg = 0b00100001; // one-shot at 100ms
                    	    gain_reg = 0b00110000; // 64X, no pre-scaler
                            break;
                        case 4:
                    	    timing_reg = 0b00100010; // one-shot at 400ms
                    	    gain_reg = 0b00110000; // 64X, no pre-scaler
                            break;
                    }
                    
                    //start conv2
				    //set ADC_EN
				    Command = 0x00|0x80; // Control register
				    Data = 0b00000011; // enable ADC
				    WriteByte(Address, Command, Data);
				    
				    //set timing
				    Command = 0x01|0x80; // Timing register
				    Data = timing_reg;
				    WriteByte(Address, Command, Data);
				    
				    //set gain
				    Command = 0x07|0x80; // Gain register
				    Data = gain_reg;
				    WriteByte(Address, Command, Data);

				    //set interrupt control
				    Command = 0x02|0x80; // Interrupt Control register
				    Data = 0b00010000; // 16X, no pre-scaler
				    WriteByte(Address, Command, Data);
		    
				    SYNC_TRIS = 1; // let SYNC_PIN go high
				    
				    for (i = 0; i <= 100; i++) //delay
				    	nop();
				    
				    SYNC_TRIS = 0;
				    
		            conv_state = 2;
		            break;
			    case 2: //write result to buffer
			    	//test INTR_PIN
			    	if (INTR_PIN) {
			    		break;
			    	}
			    	
			    	//clear INTR (SendByte format)
				    i2c_start();
				    i2c_tx(0b01110010); // Address + write (0)
				    i2c_tx(0b11100000);
				    i2c_stop();
				    			    	
			    	//clear ADC_EN
				    Command = 0x00|0x80; // Control register
				    Data = 0b00000001; // disable ADC
				    WriteByte(Address, Command, Data);
				    
			    	//read data to buffer
		            i2c_start();
		            i2c_tx(0b01110010); // Address + write (0)
		            i2c_tx(0x10|0x80);  // DATA1LOW register
		            i2c_start();
		            i2c_tx(0b01110011); // Address + read (1)
		            
		            //gain_mode
		            // 0 and 1 are for 'n' and 'y' start string
		            tx_buffer[2] = gain_mode;
		            //green
		            tx_buffer[5] = i2c_rx(1);
		            tx_buffer[6] = i2c_rx(1);
		            //red
		            tx_buffer[3] = i2c_rx(1);
		            tx_buffer[4] = i2c_rx(1);
		            //blue
		            tx_buffer[7] = i2c_rx(1);
		            tx_buffer[8] = i2c_rx(0);
		            i2c_stop();
		            
		            conv_state = 0;
		            Converting = 0;
			}
		}
    }
}


void WriteByte(char Address, char Command, char Data)
{
    i2c_start();
    i2c_tx((Address << 1) + 0); // 0 = write
    i2c_tx(Command);
    i2c_tx(Data);
    i2c_stop();
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
    tx_buffer[col] = '0' + digit;
    col++;
    
four_digits:
    digit = num / 1000;
    s = digit * 1000;
    num = num - s;
    tx_buffer[col] = '0' + digit;
    col++;

three_digits:
    digit = num / 100;
    s = 100 * digit;
    num = num - s;
    tx_buffer[col] = '0' + digit;
    col++;

two_digits:
    digit = num / 10;
    s = digit * 10;
    num = num - s;
    tx_buffer[col] = '0' + digit;
    col++;

one_digit:
    tx_buffer[col] = '0' + num;
}
