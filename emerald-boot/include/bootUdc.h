/* bootUdc.h  -- LF1000 USB Device Controller
 *
 * Copyright 2009-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef BOOTUDC_H
#define BOOTUDC_H

/* NOTE: Some of the symbols defined in this file come from Leapfrog's
 *		 Linux files.  Others come from source files we received from
 * MagicEyes.  The fastest route to initial implementation was to retain
 * symbols from both sources.  Some time we might want to make things
 * consistent with our other Linux source code.
 */

//#define LF1000_UDC_END		0xC0018880
#define LF1000_UDC_IRQ		20

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// These #defines are taken from lf1000_udc.h in order to eliminate the need
// to include it.
#define UDC_USERTEST	(0x84C)

/* Hardware register, offset, and value definitions */
#define UDC_EPINDEX		(0x0)

#define UDC_EPINT		(0x2)
#define EP3INT			(3)
#define EP2INT			(2)
#define EP1INT			(1)
#define EP0INT			(0)

#define UDC_EPINTEN		(0x4)
#define EP3INTEN		(3)
#define EP2INTEN		(2)
#define EP1INTEN		(1)
#define EP0INTEN		(0)

#define UDC_FUNCADDR		(0x6)
#define UDC_FRAMENUM		(0x8)

#define UDC_EPDIR		(0xA)
#define EP1DS			(0x1)
#define EP2DS			(0x2)
#define EP3DS			(0x3)

#define UDC_TEST		(0xC)
#define VBUS			(15)
#define EUERR			(13)
#define PERR			(12)
#define FDWR			(11)
#define SPDSEL			(6)
#define TMD			(4)
#define TEST_TPS		(3)
#define TKS			(2)
#define TJS			(1)
#define TSNS			(0)

#define UDC_SYSSTAT		(0xE)
#define BAERR			(15)
#define TMERR			(14)
#define BSERR			(13)
#define TCERR			(12)
#define DCERR			(11)
#define EOERR			(10)
#define VBUSOFF			(9)
#define VBUSON			(8)
#define TBM			(7)
#define DP			(6)
#define DM			(5)
#define HSP			(4)
#define SDE			(3)
#define HFRM			(2)
#define HFSUSP			(1)
#define HFRES			(0)

#define UDC_SYSCTL		(0x10)
#define DTZIEN			(14)
#define DIEN			(12)
#define VBUSOFFEN		(11)
#define VBUSONEN		(10)
#define RWDE			(9)
#define EIE			(8)
#define BIS			(7)
#define SPDEN			(6)
#define RRDE			(5)
#define IPS			(4)
#define MFRM			(2)
#define HSUSPE			(1)
#define HRESE			(0)

#define UDC_EP0STAT		(0x12)
#define EP0LWO			(6)
#define SHT			(4)
#define TST			(1)
#define RSR			(0)

#define UDC_EP0CTL		(0x14)
#define EP0TTE			(3)
#define EP0TTS			(2)
#define EP0ESS			(1)
#define EP0TZLS			(0)

#define UDC_EPSTAT		(0x16)
#define FUDR			(15)
#define FOVF			(14)
#define FPID			(11)
#define OSD			(10)
#define DTCZ			(9)
#define SPT			(8)
#define DOM			(7)
#define FFS			(6)
#define FSC			(5)
#define EPLWO			(4)
#define PSIF			(2)
#define TPS			(1)
#define RPS			(0)

#define UDC_EPCTL		(0x18)
#define SRE			(15)
#define INPKTHLD		(12)
#define OUTPKTHLD		(11)
#define TNPMF			(9)
#define IME			(8)
#define DUEN			(7)
#define FLUSH			(6)
#define TTE			(5)
#define TTS			(3)
#define CDP			(2)
#define ESS			(1)

/* IEMS=interrupt endpoint mode set.  Does this mean that interrupt endpoints
 * were not supported until now?
 */
#define IEMS			(0)

#define UDC_BRCR		(0x1A)
#define UDC_BWCR		(0x1C)
#define UDC_MPR			(0x1E)

#define UDC_DCR			(0x20)
#define ARDRD			(5)
#define FMDE			(4)
#define DMDE			(3)
#define TDR			(2)
#define RDR			(1)
#define DEN			(0)

#define UDC_DTCR		(0x22)
#define UDC_DFCR		(0x24)
#define UDC_DTTCR		(0x26)

/* This buffer has one 16-bit entry per EP? Do we DMA to here? */
#define UDC_EPBUFS		(0x30)

/* New registers in the USB */
#define UDC_PLICR		(0x50)
#define PLC			(8)
#define LPC			(4)

#define UDC_PCR			(0x52)
#define URSTC			(7)
#define SIDC			(6)
#define OPMC			(4)
#define TMSC			(3)
#define XCRC			(4)
#define SUSPC			(1)
#define PCE			(0)

/* The LF1000 CPU has some yet-undocumented registers */
#define UDC_CIKSEL		(0x840)

#define UDC_VBUSINTENB		(0x842)

#define UDC_VBUSPEND		(0x844)
#define VBUSPEND		(0)

#define UDC_POR			(0x846)

#define UDC_SUSPEND		(0x848)

#define UDC_USER0		(0x84A)
#define XOBLOCK_ON		(0)

#define UDC_USER1		(0x84C)
#define VBUSENB			(15)

#define UDC_CLKEN		(0x8C0)
#define UDC_PCLKMODE		(3)
#define UDC_CLKGENENB		(2)
#define UDC_CLKENB		(0)

#define UDC_CLKGEN		(0x8C4)
#define UDC_CLKSRCSEL		(1)
#define UDC_CLKDIV		(4)

/* The LF1000 uses one gpio pin to detect the vbus signal, and another to gate
 * the vbus signal to the UDC's USBVBUS pin.  In response to this latter signal,
 * the UDC pulls the D- line high to perform speed negotiation with the host.
 * In principle, it can also be used to detect the vbus status, but that's just
 * not how we do it.
 */
#include <gpio.h>
#define VBUS_DET_PIN GPIO_PIN5
/* On the LF1000 board made by MagicEyes, the VBUS pin goes directly to the
 * USBVBUS pin on the chip, so the GPIO pins are not used for VBUS handling.
 */

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// Symbol from Magic Eyes				    // Symbol in lf1000_udc.h
#define EP_INT_REG_OFFSET           0x02		// UDC_EPINT
#define EP_INT_EN_REG_OFFSET        0x04		// UDC_EPINTEN
#define FUNC_ADDR_REG_OFFSET        0x06		// UDC_FUNCADDR
#define EP_DIR_REG_OFFSET			0x0A		// UDC_EPDIR
#define TEST_REG_OFFSET				0x0C		// UDC_TEST
#define USB_INT_REG_OFFSET          0x0E		// UDC_SYSSTAT
#define USB_INT_EN_REG_OFFSET       0x10		// UDC_SYSCTL
#define EP0_STS_REG_OFFSET          0x12		// UDC_EP0STAT
#define EP0_CTL_REG_OFFSET          0x14		// UDC_EP0CTL
#define GEP_STS_REG_OFFSET          0x16		// UDC_EPSTAT
#define GEP_CTL_REG_OFFSET			0x18		// UDC_EPCTL
#define GEP_BRCR_REG_OFFSET			0x1A		// UDC_BRCR
#define GEP_BWCR_REG_OFFSET         0x1C		// UDC_BWCR
#define GEP_MPR_REG_OFFSET			0x1E		// UDC_MPR
#define EP0_FIFO_REG_OFFSET         0x30		// UDC_EPBUFS
#define EP1_FIFO_REG_OFFSET         0x32
#define EP2_FIFO_REG_OFFSET         0x34

#define DNW_IN_ENDPOINT		1
#define DNW_OUT_ENDPOINT	2

#define ENDPOINT_COUNT  	3

#define EP0_INT_FLAG		1
#define EP1_INT_FLAG		2
#define EP2_INT_FLAG		4
#define ALL_EP_INT_FLAGS	(EP0_INT_FLAG | EP1_INT_FLAG | EP2_INT_FLAG)
#define ANY_EP_INT_FLAGS	ALL_EP_INT_FLAGS

#define USB_INT_EN_HRESE_BIT        0x0001	// (1 << HRESE)
#define USB_INT_EN_HSUSPE_BIT       0x0002	// unused
#define USB_INT_EN_MFRM_BIT         0x0004	// unused
#define USB_INT_EN_SPDC_BIT         0x0008	// unused
#define USB_INT_EN_IPS_BIT          0x0010	// unused
#define USB_INT_EN_RRDE_BIT         0x0020	// (1 << RRDE)
#define USB_INT_EN_SPDEN_BIT        0x0040	// (1 << SPDEN)
#define USB_INT_EN_BIS_BIT          0x0080	// unused
#define USB_INT_EN_EIE_BIT          0x0100	// (1 << EIE)
#define USB_INT_EN_RWDE_BIT         0x0200	// unused
#define USB_INT_EN_VBUSONEN_BIT     0x0400	// unused
#define USB_INT_EN_VBUSOFFEN_BIT    0x0800	// unused
#define USB_INT_EN_DIEN_BIT         0x0100	// unused
#define USB_INT_EN_DTZIEN_BIT       0x0400	// unused

#define USB_INT_HFRES_BIT			0x0001	// (1 << HFRES)
#define USB_INT_HFSUSP_BIT          0x0002	// unused
#define USB_INT_HFRM_BIT            0x0004	// unused
#define USB_INT_SDE_BIT             0x0008	// (1 << SDE)
#define USB_INT_HSP_BIT             0x0010	// (1 << HSP)
#define USB_INT_TBM_BIT             0x0080	// (1 << TBM)
#define USB_INT_VBUSON_BIT          0x0100	// unused
#define USB_INT_VBUSOFF_BIT         0x0200	// unused
#define USB_INT_EOERR_BIT           0x0400	// (1 << EOERR)
#define USB_INT_DCERR_BIT           0x0800	// (1 << DCERR)
#define USB_INT_TCERR_BIT           0x1000	// (1 << TCERR)
#define USB_INT_BSERR_BIT           0x2000	// (1 << BSERR)
#define USB_INT_TMERR_BIT           0x4000	// (1 << TMERR)
#define USB_INT_BAERR_BIT           0x8000	// (1 << BAERR)

#define USB_TEST_VBUS				0x8000	// VBUS status (0 == off; 1 == on)
#define USB_TEST_EUERR				0x2000	// EB Underrun error
#define USB_TEST_PERR				0x1000	// PID error

#define EP0_STS_RSR_BIT             0x0001	// (1 << RSR)
#define EP0_STS_TST_BIT             0x0002	// (1 << TST)
#define EP0_STS_SHT_BIT             0x0010	// (1 << SHT)
#define EP0_STS_LWO_BIT             0x0040	// (1 << EP0LWO)

#define EP0_CTL_TZLS_BIT            0x0001	// unused
#define EP0_CTL_ESS_BIT             0x0002	// (1 << EP0ESS)
#define EP0_CTL_TTS_BIT             0x0004	// unused
#define EP0_CTL_TTE_BIT             0x0008	// unused

#define GEP_STS_RPS_BIT             0x0001	// (1 << RPS)
#define GEP_STS_TPS_BIT             0x0002	// (1 << TPS)
#define GEP_STS_PSIF_BIT            0x000C	// unused
#define GEP_STS_LWO_BIT             0x0010	// (1 << EPLWO)
#define GEP_STS_FSC_BIT             0x0020	// (1 << FSC)
#define GEP_STS_FFS_BIT             0x0040	// (1 << FFS)
#define GEP_STS_DOM_BIT             0x0080	// unused
#define GEP_STS_SPT_BIT             0x0100	// unused
#define GEP_STS_DTCZ_BIT            0x0200	// unused
#define GEP_STS_OSD_BIT             0x0400	// unused
#define GEP_STS_FPID_BIT            0x0800	// unused
#define GEP_STS_FOVF_BIT            0x4000	// unused
#define GEP_STS_FUDR_BIT            0x8000	// unused

#define GEP_CTL_IEMS_BIT            0x0001	// unused
#define GEP_CTL_ESS_BIT             0x0002	// (1 << ESS)
#define GEP_CTL_CDP_BIT             0x0004	// (1 << CDP)
#define GEP_CTL_TTS_BIT             0x0018	// unused
#define GEP_CTL_TTE_BIT             0x0020	// unused
#define GEP_CTL_FLUSH_BIT           0x0040	// (1 << FLUSH)
#define GEP_CTL_DUEN_BIT            0x0080	// (1 << DUEN)
#define GEP_CTL_IME_BIT             0x0100	// (1 << IME)
#define GEP_CTL_TNPMF_BIT           0x0600	// unused
#define GEP_CTL_OUTPKTHLD_BIT       0x0800	// (1 << OUTPKTHLD)
#define GEP_CTL_INPKTHLD_BIT        0x1000	// (1 << INPKTHLD)
#define GEP_CTL_SRE_BIT             0x8000	// unused

#define CTRLR_BASE_REG_ADDR(offset) ((volatile u16*) (UDC_BASE + offset))

#define UDC16(x)	REG16(UDC_BASE+x)


#endif	// BOOTUDC_H


