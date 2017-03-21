/*****************************************************************************
 *   rgb.c:  Driver for the RGB LED
 *
 *   Copyright(C) 2009, Embedded Artists AB
 *   All rights reserved.
 *
 ******************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/

#include "lpc17xx_gpio.h"
#include "rgb.h"

/******************************************************************************
 * Defines and typedefs
 *****************************************************************************/

/******************************************************************************
 * External global variables
 *****************************************************************************/

/******************************************************************************
 * Local variables
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/

/******************************************************************************
 * Public Functions
 *****************************************************************************/

/******************************************************************************
 *
 * Description:
 *    Initialize RGB driver
 *
 *****************************************************************************/
void rgb_init (void)
{
    GPIO_SetDir( 2, (1<<0), 1 );
    GPIO_SetDir( 0, (1<<26), 1 );
    GPIO_SetDir( 2, (1<<1), 1 );

}


/******************************************************************************
 *
 * Description:
 *    Set LED states
 *
 * Params:
 *    [in]  ledMask  - The mask is used to turn LEDs on or off
 *
 *****************************************************************************/
void rgb_setLeds (uint8_t ledMask)
{
    if ((ledMask & RGB_RED) != 0) {
        GPIO_SetValue( 2, (1<<0));
    } else {
        GPIO_ClearValue( 2, (1<<0));
    }

    if ((ledMask & RGB_BLUE) != 0) {
        GPIO_SetValue( 0, (1<<26) );
    } else {
        GPIO_ClearValue( 0, (1<<26) );
    }

    if ((ledMask & RGB_GREEN) != 0) {
        GPIO_SetValue( 2, (1<<1) );
    } else {
        GPIO_ClearValue( 2, (1<<1) );
    }

}
