/* usbDescriptors.c -- USB descriptor structures
 *
 * Copyright 2009-2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*------------------------------------------------------------------------------
 * NOTE: Previously this file's contents were in bootUsb.c.
 *		 They were moved to this file to make bootUsb.c smaller and
 * less cluttered, and to make it easier to find the strings and descriptors.
 * This file ought to be included in bootNorUsb.c.
 *----------------------------------------------------------------------------*/
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
	PRODUCTID%0x100,				// 10 product ID LSB
	PRODUCTID/0x100,				// 11 product ID MSB
	0x01,							// 12 device release LSB
	0x00,							// 13 device release MSB
	kManufacturerStringIndex, 		// 14 manufacturer string desc index
	kProductStringIndex,			// 15 product string desc index
	kSerialNumStringIndex,			// 16 serial num string desc index
	0x01							// 17 num of possible configurations
};

const u8 sc_FullSpeedUSBConfigData[TOTAL_CONFIG_LEN] =
{
	// Configuration Descriptor ---------------------------------------------
	SIZEOF_CONFIG_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_CONFIGURATION),	//  desc type (CONFIGURATION)
	TOTAL_CONFIG_LEN%0x100,			//  total length of data returned LSB
	TOTAL_CONFIG_LEN/0x100,			//  total length of data returned MSB
	0x01,							//  num of interfaces
	0x01,							//  value to select config (1 for now)
	kConfigStringIndex,				//  index of string desc
	CONFIG_ATTR_DEFAULT | SELF_POWERED,	//  self powered
	0,								//  max power, 0mA 

	// Interface Decriptor --------------------------------------------------
	SIZEOF_INTFCE_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_INTERFACE),	//  desc type (INTERFACE)
	0x00,							//  interface index.
	0x00,							//  value for alternate setting
	0x02,							//  # endpoints used, excluding EP0
	0x08,							// interface class (Mass Storage Class)
	0x06,							// subclass
	0x50,							// interface protocol
	kInterfaceStringIndex,			//  string index,

	// Endpoint descriptor (EP 1 Bulk IN) ----------------------------------
	SIZEOF_ENDPT_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_ENDPOINT),	//  desc type (ENDPOINT)
	0x81,							//  endpoint address: endpoint 1, IN
	0x02,							//  endpoint attributes: Bulk
	FULLSPEED_EP1_PKT_SIZE,			//  max packet size LSB
	0x00,							//  max packet size MSB
	0x00,							//  polling interval (4ms/bit=time,500ms)


	// Endpoint descriptor (EP 2 Bulk OUT) ----------------------------------
	SIZEOF_ENDPT_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_ENDPOINT),	//  desc type (ENDPOINT)
	0x02,							//  endpoint address: endpoint 2, OUT
	0x02,							//  endpoint attributes: Bulk
	FULLSPEED_EP2_PKT_SIZE,			//  max packet size LSB
	0x00,							//  max packet size MSB
	0x00							//  polling interval (4ms/bit=time,500ms)
};

const u8 sc_FullSpeedDeviceQualifierDescriptor[SIZEOF_DEV_QUALIFIER] =
{
	SIZEOF_DEV_QUALIFIER,				//  0 desc size
	(u8)(DESCRIPTORTYPE_DEV_QUALIFIER),	//  6 desc type (DEVICE)
	0x00,								//  2 USB release
	0x02,								//  3 => 2.00
	0x00,								//  4 class
	0x00,								//  5 subclass
	0x00,								//  6 protocol
	HIGHSPEED_EP0_PKT_SIZE,				//  7 max pack size
	0x01,								//  8 num of other-speed configurations
	0
};

// NOTE: This must be identical to sc_HighSpeedUSBConfigData[], 
//		 (except for the desc type)
const u8 sc_FullSpeedOtherSpeedConfigData[TOTAL_CONFIG_LEN] =
{
	// Configuration Descriptor -------------------------------------------
	SIZEOF_CONFIG_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_OTHER_SPEED_CONFIG), //  desc type
	TOTAL_CONFIG_LEN%0x100,			//  total length of data returned LSB
	TOTAL_CONFIG_LEN/0x100,			//  total length of data returned MSB
	0x01,							//  num of interfaces
	0x01,							//  value to select config (1 for now)
	kConfigStringIndex,				//  index of string desc
	CONFIG_ATTR_DEFAULT | SELF_POWERED,	//  self powered
	0,								//  max power, 0mA

	// Interface Decriptor ------------------------------------------------
	SIZEOF_INTFCE_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_INTERFACE),	//  desc type (INTERFACE)
	0x00,							//  interface index.
	0x00,							//  value for alternate setting
	0x02,							//  # endpoints used, excluding EP0
	0x08,							// interface class (Mass Storage Class)
	0x06,							// subclass
	0x50,							// interface protocol
	kInterfaceStringIndex,			//  string index,

	// Endpoint descriptor (EP 1 Bulk IN) ----------------------------------
	SIZEOF_ENDPT_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_ENDPOINT),	//  desc type (ENDPOINT)
	0x81,							//  endpoint address: endpoint 1, IN
	0x02,							//  endpoint attributes: Bulk
	HIGHSPEED_EP1_PKT_SIZE%0x100,	//  max packet size LSB
	HIGHSPEED_EP1_PKT_SIZE/0x100,	//  max packet size MSB
	0x00,							//  polling interval (4ms/bit=time,500ms)


	// Endpoint descriptor (EP 2 Bulk OUT) ---------------------------------
	SIZEOF_ENDPT_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_ENDPOINT),	//  desc type (ENDPOINT)
	0x02,							//  endpoint address: endpoint 2, OUT
	0x02,							//  endpoint attributes: Bulk
	HIGHSPEED_EP2_PKT_SIZE%0x100,	//  max packet size LSB
	HIGHSPEED_EP2_PKT_SIZE/0x100,	//  max packet size MSB
	0x01							//  NAK every microframe
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
	PRODUCTID%0x100,				// 10 product ID LSB
	PRODUCTID/0x100,				// 11 product ID MSB
	0x01,							// 12 device release LSB
	0x00,							// 13 device release MSB
	kManufacturerStringIndex, 		// 14 manufacturer string desc index
	kProductStringIndex,			// 15 product string desc index
	kSerialNumStringIndex,			// 16 serial num string desc index
	0x01							// 17 num of possible configurations
};

const u8 sc_HighSpeedUSBConfigData[TOTAL_CONFIG_LEN] =
{
	// Configuration Descriptor -------------------------------------------
	SIZEOF_CONFIG_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_CONFIGURATION),	//  desc type (CONFIGURATION)
	TOTAL_CONFIG_LEN%0x100,			//  total length of data returned LSB
	TOTAL_CONFIG_LEN/0x100,			//  total length of data returned MSB
	0x01,							//  num of interfaces
	0x01,							//  value to select config (1 for now)
	kConfigStringIndex,				//  index of string desc
	CONFIG_ATTR_DEFAULT | SELF_POWERED,	//  self powered
	0,								//  max power, 0mA

	// Interface Decriptor ------------------------------------------------
	SIZEOF_INTFCE_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_INTERFACE),	//  desc type (INTERFACE)
	0x00,							//  interface index.
	0x00,							//  value for alternate setting
	0x02,							//  # endpoints used, excluding EP0
	0x08,							// interface class (Mass Storage Class)
	0x06,							// subclass
	0x50,							// interface protocol
	kInterfaceStringIndex,			//  string index,

	// Endpoint descriptor (EP 1 Bulk IN) ----------------------------------
	SIZEOF_ENDPT_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_ENDPOINT),	//  desc type (ENDPOINT)
	0x81,							//  endpoint address: endpoint 1, IN
	0x02,							//  endpoint attributes: Bulk
	HIGHSPEED_EP1_PKT_SIZE%0x100,	//  max packet size LSB
	HIGHSPEED_EP1_PKT_SIZE/0x100,	//  max packet size MSB
	0x00,							//  polling interval (4ms/bit=time,500ms)


	// Endpoint descriptor (EP 2 Bulk OUT) ---------------------------------
	SIZEOF_ENDPT_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_ENDPOINT),	//  desc type (ENDPOINT)
	0x02,							//  endpoint address: endpoint 2, OUT
	0x02,							//  endpoint attributes: Bulk
	HIGHSPEED_EP2_PKT_SIZE%0x100,	//  max packet size LSB
	HIGHSPEED_EP2_PKT_SIZE/0x100,	//  max packet size MSB
	0x01							//  NAK every microframe
};

const u8 sc_HighSpeedDeviceQualifierDescriptor[SIZEOF_DEV_QUALIFIER] =
{
	SIZEOF_DEV_QUALIFIER,				//  0 desc size
	(u8)(DESCRIPTORTYPE_DEV_QUALIFIER),	//  6 desc type (DEVICE)
	0x00,								//  2 USB release
	0x02,								//  3 => 1.00
	0x00,								//  4 class
	0x00,								//  5 subclass
	0x00,								//  6 protocol
	FULLSPEED_EP0_PKT_SIZE,				//  7 max pack size
	0x01,								//  8 num of possible configurations
   0
};

// NOTE: This must be identical to sc_FullSpeedUSBConfigData[]
//		 (except for the desc type)
const u8 sc_HighSpeedOtherSpeedConfigData[TOTAL_CONFIG_LEN] =
{
	// Configuration Descriptor ---------------------------------------------
	SIZEOF_CONFIG_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_OTHER_SPEED_CONFIG),	//  desc type
	TOTAL_CONFIG_LEN%0x100,			//  total length of data returned LSB
	TOTAL_CONFIG_LEN/0x100,			//  total length of data returned MSB
	0x01,							//  num of interfaces
	0x01,							//  value to select config (1 for now)
	kConfigStringIndex,				//  index of string desc
	CONFIG_ATTR_DEFAULT | SELF_POWERED,	//  self powered
	0,								//  max power, 0mA 

	// Interface Decriptor --------------------------------------------------
	SIZEOF_INTFCE_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_INTERFACE),	//  desc type (INTERFACE)
	0x00,							//  interface index.
	0x00,							//  value for alternate setting
	0x02,							//  # endpoints used, excluding EP0
	0x08,							// interface class (Mass Storage Class)
	0x06,							// subclass
	0x50,							// interface protocol
	kInterfaceStringIndex,			//  string index,

	// Endpoint descriptor (EP 1 Bulk IN) ----------------------------------
	SIZEOF_ENDPT_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_ENDPOINT),	//  desc type (ENDPOINT)
	0x81,							//  endpoint address: endpoint 1, IN
	0x02,							//  endpoint attributes: Bulk
	FULLSPEED_EP1_PKT_SIZE,			//  max packet size LSB
	0x00,							//  max packet size MSB
	0x00,							//  polling interval (4ms/bit=time,500ms)


	// Endpoint descriptor (EP 2 Bulk OUT) ----------------------------------
	SIZEOF_ENDPT_DESCRIPTOR,		//  desc size
	(u8)(DESCRIPTORTYPE_ENDPOINT),	//  desc type (ENDPOINT)
	0x02,							//  endpoint address: endpoint 2, OUT
	0x02,							//  endpoint attributes: Bulk
	FULLSPEED_EP2_PKT_SIZE,			//  max packet size LSB
	0x00,							//  max packet size MSB
	0x00							//  polling interval (4ms/bit=time,500ms)
};


