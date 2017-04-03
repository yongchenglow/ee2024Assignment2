/*****************************************************************************
 *   A demo example using several of the peripherals on the base board
 *
 *   Copyright(C) 2011, EE2024
 *   All rights reserved.
 *
 ******************************************************************************/

/**
 * Import Libraries from LPC17
 */
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h"

/**
 * Import Libraries from Baseboard
 */
#include "joystick.h"
#include "pca9532.h"
#include "acc.h"
#include "oled.h"
#include "rgb.h"
#include "led7seg.h"
#include "light.h"
#include "temp.h"

#include <stdio.h>
#include <string.h>

/**
 * Declaration of Variables
 */
static uint8_t barPos = 2;
uint32_t msTicks = 0;
uint32_t light = 0;
uint32_t temperature = 0;
int8_t x = 0, y = 0, z = 0;

//OLED Strings
char OLED_TEMPERATURE[15];
char OLED_LIGHT[15];
char OLED_X[15];
char OLED_Y[15];
char OLED_Z[15];

//Enum
typedef enum{
	MODE_STABLE, MODE_MONITOR
};

//Read Sensors
void readSensors(uint32_t* light, uint32_t* temperature, uint8_t* x, uint8_t* y, uint8_t* z){
		*temperature = temp_read();
		*light = light_read();
		acc_read(&*x, &*y, &*z);
}

/**
 * Move Bar
 */
static void moveBar(uint8_t steps, uint8_t dir)
{
    uint16_t ledOn = 0;

    if (barPos == 0)
        ledOn = (1 << 0) | (3 << 14);
    else if (barPos == 1)
        ledOn = (3 << 0) | (1 << 15);
    else
        ledOn = 0x07 << (barPos-2);

    barPos += (dir*steps);
    barPos = (barPos % 16);

    pca9532_setLeds(ledOn, 0xffff);
}

/**
 * Draw O Led
 */
static void drawOled(uint8_t joyState)
{
    static int wait = 0;
    static uint8_t currX = 48;
    static uint8_t currY = 32;
    static uint8_t lastX = 0;
    static uint8_t lastY = 0;

    if ((joyState & JOYSTICK_CENTER) != 0) {
        oled_clearScreen(OLED_COLOR_BLACK);
        return;
    }

    if (wait++ < 3)
        return;

    wait = 0;

    if ((joyState & JOYSTICK_UP) != 0 && currY > 0) {
        currY--;
    }

    if ((joyState & JOYSTICK_DOWN) != 0 && currY < OLED_DISPLAY_HEIGHT-1) {
        currY++;
    }

    if ((joyState & JOYSTICK_RIGHT) != 0 && currX < OLED_DISPLAY_WIDTH-1) {
        currX++;
    }

    if ((joyState & JOYSTICK_LEFT) != 0 && currX > 0) {
        currX--;
    }

    if (lastX != currX || lastY != currY) {
        oled_putPixel(currX, currY, OLED_COLOR_WHITE);
        lastX = currX;
        lastY = currY;
    }
}


/**
 * Defining Note PIN
 */
#define NOTE_PIN_HIGH() GPIO_SetValue(0, 1<<26);
#define NOTE_PIN_LOW()  GPIO_ClearValue(0, 1<<26);


/**
 * Function to get the duration
 */
static uint32_t getDuration(uint8_t ch)
{
    if (ch < '0' || ch > '9')
        return 400;

    /* number of ms */

    return (ch - '0') * 200;
}

/**
 * Function to get pause
 */
static uint32_t getPause(uint8_t ch)
{
    switch (ch) {
    case '+':
        return 0;
    case ',':
        return 5;
    case '.':
        return 20;
    case '_':
        return 30;
    default:
        return 5;
    }
}

/**
 * Function to play song
 */

static void init_ssp(void)
{
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	/*
	 * Initialize SPI pin connect
	 * P0.7 - SCK;
	 * P0.8 - MISO
	 * P0.9 - MOSI
	 * P2.2 - SSEL - used as GPIO
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);

}

/**
 * Initialization of i2c
 */
static void init_i2c(void)
{
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

/**
 * Initialization of GPIO
 */
static void init_GPIO(void)
{
	// Initialize button SW4 (not really necessary since default configuration)
		PINSEL_CFG_Type PinCfg;
		PinCfg.Funcnum = 0;
		PinCfg.OpenDrain = 0;
		PinCfg.Pinmode = 0;
		PinCfg.Portnum = 1;
		PinCfg.Pinnum = 31;
		PINSEL_ConfigPin(&PinCfg);
		GPIO_SetDir(1, 1<<31, 0);

		//Initialize button sw3
		PinCfg.Funcnum = 0;
		PinCfg.OpenDrain = 0;
		PinCfg.Pinmode = 0;
		PinCfg.Portnum = 2;
		PinCfg.Pinnum = 10;
		PINSEL_ConfigPin(&PinCfg);
		GPIO_SetDir(2, 1 << 10, 0);

		/* ---- Speaker ------> */

		   GPIO_SetDir(2, 1<<0, 1);
		   GPIO_SetDir(2, 1<<1, 1);

		   GPIO_SetDir(0, 1<<27, 1);
		   GPIO_SetDir(0, 1<<28, 1);
		   GPIO_SetDir(2, 1<<13, 1);

		   // Main tone signal : P0.26
		   GPIO_SetDir(0, 1<<26, 1);

		   GPIO_ClearValue(0, 1<<27); //LM4811-clk
		   GPIO_ClearValue(0, 1<<28); //LM4811-up/dn
		   GPIO_ClearValue(2, 1<<13); //LM4811-shutdn

}

/**
 * Method of Tick Handler
 * Increase tick count by 1 every millisecond
 */
void SysTick_Handler(void) {
    msTicks++;
}

/**
 * Method to get the tick count
 */
uint32_t getTicks() {
    return msTicks;
}


/**
 * Function to display 7 Seg
 */
const char displayValues[] = "0123456789ABCDEF";
void run7Seg(int *segCount, uint32_t *prevGetTicks){
	if(getTicks()-(*prevGetTicks) >= 1000){
		*prevGetTicks = getTicks();
		led7seg_setChar(displayValues[*segCount], FALSE);
		if(*segCount == 15)
			*segCount = 0;
		else
			(*segCount)++;
    }
}

/**
 * Function run the blinking of RGB
 */
void runBlinkRGB(int *flag, uint32_t *prevGetFlicker, uint8_t colour){
    if(getTicks() - *prevGetFlicker >= 333){
    	*prevGetFlicker = getTicks();
    	*flag = !(*flag);
    	if(*flag == 1)
    		rgb_setLeds(colour);
    	else
    		rgb_setLeds(0);
    }
}

/**
 * Main Method
 */
int main (void) {

	int segCount = 0;
	int *digit;
	digit = &segCount;

    int32_t xoff = 0;
    int32_t yoff = 0;
    int32_t zoff = 0;

    int8_t x = 0;

    int8_t y = 0;
    int8_t z = 0;
    uint8_t dir = 1;
    uint8_t wait = 0;

    uint8_t state = 0;

    uint8_t RGB_RED_AND_BLUE = 0x03;


    init_i2c();
    init_ssp();
    init_GPIO();

    pca9532_init();
    joystick_init();
    oled_init();
    rgb_init();

    //accelerometer
    acc_init();

    //light sensor
    light_enable();

    //temp sensor
    temp_init(getTicks);

    // 7 Seg
    led7seg_init();
    uint32_t prevGetTicks = getTicks();
    uint32_t prevGetFlicker = getTicks();
    int flag = 0;

    // Setup SysTick Timer to interrupt at 1 msec intervals
	if (SysTick_Config(SystemCoreClock / 1000)) {
	    while (1);  // Capture error
	}

    /*
     * Assume base board in zero-g position when reading first value.
     */
    acc_read(&x, &y, &z);
    xoff = 0-x;
    yoff = 0-y;
    zoff = 0-z;

    /* ---- Speaker ------> */

    GPIO_SetDir(2, 1<<0, 1);
    GPIO_SetDir(2, 1<<1, 1);

    GPIO_SetDir(0, 1<<27, 1);
    GPIO_SetDir(0, 1<<28, 1);
    GPIO_SetDir(2, 1<<13, 1);
    GPIO_SetDir(0, 1<<26, 1);

    GPIO_ClearValue(0, 1<<27); //LM4811-clk
    GPIO_ClearValue(0, 1<<28); //LM4811-up/dn
    GPIO_ClearValue(2, 1<<13); //LM4811-shutdn

    /* <---- Speaker ------ */

    moveBar(1, dir);
    oled_clearScreen(OLED_COLOR_BLACK);


    while (1)
    {


        /* ####### Joystick and OLED  ###### */
        /* # */


        state = joystick_read();
        if (state != 0)
            drawOled(state);


        // 7 Segment Display
        run7Seg(&segCount,&prevGetTicks);

        // Blink Red
        //runBlinkRGB(&flag,&prevGetFlicker,RGB_RED);

        // Blink Blue
        //runBlinkRGB(&flag,&prevGetFlicker,RGB_BLUE);

        // Blink Both
        runBlinkRGB(&flag, &prevGetFlicker,RGB_RED_AND_BLUE);

        /* ############ Trimpot and RGB LED  ########### */
        /* # */


        /* ############ Trimpot and RGB LED  ########### */
        /* # */


        //oled
        //oled_clearScreen(OLED_COLOR_BLACK);
        if(*digit == 5 || *digit == 10 || *digit == 15){
        	readSensors(&light, &temperature, &x, &y, &z);
        	x = x + xoff;
			y = y + yoff;
			z = z + zoff;
			sprintf(OLED_TEMPERATURE, "Temp: %.1f", temperature/10.0);
			sprintf(OLED_LIGHT, "Light: %lu", light);
			sprintf(OLED_X, "X: %d", x);
			sprintf(OLED_Y, "y: %d", y);
			sprintf(OLED_Z, "z: %d", z);
			oled_putString(0, 0, (uint8_t*) OLED_TEMPERATURE, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
			oled_putString(0, 10, (uint8_t*) OLED_LIGHT, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
			oled_putString(0, 20, (uint8_t*) OLED_X, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
			oled_putString(0, 30, (uint8_t*) OLED_Y, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
			oled_putString(0, 40, (uint8_t*) OLED_Z, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
        }


    }
}

void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1);
}
