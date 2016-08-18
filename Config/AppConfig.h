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
 *  \brief Application Configuration Header File
 *
 *  This is a header file which is be used to configure some of
 *  the application's compile time options, as an alternative to
 *  specifying the compile time constants supplied through a
 *  makefile or build system.
 *
 *  For information on what each token does, refer to the
 *  \ref Sec_Options section of the application documentation.
 */

#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

	#define MAGIC_NUMBER_REPORT_ID 1
	#define MAGIC_NUMBER_REPORT_SIZE 133

	#define MAGIC_BOOT_KEY 0xDC42ACCA
	#define BOOTLOADER_SEC_SIZE_BYTES 4096
	#define FLASH_SIZE_BYTES 0x8000
	#define BOOTLOADER_START_ADDRESS ((FLASH_SIZE_BYTES - BOOTLOADER_SEC_SIZE_BYTES) >> 1)
	#define START_BOOTLOADER_REPORT_ID 0xff

	#define DEVICE_SERIALNUMBER_BYTE_LENGTH 8
	#define GET_SERIAL_NUMBER_REPORT_ID 0xfe

#define TIMESTAMP_OFFSET_FR_ID 1
#define MSG_ADMIN_ID 1
#define MSG_ADMIN_SIZE 144

#define MSG_STATUS_ID 1
#define MSG_STATUS_SIZE (8+4+4)

#define MSG_CONFIG_ID 2
#define MSG_CONFIG_SIZE (8+10*1)

#define MSG_WALL_ERROR_ID 12
#define MSG_WALL_ERROR_SIZE (8+4)

#define MSG_DROP_ERROR_ID 13
#define MSG_DROP_ERROR_SIZE 8

#define MSG_POKE_ID 14
#define MSG_POKE_SIZE (8+1)

#define MSG_PEG_ID 15
#define MSG_PEG_SIZE (8+1+1)

#define MSG_TOOL_ID 16
#define MSG_TOOL_SIZE (8+1)

#define MSG_EVENT_ID 17
#define MSG_EVENT_SIZE (8+1)

//TODO make a single file that describes every region of the eeprom in use

	#define DEVICE_NAME_REPORT_ID 2
	#define DEVICE_NAME_REPORT_SIZE 255
//TODO assert that str is no longer than 255
	#define DEFINE_PSTRING(var,str) const struct {unsigned char len; char content[sizeof(str)];} PROGMEM (var) = {sizeof(str)-1, (str)}
		

	#define UNUSED(x) (void)x

#endif
