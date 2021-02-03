/* usbDescriptors-emerald.c -- USB descriptor structures
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <common.h>
#include <bootUsb.h>
#include <versions.h>

/*------------------------------------------------------------------------------
 * These are the strings associated with the USB descriptors.  
 *
 * NOTE: By declaring these as u16, we ensure each character uses 2 bytes
 *		 and the msb is 0
 *----------------------------------------------------------------------------*/
const u16 USB_STR_0[ 2] = {0x300 + sizeof(USB_STR_0),0x0409};

const u16 USB_STR_1[26] = {0x300 + sizeof(USB_STR_1),
						  'L','e','a','p','F','r','o','g',' ',
						  'E','n','t','e','r','p','r','i','s','e','s',
						  ' ','I','n','c','.'};

const u16 USB_STR_2[18] = {0x300 + sizeof(USB_STR_2),
					      'L','e','a','p','s','t','e','r',' ',
						  'E','x','p','l','o','r','e','r'};

/*  Currently we expect that the boot-usb code will always report a 
 *	serial number of all zeroes.
 *	It's possible that we might sometime need to read the serial number 
 *	from NOR/NAND and store it in u16 USB_STR_3[].  
 *	If the serial number in NOR/NAND is not stored as unicode, it must be
 *	converted when USB_STR_4[] is initialized.
 */
const u16 USB_STR_3[17] = {0x300 + sizeof(USB_STR_3),
					      '0','0','0','0','0','0','0','0','0','0',
						  '0','0','0','0','0', '0'};

const u16 USB_STR_n[17] = {0x300 + sizeof(USB_STR_n),
					      'B','A','D',' ','S','T','R','I','N','G',
						  ' ','I','n','d','e','x'};

const u8_ptr USB_STRING_DESC[USB_STR_NUM+2] = {
   (u8_ptr)(USB_STR_0),
   (u8_ptr)(USB_STR_1),
   (u8_ptr)(USB_STR_2),
   (u8_ptr)(USB_STR_3),
   (u8_ptr)(USB_STR_n)
};

const u8 sc_FullSpeedDeviceDescriptor[SIZEOF_DEV_DESCRIPTOR] =
{
	SIZEOF_DEV_DESCRIPTOR,			//  0 desc size
	(u8)(DESCRIPTORTYPE_DEVICE),	//  1 desc type (DEVICE)
	0x00,							//  2 USB release
	0x02,							//  3 => 2.00
	0x00,							//  4 class
	0x00,							//  5 subclass
	0x00,							//  6 protocol
	FULLSPEED_EP0_PKT_SIZE,			//  7 max pack size
	VENDORID%0x100,					//  8 vendor ID LSB
	VENDORID/0x100,					//  9 vendor ID MSB
	PRODUCTID_EMERALD%0x100,				// 10 product ID LSB
	PRODUCTID_EMERALD/0x100,				// 11 product ID MSB
	0x01,							// 12 device release LSB
	0x00,							// 13 device release MSB
	kManufacturerStringIndex, 		// 14 manufacturer string desc index
	kProductStringIndex,			// 15 product string desc index
	kSerialNumStringIndex,			// 16 serial num string desc index
	0x01							// 17 num of possible configurations
};

const u8 sc_HighSpeedDeviceDescriptor[SIZEOF_DEV_DESCRIPTOR] =
{
	SIZEOF_DEV_DESCRIPTOR,			//  0 desc size
	(u8)(DESCRIPTORTYPE_DEVICE),	//  1 desc type (DEVICE)
	0x00,							//  2 USB release
	0x02,							//  3 => 2.00
	0x00,							//  4 class
	0x00,							//  5 subclass
	0x00,							//  6 protocol
	HIGHSPEED_EP0_PKT_SIZE,			//  7 max pack size
	VENDORID%0x100,					//  8 vendor ID LSB
	VENDORID/0x100,					//  9 vendor ID MSB
	PRODUCTID_EMERALD%0x100,				// 10 product ID LSB
	PRODUCTID_EMERALD/0x100,				// 11 product ID MSB
	0x01,							// 12 device release LSB
	0x00,							// 13 device release MSB
	kManufacturerStringIndex, 		// 14 manufacturer string desc index
	kProductStringIndex,			// 15 product string desc index
	kSerialNumStringIndex,			// 16 serial num string desc index
	0x01							// 17 num of possible configurations
};

// data to be sent in response to an Inquiry command
// SBC direct access device
// Removable
const unsigned char device_info[] = {
#ifndef RAMDISK
	0, 0, 0, 0, 0x1F, 0, 0, 0,
#else	// with the RMB bit set
	0, 0x80, 0, 0x01, 0x1F, 0, 0, 0,
#endif
	/* Vendor information: "LeapFrog" */
	'L', 'e', 'a', 'p', 'f', 'r', 'o', 'g', 
	/* Product information: "LeapsterExplorer" (max 16 chars) */
	'L', 'e', 'a', 'p', 's', 't', 'e', 'r',
	'E', 'x', 'p', 'l', 'o', 'r', 'e', 'r', 
    /* Product Revision Level (#defined in versions.h) */
    INQ_PRODUCT_REVISION_LEVEL
};

const int device_info_size = sizeof(device_info);
