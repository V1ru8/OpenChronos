/*
 *  tide.h
 *  OpenChronos
 *
 *  Created by Thomas Post on 26.11.10.
 *
 */

#ifndef TIDE_H_
#define TIDE_H_

extern void sx_tide(u8 line);
extern void mx_tide(u8 line);
extern void tide_tick();
extern void display_tide(u8 line, u8 mode);
extern u8 tide_display_needs_updating(void);

#define TIDE_ON				(1u)
#define TIDE_OFF			(2u)

struct tide {
	u8 hoursLeft;	//hours left to next low tide
	u8 minutesLeft; //minutes left to next low tide
	u8 secondsLeft; //seconds left to next low tide
};
extern struct tide aTide;

#endif //TIDE_H_