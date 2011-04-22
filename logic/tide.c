/*
 Tide function for ez430 chronos watch.
 
 Copyright (C) 2011, Thomas Post <openchronos@post-net.ch>
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "project.h"

#include "stdio.h"

#include "tide.h"

#include "buzzer.h"
#include "clock.h"
#include "display.h"
#include "menu.h"
#include "ports.h"
#include "user.h"

typedef enum {
	TideDisplayState_Graph = 0,
	TideDisplayState_ToLowCounter,
	TideDisplayState_ToHighCounter,
	//editing mode
	TideDisplayState_Editing,
} TideDisplayState;

typedef enum {
	TideDisplayEditState_Hour = 0,
	TideDisplayEditState_Minute,
	TideDisplayEditState_Second,
} TideDisplayEditState;

u8 displayState = TideDisplayState_Graph;

struct tide aTide;

extern void menu_skip_next(line_t line); //ezchronos.c

u8 init = 0;

u8 graphOffset = 0;

u32 twentyFourHoursInSeconds = (u32)86400;
u16 fullTideTime = (u16)44700;
u16 halfTideTime = (u16)22350;

const u8 waveFormBits[] = {
							SEG_D							,
			SEG_B |					SEG_E |			SEG_G	,
	SEG_A													,
					SEG_C |					SEG_F | SEG_G	,	
};

void update_tide_timer() {
}

u32 timeInSeconds(struct tide theTide) {
	u32 result = theTide.secondsLeft;
	result += theTide.minutesLeft * 60;
	result += theTide.hoursLeft * (u16)3600;
	return result;
}

u32 timeNowInSeconds() {
	struct tide timeNow;
	timeNow.hoursLeft = sTime.hour;
	timeNow.minutesLeft = sTime.minute;
	timeNow.secondsLeft = sTime.second;
	return timeInSeconds(timeNow);
}

struct tide timeFromSeconds(u32 seconds) {
	struct tide newTide;
	newTide.hoursLeft = seconds/3600;
	u32 hoursInSeconds = newTide.hoursLeft*(u32)3600;
	seconds -= hoursInSeconds;
	
	newTide.minutesLeft = seconds/60;
	seconds -= newTide.minutesLeft*60;
	
	newTide.secondsLeft = seconds;
	
	return newTide;
}

void tide_tick() {
	if (init == 0) {
		aTide = timeFromSeconds(fullTideTime);
		init = 1;
	}
	u16 tideInSecondes =  aTide.secondsLeft + aTide.minutesLeft * 60 + aTide.hoursLeft * (u16)3600;
	graphOffset = (fullTideTime - tideInSecondes)/((u16)11175);
	
	if (aTide.secondsLeft == 0) {
		aTide.secondsLeft = 60;
		if (aTide.minutesLeft == 0) {
			aTide.minutesLeft = 60;
			if (aTide.hoursLeft == 0) {
				aTide = timeFromSeconds(fullTideTime+1);
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
		displayState = (displayState + 1) % 3;
	}
}

void mx_tide(u8 line) {
	
	//Clear display
	clear_display_all();
	
	TideDisplayEditState state = TideDisplayEditState_Hour;
	u8 displayMode = SETVALUE_ROLLOVER_VALUE | SETVALUE_DISPLAY_VALUE | SETVALUE_NEXT_VALUE;
	
	s32 secondsToLowTide = timeInSeconds(aTide);
	s32 nowInSeconds = timeNowInSeconds();
	
	s32 lowTideTime = (nowInSeconds + secondsToLowTide)  % twentyFourHoursInSeconds;
	
	struct tide lowTide = timeFromSeconds(lowTideTime); 
	s32 hours = lowTide.hoursLeft;
	s32 minutes = lowTide.minutesLeft;
	s32 seconds = lowTide.secondsLeft;
		
	u8 *strMinutes = itoa(minutes, 2, 1);
	display_chars(LCD_SEG_L2_3_2, strMinutes, SEG_ON);
		
	u8 *strSeconds = itoa(seconds, 2, 1);
	display_chars(LCD_SEG_L2_1_0, strSeconds, SEG_ON);
	
	display_symbol(LCD_SEG_L2_COL0, SEG_ON);
	
	display_chars(LCD_SEG_L1_3_2, (u8 *)"L ", SEG_ON);
	display_chars(LCD_SEG_L2_5_4, (u8 *)"  ", SEG_ON);
	
	//request to the user to enter the time of the next low tide
	while (1) {
		//Idle timeout: exit without saving
		if (sys.flag.idle_timeout) {
			break;
		}
		
		if (button.flag.star) {
			struct tide enteredTide;
			enteredTide.hoursLeft = hours;
			enteredTide.minutesLeft = minutes;
			enteredTide.secondsLeft = seconds;
			
			s32 nextLowSeconds = timeInSeconds(enteredTide);
			s32 nowInSeconds = timeNowInSeconds();
			
			s32 diff = nextLowSeconds - nowInSeconds;
			if (diff < 0) {
				diff = (twentyFourHoursInSeconds - nowInSeconds) + nextLowSeconds;
			}
			
			aTide = timeFromSeconds(diff);
			
			break;
		}
		
		switch (state) {
			case TideDisplayEditState_Hour:
			{
				set_value(&hours, 2, 0, 0, 23, displayMode, LCD_SEG_L1_1_0, display_value1);
				state = TideDisplayEditState_Minute;
				break;
			}
			case TideDisplayEditState_Minute:
			{
				set_value(&minutes, 2, 1, 0, 59, displayMode, LCD_SEG_L2_3_2, display_value1);
				state = TideDisplayEditState_Second;
				break;
			}
			case TideDisplayEditState_Second:
			{
				set_value(&seconds, 2, 1, 0, 59, displayMode, LCD_SEG_L2_1_0, display_value1);
				state = TideDisplayEditState_Hour;
				break;
			}
			default:
				break;
		}
		
	}
	
	// Clear button flag
	button.all_flags = 0;
	
	display_symbol(LCD_SEG_L2_COL1, SEG_OFF);
	display_symbol(LCD_SEG_L2_COL0, SEG_OFF);
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
			
			//reset line 1
			menu_L1[menu_L1_position]->display_function(LINE1, mode);
			break;
		}
		case TideDisplayState_ToHighCounter:
		{
			display_char(LCD_SEG_L2_4, 'H', SEG_ON);
			
			//calculate time left untli next high tide
			u16 leftUntilLow = timeInSeconds(aTide);
			u16 leftUntilHigh = halfTideTime; //22350 is seconds from low to gigh
			if (leftUntilLow > leftUntilHigh) {	
				leftUntilHigh = leftUntilLow - leftUntilHigh;
			} else {
				leftUntilHigh = leftUntilLow + leftUntilHigh;
			}
			struct tide highTide = timeFromSeconds(leftUntilHigh);

			//show the gighTide			
			display_chars(LCD_SEG_L2_3_2, itoa(highTide.hoursLeft, 2, 1), SEG_ON);
			display_chars(LCD_SEG_L2_1_0, itoa(highTide.minutesLeft, 2, 0), SEG_ON);
			display_symbol(LCD_SEG_L2_COL0, SEG_ON_BLINK_ON);
			
			//calculate the time of the next high tide
			u32 nowInSeconds = timeNowInSeconds();
			u32 highTideTime = (nowInSeconds + leftUntilHigh) % twentyFourHoursInSeconds;
			
			struct tide timeNow = timeFromSeconds(highTideTime);
			
			display_chars(LCD_SEG_L1_3_2, itoa(timeNow.hoursLeft, 2, 1), SEG_ON);
			display_chars(LCD_SEG_L1_1_0, itoa(timeNow.minutesLeft, 2, 0), SEG_ON);
						
			break;
		}
		case TideDisplayState_ToLowCounter:
		{
			display_char(LCD_SEG_L2_4, 'L', SEG_ON);
			
			
			display_chars(LCD_SEG_L2_3_2, itoa(aTide.hoursLeft, 2, 1), SEG_ON);
			display_chars(LCD_SEG_L2_1_0, itoa(aTide.minutesLeft, 2, 0), SEG_ON);
			display_symbol(LCD_SEG_L2_COL0, SEG_ON_BLINK_ON);
			
			//calculate the time of the next high tide
			u32 lowInSeconds = timeInSeconds(aTide);
			u32 nowInSeconds = timeNowInSeconds();
			u32 lowTideTime = (nowInSeconds + lowInSeconds) % twentyFourHoursInSeconds;
			
			struct tide timeNow = timeFromSeconds(lowTideTime);
			display_chars(LCD_SEG_L1_3_2, itoa(timeNow.hoursLeft, 2, 1), SEG_ON);
			display_chars(LCD_SEG_L1_1_0, itoa(timeNow.minutesLeft, 2, 0), SEG_ON);
			
			break;
		}
		default:
			break;
	}
}

u8 tide_display_needs_updating() {
	return 1;
}