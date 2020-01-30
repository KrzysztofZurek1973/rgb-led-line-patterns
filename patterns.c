
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "rgb_color.h"
#include "patterns.h"

#define RD 255
#define MAX_STEPS_RGB 255
#define COLORS 3
#define TAB_COLORS 6
#define BRGH_STEPS 8

rgb_t color_tab[TAB_COLORS + 1] = {
	[0] = {255,	0,		0},
	[1] = {0,	130,	130},	//sea
	[2] = {0,	255,	0},
	[3] = {0,	0,		255},
	[4] = {130,	0,		130},	//pink
	[5] = {130,	130,	0},		//yellow
	[6] = {90,	90,		90} 	//not used in "new_year"
};

//global variables
static int16_t step = 0; //tetris
static int16_t j = 0; //tetris
pattParam_t movingLedParam, standbyPattParam;
pattParam_t rgbPattParam, tetrisPattParam, staticPattParam;
pattParam_t floatingEndsPattParam, christmas_param, new_year_param;

/* ***********************************************************
 * colors moving smoothly on the line
 *
 * parameters:
 * 	param_1: 0..7 - first diode number, 8..15 - segment length
 * 	param_2: 0..7 - brightening step, 8..15 - movement step
 * 	param_3: 0..7 - next color index, 8 .. - previous color index
 *
 * ***********************************************************/
uint8_t christmas(uint8_t flags){
	int16_t up_delta[3], down_delta[3], on_diodes, old_diodes;
	rgb_t next_tab_color, prev_tab_color, up_color, down_color;
	uint8_t move_step, brgh_step, first_diode, next_color_index;
	uint8_t seg_len, brgh, prev_color_index;
	rgb_t *buff;
	pattParam_t *p;

	p = &christmas_param;
    //copy all needed parameters
    on_diodes = *(p -> on_diodes);
    old_diodes = p -> old_diodes;
    if (on_diodes != old_diodes){
		if (on_diodes <= 30){
			seg_len = 4;
		}
		else if (on_diodes <= 100){
			seg_len = 6;
		}
		else{
			seg_len = 8;
		}
		p -> param_1 = (p -> param_1 & 0x00FF) | (seg_len << 8);
    }


	next_color_index = (p -> param_3) & 0x00FF;
	prev_color_index = ((p -> param_3) >> 8) & 0x00FF;
	brgh_step = p -> param_2;
	move_step = (p -> param_2) >> 8;
	//on_diodes = *(p -> on_diodes);
	first_diode = (p -> param_1);
	seg_len = (p -> param_1) >> 8;

	next_tab_color = color_tab[next_color_index]; //next color
	prev_tab_color = color_tab[prev_color_index]; //previous color
	//next diode step
	up_delta[0] = next_tab_color.red << 2; //8 brightening steps
	up_delta[1] = next_tab_color.green << 2;
	up_delta[2] = next_tab_color.blue << 2;
	//next diode step
	down_delta[0] = prev_tab_color.red << 2; //8 brightening steps
	down_delta[1] = prev_tab_color.green << 2;
	down_delta[2] = prev_tab_color.blue << 2;

	//calculate new color values
	up_color.red = (brgh_step * up_delta[0]) >> 5;
	up_color.green = (brgh_step * up_delta[1]) >> 5;
	up_color.blue = (brgh_step * up_delta[2]) >> 5;
	brgh = p -> brightness;
	newColorBrgh(&up_color, brgh);
	down_color.red = ((BRGH_STEPS - brgh_step) * down_delta[0]) >> 5;
	down_color.green = ((BRGH_STEPS - brgh_step) * down_delta[1]) >> 5;
	down_color.blue = ((BRGH_STEPS - brgh_step) * down_delta[2]) >> 5;
	newColorBrgh(&down_color, brgh);

	uint8_t down_diode;
	if (first_diode == 0){
		down_diode = seg_len - 1;
	}
	else{
		down_diode = first_diode - 1;
	}

	buff = p -> rgb_color_buff;
	for (int16_t i = 0; i < on_diodes; i++){
		//change color of only 2 on_diodes
		if ((i%seg_len) == first_diode){
			buff[i] = up_color;
		}
		else if ((i%seg_len) == (down_diode)){
			buff[i] = down_color;
		}
		else{
			buff[i] = (rgb_t){0, 0, 0};
		}
	}

	brgh_step++;
	if (brgh_step == (BRGH_STEPS + 1)){
		brgh_step = 1;
		first_diode++;
		if (first_diode == seg_len){
			first_diode = 0;
		}
		move_step--;
		if (move_step == 0){
			move_step = seg_len;
			prev_color_index = next_color_index;
			next_color_index++;
			if (next_color_index == 7){
				next_color_index = 0;
			}
		}
		else{
			prev_color_index = next_color_index;
		}
	}

	p -> param_3 = (prev_color_index << 8) | next_color_index;
	p -> param_2 = brgh_step + (move_step << 8);
	p -> param_1 = (p -> param_1 & 0xFF00) + first_diode;

	return 1;
}


/***********************************************************
 *
 * parameters:
 * 	- param_1: number of diode which is changing color
 *	- param_2:  0..7: current color index
 *				8..15: step of color change
 * 	- param_3:	0 - diode number is going to the middle
 *				1 - diode number is going to the ends
 *
 * *********************************************************/
uint8_t new_year(uint8_t flags){
	int16_t diode_to_change, delta, on_diodes;
	uint8_t current_color_index, prev_color_index, step;
	uint8_t brgh;
	int16_t new_color[3], prev_color[3], next_color[3];
	uint8_t next_diode = 0;
	rgb_t new_rgb, prev_rgb;
	rgb_t *buff;
	pattParam_t *p;

	p = &new_year_param;
	diode_to_change = p -> param_1;
	current_color_index = (uint8_t)((p -> param_2) & 0x00FF);
	step = (uint8_t)(((p -> param_2) & 0xFF00) >> 8);
	brgh = p -> brightness;
	on_diodes = *(p -> on_diodes);

	if (current_color_index == 0){
		prev_color_index = TAB_COLORS - 1;
	}
	else{
		prev_color_index = current_color_index - 1;
	}

	rgb_t c1 = color_tab[prev_color_index];
	newColorBrgh(&c1, brgh);
	prev_rgb = c1;
	prev_color[0] = (int16_t)c1.red; //default white
	prev_color[1] = (int16_t)c1.green;
	prev_color[2] = (int16_t)c1.blue;

	c1 = color_tab[current_color_index];
	newColorBrgh(&c1, brgh);
	new_rgb = c1;
	new_color[0] = (int16_t)c1.red;
	new_color[1] = (int16_t)c1.green;
	new_color[2] = (int16_t)c1.blue;

	//calculate step of transformation
	for (uint8_t j = 0; j < 3; j++){
		delta = ((new_color[j] - prev_color[j]) << 2); //16 steps
		next_color[j] = (prev_color[j] << 5) + delta * step;
		if (delta >= 0){
			if ((next_color[j] >= (new_color[j] << 5)) && (step != 0) &&
					(delta != 0)){
				next_diode = 1;
				next_color[j] = new_color[j] << 5;
			}
		}
		else{
			if ((next_color[j] <= (new_color[j] << 5)) && (step != 0) &&
					(delta != 0)){
				next_diode = 1;
				next_color[j] = new_color[j] << 5;
			}
		}
	}

	buff = p -> rgb_color_buff;
	for (int16_t i = 0; i < on_diodes; i++){
		//diodes for change
		if ((i == diode_to_change) || (i == (on_diodes - diode_to_change - 1))){
			buff[i] = (rgb_t){next_color[0] >> 5,
				next_color[1] >> 5, next_color[2] >> 5};
		}
		else{
			if (p -> param_3 == 0){
				if ((i < diode_to_change) || (i > (on_diodes - diode_to_change - 1))){
					buff[i] = new_rgb;
				}
				else{
					buff[i] = prev_rgb;
				}
			}
			else{
				if ( ((i > diode_to_change) && (i < (on_diodes - diode_to_change - 1))) ||
						((i < diode_to_change) && (i > (on_diodes - diode_to_change - 1))) ){
					buff[i] = new_rgb;
				}
				else{
					buff[i] = prev_rgb;
				}
			}
		}
	}

	if (next_diode == 0){
		p -> param_2 = (uint16_t)current_color_index + ((uint16_t)++step << 8);
	}
	else{
		//next diode will be changing in the next step
		uint8_t change_color = 0;
		p -> param_1++;
		if (p -> param_1 == on_diodes){
			//end of line reached
			change_color = 1;
			p -> param_1 = 0; //diode number
			p -> param_3 = 0;
		}
		else{
			if (p -> param_3 == 0){
				//changes are going in direction of the middle
				if (on_diodes%2 == 1){
					//odd number of diodes
					if (p -> param_1 == ((on_diodes >> 1) + 1)){
						//middle of strip
						change_color = 1;
						p -> param_1--;
						p -> param_3 = 1;
					}
				}
				else{
					if (p -> param_1 == (on_diodes >> 1)){
						change_color = 1;
						p -> param_3 = 1;
					}
				}
			}
		}

		if (change_color == 1){
			current_color_index++;
			if (current_color_index == TAB_COLORS){
				current_color_index = 0;
			}
		}

		step = 0;
		p -> param_2 = (uint16_t)current_color_index + ((uint16_t)step << 8);
	}

	return 1;
}

//******************************************************
//floating ends
uint8_t floating_ends(uint8_t flags){
    int16_t diodes, variant;
    rgb_t startColor, endColor;
    uint8_t brgh, stepUp, stepDown, timeStep;
    rgb_t *buff;
    int16_t dRed, dGreen, dBlue;
    pattParam_t *param;

    param = &floatingEndsPattParam;
    //read pattern parameters
    variant = param -> param_1;
    timeStep = (uint8_t)(param -> param_2);
    diodes = *(param -> on_diodes);

	//define start/end colors
	stepUp = timeStep;
	stepDown = 255 - timeStep;
	startColor = (rgb_t){0, 0, 0};
	switch (variant){
		case (0):
			//start: red, end green -> blue
			startColor = (rgb_t){255, 0, 0};
			endColor = (rgb_t){0, stepDown, stepUp};
			break;
		case (1):
			//start: red -> green, end: blue
			startColor = (rgb_t){stepDown, stepUp, 0};
			endColor = (rgb_t){0, 0, 255};
			break;
		case (2):
			//start: green, end: blue -> red
			startColor = (rgb_t){0, 255, 0};
			endColor = (rgb_t){stepUp, 0, stepDown};
			break;
		case (3):
			//start: green -> blue, end: red
			startColor = (rgb_t){0, stepDown, stepUp};
			endColor = (rgb_t){255, 0, 0};
			break;
		case (4):
			//start: blue, end: red -> green
			startColor = (rgb_t){0, 0, 255};
			endColor = (rgb_t){stepDown, stepUp, 0};
			break;
		case (5):
			//start: blue -> red, end: green
			startColor = (rgb_t){stepUp, 0, stepDown};
			endColor = (rgb_t){0, 255, 0};
	}
	//calculate difference between diodes
	dRed = (((int16_t)endColor.red - (int16_t)startColor.red) << 5)/
			(diodes - 1);
	dGreen = (((int16_t)endColor.green - (int16_t)startColor.green) << 5)/
			(diodes - 1);
	dBlue = (((int16_t)endColor.blue - (int16_t)startColor.blue) << 5)/
			(diodes - 1);

    brgh = param -> brightness;
    buff = param -> rgb_color_buff;

	//update RGB buffer
	int16_t red, green, blue;
	
	red = (((int16_t)startColor.red) << 5);
	green = (((int16_t)startColor.green) << 5);
	blue = (((int16_t)startColor.blue) << 5);
	for (uint16_t i = 0; i < diodes; i++){
		endColor.red = (uint8_t)((red + (i * dRed)) >> 5);
		endColor.green = (uint8_t)((green + (i * dGreen)) >> 5);
		endColor.blue = (uint8_t)((blue + (i * dBlue)) >> 5);
	    newColorBrgh(&endColor, brgh);
	    buff[i] = (rgb_t){endColor.red, endColor.green, endColor.blue};
    }

	//update parameters in RAM
	if (timeStep == 255){
		//timeStep = 0;
		variant++;
		if (variant == 6){
			variant = 0;
		}
		param -> param_1 = variant;
	}
	timeStep++;
    param -> param_2 = timeStep;

    return 1;
}

//******************************************************
//static color
uint8_t static_color(uint8_t flags){
    int16_t diodes, i;
    rgb_t c1;
    uint8_t brgh;//, res, diff, prevBrgh;
    rgb_t *buff;
    pattParam_t *param;

    param = &staticPattParam;
    //read current color
    c1.red = param -> color_1.red;
    c1.green = param -> color_1.green;
    c1.blue = param -> color_1.blue;

    //diodes = *(param -> on_diodes);
    brgh = param -> brightness;
    //prevBrgh = (uint8_t)param -> param_1;
    buff = param -> rgb_color_buff;

    newColorBrgh(&c1, brgh);
    //refresh RGB values
    diodes = *(param -> on_diodes);
    for (i = 0; i < diodes; i++){
    	buff[i] = (rgb_t){c1.red, c1.green, c1.blue};
    }

    return 1;
}


//******************************************************
//points are added to the end, when the whole line is ON
//points start to jump out
uint8_t tetris(uint8_t flags){
    uint16_t i, allDiodes, old_diodes;
    rgb_t c1;
    uint8_t brgh, colorNr;
    rgb_t *buff;
    int16_t t1, on_diodes, direction;
    pattParam_t *param;

    param = &tetrisPattParam;
    allDiodes = *(param -> on_diodes);
    old_diodes = param -> old_diodes;
    if (allDiodes != old_diodes){
    	//if the nr of diodes changed reset pattern
    	param -> old_diodes = allDiodes;
    	param -> param_1 = 1;
        param -> param_2 = 0;
        param -> param_3 = 0;
        step = 0;
        j = 0;
    }
    brgh = param -> brightness;

	direction = param -> param_1;
	colorNr = (uint8_t)(param -> param_2);
	on_diodes = 	param -> param_3;
	
	//=====
	if ((flags & BRGH_ONLY) == 0){
    	step += direction;

	    if (direction == 1){
	        if (step == (allDiodes - on_diodes)){
	            //diode arrived to the end of line
	            on_diodes += direction;
	            step = 0;
	            if (on_diodes == allDiodes){
	                direction = -direction;
	                step = 1;
	            }
	        }
	    }
	    else{
	        j++;
	        if (step == 0){
	            on_diodes += direction;
	            if (on_diodes == 0){
	                direction = -direction;
	                step = 0;
	                colorNr++;
	                if (colorNr == COLORS){
	                    colorNr = 0;
	                }
	            }
	            else{
	                step = allDiodes - on_diodes + 1;
	                j = 0;
	            }
	        }
	    }

    	param -> param_1 = direction;
	    param -> param_2 = colorNr;
	    param -> param_3 = on_diodes;
    }
	//======
		
    c1 = (rgb_t){0, 0, 0};
    //set colors
    switch (colorNr){
		case 0: //red
			c1 = (rgb_t){255, 0, 0};;
			break;
		case 1: //green
			c1 = (rgb_t){0, 255, 0};
			break;
		case 2: //blue
			c1 = (rgb_t){0, 0, 255};
	}
	newColorBrgh(&c1, brgh);

    //refresh RGB values for all diodes
    buff = param -> rgb_color_buff;
    t1 = allDiodes - on_diodes;
    for (i = 0; i < allDiodes; i++){
        if (direction == 1){
            if ((i == step)|(i > (t1 - 1))){
            	buff[i] = (rgb_t){c1.red, c1.green, c1.blue};
            }
            else {
            	buff[i] = (rgb_t){0, 0, 0};
            }
        } //direction 1
        else {
            //direction = -1, switching diodes OFF
            if ((i == (t1 - 1 - j))|(i > (t1))){
            	buff[i] = (rgb_t){c1.red, c1.green, c1.blue};
            }
            else {
            	buff[i] = (rgb_t){0, 0, 0};
            }
        }
    }
    return 1;
}


//******************************************************
//smooth go through colors in table
uint8_t rgb_palette(uint8_t flags){
    uint16_t diodes, old_diodes, step, i;
    rgb_t c1;
    uint8_t brgh;
    uint8_t red, green, blue;
    rgb_t *buff;
    uint8_t colorNr;
    pattParam_t *param;

    param = &rgbPattParam;
    diodes = *(param -> on_diodes);
    old_diodes = param -> old_diodes;
    if (diodes != old_diodes){
    	//if the nr of diodes changed reset pattern
    	param -> old_diodes = diodes;
    	param -> param_1 = 0;
        param -> param_2 = 0;
        param -> param_3 = 0;
    }
    brgh = param -> brightness;

	//calculate new (for this step) color value
    colorNr = (uint8_t)(param -> param_2);
    c1 = param -> color_1;
    red = 0;
    green = 0;
    blue = 0;
    switch (colorNr){
		case 0: //red
			red = c1.red - 1;
			green = c1.green + 1;
			break;
		case 1: //green
			blue = c1.blue + 1;
			green = c1.green - 1;
			break;
		case 2: //blue
			red = c1.red + 1;
			blue = c1.blue - 1;
	}

	c1 = (rgb_t){red, green, blue};
	param -> color_1 = (rgb_t){c1.red, c1.green, c1.blue};
	newColorBrgh(&c1, brgh);

    //refresh RGB values for all diodes
    buff = param -> rgb_color_buff;
    for (i = 0; i < diodes; i++){
    	buff[i] = (rgb_t){c1.red, c1.green, c1.blue};
    }

	step = param -> param_1;
	step++;
	if (step == MAX_STEPS_RGB){ 	
    	colorNr = param -> param_2;
    	step = 0;
    	//set colors
        switch (colorNr){
			case 0: //red
				c1 = (rgb_t){255, 0, 0};
				colorNr = 1;
				break;
			case 1: //green
				c1 = (rgb_t){0, 255, 0};
				colorNr = 2;
				break;
			case 2: //blue
				c1 = (rgb_t){0, 0, 255};
				colorNr = 0;
		}
		param -> param_2 = colorNr;
		param -> color_1 = (rgb_t){c1.red, c1.green, c1.blue}; //color
    }
    param -> param_1 = step;

    return 1;
}


//******************************************************
//pattern 1
uint8_t moving_point(uint8_t flags){
    uint16_t param1, on_diodes, old_diodes, i;
    rgb_t *buff;
    rgb_t c1;
    uint8_t brgh;
    pattParam_t *param;

    param = &movingLedParam;
    //copy all needed parameters to local variables
    buff = param -> rgb_color_buff;
    //copy color_1
    c1.red = param -> color_1.red;
    c1.green = param -> color_1.green;
    c1.blue = param -> color_1.blue;
    on_diodes = *(param -> on_diodes);
    old_diodes = param -> old_diodes;
    if (on_diodes != old_diodes){
    	//if the nr of diodes changed reset pattern
    	param -> old_diodes = on_diodes;
    	param -> param_1 = -1;
        param -> param_2 = 0;
        param -> param_3 = 0;
    }
    param1 = (uint16_t)(param -> param_1); //nr of ON diode

	if ((flags & BRGH_ONLY) == 0){	
	    if (param1 >= on_diodes){
	    	param1 = 0;
	    }
	    else{
	    	param1++;
	    }
	    param -> param_1 = param1;
    }
    brgh = param -> brightness;
    newColorBrgh(&c1, brgh);

    //refresh RGB values
    for (i = 0; i < on_diodes; i++){
    	if (i != param1){
    		buff[i] = (rgb_t){0, 0, 0};
		}
		else{
			//only first diode is ON
			buff[i] = (rgb_t){c1.red, c1.green, c1.blue};
		}
    }
    return 1;
}


// ******************************************************
// standby pattern
uint8_t standby_patt(uint8_t flags){
	uint16_t n, i, on_diodes, old_diodes, ledA, ledB, dir;
	rgb_t *buff;
	rgb_t c1;
	uint8_t brgh, res = 0;
	pattParam_t *param;

	if ((flags & BRGH_ONLY) != 0){
		return 1;
	}

	param = &standbyPattParam;
    //t = param -> param_3;
    on_diodes = *(param -> on_diodes);
    old_diodes = param -> old_diodes;
    if (on_diodes != old_diodes){
    	//if the nr of diodes changed reset pattern
    	param -> old_diodes = on_diodes;
    	param -> param_1 = -1;
    	param -> param_2 = 1;
    	param -> param_3 = 2;
    }
    //-------------
    //copy all needed parameters
    brgh = param -> brightness;
    buff = param -> rgb_color_buff;
    //copy color_1
    c1.red = param -> color_1.red;
    c1.green = param -> color_1.green;
    c1.blue = param -> color_1.blue;
    n = param -> param_1; //nr of ON diode
    dir = param -> param_2;
    //----------
    n += dir;
    if (n == ((on_diodes >> 1) - 1 + on_diodes%2)){
    	dir = -1;
    }
    else if (n == 0){
       	uint8_t temp;
        	
        dir = 1;
        //change color
        temp = c1.red;
        c1.red = c1.green;
        c1.green = c1.blue;
        c1.blue = temp;
        //save new color
        param -> color_1.red = c1.red;
    	param -> color_1.green = c1.green;
    	param -> color_1.blue = c1.blue;
    }
    param -> param_1 = n;
    param -> param_2 = dir;

	newColorBrgh(&c1, brgh);

    //which diode will be on
    ledA = n;
    ledB = on_diodes - 1 - n;

    //refresh RGB values
    for (i = 0; i < on_diodes; i++){
      	if ((i == ledA) || (i == ledB)) {
    		//switch only 2 diodes
    		buff[i] = (rgb_t){c1.red, c1.green, c1.blue};
    	}
    	else{
    		buff[i] = (rgb_t){0, 0, 0};
    	}
    }
    res = 1;

    return res;
}


//***********************************************
//init pattern's parameter structure
void initPattParam(pattParam_t **param_tab,
					uint8_t *buffCol,
               		int32_t *diodes){ //brightness
	pattParam_t *param;
	rgb_t initColor;

	initColor = (rgb_t){0, 255, 0};

    for (uint8_t i = 0; i < PATTERNS_NR; i++){
   		//TODO read parameters saved in flash
   		param = param_tab[i];	
		param -> freq_max = 100;
		param -> freq_min = 1;
		param -> dt = 1000;
		param -> speed = 1;
		param -> color_1.red = initColor.red;
		param -> color_1.blue = initColor.blue;
		param -> color_1.green = initColor.green;
		param -> param_1 = 1;
		param -> param_2 = 0;
		param -> param_3 = 0;
		param -> brightness = 10;
		param -> on_diodes = diodes;
		param -> old_diodes = *diodes;
		param -> rgb_color_buff = (rgb_t *)buffCol;
	}
}


//***************************************************
//in every parameter set new RGB buffer (after the change of diodes number)
void set_rgb_buff(pattParam_t **paramTab, uint8_t *rgb_buff){
	for (uint8_t i = 0; i < PATTERNS_NR; i++){
		paramTab[i] -> rgb_color_buff = (rgb_t *)rgb_buff;
	}
}


/*****************************************************
*
* init RGB patterns
* initialize patterns and their parameter structures
*
* ****************************************************/
void initRgbPatterns(int32_t *diodes,
					uint8_t *rgb_buff,
					pattParam_t **paramTab,
					pRefreshFun *funTab){	

	funTab[0] = &standby_patt;
	paramTab[0] = &standbyPattParam;
	funTab[1] = &moving_point;
	paramTab[1] = &movingLedParam;
	funTab[2] = &rgb_palette;
	paramTab[2] = &rgbPattParam;
	funTab[3] = &tetris;
	paramTab[3] = &tetrisPattParam;
	funTab[4] = &static_color;
	paramTab[4] = &staticPattParam;
	funTab[5] = &floating_ends;
	paramTab[5] = &floatingEndsPattParam;
	funTab[6] = &new_year;
	paramTab[6] = &new_year_param;
	funTab[7] = &christmas;
	paramTab[7] = &christmas_param;
   	
	initPattParam(paramTab, rgb_buff, diodes);
	
   	//individual pattern settings
   	standbyPattParam.brightness = 1;
   	standbyPattParam.param_1 = -1;
	standbyPattParam.param_2 = 1;
	//standbyPattParam.param_3 = 2;
	standbyPattParam.dt = 2000;
   	rgbPattParam.param_1 = 0;
   	floatingEndsPattParam.param_1 = 0;
   	movingLedParam.param_1 = *diodes;

	//moving_point
	movingLedParam.freq_max = 20;
	//tetris
	tetrisPattParam.freq_max = 30;
	//new year
	new_year_param.freq_max = 50;
	//floating ends
	floatingEndsPattParam.freq_max = 150;
   	
   	//for christmas pattern
   	christmas_param.freq_max = 50;
	uint16_t segment_len;
	if (*diodes <= 30){
		segment_len = 4;
	}
	else if (*diodes <= 100){
		segment_len = 5;
	}
	else{
		segment_len = 6;
	}
	christmas_param.param_1 = 0 | (segment_len << 8);
	uint8_t move_step = 2;
	christmas_param.param_2 = 1 | (move_step << 8);
	uint8_t next_col = 5;
	uint8_t prev_col = next_col;
	christmas_param.param_3 = next_col | (prev_col << 8);
	//end of christmas settings
}
