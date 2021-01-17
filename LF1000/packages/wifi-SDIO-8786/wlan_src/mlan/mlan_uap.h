/** @file mlan_uap.h
 *
 *  @brief This file contains related macros, enum, and struct
 *  of uap functionalities
 *
 *  Copyright (C) 2009, Marvell International Ltd. 
 *
 *  This software file (the "File") is distributed by Marvell International 
 *  Ltd. under the terms of the GNU General Public License Version 2, June 1991 
 *  (the "License").  You may use, redistribute and/or modify this File in 
 *  accordance with the terms and conditions of the License, a copy of which 
 *  is available by writing to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
 *  worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 *  THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 *  this warranty disclaimer.
 */

/********************************************************
Change log:
    02/05/2009: initial version
********************************************************/

#ifndef _MLAN_UAP_H_
#define _MLAN_UAP_H_
/**  Host Command id: SYS_INFO */
#define HOST_CMD_APCMD_SYS_INFO               0x00ae
/** Host Command id: sys_reset */
#define HOST_CMD_APCMD_SYS_RESET              0x00af
/** Host Command id: SYS_CONFIGURE  */
#define HOST_CMD_APCMD_SYS_CONFIGURE          0x00b0
/** Host Command id: BSS_START */
#define HOST_CMD_APCMD_BSS_START              0x00b1
/** Host Command id: BSS_STOP  */
#define HOST_CMD_APCMD_BSS_STOP               0x00b2
/** Host Command id: sta_list */
#define HOST_CMD_APCMD_STA_LIST               0x00b3
/** Host Command id: STA_DEAUTH */
#define HOST_CMD_APCMD_STA_DEAUTH             0x00b5

/** Event ID: STA deauth */
#define EVENT_MICRO_AP_STA_DEAUTH   44

/** Event ID: STA assoicated */
#define EVENT_MICRO_AP_STA_ASSOC    45
/** Event ID: BSS started */
#define EVENT_MICRO_AP_BSS_START    46

/** Event ID: BSS idle event */
#define EVENT_MICRO_AP_BSS_IDLE	     67

/** Event ID: BSS active event */
#define EVENT_MICRO_AP_BSS_ACTIVE	     68

/** TLV ID : Management Frame */
#define MRVL_MGMT_FRAME_TLV_ID       0x0168
/** Assoc Request */
#define SUBTYPE_ASSOC_REQUEST        0
/** TLV : Packet forwarding control */
#define MRVL_PKT_FWD_CTL_TLV_ID      0x0136

/** Convert TxPD to little endian format from CPU format */
#define uap_endian_convert_TxPD(x);                                         \
    {                                                                   \
        (x)->tx_pkt_length = wlan_cpu_to_le16((x)->tx_pkt_length);      \
        (x)->tx_pkt_offset = wlan_cpu_to_le16((x)->tx_pkt_offset);      \
        (x)->tx_pkt_type = wlan_cpu_to_le16((x)->tx_pkt_type);      \
        (x)->tx_control = wlan_cpu_to_le32((x)->tx_control);      \
    }

/** Convert RxPD from little endian format to CPU format */
#define uap_endian_convert_RxPD(x);                                         \
    {                                                                   \
        (x)->rx_pkt_length = wlan_le16_to_cpu((x)->rx_pkt_length);      \
        (x)->rx_pkt_offset = wlan_le16_to_cpu((x)->rx_pkt_offset);      \
        (x)->rx_pkt_type = wlan_le16_to_cpu((x)->rx_pkt_type);      \
        (x)->seq_num = wlan_le16_to_cpu((x)->seq_num);              \
    }

mlan_status mlan_uap_ioctl(t_void * adapter, pmlan_ioctl_req pioctl_req);

mlan_status mlan_uap_prepare_cmd(IN t_void * priv,
                                 IN t_u16 cmd_no,
                                 IN t_u16 cmd_action,
                                 IN t_u32 cmd_oid,
                                 IN t_void * pioctl_buf,
                                 IN t_void * pdata_buf, IN t_void * pcmd_buf);

mlan_status mlan_process_uap_cmdresp(IN t_void * priv,
                                     IN t_u16 cmdresp_no,
                                     IN t_void * pcmd_buf, IN t_void * pioctl);

mlan_status mlan_process_uap_rx_packet(IN t_void * adapter,
                                       IN pmlan_buffer pmbuf);

mlan_status mlan_process_uap_event(IN t_void * priv);

t_void *mlan_process_uap_txpd(IN t_void * priv, IN pmlan_buffer pmbuf);

mlan_status mlan_uap_init_cmd(IN t_void * priv, IN t_u8 first_sta);

#endif /* _MLAN_UAP_H_ */
