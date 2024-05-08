#include <p33Fxxxx.h>
//do not change the order of the following 3 definitions
#define FCY 12800000UL 
#define PLAYER1 1 
#define PLAYER2 2 
#define N 3 // order of filter

#include <stdio.h>
#include <libpic30.h>
#include "lcd.h"
#include "servo.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "joystick.h"
#include "touchscreen.h"


/* Initial configuration by EE */
// Primary (XT, HS, EC) Oscillator with PLL
_FOSCSEL(FNOSC_PRIPLL);

// OSC2 Pin Function: OSC2 is Clock Output - Primary Oscillator Mode: XT Crystal
_FOSC(OSCIOFNC_OFF & POSCMD_XT);

// Watchdog Timer Enabled/disabled by user software
_FWDT(FWDTEN_OFF);

// Disable Code Protection
_FGS(GCP_OFF);


/*--------------Define Struct For Angle Use--------------*/
typedef struct {
    uint16_t x;
    uint16_t y;
} Pair;

uint16_t n = 10; // Number of unique points
uint16_t r = 2; // radius of ball
uint16_t time_out = 30; // sec

/*--------------Declare Parameters for PID--------------*/
Pair position = {0, 0};
Pair LCD_max = {105, 63};
Pair LCD_min = {0, 0};

//A10
Pair Board_max = {2892, 2530};
Pair Board_min = {245, 358};

volatile uint16_t counter = 0;
volatile uint8_t End_game = 0;

int flag_x = 0;
int flag_y = 0;

uint16_t player1_score = 0;
uint16_t player2_score = 0;


/*--------------Setting Filter--------------*/
double a[N+1] = {1.0, -0.577, 0.4217, -0.056};
double b[N+1] = {0.09853, 0.2955, 0.2955, 0.098};
double x1[N+1];//x
double y1[N+1];
double x2[N+1];//y
double y2[N+1];

/*--------------Pass the signal into digital filter--------------*/
double filter_update_x(uint16_t val) {
    double yo = 0.0;
    int i = 0;
    int j = 0;
    int k = 0;
    
    for (j = N; j > 0; j--) {
        x1[j] = x1[j-1];
        y1[j] = y1[j-1];
    }
    
    x1[0] = val;
    
    for (i = 0; i <= N; i++) {
        yo += b[i] * x1[i];
    }
    
    for (k = 1; k <= N; k++) {
        yo -= a[k] * y1[k];
    }
    
    y1[0] = yo;
    return yo;
}
/*--------------Pass the signal into digital filter--------------*/
double filter_update_y(uint16_t val) {
    double yo = 0.0;
    int i = 0;
    int j = 0;
    int k = 0;
    
    for (j = N; j > 0; j--) {
        x2[j] = x2[j-1];
        y2[j] = y2[j-1];
    }
    
    x2[0] = val;
    
    for (i = 0; i <= N; i++) {
        yo += b[i] * x2[i];
    }
    
    for (k = 1; k <= N; k++) {
        yo -= a[k] * y2[k];
    }
    
    y2[0] = yo;
    return yo;
}

uint16_t bound_x(double x){
    uint16_t new_x;
    if(x>Board_max.x){
        new_x = Board_max.x;
    } else if(x<Board_min.x){
        new_x = Board_min.x;
    } else{
        new_x = (uint16_t) x;
    }
    return new_x;
}

uint16_t bound_y(double y){
   uint16_t new_y;
    if(y>Board_max.y){
        new_y = Board_max.y;
    } else if(y<Board_min.y){
        new_y = Board_min.y;
    } else{
        new_y = (uint16_t) y;
    }
    return new_y;
}

void adjust_bound(){
    LCD_max.x -= r;
    LCD_max.y -= r;
    LCD_min.x += r;
    LCD_min.y += r;
}

/*--------------Timer1 Interrupt call function--------------*/
void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt (void)
{
    if(counter==(time_out*40)){
        End_game = 1;
    }
    // Read and Control x
    if (counter % 2 == 0){
        touch_pos_update_x();
        position.x = bound_x(filter_update_x(position.x));
        flag_x = 1;
        counter += 1;
        CLEARBIT(IFS0bits.T1IF);
    }
    // Read and Control y
    else {
        touch_pos_update_y();
        position.y = bound_y(filter_update_y(position.y));
        flag_y = 1;
        counter += 1;
        CLEARBIT(IFS0bits.T1IF);
    }
    
}

/*--------------Read and Update Position from touch screen--------------*/
void touch_pos_update_x() {
    touch_init();
    touch_select_dim(1);
    __delay_ms(15);
    position.x = touch_read();
}

/*--------------Read and Update Position from touch screen--------------*/
void touch_pos_update_y() {
    touch_init();
    touch_select_dim(2);
     __delay_ms(15);
    position.y = touch_read(); 
}

void setTimer1() {
    /*------------------- Setting Timer 1 ----------------------*/
    /* Initialize Timer in Normal mode, internal clock*/
    CLEARBIT(T1CONbits.TON);
    CLEARBIT(T1CONbits.TCS);
    CLEARBIT(T1CONbits.TGATE);   
    
    /* Reset timer counter*/ 
    TMR1 = 0x00;   
    
    /* Select 1:256 prescaler*/
    T1CONbits.TCKPS =0b11;  

    /* Select reload period*/
    PR1 = 1250;//40hz

    /* Select Interrupt Priority 1*/   
    IPC0bits.T1IP = 0x01;    

    /* Clear Pending Flag, Enable Interrupt 0*/    
    CLEARBIT(IFS0bits.T1IF);   
    SETBIT(IEC0bits.T1IE);

    
}
void startTimer(){
    /* Turn on TImer 1*/   
    SETBIT(T1CONbits.TON);
}
void stopTimer(){
    /* Turn on TImer 1*/   
    CLEARBIT(T1CONbits.TON);
}

int isUnique(const Pair *points, uint16_t n, Pair p) {
    uint16_t i;
    for (i = 0; i < n; i++) {
        if (points[i].x == p.x && points[i].y == p.y) {
            return 0; // Not unique
        }
    }
    return 1; // Unique
}

uint16_t random(uint16_t min, uint16_t max){
    return min + rand() % (max - min + 1);
}

void generateUniquePoints(uint16_t n, Pair *pairs) {

    srand(TMR1);
    
    uint16_t count = 0;
    while (count < n) {
        Pair newPoint;
        
        newPoint.x = random(LCD_min.x,LCD_max.x);
        newPoint.y = random(LCD_min.y,LCD_max.y);

        if (isUnique(pairs, count, newPoint)==1) {
            pairs[count] = newPoint;
            count+=1;
        }
    }
}

Pair mapping() {
    Pair LCD;
//    LCD.x = (uint16_t) ((position.x - Board_min.x) * ((double)(LCD_max.x - LCD_min.x)/ (double)(Board_max.y - Board_min.y)) + LCD_min.x);
//    LCD.y = (uint16_t) ((position.y - Board_min.y) * ((double)(LCD_max.y - LCD_min.y)/ (double)(Board_max.y - Board_min.y)) + LCD_min.y); 
    
    LCD.x = (uint16_t) ((Board_max.x - position.x) * ((double)(LCD_max.x - LCD_min.x)/ (double)(Board_max.y - Board_min.y)) + LCD_min.x);
    LCD.y = (uint16_t) ((Board_max.y - position.y) * ((double)(LCD_max.y - LCD_min.y)/ (double)(Board_max.y - Board_min.y)) + LCD_min.y); 
    return LCD;
}

void game(uint16_t player){
    End_game = 0;
    lcd_clear();
    Pair ball = {0, 0};
    Pair ball_prev = {0, 0};
    position = Board_min;

    uint16_t current_score = 0;
    Pair points[n]; // Array to hold the unique points
    lcd_clear();
    generateUniquePoints(n, points);
    int i;
    for (i=0; i<n; i++){
        drawBox(points[i].x, points[i].y,points[i].x,points[i].y,1);
    }
    counter = 0;
   
    while(current_score<n && End_game==0){
        ball = mapping();
        uint16_t bound_x_low = ball.x - r;
        uint16_t bound_x_high = ball.x + r;
        uint16_t bound_y_low = ball.y - r;
        uint16_t bound_y_high = ball.y + r;
  
        int j;
        for(j = 0; j<n; j++){
            if (points[j].x >= bound_x_low && points[j].y >= bound_y_low &&points[j].x <= bound_x_high && points[j].y <= bound_y_high){
                points[j].x = 200;
                points[j].y = 200;
                current_score+=1;
            }
        }
        drawBox(bound_x_low, bound_y_low, bound_x_high, bound_y_high, 1);

        ball_prev = ball;
        __delay_ms(2);
        eraseBlock(ball_prev.x-r, ball_prev.y-r, ball_prev.x+r, ball_prev.y+r); 
    }
    if(player==PLAYER1){
        player1_score = current_score;
    } else if(player==PLAYER2){
        player2_score = current_score;
    }
}

void press(){
    uint8_t bnt1_state_current = 0x0;
    uint8_t bnt1_state_previous = 0x0;
    uint8_t bnt1_state_count = 0;
    while(1) {
        bnt1_state_previous = bnt1_state_current;
        bnt1_state_current  = BTN1_PRESSED();
        //debouncing
        if(bnt1_state_current != bnt1_state_previous){
            if (bnt1_state_count > 8){
                if (bnt1_state_current == 1){   
                    break;
                }
            }
            bnt1_state_count = 0;
        }else{
            bnt1_state_count += 1;
        }
        __delay_ms(100);
    }
}
    
void start_game(){
    lcd_clear();
    lcd_locate(0, 0);
    lcd_printf("P1 Press to start.");   
    press();
    startTimer();
    game(PLAYER1);
    stopTimer();
    
    lcd_clear();
    lcd_locate(0, 0);
    lcd_printf("P2 Press to start.");  
    press();
    startTimer();
    game(PLAYER2);
    stopTimer();
    
    
    lcd_clear();
    lcd_locate(0, 0);
    lcd_printf("P1 Score : %d", player1_score);  
    lcd_locate(0, 1);
    lcd_printf("P2 Score : %d", player2_score);  
    lcd_locate(0, 2);
    if(player1_score>player2_score){
        lcd_printf("P1 Win!!");
    } else if(player1_score<player2_score){
        lcd_printf("P2 Win!!");
    } else{
        lcd_printf("Draw!!");
    }
    lcd_locate(0, 6);
    lcd_printf("Press to restart.");   
    press();
    player1_score = 0;
    player2_score = 0;
}
            
    

void main() {   
    __C30_UART = 1;
    lcd_initialize();
    lcd_clear();
    bot_initialize(); 
    SETBIT(TRISEbits.TRISE8);
    adjust_bound();
    setTimer1();
    touch_init();
    while (1) {
        start_game();
    }

}