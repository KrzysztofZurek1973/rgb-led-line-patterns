/*
 * rgb_color.h
 *
 *  Created on: Aug 21, 2019
 *      Author: kz
 */

#ifndef RGB_COLOR_H_
#define RGB_COLOR_H_

//#include <stdio.h>

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} rgb_t;

void newColorBrgh(rgb_t *col, uint8_t brgh);

#endif /* RGB_COLOR_H_ */
