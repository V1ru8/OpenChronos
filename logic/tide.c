/*
 *  tide.c
 *  OpenChronos
 *
 *  Created by Thomas Post on 26.11.10.
 *  Copyright 2010 equinux AG. All rights reserved.
 *
 */

#include "project.h"

#include "buzzer.h"
#include "display.h"
#include "clock.h"
#include "tide.h"

typedef enum {
	TideDisplayState_Graph = 0,
	TideDisplayState_ToLowCounter,
	TideDisplayState_ToHighCounter,
	//editing mode
	TideDisplayState_Editing,
} TideDisplayState;

TideDisplayState displayState = TideDisplayState_Graph;

struct tide aTide;

extern void menu_skip_next(line_t line); //ezchronos.c

u8 init = 0;

u8 graphOffset = 0;

const u8 waveFormBits[] = {
							SEG_D							,
			SEG_B |					SEG_E |			SEG_G	,
	SEG_A													,
					SEG_C |					SEG_F | SEG_G	,	
};

void update_tide_timer() {
}

u32 timeInSeconds(struct tide theTide) {
	return theTide.secondsLeft + theTide.minutesLeft * 60 + theTide.hoursLeft * 3600;
}

struct tide timeFromSeconds(u32 seconds) {
	struct tide newTide;
	newTide.hoursLeft = seconds/3600;
	seconds -= newTide.hoursLeft*3600;
	
	newTide.minutesLeft = seconds/60;
	seconds -= newTide.minutesLeft*60;
	
	newTide.secondsLeft = seconds;
	
	return newTide;
}

void tide_tick() {
	if (init == 0) {
		aTide.secondsLeft = 0;
		aTide.minutesLeft = 25;
		aTide.hoursLeft = 12;
		init = 1;
	}
	u32 tideInSecondes =  aTide.secondsLeft + aTide.minutesLeft * 60 + aTide.hoursLeft * 3600;
	graphOffset = (44700 - tideInSecondes)/11175;
	
	if (aTide.secondsLeft == 0) {
		aTide.secondsLeft = 60;
		if (aTide.minutesLeft == 0) {
			aTide.minutesLeft = 60;
			if (aTide.hoursLeft == 0) {
				aTide = timeFromSeconds(44701);
				return;
			}
			aTide.hoursLeft--;
		}
		aTide.minutesLeft--;
	}
	aTide.secondsLeft--;
}

void sx_tide(u8 line) {
	if (line == LINE2) {
		displayState = (++displayState) % 3;
	}
}

void mx_tide(u8 line) {
	//start_buzzer(2, BUZZER_ON_TICKS, BUZZER_OFF_TICKS);
}

void display_tide(u8 line, u8 mode) {
	switch (displayState) {
		case TideDisplayState_Graph:
		{
			u8 offset = graphOffset;
			
			u8 i;
			for (i = 0; i < 5; i++) {
				u8 bits = waveFormBits[offset];
				u8 mask;
				u8 *mem;
				
				switch (i) {
					case 0:
					{
						mask = LCD_SEG_L2_4_MASK;
						mem = (u8*)LCD_SEG_L2_4_MEM;
						break;
					}
					case 1:
					{
						mask = LCD_SEG_L2_3_MASK;
						mem = (u8*)LCD_SEG_L2_3_MEM;
						break;
					}
					case 2:
					{
						mask = LCD_SEG_L2_2_MASK;
						mem = (u8*)LCD_SEG_L2_2_MEM;
						break;
					}
					case 3:
					{
						mask = LCD_SEG_L2_1_MASK;
						mem = (u8*)LCD_SEG_L2_1_MEM;
						break;
					}
					case 4:
					{
						mask = LCD_SEG_L2_0_MASK;
						mem = (u8*)LCD_SEG_L2_0_MEM;
						break;
					}
					default:
						break;
				}
				// When addressing LINE2 7-segment characters need to swap high- and low-nibble,
				// because LCD COM/SEG assignment is mirrored against LINE1
				bits = ((bits << 4) & 0xF0) | ((bits >> 4) & 0x0F);
				
				write_lcd_mem(mem, bits, mask, SEG_ON);
				offset = (offset+1) % 4;
				
			}
			//marker for 'now'
			display_symbol(LCD_SEG_L2_COL1, SEG_ON);
			break;
		}
		case TideDisplayState_ToHighCounter:
		{
			display_char(LCD_SEG_L2_4, 'H', SEG_ON);
			
			//calculate time left untli next high tide
			u32 leftUntilLow = timeInSeconds(aTide);
			u32 leftUntilHigh = 22350; //22350 is seconds from low to gigh
			if (leftUntilLow > leftUntilHigh) {	
				leftUntilHigh = leftUntilLow - leftUntilHigh;
			} else {
				leftUntilHigh = leftUntilLow + leftUntilHigh;
			}
			struct tide highTide = timeFromSeconds(leftUntilHigh);

			//show the gighTide			
			display_chars(LCD_SEG_L2_3, itoa(highTide.hoursLeft, 2, 0), SEG_ON);
			display_chars(LCD_SEG_L2_2, itoa(highTide.hoursLeft, 1, 0), SEG_ON);
			display_chars(LCD_SEG_L2_1, itoa(highTide.minutesLeft, 2, 0), SEG_ON);
			display_chars(LCD_SEG_L2_0, itoa(highTide.minutesLeft, 1, 0), SEG_ON);
			display_symbol(LCD_SEG_L2_COL0, SEG_ON_BLINK_ON);
			break;
		}
		case TideDisplayState_ToLowCounter:
		{
			display_char(LCD_SEG_L2_4, 'L', SEG_ON);
			
			
			display_chars(LCD_SEG_L2_3, itoa(aTide.hoursLeft, 2, 0), SEG_ON);
			display_chars(LCD_SEG_L2_2, itoa(aTide.hoursLeft, 1, 0), SEG_ON);
			display_chars(LCD_SEG_L2_1, itoa(aTide.minutesLeft, 2, 0), SEG_ON);
			display_chars(LCD_SEG_L2_0, itoa(aTide.minutesLeft, 1, 0), SEG_ON);
			display_symbol(LCD_SEG_L2_COL0, SEG_ON_BLINK_ON);
			break;
		}
		default:
			break;
	}
}

u8 tide_display_needs_updating() {
	return 1;
}