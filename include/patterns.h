#ifndef PATTERNS_H
#define PATTERNS_H

#include <stdint.h>

#include "rgb_color.h"

#define PATTERNS_NR 8
#define LEDS_MIN 5
#define LEDS_MAX 1000 //max number of controlled RGB diodes

//flags for pattern functions
#define BRGH_ONLY 0x01

//parameters of pattern
typedef struct {
	int16_t dt;					//RGB refresh period in miliseconds
	uint16_t freq_max;			//max refresh frequency in miliseconds
	uint16_t freq_min;			//min dt in ms
	uint16_t speed;
	rgb_t color_1;				//
	int16_t param_1, param_2, param_3;
	uint8_t brightness;			//0 .. 100 [%]
	int32_t *on_diodes;          //nr of controller diodes
	int32_t old_diodes;
	rgb_t *rgb_color_buff;           //colors buffer
} pattParam_t;

//inputs: RGB buffer, parameters struct
typedef uint8_t (*pRefreshFun)(uint8_t);


//parameters for refreshRGB function
typedef struct {
	pRefreshFun *funTab;
	pattParam_t **paramTab;
	uint16_t runningPattern;
} refreshParam_t;

typedef struct {
	uint16_t dt;
	rgb_t color;
} PattData_t;

void initRgbPatterns(int32_t *diodes, uint8_t *rgb_buff, pattParam_t **paramTab,
						pRefreshFun *funTab);

void set_rgb_buff(pattParam_t **paramTab, uint8_t *rgb_buff);

#endif



















