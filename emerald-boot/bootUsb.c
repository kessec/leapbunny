/* bootUsb.c -- boot USB device functions.
 *
 * Copyright 2007-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*------------------------------------------------------------------------------
 * This file contains the code for the usb device driver that is part of the
 * Emerald boot module.  The driver is used only during a recovery boot.
 *
 * The driver controls and monitors the LF1000's USB Device Controller (UDC).
 * Its interaction with the UDC is modeled on several other drivers.  
 * One is Magic Eyes's MBOOT usb driver.  Its source code is in
 * pollux_mbootusb.cpp. 
 * Another is the linux usb gadget driver for the lf1000.  The source code for
 * this driver is in linux.../drivers/usb/gadget/lf1000_udc.c.  
 * A third is the linux usb gadget driver for the Samsung s3c2410 SoC's UDC,
 * which is very similar to the LF1000's UDC.  This driver's source code is in
 * linux.../drivers/usb/gadget/s3c2410_udc.c.  
 * A fourth is the WinCE driver for the Samsung s3c2443 SoC.  The source code
 * for this driver is available on google.code, in a file named sc2443pdd.cpp.
 *
 * It was necessary to refer to these other drivers because the documentation
 * for the LF1000 UDC is incomplete and apparently inconsistent.
 *
 *----------------------------------------------------------------------------
 * When the emerald boot code determines that a recovery boot is needed, it
 * calls usb_init() to configure the LF1000's UDC.  It configures endpoint 1
 * as an INbound endpoint and endpoint 2 as an OUTbound endpoint.  It enables
 * UDC interrupts triggered by events on endpoints 0, 1, and 2 and by
 * various bus conditions.
 *
 * The driver is entirely interrupt-driven, and all of its processing except
 * for usb_init() occurs while interrupts are being serviced.
 * The initial interrupt service code calls LF_IRQHandler(), which checks
 * if the interrupt was triggered by the UDC.  If it was, LF_IRQHandler() calls
 * HandleUDC_Interrupt(), which reads and analyzes several UDC status registers
 * and calls other routines to supervise further processing of various types
 * of interrupts.
 *   It calls HandleUSBBusIrq() to process various bus status or error
 * interrupts.
 *   It calls HandleEndpoint0Event() to process interrupts associated with
 * endpoint 0, HandleEndpoint1Event() to process interrupts associated with
 * endpoint 1, and HandleEndpoint2Event() to process interrupts associated with
 * endpoint 2.
 *   You can look in the routines' preambles for additional information
 * about them.
 *
 *----------------------------------------------------------------------------
 * The driver enumerates as a mass storage class device with two bulk endpoints.
 * Endpoint 1 is the bulk IN ep, and endpoint 2 is the bulk OUT ep.
 * When the boot module is built with RAMDISK defined, it contains support for
 * a 12 MB ramdisk with a FAT-16 file system.  In this case the driver 
 * enumerates as a mass storage class device with media.
 * When the boot module is built with RAMDISK undefined, it does not contain
 * support for any media or file system.  In this case the driver enumerates
 * as a mass storage class device without media.
 *
 * Normally the boot module is built with RAMDISK undefined.  In this 
 * configuration the driver enumerates like Scout, as a MSC device with no
 * media.  This is the way Middleware wants the emerald boot usb driver to
 * enumerate.
 *
 * The boot module was built with RAMDISK defined for initial testing, before
 * OmegaTerm support for Emerald Firmware was available.  The module is still
 * built with RAMDISK defined for testing of MSC compliance, using the USB20CV
 * program.
 *
 *----------------------------------------------------------------------------
 * This driver has been tested with the USB20CV compliance test program from
 * usb.org.  The driver passes the program's Chapter 9 tests.  With a special
 * hack in ProcessCBW(), when the driver is built with RAMDISK defined, it
 * passes USB20CV's Mass Storage Class test.
 *   Unfortunately success on these tests does not guarantee that the driver
 * will work successfully with every USB host, or even with every USB host
 * running Windows or Mac OS X. 
 *
 * The driver has also been tested with version 0.2.32 of OmegaTerm on
 * Windows XP and on Mac OS.  OmegaTerm is able to connect to Emerald Firmware
 * and to download a recovery kernel (Surgeon.xxx.lfp).  After the download,
 * the driver jumps to the downloaded kernel.
 *
 *----------------------------------------------------------------------------
 * Complete specifications of USB devices and mass storage class devices is
 * available from usb.org.  In addition to the USB 2.0 spec, the following
 * documents are particularly useful:
 *       Universal Serial Bus Mass Storage Class Bulk-Only Transport
 *       Universal Serial Bus Mass Storage Class Specification for Bootability
 *
 * The web has many other descriptions of USB communication protocol.  "USB in
 * a Nutshell" (www.beyondlogic.org/usbnutshell/usb-in-a-nutshell.pdf) is a
 * good introduction to USB.
 *
 * Jan Axelson's "USB Complete" and "USB Mass Storage" books are helpful
 * resources for explaining the specifications and for describing real-world
 * considerations, such as anomalies of Windows USB drivers.  Her website,
 * lvr.com, is also useful.
 *
 *----------------------------------------------------------------------------
 * Here's a very brief overview of the USB communications handled by this
 * driver.
 * When a USB host detects a device on the bus, it sends a sequence of commands
 * that elicit information about the device's characteristics.  This process
 * is called 'enumeration', and the commands and responses are sent on the
 * control endpoint, endpoint 0.
 * The information sent by the driver identifies the device as a Mass Storage
 * Class device that recognizes the SCSI transparent command set and uses the
 * bulk-only transport protocol.  The host sends SCSI commands on the bulk
 * OUT endpoint (ep2), and the device sends responses on the bulk IN endpoint
 * (ep1).
 * Commands are sent in 31-byte Command Block Wrappers (CBW).  A CBW may be
 * followed by either outbound data (on ep2), inbound data (on ep1), or no 
 * data.  After the data has been transferred, the driver sends a 13-byte
 * Command Status Wrapper (CSW) on ep1.  If the device detects an error while
 * processing a CBW, it indicates the error in the CSW.  In some cases it also
 * stalls either ep1 or ep2 or both.
 *----------------------------------------------------------------------------
 * Known problem:
 *
 * During the Error Recovery portion of USB20CV's Mass Storage Class compliance
 * test, the host sends an invalid command block wrapper (CBW).  As specified,
 * the device stalls both bulk endpoints and does not clear them until after
 * the host has sent a MSC Reset request and subsequent Clear Feature (endpoint
 * halt) requests for the two endpoints.  Then the host reads the command
 * status wrapper (CSW) and sends a Request Sense CBW to get more info about
 * the source of the problem.  When the host sends the Request Sense CBW, the
 * UDC generates an interrupt that indicates a packet has been received on the
 * bulk OUT endpoint.  When the driver reads the endpoint's BRCR register to
 * determine the number of bytes that have been received, the register indicates
 * either 0 or 1 instead of 31.  We have asked Magic Eyes why this happens
 * and how to prevent it, but they have not answered our question.
 *
 * This problem has appeared only during compliance testing, using USB20CV.  
 * It does not appear during normal communications with OmegaTerm on either
 * Windows XP and Mac OS X.  The problem is a concern primarily because other
 * versions of these operating systems might require more complete compliance
 * with the USB specification.
 *----------------------------------------------------------------------------*/

#include <board.h>
#include <common.h>
#include <base.h>
#include <gpio.h>
#include <gpio_map.h>

#include <bootUdc.h>
#include <bootUsb.h>
#include <usb_payload.h>
#include <scsi.h>
#ifdef RAMDISK
#include <ramdisk.h>
#else
#define BYTES_PER_SECTOR 	0x200
#endif

#include <buttons.h>
#include <cbf.h>
#include <debug.h>
#include <display.h>
#include <global.h>
#include <mlc.h>
#include <setup.h>
#include <string.h>
#include <screens.h>
#include <versions.h>
#include <lfp100.h>

 /* offsets from IC_BASE */
#define INTMODEL        0x08
#define INTMODEH        0x0C
#define INTMASKL        0x10
#define INTMASKH        0x14
#define PRIORDER        0x18
#define INTPENDL        0x20
#define INTPENDH        0x24

#define POWER_PORT	GPIO_PORT_C	/* power button */
#define POWER_PIN	GPIO_PIN20

#if 1	// 6apr11
#define TRUE   ((bool)1)
#define FALSE  ((bool)0)
#else
const bool TRUE   = 1;
const bool FALSE  = 0;
#endif	// 6apr11

void DisableInterrupts();
void EnableInterrupts();
void cleanup_for_linux(void);
void die(void);

/*------------------------------------------------------------------------------
 * The USB descriptor structures (device, configuration, interface, endpoint)
 * and associated strings are in usbDescriptors.c
 *----------------------------------------------------------------------------*/
//#include "usbDescriptors.c"
#include <usbDescriptors.h>

/*---------------------------
 * Static Variables
 *-------------------------*/

static enum { SCR_ATTENTION, SCR_VISIT, SCR_PROGRESS } shown_screen;
static u32 idle_time;

static STransfer Transfer[ENDPOINT_COUNT];
static u32  	 deviceAddress;
static u32  	 deviceConfig;
static u32		 deviceInterface;
static u32		 deviceSetting;

static volatile tCSW csw;

static volatile int csw_ready;
static u32			SensePtr;
static u32			longXferTag;
static u32			bytesYetToRcve;
static u32			bytesYetToStore;
static u32			bytesYetToXmit;
static u8		 *	nextRcvePtr;
static u16 			BULK_PacketSize;
static u16 			EP0_PacketSize;
static const u8  *  sc_USBDevDescriptor;
static const u8	 *  sc_USBDevQualifierData;
static const u8  *  sc_USBConfigData;
static const u8	 *  sc_USBOtherSpeedConfigData;
static u32			epBulkInStalled;
static u32			epBulkOutStalled;
static u8			epHalted[ENDPOINT_COUNT];
static u8			awaitingRecoveryReset[ENDPOINT_COUNT];

	// vbusStatus indicates the status of the VBUS line.
	// It's initialized to OFF.  It's updated as necessary when 
	// there is a VBUS_ON or VBUS_OFF interrupt.
static volatile u32			vbusStatus;
#define VBUS_STATUS_OFF	0
#define VBUS_STATUS_ON  1

static u32			sendCswStatusResetRqd;
static u32			sendCswResidue;
static u32			stallInEPAfterData;

	/* default buffer for receiving a packet */
	/* (make it big enough for either speed) */
static u8			packetBuffer[HIGHSPEED_BULK_PKT_SIZE];

static u32			downloadVerified;
static u32			kernelStatus;
#define KERNEL_NONE				0
#define KERNEL_RECEIVING		1
#define KERNEL_RECEIVED_OK		2
#define KERNEL_RECEIVED_ERROR	3


static volatile u32	msd_state;
#define MSD_STATE_AWAITING_CBW		0
#define MSD_STATE_DATA_IN			1	// ready to send data to host
#define MSD_STATE_DATA_OUT			2	// awaiting data from host

#ifdef RAMDISK
#define MAX_MODE_SELECT_DATA_LEN	64
static u8 modeSelectData[MAX_MODE_SELECT_DATA_LEN];
#endif

/*---------------------------
 * External functions
 *-------------------------*/
#ifdef RAMDISK
u8 * GetRamDiskPtr( u32 offset ); 	/* offset is in bytes */
void DiskInit( void );
int  CopyKernelBinFromRamdiskThruCbf( );
#endif

static int TransmitCSW(void);

/*---------------------------
 * Time Routines
 *-------------------------*/

static inline u32 now () {
	u32 t;
	volatile u32 *rtccntread = (u32 *)(RTC_BASE+4);
	t = *rtccntread;
	return t;
}	


/*---------------------------
 * Inline Routines
 *-------------------------*/

static inline int InAddressState() {
	return (deviceAddress != 0);
}

static inline int InDefaultState() {
	return (deviceAddress == 0);
}

static inline int InConfiguredState() {
	return (InAddressState() && (deviceConfig != 0));
}

static inline u16 GetU16le(u8 * p) 
{
	return ((u16)*p + (*(p+1) << 8));
}

static inline u16 GetU16be(u8 * p) 
{
	return (((u16)*p << 8) + *(p+1));
}

static inline u32 GetU32le(u8 * p) 
{
	return (*p + (*(p+1) << 8) + (*(p+2) << 16) + (*(p+3) << 24));
}

static inline u32 GetU32be(u8 * p) 
{
	return ((*p << 24) + (*(p+1) << 16) + (*(p+2) << 8) + *(p+3));
}

static inline u16 udc_readh(u32 reg)
{
	return UDC16(reg);
}
static inline void udc_writeh(u16 value, u32 reg)
{
	UDC16(reg) = value;
}

static inline u16
ReadReg(u32 dwOffset)
{
	volatile u16 *pbReg = CTRLR_BASE_REG_ADDR(dwOffset);
	return *pbReg;
}

static inline void
WriteReg(u32 dwOffset, u16 bValue)
{
	volatile u16 *pbReg = CTRLR_BASE_REG_ADDR(dwOffset);
	*pbReg = bValue;
}


static inline u16
ClearRegBits(u32  dwOffset, u16  dwMask)
{
	volatile u16 *pbReg = CTRLR_BASE_REG_ADDR(dwOffset);
	u16 bValue = *pbReg;

	bValue &= ~dwMask;
	*pbReg  = bValue;

	return bValue;
}

static inline u16
SetRegBits(u32  dwOffset, u16  dwMask)
{
	volatile u16 *pbReg = CTRLR_BASE_REG_ADDR(dwOffset);
	u16 bValue = *pbReg;

	bValue |= dwMask;
	*pbReg  = bValue;

	return bValue;
}

static inline u16
ClearIndexedRegBits(u32 dwEndpoint,
					u32 dwOffset,
					u16 dwMask)
{
	// Write the EP number to the index reg
	WriteReg(UDC_EPINDEX, (u16) dwEndpoint);	

	return ClearRegBits(dwOffset, dwMask);
}

static inline u16
SetIndexedRegBits(	u32 dwEndpoint,
					u32 dwOffset,
					u16 dwMask)
{
	// Write the EP number to the index reg
	WriteReg(UDC_EPINDEX, (u16) dwEndpoint);	

	return SetRegBits(dwOffset, dwMask);
}



static inline u16
ReadIndexedReg(u32 dwEndpoint, u32 regOffset)
{
	// Write the EP number to the index reg
	WriteReg(UDC_EPINDEX, (u16) dwEndpoint);
	// Now Read the Register associated with this Endpoint for a given offset
	u16 bValue = ReadReg(regOffset);
	return bValue;
}

static inline void
WriteIndexedReg(u32 dwEndpoint,
				u32 regOffset,
				u16 bValue)
{  
	// Write the EP number to the index reg
	WriteReg(UDC_EPINDEX, (u16) dwEndpoint);
	// Now Write the Register associated with this Endpoint for a given offset
	WriteReg(regOffset, bValue);
}

static inline void 
SetAddress(u8 bAddress)
{
	WriteReg(FUNC_ADDR_REG_OFFSET, bAddress);
}




static inline u16
EpToIrqStatBit(u32 dwEndpoint)
{
	return (1 << (u8)dwEndpoint);
}

static inline void
EnableEndpointInterrupt(u32 dwEndpoint)
{
	u16 bIrqEnBit = EpToIrqStatBit(dwEndpoint);

	WriteReg(EP_INT_EN_REG_OFFSET, bIrqEnBit | ReadReg(EP_INT_EN_REG_OFFSET));
}

#if 0	// 28may09	Removed because not called; since EnableEndpointInterrupt()
		//			is called from several places, should this be called from
		//			someplace?
static inline void
DisableEndpointInterrupt(u32 dwEndpoint)
{
	u16 bIrqEnBit = EpToIrqStatBit(dwEndpoint);

	WriteReg(EP_INT_EN_REG_OFFSET, ~bIrqEnBit & ReadReg(EP_INT_EN_REG_OFFSET));
}
#endif

/*------------------------------------------------------------------------------ 
 * Writing a '1' to a bit in the Endpoint Interrupt Register clears the
 * interrupt for the endpoint associated with the bit.
 *----------------------------------------------------------------------------*/
static inline void
ClearEndpointInterrupt(u32 dwEndpoint)
{
	WriteReg(EP_INT_REG_OFFSET, EpToIrqStatBit(dwEndpoint));
}


/** ----------------------------------------------------------------------------
 * StallEndpoint - stall the specified endpoint
 *
 * @Parameters:	dwEndpoint is the index of the endpoint to stall (0, 1, or 2)
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine sets the ESS bit of the endpoint's control 
 *				register and sets the endpoint's entry in epHalted[] to 1.
 *	If dwEndpoint is DNW_IN_ENDPOINT or DNW_OUT_ENDPOINT, the routine also
 *	sets epBulkInStalled or epBulkOutStalled, respectively.
 *
 *	If dwEndpoint is 0, the routine waits for the SHT bit in ep0's status
 *	register to be set, indicating that a STALL PID has been sent.  Then it 
 *	clears the ESS bit in ep0's control register and the SHT bit in ep0's
 *	status register.
 *	NOTE: Magic Eyes said this is the way to stall ep0 during the status stage
 *		  of a transfer that has no data stage; do this instead of clearing
 *	the RSR bit in the ep0 status register.
 *	This also seems to be an effective way of stalling ep0 during other
 *	transfers, though in some of those cases it is necessary first to 
 *	clear the RSR bit in the ep0 status register.
 *----------------------------------------------------------------------------*/
static void
StallEndpoint(u32 dwEndpoint)
{
	idb_putchar('X');
	idb_putchar('0' + (dwEndpoint & 0x0F));
	if (dwEndpoint == 0) {
#if 1   // 5feb10
        u32 i = 0;
		SetRegBits(EP0_CTL_REG_OFFSET, EP0_CTL_ESS_BIT);
		while (   !(ReadReg(EP0_STS_REG_OFFSET) & EP0_STS_SHT_BIT)
               && (i++ < 0x00400000) )  // don't wait forever
			;
#else
		SetRegBits(EP0_CTL_REG_OFFSET, EP0_CTL_ESS_BIT);
		while(!(ReadReg(EP0_STS_REG_OFFSET) & EP0_STS_SHT_BIT))
			;
#endif  // 5feb10
		// this code was copied from ClearEndpointStall(0);
		WriteReg(EP0_STS_REG_OFFSET, EP0_STS_SHT_BIT);
		ClearRegBits(EP0_CTL_REG_OFFSET, EP0_CTL_ESS_BIT);
		WriteReg(EP0_STS_REG_OFFSET, EP0_STS_SHT_BIT);
	}
	else{
		SetIndexedRegBits(dwEndpoint, GEP_CTL_REG_OFFSET, GEP_CTL_ESS_BIT);
		if (dwEndpoint == DNW_IN_ENDPOINT) {
			epBulkInStalled = 1;
			epHalted[DNW_IN_ENDPOINT] = 1;
		}
		else if (dwEndpoint == DNW_OUT_ENDPOINT) {
			epBulkOutStalled = 1;
			epHalted[DNW_OUT_ENDPOINT] = 1;
		}
	}
}

/** ----------------------------------------------------------------------------
 * ClearEndpointStall - clear a stall on the specified endpoint
 *
 * @Parameters:	dwEndpoint is the index of the endpoint to unstall (0, 1, or 2)
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine sets and then clears the ESS bit of the endpoint's
 *				control register.  Then it writes '1' to the 'stalled' bit
 *	of the endpoint's status register and zeroes the endpoint's entry in 
 *	epHalted[].
 *
 *	For endpoint 0, the 'stalled' bit of the status register is the SHT bit.
 *	For endpoints 1 and 2, the 'stalled' bit of the status register is the
 *	FSC bit.
 *
 *	If dwEndpoint is DNW_IN_ENDPOINT or DNW_OUT_ENDPOINT, the routine also
 *	zeroes epBulkInStalled or epBulkOutStalled, respectively.
 *----------------------------------------------------------------------------*/
static void 
ClearEndpointStall(u32 dwEndpoint)
{
	if (dwEndpoint == 0) {
		WriteReg(EP0_STS_REG_OFFSET, EP0_STS_SHT_BIT);
		ClearRegBits(EP0_CTL_REG_OFFSET, EP0_CTL_ESS_BIT);
		WriteReg(EP0_STS_REG_OFFSET, EP0_STS_SHT_BIT);
		epHalted[0] = 0;
	}
	else{
		// 7jul09	Mimic the code in the Samsung s3c2443 device driver
		//			which I found on google.code
		//	Set   the ESS bit in the ep's Control register
		//	Clear the ESS bit in the ep's Control register
		//	Clear the FSC bit in the ep's Status  register by writing 1 to it.

		SetIndexedRegBits(dwEndpoint, GEP_CTL_REG_OFFSET, GEP_CTL_ESS_BIT);
		ClearIndexedRegBits(dwEndpoint, GEP_CTL_REG_OFFSET, GEP_CTL_ESS_BIT);
		WriteIndexedReg(dwEndpoint, GEP_STS_REG_OFFSET, GEP_STS_FSC_BIT);

		if (dwEndpoint == DNW_IN_ENDPOINT) {
			epBulkInStalled			  = 0;
			epHalted[DNW_IN_ENDPOINT] = 0;
		}
		else if (dwEndpoint == DNW_OUT_ENDPOINT) {
#if 0	// 15jul09  1700 Removed, retaining in case we want to see BRCR
			u16 inFifo;
				// if the fifo isn't empty, flush it.
			inFifo = ReadIndexedReg(DNW_OUT_ENDPOINT, GEP_BRCR_REG_OFFSET);
			if (inFifo) {
#if 0	// 15jul09 1610	// This did not help, so removed 1625
				SetIndexedRegBits(DNW_OUT_ENDPOINT, GEP_CTL_REG_OFFSET, 
													GEP_CTL_FLUSH_BIT);
#endif	// 15jul09 1610
				idb_putchar('F');
				idb_16(inFifo);
				idb_putchar('}');
			}
#endif	// 15jul09  1700
			epBulkOutStalled		   = 0;
			epHalted[DNW_OUT_ENDPOINT] = 0;
		}
	}
}

/** ----------------------------------------------------------------------------
 * ClearEndpointDataPid - sets the data PID for the specified endpoint to 0
 *
 * @Parameters:	dwEndpoint is the index of the endpoint
 *
 * @Returns:	(void).
 *
 * @Details: 	When dwEndpoint is 0, this routine does nothing.
 *				When dwEndpoint is 1 or 2, this routine sets the CDP bit of
 *	the endpoint's control register.
 *----------------------------------------------------------------------------*/
static void ClearEndpointDataPid(u32 dwEndpoint)
{
	if (dwEndpoint == 0) {
		//WriteReg(EP0_STS_REG_OFFSET, EP0_STS_SHT_BIT);
		//ClearRegBits(EP0_CTL_REG_OFFSET, EP0_CTL_ESS_BIT);
		//WriteReg(EP0_STS_REG_OFFSET, EP0_STS_SHT_BIT);
	}
	else{
		SetIndexedRegBits(dwEndpoint, GEP_CTL_REG_OFFSET, GEP_CTL_CDP_BIT);
	}
}

/** ----------------------------------------------------------------------------
 * JumpToKernel - start execution of the downloaded recovery kernel
 *
 * @Parameters:	(void)
 *
 * @Returns:	The routine returns only if cbf_get_jump_address() does not
 *				return the entry address of the downloaded recovery kernel.
 *
 * @Details: 	The routine calls cbf_get_jump_address() to get the entry
 *				address of the downloaded recovery kernel.  If there is no 
 *	entry address, this routine calls show_attention_needed() and returns.
 *
 *	If it obtained an entry address for the recovery kernel, this routine 
 *	calls calc_PARAMS_ADDRESS() to get the address of a buffer where the
 *	kernel's command line ought to be stored.  
 *	Then the routine disables the LF1000's USB controller, disables interrupts,
 *	and calls cleanup_for_linux(), which turns off and flushes the caches.
 *	The routine calls build_params(), which initializes the parameter structure
 *	that will be passed to the kernel.
 *	Finally the routine makes a subroutine call to the kernel entry address,
 *	passing it the information it needs.  
 *	The routine does not expect the kernel to return.
 *----------------------------------------------------------------------------*/
void 	build_params(char *cmdline, struct tag *params);
u32 	calc_PARAMS_ADDRESS();

static void JumpToKernel() 
{
	void * jumpAddr;
	global_var * gptr = get_global();

	if (0 == cbf_get_jump_address( &jumpAddr) ) {

		struct tag * pParams = (struct tag *)calc_PARAMS_ADDRESS();

		idb_puts (gptr->cmdline_buffer); idb_puts ("\n");
		/* disable the USB controller */
		BIT_SET(REG16(UDC_BASE+UDC_PCR), PCE);

		DisableInterrupts();
		cleanup_for_linux();
		build_params(gptr->cmdline_buffer, pParams);

            // Casting jumpAddr to u32 before casting it to a function pointer
            // prevents a warning from the compiler
		((void(*)(int r0, int r1, unsigned int r2))((u32)jumpAddr))
			(0, MACH_TYPE, (unsigned int)pParams);

            // We do not expect the kernel to return.  If it does, the unit
            // is in an unexpected and unknown condition.  Just power off.
        die();  
	}
	else
	{
		show_attention_needed();
	}
}


/* Accessor functions for the kernelStatus variable */
static inline void SetKernelNone() {
	kernelStatus = KERNEL_NONE;
}

static inline void SetKernelReceiving() {
	kernelStatus = KERNEL_RECEIVING;
}

static inline void SetKernelReceivedOk() {
	kernelStatus = KERNEL_RECEIVED_OK;
}

static inline void SetKernelReceivedError() {
	kernelStatus = KERNEL_RECEIVED_ERROR;
}

static inline int ReceivingRecoveryKernel() {
	return (kernelStatus == KERNEL_RECEIVING);
}

static inline int ReceivedKernelOk() {
	return (kernelStatus == KERNEL_RECEIVED_OK);
}

static inline int ReceivedKernelError() {
	return (kernelStatus == KERNEL_RECEIVED_ERROR);
}



/** ----------------------------------------------------------------------------
 * ConfigureBulkEndpoint - configure one of the bulk endpoints
 *
 * @Parameters:	ep - index of the endpoint (1 or 2)
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine configures a bulk endpoint by doing the following:
 *		enable dual fifo mode, flush the fifo, and disable Interrupt mode.
 *		specify the max packet size
 *		make sure it isn't stalled
 *		clear all of the ep's interrupt flags:
 *			Fifo underflow (which is used only for Isochronous mode)
 *			Fifo overflow  (which is used only for Isochronous mode)
 *			OUT start DMA
 *			DMA Total Count Zero
 *			Short packet received
 *			Fifo flushed
 *			Functional Stall condition
 *			Transmit Packet success
 *		clear the endpoint interrupt (flag in endpoint interrupt reg)
 *		enable the endpoint interrupt (set bit in ep int. enable reg)
 *----------------------------------------------------------------------------*/
static void ConfigureBulkEndpoint(u32 ep)
{
	SetIndexedRegBits(ep, GEP_CTL_REG_OFFSET, 
						(GEP_CTL_FLUSH_BIT | GEP_CTL_DUEN_BIT));
	ClearIndexedRegBits(ep, GEP_CTL_REG_OFFSET, GEP_CTL_IME_BIT);

	WriteIndexedReg(ep, GEP_MPR_REG_OFFSET, BULK_PacketSize);
	ClearEndpointStall(ep);

	WriteIndexedReg(ep, GEP_STS_REG_OFFSET, 
						GEP_STS_FUDR_BIT
						| GEP_STS_FOVF_BIT
						| GEP_STS_OSD_BIT
						| GEP_STS_DTCZ_BIT
						| GEP_STS_SPT_BIT
						| GEP_STS_FFS_BIT
						| GEP_STS_FSC_BIT
						| GEP_STS_TPS_BIT); 
	ClearEndpointInterrupt(ep);
	EnableEndpointInterrupt(ep);
}

/** ----------------------------------------------------------------------------
 * reset_vars - initialize variables to their power-on / reset states
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine initializes many variables to their power-on /
 *				reset states.
 *----------------------------------------------------------------------------*/
static void reset_vars() {
	deviceAddress				= 0;
	deviceConfig				= 0;
	deviceInterface				= 0;
	deviceSetting				= 0;
	bytesYetToXmit				= 0;
	bytesYetToRcve				= 0;
	bytesYetToStore				= 0;
	nextRcvePtr					= NULL;

	csw_ready					= 0;
	longXferTag					= 0;
	SensePtr					= senseOk;
	sendCswStatusResetRqd		= 0;
	sendCswResidue				= 0;
	stallInEPAfterData			= 0;

	epBulkInStalled				= 0;
	epBulkOutStalled			= 0;
	epHalted[0]					= 0;
	epHalted[DNW_IN_ENDPOINT]  	= 0;
	epHalted[DNW_OUT_ENDPOINT] 	= 0;

	awaitingRecoveryReset[0]				= 0;
	awaitingRecoveryReset[DNW_IN_ENDPOINT] 	= 0;
	awaitingRecoveryReset[DNW_OUT_ENDPOINT]	= 0;

	msd_state					= MSD_STATE_AWAITING_CBW;
}

/** ----------------------------------------------------------------------------
 * usb_init - initialize variables and configure the LF1000's UDC
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine initializes variables and configures the LF1000's
 *				USB device controller (UDC).  It ought to be called before UDC
 *	interrupts are enabled.
 *
 *	NOTE: This routine's processing is modelled after both our Linux usb gadget
 *		  lf1000 driver code and MagicEyes's pollux_mbootusb code.
 *
 *----------------------------------------------------------------------------*/
void usb_init() {
	u16 tmp;

	reset_vars();

		/* NOTE: The following variables are not initialized in reset_vars().
		 *		 Therefore they retain their values when the host resets the
		 * the USB. 
		 * When the USB cable is detached and then re-attached, vbusStatus
		 * is updated as the state of VBUS changes.  After re-attachment the
		 * speed of the connection is again detected, a Speed Detection End
		 * (SDE) interrupt is generated, and the interrupt handler sets
		 * BULK_PacketSize and EP0_PacketSize to values that are appropriate
		 * for the detected speed.
		 *
		 * The variable kernelStatus is updated at the start of a kernel
		 * download (i.e., on receipt of a scsi Write10 command with logical
		 * block address of 0) and on completion of a kernel download.
		 * The variable downloadVerified is updated when a scsi Verify10
		 * command is received after successful download of a kernel.
		 * If the USB cable is detached after a kernel has been downloaded
		 * successfully and before a Verify10 command has been processed,
		 * then after the cable is re-attached, the host need not download
		 * the kernel again.  It can simply send the Verify10 command.
		 */
	BULK_PacketSize				= FULLSPEED_BULK_PKT_SIZE;
	EP0_PacketSize				= FULLSPEED_MAX_EP0_PKT_SIZE;
	vbusStatus					= VBUS_STATUS_OFF;
	downloadVerified			= 0;
	SetKernelNone();

	/* We know coming in that the Attention screen was put up by 
	 * show_attention_needed_and_wait in main.c */
	shown_screen = SCR_ATTENTION;

	idb_putchar('Z');
#ifdef RAMDISK
	DiskInit();
#endif

		/*	First select the external clock as the USB Clock source,
		 *	with a divider of 1 */
	udc_writeh((0<<UDC_CLKDIV)|(0x3<<UDC_CLKSRCSEL), UDC_CLKGEN);

		/* Enable the clock: PCLK always on, enable clock generation,
		 *					 USBD Clock always enabled */
	udc_writeh((1<<UDC_PCLKMODE)|(1<<UDC_CLKGENENB)|(3<<UDC_CLKENB),
		   		UDC_CLKEN);

		/* disable phy block */
		/* NOTE: this was already done at the beginning of main() */
	tmp  = udc_readh(UDC_PCR);
	tmp |= 1<<PCE;
	udc_writeh(tmp, UDC_PCR);

	WriteIndexedReg(0, GEP_MPR_REG_OFFSET, 64);	// ep0 max packet size is 64

		/* Set the OUTbound endpoint direction and configure it */
		/* (direction bit: 1 for TX, 0 for RX; we want RX for outbound) */
	ClearRegBits(EP_DIR_REG_OFFSET, EpToIrqStatBit(DNW_OUT_ENDPOINT));
	ConfigureBulkEndpoint(DNW_OUT_ENDPOINT);

		/* Set the INbound endpoint direction and configure it */
	SetRegBits(EP_DIR_REG_OFFSET, EpToIrqStatBit(DNW_IN_ENDPOINT));
	ConfigureBulkEndpoint(DNW_IN_ENDPOINT);

		// Enable Device interrupts (in the UDC System Control register)
	SetRegBits(USB_INT_EN_REG_OFFSET,  USB_INT_EN_HRESE_BIT		// reset
										| USB_INT_EN_RRDE_BIT	// byte reverse
										| USB_INT_EN_SPDEN_BIT	// speed detect
										| USB_INT_EN_EIE_BIT 	// error int en
										| USB_INT_EN_VBUSONEN_BIT
										| USB_INT_EN_VBUSOFFEN_BIT);
		// enable external VBUS
	SetRegBits(UDC_USERTEST, 1 << VBUSENB);

		// Clear ep0 interrupt and enable the ep0 interrupt
	ClearEndpointInterrupt(0);
	EnableEndpointInterrupt(0);

		// Disable PHY Control (to turn on the PHY block)
	ClearRegBits(UDC_PCR, 1 << PCE);		// phy block on
}

//------------------------------------------------------------------------------
// This routine checks if the RSR bit in ep0's status register is set.  If it
// is set, this routine writes '1' to it, in order to clear it.
// Apparently this action causes the USB IP to send an ACK to the host.
// Usually this routine is called after an ep0 interrupt with the RSR bit set.
// Here's the exception:
//   If a Setup packet's direction bit indicates the host does not expect
// the transfer to contain a Data Stage, and if the Setup packet is invalid
// or unrecognized, the driver stalls ep0 instead of calling this routine.
//------------------------------------------------------------------------------
static void 
SendControlStatusHandshake()
{
	if(ReadReg(EP0_STS_REG_OFFSET) & EP0_STS_RSR_BIT)
		WriteReg(EP0_STS_REG_OFFSET, EP0_STS_RSR_BIT);
}



/** ----------------------------------------------------------------------------
 * HandleBulkInTx - write a packet into the bulk IN endpoint's fifo
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine writes data described by the bulk IN endpoint's
 *				Transfer[] structure into the endpoint's fifo.  
 *	It stores the packet's size (in bytes) in the endpoint's BWCR register.
 *	It updates the endpoint's Transfer[] structure to indicate the number of
 *	bytes that were stored in the fifo.
 *
 *	The Transfer[] structure contains the address of a buffer containing
 *	data that ought to be sent to the host.  It also contains the number of
 *	bytes in the buffer (cbBuffer) and the number of those bytes that have
 *	already been transferred to the host (cbTransferred).
 *
 *	This routine calculates the address of the first byte it ought to transfer.
 *	It also calculates the number of bytes it ought to transfer.  This number
 *	must be no more than BULK_PacketSize.
 *
 *	Data is written to the endpoint's fifo in 16-bit words, so the routine
 *	calculates the number of words it ought to write to the fifo.  If the 
 *	number of bytes to be transferred is even, the number of words is just
 *	half the number of bytes.  If the number of bytes is odd, the number of
 *	words is one more than half the number of bytes.
 *
 *	If the first byte to be transferred is stored at an even address, this 
 *	routine fetches the data a word at a time.  If it is stored at an odd
 *	address, the routine fetches the data as bytes, joins two successive
 *	bytes into a word, and writes the word to the fifo.
 *
 *	If it needs to transfer an odd number of bytes, the routine fetches the
 *	final byte, puts it into the less significant byte of a word whose more
 *	significant byte is zero, and writes the word to the fifo.
 *
 *  Before returning, the routine calculates 'bytesYetToXmit'.
 *----------------------------------------------------------------------------*/
static void
HandleBulkInTx()
{
	volatile u16 *pulFifoReg = CTRLR_BASE_REG_ADDR(EP1_FIFO_REG_OFFSET);
	u16 temp;
	PSTransfer	  pTransfer = &Transfer[DNW_IN_ENDPOINT];
	union{
		u8 * pbBuffer;
		u16* pwBuffer;
	}DataBuffer;
	DataBuffer.pbBuffer = (u8*) pTransfer->pvBuffer 
								+ pTransfer->cbTransferred;
	u32 cbBuffer = pTransfer->cbBuffer - pTransfer->cbTransferred;

	u32 cwWritten = 0;

	// Min of input byte count and supported size
	u32 cbToWrite = min(cbBuffer, BULK_PacketSize);
	u32 cwToWrite = cbToWrite / sizeof(u16);

	idb_puts("BI("); idb_byte((u8)cbToWrite);

	WriteIndexedReg(DNW_IN_ENDPOINT, GEP_BWCR_REG_OFFSET, (u16)cbToWrite);

	idb_putchar('{'); 
	idb_byte((u8)ReadIndexedReg(DNW_IN_ENDPOINT, GEP_BWCR_REG_OFFSET));

		// if the source address is odd we must fetch byte-by-byte
	if (1 & ((u32)DataBuffer.pbBuffer)) 
	{
		for (cwWritten = 0; cwWritten < cwToWrite; cwWritten++) {
			temp  = *DataBuffer.pbBuffer++;
			temp |= ((u16)(*DataBuffer.pbBuffer++) << 8);
			*pulFifoReg = temp;
		}
	}
	else {	// else the source address is even, so we can access as u16s
		for (cwWritten = 0; cwWritten < cwToWrite; cwWritten++) {
			*pulFifoReg = *DataBuffer.pwBuffer++;
		}
	}
		// if there's one more byte to transfer, fetch it and write it
		// to the fifo (as the lsb of a word)
	if ((cbToWrite % sizeof(u16)) > 0)
	{
		*pulFifoReg = (u16)(*DataBuffer.pbBuffer);
	}

	// Update the Transfered Count
	pTransfer->cbTransferred += cbToWrite;
	bytesYetToXmit = cbBuffer - cbToWrite;

	idb_putchar('['); 
	idb_byte((u8)ReadIndexedReg(DNW_IN_ENDPOINT, GEP_BWCR_REG_OFFSET));
	idb_putchar('\n'); 
}

/** ----------------------------------------------------------------------------
 * EP0Send - copy data into the endpoint 0 fifo for transmission to the host
 *
 * @Parameters:	(void)
 *
 * @Returns:	(void)
 *
 * @Details: 	This routine supervises processing that copies data into
 *				ep0's Inbound fifo for transmission to the USB Host.
 *
 *	The routine looks in Transfer[0] for the location and number of bytes
 *	to be written to the fifo.  The routine ensures that it doesn't transfer
 *	more bytes than ep0's maximum packet size.  It stores the number of bytes
 *	in ep0's Byte Write Count Register.  It also updates Transfer[0]'s entry
 *	that contains the number of bytes that have been transmitted.
 *----------------------------------------------------------------------------*/
static void
EP0Send(void)
{
	u16 temp;
	PSTransfer pTransfer = &Transfer[0];
	union{
		u8*		pbBuffer;
		u16*	pwBuffer;
	}DataBuffer;
	DataBuffer.pbBuffer = (u8*)pTransfer->pvBuffer + pTransfer->cbTransferred;
	u32 cbBuffer = pTransfer->cbBuffer - pTransfer->cbTransferred;

	volatile u16 *pulFifoReg = CTRLR_BASE_REG_ADDR(EP0_FIFO_REG_OFFSET);
	u32 cwWritten = 0;
	// Min of input byte count and supported size
	u32 cbToWrite = min(cbBuffer, EP0_PacketSize);

	u32 cwToWrite = cbToWrite / sizeof(u16);

	WriteIndexedReg(0, GEP_MPR_REG_OFFSET, EP0_PacketSize);
	WriteIndexedReg(0, GEP_BWCR_REG_OFFSET, (u16)cbToWrite);

		// if the source address is odd we must fetch byte-by-byte
	if (1 & ((u32)DataBuffer.pbBuffer)) 
	{									
		for (cwWritten = 0; cwWritten < cwToWrite; cwWritten++) {
			temp  = *DataBuffer.pbBuffer++;
			temp |= ((u16)(*DataBuffer.pbBuffer++) << 8);
			*pulFifoReg = temp;
		}
	}
	else {	// else the source address is even, so we can access as u16s
		for (cwWritten = 0; cwWritten < cwToWrite; cwWritten++) {
			*pulFifoReg = *DataBuffer.pwBuffer++;
		}
	}
		// if there's one more byte to transfer, fetch it and write it
		// to the fifo (as the lsb of a word)
	if((cbToWrite % sizeof(u16)) > 0)	
	{
		*pulFifoReg = (u16)(*DataBuffer.pbBuffer);
	}
	pTransfer->cbTransferred += cbToWrite;
}

/** ----------------------------------------------------------------------------
 * CheckEP0Send - check if there's something to send on ep0 and, if so, send it
 *
 * @Parameters:	(void)
 *
 * @Returns:	0 if there was nothing to send
 *				1 if there was something to send, and it was sent
 *
 * @Details: 	This routine checks if Transfer[0]'s cbBuffer and cbTransferred
 *				members are equal, indicating there is nothing to send on ep0.
 *	If they are equal, this routine returns 0.
 *	If they are not equal, this routine calls EP0Send() and returns 1.
 *----------------------------------------------------------------------------*/
static int CheckEP0Send(void) {
	int status;

	PSTransfer pTransfer = &Transfer[0];
	if (pTransfer->cbTransferred == pTransfer->cbBuffer) {
		status = 0;
	}
	else {
		status = 1;
		EP0Send();
	}
	return status;
}

/** ----------------------------------------------------------------------------
 * EP0GetCommand - read bytes from the ep0 Rx fifo and store them in a buffer
 *
 * @Parameters:	pbuffer - the address of the buffer
 *
 * @Returns:	TRUE if the expected # of bytes were read from the fifo and
 *					 stored in the buffer;
 *				FALSE otherwise.
 *
 * @Details: 	This routine reads the ep0 status register and the ep0 Byte Read
 *				Count register.  Because we're using a 16-bit interface, the
 *	routine doubles the value read from the BRCR (which is # of words) to 
 *	calculate the number of bytes.  If the LWO bit of the ep0 status register
 *	is set, indicating that the high order byte of the fifo's final word is
 *	not valid, the routine decrements the number of bytes.  Then the routine
 *	divides the number of bytes by two to obtain the number of words to read
 *	from the fifo.  After it has read that many words, the routine checks if
 *	the number of bytes is odd.  If it is, it reads from the fifo once more
 *	and stores the less significant byte in the buffer.
 *
 *	If a packet had been received into the fifo and if the expected number of 
 *	bytes (8) were read from the fifo, this function returns TRUE.  Otherwise
 *	it returns FALSE.
 *
 *	NOTE: 	This function assumes the buffer starts at an even address
 *			and is large enough to contain all the bytes read from the
 *			ep0 fifo.  I think the max number of bytes is the endpoint's
 *			max packet size, which is 64.
 *
 * NOTE:
 *		This routine does not clear the RSR bit of ep0's status register.
 * The routine's caller ought to do that when appropriate.  Sometimes the
 * caller ought just to call StallEndpoint(0) instead.
 *----------------------------------------------------------------------------*/
static bool
EP0GetCommand(void* pbuffer)
{
	u16 bEP0IrqStatus = ReadReg(EP0_STS_REG_OFFSET);
	u32 cbFifo		  = sizeof(u16) * ReadIndexedReg(0, GEP_BRCR_REG_OFFSET);

	if (cbFifo != sizeof(USB_DEVICE_REQUEST)) {
		idb_putchar('G');
		idb_putchar('C');
		idb_putchar(' ');
		idb_16(bEP0IrqStatus);
		idb_putchar(' ');
		idb_16((u16)cbFifo);
		idb_putchar('\n');
	}
	if(bEP0IrqStatus & EP0_STS_LWO_BIT) {
		if (cbFifo > 0) {
			cbFifo--;
		}
	}
	if (bEP0IrqStatus & EP0_STS_RSR_BIT) {

		USB_DEVICE_REQUEST *pudr=(USB_DEVICE_REQUEST *)pbuffer;

		union{
			u16*	pwUdr;
			u8*		pbUdr;
		}DataCount;

		DataCount.pbUdr = (u8*)pudr;
		volatile u16 *pulFifoReg = CTRLR_BASE_REG_ADDR(EP0_FIFO_REG_OFFSET);
		
		u32 cwWordsRemaining = cbFifo/ sizeof(u16);
		while (cwWordsRemaining--)
		{
			*DataCount.pwUdr++ = *pulFifoReg;
		}
		if(cbFifo % sizeof(u16))
		{
			*DataCount.pbUdr = (u8)*pulFifoReg;
		}

		if (cbFifo != sizeof(USB_DEVICE_REQUEST))
		{
			return FALSE;
		}else
			return TRUE;
	}
	return FALSE;
}


static void 
PrepareToReceiveAndDiscard( u32 numRcve ) {
	bytesYetToRcve  = numRcve;
	bytesYetToStore = 0;
	nextRcvePtr		= NULL;	// discard all bytes sent by the host
}

/** ----------------------------------------------------------------------------
 * ReadBulkOutPacket - read bytes from the bulk OUT endpoint fifo
 *
 * @Parameters:	pDest - the address of a buffer in which to store the bytes
 *
 * @Returns:	the number of bytes read from the fifo and stored in the buffer
 *
 * @Details: 	This routine reads bytes from the bulk OUT endpoint's fifo and
 *				stores them in the buffer at 'pDest'.  The routine calculates
 *	'maxBytesToRead' as the minimum of BULK_PacketSize and bytesYetToReceive.
 *	The routine calculates the number of bytes in the fifo by reading the 
 *	endpoint's BRCR (which is # of words) and doubling the value.  If the 
 *	LWO bit of the endpoint's status register is set, indicating that the
 *	high order byte of the fifo's final word is not valid, the routine 
 *	decrements the number of bytes.  
 *	  The routine reads the lesser of 'maxBytesToRead' and the number of  
 *	bytes in the fifo.
 *	  The routine divides the number of bytes by two to obtain the number of 
 *	words to read from the fifo.  After it has read that many words, the 
 *	routine checks if the number of bytes is odd.  If it is, it reads from
 *	the fifo once more and stores the less significant byte in the buffer.
 *
 *	  If 'pDest' is an even address, the routine can store the words it reads
 *	from the fifo directly into the buffer.  If 'pDest' is an odd address, the
 *	routine stores the words a byte at a time.
 *
 *  Before returning, the routine subtracts the number of bytes read from 
 *  'bytesYetToRcve'.
 *----------------------------------------------------------------------------*/
static u32 ReadBulkOutPacket(void * pDest)
{
	volatile u16 *pulFifoReg = CTRLR_BASE_REG_ADDR(EP2_FIFO_REG_OFFSET);
	u16			  wGepStatus = ReadIndexedReg(DNW_OUT_ENDPOINT, 
											  GEP_STS_REG_OFFSET);
	u32	maxBytesToRead;
	union{
		u8 * pbBuffer;
		u16* pwBuffer;
	}DataBuffer;

	DataBuffer.pbBuffer = (u8*)pDest;
	maxBytesToRead = ((bytesYetToRcve <= BULK_PacketSize)
						? bytesYetToRcve
						: BULK_PacketSize);

		// # of bytes in fifo is 2 * number of words in fifo
		//						(minus 1 if LWO bit is set)
	u32 numBytesInFifo = sizeof(u16) * ReadIndexedReg(DNW_OUT_ENDPOINT, 
											  		  GEP_BRCR_REG_OFFSET);
	idb_puts("RBOP:"); idb_16((u16)numBytesInFifo);

		/* This check was added because in some cases the ep's RSR bit is
		 * set and its BRCR register contains 0.
		 */
	if (numBytesInFifo > 0) {
		if(( wGepStatus & GEP_STS_LWO_BIT) )
			numBytesInFifo--;
		idb_putchar(':'); idb_16((u16)numBytesInFifo);
	}
		// Read from the FIFO
	u32 numBytesToRead = min(numBytesInFifo, maxBytesToRead);
	u32 numBytesRead;

	u32 numWordsToRead = numBytesToRead / sizeof(u16);

	numBytesRead = numWordsToRead * sizeof(u16);

	if ((u32)DataBuffer.pwBuffer & 1) {	// if an odd destination address
		u16 tmp;
		while (numWordsToRead--)
		{
			tmp					   = *pulFifoReg;
			*DataBuffer.pbBuffer++ = (u8)(tmp & 0x00ff);
			*DataBuffer.pbBuffer++ = (u8)((tmp >> 8) & 0x00ff);
		}
	}
	else {
		while (numWordsToRead--)
		{
			*DataBuffer.pwBuffer++ = *pulFifoReg;
		}
	}

	if((numBytesInFifo % sizeof(u16)) && (maxBytesToRead - numBytesRead))
	{
		*DataBuffer.pbBuffer = (u8)(*pulFifoReg);
		numBytesRead ++;
	}

	if (0 == (ReadIndexedReg(DNW_OUT_ENDPOINT, GEP_STS_REG_OFFSET) 
				& GEP_STS_RPS_BIT) )
					// automatically cleared if all fifo data read
	{
		ClearEndpointInterrupt(DNW_OUT_ENDPOINT);
	}
	else {
		idb_putchar('%');	// apparently there's more in the fifo
	}
	bytesYetToRcve -= numBytesRead;

	return numBytesRead;
}


static void WritePacket(u32 dwEndpoint, void *Src, u32 PktSize)
{
	Transfer[dwEndpoint].pvBuffer	   = Src;
	Transfer[dwEndpoint].cbBuffer	   = PktSize;
	Transfer[dwEndpoint].cbTransferred = 0;
	if(dwEndpoint)
		HandleBulkInTx();	
	else
		EP0Send();
}

/** ----------------------------------------------------------------------------
 * HandleGetStatusRequest - respond to a Chapter 9 control request to GetStatus
 *
 * @Parameters:	pSetupPacket - the address of the request's setup packet
 *
 * @Returns:	(void)
 *
 * @Details: 	This routine checks if the setup packet indicates that the
 *				host expects the device to respond with a data packet.
 *	If not, the routine calls StallEndpoint() to stall ep0.
 *	If the host expects to receive a data packet, this routine parses the
 *	request.  If the request is valid, the routine constructs an appropriate
 *  data packet, calls SendControlStatusHandshake() to clear the RSR bit of
 *  ep0's status register, and then calls WritePacket() to send the response.
 *  If the request is invalid, the routine calls SendControlStatusHandshake()
 *  to clear the RSR bit of ep0's status register, and then calls 
 *  StallEndpoint() to stall ep0.
 *----------------------------------------------------------------------------*/
static void HandleGetStatusRequest( tSetupPacket * pSetupPacket )
{
	u8 status[2] = {0,0};
	u8 ep;

	if (   (pSetupPacket->bmRequestType & BM_RQST_XFER_DIR)
		== BM_RQST_XFER_IN)
	{
		int error = 0;

			// if wValue is not zero 
			// or if wLength is not 2
			// of if wIndex is not zero for a device status request,
			// the behavior is not specified; we'll treat those conditions
			// as errors and stall endpoint 0
		if (   (pSetupPacket->wValue  != 0)
			|| (pSetupPacket->wLength != 2)) {
			error = 1;
		}
		switch (pSetupPacket->bmRequestType & BM_RQST_RECIPIENT) {
			case BM_RQST_RECIP_DEVICE:
				if (pSetupPacket->wIndex != 0) {
					error = 2;
				}
				else {
					status[0] = DEVICE_STATUS_SELF_POWERED;
				}
				break;
			case BM_RQST_RECIP_INTFCE:
				// if not in 'Configured State' or if wIndex indicates a 
				// non-existent interface, respond w/ a Request Error
				// (stall ep0)
				if (   !InConfiguredState() 
					|| (pSetupPacket->wIndex != 0)) {
					error = 4;
				}
				break;
			case BM_RQST_RECIP_ENDPT:
				// if in 'Configured State' and wIndex indicates a valid
				//		endpoint, 
				// or if in any other state and wIndex is zero, 
				// indicate the endpoint's status in bit 0 of status[0]
				//		(set the bit if the endpoint is halted).
				// Otherwise respond with a Request Error (stall ep0)
				ep = pSetupPacket->wIndex & 0x000F;
				if (   (InConfiguredState() && (ep < ENDPOINT_COUNT))
					|| (ep == 0) )
				{
					if (epHalted[ep]) {
						status[0] |= ENDPOINT_STATUS_HALTED;
					}
				}
				else error = 8;
				break;
			default:
				break;
		}
			// Clear the RSR bit
		SendControlStatusHandshake();
		if (error) {
			StallEndpoint(0);
        }
		else {		
			WritePacket(0, (void*)status, 2);
		}
	}
	else {	// else the transfer direction is unexpected; stall ep0
		StallEndpoint(0);
	}
}


/** ----------------------------------------------------------------------------
 * HandleGetDescriptor - respond to a Chapter 9 control request to GetDescriptor
 *
 * @Parameters:	pSetupPacket - the address of the request's setup packet
 *
 * @Returns:	(void)
 *
 * @Details: 	This routine checks if the setup packet indicates that the
 *				host expects the device to respond with a data packet.
 *	If not, the routine calls StallEndpoint() to stall ep0.
 *	If so, the routine checks the more significant byte of the setup
 *	packet's wValue element to determine the type of descriptor that
 *	is requested.  If it is a type that this routine recognizes, the
 *	routine copies the descriptor data to the tempdata[] array, calls
 *  SendControlStatusHandshake() to clear the RSR bit of ep0's status register,
 *  and then calls WritePacket() to send the data to the host.
 *	If the descriptor type is not recognized, this routine calls
 *  SendControlStatusHandshake() to clear the RSR bit of ep0's status
 *  register, and then calls StallEndpoint() to stall endpoint 0.
 *----------------------------------------------------------------------------*/
static void HandleGetDescriptor( tSetupPacket * pSetupPacket )
{
	if (   (pSetupPacket->bmRequestType & BM_RQST_XFER_DIR)
		== BM_RQST_XFER_IN)
	{
		char dCode;
		u16 tempdata[0x100];		//becauseof align fault
		S32 len, totLen;

		switch ((u8)(pSetupPacket->wValue>>8)) 
		{
			case DESCRIPTORTYPE_DEVICE:	// 0x01
				totLen = SIZEOF_DEV_DESCRIPTOR;
				len    = (totLen > pSetupPacket->wLength) 
							? pSetupPacket->wLength 
							: totLen;
				memcpy((char *)tempdata, (char *)sc_USBDevDescriptor, (u32)len);
				dCode = 'x';
				break;

			case DESCRIPTORTYPE_CONFIGURATION:	// 0x02
				totLen = TOTAL_CONFIG_LEN;
				len	   = (totLen > pSetupPacket->wLength) 
							? pSetupPacket->wLength 
							: totLen;
				memcpy((char *)tempdata, (char *)sc_USBConfigData, (u32)len);
				dCode = 'y';
				break;

			case DESCRIPTORTYPE_STRING:	// 0x03
			{
				int stringIndex = pSetupPacket->wValue & 0x00FF;

			    if (stringIndex > USB_STR_NUM) 
				{
					// Axelson's USB Complete, 3rd Edition, p114:
					// Microsoft has defined its own Microsoft OS descriptor
					// for use with devices in vendor-defined classes.
					// The descriptor consists of a special string descriptor
					// and one or more Microsoft OS feature descriptors.
					// The string descriptor must have an index of 0xEE and
					// contains an embedded signature.
					// Windows XP SP1 and later request this string descriptor
					// on first attachment.  A device that doesn't support this
					// descriptor should return a STALL.
					if (stringIndex == 0xEE) 
					{
						StallEndpoint(0);
						return;
					}
					stringIndex = USB_STR_NUM+1;
			    } 
				totLen = USB_STRING_DESC[stringIndex][0] & 0x00FF;
				len    = (totLen > pSetupPacket->wLength) 
							? pSetupPacket->wLength 
							: totLen;
				memcpy((char *)tempdata, (char *)USB_STRING_DESC[stringIndex],
						(u32)len);
				dCode = 's';
			}
			    break;
		        
			case DESCRIPTORTYPE_DEV_QUALIFIER:	// 0x06
				totLen = SIZEOF_DEV_QUALIFIER;
				len	   = (totLen > pSetupPacket->wLength) 
							? pSetupPacket->wLength 
							: totLen;
				memcpy((char *)tempdata, 
					   (char *)sc_USBDevQualifierData, (u32)len);
				dCode = 'q';
				break;
		        
			case DESCRIPTORTYPE_OTHER_SPEED_CONFIG:	// 0x07
				totLen = TOTAL_CONFIG_LEN;
				len	   = (totLen > pSetupPacket->wLength) 
							? pSetupPacket->wLength 
							: totLen;
				memcpy((char *)tempdata, 
					   (char *)sc_USBOtherSpeedConfigData, (u32)len);
				dCode = 'o';
				break;

			default:
				dCode = 'z';
				totLen = 0;
				len = 0;
				break;
		}

			// Clear the RSR bit
		SendControlStatusHandshake();
		if (totLen > 0) {	// if we recognize the descriptor type
			// Send the reply data.
			WritePacket(0, (void*)tempdata, len);
		}
		else {	// else it's a Request Error; stall ep0
			StallEndpoint(0);
		}
		idb_putchar(dCode);
	}
	else {	// else the direction bit is not set
		StallEndpoint(0);
	}
}

/** ----------------------------------------------------------------------------
 * HandleGetConfigRequest - respond to a Chap 9 Get Configuration request
 *
 * @Parameters:	pSetupPacket - the address of the request's setup packet
 *
 * @Returns:	(void)
 *
 * @Details: 	This routine checks if the setup packet indicates that the
 *				host expects the device to respond with a data packet.
 *	If not, the routine calls StallEndpoint() to stall ep0.
 *	If the host expects to receive a data packet, this routine calls
 *  SendControlStatusHandshake() to clear the RSR bit in ep0's status
 *  register and calls WritePacket() to send the configuration index.
 *----------------------------------------------------------------------------*/
static void HandleGetConfigRequest( tSetupPacket * pSetupPacket )
{
	if (   (pSetupPacket->bmRequestType & BM_RQST_XFER_DIR)
		== BM_RQST_XFER_IN)
	{
			// Clear the RSR bit
		SendControlStatusHandshake();
            // and send the configuration index
		WritePacket(0, (void*)&deviceConfig, 1);
	}
	else {	// else the transfer direction is unexpected; stall ep0
		StallEndpoint(0);
	}
}

/** ----------------------------------------------------------------------------
 * HandleGetInterfaceRequest - respond to a Chap 9 Get Interface request
 *
 * @Parameters:	pSetupPacket - the address of the request's setup packet
 *
 * @Returns:	(void)
 *
 * @Details: 	This routine checks if the setup packet indicates that the
 *				host expects the device to respond with a data packet.
 *	If not, the routine calls StallEndpoint() to stall ep0.
 *	If the host expects to receive a data packet, this routine calls
 *  SendControlStatusHandshake() to clear the RSR bit in ep0's status
 *  register and calls WritePacket() to send the interface index.
 *----------------------------------------------------------------------------*/
static void HandleGetInterfaceRequest( tSetupPacket * pSetupPacket )
{
	if (   (pSetupPacket->bmRequestType & BM_RQST_XFER_DIR)
		== BM_RQST_XFER_IN)
	{
			// Clear the RSR bit
		SendControlStatusHandshake();
            // and send the interface index
		WritePacket(0, (void*)&deviceInterface, 1);
	}
	else {	// else the transfer direction is unexpected; stall ep0
		StallEndpoint(0);
	}
}

/** ----------------------------------------------------------------------------
 * HandleClearFeatureRequest - respond to a Chap. 9 request to Clear Feature
 *
 * @Parameters:	pSetupPacket - the address of the request's setup packet
 *
 * @Returns:	(void)
 *
 * @Details: 	This routine checks if the setup packet indicates that the
 *				host expects the device to respond with a data packet.
 *	If so, the routine calls SendControlStatusHandshake() to clear the RSR 
 *  bit of ep0's status register, and then calls StallEndpoint() to stall ep0.
 *	If not, the routine uses the setup packet's wValue element as the feature
 *  selector and the 'request recipient' bits of the bmRequestType element as
 *  the recipient selector.  If the (recipient, feature) pair is (endpoint,
 *  Halt), and if the endpoint (in wIndex) is valid, and if the device's state
 *  is suitable, the routine processes the request and calls 
 *  SendControlStatusHandshake().
 *  In all other cases the routine calls StallEndpoint() to stall endpoint 0.
 *----------------------------------------------------------------------------*/
static void HandleClearFeatureRequest( tSetupPacket * pSetupPacket )
{
	int status = 0;	// set to 1 if SendControlHandshake() ought to be called
					// 0 == call StallEndpoint(0)
	if (   (pSetupPacket->bmRequestType & BM_RQST_XFER_DIR)
		== BM_RQST_XFER_OUT)
	{
		u16	featureSelector = pSetupPacket->wValue;

		switch (pSetupPacket->bmRequestType & BM_RQST_RECIPIENT) {
			case BM_RQST_RECIP_DEVICE:
				switch (featureSelector) {
					case FEATURE_DEVICE_TEST_MODE:
						// Chap 9 says the Test Mode feature cannot be cleared
						break;
					case FEATURE_DEVICE_REMOTE_WAKEUP:
						// This device doesn't support remote wakeup
						break;
					default:
						break;
				}
				break;
			case BM_RQST_RECIP_INTFCE:
				// No feature is recognized for an interface
				break;
			case BM_RQST_RECIP_ENDPT:
				if (featureSelector == FEATURE_ENDPOINT_HALT) {
					u8 ep = pSetupPacket->wIndex & 0x000F;
					idb_putchar('c');
					idb_putchar('0' + ep);
					switch (ep) {
						case 0:	
							if (InAddressState() || InConfiguredState())
							{
								ClearEndpointStall(0);
								ClearEndpointDataPid(0);
								status = 1;	
							}
							break;
						case DNW_IN_ENDPOINT:
						case DNW_OUT_ENDPOINT:
							// if state is configured
							if (InConfiguredState())
							{
								status		 = 1;
								ClearEndpointDataPid(ep);
								if (!awaitingRecoveryReset[ep]) {
									// Sometimes the host sends two clear stalls
									// so we check if we're currently stalled
									// before clearing the stall.
								  if (epHalted[ep]) {
									ClearEndpointStall(ep);
									idb_putchar('&');
									if (DNW_IN_ENDPOINT == ep) {
										TransmitCSW();
									}
								  }
								  else idb_putchar('=');
								}
							}
							break;
						default:
							break;
					}
				}
				break;
			default:
				break;
		}
	    if (status != 0) {
		    SendControlStatusHandshake();
	    }
	    else {	// Request Error, so stall ep0
		    StallEndpoint(0);
	    }
	}
	else {	// Host expects a data stage; clear RSR, then stall ep0
	    SendControlStatusHandshake();
		StallEndpoint(0);
	}
}

/** ----------------------------------------------------------------------------
 * HandleSetFeatureRequest - respond to a Chap. 9 request to Set Feature
 *
 * @Parameters:	pSetupPacket - the address of the request's setup packet
 *
 * @Returns:	(void)
 *
 * @Details: 	This routine checks if the setup packet indicates that the
 *				host expects the device to respond with a data packet.
 *	If so, the routine calls SendControlStatusHandshake() to clear the RSR 
 *  bit of ep0's status register, and then calls StallEndpoint() to stall ep0.
 *	If not, the routine uses the setup packet's wValue element as the feature
 *  selector and the 'request recipient' bits of the bmRequestType element as
 *  the recipient selector.  If the (recipient, feature) pair is supported, 
 *  the routine processes the request and calls SendControlStatusHandshake().
 *  In all other cases the routine calls StallEndpoint() to stall endpoint 0.
 *----------------------------------------------------------------------------*/
static void HandleSetFeatureRequest( tSetupPacket * pSetupPacket )
{
	int status = 0;	// set to 1 if SendControlHandshake() ought to be called
					// 0 == call StallEndpoint(0)
	if (   (pSetupPacket->bmRequestType & BM_RQST_XFER_DIR)
		== BM_RQST_XFER_OUT)
	{
		u16	featureSelector = pSetupPacket->wValue;

		switch (pSetupPacket->bmRequestType & BM_RQST_RECIPIENT) {
			case BM_RQST_RECIP_DEVICE:
				switch (featureSelector) {
						// NOTE: high-speed devices must support TEST_MODE
					case FEATURE_DEVICE_TEST_MODE:
							// the lsb of wIndex must be 0.
						if (0 == (0x00FF & pSetupPacket->wIndex)) {
							switch ((pSetupPacket->wIndex >> 8) & 0x00FF) {
								// Chap9 specifies these selectors
								// NOTE: the code in this switch has not been
								//		 tested.
								case TEST_SELECTOR_J:
									SetRegBits(TEST_REG_OFFSET, (1 << TMD));
									SetRegBits(TEST_REG_OFFSET, (1 << TJS));
									status = 1;
									break;
								case TEST_SELECTOR_K:
									SetRegBits(TEST_REG_OFFSET, (1 << TMD));
									SetRegBits(TEST_REG_OFFSET, (1 << TKS));
									status = 1;
									break;
								case TEST_SELECTOR_SE0_NAK:
									SetRegBits(TEST_REG_OFFSET, (1 << TMD));
									SetRegBits(TEST_REG_OFFSET, (1 << TSNS));
									status = 1;
									break;
								case TEST_SELECTOR_PACKET:
									SetRegBits(TEST_REG_OFFSET, (1 << TMD));
									SetRegBits(TEST_REG_OFFSET, 1 << TEST_TPS);
									status = 1;
									break;
								case TEST_SELECTOR_FORCE_ENABLE:
									// This test selector is appropriate only
									// for hubs.  Therefore leave status == 0
									// so a Request Error response will be made.
								default:
									break;
							}
						}
						break;
					case FEATURE_DEVICE_REMOTE_WAKEUP:
						// This device doesn't support remote wakeup
						break;
					default:
						break;
				}
				break;
			case BM_RQST_RECIP_INTFCE:
				// No feature is recognized for an interface
				break;
			case BM_RQST_RECIP_ENDPT:
				if (featureSelector == FEATURE_ENDPOINT_HALT) {
					u8 ep = pSetupPacket->wIndex & 0x000F;
					switch (ep) {
						case 0:	
							if (InAddressState() || InConfiguredState())
							{
#if 0	// 30jul09 I think we don't really want to stall ep0??
								StallEndpoint(0);
#endif	// 30jul09
								status = 1;
							}
							break;
						case DNW_IN_ENDPOINT:
						case DNW_OUT_ENDPOINT:
							// if state is configured
							if (InConfiguredState())
							{
								StallEndpoint(ep);
								status = 1;
							}
							break;
						default:
							break;
					}
				}
				break;
			default:
				break;
		}
	    if (status != 0) {
		    SendControlStatusHandshake();
	    }
	    else {	// Request Error, so stall ep0
		    StallEndpoint(0);
	    }
	}
	else {	// Host expects a data stage; clear RSR, then stall ep0
	    SendControlStatusHandshake();
		StallEndpoint(0);
	}
}

/** ----------------------------------------------------------------------------
 * HandleSetAddressRequest - respond to a Chap. 9 request to Set Address
 *
 * @Parameters:	pSetupPacket - the address of the request's setup packet
 *
 * @Returns:	(void)
 *
 * @Details: 	This routine checks if the setup packet indicates that the
 *				host expects the device to respond with a data packet.
 *	If so, the routine calls SendControlStatusHandshake() to clear the RSR 
 *  bit of ep0's status register, and then calls StallEndpoint() to stall ep0.
 *	If not, the routine uses the least significant 7 bits of the setup packet's
 *  wValue element as the address, saves it in 'deviceAddress', writes it
 *  to the UDC's address register, and calls SendControlStatusHandshake().
 *----------------------------------------------------------------------------*/
static void HandleSetAddressRequest( tSetupPacket * pSetupPacket )
{
	if (   (pSetupPacket->bmRequestType & BM_RQST_XFER_DIR)
		== BM_RQST_XFER_OUT)
	{
		    // Save the device address.
		    // NOTE: if device address is greater than 127 or if the
		    // device is in Configured state, behavior is not specified.
		    // We use the least significant 7 bits, and we update the address
		    // even when the device is in Configured state.
	    deviceAddress = pSetupPacket->wValue & 0x7F;
	    SetAddress((u8)deviceAddress);	// store address in the UDC
	    SendControlStatusHandshake();
    }
	else {	// Request Error, so stall ep0
	    SendControlStatusHandshake();
		StallEndpoint(0);
	}
}

/** ----------------------------------------------------------------------------
 * HandleSetConfigurationRequest - respond to a Chap. 9 request to Set Config'n
 *
 * @Parameters:	pSetupPacket - the address of the request's setup packet
 *
 * @Returns:	(void)
 *
 * @Details: 	This routine checks if the setup packet indicates that the
 *				host expects the device to respond with a data packet.
 *	If so, the routine calls SendControlStatusHandshake() and then calls 
 *  StallEndpoint() to stall ep0.
 *	If not, the routine uses the less significant byte of the setup packet's
 *  wValue element as the configuration index.  If it is 0 or is a recognized
 *  configuration index, and if the device's address has been set, the routine
 *  stores the configuration index in 'deviceConfig', zeroes the DataPID of 
 *  each bulk endpoint, and calls SendControlStatusHandshake().
 *  If the device's address has not been set or if the configuration index is
 *  is invalid, the routine calls StallEndpoint() to stall ep0.
 *----------------------------------------------------------------------------*/
static void HandleSetConfigurationRequest( tSetupPacket * pSetupPacket )
{
	if (   (pSetupPacket->bmRequestType & BM_RQST_XFER_DIR)
		== BM_RQST_XFER_OUT)
    {
	    if (   InAddressState()
            && (   (0 == (u8)pSetupPacket->wValue)
    		    || (sc_USBConfigData[5] == (u8)pSetupPacket->wValue))) {
		    deviceConfig = (u8)pSetupPacket->wValue;
		    ClearEndpointDataPid(DNW_IN_ENDPOINT);
		    ClearEndpointDataPid(DNW_OUT_ENDPOINT);
		    SendControlStatusHandshake();
        }
        else {	// Invalid config selector
    		StallEndpoint(0);
        }
	}
	else {  // Invalid direction (IN)
	    SendControlStatusHandshake();
		StallEndpoint(0);
	}
	idb_putchar('h');
}

/** ----------------------------------------------------------------------------
 * HandleSetInterfaceRequest - respond to a Chap. 9 request to Set Interface
 *
 * @Parameters:	pSetupPacket - the address of the request's setup packet
 *
 * @Returns:	(void)
 *
 * @Details: 	This routine checks if the setup packet indicates that the
 *				host expects the device to respond with a data packet.
 *	If so, the routine calls SendControlStatusHandshake() and then calls 
 *  StallEndpoint() to stall ep0.
 *	If not, the routine uses the the setup packet's wIndex and wValue elements
 *  as the indices of the interface and an alternate setting.  If either is
 *  invalid, or if the device's configuration is not set, the routine calls
 *  StallEndpoint() to stall endpoint 0.
 *  If both elements are valid and if the device's configuration is set, the 
 *  routine saves the interface and the alternate setting in variables, zeroes
 *  the DataPID of each bulk endpoint, and calls SendControlStatusHandshake().
 *----------------------------------------------------------------------------*/
static void HandleSetInterfaceRequest( tSetupPacket * pSetupPacket )
{
	if (   (pSetupPacket->bmRequestType & BM_RQST_XFER_DIR)
		== BM_RQST_XFER_OUT)
    {
	    if (   !InConfiguredState()			// if not yet in Configured state, 
		    || (pSetupPacket->wIndex != 0)	// or if invalid intfce
		    || (pSetupPacket->wValue != 0))	//   or alternate setting ==> ERROR
	    {
		    StallEndpoint(0);
	    }
	    else {
		    deviceInterface = (u8)pSetupPacket->wIndex;
		    deviceSetting   = (u8)pSetupPacket->wValue;
		    ClearEndpointDataPid(DNW_IN_ENDPOINT);
		    ClearEndpointDataPid(DNW_OUT_ENDPOINT);
		    SendControlStatusHandshake();
	    }
	}
	else {  // Invalid direction (IN)
	    SendControlStatusHandshake();
		StallEndpoint(0);
	}
}


static void HandleChapter9_Requests( tSetupPacket * pSetupPacket )
{
	switch (pSetupPacket->bRequest) {
	case GET_STATUS:		HandleGetStatusRequest( pSetupPacket );		break;
	case CLEAR_FEATURE:		HandleClearFeatureRequest( pSetupPacket );	break;
	case SET_FEATURE:		HandleSetFeatureRequest( pSetupPacket );	break;
	case SET_ADDRESS:		HandleSetAddressRequest( pSetupPacket );	break;
	case GET_DESCRIPTOR: 	HandleGetDescriptor( pSetupPacket );		break;
	case GET_CONFIGURATION:	HandleGetConfigRequest( pSetupPacket );     break;
	case GET_INTERFACE:		HandleGetInterfaceRequest( pSetupPacket );  break;
	case SET_CONFIGURATION: HandleSetConfigurationRequest(pSetupPacket); break;
	case SET_INTERFACE:		HandleSetInterfaceRequest( pSetupPacket );	break;

	case SET_DESCRIPTOR:	// We don't support this command,
							//	 so we respond with a Request Error
	default:				// Unrecognized request; Request Error
			// If this is an invalid request with the IN bit set,
			// first clear the RSR bit, and then stall the endpoint
			// If the IN bit is not set, just stall the endpoint.
		if (pSetupPacket->bmRequestType & 0x80) /* IN */
		{
			SendControlStatusHandshake();
		}
		StallEndpoint(0);
		idb_putchar('k');
		break;
	}
}


/** ----------------------------------------------------------------------------
 * HandleClassSetupRequests - respond to a MSC-specific control request
 *
 * @Parameters:	pSetupPacket - address of setup packet containing the request
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine supervises processing of class-specific control
 *				requests.
 *	Two MSC-specific control requests are recognized.  The GetMaxLUN request
 *	has a request code of 0xFE.  The bulk-only-transport (BOT) MSC Reset
 *	request has a request code of 0xFF.
 *
 *	When it receives a valid GetMaxLUN request, the routine calls WritePacket()
 *	to send a one-byte data packet containing 0, the maximum LUN supported
 *	by the device.
 *
 *	When it receives a valid BOT MSC Reset request, the routine zeroes the
 *	awaitingRecoveryReset[] entry of every endpoint and initializes variables
 *	to prepare itself for receiving a command block wrapper (CBW).
 *
 *	When it receives any other type of request or an invalid instance of these
 *	two types of request, the routine calls StallEndpoint() to stall ep0.
 *----------------------------------------------------------------------------*/
	// Handle an MSC Control command
static void HandleClassSetupRequests( tSetupPacket * pSetupPacket )
{
	idb_putchar('M');  
		/* first check the request.  We recognize only two: 0xFE and 0xFF */
	switch (pSetupPacket->bRequest) {
		case 0xFE:
			if (   (pSetupPacket->bmRequestType & 0x80) /* IN */
				&& (pSetupPacket->wValue   == 0)
				&& (pSetupPacket->wIndex   == 0)
				&& (pSetupPacket->wLength  >= 1)) 
			{
				unsigned char maxlun[2] = {0, 0};
				
				SendControlStatusHandshake();	// Clear the RSR bit
				WritePacket(0, (void*)&maxlun[0], 1);
				idb_putchar('e');
			}
				// else this is an invalid request.
				// If the IN bit set, first clear the RSR bit, and then stall
				// the endpoint.
				// If the IN bit is not set, just stall ep0
			else {
				if (pSetupPacket->bmRequestType & 0x80) /* IN */
				{
					SendControlStatusHandshake();
				}
				StallEndpoint(0);
				idb_putchar('f');
			}
			break;

		case 0xFF:
			if (   (  (pSetupPacket->bmRequestType & BM_RQST_RECIPIENT) 
					== BM_RQST_RECIP_INTFCE)
				&& (pSetupPacket->wValue  == 0)
				&& (pSetupPacket->wIndex  == 0)	// this is the intfce index
				&& (pSetupPacket->wLength == 0)) 
			{	// Bulk-only mass storage reset
				// The spec says to ready the device for the next CBW
				// from the host.
				// NAK the status stage until the reset is complete.
				// Since we're doing nothing, we drop through and 
				// ACK immediately.

				csw_ready		= 0;
				bytesYetToRcve	= 0;
				bytesYetToStore	= 0;
				bytesYetToXmit	= 0;
				awaitingRecoveryReset[0] 				= 0;
				awaitingRecoveryReset[DNW_IN_ENDPOINT]  = 0;
				awaitingRecoveryReset[DNW_OUT_ENDPOINT] = 0;
				msd_state		= MSD_STATE_AWAITING_CBW;
				SendControlStatusHandshake();
				idb_putchar('r');
			}
				// else this is an invalid request.
				// If the IN bit set, first clear the RSR bit, and then stall
				// the endpoint.
				// If the IN bit is not set, just stall ep0
			else {
				if (pSetupPacket->bmRequestType & 0x80) /* IN */
				{
					SendControlStatusHandshake();
				}
				StallEndpoint(0);
				idb_putchar('q');
				idb_byte( pSetupPacket->bmRequestType);
				idb_putchar(' ');
				idb_byte( pSetupPacket->bRequest);
				idb_putchar(' ');
				idb_16( pSetupPacket->wValue);
				idb_putchar(' ');
				idb_16( pSetupPacket->wIndex);
				idb_putchar(' ');
				idb_16( pSetupPacket->wLength);
				idb_putchar('\n');
			}
			break;

		default:	/* for everything else, stall ep0 */
			if (pSetupPacket->bmRequestType & 0x80) /* IN */
			{
				SendControlStatusHandshake();
			}
			StallEndpoint(0);
			idb_putchar('d');
			break;
	}
}


/** ----------------------------------------------------------------------------
 * HandleEndpoint0Event - handle interrupts caused by activity on endpoint 0
 *
 * @Parameters:	(void).
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine supervises processing that deals with interrupts
 *				that are caused by activity on endpoint 0, the control endpoint.
 *	The routine reads the UDC's EP0 Status register, whose bits indicate the
 *	endpoint status and the reason for the interrupt.
 *
 *	If the status register's SHT bit is set, indicating that a STALL handshake
 *	has been sent to the Host, this routine calls ClearEndpointStall() to 
 *	clear the stall condition.  
 *
 *	If neither the TST bit nor the RSR bit is set, this routine just returns.
 *
 *	If the status register's TST bit is set, indicating that a complete packet
 *	has been transmitted to the Host and ACKed, this routine calls 
 *	CheckEP0Send().  If its return value indicates that it sent another packet,
 *	this routine just returns.  Otherwise it proceeds to check the RSR bit.
 *
 *	If the status register's RSR bit is set, indicating that an  error-free
 *	packet has been received from the Host, this routine calls EP0GetCommand()
 *	to read the received Setup packet.  Then this routine parses and process
 *	the packet's command.
 *----------------------------------------------------------------------------*/
static void
HandleEndpoint0Event(void)
{
	u16 bEP0IrqStatus = ReadReg(EP0_STS_REG_OFFSET);

	idle_time = now();	// Reset idle timeout counter -- we're active

		// If ep0 is stall, clear the stall.
	if (bEP0IrqStatus & EP0_STS_SHT_BIT) {
		ClearEndpointStall(0);
		idb_putchar('$');
	}
		/* if neither TST nor RSR, just return */
	if (0 == (bEP0IrqStatus & (EP0_STS_TST_BIT | EP0_STS_RSR_BIT))) {
		//idb_putchar('-');
		idb_byte((char)bEP0IrqStatus);
		return;
	}
	if (bEP0IrqStatus & EP0_STS_TST_BIT) {
			// Clear the TST bit 
		WriteReg(EP0_STS_REG_OFFSET, EP0_STS_TST_BIT);
		idb_putchar('t');
		if (CheckEP0Send()) {	// if we sent something more, return
			idb_putchar('T');
			return;
		}
	}	// if we didn't send anything more, check for RSR

	if (bEP0IrqStatus & EP0_STS_RSR_BIT)
	{
			// NOTE: We check only for Setup packets, because none of the
			//		 requests we recognize have an outbound data stage.
			// If that ever changes, we'll need to change the code to
			// keep track of whether its expecting a setup packet or a
			// data packet and to branch accordingly.
 
			// We put the setup packet in a union to ensure that it's on
			// an even address, as assumed by EP0GetCommand(), and to
			// ensure that even if EP0GetCommand() reads and stores more than 
			// sizeof(tSetupPacket) bytes, it won't cause problems.
		union {
			u16 a[HIGHSPEED_MAX_EP0_PKT_SIZE/sizeof(u16)];
			tSetupPacket SetupPacket;
		} u;

		idb_putchar('r');
		// Read the command from the input FIFO
		if (EP0GetCommand((void*)&u.SetupPacket) == FALSE)
		{
			StallEndpoint(0);
			idb_putchar('f');
			return;
		}
		// Decode and execute the command. 
		// We support two sets of commands:
		//		 class commands for MSC
		//   and chapter 9 standard commands.
		// We do not support vendor commands
		switch (u.SetupPacket.bmRequestType & BM_RQST_TYPE)
		{
			case BM_RQST_TYPE_STD: // standard chapter 9 commands
				HandleChapter9_Requests( &u.SetupPacket );
				break;

			case BM_RQST_TYPE_CLASSS:
				HandleClassSetupRequests( &u.SetupPacket );
				break;

			default:	// VENDOR, reserved
					// If this is an unrecognized request with the IN bit set,
					// first clear the RSR bit, and then stall the endpoint
					// If the IN bit is not set, just stall the endpoint.
				if (u.SetupPacket.bmRequestType & 0x80) /* IN */
				{
					SendControlStatusHandshake();
				}
				StallEndpoint(0);
				idb_puts("Vendor-Control\n");
				break;
		}
	}
	idb_putchar(';');
}



// From SCSI spec SPC (SCSI primary commands)
// Byte 0 -- 70 = Current error
// Byte 1 -- Segment number
// Byte 2 -- Sense key (SPC table 107)
//             5 = ILLEGAL REQUEST. Indicates that there was an illegal 
//					parameter in the CDB or in the additional parameters 
//					supplied as data for some commands
//             B = ABORTED COMMAND. Indicates that the device server aborted the
//					command. The application client may be able to recover by 
//					trying the command again.
//             E = MISCOMPARE. Indicates that the source data did not match the 
//					data read from the medium.
// Byte 3-6  -- Information (not used)
// Byte 7    -- add'l sense length 
// byte 8-11 -- Command specific information
// byte 12   -- ASC (Add'l sense code)
// byte 13   -- ASQ (Add'l sense qualifier)
//        Key                                     ASC ASQ
//  0  1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
//const char code senseTemplate[] =  {
//		0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
//		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};   

//                                          SK    ASC   ASQ
	// copied from LeapsterUSB's disk.c
const unsigned char senseArray[ 15 ][ 3 ] = 
    {
    {0x0b, 0x08, 0x03},   // senseCRCError                0 
    {0x05, 0x24, 0x00},    // senseInvalidFieldInCDB      1
    {0x00, 0x00, 0x00},    // senseOk                     2
    {0x02, 0x3a, 0x00},    // senseNoMedia                3
    {0x03, 0x03, 0x00},    // senseWriteFault             4
    {0x03, 0x11, 0x00},    // senseReadError              5
    {0x03, 0x12, 0x00},    // senseAddrNotFound           6
    {0x05, 0x20, 0x00},    // senseInvalidOpcode          7
    {0x05, 0x21, 0x00},    // senseInvalidLBA             8
    {0x05, 0x26, 0x00},    // senseInvalidParameter       9
    {0x05, 0x53, 0x02},    // senseCantEject              0xa
    {0x06, 0x28, 0x00},    // senseMediaChanged           0xb
    {0x06, 0x29, 0x00},    // senseDeviceReset            0xc
    {0x07, 0x27, 0x00},    // senseWriteProtected         0xd
    {0x02, 0x04, 0x01}     // senseNotReady               0x0e  
    };


#ifdef RAMDISK
	// This contains the last logical block address and the block length 
	// in bytes.
	// Each is stored (BigEndian) in four bytes.
const unsigned char device_capacity[8] = {
		// we subtract 1 to get the index of the last logical block (sector)
	0x00, 0x00, (SECTORS_PER_DISK - 1)/256, (SECTORS_PER_DISK - 1) % 256,
	0x00, 0x00, 0x02, 0x00				// 0x200 = 512
};
#endif


const unsigned char mode_sense6_response[16] = {
	15,			// length - 1
	0, 0, 0,
	8,			// page code
	10,			// page length
	4,			// write cache enable
	0,
	0xFF, 0xFF, // Don't disable prefetch
	0, 0,
	0xFF, 0xFF, // Maximum prefetch 
	0xFF, 0xFF	// Maximum prefetch ceiling
};

#ifdef RAMDISK
const unsigned char format_capacities[] = {
	0,0,0,
	8,			// capacity list length
	0,0,SECTORS_PER_DISK/256,SECTORS_PER_DISK%256,	// # of blocks (sectors)
	2,			// formatted media, current capacity
	0,2,0		// 512-byte block (sector)
};
#endif	// RAMDISK


void InitCSW(u32 tag, u32 residue, u8 status) 
{
	if (0 == csw_ready) {
		csw.b[OFFSET_CSW_SIGNATURE]	  = 'U';
		csw.b[1+OFFSET_CSW_SIGNATURE] = 'S';
		csw.b[2+OFFSET_CSW_SIGNATURE] = 'B';
		csw.b[3+OFFSET_CSW_SIGNATURE] = 'S';

		csw.b[OFFSET_CSW_TAG]   = tag & 0x00FF;
		csw.b[1+OFFSET_CSW_TAG] = (tag >> 8)  & 0x00FF;
		csw.b[2+OFFSET_CSW_TAG] = (tag >> 16) & 0x00FF;
		csw.b[3+OFFSET_CSW_TAG] = (tag >> 24) & 0x00FF;

		if (sendCswResidue > 0) {
			residue			= sendCswResidue;
			sendCswResidue	= 0;
		}

		csw.b[OFFSET_CSW_RESIDUE]   = residue & 0x00FF;
		csw.b[1+OFFSET_CSW_RESIDUE] = (residue >> 8)  & 0x00FF;
		csw.b[2+OFFSET_CSW_RESIDUE] = (residue >> 16) & 0x00FF;
		csw.b[3+OFFSET_CSW_RESIDUE] = (residue >> 24) & 0x00FF;

		csw.b[OFFSET_CSW_STATUS] = (sendCswStatusResetRqd) 
											? CSW_STATUS_RESET_RQD
											: status;
		sendCswStatusResetRqd = 0;

		csw_ready = 1;
		idb_putchar('z');
	}
	else idb_putchar('X');
}



static int TransmitCSW(void)
{
	int xmitted;

	if (csw_ready) {
		if (!epBulkInStalled) {
			WritePacket(DNW_IN_ENDPOINT, (void*)&csw, SIZEOF_CSW);
			csw_ready = 0;
			idb_putchar('Z');
			xmitted   = 1;
			msd_state = MSD_STATE_AWAITING_CBW;
		}
		else {
			idb_putchar('P');
			xmitted = 0;
		}
	}
	else {
		idb_putchar('Y');
		xmitted = 0;
	}
	return xmitted;
}


/** ----------------------------------------------------------------------------
 * ScsiXXX - routines for processing scsi commands in CBWs
 *
 * @Parameters:	cbwTag			the tag specified in the CBW
 *				cbwDataXferLen	the dataXferLen specified in the CBW
 *				pCBW			the address of the CBW
 * @Returns:	(void).
 *
 * @Details: 	These routines supervise processing of scsi commands that
 *				have been received in command block wrappers (CBWs).
 *
 *----------------------------------------------------------------------------*/

void StdScsiInputRequest(u32 cbwTag, u32 cbwDataXferLen, u8 * pCBW,
						 const u8 * pResp, u32 respLen)
{
	u32	bytesToSend;

		// Send back the response (@ pResp).
		// Ensure that we don't try to send more than the host 
		// asked for (cbwDataXferLen).
	bytesToSend = (respLen <= cbwDataXferLen)
					? respLen
					: cbwDataXferLen;
	if (bytesToSend == 0) {	// if no data transfer
		InitCSW(cbwTag, cbwDataXferLen - bytesToSend, 0) ;
		TransmitCSW();
	}
	else if (pCBW[OFFSET_CBW_FLAGS] == 0x80) {	// if host expects to receive
		WritePacket(DNW_IN_ENDPOINT, (void*)pResp, bytesToSend);
		InitCSW(cbwTag, cbwDataXferLen - bytesToSend, 0) ;

			// if we're sending less than requested, prepare to stall the
			// bulk IN endpoint after the data has been transmitted.
			// This is case #5 (Hi > Di).
		if (cbwDataXferLen - bytesToSend > 0) {
			sendCswResidue	   = cbwDataXferLen - bytesToSend;
			stallInEPAfterData = 1;
			longXferTag		   = cbwTag;
		}
	}
	else {	// else host expects to send data.  Case #10 (Ho <> Di)
			// Prepare to receive and discard the bytes sent by the host;
			// prepare a CSW with status CSW_STATUS_RESET_RQD.
		PrepareToReceiveAndDiscard(cbwDataXferLen);
		InitCSW(cbwTag, cbwDataXferLen, CSW_STATUS_RESET_RQD);
	}
}


#ifndef RAMDISK	// RAMDISK is not defined, so there's no media;  
				// If cbwDataXferLen is 0, just send the CSW
				// else if host expects to receive data, stall the IN ep
				// else (host expects to send data), stall the OUT ep
static void ScsiMeaningless( u32 cbwTag, u32 cbwDataXferLen, u8 * pCBW )
{
	SensePtr = senseNoMedia;
	InitCSW(cbwTag, cbwDataXferLen, 1);
	if (cbwDataXferLen == 0) {
		TransmitCSW();
	}
	else if (pCBW[OFFSET_CBW_FLAGS] == 0x80) {	// host expects to receive
		StallEndpoint(DNW_IN_ENDPOINT);
	}
	else {	// else host expects to send data.
		StallEndpoint(DNW_OUT_ENDPOINT);
	}
}
#endif	

void ScsiRequestSense( u32 cbwTag, u32 cbwDataXferLen, u8 * pCBW )
{
	int 	 	  i;
	unsigned char response[REQUEST_SENSE_RESPONSE_LEN];

	idb_putchar('S');
	for (i = 0; i < REQUEST_SENSE_RESPONSE_LEN; ++i) {
		response[i] = 0;
	}
	response[  0 ] = 0x70;
	response[  7 ] = 0x0a;
	response[  2 ] = senseArray[ SensePtr ][ 0 ];	// SenseKey
	response[ 12 ] = senseArray[ SensePtr ][ 1 ];	// ASC
	response[ 13 ] = senseArray[ SensePtr ][ 2 ];	// ASCQ
	StdScsiInputRequest( cbwTag, cbwDataXferLen, pCBW,
						 response, REQUEST_SENSE_RESPONSE_LEN);
}


#ifdef RAMDISK
static void ScsiRead10( u32 cbwTag, u32 cbwDataXferLen, u8 * pCBW )
{
	idb_putchar('r');
	int bytesRequested;
	int offset;
	u8 * xmitPtr;

		// First check if cbwDataXferLen is 0.  
		//   If it is, check if the CDB's transfer length is 0.  
		//   If both are zero (this is the normal Case 1), prepare a CSW
		//		with residue 0 and status OK.
		//   If the CDB's transfer length is positive (this is an abnormal
		//		Case 2), prepare a CSW with residue 0 and status RESET_RQD.
		//		We may stall the bulk IN endpoint, but for now we don't.  
		//		When the host reads the CSW with status RESET_RQD, it performs
		//		a Reset Recovery.
	if (0 == cbwDataXferLen)
	{
		u32 cdbXferLen;

		cdbXferLen	= BYTES_PER_SECTOR 
						* GetU16be(&pCBW[OFFSET_CBW_READ10_XFER_LEN]);
		if (0 == cdbXferLen) {	// ok; case 1.
			InitCSW(cbwTag, 0, CSW_STATUS_OK);
		}
		else {	// case 2
			//StallEndpoint(DNW_IN_ENDPOINT); /* Axelson says this is optional*/
			InitCSW(cbwTag, 0, CSW_STATUS_RESET_RQD);
		}
		TransmitCSW();
		return;
	}

	offset			= BYTES_PER_SECTOR 
						* GetU32be( &pCBW[OFFSET_CBW_READ10_LBA] );
	xmitPtr			= GetRamDiskPtr(offset);
	bytesRequested	= BYTES_PER_SECTOR 
						* GetU16be( &pCBW[OFFSET_CBW_READ10_XFER_LEN] );

		// check that the CBW's flags byte indicates xfer IN to host (0x80)
		// If it doesn't, check if the CDB indicates the device ought to send
		// zero bytes or more than zero bytes. 
		// If 0, we're in Case #9.  In this case:
		//		accept cbwDataXferLen bytes from the host and discard them;
		//		initialize the CSW's residue to cbwDataXferLen and its status
		//			to 0 (OK);
		//		after receiving the bytes, send the CSW.
		//
		// If more than 0, we're in Case #10.  In this case:
		//		accept cbwDataXferLen bytes from the host and discard them;
		//		initialize the CSW's residue to cbwDataXferLen and its status
		//			to 2 (CSW_STATUS_RESET_RQD), which will cause the host to
		//			ignore the residue;
		//		after receiving the bytes, send the CSW.
		
	if (pCBW[OFFSET_CBW_FLAGS] != 0x80) 
	{
		PrepareToReceiveAndDiscard(cbwDataXferLen);
		if (bytesRequested == 0) {	// Case 9
			InitCSW(cbwTag, cbwDataXferLen, 0);
		}
		else {						// Case 10
			InitCSW(cbwTag, cbwDataXferLen, CSW_STATUS_RESET_RQD);
		}
		return;
	}

	// CBW's flags byte indicates transfer IN to the host
	// We know that cbwDataXferLen is > 0.
	// If cbwDataXferLen equals bytesRequested, we're in case 6; we'll send
	//		cbwDataXferLen bytes and initialize the CSW's residue to 0 and
	//		its status to 0.
	//
	// If cbwDataXferLen < bytesRequested, we're in case 7; we'll send 
	//		cbwDataXferLen bytes and initialize the CSW's residue to 0 and
	//		its status to 2.  Axelson say we may stall the bulk IN e.p., but 
	//		for now we won't.
	//
	// If cbwDataXferLen > bytesRequested, we're in either case 4 (if 
	//		bytesRequested is 0) or case 5 (if bytesRequested > 0).
	//		In either case we'll pad the data and send cbwDataXferLen bytes
	//		and initialize the CSW's residue to (cbwDataXferLen - bytesRequested)
	//		and its status to 0.

	if (cbwDataXferLen < bytesRequested) {
		//Hi < Di  Case #7
		bytesRequested		  = cbwDataXferLen;
		sendCswStatusResetRqd = 1;	// after xmitting data, send CSW with
	}								// status RESET_RQD.

		//	Case 4 (Dn) or Case 5 (Di < Hi)
	else if (cbwDataXferLen > bytesRequested) {
		//InitCSW(cbwTag, cbwDataXferLen - bytesRequested, CSW_STATUS_OK);
		sendCswResidue = cbwDataXferLen - bytesRequested;
		bytesRequested = cbwDataXferLen;
	}
	WritePacket(DNW_IN_ENDPOINT, xmitPtr, bytesRequested);

	longXferTag = cbwTag;
	if (bytesYetToXmit == 0) {
		InitCSW(longXferTag, 0, 0) ;
	}
}
#endif	// RAMDISK


static void ScsiWrite10( u32 cbwTag, u32 cbwDataXferLen, u8 * pCBW )
{
	int  offset;
	idb_putchar('w');
		// Check if cbwDataXferLen is 0.  If it is, check if the CDB's
		// transfer length is 0.  
		// If both are zero, ok (case 1).
		// If the CDB's transfer length is positive, error condition (case 3).
	if (0 == cbwDataXferLen)
	{
		u32 cdbXferLen;

		cdbXferLen	= BYTES_PER_SECTOR 
						* GetU16be( &pCBW[OFFSET_CBW_WRITE10_XFER_LEN] );
		if (0 == cdbXferLen) {	// ok; case 1.
			InitCSW(cbwTag, 0, CSW_STATUS_OK);
		}
		else {	// case 3
			InitCSW(cbwTag, 0, CSW_STATUS_RESET_RQD);
		}
		TransmitCSW();
		return;
	}
		// check that the CBW's flags byte indicates xfer OUT to device (0x00)
		// if it doesn't, we're in case #8 (Hi <> Do).  
		// Send 1 byte of (bogus) data, then stall the IN ep, and set the
		// CSW residue to cbwDataXferLen.
	if (pCBW[OFFSET_CBW_FLAGS] != 0x00) 
	{
		u8 bogus = 0;
		WritePacket(DNW_IN_ENDPOINT, &bogus, 1);
		stallInEPAfterData	  = 1;
		sendCswStatusResetRqd = 1;
		sendCswResidue		  = cbwDataXferLen;
		longXferTag			  = cbwTag;
		InitCSW(cbwTag, sendCswResidue, CSW_STATUS_RESET_RQD);
		return;
	}

		// If we reach this point, cbwDataXferLen is nonzero, and the
		// transfer direction is outbound.
	offset = BYTES_PER_SECTOR * GetU32be( &pCBW[OFFSET_CBW_WRITE10_LBA]);
#ifdef RAMDISK
	nextRcvePtr	   = GetRamDiskPtr(offset);
	bytesYetToRcve = BYTES_PER_SECTOR 
						* GetU16be(&pCBW[OFFSET_CBW_WRITE10_XFER_LEN]);

	if (cbwDataXferLen != bytesYetToRcve) {
		if (cbwDataXferLen < bytesYetToRcve) {	// case #13
				// Prepare to receive cbwDataXferLen bytes.
				// After receiving all the bytes, set
				// the CSW's status to 2 and the residue to 0.
			bytesYetToStore = cbwDataXferLen;
			bytesYetToRcve  = cbwDataXferLen;
			sendCswResidue  = 0;
			sendCswStatusResetRqd = 1;
			return;
		}
		else if (cbwDataXferLen > bytesYetToRcve) {	// Case #11
				// Prepare to receive cbwDataXferLen bytes but process only 
				// only bytesYetToRcve.  After receiving all the bytes, set
				// the CSW's status to 0 and the residue to 
				// cbwDataXferLen - bytesYetToRcve
			bytesYetToStore = bytesYetToRcve;
			sendCswResidue  = cbwDataXferLen - bytesYetToRcve;
			bytesYetToRcve  = cbwDataXferLen;
		}
	}
	else {
		bytesYetToStore = cbwDataXferLen;
	}
	longXferTag = cbwTag;
#else	// RAMDISK is not defined, so there's no media; 
		// We will store the data at an address	that is specified in a header 
		// at the beginning of the first transfer.  
		// If 'offset' is 0, this transfer contains the recovery kernel header.
		//		Set the status to 'receiving kernel'
		//		and remember the number of bytes we expect to receive.
		// If 'offset' is not 0, discard the downloaded data unless
		// kernelStatus indicates we're receiving a kernel.  In other words,
		// discard the downloaded data unless we've received a command with
		// offset zero and have not yet finished receiving the kernel (as
		// indicated by the header's "number of bytes" field).
	bytesYetToRcve	= cbwDataXferLen;
	bytesYetToStore = cbwDataXferLen;
	longXferTag		= cbwTag;
		// Check first for offset == 0
	if (offset == 0) {		// if this is the 1st chunk of a kernel
		nextRcvePtr	= packetBuffer;
		cbf_init( (void *)calc_SDRAM_ADDRESS() );
		SetKernelReceiving();
	}
		// else if we're not receiving a kernel, just discard the data
	else if ( !ReceivingRecoveryKernel() ) { 
		nextRcvePtr		= NULL;
		bytesYetToStore = 0;
		InitCSW(cbwTag, cbwDataXferLen, 0);	// Prepare to say we received all
											// and used none
	}
		// else there's no need to do anything, because nextRcvePtr is already
		// initialized from the previous CBW
#endif
}

static void ScsiUnrecognizedCommand( u32 cbwTag, u32 cbwDataXferLen, u8 * pCBW )
{
	idb_putchar('!');
	idb_byte(pCBW[OFFSET_CBW_CB_OPCODE]);

	SensePtr = senseInvalidOpcode;
	InitCSW(cbwTag, cbwDataXferLen, 1) ;
	if (0 == cbwDataXferLen) {	// if no data phase, just send CSW
		TransmitCSW();
	}
	else if (pCBW[OFFSET_CBW_FLAGS] & 0x80) {	// if host expects a response 
												//   in the data phase
		StallEndpoint(DNW_IN_ENDPOINT);			// the CSW will be sent after
												//   stall is cleared
	}
	else {	// Host expects to send data
			// Receive and discard all of it.
			// After receiving all the bytes, set the CSW's status to 1 
			// and the residue to cbwDataXferLen.
		PrepareToReceiveAndDiscard(cbwDataXferLen);
	}
}

static void ScsiVerify10( u32 cbwTag, u32 cbwDataXferLen, u8 * pCBW )
{
#ifndef RAMDISK
	idb_putchar('v');
		// We expect the CBW's xfer length to be 0.
		// If the xfer length is not zero, the device's response depends on
		// the direction bit in the CBW's flags byte.
		// If the direction bit indicates the host expects to receive data
		// (case #4), the device stalls the IN endpoint, sets the CSW's
		// residue to cbwDataXferLen, the CSW's status to 1, and the sensePtr
		// to senseInvalidParameter.
		// If the direction bit indicates the host expects to send data
		// (case #9), the device prepares to receive and discard cbwDataXferLen
		// bytes, set the CSW's residue to cbwDataXferLen, the CSW's status to
		// 1, and the sensePtr to senseInvalidParameter.
		// Then the routine returns.
	if (cbwDataXferLen != 0x00) 
	{
		if (0 == (pCBW[OFFSET_CBW_FLAGS] & 0x80)) {	// host expects to send data
			PrepareToReceiveAndDiscard(cbwDataXferLen);
			SensePtr = senseInvalidParameter;
			InitCSW(cbwTag, cbwDataXferLen, CSW_STATUS_FAILED);
			// we don't transmit the CSW now; it will be transmitted after
			// cbwDataXferLen bytes have been received from the host
		}
		else {	// else host expects to receive data (case #4)
			StallEndpoint(DNW_IN_ENDPOINT);
			SensePtr = senseInvalidParameter;
			InitCSW(cbwTag, cbwDataXferLen, CSW_STATUS_FAILED);
			// we don't transmit the CSW now; it will be transmitted after
			// the host clears the stall on the IN endpoint
		}
		return;
	}
	// The xfer length is 0, so we don't care about the direction flag.

	// Ensure the LUN is 0 and the ByteChk bit is 0 (no data will be sent 
	// for comparison with device contents).
	// If not, return a CSW status of 1, and indicate bad CDB contents 
	// (in sensePtr)
	if (pCBW[OFFSET_CBW_VERIFY10_LUN_BYTECHK] != 0x00)
	{
		SensePtr = senseInvalidFieldInCDB;
		InitCSW(cbwTag, 0, CSW_STATUS_FAILED);
		TransmitCSW();
		return;
	}

	// We ignore the logical block address and the verification length.
	// Instead we take the starting address and the verification length
	// from the most recently received Write(10) CBW that had a LBA of 0;
	// i.e., from the most recently received header.

	// If no Write(10) CBW with LBA of 0 has been received or we have not
	// downloaded a kernel, return CSW with status of 1, and indicate bad CDB
	// contents (in sensePtr).
	// If we have received a Write(10) CBW with LBA of 0 and downloaded a
	// kernel, verify the downloaded data according to the header info.
	// If it verifies ok, send a CSW with status 0.  Set the downloadVerified
	// flag, which will cause the boot code to jump to the downloaded kernel
	// after the CSW has been sent.

	if (ReceivedKernelOk())
	{	
		InitCSW(cbwTag, 0, CSW_STATUS_OK);
		downloadVerified = 1;
	}
	else if (ReceivedKernelError())
	{
			// If the downloaded data isn't verified ok, send a CSW with 
			// status 1, and indicate Read Error (in sensePtr).
		SensePtr = senseReadError;
		InitCSW(cbwTag, 0, CSW_STATUS_FAILED);
	}
	else
	{
		SensePtr = senseInvalidFieldInCDB;
		InitCSW(cbwTag, 0, CSW_STATUS_FAILED);
	}
	TransmitCSW();
#else	// RAMDISK is defined;
	// For now, just send a CSW with 'good' status.
	InitCSW(cbwTag, 0, 0);
	TransmitCSW();
#endif
}

static void ScsiTestUnitReady( u32 cbwTag, u32 cbwDataXferLen, u8 * pCBW  )
{
	idb_putchar('U');
	// Init the CSW residue to cbwDataXferLen
	// If there's a ramdisk, set the status to 0.
	// If htere's no media, set the status to 1
#ifdef RAMDISK
	InitCSW(cbwTag, cbwDataXferLen, 0) ;
#else
	InitCSW(cbwTag, cbwDataXferLen, 1);
	SensePtr = senseNoMedia;
#endif
	if (cbwDataXferLen == 0) {
		TransmitCSW();
	}
		// CBW's transfer length is > 0
	else if (0x80 & pCBW[OFFSET_CBW_FLAGS]) {	// CBW direction is IN; 
												//    case 4 (Hi > Dn)
		// Device will send no data; stall the BULK IN ep
		StallEndpoint(DNW_IN_ENDPOINT);
	}
	else {					// CBW direction is OUT; case 9 (Ho > Dn)
		PrepareToReceiveAndDiscard(cbwDataXferLen);
		idb_putchar('#');
	}
}

	// handles Start/Stop and Prevent/Allow-Removal and all others that
	// normally do nothing but send a CSW with status 0.
static void ScsiDoNothing( u32 cbwTag, u32 cbwDataXferLen, u8 * pCBW  )
{
	// Init the CSW residue to cbwDataXferLen and the status to 0.
	InitCSW(cbwTag, cbwDataXferLen, 0) ;
	if (cbwDataXferLen == 0) {	// this is the expected case
		TransmitCSW();
	}
		// CBW's transfer length is > 0
	else if (0x80 & pCBW[OFFSET_CBW_FLAGS]) {	// CBW direction is IN; 
												//    case 4 (Hi > Dn)
		// Device will send no data; stall the BULK IN ep
		StallEndpoint(DNW_IN_ENDPOINT);
	}
	else {					// CBW direction is OUT; case 9 (Ho > Dn)
		PrepareToReceiveAndDiscard(cbwDataXferLen);
		idb_putchar('#');
	}
}


#ifdef RAMDISK
static void ScsiModeSelect06( u32 cbwTag, u8 * pCBW )
{
	bytesYetToRcve	= pCBW[19];
	nextRcvePtr		= &modeSelectData[0];
	longXferTag		= cbwTag;
	if (bytesYetToRcve > MAX_MODE_SELECT_DATA_LEN) {
		InitCSW(longXferTag,0, 1) ;
		TransmitCSW();
		SensePtr = senseInvalidParameter;
	}
}
#endif	// RAMDISK

static inline int IsValidCBW(u8 * pCBW) 
{
	return (CBW_SIGNATURE == GetU32le( &pCBW[OFFSET_CBW_SIGNATURE] ));
}

static inline int IsMeaningfulCBW(u8 * pCBW)
{
	return (   (pCBW[OFFSET_CBW_LUN] == 0)				// LUN ought to be 0
			&& (pCBW[OFFSET_CBW_CB_LEN] >= 1)			// CBWCBLength must be
			&& (pCBW[OFFSET_CBW_CB_LEN] <= 16)			//   between 1 and 16
			&& (   (pCBW[OFFSET_CBW_FLAGS] == 0) 		// CBWFlags must be 
				|| (pCBW[OFFSET_CBW_FLAGS] == 0x80)) ); // 0 or 0x80
				 
}

/** ----------------------------------------------------------------------------
 * ProcessCBW - read and process a command block wrapper (CBW)
 *
 * @Parameters:	(void)
 *
 * @Returns:	(void)
 *
 * @Details: 	This routine supervises processing that reads, validates,
 *				parses, and processes a command block wrapper (CBW).
 *
 *	The routine first calls ReadBulkOutPacket() to read the putative CBW
 *	from the bulk out endpoint's fifo.  If ReadBulkOutPacket() did not read
 *	the expected number of bytes (SIZEOF_CBW) from the fifo, or if the bytes
 *	it read are not a valid and meaningful CBW, this routine stalls both the
 *	bulk IN and the bulk OUT endpoints, sets flags that indicate the endpoint
 *	stalls ought not to be cleared until the host has sent a MSC Reset request
 *	on endpoint 0, and calls InitCSW() to set the CSW's status to indicate
 *	that a reset is required.
 *
 *	If ReadBulkOutPacket() did read the expected number of bytes and they
 *	were a valid and meaningful CBW, this routine initializes some global
 *	variables, extracts the cbwTag and cbwXferLen from the received CBW,
 *	and dispatches further processing of the CBW according to its command code.
 *
 * NOTE: 21jul09
 *	Immediately following the routine's call to ReadBulkOutPacket() is some
 *	special purpose code that was added to allow the driver to pass the USB20CV
 *	MSC compliance test's Error Recovery Test.  Because of the hardcoded cbwTag
 *	value that is passed to InitCSW(), this code will not be useful for any
 *	purpose except passing the MSC compliance test.  On the other hand, I don't
 *	expect it to cause any further problems in the general case.
 *	The code was added because after the reset recovery sequence, the bulk OUT
 *	endpoint's BRCR register apparently contains the wrong value after a valid
 *	CBW is sent by the host.  In the test, the host sends a Test Unit Ready
 *	or Request Sense CBW, and the BRCR contains 0 or 1.
 *----------------------------------------------------------------------------*/
static void ProcessCBW()
{
	int numBytesRead;
	u8  tempbuf[1 + SIZEOF_CBW];

		// Whether or not the CBW is valid, we leave the 'AwaitingCBW' state
	msd_state = MSD_STATE_DATA_IN;

	bytesYetToRcve = 1 + SIZEOF_CBW;
	numBytesRead   = ReadBulkOutPacket((void *)tempbuf);
#if 1	// 15jul09	This is special purpose code (a hack) whose only purpose
		//			is to allow the code to pass the USB20CV MSC compliance
		//			test's Error Recovery Test.
		//  This appears to be needed because of the problems with BRCR after
		//  receiving a bad CBW, stalling both endpoints, and eventually
		//	processing a BOT MSC Reset sequence.
#ifdef RAMDISK  // This hack is used only for the MSC compliance test,
                // which is run only when RAMDISK is defined
	if (   (0 == numBytesRead)
		|| (1 == numBytesRead)) {

		ClearEndpointInterrupt(DNW_OUT_ENDPOINT);

		// Clear the RPS status bit and flush the fifo
		// stall the IN endpoint
		// construct a CSW w/ status Reset Required
		WriteIndexedReg(DNW_IN_ENDPOINT, GEP_STS_REG_OFFSET, GEP_STS_RPS_BIT);
		SetIndexedRegBits(DNW_OUT_ENDPOINT, GEP_CTL_REG_OFFSET, 
											GEP_CTL_FLUSH_BIT);

		StallEndpoint(DNW_IN_ENDPOINT);
		InitCSW( 0x4c455943, 0, 1); // canned tag
		SensePtr = senseNotReady;
	}
	else
#endif  // RAMDISK
#endif	// 15jul09
	if (   (numBytesRead == SIZEOF_CBW)
		&& IsValidCBW(tempbuf)
		&& IsMeaningfulCBW(tempbuf))
	{
		u32		cbwTag;	
		u32		cbwDataXferLen;

		sendCswStatusResetRqd = 0;
		sendCswResidue		  = 0;
		stallInEPAfterData	  = 0;
		csw_ready			  = 0;

		idb_putchar('B');
		cbwTag		   = GetU32le( &tempbuf[OFFSET_CBW_TAG]);
		cbwDataXferLen = GetU32le( &tempbuf[OFFSET_CBW_DATA_XFER_LEN]);

		switch (tempbuf[OFFSET_CBW_CB_OPCODE]) {	// the CDB's opcode
		case TEST_UNIT_READY:	// 0x00
			ScsiTestUnitReady( cbwTag, cbwDataXferLen, tempbuf );
			break;

		case REQUEST_SENSE:	
			ScsiRequestSense( cbwTag, cbwDataXferLen, tempbuf );
			break;

		case INQUIRY:
			idb_putchar('I');
			StdScsiInputRequest( cbwTag, cbwDataXferLen, tempbuf,
								 device_info, device_info_size);
			break;

#ifdef RAMDISK
		case MODE_SELECT_06:			// 0x15
			ScsiModeSelect06( cbwTag, tempbuf );
			break;

		case MODE_SENSE_06:				// 0x1A
			StdScsiInputRequest( cbwTag, cbwDataXferLen, tempbuf,
								 mode_sense6_response, 
								 sizeof(mode_sense6_response));
			break;

		case READ_FORMAT_CAPACITIES:	// 0x23	
		case READ_CAPACITY:				// 0x25
			StdScsiInputRequest( cbwTag, cbwDataXferLen, tempbuf,
								 device_capacity, sizeof(device_capacity));
			break;

		case READ_10:					// 0x28
			ScsiRead10( cbwTag, cbwDataXferLen, tempbuf );
			break;

#else	// RAMDISK not defined
		case MODE_SENSE_06:				// 0x1A
		case READ_FORMAT_CAPACITIES:	// 0x23	
		case READ_CAPACITY:				// 0x25
		case READ_10:					// 0x28
			ScsiMeaningless(cbwTag, cbwDataXferLen, tempbuf);
			break;

#endif

		case STOP_START_UNIT:				// 0x1B
		case PREVENT_ALLOW_MEDIUM_REMOVAL:	// 0x1E
			ScsiDoNothing(cbwTag, cbwDataXferLen, tempbuf);
			break;

		case WRITE_10:	// 0x2A
			ScsiWrite10( cbwTag, cbwDataXferLen, tempbuf );
			break;

		case VERIFY_10:	// 0x2F
			ScsiVerify10(cbwTag, cbwDataXferLen, tempbuf );
			break;

		default:
			ScsiUnrecognizedCommand( cbwTag, cbwDataXferLen, tempbuf );
			break;
		}
	}
	else {	// Invalid or meaningless CBW.  Stall both bulk endpoints
		awaitingRecoveryReset[DNW_IN_ENDPOINT]  = 1;
		awaitingRecoveryReset[DNW_OUT_ENDPOINT] = 1;
		StallEndpoint(DNW_IN_ENDPOINT);
		StallEndpoint(DNW_OUT_ENDPOINT);
		InitCSW( GetU32le( &tempbuf[4]), 0, CSW_STATUS_RESET_RQD);

		idb_puts("\nBad CBW:");
		idb_int(numBytesRead);
		idb_puts(": ");
		int b;
		for (b = 0; b < numBytesRead; ++b) {
			idb_byte(tempbuf[b]); idb_putchar(' ');
		}
		idb_puts("\n");
	}
}

/** ----------------------------------------------------------------------------
 * RX_EndpointHandler - handle RX interrupts on the Outbound bulk endpoint
 *
 * @Parameters:	(void)
 *
 * @Returns:	(void)
 *
 * @Details: 	This routine supervises processing that deals with reception
 *				interrupts on ep2, the Outbound bulk endpoint.
 *
 *	The routine first checks if the device has received a CBW and is waiting
 *	for subsequent outbound data.  If not, this packet probably contains a
 *	CBW, so the routine calls ProcessCBW() to attempt to read, validate, and
 *	process a CBW.
 *
 *  If the device has received a CBW and is waiting for subsequent outbound
 *	data, the routine calls ReadBulkOutPacket() to read the received packet
 *	into a buffer.  After ReadBulkOutPacket() returns, this routine updates 
 *	the number of bytes yet to receive and the number of bytes yet to store.
 *	If the device discarded all the just-received data (as indicated by 
 *	nextRcvePtr equal to NULL), it does not update nextRcvePtr.  
 *	If the device is receiving an update kernel from the Host, 
 *	cbf_process_chunk() updates nextRcvePtr.  Depending on the value returned
 *	by cbf_process_chunk(), this routine updates the status of the download
 *	process.
 *	In other cases, the routine increments nextRcvePtr by the number of bytes
 *	just stored.
 *
 *	If the just-received packet is the last of the transmission, this routine
 *	calls InitCSW() to construct the CSW and then calls TransmitCSW() to write
 *	it to the fifo.
 *----------------------------------------------------------------------------*/

void RX_EndpointHandler( void )
{
	if (msd_state == MSD_STATE_AWAITING_CBW) 
	{
		ProcessCBW();
	}
	else // Receiving data
	{
		int		numBytesRead;

		if (nextRcvePtr == NULL ) {	// if we're discarding all bytes
			numBytesRead    = ReadBulkOutPacket(packetBuffer);
			if (bytesYetToRcve == 0) {	// we've rcvd all we expected, send
				TransmitCSW();			// the CSW, which was init'd earlier.
			}
		}
		else {	// else we're not discarding ALL received bytes
			u32     offset;	// add this to nextRcvePtr (in some cases)

				// NOTE: this check is closely related to ReadBulkOutPacket's
				//		 calculation of the max number of bytes to read.
			if (   (bytesYetToRcve  <= bytesYetToStore) 
				|| (BULK_PacketSize <= bytesYetToStore) )
			{	// if we'll store all these bytes
				numBytesRead = ReadBulkOutPacket(nextRcvePtr);
				offset			 = numBytesRead;
				bytesYetToStore -= numBytesRead;
			}
			else {	// we'll store only some of the bytes; 
					// first read all of them into packetBuffer, then copy
					// bytesYetToStore to nextRcvePtr
				int i;
				int limit;
				u8 * pd;
			
				numBytesRead = ReadBulkOutPacket(packetBuffer);
				limit = (numBytesRead < bytesYetToStore) ? numBytesRead
													     : bytesYetToStore;
				for (i = 0, pd = nextRcvePtr; i < limit; ++i) {
					*pd++ = packetBuffer[i];
				}
				offset			 = limit;
				bytesYetToStore -= limit;
			}
#ifndef RAMDISK
			// If we are receiving the update kernel download.
			if ( ReceivingRecoveryKernel() ) 
			{
				int	cbf_status = cbf_process_chunk(nextRcvePtr, numBytesRead, 
	                                               &nextRcvePtr );

				switch (cbf_status) {
					case -1:	// error detected
						SetKernelReceivedError();
						break;

					case 0:		// download completed and validated ok
						SetKernelReceivedOk();
						break;
	
					default:
					case 1:		// cbf thinks there's more to receive
						break;
				}
			}
				// if we're receiving something besides a recovery kernel,
				// update nextRcvePtr.
			else if (!ReceivedKernelOk() && !ReceivedKernelError()) {
				if (nextRcvePtr != NULL) {	// if not discarding it
					nextRcvePtr += offset;	// update pointer for next chunk
				}
			}
			if (bytesYetToRcve == 0) {	// we've rcvd all we expected
				InitCSW(longXferTag, 0, 0) ;
				TransmitCSW();
			}
#else	// RAMDISK is defined; update the addr where we'll store the next chunk
			nextRcvePtr += offset;

			//DecrementCSWResidue( numBytesRead );
			if (bytesYetToRcve == 0) {
				InitCSW(longXferTag, 0, 0) ;
				TransmitCSW();
			}
#endif
			idb_putchar('r');
		}
	}
}



/** ----------------------------------------------------------------------------
 * HandleEndpoint1Event - handle interrupts caused by activity on ep1
 *
 * @Parameters:	(void)
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine supervises processing that deals with interrupts
 *				that are caused by activity on ep1, the bulk IN endpoint.
 *	The routine first reads the endpoint's Endpoint Status register, whose
 *	bits indicate the endpoint status and the reason for the interrupt.
 *
 *	If the status register's TPS bit is set, indicating that a complete packet
 *	has been sent from the fifo to the Host, this routine writes a '1' to the
 *	register's TPS bit in order to clear it.  Then it checks if the endpoint
 *	ought to be stalled after all of the current transfer's data has been sent.
 *	(The endpoint ought to be stalled if the final packet contains less data
 *	than the Host expects.)   If the endpoint ought to be stalled, this routine
 *	checks if there are more bytes to transfer.  If there are, it calls
 *	WritePacket() to send the next packet.  If all of the transfer's bytes
 *	have been sent, this routine calls StallEndpoint() to stall ep1 and calls
 *	InitCSW() to initialize the CSW that will be sent after the Host clears the
 *	stall.
 *	If the endpoint ought not to be stalled after all the transfer's bytes have
 *	been sent, the routine calls TransferCSW(), which checks if a CSW is 
 *	available for transmission and, if available, sends it.  If TransferCSW()
 *	did not send a CSW, this routine checks if there are more bytes to be sent
 *	as part of the transfer.  If there are, it calls WritePacket() to store 
 *	them in the endpoint's fifo for transmission.  After WritePacket() returns,
 *	this routine updates the number of bytes yet to be sent and the pointer to
 *	the next byte.  If there are no more bytes to be sent, this routine calls
 *	InitCSW() to initialize the CSW, which will be sent after the just-written
 *	packet has been transferred.
 *	If the downloadVerified flag is set, this routine calls JumpToKernel().
 *	The flag is set by the code that processes a scsi Verify command when it
 *	finds that the downloaded kernel was verified as ok.  That code also
 *	initializes and transmits a CSW.  This routine is entered after the CSW
 *	has been transferred to the Host and acknowledged.  That's when it calls
 *	JumpToKernel.
 *
 *	If the status register's TPS bit is not set, this routine checks if the
 *	FSC (functional stall condition) flag is set.  If it is set, the routine
 *	writes a '1' to the bit in order to clear it.
 *
 * NOTE: JumpToKernel() is called from this routine while an interrupt
 *		 is being serviced.  JumpToKernel() disables interrupts and disables
 * the caches.  Apparently the kernel (or the code that decompresses it) takes
 * care of putting the processor in the correct mode.
 * If you suspect that this approach is causing problems, you can add a new
 * variable (downloadVerifyConfirmed), set it to zero in usb_init(), and set
 * it nonzero in this routine where it now calls JumpToKernel().  Also change
 * the background loop in wait_for_usb_host() to check if the new variable is
 * nonzero.  When it's nonzero, call JumpToKernel().
 *----------------------------------------------------------------------------*/
static void
HandleEndpoint1Event()	// Bulk IN ep
{
	u16 bEpIrqStat = ReadIndexedReg(DNW_IN_ENDPOINT, GEP_STS_REG_OFFSET);

	idb_putchar('?');

	idb_byte((char)bEpIrqStat);
	idb_putchar('.');

	if (bEpIrqStat & GEP_STS_TPS_BIT)
	{
		idle_time = now();	// Reset idle timeout counter -- we're active
		idb_putchar('t');
			// Clear the TPS bit
		WriteIndexedReg(DNW_IN_ENDPOINT, GEP_STS_REG_OFFSET, GEP_STS_TPS_BIT);

		if (stallInEPAfterData) {	// data phase ends with 0-length or short pkt
			idb_putchar('l');
			if (bytesYetToXmit > 0) {
				HandleBulkInTx();
			}
			else {
				idb_putchar('2');
				InitCSW(longXferTag, sendCswResidue, 0) ;
				longXferTag = 0;
				StallEndpoint(DNW_IN_ENDPOINT);
				stallInEPAfterData = 0;
			}
		}
		else if (!TransmitCSW())
		{
			idb_putchar('a');
			if (bytesYetToXmit > 0) {
				HandleBulkInTx();
				if (bytesYetToXmit == 0) {
					idb_putchar('c');
					InitCSW(longXferTag, 0, 0) ;
					longXferTag = 0;
				}
				else idb_putchar('d');
			}
			else idb_putchar('e');
		}
		else idb_putchar('j');

#ifndef RAMDISK
		if (downloadVerified) {
			idb_puts("Jump to kernel after verify\n");
			JumpToKernel();
		}
#endif	// RAMDISK
	}
	// else why are we here? Error bit?
	else {
		idb_putchar('v');
		if (bEpIrqStat & GEP_STS_FSC_BIT) {
				// write 1 to only the FSC bit in the endpoint's status register
			WriteIndexedReg(DNW_IN_ENDPOINT, 
							GEP_STS_REG_OFFSET, GEP_STS_FSC_BIT);
		}
	}
	idb_putchar('|');
}

/** ----------------------------------------------------------------------------
 * HandleEndpoint2Event - handle interrupts caused by activity on ep2
 *
 * @Parameters:	(void)
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine supervises processing that deals with interrupts
 *				that are caused by activity on ep2, the bulk OUT endpoint.
 *	The routine first reads the endpoint's Endpoint Status register, whose
 *	bits indicate the endpoint status and the reason for the interrupt.
 *
 *	If the status register bits indicate a complete packet has been received
 *	into the UDC's fifo, the routine calls RX_EndpointHandler() to read the 
 *	packet from the fifo and deal with it.
 *
 *	If the status register bits do not indicate a complete packet has been
 *	received, the routine clears the endpoint's bit in the UDC's interrupt 
 *	register.  If the status register's FSC (functional stall condition) bit
 *	or FFS (fifo flushed) bit is set, this routine writes '1' to the bit(s)
 *	to clear them.
 *----------------------------------------------------------------------------*/

static void
HandleEndpoint2Event() // Bulk OUT ep
{
	u16 bEpIrqStat = ReadIndexedReg(DNW_OUT_ENDPOINT, GEP_STS_REG_OFFSET);

	idb_putchar('!');
	idb_byte((char)bEpIrqStat);
	idb_putchar('.');

		// receive packet complete && receive fifo not empty 
		// ==> out token + data rx event
	if (bEpIrqStat & GEP_STS_RPS_BIT) {
		idle_time = now();	// Reset idle timeout counter -- we're active
		idb_putchar('R');
		RX_EndpointHandler();
	}
	else {
		ClearEndpointInterrupt(DNW_OUT_ENDPOINT);
			// else: why else would we be here?
		if (bEpIrqStat & GEP_STS_FSC_BIT) {
				// write 1 to only the FSC bit in the endpoint's status register
			WriteIndexedReg(DNW_OUT_ENDPOINT, 
							GEP_STS_REG_OFFSET, GEP_STS_FSC_BIT);
		}
		if (bEpIrqStat & GEP_STS_FFS_BIT) {
				// write 1 to only the FFS bit in the endpoint's status register
			WriteIndexedReg(DNW_OUT_ENDPOINT, 
							GEP_STS_REG_OFFSET, GEP_STS_FFS_BIT);
		}
		idb_putchar('}');
	}
}



/** ----------------------------------------------------------------------------
 * HandleUSBBusIrq - handle interrupts that are due to errors 
 *					 or exceptional conditions
 *
 * @Parameters:	bUSBBusIrqStat - the value read from the UDC's System Status
 *								 register.
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine supervises processing that deals with USB bus
 *				errors and other exceptional conditions that are indicated
 *	by the bits in the UDC's System Status register.  Here's a list of the
 *	errors and conditions it deals with.
 *		Host-forced Reset
 *		Host-forced Suspend
 *		Host-forced Resume
 *		Completion of the bus speed detection handshake process
 *		Toggle Bit Mismatch
 *		VBUS change (to On or Off)
 *		EB Overrun error
 *		Data CRC error
 *		Token CRC error
 *		Bit Stuff error
 *		Timeout error
 *		Byte Align error
 *
 *	For most of the errors and conditions, this routine just writes '1's to the 
 *	corresponding bits in the UDC's System Status register in order to clear 
 *	them.
 *
 *	When a host-forced reset is detected, this routine calls reset_vars() to
 *	set variables to their reset values.
 *
 *	When the bus speed detection handshake process has completed, this routine
 *	updates variables that indicate the bus speed, the max packet sizes on
 *	ep0, ep1, and ep2, and the address of the configuration data that's 
 *	appropriate for the detected bus speed.  It also writes the maximum
 *	packet sizes to the UDC Max Packet Size registers.
 *
 *	When a VBUS change is detected, this routine reads the Test register to 
 *	determine the current VBUS level and sets vbusStatus accordingly.
 *----------------------------------------------------------------------------*/
static void
HandleUSBBusIrq(u16 bUSBBusIrqStat)
{
	if (bUSBBusIrqStat & USB_INT_HFRES_BIT) {
		reset_vars();
	}
	if (bUSBBusIrqStat & USB_INT_SDE_BIT) {
		if (bUSBBusIrqStat & USB_INT_HSP_BIT) {
			EP0_PacketSize   			= HIGHSPEED_EP0_PKT_SIZE;
			BULK_PacketSize				= HIGHSPEED_BULK_PKT_SIZE;
			sc_USBDevDescriptor			= sc_HighSpeedDeviceDescriptor;
			sc_USBConfigData			= sc_HighSpeedUSBConfigData;
			sc_USBDevQualifierData		= sc_HighSpeedDeviceQualifierDescriptor;
			sc_USBOtherSpeedConfigData	= sc_HighSpeedOtherSpeedConfigData;
			idb_putchar('H');
		}
		else {
			EP0_PacketSize	 			= FULLSPEED_EP0_PKT_SIZE;
			BULK_PacketSize  			= FULLSPEED_BULK_PKT_SIZE;
			sc_USBDevDescriptor			= sc_FullSpeedDeviceDescriptor;
			sc_USBConfigData			= sc_FullSpeedUSBConfigData;
			sc_USBDevQualifierData		= sc_FullSpeedDeviceQualifierDescriptor;
			sc_USBOtherSpeedConfigData	= sc_FullSpeedOtherSpeedConfigData;
			idb_putchar('F');
		}
		WriteIndexedReg(0, GEP_MPR_REG_OFFSET, EP0_PacketSize);
		WriteIndexedReg(DNW_IN_ENDPOINT, GEP_MPR_REG_OFFSET, BULK_PacketSize);
		WriteIndexedReg(DNW_OUT_ENDPOINT, GEP_MPR_REG_OFFSET, BULK_PacketSize);
	}
		// If there's a VBUS-related interrupt, base actions on the status of
		// TEST reg's VBUS bit.  
		// if it's set, act as though a VBUSON interrupt has occurred
		// if it's cleared, act as though a VBUSOFF interrupt has occurred
		// We do this because tests showed cases where both USB_INT_VBUSON_BIT
		// and USB_INT_VBUSOFF_BIT were set.  In those cases we need to check
		// the TEST register's VBUS bit.  To simplify the code, we just check
		// it in all cases.
	if ( bUSBBusIrqStat & (USB_INT_VBUSOFF_BIT | USB_INT_VBUSON_BIT))
	{
		u16 wTstReg	= ReadReg(TEST_REG_OFFSET);
		idb_putchar('V');
		if (wTstReg & USB_TEST_VBUS) {
			if (vbusStatus == VBUS_STATUS_OFF) {
				vbusStatus = VBUS_STATUS_ON;
				//idb_putchar('+');
				// After VBUS goes on, we expect the host will Reset the
				// connection.  When we detect the reset, we'll initialize
				// other variables.
			}
		}
		else {
			if (vbusStatus == VBUS_STATUS_ON) {
				vbusStatus = VBUS_STATUS_OFF;
				//idb_putchar('-');
				// I think there's no need to change any other variables, 
				// because we won't receive anything on USB while VBUS is off.
			}
		}
	}
}

/** ----------------------------------------------------------------------------
 * HandleUDC_Interrupt - USB Device Controller interrupt handler
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine supervises processing of UDC interrupts.
 *
 *	First it reads 3 of the LF1000's UDC registers in order to determine the
 *	reason(s) for the interrupt.  Specifically it reads the
 *		Endpoint Interrupt register, which contains interrupt flags for the
 *									 three endpoints (0, 1, and 2);
 *		System Status register, whose bits indicate various USB bus error and
 *									 status conditions;
 *		Test register, which has bits that indicate EB Underrun error and PID
 *									 error.
 *	
 *	If the System Status register's bits indicate detection of an error, a
 *	change in VBUS status, or a USB Host-forced reset, suspend, or resume, 
 *	this routine calls HandleUSBBusIrq() to deal with the situation.
 *
 *	If the Test register's bits indicate either an EB Underrun error or a PID
 *	error, this routine sets those bits in the Test register.
 *
 *	If the Endpoint Interrupt register indicates an interrupt on one or more
 *	of the endpoints, this routine checks each of the 3 endpoints for an
 *	interrupt.  If endpoint 0 has interrupted, this routine calls 
 *	HandleEndpoint0Event() to deal with the situation.  If endpoint 1 or 
 *	endpoint 2 has interrupted, this routine calls HandleEndpoint1Event() 
 *	or HandleEndpoint2Event() to supervise appropriate processing.  
 *----------------------------------------------------------------------------*/
static void HandleUDC_Interrupt(void)
{
	u16 bEpIrqStat		= ReadReg(EP_INT_REG_OFFSET);
	u16 bUSBBusIrqStat	= ReadReg(USB_INT_REG_OFFSET);
		// Immediately clear the System Status register's interrupt bits
	WriteReg(USB_INT_REG_OFFSET, bUSBBusIrqStat);
	u16 wTstReg			= ReadReg(TEST_REG_OFFSET);

	//idb_putchar('+');
	if (bUSBBusIrqStat & (  USB_INT_BAERR_BIT		// byte align error 
						  	| USB_INT_TMERR_BIT		// timeout error
							| USB_INT_BSERR_BIT 	// bit stuff error
							| USB_INT_TCERR_BIT		// token crc error
							| USB_INT_DCERR_BIT 	// data crc error
							| USB_INT_EOERR_BIT		// EB Overrun error
							| USB_INT_VBUSOFF_BIT 	// VBUS Off
							| USB_INT_VBUSON_BIT	// VBUS On
							| USB_INT_TBM_BIT   	// toggle bit mismatch
							| USB_INT_SDE_BIT		// speed detect'n end
							| USB_INT_HFRM_BIT  	// host forced resume
							| USB_INT_HFSUSP_BIT	// host forced suspend
							| USB_INT_HFRES_BIT) ) 	// host forced reset
	{
		idb_putchar('@');
		idb_16(bUSBBusIrqStat);
		HandleUSBBusIrq(bUSBBusIrqStat);
	}
	if(wTstReg & (USB_TEST_EUERR 				// EB Underrun error
				  | USB_TEST_PERR))				// PID error
	{
		//idb_putchar('-');
		SetRegBits(TEST_REG_OFFSET, 
					wTstReg & (USB_TEST_EUERR | USB_TEST_PERR));
		// call SetRegBits() instead of WriteReg() because the TEST register
		// has some bits that are R/W, and we don't want to change their
		// values.  The EUERR and PERR bits are R/C.
	}
	if (bEpIrqStat & ANY_EP_INT_FLAGS) {
		idb_putchar('<');
		if (bEpIrqStat & EP0_INT_FLAG) {
			idb_putchar('0');
			WriteReg(EP_INT_REG_OFFSET, EP0_INT_FLAG);
			HandleEndpoint0Event();
		}
		if (bEpIrqStat & EP1_INT_FLAG) {
			idb_putchar('1');
				// clear the interrupt flag in case another occurs while
				// sevicing this one.
			WriteReg(EP_INT_REG_OFFSET, EP1_INT_FLAG);
				// then handle this interrupt
			HandleEndpoint1Event();
		}
		if (bEpIrqStat & EP2_INT_FLAG) {
			idb_putchar('2');
			HandleEndpoint2Event();
				// ep2's interrupt flag is cleared in HandleEndpoint2Event() 
				// or ReadBulkOutPacket().
		}
	}
}

/** ----------------------------------------------------------------------------
 * LF_IRQHandler - main interrupt service routine
 *
 * @Returns:	(void).
 *
 * @Details: 	This routine is called from the primary IRQ interrupt handler,
 *				IRQHandler, to which the IRQ vector points.
 *	After reading the LF1000's interrupt pending registers (INTPENDL and
 *	INTPENDH), the routine checks their bits to determine whether or not 
 *	a USB Device Controller interrupt has occurred.  When it detects a UDC 
 *	interrupt, this routine calls HandleUDC_Interrupt() to process it.
 *
 *	Before returning, this routine clears the interrupt pending flags by writing
 *	back to the INTPENDL/H registers the values it previously read from them.
 *----------------------------------------------------------------------------*/
void LF_IRQHandler() {
	unsigned int pendL;
	unsigned int pendH;

	pendL = REG32(IC_BASE+INTPENDL);
	pendH = REG32(IC_BASE+INTPENDH);

		// If it's a UDC interrupt
	if (pendL & 0x00100000)
	{
		HandleUDC_Interrupt();
	}
	else idb_putchar(']');

		// Clear the pending interrupt flags
	REG32(IC_BASE+INTPENDL) = pendL;
	REG32(IC_BASE+INTPENDH) = pendH;

	//idb_putchar('*');
}

#ifdef RAMDISK
int GetJumpButton(void) {
	int button_l;
	global_var * gptr = get_global();

	switch(gptr->board_id) {
	case LF1000_BOARD_DEV:
		button_l = !gpio_get_val(DEV_BUTTON_PAUSE_PORT, DEV_BUTTON_PAUSE_PIN);
		break;
	case LF1000_BOARD_DIDJ:
	case LF1000_BOARD_DIDJ_09:
		button_l = !gpio_get_val(DIDJ_BUTTON_PAUSE_PORT, DIDJ_BUTTON_PAUSE_PIN);
		break;
	case LF1000_BOARD_ACORN:
		button_l = !gpio_get_val(ACORN_BUTTON_PAUSE_PORT, ACORN_BUTTON_PAUSE_PIN);
		break;
    case LF1000_BOARD_EMERALD_NOTV_NOCAP:
    case LF1000_BOARD_EMERALD_TV_NOCAP:
    case LF1000_BOARD_EMERALD_NOTV_CAP:
	    button_l = !gpio_get_val(EMERALD_BUTTON_PAUSE_PORT,
                                 EMERALD_BUTTON_PAUSE_PIN);
		break;
	default:
		db_puts("unknown board type\n");
		button_l = 0;
		break;
	}
	return button_l;
}	
#endif


/** ----------------------------------------------------------------------------
 * wait_for_usb_host - main background loop during a recovery (USB) boot
 *
 * @Returns:	Does not return.
 *
 * @Details: 	This routine contains the main background loop that is 
 *				executed during a recovery boot.
 * Before entering the loop, the routine calls usb_init() to configure the
 * LF1000's USB device controller.  
 *
 * Then it configures the LF1000's interrupt controller so all interrupt
 * sources use normal interrupt mode and so all interrupt sources except
 * the UDC are masked.
 *
 * After enabling interrupts and initializing a few variables, the routine
 * then enters the background loop.
 *
 * The loop code monitors the state of the power button and the system's idle
 * time.  When the power button has been pressed for about 4 seconds, or when
 * the system has been idle for about 5 minutes, the code calls die(), which
 * powers off the unit.
 *
 * The loop code also checks for connection to a USB host.  When it detects 
 * a connection, it checks if the "Connecting to PC" screen has been displayed.
 * If it has not yet been displayed, the code calls show_download_in_progress()
 * to display it.
 *
 * If the boot code was built to support a ramdisk, the loop code checks if
 * the "find and jump to kernel" button has been pressed.  If it has, the
 * code calls CopyKernelBinFromRamdiskThruCbf() to find the downloaded
 * kernel file and copy it to another part of ram.  If the file was found and
 * copied, this routine calls JumpToKernel(), which returns only if it
 * detects an error before jumping to the kernel.
 *----------------------------------------------------------------------------*/

void wait_for_usb_host(struct display_module *disp)
{
            // power_button_debouncer is initialized to -1.
            // when this code detects the power button as unpressed,
            // it sets power_button_debouncer to zero.
            // After that, whenever the code detects the power button pressed,
            // it increments power_button_debouncer.  When its value
            // equals a threshold, the code calls die() to power off the 
            // device.
	int     power_button_debouncer;
	u32	loop_counter;
	u32	l_shldr_btn_pressed;
	u32	cur_time;
	int	power_button;

	global_var * gptr = get_global();	/* point at global data area */


	/* Configure the LF1000's UDC */
	usb_init();
	db_puts("After usb_init()...\n");

		/* Now set the interrupt mode register to 0.
		 * This makes all interrupt sources use Normal interrupt mode (IRQ)
		 * instead of Fast interrupt mode (FIQ).
		 */
	REG32(IC_BASE+INTMODEL) = 0;
	REG32(IC_BASE+INTMODEH) = 0;
	db_puts("...1\n");
		/* Mask all interrupts except UDC           */
	REG32(IC_BASE+INTMASKL) = ~(1 << 20);
	REG32(IC_BASE+INTMASKH) = ~0;
	db_puts("...2\n");

		/* Enable interrupts                        */
	EnableInterrupts();
	loop_counter = 0;
    power_button_debouncer = -1;
	idle_time = now ();

    db_puts("Initial idle_time: "); db_int(idle_time); db_puts("\n");

	l_shldr_btn_pressed = 0;
	while (1) {
#ifdef RAMDISK
		if ((loop_counter & 0x01ffff) == 0) {
			if (GetJumpButton()) {
				if (!l_shldr_btn_pressed) {
					// show_update_in_progress ();
					db_puts("L\n");
					++l_shldr_btn_pressed;
					if (0 == CopyKernelBinFromRamdiskThruCbf()) 
					{
						// if we found kernel.bin on the ramdisk
						// and copied it to 0x8000, prepare to jump to it
						// and then jump.
						db_puts("Copied " USB_PAYLOAD_NAME "\n");
						db_puts("Ready to jump...\n");
						JumpToKernel();
					}
					else {
						db_puts("Didn't find and copy " USB_PAYLOAD_NAME "\n");
						db_puts("Continuing...\n");
					}
				}
				// else ignore the press until the button has been released
			}
			else {	// the button is not pressed
				l_shldr_btn_pressed = 0;
			}
		}
		else 
#endif

		{	// output debug info that's been generated while handling interrupts
			idb_output();
		}
		if ((++loop_counter & 0x01ffff) == 0) {
			//db_puts(">>\n");

		// If the power button is pressed, check if it has been released
		// and if it was pressed during the previous pass through here.
		// (This provides a little debouncing of the button press.)  
            // If it was, call die() to power off.

			if (gptr->use_lfp100)
				power_button = lfp100_get_power_button();
			else
				power_button =
					gpio_get_val(POWER_PORT, POWER_PIN);

			if (power_button)
			{

#define POWER_OFF_THRESHOLD 2
/* 2 is the smallest value that provides any debouncing */
				if (   (power_button_debouncer >= 0)
                    && (++power_button_debouncer >= POWER_OFF_THRESHOLD))
				{
					die ();
				}
			}
			else
			{
                power_button_debouncer = 0;
			}
			// Check for idle timeout of 5 minutes = 300 seconds
            cur_time = now();
			if ((cur_time > idle_time) && (cur_time - idle_time > 300))
			{
				die ();
			}
            //db_puts("now: "); db_int(cur_time); db_puts("\n");
		}

		// Show a new screen if there was a change: timeout, or vbus changing state
        cur_time = now();
#ifdef DEBUG
        if (cur_time < idle_time) {
            db_puts("Found cur_time < idle_time\n");
        }
#endif
		if (   (shown_screen == SCR_PROGRESS)
            && (cur_time > idle_time)
            && (cur_time - idle_time > 45))
		{
            serio_puts("Show_visit trigger; cur_time "); serio_int( cur_time );
            serio_puts("; idle_time: "); serio_int(idle_time); serio_puts("\n");

			show_visit ();
			display_init();
			shown_screen = SCR_VISIT;
		}
		else if (vbusStatus == VBUS_STATUS_OFF)
		{
			if (shown_screen != SCR_ATTENTION)
			{
				show_attention_needed ();
				shown_screen = SCR_ATTENTION;
			}
		}
		else
		{
			if (shown_screen == SCR_ATTENTION)
			{
				show_download_in_progress ();
				display_init();
				shown_screen = SCR_PROGRESS;
				idle_time = now ();
			}
		}
	}
}


