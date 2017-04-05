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
#include "lpc17xx_uart.h"

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

/**
 * Import Libraries from C
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Define Constants
 */
#define LIGHT_LOW_WARNING 50
#define TEMP_HIGH_WARNING 26

/**
 * Define the two different types of mode
 */
typedef enum{
	MODE_STABLE, MODE_MONITOR
} system_mode;

/**
 * Initialize Variables
 */
volatile system_mode mode;
uint32_t msTicks = 0;

/**
 * Read Sensors
 * 	1) Light
 * 	2) Temperature
 * 	3) Accelerometer
 */
void readSensors(uint32_t* light, uint32_t* temperature, uint8_t* x, uint8_t* y, uint8_t* z){
	*light = light_read();
	*temperature = temp_read();
	acc_read(&*x, &*y, &*z);
}

/**
 * Define the pins used for UART
 */
void pinsel_uart3(void){
    PINSEL_CFG_Type PinCfg;
    PinCfg.Funcnum = 2;
    PinCfg.Pinnum = 0;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 1;
    PINSEL_ConfigPin(&PinCfg);
}

/**
 * Initialize UART terminal
 */
void init_uart(void){
    UART_CFG_Type uartCfg;
    uartCfg.Baud_rate = 115200;
    uartCfg.Databits = UART_DATABIT_8;
    uartCfg.Parity = UART_PARITY_NONE;
    uartCfg.Stopbits = UART_STOPBIT_1;
    //pin select for uart3;
    pinsel_uart3();
    //supply power & setup working parameters for uart3
    UART_Init(LPC_UART3, &uartCfg);
    //enable transmit for uart3
    UART_TxCmd(LPC_UART3, ENABLE);
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
 * Initialize Standard Private SSP Interrupt handler
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
}

/**
 * Function of Tick Handler
 * Increase tick count by 1 every millisecond
 */
void SysTick_Handler(void) {
    msTicks++;
}

/**
 * Function to get the tick count
 */
uint32_t getTicks() {
    return msTicks;
}

/**
 * Function to display 7 Seg
 */
const char displayValues[] = "0123456789ABCDEF";
int run7Seg(int *segCount, uint32_t *prevGetTicks){
	if(getTicks()-(*prevGetTicks) >= 1000){
		*prevGetTicks = getTicks();
		led7seg_setChar(displayValues[*segCount], FALSE);
		if(*segCount == 15)
			*segCount = 0;
		else
			(*segCount)++;
		return 1;
    }
	return 0;
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
 * Main Function
 */
int main (void) {

	/**
	 * Setup SysTick Timer to interrupt at 1msec intervals
	 */
	if (SysTick_Config(SystemCoreClock / 1000))
	    while (1);  // Capture error

	/**
	 * Initialization of Devices
	 */
    init_i2c();
    init_ssp();
    init_GPIO();
    init_uart();
    pca9532_init();
    joystick_init();
    oled_init();
    rgb_init();
    acc_init();
    light_enable();
    temp_init(getTicks);
    led7seg_init();

	/**
	 * Initialize OLED
	 */
	char OLED_TEMPERATURE[15];
	char OLED_LIGHT[15];
	char OLED_X[15];
	char OLED_Y[15];
	char OLED_Z[15];
	uint8_t clearScreen = 0;
	oled_clearScreen(OLED_COLOR_BLACK);

	/**
	 * Initialization of Accelerometer Variables
	 */
    int32_t xoff = 0;
    int32_t yoff = 0;
    int32_t zoff = 0;
    int8_t x = 0;
    int8_t y = 0;
    int8_t z = 0;
    int8_t prevX = 0;
    int8_t prevY = 0;
    int8_t prevZ = 0;
    int32_t movement = 0;

    /*
     * Assume base board in zero-g position when reading first value.
     */
    acc_read(&x, &y, &z);
    xoff = 0-x;
    yoff = 0-y;
    zoff = 0-z;

	/**
	 * Define Necessary variables required
	 */
	// UART
	char enterMonitor[] = "Entering MONITOR mode.\r\n";
	unsigned char result[100] = "";
	uint8_t firstTimeEnterMonitor = 0;
	int message = 0;

	// Time
    uint32_t prevGetTicks = getTicks();
    uint32_t prevGetFlicker = getTicks();

    // 7 Segment
    int segCount = 0;

    // Light and Temperature Values
	uint32_t light = 0;
	uint32_t temperature = 0;
	int update = 0;

	// RGB
    uint8_t RGB_RED_AND_BLUE = 0x03;
    int onOrOff = 0;
    int32_t warning = 0;

    // SwitchButton 4
    uint8_t sw4 = 0;
    uint32_t sw4PressTicks = getTicks();

    while (1)
    {
		/**
		 * Switch button to determine the mode of the board
		 */
    	sw4 = (GPIO_ReadValue(1) >> 31) & 0x01;
    	if((sw4 == 0) && (getTicks()- sw4PressTicks >= 500)){
    		sw4PressTicks = getTicks();
    		if(mode == MODE_STABLE){
    			mode = MODE_MONITOR;
    		}else{
    			mode = MODE_STABLE;
    			clearScreen = 0;
    		}
    	}

		/**
		 * Configure the board to run according to the mode
		 */
    	switch(mode){
    		/**
    		 * All Sensors are turned off, no readings are being made
    		 */
    		case MODE_STABLE:
    			if(clearScreen == 0){
					// Turns the OLED Screen off
					oled_clearScreen(OLED_COLOR_BLACK);

					// Turns the OLED Screen off
					led7seg_setChar(' ', FALSE);

					// Reset the count of the 7 Seg
					segCount = 0;

					// Ensure that RGB is off
					rgb_setLeds(0);

					// Indicate that the screen is cleared
					clearScreen = 1;

					// Flag to indicate entering monitor mode
					firstTimeEnterMonitor = 0;
    			}

    		break;

    		/**
    		 * Turn the following devices on:
    		 * 	1) 7 Segment Display
    		 * 	2) Temperature Sensor
    		 * 	3) Light Sensors
    		 * 	4) Accelerometer
    		 * 	5) OLED
    		 * 	6) UART
    		 * 	7) Blinking Lights
    		 */
    		case MODE_MONITOR:

    	        /**
    	         * Check to see if this is the first time entering monitor mode
    	         */
    			if(firstTimeEnterMonitor == 0){
    				// Send message to UART
    				UART_Send(LPC_UART3, (uint8_t *) enterMonitor, strlen(enterMonitor), BLOCKING);

    				// Change Flag
    				firstTimeEnterMonitor = 1;

					// Flag to set clearScreen when change mode
					clearScreen = 0;
    			}

    	        /**
    	         * 7 Segment Display
    	         * Increases segCount after use
    	         */
    	        update = run7Seg(&segCount,&prevGetTicks);

    	        if(update == 1){
					/**
					 * OLED
					 * Values changes when 7Seg display 5,10,15
					 * segCount is 1 value higher as it is incremented by run7seg
					 */
					if(segCount == 6 || segCount == 11 || segCount == 0){
						readSensors(&light, &temperature, &x, &y, &z);
						x = x + xoff;
						y = y + yoff;
						z = z + zoff;
						sprintf(OLED_TEMPERATURE, "Temp: %-5.1f", temperature/10.0);
						sprintf(OLED_LIGHT, "Light: %-5lu", light);
						sprintf(OLED_X, "X: %-5d", x);
						sprintf(OLED_Y, "y: %-5d", y);
						sprintf(OLED_Z, "z: %-5d", z);
						oled_putString(0,  0, (uint8_t*) "    MONITOR    ", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
						oled_putString(0, 10, (uint8_t*) OLED_TEMPERATURE, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
						oled_putString(0, 20, (uint8_t*) OLED_LIGHT, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
						oled_putString(0, 30, (uint8_t*) OLED_X, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
						oled_putString(0, 40, (uint8_t*) OLED_Y, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
						oled_putString(0, 50, (uint8_t*) OLED_Z, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
					}

					/**
					 * Updates warnings every 15 seconds
					 * segCount is 1 value higher as it is incremented by run7seg
					 */
					if(segCount == 0){
						// Determine if there is movement
						if(abs(x-prevX) > 15 || abs(y-prevY) > 15 || abs(z-prevZ) > 15)
							movement = 1;
						else
							movement = 0;

						// Store the accelerometer values
						prevX = x;
						prevY = y;
						prevZ = z;

						// Issue Warning accordingly
						if(temperature/10.0 > TEMP_HIGH_WARNING && movement == 1 && light < LIGHT_LOW_WARNING){
							warning = 3;
						} else if (temperature/10.0 > TEMP_HIGH_WARNING){
							warning = 2;
						} else if (movement == 1 && light < LIGHT_LOW_WARNING){
							warning = 1;
						} else{
							warning = 0;
						}
					}

					/**
					 * UART
					 * Values changes when 7Seg display 15
					 * segCount is 1 value higher as it is incremented by run7seg
					 */
					if(segCount == 0){
						sprintf(result, "%03d_-_T%-5.1f_L%-5lu_AX%-5d_AY%-5d_AZ%-5d\r\n", message, temperature / 10.0, light, x, y, z);
						UART_Send(LPC_UART3, (uint8_t *) result, strlen(result), BLOCKING);
						message++;
					}
    	        }

				/**
				 * Run Warnings
				 * High Temperature: 		Blink Red
				 * Movement and Low Light: 	Blink Blue
				 * Do note that both can occur at the same time
				 */
				if(warning == 3)
					runBlinkRGB(&onOrOff, &prevGetFlicker,RGB_RED_AND_BLUE);
				else if(warning == 2)
					runBlinkRGB(&onOrOff,&prevGetFlicker,RGB_RED);
				else if(warning == 1)
					runBlinkRGB(&onOrOff,&prevGetFlicker,RGB_BLUE);
				else
					rgb_setLeds(0);

    		break;
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
