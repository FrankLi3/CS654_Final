#include "touchscreen.h"

void touch_init(void){
//    //disable ADC
//    CLEARBIT(AD1CON1bits.ADON);
//    
//    //initialize PIN for x axis
//    SETBIT(TRISBbits.TRISB4); //set TRISE RE8 to input
//    CLEARBIT(AD1PCFGLbits.PCFG9); //set AD1 AN20 input pin as analog
//    
//    // init pin for y axis
//    SETBIT(TRISBbits.TRISB5); //set TRISE RE8 to input
//    CLEARBIT(AD1PCFGLbits.PCFG15); //set AD1 AN20 input pin as analog

    //Configure AD1CON1
    SETBIT(AD1CON1bits.AD12B); //set 12b Operation Mode
    AD1CON1bits.FORM = 0; //set integer output
    AD1CON1bits.SSRC = 0x7; //set automatic conversion
    //Configure AD1CON2
    AD1CON2 = 0; //not using scanning sampling
    //Configure AD1CON3
    CLEARBIT(AD1CON3bits.ADRC); //internal clock source
    AD1CON3bits.SAMC = 0x1F; //sample-to-conversion clock = 31Tad
    AD1CON3bits.ADCS = 0x2; //Tad = 3Tcy (Time cycles)
    //Leave AD1CON4 at its default value
    //enable ADC
    SETBIT(AD1CON1bits.ADON);
}

void touch_select_dim(uint8_t dimension){
    //set up the I/O pins E1, E2, E3 to be output pins
    CLEARBIT(TRISEbits.TRISE1); //I/O pin set to output
    CLEARBIT(TRISEbits.TRISE2); //I/O pin set to output
    CLEARBIT(TRISEbits.TRISE3); //I/O pin set to output
    
    if (dimension == 1) {
        //set up the I/O pins E1, E2, E3 so that the touchscreen X-coordinate pin
        //connects to the ADC
        CLEARBIT(LATEbits.LATE1);
        SETBIT(LATEbits.LATE2);
        SETBIT(LATEbits.LATE3);
        AD1CHS0bits.CH0SA = 0x0F; 
    }
    else if(dimension == 2) {
        SETBIT(LATEbits.LATE1);
        CLEARBIT(LATEbits.LATE2);
        CLEARBIT(LATEbits.LATE3);
        AD1CHS0bits.CH0SA = 0x09; 
    }
    
}

uint16_t touch_read(void){
    SETBIT(AD1CON1bits.SAMP); //start to sample
    while(!AD1CON1bits.DONE); //wait for conversion to finish
    CLEARBIT(AD1CON1bits.DONE); //MUST HAVE! clear conversion done bit
    return ADC1BUF0; //return sample
}