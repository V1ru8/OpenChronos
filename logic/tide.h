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