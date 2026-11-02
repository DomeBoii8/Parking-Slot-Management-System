/* 
=================================================================
Project Title: Parking Slot Counter System

*****************************************************************
Description: A simple parking slot counter system that
utilizes the onboard peripherals (LEDs, LCD, Matrix 
Keyboard, etc.) for simulation. 8 built-in LEDs represent 
8 parking slots. States ON/OFF represent the occupancy 
status of the slot for the drivers. LCD allows the parking 
control room operator to view, and modify the states of the 
parking slots via the keyboard interface. It displays 4 
menus/modes. Following are their in-depth description

a) Mode A (Enabling/Disabling a parking slot):
---------------
1 2 3 4 5 6 7 8
A I A I A A I A
---------------
Mode A allows the user to enable/disable a parking slot. 
States A and I represent Active and Inactive respectively.
By default all modes are set to A; user can cycle through
A-I-A-I... so on via the keyboard keys mapped 1-8. If a
slot is A, it can be modified in mode C for occupancy 
status, while changes will be reflected accordingly in 
mode B and D respectively.

b) Mode B (Display count of vacant and occupied slots):
-------------------------------------------
Vacant   : <>/<no. of active parking slots>
Occupied : <>/<no. of active parking slots>
-------------------------------------------
Mode B displays the count of vacant and occupied parking 
slots to the user with respect to the active parking slots.
The placeholders represent the value that will be displayed
on the LCD. The user cannot modify anything in this display
mode, therefore keys 1-8 of the keyboard will be disabled.

c) Mode C (Occupancy Status):
---------------
1 2 3 4 5 6 7 8
V X O X R V X O
---------------
Mode C allows the user to cycle through the occupancy 
status of the parking slots. If the parking slot is active 
it can cycle through V-O-R-V-O-R-.... representing Vacant,
Occupied, and Reserved respectively, and can be chosen from
the keyboard mapping similar to mode A. If a slot is inactive 
it will display X indicating the slot is inactive and cannot
participate in this display mode for cycling through the 
options; respective keys on the keyboard will be disabled. 
By default all modes will be V for active slots, and X for 
inactive slots. Changes made here will be respectively 
displayed on the other modes.

d) Mode D (Summary of Mode A and C combined):
---------------
A I A I A A I A
V X O X R V X O
---------------
Mode D summarizes mode A and C together. It is a read-only 
mode and respective keys on the keyboard will be disabled.
The first line summarizes the A/I status of the slot, while
the second line summarizes the V/O/R/X status of the slots.

With the above modes set in place, following is the keyboard
mapping that will be followed for the application:

A B C D
1 2 3 4
5 6 7 8
# ^ v *

where the first row keys allow the user to choose a display
mode, and will be active all the times. Second and third row
of keys will be active only for modes A and C for cycling 
through the changes while being disabled for the remaining 
modes. Fourth row: '^' (Up) and 'v' (Down) scroll the lines of Mode B
and are disabled in all other modes. First key '#' is for saving configuration that will
be enabled when any modification has been made in modes A and C.
'*' represents 'Go Back to Prev Page' acting like a previous page
similar to websites, supporting a memory of upto 5 pages for now.

******************************************************************
Connections:
CNA - CNA1 (pins 1-8 : 0.4-0.11 : LEDs 1-8)
CNB - CNB3 (pins 1-8 : 1.23-1.26 and 2.10-2.13 : Keyboard)
CNC - Left Empty
CND - CNAD (pins )
LCD (onboard) : P0.23-0.26 = D4-D7, P0.27 = RS, P0.28 = EN
Blink output  : P1.28 = MAT0.0 (Timer0, toggles every 1 s while any
                saved slot is Reserved)

******************************************************************
Version: 0.2 (Reserve count, Timer0 MAT0.0 blink, Mode B scrolling)

Created at: 17/10/2026
By: Aditya Prashanth, Chetana Umesh, Sana Sinha
==================================================================
*/


#include<LPC17xx.h>
#include<stdio.h>

#define SLOT_COUNT      8
#define HISTORY_SIZE    5
#define DEBOUNCE_DELAY  300000

/* ---------- Pin masks on PORT 0 ----------
   LEDs     : P0.4  - P0.11  (8 bits)
   LCD data : P0.23 - P0.26  (4 bits, D4-D7)
   LCD RS   : P0.27
   LCD EN   : P0.28
   Every write to PORT 0 goes through these masks so that
   touching the LCD never disturbs the LEDs and vice versa. */
#define LED_MASK        (0xFFu << 4)
#define LCD_DATA_MASK   (0xFu  << 23)
#define LCD_RS          (1u    << 27)
#define LCD_EN          (1u    << 28)

/* ---------- Timer0 blink (MAT0.0 = P1.28, PINSEL3 bits 25:24 = 11) ----------
   PCLK_HZ must match the peripheral clock feeding Timer0
   (the lecture example assumes 3 MHz). Delay = (MR0+1)(PR+1)/PCLK, so
   PR = 0 and MR0 = PCLK_HZ - 1 give 1 s per tick,
   i.e. 1 s ON / 1 s OFF. */
#define PCLK_HZ         3000000UL
#define MAT00_PIN       (1u << 28)
#define MAT00_PINSEL    (3u << 24)

char active[SLOT_COUNT];
char status[SLOT_COUNT];

char saved_active[SLOT_COUNT];
char saved_status[SLOT_COUNT];

char msg[17]      = "1 2 3 4 5 6 7 8";
char enable[17]   = "A A A A A A A A";
char occ_stat[17] = "V V V V V V V V";

int modified    = 0;     // unsaved changes exist in modes A / C
int needRefresh = 1;     // LCD + LEDs must be redrawn

char currentMode = 'A';
char history[HISTORY_SIZE];   // page memory for '*' (Go Back)
int  histCount = 0;

int  modeBTop      = 0;   // first visible line of Mode B (0 or 1)
int  blinkRunning  = 0;   // Timer0 blink active?
unsigned int blinkState = 0;   // mirror of EM0 (MAT0.0)

unsigned int j, k, temp1, temp2, flag, flag1, flag2, row, col;
unsigned int init_cmd[] = {0x30, 0x30, 0x30, 0x20, 0x28, 0x0C, 0x06, 0x01, 0x80};
unsigned char keys[16] = {
	'A', 'B', 'C', 'D', 
	'1', '2', '3', '4', 
	'5', '6', '7', '8',	
	'#', '^', 'v', '*'      // '^' = Up, 'v' = Down (Mode B only)
};

int activeCount   = 0;
int vacantCount   = 0;
int occupiedCount = 0;
int reserveCount  = 0;

/* ================= Low level helpers ================= */

void delay(unsigned int count){
	volatile unsigned int i;
	for(i=0; i<count; i++);
}

/* Drives the 4 LCD data lines + RS + pulses EN.
   Only P0.23-P0.28 are touched; LEDs (P0.4-P0.11) are untouched. */
void port_write(void){
	// set the data bits that are 1, clear the data bits that are 0 (masked)
	LPC_GPIO0->FIOSET = temp2 & LCD_DATA_MASK;
	LPC_GPIO0->FIOCLR = (~temp2) & LCD_DATA_MASK;

	if(flag1)
		LPC_GPIO0->FIOSET = LCD_RS;
	else
		LPC_GPIO0->FIOCLR = LCD_RS;

	LPC_GPIO0->FIOSET = LCD_EN;
	delay(25);
	LPC_GPIO0->FIOCLR = LCD_EN;
	delay(30000);
}

void lcd_write(void){
	flag2 = (flag1 == 1)?0:((temp1==0x30) || (temp1 == 0x20))?1:0;
	temp2 = (temp1 & 0xF0) << 19;
	port_write();
	if(flag2 == 0){
		temp2 = (temp1 & 0xF) << 23;
		port_write();
	}
}

void init_lcd(void){
	flag1=0;
	for(j=0; j<9; j++){
		temp1 = init_cmd[j];
		lcd_write();
	}
	flag1=1;
}

void display_msg(unsigned char msg[]){
	j=0;
	while(msg[j] != '\0'){
		temp1 = msg[j++];
		lcd_write();
	}
}

void display_char(unsigned char ch){
	temp1 = ch + 0x30;
	lcd_write();
}

void lcd_command(unsigned char cmd){
	flag1=0;
	temp1 = cmd;
	lcd_write();
	flag1=1;
}

/* ================= Keypad ================= */

void scan(void){
	unsigned int temp3;
	temp3 = LPC_GPIO1->FIOPIN & (0xF << 23);
	temp3 = (temp3 >> 23)&0xF;
	if(temp3){
		flag = 1;
		if(temp3 == 0x1)
			col=0;
		else if (temp3 == 0x2)
			col=1;
		else if (temp3 == 0x4)
			col=2;
		else if (temp3 == 0x8)
			col=3;
	}
}

int keyboard_polling(void){
	for(k=0; k<4; k++){
		LPC_GPIO2->FIOCLR = 0xF << 10;
		LPC_GPIO2->FIOSET = 1 << (10+k);
		flag = 0;
		delay(500);
		scan();
		if(flag == 1){
			row = k;
			return 4*row + col;
		}
	}
	return -1;
}

/* ================= Slot logic ================= */

// MODE A
void toggle_active(int i) {
    if (active[i] == 'A') {
        active[i] = 'I';
        status[i] = 'X';
    } else {
        active[i] = 'A';
        status[i] = 'V';
    }

    modified = 1;
}

// MODE C
void cycle_status(int i) {
    if (active[i] != 'A')
        return;

    if (status[i] == 'V')
        status[i] = 'O';
    else if (status[i] == 'O')
        status[i] = 'R';
    else
        status[i] = 'V';

    modified = 1;
}

// Recompute the counters shown in Mode B from the SAVED configuration only
void recompute_counts(void) {
    int i;
    activeCount = vacantCount = occupiedCount = reserveCount = 0;

    for (i = 0; i < SLOT_COUNT; i++) {
        if (saved_active[i] == 'A') {
            activeCount++;
            if (saved_status[i] == 'V')
                vacantCount++;
            else if (saved_status[i] == 'O')
                occupiedCount++;
            else if (saved_status[i] == 'R')
                reserveCount++;
        }
    }
}

// '#' : commit the working copy
void save_config(void) {
    int i;
    for (i = 0; i < SLOT_COUNT; i++) {
        saved_active[i] = active[i];
        saved_status[i] = status[i];
    }
    modified    = 0;
    needRefresh = 1;
}

/* ================= Page navigation ================= */

void change_mode(char newMode) {
    int i;
    if (newMode == currentMode)
        return;

    // history full -> forget the oldest page
    if (histCount == HISTORY_SIZE) {
        for (i = 0; i < HISTORY_SIZE - 1; i++)
            history[i] = history[i + 1];
        histCount--;
    }

    history[histCount++] = currentMode;
    currentMode  = newMode;
    if (currentMode == 'B')
        modeBTop = 0;
    needRefresh  = 1;
}

// '*' : go back to previous page
void go_back(void) {
    if (histCount == 0)
        return;

    currentMode = history[--histCount];
    if (currentMode == 'B')
        modeBTop = 0;
    needRefresh = 1;
}

/* ================= Outputs: LEDs ================= */

/* LED i (saved configuration = what drivers see):
     Occupied -> steady ON
     Reserved -> follows the Timer0 blink state (1 s ON / 1 s OFF)
     Vacant / Inactive -> OFF */
int slot_led_on(int i) {
    if (saved_active[i] != 'A')
        return 0;
    if (saved_status[i] == 'O')
        return 1;
    if (saved_status[i] == 'R')
        return blinkState;
    return 0;
}

void update_leds(void) {
    unsigned int pattern = 0;
    int i;

    for (i = 0; i < SLOT_COUNT; i++) {
        if (slot_led_on(i))
            pattern |= (1u << i);
    }

    pattern = (pattern << 4) & LED_MASK;      // slot 0 -> P0.4 ... slot 7 -> P0.11

    // masked update: only P0.4-P0.11 are touched, and no all-off glitch
    LPC_GPIO0->FIOSET = pattern;
    LPC_GPIO0->FIOCLR = (~pattern) & LED_MASK;
}

// TIMER 0 configuration
// PR = 999
// MR0 = 2999

void blink_timer_start(void) {
    LPC_TIM0->TCR = 0x02;                  // Timer0 reset
    LPC_TIM0->EMR = 0x20;                  // set EM0 upon match (also clears it now)
    LPC_TIM0->PR  = 999;
    LPC_TIM0->MR0 = 2999;             // 1 s
    LPC_TIM0->MCR = 0x04;                  // stop PC and TC on MR0
    LPC_TIM0->TCR = 0x01;                  // Timer0 enable
}
 
void blink_timer_stop(void) {
    LPC_TIM0->TCR = 0x00;
    blinkState    = 0;
    blinkRunning  = 0;
}
 
// Run the blink only while at least one SAVED slot is Reserved
void update_blink_timer(void) {
    int need = (reserveCount > 0);   // from saved config (recompute_counts runs first)
 
    if (need && !blinkRunning) {
        blinkState   = 0;
        blinkRunning = 1;
        blink_timer_start();
    }
    else if (!need && blinkRunning) {
        blink_timer_stop();
    }
}
 
// Poll once per main-loop pass (also while waiting for key release)
void service_blink(void) {
    if (!blinkRunning)
        return;
 
    if (LPC_TIM0->EMR & 0x01) {            // match happened -> 1 s elapsed
        blinkState ^= 1;
        update_leds();                     // reserved slot LEDs ON/OFF
        blink_timer_start();               // next second
    }
}

/* ================= Outputs: LCD ================= */

// turn "ABCDEFGH" into "A B C D E F G H" (15 chars + '\0')
void build_row(char *dst, char *src) {
    int i;
    for (i = 0; i < SLOT_COUNT; i++) {
        dst[2 * i] = src[i];
        if (i < SLOT_COUNT - 1)
            dst[2 * i + 1] = ' ';
    }
    dst[2 * SLOT_COUNT - 1] = '\0';
}

void show_lines(char *line1, char *line2) {
    lcd_command(0x01);                    // clear display
    lcd_command(0x80);                    // line 1, col 0
    display_msg((unsigned char *)line1);
    lcd_command(0xC0);                    // line 2, col 0
    display_msg((unsigned char *)line2);
}

void display_mode(void) {
    char vac[17], occ[17], res[17];
    char *lines[3];

    switch (currentMode) {
        case 'A':
            build_row(enable, active);
            show_lines(msg, enable);
            break;

        case 'B':
            recompute_counts();
            sprintf(vac, "Vacant   : %d/%d", vacantCount,   activeCount);
            sprintf(occ, "Occupied : %d/%d", occupiedCount, activeCount);
            sprintf(res, "Reserved : %d/%d", reserveCount,   activeCount);

            // 3 lines, 2-line window starting at modeBTop
            lines[0] = vac;  lines[1] = occ;  lines[2] = res;
            show_lines(lines[modeBTop], lines[modeBTop + 1]);
            break;

        case 'C':
            build_row(occ_stat, status);
            show_lines(msg, occ_stat);
            break;

        case 'D':
            build_row(enable,   active);
            build_row(occ_stat, status);
            show_lines(enable, occ_stat);
            break;
    }
}

/* ================= Key handling ================= */

void process_key(int idx) {
    char key = keys[idx];
    int slot;

    if (idx >= 0 && idx <= 3) {                      // A B C D : mode select
        change_mode(key);
    }
    else if (idx >= 4 && idx <= 11) {                // 1-8 : slot keys
        slot = idx - 4;

        if (currentMode == 'A') {
            toggle_active(slot);
            needRefresh = 1;
        }
        else if (currentMode == 'C') {
            if (active[slot] == 'A') {               // inactive slots: key disabled
                cycle_status(slot);
                needRefresh = 1;
            }
        }
        // modes B and D: keys 1-8 disabled
    }
    else if (idx == 13) {                            // '^' Up   (Mode B only)
        if (currentMode == 'B' && modeBTop > 0) {
            modeBTop--;
            needRefresh = 1;
        }
    }
    else if (idx == 14) {                            // 'v' Down (Mode B only)
        if (currentMode == 'B' && modeBTop < 1) {
            modeBTop++;
            needRefresh = 1;
        }
    }
    else if (key == '#') {                           // save (only if modified)
        if (modified)
            save_config();
    }
    else if (key == '*') {                           // go back
        go_back();
    }
}

/* ================= Main ================= */

int main(){
	int key;

	SystemInit();
	SystemCoreClockUpdate();
	
	LPC_PINCON->PINSEL0 = 0; // 0.4-0.11 in gpio mode [LEDs]
	LPC_PINCON->PINSEL1 = 0; // 0.23-0.28 in gpio mode [LCD]
	LPC_PINCON->PINSEL3 = 0; // 1.23-1.26 in gpio mode [Columns]
	LPC_PINCON->PINSEL4 = 0; // 2.10-2.13 in gpio mode [Rows]
	
	LPC_GPIO0->FIODIR |= LED_MASK | LCD_DATA_MASK | LCD_RS | LCD_EN; // [OUTPUTS for LEDS and LCD]
	LPC_GPIO1->FIODIR &= ~(0xFu << 23);                              // [INPUTS for Columns]
	LPC_GPIO2->FIODIR |= (0xFu << 10);                               // [OUTPUTS for Rows]
	LPC_GPIO1->FIODIR |= MAT00_PIN;                                  // P1.28 (MAT0.0 / blink pin) output
	
	// Step 1 : Initialization - all slots Active + Vacant
	for (j = 0; j < SLOT_COUNT; j++) {
		active[j] = 'A';
		status[j] = 'V';

		saved_active[j] = 'A';
		saved_status[j] = 'V';
	}
	
	init_lcd();
	recompute_counts();
	
	while(1){
		// Step 2 / 5 : refresh outputs only when something changed
		if (needRefresh) {
			recompute_counts();
			update_blink_timer();
			update_leds();
			display_mode();
			needRefresh = 0;
		}

		service_blink();                              // keep reserved LEDs blinking

		// Step 3 : scan keypad
		key = keyboard_polling();

		if (key != -1) {
			delay(DEBOUNCE_DELAY);                    // debounce
			if (keyboard_polling() == key) {          // still the same key?
				// Step 4 : process key
				process_key(key);

				while (keyboard_polling() != -1)      // wait for release
					service_blink();                  // ...but keep blinking
				delay(DEBOUNCE_DELAY);
			}
		}
	}
}