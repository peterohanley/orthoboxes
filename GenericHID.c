/*
             LUFA Library
     Copyright (C) Dean Camera, 2015.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2015  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the GenericHID demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "GenericHID.h"

/** Buffer to hold the previously generated HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevHIDReportBuffer[255]; 

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Generic_HID_Interface =
	{
		.Config =
			{
				.InterfaceNumber              = INTERFACE_ID_GenericHID,
				.ReportINEndpoint             =
					{
						.Address              = GENERIC_IN_EPADDR,
						.Size                 = GENERIC_EPSIZE,
						.Banks                = 1,
					},
				.PrevReportINBuffer           = PrevHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevHIDReportBuffer),
			},
	};


/* the bootloader stuff */
uint32_t Boot_Key ATTR_NO_INIT;
void Bootloader_Jump_Check(void)
{
    // If the reset source was the bootloader and the key is correct, clear it and jump to the bootloader
    if ((MCUSR & (1 << WDRF)) && (Boot_Key == MAGIC_BOOT_KEY))
    {
		//overwrite boot key so that if the code that's loaded used the same location and value it will not re-bootload.
		//MCUSR &= 0 ; //~(1 << WDRF);
        Boot_Key = 0;
        ((void (*)(void))BOOTLOADER_START_ADDRESS)();
    }
}

void Jump_To_Bootloader(void)
{
    // If USB is used, detach from the bus and reset it
    //USB_Disable();
	USB_Detach();
    // Disable all interrupts
    cli();
    // Wait two seconds for the USB detachment to register on the host
    Delay_MS(1500);
    // Set the bootloader key to the magic value and force a reset
    Boot_Key = MAGIC_BOOT_KEY;
    wdt_enable(WDTO_500MS);
    for (;;);
}

/* end bootloader stuff*/

uint8_t send_raw;
uint8_t send_status;
uint8_t status_sent;

uint32_t serial_number;

#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)

/* DEVICE NAME */
DEFINE_PSTRING(device_name_string, "orthobox");

uint8_t magic_number[MAGIC_NUMBER_REPORT_SIZE] = {
	0x00, 0x0D, 0x01, 0x04, 0x05, 0x61, 0x64, 0x6D, 0x69, 0x6E, 0x01, 0x00,
	0x00, 0x00, 0x0F, 0x01, 0x02, 0x09, 0x74, 0x69, 0x6D, 0x65, 0x73, 0x74,
	0x61, 0x6D, 0x70, 0x21, 0x00, 0x11, 0x01, 0x01, 0x06, 0x73, 0x74, 0x61,
	0x74, 0x75, 0x73, 0x21, 0x11, 0x05, 0x05, 0x05, 0x05, 0x00, 0x16, 0x02,
	0x04, 0x06, 0x63, 0x6F, 0x6E, 0x66, 0x69, 0x67, 0x21, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x00, 0x11, 0x0C, 0x01, 0x0A,
	0x77, 0x61, 0x6C, 0x6C, 0x5F, 0x65, 0x72, 0x72, 0x6F, 0x72, 0x21, 0x11,
	0x00, 0x10, 0x0D, 0x01, 0x0A, 0x64, 0x72, 0x6F, 0x70, 0x5F, 0x65, 0x72,
	0x72, 0x6F, 0x72, 0x21, 0x00, 0x0B, 0x0E, 0x01, 0x04, 0x70, 0x6F, 0x6B,
	0x65, 0x21, 0x05, 0x00, 0x0B, 0x0F, 0x01, 0x03, 0x70, 0x65, 0x67, 0x21,
	0x05, 0x05, 0x00, 0x0B, 0x10, 0x01, 0x04, 0x74, 0x6F, 0x6F, 0x6C, 0x21,
	0x05
};
/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();
	buzzer_init();
	timer_init();
	box_init();
	adc_task();
	
	box_type = determine_box_type();
	if (box_type == BOX_TYPE_PEGGY) {
		peggy_hardware();
		read_peggy_thresholds();
	} else if (box_type == BOX_TYPE_POKEY) {
		pokey_init();
	}
	
	//TODO seed rng from adc read of unconnected line
	
	for (;;)
	{
		HID_Device_USBTask(&Generic_HID_Interface);
		USB_USBTask();
		adc_task();
		
		box_flash_handler();
		box_tick();
		if (box_type == BOX_TYPE_PEGGY) {
			peggy_stm_loop();
		} else if (box_type == BOX_TYPE_POKEY) {
			read_buttons(); // pokey
			pokey_loop();
			pokey_flash_handler();
		}
		
		
		//PORTB &= ~(1<<PB0); // written down to keep it down, not clear why
		
	}
}



/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
#if (ARCH == ARCH_AVR8)
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* enable and configure ADC */
	ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); /* sets clock division */ 
	ADCSRA |= (1<<ADEN); /*enable*/
	/* to select channel, write to ADMUX */

	ADMUX = (1<<REFS0) /*| (1<<REFS1)*/; /* set ref, arduino uses 1<<6 */

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
	ADCSRA |= (1<<ADSC); /* do the longer first conversion */
	
#elif (ARCH == ARCH_XMEGA)
	/* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
	XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
	XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

	/* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
	XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
	XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
#endif

	/* Hardware Initialization */
	LEDs_Init();
	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Generic_HID_Interface);

	USB_Device_EnableSOFEvents();

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Generic_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Generic_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be sent
 */
int times;
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
	uint8_t* Data        = (uint8_t*)ReportData;
	uint16_t *u16d = (uint16_t*) ReportData;
	//uint8_t  CurrLEDMask = LEDs_GetLEDs();
	//uint16_t cb_wValue;
	//const USB_Descriptor_String_t* str_addr = NULL;
	//const void** const cb_descriptoraddr = (void*) &str_addr;
	//const uint8_t* strchars;
	//uint16_t cb_strlen;
	//uint16_t strlen_rem;
	static int times;
	int ix;
	UNUSED(HIDInterfaceInfo);
	switch (ReportType) {
	case HID_REPORT_ITEM_Feature:
		if (*ReportID == MSG_CONFIG_ID) {
			//return the current timeout
			time_to_wire(timeout, Data); // TODO this is 64 bits, timeout is 32
			Data+=8;
			memcpy(Data, target_order, 10);
			*ReportSize = MSG_CONFIG_SIZE;
			return true;
		} /* else if (*ReportID == DEVICE_NAME_REPORT_ID) {
			int len = device_name_string.len;
			len = len > 0xfe ? 0xfe : len;
			Data[0] = len;
			for (int i = 0; i < len; i++) {
				Data[1+i] = pgm_read_byte(device_name_string.content + i);
			}
			*ReportSize = DEVICE_NAME_REPORT_SIZE;
			return true;
		} */
			else if (*ReportID == MAGIC_NUMBER_REPORT_ID) {
			*ReportID = MAGIC_NUMBER_REPORT_ID;
			*ReportSize = MAGIC_NUMBER_REPORT_SIZE;
			for (int i = 0; i < MAGIC_NUMBER_REPORT_SIZE; i++)
				Data[i] = magic_number[i];
			return true;
		} else if (*ReportID == 69) {
			Data[0] = get_box_type();
			*ReportSize = 1;
			return true;
		} else if (*ReportID == 70) {
			//TODO return hardware fault
		} else if (*ReportID == 71) {
			//return peg thresholds
			for (int i = 0; i < PEG_COUNT;i++)
				u16d[i] = pegs[i].thresh;
			*ReportSize = 12;
			return true;
		}
		break;
	case HID_REPORT_ITEM_In:
		//send start, task success, end messages
		//send error messages
	
		if (send_status) {
			send_status = 0;
			status_sent = 1;
			//timestamp
			time_to_wire(host_millis(), Data);
			//serial number
			uint32_to_wire(serial_number, Data+8);
			//status
			uint32_to_wire(status, Data+8+4);
			
			*ReportID = MSG_STATUS_ID;
			*ReportSize = MSG_STATUS_SIZE;
			return true;
		}
		// Eventbufs must be first because we cannot lose them and that will not happen if there is a connection
		//send event MSG_EVENT_ID
		//TODO FEATURE add output repot on this number that toggles sending these on or off
		if (0){//evtbuf.occupancy) {
			extract_event(&evtbuf, Data, MSG_EVENT_SIZE);
			*ReportID = MSG_EVENT_ID;
			*ReportSize = MSG_EVENT_SIZE;
			return true;
		}
		//send poke event on MSG_POKE_ID
		if (pokebuf.occupancy) {
			extract_poke(&pokebuf, Data, MSG_POKE_SIZE);
			*ReportID = MSG_POKE_ID;
			*ReportSize = MSG_POKE_SIZE;
			return true;
		}
		//send peg event on MSG_PEG_ID
		if (pegbuf.occupancy && status_sent) {
			extract_peg(&pegbuf, Data, MSG_PEG_SIZE);
			*ReportID = MSG_PEG_ID;
			*ReportSize = MSG_PEG_SIZE;
			return true;
		}
		//send wall_error report on MSG_WALL_ERROR_ID
		if (werrbuf.occupancy) {
			extract_wall_error(&werrbuf, Data, MSG_WALL_ERROR_SIZE);
			*ReportID = MSG_WALL_ERROR_ID;
			*ReportSize = MSG_WALL_ERROR_SIZE;
			return true;
		}
		//send drop_error report on MSG_DROP_ERROR_ID
		if (derrbuf.occupancy) {
			extract_drop_error(&derrbuf, Data, MSG_DROP_ERROR_SIZE);
			*ReportID = MSG_DROP_ERROR_ID;
			*ReportSize = MSG_DROP_ERROR_SIZE;
			return true;
		}
		//send tool state transition event on MSG_TOOL_ID
		//this records whether it's in or out, not the tool state
		// don't send until status is sent, so that first response to timestamp is correct
		if (toolbuf.occupancy && status_sent) {
			extract_tool(&toolbuf, Data, MSG_TOOL_SIZE);
			*ReportID = MSG_TOOL_ID;
			*ReportSize = MSG_TOOL_SIZE;
			return true;
		}
		
		//if send_raw then send raw values on 69
		if (send_raw) { //implicitly nothing else needs to be sent now
			//ratelimit because chrome
			times++;
			if (times < 100) return false;
			times = 0;
			
			//send all 3 tool adcs
			uint16_t* d = (uint16_t*) Data;
			d[0] = adc_values[TOOL_ERROR_LINE];
			d[1] = adc_values[TOOL_HOLDER_LINE];
			d[2] = adc_values[TOOL_CONNECTED_LINE];
			Data+=6;
			
			//send all 6+1 peggy optic adcs
			d = (uint16_t*) Data;
			ix = 0;
			d[ix++] = adc_values[0];
			d[ix++] = adc_values[1];
			d[ix++] = adc_values[4];
			d[ix++] = adc_values[5];
			d[ix++] = adc_values[6];
			d[ix++] = adc_values[7];
			d[ix++] = adc_values[DROP_ERROR_LINE];
			Data += 14;
			
			//send all 10 pokey buttons
			memcpy(Data,button_values,10);
			
			*ReportSize = (3+7)*2 + 10;
			*ReportID = 69;
			return true;
		}
	}
	return false;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
	uint8_t* Data = (uint8_t*)ReportData;
	uint16_t *u16d = (uint16_t*) ReportData;
	//uint8_t  NewLEDMask = LEDS_NO_LEDS;
	UNUSED(HIDInterfaceInfo);
	UNUSED(ReportSize);
	switch (ReportType) {
	case HID_REPORT_ITEM_Feature:
		if (ReportID == START_BOOTLOADER_REPORT_ID) {
			//check that proper code was supplied
			//FIXME lol always succeed
			//start bootloader
			LEDs_SetAllLEDs(LEDS_LED1|LEDS_LED2|LEDS_LED3);
			Jump_To_Bootloader();
		} else if (ReportID == 69) {
			set_box_type(Data[0]);
		} else if (ReportID == 2) {
			//set timestamp and task order
			TIME_t oset;
			oset = time_from_wire(Data);
			set_time_oset(oset);
			Data+=8;
			//check that these are actually 0-9
			for (int i = 0; i < 10; i++)
				if (Data[i] > 9)
					Data[i] = 0;
			// If you send the same target twice in a row, it only counts as one because the second is completed in the same press. Not a bug.
			memcpy(target_order,Data, 10);
		} else if (ReportID == 71) {
			//update peg thresholds in eeprom
			//hard 6 because it doesn't magically update if I change the number of pegs
			//PEG_COUNT so that looking for it will find that it is used here
			for (int i = 0; i < 6; i++) {
				pegs[i].thresh = u16d[i];
			}
			write_peggy_thresholds();
		}
		break;
	case HID_REPORT_ITEM_Out:
		//process commands from chrome app
		if (ReportID == TIMESTAMP_OFFSET_FR_ID) {
			TIME_t oset;
			oset = time_from_wire(Data);
			set_time_oset(oset);
			send_status = 1;
			lms_said_to_start = 1; // Also restarts the task completely.
			// TODO set a flag that controls box type auto-detection so that it doesn't run until there has been a message from a computer received. This will prevent bare boards incorrectly autoconfiguring themselves.
			// Fixup the timestamps of the tool buffer because one was being put in early. Direct call is much less space than funtion call.
			for (int i = 0; i < TOOL_BUFFER_SIZE;i++)
				toolbuf.stamps[i] += oset;
			for (int i = 0; i < PEG_BUFFER_SIZE;i++)
				pegbuf.stamps[i] += oset;
		} else if (ReportID == 69) {
			send_raw = !send_raw;
		}
		break;
	}
}

