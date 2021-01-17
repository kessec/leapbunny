/* bootUsb.h  -- USB communication protocol
 *
 * Copyright 2009-2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef BOOTUSB_H
#define BOOTUSB_H

#define min(a,b)	(((a) <= (b)) ? (a) : (b))

typedef u8 * u8_ptr;
typedef int S32;
typedef int	bool;


enum DESCRIPTORTYPE
{
        DESCRIPTORTYPE_DEVICE        = 1,
        DESCRIPTORTYPE_CONFIGURATION = 2,
        DESCRIPTORTYPE_STRING        = 3,
        DESCRIPTORTYPE_INTERFACE     = 4,
        DESCRIPTORTYPE_ENDPOINT      = 5,
		DESCRIPTORTYPE_DEV_QUALIFIER = 6,
		DESCRIPTORTYPE_OTHER_SPEED_CONFIG = 7
};

#define FULLSPEED_MAX_EP0_PKT_SIZE	64
#define FULLSPEED_BULK_PKT_SIZE		0x40
#define HIGHSPEED_MAX_EP0_PKT_SIZE  64
#define HIGHSPEED_BULK_PKT_SIZE		0x200

enum {
        FULLSPEED_EP0_PKT_SIZE = FULLSPEED_MAX_EP0_PKT_SIZE,
        FULLSPEED_EP1_PKT_SIZE = FULLSPEED_BULK_PKT_SIZE,
        FULLSPEED_EP2_PKT_SIZE = FULLSPEED_BULK_PKT_SIZE
};

enum { 
        HIGHSPEED_EP0_PKT_SIZE = HIGHSPEED_MAX_EP0_PKT_SIZE,
        HIGHSPEED_EP1_PKT_SIZE = HIGHSPEED_BULK_PKT_SIZE,
        HIGHSPEED_EP2_PKT_SIZE = HIGHSPEED_BULK_PKT_SIZE
};

#define SIZEOF_DEV_DESCRIPTOR		18
#define SIZEOF_DEV_QUALIFIER		10
#define SIZEOF_CONFIG_DESCRIPTOR	9
#define SIZEOF_INTFCE_DESCRIPTOR	9
#define SIZEOF_ENDPT_DESCRIPTOR		7

#define TOTAL_CONFIG_LEN			(SIZEOF_CONFIG_DESCRIPTOR \
									 + SIZEOF_INTFCE_DESCRIPTOR \
									 + 2 * (SIZEOF_ENDPT_DESCRIPTOR))

#define SELF_POWERED		(1 << 6)
#define CONFIG_ATTR_DEFAULT	(1 << 7)	// usb 2.0 requires bit 7 to be set


typedef enum {
        BUS_SPEED_UNKNOWN = 0,
        BUS_SPEED_FULL = 1,
        BUS_SPEED_HIGH = 2
} BUS_SPEED;


typedef struct _STransfer {
        void*   pvBuffer;       // buffer pointer
        u32     cbBuffer;       // total buffer size
        u32     cbTransferred;	// transferred size
} STransfer, *PSTransfer;

typedef struct _USB_DEVICE_REQUEST
{
        u8      bmRequestType;
        u8      bRequest;
        u16     wValue;
        u16     wIndex;
        u16     wLength;
} tSetupPacket, USB_DEVICE_REQUEST, * PUSB_DEVICE_REQUEST;

// Masks for the fields in a setup packet's bmRequestType member
#define BM_RQST_XFER_DIR	0x80	// mask for Direction bit of bmRequestType
#define BM_RQST_XFER_OUT	  0x00
#define BM_RQST_XFER_IN 	  0x80

#define BM_RQST_TYPE		0x60	// mask for bmRequestType's 'type' field
#define BM_RQST_TYPE_STD	  0x00
#define BM_RQST_TYPE_CLASSS	  0x20
#define BM_RQST_TYPE_VENDOR	  0x40
#define BM_RQST_TYPE_RSRVED	  0x60

#define BM_RQST_RECIPIENT	0x1F	// mask for bmRequestType's 'recipient' field
#define BM_RQST_RECIP_DEVICE  0x00
#define BM_RQST_RECIP_INTFCE  0x01
#define BM_RQST_RECIP_ENDPT   0x02
#define BM_RQST_RECIP_OTHER   0x03

// Recognized values of a setup packet's bRequest member
#define GET_STATUS 			0
#define CLEAR_FEATURE 		0x01
#define SET_FEATURE 		0x03
#define SET_ADDRESS 		0x05  
#define GET_DESCRIPTOR 		0x06
#define SET_DESCRIPTOR 		0x07
#define GET_CONFIGURATION 	0x08
#define SET_CONFIGURATION 	0x09
#define GET_INTERFACE 		0x0a
#define SET_INTERFACE 		0x0b
#define SYNCH_FRAME 		0x0c   

#define FEATURE_ENDPOINT_HALT			0
#define FEATURE_DEVICE_REMOTE_WAKEUP	1
#define FEATURE_DEVICE_TEST_MODE		2

#define TEST_SELECTOR_J				0x01
#define TEST_SELECTOR_K				0x02
#define TEST_SELECTOR_SE0_NAK		0x03
#define TEST_SELECTOR_PACKET		0x04
#define TEST_SELECTOR_FORCE_ENABLE	0x05


#define DEVICE_STATUS_SELF_POWERED	1
#define DEVICE_STATUS_REMOTE_WAKEUP	2

#define ENDPOINT_STATUS_HALTED		1

#define REQUEST_SENSE_RESPONSE_LEN	18

const bool TRUE   = 1;
const bool FALSE  = 0;


#define kManufacturerStringIndex	1
#define kProductStringIndex			2
#define kSerialNumStringIndex		3
#define kConfigStringIndex			0
#define kInterfaceStringIndex		0
#define kVersionStringIndex			0
/* number of strings in the table not including 0 or n. */
#define USB_STR_NUM (3)

#define VENDORID	0x0f63	// Leap Frog VID
#define PRODUCTID	0x0016	// Emerald Firmware's product ID


#define SIZEOF_CBW	31
#define SIZEOF_CSW	13
typedef union {
	unsigned char b[SIZEOF_CSW];
	struct {
		u32 signature;
		u32 tag;
		u32	residue;
		u8	status;
	} s;
} tCSW;
#define CSW_STATUS_OK			0
#define CSW_STATUS_FAILED		1
#define CSW_STATUS_RESET_RQD	2

#define CSW_SIGNATURE	('U' + ('S' << 8) + ('B' << 16) + ('S' << 24))

/* offsets of fields in CSW */
#define OFFSET_CSW_SIGNATURE	0
#define OFFSET_CSW_TAG			4
#define OFFSET_CSW_RESIDUE		8
#define OFFSET_CSW_STATUS		12

#define CBW_SIGNATURE	('U' + ('S' << 8) + ('B' << 16) + ('C' << 24))
/* offsets of fields in CBW */
#define OFFSET_CBW_SIGNATURE		0
#define OFFSET_CBW_TAG				4
#define OFFSET_CBW_DATA_XFER_LEN	8
#define OFFSET_CBW_FLAGS			12
#define OFFSET_CBW_LUN				13
#define OFFSET_CBW_CB_LEN			14
#define OFFSET_CBW_CB				15

/* offsets from start of CBW to fields in CDB */
#define OFFSET_CBW_CB_OPCODE		OFFSET_CBW_CB

#define OFFSET_CBW_READ10_LBA		(2 + OFFSET_CBW_CB)
#define OFFSET_CBW_READ10_XFER_LEN	(7 + OFFSET_CBW_CB)

#define OFFSET_CBW_WRITE10_LBA		(2 + OFFSET_CBW_CB)
#define OFFSET_CBW_WRITE10_XFER_LEN	(7 + OFFSET_CBW_CB)

#define OFFSET_CBW_VERIFY10_LUN_BYTECHK	(1 + OFFSET_CBW_CB)


#endif	// BOOTUSB_H

