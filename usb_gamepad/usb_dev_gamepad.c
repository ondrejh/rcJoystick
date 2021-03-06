//*****************************************************************************
//
// usb_dev_gamepad.c - Main routines for the gamepad example.
//
// Copyright (c) 2013-2017 Texas Instruments Incorporated.  All rights reserved.
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

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_timer.h"
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usbdhidgamepad.h"
#include "usb_gamepad_structs.h"
//#include "drivers/buttons.h"
#include "utils/uartstdio.h"
#include "driverlib/timer.h"
#include "buttons.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HID Gamepad Device (usb_dev_gamepad)</h1>
//!
//! This example application turns the evaluation board into USB game pad
//! device using the Human Interface Device gamepad class.  The buttons on the
//! board are reported as buttons 1 and 2.  The X, Y, and Z coordinates are
//! reported using the ADC input on GPIO port E pins 1, 2, and 3.  The X input
//! is on PE3, the Y input is on PE2 and the Z input is on PE1.  These are
//! not connected to any real input so the values simply read whatever is on
//! the pins.  To get valid values the pins should have voltage that range
//! from VDDA(3V) to 0V.  The blue LED on PF5 is used to indicate gamepad
//! activity to the host and blinks when there is USB bus activity.
//
//*****************************************************************************

#define SW1_PRESSED (ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4)==0)
#define SW2_PRESSED (ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0)==0)

#define CH1 1
#define CH2 0
#define CH3 5

#define CH_MASK ((1<<CH1)|(1<<CH2)|(1<<CH3))

#define BIND CH3

//*****************************************************************************
//
// The HID gamepad report that is returned to the host.
//
//*****************************************************************************
static tGamepadReport sReport;

//*****************************************************************************
//
// An activity counter to slow the LED blink down to a visible rate.
//
//*****************************************************************************
static uint32_t g_ui32Updates;

//*****************************************************************************
//
// This enumeration holds the various states that the gamepad can be in during
// normal operation.
//
//*****************************************************************************
volatile enum
{
    //
    // Not yet configured.
    //
    eStateNotConfigured,

    //
    // Connected and not waiting on data to be sent.
    //
    eStateIdle,

    //
    // Suspended.
    //
    eStateSuspend,

    //
    // Connected and waiting on data to be sent out.
    //
    eStateSending
}
g_iGamepadState;

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
// Macro used to convert the 12-bit unsigned values to an eight bit signed
// value returned in the HID report.  This maps the values from the ADC that
// range from 0 to 2047 over to 127 to -128.
//
//*****************************************************************************
#define Convert8Bit(ui32Value)  ((int8_t)((0x7ff - ui32Value) >> 4))

//*****************************************************************************
//
// Handles asynchronous events from the HID gamepad driver.
//
// \param pvCBData is the event callback pointer provided during
// USBDHIDGamepadInit().  This is a pointer to our gamepad device structure
// (&g_sGamepadDevice).
// \param ui32Event identifies the event we are being called back for.
// \param ui32MsgData is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the HID gamepad driver to inform the application
// of particular asynchronous events related to operation of the gamepad HID
// device.
//
// \return Returns 0 in all cases.
//
//*****************************************************************************
uint32_t
GamepadHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgData,
               void *pvMsgData)
{
    switch (ui32Event)
    {
        //
        // The host has connected to us and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            g_iGamepadState = eStateIdle;

            //
            // Update the status.
            //
            UARTprintf("\nHost Connected...\n");

            break;
        }

        //
        // The host has disconnected from us.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_iGamepadState = eStateNotConfigured;

            //
            // Update the status.
            //
            UARTprintf("\nHost Disconnected...\n");

            break;
        }

        //
        // This event occurs every time the host acknowledges transmission
        // of a report.  It is to return to the idle state so that a new report
        // can be sent to the host.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            //
            // Enter the idle state since we finished sending something.
            //
            g_iGamepadState = eStateIdle;

            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

            break;
        }

        //
        // This event indicates that the host has suspended the USB bus.
        //
        case USB_EVENT_SUSPEND:
        {
            //
            // Go to the suspended state.
            //
            g_iGamepadState = eStateSuspend;

            //
            // Suspended.
            //
            UARTprintf("\nBus Suspended\n");

            ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

            break;
        }

        //
        // This event signals that the host has resumed signaling on the bus.
        //
        case USB_EVENT_RESUME:
        {
            //
            // Go back to the idle state.
            //
            g_iGamepadState = eStateIdle;

            //
            // Resume signaled.
            //
            UARTprintf("\nBus Resume\n");

            break;
        }

        //
        // Return the pointer to the current report.  This call is
        // rarely if ever made, but is required by the USB HID
        // specification.
        //
        case USBD_HID_EVENT_GET_REPORT:
        {
            *(void **)pvMsgData = (void *)&sReport;

            break;
        }

        //
        // We ignore all other events.
        //
        default:
        {
            break;
        }
    }

    return(0);
}

//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_SYSTEM);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, ROM_SysCtlClockGet());
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

// linux version
/*uint8_t Servo8Bit(uint32_t serv)
{
    int32_t val32 = (((int32_t)serv - 80000) * 256 / 80000);
    if (val32<0)
        return (0);
    if (val32>255)
        return (255);
    return val32;
}*/

// windows version
int8_t Servo8Bit(uint32_t serv)
{
    int32_t val32 = (((int32_t)serv - 80000) * 256 / 80000) - 128;
    if (val32<-128)
        return (-128);
    if (val32>127)
        return (127);
    return val32;
}

int8_t invert8bit(int8_t serv)
{
    return -(serv+1);
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int
main(void)
{
    uint8_t ui8ButtonsChanged, ui8Buttons;
    bool bUpdate;

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Onboard leds
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
    // Onboard buttons
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);
    ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // binding
    if (SW1_PRESSED) {
        // set binding input low
        ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, (1<<BIND));
        ROM_GPIOPinWrite(GPIO_PORTB_BASE, (1<<5), 0);
        // red led on (to show binding status)
        ROM_GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1,GPIO_PIN_1);
        // loop forever
        while (true);
    }

    // Set the clocking to run from the PLL
    //ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ); // 80MHz

    init_timer();

    /*ROM_GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_0);
    ROM_GPIOPadConfigSet(GPIO_PORTD_BASE,GPIO_PIN_0,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);*/
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE,0xFF); // falling edge ch1..3
    ROM_GPIOPadConfigSet(GPIO_PORTB_BASE,0xFF,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);

    //
    // Open UART0 and show the application name on the UART.
    //
    ConfigureUART();

    UARTprintf("\033[2JTiva C Series USB gamepad device example\n");
    UARTprintf("---------------------------------\n\n");

    //
    // Not configured initially.
    //
    g_iGamepadState = eStateNotConfigured;

    //
    // Enable the GPIO peripheral used for USB, and configure the USB
    // pins.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_AHB_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    //
    // Configure the GPIOS for the buttons.
    //
    ButtonsInit();

    //
    // Tell the user what we are up to.
    //
    UARTprintf("Configuring USB\n");

    //
    // Set the USB stack mode to Device mode.
    //
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    //
    // Pass the device information to the USB library and place the device
    // on the bus.
    //
    USBDHIDGamepadInit(0, &g_sGamepadDevice);

    //
    // Zero out the initial report.
    //
    sReport.ui8Buttons = 0;
    sReport.i8XPos = 0;
    sReport.i8YPos = 0;
    sReport.i8ZPos = 0;
    //sReport.i8RXPos = 0;

    //
    // Tell the user what we are doing and provide some basic instructions.
    //
    UARTprintf("\nWaiting For Host...\n");
    UARTprintf("\n%d\n",sizeof(sReport));

    //
    // The main loop starts here.  We begin by waiting for a host connection
    // then drop into the main gamepad handling section.  If the host
    // disconnects, we return to the top and wait for a new connection.
    //
    uint8_t pp = 0;
    uint32_t st[8];
    uint8_t sf = 0;

    while(1)
    {
        //
        // Wait here until USB device is connected to a host.
        //
        if(g_iGamepadState == eStateIdle)
        {
            uint32_t t = get_fast_ticks();
            //
            // No update by default.
            //
            bUpdate = false;

            //
            // See if the buttons updated.
            //
            ButtonsPoll(&ui8ButtonsChanged, &ui8Buttons);

            sReport.ui8Buttons = 0;

            static int8_t z = -128;
            static bool b = false;
            static uint32_t bt;
            if (z<0) {
                if (sReport.i8ZPos>0) {
                    z = 127;
                    b = true;
                    bt = t;
                }
            }
            else {
                if (sReport.i8ZPos<0) {
                    z = -128;
                    b = true;
                    bt = t;
                }
            }

            if (b) {
                if ((t-bt)>16000000)
                    b = false;
                sReport.ui8Buttons |= 0x01;
            }

            //
            // Set button 2 if right pressed.
            //
            //if(ui8Buttons & RIGHT_BUTTON) sReport.ui8Buttons |= 0x04;

            //
            // Set button 1 if left pressed.
            //
            //if(ui8Buttons & LEFT_BUTTON) sReport.ui8Buttons |= 0x02;

            sReport.ui8Buttons |= (ui8Buttons&0x7E);

            if(ui8ButtonsChanged&0x7E)
            {
                bUpdate = true;
            }

            uint8_t p = ROM_GPIOPinRead(GPIO_PORTB_BASE,CH_MASK);

            int i;

            for (i=0;i<8;i++) {
                uint8_t m = (1<<i);
                if ((p&m)!=(pp&m)) {
                    if (p&m) { // rising edge
                        st[i]=t;
                        sf&=~m;
                    }
                    else { // falling edge
                        st[i] = t-st[i];
                        sf|=m;
                    }
                }
            }
            pp = p;

            if (sf==CH_MASK) {
                sf = 0;

                //sReport.i8XPos = invert8bit(Servo8Bit(st[3]));
                sReport.i8XPos = Servo8Bit(st[CH1]);
                sReport.i8YPos = Servo8Bit(st[CH2]);
                sReport.i8ZPos = Servo8Bit(st[CH3]);
                bUpdate = true;

                for (i=0;i<8;i++)
                    UARTprintf("%02X ",st[i]);
                UARTprintf("\n\r");

                static int cnt = 0;
                cnt++;
                if (cnt>50) {
                    ROM_GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1,GPIO_PIN_1);
                    cnt=0;
                }
                else
                    ROM_GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1,0);
            }

            //
            // Send the report if there was an update.
            //
            if(bUpdate)
            {
                USBDHIDGamepadSendReport(&g_sGamepadDevice, &sReport,
                                         sizeof(sReport));

                //
                // Now sending data but protect this from an interrupt since
                // it can change in interrupt context as well.
                //
                IntMasterDisable();
                g_iGamepadState = eStateSending;
                IntMasterEnable();

                //
                // Limit the blink rate of the LED.
                //
                if(g_ui32Updates++ == 40)
                {
                    //
                    // Turn on the blue LED.
                    //
                    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);

                    //
                    // Reset the update count.
                    //
                    g_ui32Updates = 0;
                }
            }
        }
    }
}
