/*
 * rgb_color.c
 *
 *  Created on: Aug 21, 2019
 *      Author: kz
 */
#include <stdio.h>
#include <stdint.h>

#include "rgb_color.h"

// ********************************************
void newColorBrgh(rgb_t *col, uint8_t brgh){
    uint16_t temp;

    temp = ((uint16_t)(col -> green) * (uint16_t)(brgh))/100;
    if (temp > 255){
        col -> green = 255;
    }
    else{
        col -> green = (uint8_t)temp;
    }

	temp = ((uint16_t)(col -> red) * (uint16_t)(brgh))/100;
    if (temp > 255){
        col -> red = 255;
    }
    else{
        col -> red = (uint8_t)temp;
    }

	temp = ((uint16_t)(col -> blue) * (uint16_t)(brgh))/100;
    if (temp > 255){
        col -> blue = 255;
    }
    else{
        col -> blue = (uint8_t)temp;
    }
}
