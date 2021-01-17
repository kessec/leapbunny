/** @file  mlanmisc.h
  *
  * @brief This file contains command definitions for application
  *
  * Copyright (C) 2008-2009, Marvell International Ltd.
  *
  * This software file (the "File") is distributed by Marvell International
  * Ltd. under the terms of the GNU General Public License Version 2, June 1991
  * (the "License").  You may use, redistribute and/or modify this File in
  * accordance with the terms and conditions of the License, a copy of which
  * is available by writing to the Free Software Foundation, Inc.,
  * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
  * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
  *
  * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
  * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
  * this warranty disclaimer.
  *
  */
/************************************************************************
Change log:
     03/10/2009: initial version
************************************************************************/

#ifndef _MLANCMD_H_
#define _MLANCMD_H_

/** Maximum size of IEEE Information Elements */
#define IEEE_MAX_IE_SIZE  256

#ifdef FALSE
#undef FALSE
#endif

#ifdef TRUE
#undef TRUE
#endif

/** Type definition: boolean */
typedef enum
{ FALSE, TRUE } boolean;

/** Type enumeration of WMM AC_QUEUES */
typedef enum _mlan_wmm_ac_e
{
    WMM_AC_BK,
    WMM_AC_BE,
    WMM_AC_VI,
    WMM_AC_VO
} mlan_wmm_ac_e;

/** Maximum length of SSID */
#define MRVDRV_MAX_SSID_LENGTH          32
/** Infrastructure BSS scan type in wlan_scan_cmd_config */
#define WLAN_SCAN_BSS_TYPE_BSS         1
/** Adhoc BSS scan type in wlan_scan_cmd_config */
#define WLAN_SCAN_BSS_TYPE_IBSS        2
/** Adhoc or Infrastructure BSS scan type in wlan_scan_cmd_config, no filter */
#define WLAN_SCAN_BSS_TYPE_ANY         3
/** Length of ethernet address */
#ifndef ETH_ALEN
#define ETH_ALEN            6
#endif
/** Maximum length of SSID list */
#define MRVDRV_MAX_SSID_LIST_LENGTH         10

/** Maximum number of channels that can be sent in a setuserscan ioctl */
#define WLAN_IOCTL_USER_SCAN_CHAN_MAX  50

/** IEEE Type definitions  */
typedef enum _IEEEtypes_ElementId_e
{
    SSID = 0,
    SUPPORTED_RATES = 1,
    FH_PARAM_SET = 2,
    DS_PARAM_SET = 3,
    CF_PARAM_SET = 4,

    IBSS_PARAM_SET = 6,

    COUNTRY_INFO = 7,

    HT_CAPABILITY = 45,
    HT_OPERATION = 61,
    BSSCO_2040 = 72,
    OVERLAPBSSSCANPARAM = 74,
    EXT_CAPABILITY = 127,

    ERP_INFO = 42,
    EXTENDED_SUPPORTED_RATES = 50,

    VENDOR_SPECIFIC_221 = 221,
    WMM_IE = VENDOR_SPECIFIC_221,

    WPS_IE = VENDOR_SPECIFIC_221,

    WPA_IE = VENDOR_SPECIFIC_221,
    RSN_IE = 48,
} __attribute__ ((packed)) IEEEtypes_ElementId_e;

/** Capability Bit Map*/
#ifdef BIG_ENDIAN
typedef struct _IEEEtypes_CapInfo_t
{
    t_u8 rsrvd1:2;
    t_u8 dsss_ofdm:1;
    t_u8 rsvrd2:2;
    t_u8 short_slot_time:1;
    t_u8 rsrvd3:1;
    t_u8 spectrum_mgmt:1;
    t_u8 chan_agility:1;
    t_u8 pbcc:1;
    t_u8 short_preamble:1;
    t_u8 privacy:1;
    t_u8 cf_poll_rqst:1;
    t_u8 cf_pollable:1;
    t_u8 ibss:1;
    t_u8 ess:1;
} __attribute__ ((packed)) IEEEtypes_CapInfo_t, *pIEEEtypes_CapInfo_t;
#else
typedef struct _IEEEtypes_CapInfo_t
{
    /** Capability Bit Map : ESS */
    t_u8 ess:1;
    /** Capability Bit Map : IBSS */
    t_u8 ibss:1;
    /** Capability Bit Map : CF pollable */
    t_u8 cf_pollable:1;
    /** Capability Bit Map : CF poll request */
    t_u8 cf_poll_rqst:1;
    /** Capability Bit Map : privacy */
    t_u8 privacy:1;
    /** Capability Bit Map : Short preamble */
    t_u8 short_preamble:1;
    /** Capability Bit Map : PBCC */
    t_u8 pbcc:1;
    /** Capability Bit Map : Channel agility */
    t_u8 chan_agility:1;
    /** Capability Bit Map : Spectrum management */
    t_u8 spectrum_mgmt:1;
    /** Capability Bit Map : Reserved */
    t_u8 rsrvd3:1;
    /** Capability Bit Map : Short slot time */
    t_u8 short_slot_time:1;
    /** Capability Bit Map : APSD */
    t_u8 Apsd:1;
    /** Capability Bit Map : Reserved */
    t_u8 rsvrd2:1;
    /** Capability Bit Map : DSS OFDM */
    t_u8 dsss_ofdm:1;
    /** Capability Bit Map : Reserved */
    t_u8 rsrvd1:2;
} __attribute__ ((packed)) IEEEtypes_CapInfo_t, *pIEEEtypes_CapInfo_t;
#endif /* BIG_ENDIAN */

/** IEEE IE header */
typedef struct _IEEEtypes_Header_t
{
    /** Element ID */
    t_u8 element_id;
    /** Length */
    t_u8 len;
} __attribute__ ((packed)) IEEEtypes_Header_t, *pIEEEtypes_Header_t;

/** Vendor specific IE header */
typedef struct _IEEEtypes_VendorHeader_t
{
    /** Element ID */
    t_u8 element_id;
    /** Length */
    t_u8 len;
    /** OUI */
    t_u8 oui[3];
    /** OUI type */
    t_u8 oui_type;
    /** OUI subtype */
    t_u8 oui_subtype;
    /** Version */
    t_u8 version;
} __attribute__ ((packed)) IEEEtypes_VendorHeader_t, *pIEEEtypes_VendorHeader_t;

/** Vendor specific IE */
typedef struct _IEEEtypes_VendorSpecific_t
{
    /** Vendor specific IE header */
    IEEEtypes_VendorHeader_t vend_hdr;
    /** IE Max - size of previous fields */
    t_u8 data[IEEE_MAX_IE_SIZE - sizeof(IEEEtypes_VendorHeader_t)];
}
__attribute__ ((packed)) IEEEtypes_VendorSpecific_t,
    *pIEEEtypes_VendorSpecific_t;

/** IEEE IE */
typedef struct _IEEEtypes_Generic_t
{
    /** Generic IE header */
    IEEEtypes_Header_t ieee_hdr;
    /** IE Max - size of previous fields */
    t_u8 data[IEEE_MAX_IE_SIZE - sizeof(IEEEtypes_Header_t)];
}
__attribute__ ((packed)) IEEEtypes_Generic_t, *pIEEEtypes_Generic_t;

/** Maximum number of AC QOS queues available in the driver/firmware */
#define MAX_AC_QUEUES 4

/** Data structure of WMM QoS information */
typedef struct _IEEEtypes_WmmQosInfo_t
{
#ifdef BIG_ENDIAN
    /** QoS UAPSD */
    t_u8 qos_uapsd:1;
    /** Reserved */
    t_u8 reserved:3;
    /** Parameter set count */
    t_u8 para_set_count:4;
#else
    /** Parameter set count */
    t_u8 para_set_count:4;
    /** Reserved */
    t_u8 reserved:3;
    /** QoS UAPSD */
    t_u8 qos_uapsd:1;
#endif
} __attribute__ ((packed)) IEEEtypes_WmmQosInfo_t, *pIEEEtypes_WmmQosInfo_t;

/** Data structure of WMM Aci/Aifsn */
typedef struct _IEEEtypes_WmmAciAifsn_t
{
#ifdef BIG_ENDIAN
    /** Reserved */
    t_u8 reserved:1;
    /** Aci */
    t_u8 aci:2;
    /** Acm */
    t_u8 acm:1;
    /** Aifsn */
    t_u8 aifsn:4;
#else
    /** Aifsn */
    t_u8 aifsn:4;
    /** Acm */
    t_u8 acm:1;
    /** Aci */
    t_u8 aci:2;
    /** Reserved */
    t_u8 reserved:1;
#endif
} __attribute__ ((packed)) IEEEtypes_WmmAciAifsn_t, *pIEEEtypes_WmmAciAifsn_t;

/** Data structure of WMM ECW */
typedef struct _IEEEtypes_WmmEcw_t
{
#ifdef BIG_ENDIAN
    /** Maximum Ecw */
    t_u8 ecw_max:4;
    /** Minimum Ecw */
    t_u8 ecw_min:4;
#else
    /** Minimum Ecw */
    t_u8 ecw_min:4;
    /** Maximum Ecw */
    t_u8 ecw_max:4;
#endif
} __attribute__ ((packed)) IEEEtypes_WmmEcw_t, *pIEEEtypes_WmmEcw_t;

/** Data structure of WMM AC parameters  */
typedef struct _IEEEtypes_WmmAcParameters_t
{
    IEEEtypes_WmmAciAifsn_t aci_aifsn;       /**< AciAifSn */
    IEEEtypes_WmmEcw_t ecw;                 /**< Ecw */
    t_u16 tx_op_limit;                        /**< Tx op limit */
} __attribute__ ((packed)) IEEEtypes_WmmAcParameters_t,
    *pIEEEtypes_WmmAcParameters_t;

/** Data structure of WMM Info IE  */
typedef struct _IEEEtypes_WmmInfo_t
{

    /**
     * WMM Info IE - Vendor Specific Header:
     *   element_id  [221/0xdd]
     *   Len         [7] 
     *   Oui         [00:50:f2]
     *   OuiType     [2]
     *   OuiSubType  [0]
     *   Version     [1]
     */
    IEEEtypes_VendorHeader_t vend_hdr;

    /** QoS information */
    IEEEtypes_WmmQosInfo_t qos_info;

} __attribute__ ((packed)) IEEEtypes_WmmInfo_t, *pIEEEtypes_WmmInfo_t;

/** Data structure of WMM parameter IE  */
typedef struct _IEEEtypes_WmmParameter_t
{
    /**
     * WMM Parameter IE - Vendor Specific Header:
     *   element_id  [221/0xdd]
     *   Len         [24] 
     *   Oui         [00:50:f2]
     *   OuiType     [2]
     *   OuiSubType  [1]
     *   Version     [1]
     */
    IEEEtypes_VendorHeader_t vend_hdr;

    /** QoS information */
    IEEEtypes_WmmQosInfo_t qos_info;
    /** Reserved */
    t_u8 reserved;

    /** AC Parameters Record WMM_AC_BE, WMM_AC_BK, WMM_AC_VI, WMM_AC_VO */
    IEEEtypes_WmmAcParameters_t ac_params[MAX_AC_QUEUES];

} __attribute__ ((packed)) IEEEtypes_WmmParameter_t, *pIEEEtypes_WmmParameter_t;

typedef struct _wlan_get_scan_table_fixed
{
    /** BSSID of this network */
    t_u8 bssid[MLAN_MAC_ADDR_LENGTH];
    /** Channel this beacon/probe response was detected */
    t_u8 channel;
    /** RSSI for the received packet */
    t_u8 rssi;
    /** TSF value from the firmware at packet reception */
    t_u64 network_tsf;
} __attribute__ ((packed)) wlan_get_scan_table_fixed;

/**
 *  Structure passed in the wlan_ioctl_get_scan_table_info for each
 *    BSS returned in the WLAN_GET_SCAN_RESP IOCTL
 */
typedef struct _wlan_ioctl_get_scan_table_entry
{
    /**
     *  Fixed field length included in the response.
     *
     *  Length value is included so future fixed fields can be added to the
     *   response without breaking backwards compatibility.  Use the length
     *   to find the offset for the bssInfoLength field, not a sizeof() calc.
     */
    t_u32 fixed_field_length;

    /**
     *  Always present, fixed length data fields for the BSS
     */
    wlan_get_scan_table_fixed fixed_fields;

    /**
     *  Length of the BSS Information (probe resp or beacon) that
     *    follows starting at bssInfoBuffer
     */
    t_u32 bss_info_length;

    /**
     *  Probe response or beacon scanned for the BSS.
     *
     *  Field layout:
     *   - TSF              8 octets
     *   - Beacon Interval  2 octets
     *   - Capability Info  2 octets
     *
     *   - IEEE Infomation Elements; variable number & length per 802.11 spec
     */
    t_u8 bss_info_buffer[1];
} __attribute__ ((packed)) wlan_ioctl_get_scan_table_entry;

/**
 *  Sructure to retrieve the scan table
 */
typedef struct
{
    /**
     *  - Zero based scan entry to start retrieval in command request
     *  - Number of scans entries returned in command response
     */
    t_u32 scan_number;
    /**
     * Buffer marker for multiple wlan_ioctl_get_scan_table_entry structures.
     *   Each struct is padded to the nearest 32 bit boundary.
     */
    t_u8 scan_table_entry_buf[1];
} __attribute__ ((packed)) wlan_ioctl_get_scan_table_info;

typedef struct
{
    t_u8 chan_number;  /**< Channel Number to scan */
    t_u8 radio_type;   /**< Radio type: 'B/G' Band = 0, 'A' Band = 1 */
    t_u8 scan_type;    /**< Scan type: Active = 0, Passive = 1 */
    t_u8 reserved;    /**< Reserved */
    t_u32 scan_time;   /**< Scan duration in milliseconds; if 0 default used */
} __attribute__ ((packed)) wlan_ioctl_user_scan_chan;

typedef struct
{
    char ssid[MRVDRV_MAX_SSID_LENGTH + 1];  /**< SSID */
    t_u8 max_len;                              /**< Maximum length of SSID */
} __attribute__ ((packed)) wlan_ioctl_user_scan_ssid;

typedef struct
{

    /** Flag set to keep the previous scan table intact */
    t_u8 keep_previous_scan;    /* Do not erase the existing scan results */

    /** BSS Type to be sent in the firmware command */
    t_u8 bss_type;

    /** Configure the number of probe requests for active chan scans */
    t_u8 num_probes;

    /** Reserved */
    t_u8 reserved;

    /** BSSID filter sent in the firmware command to limit the results */
    t_u8 specific_bssid[ETH_ALEN];
    /** SSID filter list used in the to limit the scan results */
    wlan_ioctl_user_scan_ssid ssid_list[MRVDRV_MAX_SSID_LIST_LENGTH];

    /** Variable number (fixed maximum) of channels to scan up */
    wlan_ioctl_user_scan_chan chan_list[WLAN_IOCTL_USER_SCAN_CHAN_MAX];

} __attribute__ ((packed)) wlan_ioctl_user_scan_cfg;

int process_setuserscan(int argc, char *argv[]);
int process_getscantable(int argc, char *argv[]);

#endif /* _MLANCMD_H_ */
