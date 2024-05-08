/* 
 * File:   touchscreen.h
 * Author: team-2b
 *
 * Created on April 5, 2024, 12:09 PM
 */

#ifndef TOUCHSCREEN_H
#define	TOUCHSCREEN_H

#include <p33Fxxxx.h>
#include "types.h"

void touch_init(void);
void touch_select_dim(uint8_t dimension);
uint16_t touch_read(void);

#endif	/* TOUCHSCREEN_H */


