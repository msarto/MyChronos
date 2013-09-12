// *************************************************************************************************
//
//      Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
//
//
//        Redistribution and use in source and binary forms, with or without
//        modification, are permitted provided that the following conditions
//        are met:
//
//          Redistributions of source code must retain the above copyright
//          notice, this list of conditions and the following disclaimer.
//
//          Redistributions in binary form must reproduce the above copyright
//          notice, this list of conditions and the following disclaimer in the
//          documentation and/or other materials provided with the
//          distribution.
//
//          Neither the name of Texas Instruments Incorporated nor the names of
//          its contributors may be used to endorse or promote products derived
//          from this software without specific prior written permission.
//
//        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//        "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//        LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//        A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//        OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//        SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//        LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//        DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//        THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//        (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//        OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// *************************************************************************************************
// Temperature measurement functions.
// *************************************************************************************************

// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "temperature.h"
#include "ports.h"
#include "display.h"
#include "adc12.h"
#include "timer.h"

// logic
#include "user.h"

// *************************************************************************************************
// Prototypes section
u8 is_temp_measurement(void);
s16 convert_C_to_F(s16 value);
s16 convert_F_to_C(s16 value);

// *************************************************************************************************
// Defines section

// *************************************************************************************************
// Global Variable section
struct temp sTemp;
u16 tempMax = 999;
u16 tempMin = 999;

// *************************************************************************************************
// Extern section

// *************************************************************************************************
// @fn          reset_temp_measurement
// @brief       Reset temperature measurement module.
// @param       none
// @return      none
// *************************************************************************************************
void reset_temp_measurement(void)
{
    // Set flag to off
    sTemp.state = MENU_ITEM_NOT_VISIBLE;

    // Perform one temperature measurements with disabled filter
    temperature_measurement(FILTER_OFF);
}

// *************************************************************************************************
// @fn          temperature_measurement
// @brief       Init ADC12. Do single conversion of temperature sensor voltage. Turn off ADC12.
// @param       none
// @return      none
// *************************************************************************************************
void temperature_measurement(u8 filter)
{
    u16 adc_result;
    volatile s32 temperature;

    // Convert internal temperature diode voltage
    adc_result = adc12_single_conversion(REFVSEL_0, ADC12SHT0_8, ADC12INCH_10);

    // Convert ADC value to "xx.x °C"
    // Temperature in Celsius
    // ((A10/4096*1500mV) - 680mV)*(1/2.25mV) = (A10/4096*667) - 302
    // = (A10 - 1855) * (667 / 4096)
    temperature = (((s32) ((s32) adc_result - 1855)) * 667 * 10) / 4096;

    // Add temperature offset
    temperature += sTemp.offset;

    // Store measured temperature
    if (filter == FILTER_ON)
    {
        // Change temperature in 0.1° steps towards measured value
        if (temperature > sTemp.degrees)
            sTemp.degrees += 1;
        else if (temperature < sTemp.degrees)
            sTemp.degrees -= 1;
    }
    else
    {
        // Override filter
        sTemp.degrees = (s16) temperature;
    }

    if (tempMax==999)
    	tempMax=temperature;
    if (tempMin==999)
    	tempMin=temperature;
    if (tempMax<temperature)
    	tempMax=temperature;
    if (tempMin>temperature)
    	tempMin=temperature;


    // New data is available --> do display update
    display.flag.update_temperature = 1;
}

// *************************************************************************************************
// @fn          convert_C_to_F
// @brief       Convert °C to °F
// @param       s16 value               Temperature in °C
// @return      s16                     Temperature in °F
// *************************************************************************************************
s16 convert_C_to_F(s16 value)
{
    s16 DegF;

    // Celsius in Fahrenheit = (( TCelsius × 9 ) / 5 ) + 32
    DegF = ((value * 9 * 10) / 5 / 10) + 32 * 10;

    return (DegF);
}

// *************************************************************************************************
// @fn          convert_F_to_C
// @brief       Convert °F to °C
// @param       s16 value               Temperature in 2.1 °F
// @return      s16                     Temperature in 2.1 °C
// *************************************************************************************************
s16 convert_F_to_C(s16 value)
{
    s16 DegC;

    // TCelsius =( TFahrenheit - 32 ) × 5 / 9
    DegC = (((value - 320) * 5)) / 9;

    return (DegC);
}

// *************************************************************************************************
// @fn          is_temp_measurement
// @brief       Returns TRUE if temperature measurement is enabled.
// @param       none
// @return      u8
// *************************************************************************************************
u8 is_temp_measurement(void)
{
    return (sTemp.state == MENU_ITEM_VISIBLE);
}

// *************************************************************************************************
// @fn          mx_temperature
// @brief       Mx button handler to set the temperature offset.
// @param       u8 line         LINE1
// @return      none
// *************************************************************************************************
void mx_temperature(u8 line)
{
    s32 temperature;
    s16 temperature0;
    volatile s16 temperature1;
    volatile s16 offset;

    // Clear display
    clear_display_all();

    // When using English units, convert internal °C to °F before handing over value to set_value
    // function
    if (!sys.flag.use_metric_units)
    {
        // Convert global variable to local variable
        temperature = convert_C_to_F(sTemp.degrees);
        temperature0 = sTemp.degrees;
    }
    else
    {
        // Convert global variable to local variable
        temperature = sTemp.degrees;
        temperature0 = temperature;
    }

    // Loop values until all are set or user breaks set
    while (1)
    {
        // Idle timeout: exit without saving
        if (sys.flag.idle_timeout)
            break;

        // Button STAR (short): save, then exit
        if (button.flag.star)
        {
            // For English units, convert set °F to °C
            if (!sys.flag.use_metric_units)
            {
                temperature1 = convert_F_to_C(temperature);
            }
            else
            {
                temperature1 = temperature;
            }

            // New offset is difference between old and new value
            offset = temperature1 - temperature0;
            sTemp.offset += offset;

            // Force filter to new value
            sTemp.degrees = temperature1;

            // Set display update flag
            display.flag.line2_full_update = 1;


            break;
        }



        // Set current temperature - offset is set when leaving function
        set_value(&temperature, 3, 1, -999, 999, SETVALUE_DISPLAY_VALUE + SETVALUE_DISPLAY_ARROWS,
        		LCD_SEG_L2_2_0,
                  display_value);
        display_symbol(LCD_SEG_L2_DP, SEG_ON);
        // emfanizei to noymero sta 5 teleftaia
        //        		LCD_SEG_L2_4_0,
        // emfanizei to noymero sta 4 teleftaia
        //        		LCD_SEG_L2_3_0,
    }

    // Clear button flags
    button.all_flags = 0;
}

// *************************************************************************************************
// @fn          display_temperature
// @brief       Common display routine for metric and English units.
// @param       u8 line                 LINE1
//                              u8 update               DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_CLEAR
// @return      none
// *************************************************************************************************
void display_temperature(u8 line, u8 update)
{
    u8 *str;
    s16 temperature;

    // Redraw whole screen
    if (update == DISPLAY_LINE_UPDATE_FULL)
    {
        // Menu item is visible
        sTemp.state = MENU_ITEM_VISIBLE;


        // Perform one temperature measurement with disabled filter
        temperature_measurement(FILTER_OFF);

        // Display temperature
        display_temperature(LINE2, DISPLAY_LINE_UPDATE_PARTIAL);
    }
    else if (update == DISPLAY_LINE_UPDATE_PARTIAL)
    {
    	if (sTemp.display == DISPLAY_DEFAULT_VIEW)
    	{
    		// When using English units, convert °C to °F (temp*1.8+32)
    		if (!sys.flag.use_metric_units)
    		{
    			temperature = convert_C_to_F(sTemp.degrees);
    		}
    		else
    		{
    			temperature = sTemp.degrees;
    		}

    		// Indicate temperature sign through arrow up/down icon
    		if (temperature < 0)
    		{
    			// Convert negative to positive number
    			temperature = ~temperature;
    			temperature += 1;

    			display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
    			display_symbol(LCD_SYMB_ARROW_DOWN, SEG_ON);
    		}
    		else                    // Temperature is >= 0
    		{
    			display_symbol(LCD_SYMB_ARROW_UP, SEG_ON);
    			display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
    		}

    		// Limit min/max temperature to +/- 99.9 °C / °F
    		if (temperature > 999)
    			temperature = 999;



    		// Display result in xx.x format
    		str = int_to_array(temperature, 3, 1);
    		display_chars(LCD_SEG_L2_3_0, str, SEG_ON);
    		// Display °C / °F
    		display_symbol(LCD_SEG_L2_DP, SEG_ON);
    		//        display_symbol(LCD_UNIT_L1_DEGREE, SEG_ON);
    		if (sys.flag.use_metric_units)
    			display_char(LCD_SEG_L2_0, 'C', SEG_ON);
    		else
    			display_char(LCD_SEG_L2_0, 'F', SEG_ON);

    	}
    	else
    		if (sTemp.display == DISPLAY_ALTERNATIVE_VIEW)
    		{
    			str = int_to_array(tempMax,3,1);
   	    		display_chars(LCD_SEG_L2_3_0, str, SEG_ON);
   	    		// Display °C / °F
   	    		display_symbol(LCD_SEG_L2_DP, SEG_ON);
   	    		//        display_symbol(LCD_UNIT_L1_DEGREE, SEG_ON);
   	    		if (sys.flag.use_metric_units)
   	    			display_char(LCD_SEG_L2_0, 'C', SEG_ON);
   	    		else
   	    			display_char(LCD_SEG_L2_0, 'F', SEG_ON);
    			display_symbol(LCD_SYMB_ARROW_UP, SEG_ON);
    			display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
    		}
    		else
        		if (sTemp.display == DISPLAY_ALTERNATIVE_VIEW_2)
        		{

        			str = int_to_array(tempMin,3,1);
       	    		display_chars(LCD_SEG_L2_3_0, str, SEG_ON);
       	    		// Display °C / °F
       	    		display_symbol(LCD_SEG_L2_DP, SEG_ON);
       	    		//        display_symbol(LCD_UNIT_L1_DEGREE, SEG_ON);
       	    		if (sys.flag.use_metric_units)
       	    			display_char(LCD_SEG_L2_0, 'C', SEG_ON);
       	    		else
       	    			display_char(LCD_SEG_L2_0, 'F', SEG_ON);
        			display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
        			display_symbol(LCD_SYMB_ARROW_DOWN, SEG_ON);
        		}
    }
    else
    	if (update == DISPLAY_LINE_CLEAR)
    	{
    		sTemp.display = DISPLAY_DEFAULT_VIEW;
    		// Menu item is not visible
    		sTemp.state = MENU_ITEM_NOT_VISIBLE;

    		// Clean up function-specific segments before leaving function
    		display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
    		display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
    		//        display_symbol(LCD_UNIT_L1_DEGREE, SEG_OFF);
    		display_symbol(LCD_SEG_L2_DP, SEG_OFF);
    	}
}

//*************************************************************************************************
// @fn          sx_temperature
// @brief       Temperature user routine. Toggles view between Temperature, Max, Min.
// @param       line            LINE1, LINE2
// @return      none
// *************************************************************************************************
//void sx_date(u8 line)
//{
//    // Toggle display items
//    if (sDate.display == DISPLAY_DEFAULT_VIEW)
//        sDate.display = DISPLAY_ALTERNATIVE_VIEW;
//    else
//        sDate.display = DISPLAY_DEFAULT_VIEW;
//}
void sx_temperature(u8 line)
{
    // Toggle display items
    if (sTemp.display == DISPLAY_DEFAULT_VIEW) { sTemp.display = DISPLAY_ALTERNATIVE_VIEW; }
    else if (sTemp.display == DISPLAY_ALTERNATIVE_VIEW  ) { sTemp.display = DISPLAY_ALTERNATIVE_VIEW_2; }
    else { sTemp.display = DISPLAY_DEFAULT_VIEW; }
}
