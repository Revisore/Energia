/*
 ************************************************************************
 *	wiring_analog.c
 *
 *	Arduino core files for MSP430
 *		Copyright (c) 2012 Robert Wessels. All right reserved.
 *
 *
 ***********************************************************************
  Derived from:
  wiring_analog.c - analog input and output
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2005-2006 David A. Mellis

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA
 */

#include "wiring_private.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/timer.h"

uint32_t getTimerBase(uint8_t timer) {

    if(timer >= WT2A) {
        return (WTIMER2_BASE + ((timerToOffset(timer) - 6) << 12));
    }
    else {
        return (TIMER0_BASE + (timerToOffset(timer) << 12));
    }

}

uint8_t getTimerInterrupt(uint8_t timer) {

    if(timer < T3A) {
        return(INT_TIMER0A + timer);
    }
    else if(timer == T3A || timer == T3B){
        return (INT_TIMER3A + timer - 11);
    }
    else {
        return(INT_TIMER4A + timer - 13);
    }

}

//
//empty function due to single reference
//
void analogReference(uint16_t mode)
{
}

void PWMWrite(uint8_t pin, uint32_t analog_res, uint32_t duty, unsigned int freq)
{
    uint8_t bit = digitalPinToBitMask(pin); // get pin bit
    uint8_t port = digitalPinToPort(pin);   // get pin port
    uint8_t timer = digitalPinToTimer(pin);

    if (duty == 0) {
        digitalWrite(pin, LOW);
    }
    else if (duty > analog_res) {
        duty = analog_res;
    }
    else {
        uint32_t periodPWM = ROM_SysCtlClockGet()/freq;
        uint32_t portBase = (uint32_t) portBASERegister(port);
        uint32_t offset = timerToOffset(timer);
        uint32_t * timerBase = (uint32_t *) getTimerBase(timer);
        uint32_t timerAB = TIMER_A << timerToAB(timer);

        if(offset > TIMER3) {
            SysCtlPeripheralEnable((SYSCTL_PERIPH_WTIMER0 - 1) + (1 <<(offset-4)));
            timerBase = (uint32_t *)(WTIMER2_BASE + ((offset - 6) << 12));
        }
        else {
            SysCtlPeripheralEnable((SYSCTL_PERIPH_TIMER0 - 1) + (1 << offset));
            timerBase =(uint32_t *)(TIMER0_BASE + (offset << 12));
        }

        if (port == NOT_A_PORT) return; 	// pin on timer?
        ROM_GPIOPinConfigure(timerToPinConfig(timer));
        ROM_GPIOPinTypeTimer((long unsigned int) portBase, bit);
        ROM_TimerConfigure((long unsigned int) timerBase, TIMER_CFG_16_BIT_PAIR | (TIMER_CFG_A_PWM << timerToAB(timer)));
        ROM_TimerLoadSet((long unsigned int) timerBase, timerAB, periodPWM & 0xFFFF);
        ROM_TimerMatchSet((long unsigned int) timerBase, timerAB, ((analog_res-duty)*periodPWM/analog_res) & 0xFFFF);
        if(periodPWM > 0xFFFF) {
            ROM_TimerPrescaleSet((unsigned long) timerBase, timerAB, (periodPWM & 0xFFFF0000) >> 16);
            ROM_TimerPrescaleMatchSet((unsigned long) timerBase, timerAB, (((analog_res-duty)*periodPWM/analog_res) & 0xFFFF0000) >> 16);
        }
        ROM_TimerEnable((long unsigned int) timerBase, timerAB);
    }
}
void analogWrite(uint8_t pin, int val)//val=the duty cycle
{
    PWMWrite(pin, 255, val, 490);
}

uint16_t analogRead(uint8_t pin)
{

    uint8_t port = digitalPinToPort(pin);
    uint16_t value[1];
    uint32_t channel = digitalPinToADCIn(pin);
    if (pin == NOT_ON_ADC) { //invalid ADC pin
        return 0;
    }
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    ROM_GPIOPinTypeADC((uint32_t) portBASERegister(port), digitalPinToBitMask(pin));
    ROM_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 3, 0, channel | ADC_CTL_IE | ADC_CTL_END);
    ROM_ADCSequenceEnable(ADC0_BASE, 3);

    ROM_ADCIntClear(ADC0_BASE, 3);
    ROM_ADCProcessorTrigger(ADC0_BASE, 3);
    while(!ROM_ADCIntStatus(ADC0_BASE, 3, false))
    {
    }
	ROM_ADCIntClear(ADC0_BASE, 3);
    ROM_ADCSequenceDataGet(ADC0_BASE, 3, (unsigned long*) value);
    return value[0];
}
