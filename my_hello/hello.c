//*****************************************************************************
//
// hello.c - Simple hello world example.
//
// Copyright (c) 2012-2017 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.4.178 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_gpio.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "driverlib/timer.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Hello World (hello)</h1>
//!
//! A very simple ``hello world'' example.  It simply displays ``Hello World!''
//! on the UART and is a starting point for more complicated applications.
//!
//! UART0, connected to the Virtual Serial Port and running at
//! 115,200, 8-N-1, is used to display messages from this application.
//
//*****************************************************************************

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
void
ConfigureUART(void)
{
    // Enable the GPIO Peripheral used by the UART.
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Enable UART0
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    // Configure GPIO Pins for UART mode.
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTClockSourceSet(UART0_BASE, UART_CLOCK_SYSTEM);

    UARTStdioConfig(0, 115200, SysCtlClockGet());
}

#define get_fast_ticks() TimerValueGet(TIMER0_BASE,TIMER_A)

void init_timer(void)
{
    // peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0); // timer0

    // free running timer
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC_UP);
    TimerEnable(TIMER0_BASE, TIMER_A);
}

volatile uint32_t sTime, ch0T, ch1T, ch2T, ch3T, ch4T, ch5T, ch6T, ch7T;
volatile uint32_t flags = 0;

void PortDInt(void)
{
    uint32_t status=0;

    status = GPIOIntStatus(GPIO_PORTD_BASE,true);
    GPIOIntClear(GPIO_PORTD_BASE,status);

    sTime = get_fast_ticks();
    flags = 0x100;
}

void PortBInt(void)
{
    uint32_t s = GPIOIntStatus(GPIO_PORTB_BASE,false);
    uint32_t t = get_fast_ticks();
    if ((t-sTime)>1000) {
        if (s & GPIO_PIN_6) {
            ch0T = t;
            GPIOIntClear(GPIO_PORTB_BASE,GPIO_PIN_6);
            flags |= 0x01;
        }
        if (s & GPIO_PIN_7) {
            ch1T = t;
            flags |= 0x02;
        }
        if (s & GPIO_PIN_2) {
            ch2T = t;
            flags |= 0x04;
        }
        if (s & GPIO_PIN_3) {
            ch3T = t;
            flags |= 0x08;
        }
        if (s & GPIO_PIN_5) {
            ch4T = t;
            flags |= 0x10;
        }
        if (s & GPIO_PIN_0) {
            ch5T = t;
            flags |= 0x20;
        }
        /*if (s & GPIO_PIN_1) {
            ch6T = t;
            flags |= 0x40;
        }
        if (s & GPIO_PIN_4) {
            ch7T = t;
            flags |= 0x80;
        }*/
    }

    GPIOIntClear(GPIO_PORTB_BASE,s);
}

//*****************************************************************************
//
// Print "Hello World!" to the UART on the evaluation board.
//
//*****************************************************************************
int
main(void)
{
    //volatile uint32_t ui32Loop;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ); // 80MHz
    //ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN); // 16MHz
    ROM_IntMasterEnable();

    init_timer();

    // Enable the GPIO ports
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3); // on board leds

    ROM_GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_0); // rising edge detect
    ROM_GPIOPadConfigSet(GPIO_PORTD_BASE,GPIO_PIN_0,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
    GPIOIntDisable(GPIO_PORTD_BASE,GPIO_PIN_0);
    GPIOIntClear(GPIO_PORTD_BASE,GPIO_PIN_0);
    GPIOIntRegister(GPIO_PORTD_BASE,PortDInt);
    GPIOIntTypeSet(GPIO_PORTD_BASE,GPIO_PIN_0,GPIO_RISING_EDGE);
    GPIOIntEnable(GPIO_PORTD_BASE, GPIO_INT_PIN_0);

    uint8_t mask = GPIO_PIN_0|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE,mask); // falling edge ch0..7
    ROM_GPIOPadConfigSet(GPIO_PORTB_BASE,mask,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
    GPIOIntDisable(GPIO_PORTB_BASE,mask);
    GPIOIntClear(GPIO_PORTB_BASE,mask);
    GPIOIntRegister(GPIO_PORTB_BASE,PortBInt);
    GPIOIntTypeSet(GPIO_PORTB_BASE,0xEC,GPIO_FALLING_EDGE);  
    GPIOIntEnable(GPIO_PORTB_BASE,GPIO_INT_PIN_0|GPIO_INT_PIN_2|GPIO_INT_PIN_3|
                                  GPIO_INT_PIN_5|GPIO_INT_PIN_6|GPIO_INT_PIN_7);

    //
    // Initialize the UART.
    //
    ConfigureUART();

    //
    // Hello!
    //
    UARTprintf("Hello, world %d!\n",SysCtlClockGet());

    uint32_t blinkPeriod = SysCtlClockGet()/10;
    uint32_t blinkTime = 0;

    // LOOP
    while(1)
    {
        if (flags>=0x13F) {
            UARTprintf("%05X %05X %05X %05X %05X %05X\n\r",ch0T-sTime,ch1T-sTime,ch2T-sTime,
                                                           ch3T-sTime,ch4T-sTime,ch5T-sTime);
            flags = 0;
        }

        if ((get_fast_ticks()-blinkTime)>=blinkPeriod) {
            blinkTime = get_fast_ticks();
            ROM_GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1,ROM_GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_1)?0:GPIO_PIN_1);
        }
    }
}
