/**
 * @file mlan_misc.c
 *
 *  @brief This file include miscellaneous functions for MLAN module
 *
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

/*************************************************************
Change Log:
    05/11/2009: initial version
************************************************************/
#include "mlan.h"
#ifdef STA_SUPPORT
#include "mlan_join.h"
#endif /* STA_SUPPORT */
#include "mlan_util.h"
#include "mlan_fw.h"
#include "mlan_main.h"
#include "mlan_wmm.h"
#include "mlan_11n.h"
#include "mlan_sdio.h"
#ifdef UAP_SUPPORT
#include "mlan_uap.h"
#endif

/********************************************************
                Local Variables
********************************************************/

/********************************************************
                Global Variables
********************************************************/

/********************************************************
                Local Functions
********************************************************/

/********************************************************
                Global Functions
********************************************************/

/** 
 *  @brief send host cmd
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_SUCCESS --success, otherwise fail
 */
mlan_status
wlan_misc_ioctl_host_cmd(IN pmlan_adapter pmadapter,
                         IN pmlan_ioctl_req pioctl_req)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_num];
    mlan_ds_misc_cfg *misc = MNULL;

    ENTER();

    misc = (mlan_ds_misc_cfg *) pioctl_req->pbuf;

    /* Send request to firmware */
    ret = wlan_prepare_cmd(pmpriv,
                           0,
                           0,
                           0,
                           (t_void *) pioctl_req,
                           (t_void *) & misc->param.hostcmd);
    if (ret == MLAN_STATUS_SUCCESS)
        ret = MLAN_STATUS_PENDING;

    LEAVE();
    return ret;
}

/**
 *  @brief Send function init/shutdown command to firmware
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_PENDING --success, otherwise fail
 */
mlan_status
wlan_misc_ioctl_init_shutdown(IN pmlan_adapter pmadapter,
                              IN pmlan_ioctl_req pioctl_req)
{
    mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_num];
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_ds_misc_cfg *misc_cfg = MNULL;
    t_u16 cmd;

    ENTER();

    misc_cfg = (mlan_ds_misc_cfg *) pioctl_req->pbuf;
    if (misc_cfg->param.func_init_shutdown == MLAN_FUNC_INIT)
        cmd = HostCmd_CMD_FUNC_INIT;
    else if (misc_cfg->param.func_init_shutdown == MLAN_FUNC_SHUTDOWN)
        cmd = HostCmd_CMD_FUNC_SHUTDOWN;
    else {
        PRINTM(MERROR, "Unsupported parameter\n");
        ret = MLAN_STATUS_FAILURE;
        goto exit;
    }

    /* Send command to firmware */
    ret = wlan_prepare_cmd(pmpriv,
                           cmd,
                           HostCmd_ACT_GEN_SET,
                           0, (t_void *) pioctl_req, MNULL);

    if (ret == MLAN_STATUS_SUCCESS)
        ret = MLAN_STATUS_PENDING;

  exit:
    LEAVE();
    return ret;
}

/** 
 *  @brief Get debug information
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_SUCCESS --success, otherwise fail
 */
mlan_status
wlan_get_info_debug_info(IN pmlan_adapter pmadapter,
                         IN pmlan_ioctl_req pioctl_req)
{
    pmlan_private pmpriv = pmadapter->priv[pioctl_req->bss_num];
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_ds_get_info *info;

    ENTER();

    info = (mlan_ds_get_info *) pioctl_req->pbuf;

    if (pioctl_req->action == MLAN_ACT_SET) {
        memcpy(pmpriv->wmm.packets_out, info->param.debug_info.packets_out,
               sizeof(pmpriv->wmm.packets_out));
        pmadapter->max_tx_buf_size =
            (t_u16) info->param.debug_info.max_tx_buf_size;
        pmadapter->tx_buf_size = (t_u16) info->param.debug_info.tx_buf_size;
        pmadapter->ps_mode = info->param.debug_info.ps_mode;
        pmadapter->ps_state = info->param.debug_info.ps_state;
#ifdef STA_SUPPORT
        pmadapter->is_deep_sleep = info->param.debug_info.is_deep_sleep;
#endif /* STA_SUPPORT */
        pmadapter->pm_wakeup_card_req =
            info->param.debug_info.pm_wakeup_card_req;
        pmadapter->pm_wakeup_fw_try = info->param.debug_info.pm_wakeup_fw_try;
        pmadapter->is_hs_configured = info->param.debug_info.is_hs_configured;
        pmadapter->hs_activated = info->param.debug_info.hs_activated;

        pmadapter->dbg.num_cmd_host_to_card_failure =
            info->param.debug_info.num_cmd_host_to_card_failure;
        pmadapter->dbg.num_cmd_sleep_cfm_host_to_card_failure =
            info->param.debug_info.num_cmd_sleep_cfm_host_to_card_failure;
        pmadapter->dbg.num_tx_host_to_card_failure =
            info->param.debug_info.num_tx_host_to_card_failure;
        pmadapter->dbg.num_event_deauth =
            info->param.debug_info.num_event_deauth;
        pmadapter->dbg.num_event_disassoc =
            info->param.debug_info.num_event_disassoc;
        pmadapter->dbg.num_event_link_lost =
            info->param.debug_info.num_event_link_lost;
        pmadapter->dbg.num_cmd_deauth = info->param.debug_info.num_cmd_deauth;
        pmadapter->dbg.num_cmd_assoc_success =
            info->param.debug_info.num_cmd_assoc_success;
        pmadapter->dbg.num_cmd_assoc_failure =
            info->param.debug_info.num_cmd_assoc_failure;
        pmadapter->dbg.num_tx_timeout = info->param.debug_info.num_tx_timeout;
        pmadapter->dbg.num_cmd_timeout = info->param.debug_info.num_cmd_timeout;
        pmadapter->dbg.timeout_cmd_id = info->param.debug_info.timeout_cmd_id;
        pmadapter->dbg.timeout_cmd_act = info->param.debug_info.timeout_cmd_act;
        memcpy(pmadapter->dbg.last_cmd_id, info->param.debug_info.last_cmd_id,
               sizeof(pmadapter->dbg.last_cmd_id));
        memcpy(pmadapter->dbg.last_cmd_act, info->param.debug_info.last_cmd_act,
               sizeof(pmadapter->dbg.last_cmd_act));
        pmadapter->dbg.last_cmd_index = info->param.debug_info.last_cmd_index;
        memcpy(pmadapter->dbg.last_cmd_resp_id,
               info->param.debug_info.last_cmd_resp_id,
               sizeof(pmadapter->dbg.last_cmd_resp_id));
        pmadapter->dbg.last_cmd_resp_index =
            info->param.debug_info.last_cmd_resp_index;
        memcpy(pmadapter->dbg.last_event, info->param.debug_info.last_event,
               sizeof(pmadapter->dbg.last_event));
        pmadapter->dbg.last_event_index =
            info->param.debug_info.last_event_index;

        pmadapter->data_sent = info->param.debug_info.data_sent;
        pmadapter->cmd_sent = info->param.debug_info.cmd_sent;
        pmadapter->mp_rd_bitmap = info->param.debug_info.mp_rd_bitmap;
        pmadapter->mp_wr_bitmap = info->param.debug_info.mp_wr_bitmap;
        pmadapter->curr_rd_port = info->param.debug_info.curr_rd_port;
        pmadapter->curr_wr_port = info->param.debug_info.curr_wr_port;
        pmadapter->cmd_resp_received = info->param.debug_info.cmd_resp_received;
    } else {                    /* MLAN_ACT_GET */
        memcpy(info->param.debug_info.packets_out, pmpriv->wmm.packets_out,
               sizeof(pmpriv->wmm.packets_out));
        info->param.debug_info.max_tx_buf_size =
            (t_u32) pmadapter->max_tx_buf_size;
        info->param.debug_info.tx_buf_size = (t_u32) pmadapter->tx_buf_size;
        info->param.debug_info.rx_tbl_num =
            wlan_get_rxreorder_tbl(pmpriv, info->param.debug_info.rx_tbl);
        info->param.debug_info.tx_tbl_num =
            wlan_get_txbastream_tbl(pmpriv, info->param.debug_info.tx_tbl);
        info->param.debug_info.ps_mode = pmadapter->ps_mode;
        info->param.debug_info.ps_state = pmadapter->ps_state;
#ifdef STA_SUPPORT
        info->param.debug_info.is_deep_sleep = pmadapter->is_deep_sleep;
#endif /* STA_SUPPORT */

        info->param.debug_info.pm_wakeup_card_req =
            pmadapter->pm_wakeup_card_req;
        info->param.debug_info.pm_wakeup_fw_try = pmadapter->pm_wakeup_fw_try;
        info->param.debug_info.is_hs_configured = pmadapter->is_hs_configured;
        info->param.debug_info.hs_activated = pmadapter->hs_activated;

        info->param.debug_info.num_cmd_host_to_card_failure
            = pmadapter->dbg.num_cmd_host_to_card_failure;
        info->param.debug_info.num_cmd_sleep_cfm_host_to_card_failure
            = pmadapter->dbg.num_cmd_sleep_cfm_host_to_card_failure;
        info->param.debug_info.num_tx_host_to_card_failure
            = pmadapter->dbg.num_tx_host_to_card_failure;
        info->param.debug_info.num_event_deauth =
            pmadapter->dbg.num_event_deauth;
        info->param.debug_info.num_event_disassoc =
            pmadapter->dbg.num_event_disassoc;
        info->param.debug_info.num_event_link_lost =
            pmadapter->dbg.num_event_link_lost;
        info->param.debug_info.num_cmd_deauth = pmadapter->dbg.num_cmd_deauth;
        info->param.debug_info.num_cmd_assoc_success =
            pmadapter->dbg.num_cmd_assoc_success;
        info->param.debug_info.num_cmd_assoc_failure =
            pmadapter->dbg.num_cmd_assoc_failure;
        info->param.debug_info.num_tx_timeout = pmadapter->dbg.num_tx_timeout;
        info->param.debug_info.num_cmd_timeout = pmadapter->dbg.num_cmd_timeout;
        info->param.debug_info.timeout_cmd_id = pmadapter->dbg.timeout_cmd_id;
        info->param.debug_info.timeout_cmd_act = pmadapter->dbg.timeout_cmd_act;
        memcpy(info->param.debug_info.last_cmd_id, pmadapter->dbg.last_cmd_id,
               sizeof(pmadapter->dbg.last_cmd_id));
        memcpy(info->param.debug_info.last_cmd_act, pmadapter->dbg.last_cmd_act,
               sizeof(pmadapter->dbg.last_cmd_act));
        info->param.debug_info.last_cmd_index = pmadapter->dbg.last_cmd_index;
        memcpy(info->param.debug_info.last_cmd_resp_id,
               pmadapter->dbg.last_cmd_resp_id,
               sizeof(pmadapter->dbg.last_cmd_resp_id));
        info->param.debug_info.last_cmd_resp_index =
            pmadapter->dbg.last_cmd_resp_index;
        memcpy(info->param.debug_info.last_event, pmadapter->dbg.last_event,
               sizeof(pmadapter->dbg.last_event));
        info->param.debug_info.last_event_index =
            pmadapter->dbg.last_event_index;

        info->param.debug_info.mp_rd_bitmap = pmadapter->mp_rd_bitmap;
        info->param.debug_info.mp_wr_bitmap = pmadapter->mp_wr_bitmap;
        info->param.debug_info.curr_rd_port = pmadapter->curr_rd_port;
        info->param.debug_info.curr_wr_port = pmadapter->curr_wr_port;
        info->param.debug_info.data_sent = pmadapter->data_sent;
        info->param.debug_info.cmd_sent = pmadapter->cmd_sent;
        info->param.debug_info.cmd_resp_received = pmadapter->cmd_resp_received;
    }

    pioctl_req->data_read_written =
        sizeof(mlan_debug_info) + MLAN_SUB_COMMAND_SIZE;

    LEAVE();
    return ret;
}

/**
 *  @brief This function wakes up the card.
 *
 *  @param pmadapter		A pointer to mlan_adapter structure
 *
 *  @return			MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
wlan_pm_wakeup_card(IN pmlan_adapter pmadapter)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    pmlan_callbacks pcb = &pmadapter->callbacks;

    ENTER();
    PRINTM(MCMND, "Wakeup device...\n");
    ret =
        pcb->moal_write_reg(pmadapter->pmoal_handle, CONFIGURATION_REG,
                            HOST_POWER_UP);

    LEAVE();
    return ret;
}

/**
 *  @brief This function resets the PM setting of the card.
 *
 *  @param pmadapter		A pointer to mlan_adapter structure
 *
 *  @return			MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
wlan_pm_reset_card(IN pmlan_adapter pmadapter)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    pmlan_callbacks pcb = &pmadapter->callbacks;

    ENTER();

    ret = pcb->moal_write_reg(pmadapter->pmoal_handle, CONFIGURATION_REG, 0);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function allocates a mlan_buffer.
 *
 *  @param pcb        Pointer to mlan_callbacks
 *  @param data_len   Data length
 *
 *  @return           mlan_buffer pointer or MNULL
 */
pmlan_buffer
wlan_alloc_mlan_buffer(pmlan_callbacks pcb, t_u32 data_len)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    pmlan_buffer pmbuf = MNULL;
    t_u32 buf_size = sizeof(mlan_buffer) + data_len + HEADER_ALIGNMENT;

    ENTER();

    ret = pcb->moal_malloc(buf_size, (t_u8 **) & pmbuf);
    if ((ret != MLAN_STATUS_SUCCESS) || !pmbuf) {
        pmbuf = MNULL;
        goto exit;
    }

    memset(pmbuf, 0, sizeof(mlan_buffer));

    pmbuf->pdesc = MNULL;
    /* Align address */
    pmbuf->pbuf =
        (t_u8 *) ALIGN_ADDR((t_u8 *) pmbuf + sizeof(mlan_buffer),
                            HEADER_ALIGNMENT);
    pmbuf->data_offset = 0;
    pmbuf->data_len = data_len;

  exit:
    LEAVE();
    return pmbuf;
}

/** 
 *  @brief This function frees a mlan_buffer.
 *
 *  @param pcb        Pointer to mlan_callbacks
 *  @param pmbuf      Pointer to mlan_buffer
 *
 *  @return           N/A
 */
t_void
wlan_free_mlan_buffer(pmlan_callbacks pcb, pmlan_buffer pmbuf)
{
    ENTER();

    if (pcb && pmbuf)
        pcb->moal_mfree((t_u8 *) pmbuf);

    LEAVE();
    return;
}

/** 
 *  @brief Delay function implementation
 *   
 *  @param pmadapter        A pointer to mlan_adapter structure
 *  @param delay            Delay value
 *  @param u                Units of delay (sec, msec or usec)
 */
t_void
wlan_delay_func(mlan_adapter * pmadapter, t_u32 delay, t_delay_unit u)
{
    t_u32 now_tv_sec, now_tv_usec;
    t_u32 upto_tv_sec, upto_tv_usec;
    pmlan_callbacks pcb = &pmadapter->callbacks;

    if (pcb->moal_udelay) {
        if (u == SEC) {
            delay *= 1000000;
        } else if (u == MSEC) {
            delay *= 1000;
        }
        pcb->moal_udelay(delay);
    } else {

        pcb->moal_get_system_time(&upto_tv_sec, &upto_tv_usec);

        switch (u) {
        case SEC:
            upto_tv_sec += delay;
            break;
        case MSEC:
            delay *= 1000;
        case USEC:
            upto_tv_sec += (delay / 1000000);
            upto_tv_usec += (delay % 1000000);
            break;
        }

        do {
            pcb->moal_get_system_time(&now_tv_sec, &now_tv_usec);
            if (now_tv_sec > upto_tv_sec)
                return;

            if ((now_tv_sec == upto_tv_sec) && (now_tv_usec >= upto_tv_usec))
                return;
        } while (MTRUE);
    }
    return;
}

#define	MLAN_CUSTOM_IE_AUTO_IDX_MASK	0xffff
/**
 *  @brief Ge the custom IE index
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *  @param mask	mask value for which the index to be returned
 *  @param ie_data	a pointer to custom_ie structure
 *  @param idx		will hold the computed index
 *
 *  @return		MLAN_STATUS_SUCCESS --success, otherwise fail
 */
mlan_status
wlan_custom_ioctl_get_autoidx(IN pmlan_adapter pmadapter,
                              IN pmlan_ioctl_req pioctl_req,
                              IN t_u16 mask,
                              IN custom_ie * ie_data, OUT t_u16 * idx)
{
    t_u16 index = 0, insert = 0;
    mlan_status ret;

    mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_num];
    /* Determine the index where the IE needs to be inserted */
    while (!insert) {
        while (index < MAX_MGMT_IE_INDEX) {
            if (pmpriv->mgmt_ie[index].mgmt_subtype_mask == mask) {
                /* Duplicate IE should be avoided */
                if (pmpriv->mgmt_ie[index].ie_length) {
                    if (!memcmp(pmpriv->mgmt_ie[index].ie_buffer,
                                ie_data->ie_buffer,
                                pmpriv->mgmt_ie[index].ie_length)) {
                        PRINTM(MERROR,
                               "IE with the same mast exists at index %d\n",
                               index);
                        *idx = -1;
                        ret = MLAN_STATUS_SUCCESS;
                        goto done;
                    }
                }
                /* Check if enough space is available */
                if (pmpriv->mgmt_ie[index].ie_length + ie_data->ie_length >
                    MAX_IE_SIZE) {
                    index++;
                    continue;
                }
                insert = 1;
                break;
            }
            index++;
        }
        if (!insert) {
            for (index = 0; index < MAX_MGMT_IE_INDEX; index++) {
                if (pmpriv->mgmt_ie[index].ie_length == 0) {
                    insert = 1;
                    break;
                }
            }
        }
        if (index == MAX_MGMT_IE_INDEX && !insert) {
            PRINTM(MERROR, "Failed to Set the IE buffer\n");
            ret = MLAN_STATUS_FAILURE;
            goto done;
        }
    }

    *idx = index;
    ret = MLAN_STATUS_SUCCESS;
  done:
    return ret;
}

/**
 *  @brief Delete custom IE
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *  @param ie_data	a pointer to custom_ie structure
 *  @param idx		index supplied
 *
 *  @return		MLAN_STATUS_SUCCESS --success, otherwise fail
 */

mlan_status
wlan_custom_ioctl_auto_delete(IN pmlan_adapter pmadapter,
                              IN pmlan_ioctl_req pioctl_req,
                              IN custom_ie * ie_data, IN t_u16 idx)
{
    mlan_status ret = MLAN_STATUS_FAILURE;
    mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_num];
    t_u16 index = 0, insert = 0, del_len;
    t_u8 del_ie[MAX_IE_SIZE], ie[MAX_IE_SIZE];
    t_s32 cnt, tmp_len = 0;
    t_u8 *tmp_ie;

    memset(del_ie, 0, MAX_IE_SIZE);
    memcpy(del_ie, ie_data->ie_buffer, ie_data->ie_length);
    del_len = ie_data->ie_length;

    for (index = 0; index < MAX_MGMT_IE_INDEX; index++) {
        if (MLAN_CUSTOM_IE_AUTO_IDX_MASK != idx)
            index = idx;
        tmp_ie = pmpriv->mgmt_ie[index].ie_buffer;
        tmp_len = pmpriv->mgmt_ie[index].ie_length;
        cnt = 0;
        while (tmp_len) {
            if (!memcmp(tmp_ie, del_ie, del_len)) {
                memcpy(ie, pmpriv->mgmt_ie[index].ie_buffer, cnt);
                if (pmpriv->mgmt_ie[index].ie_length > (cnt + del_len))
                    memcpy(&ie[cnt],
                           &pmpriv->mgmt_ie[index].ie_buffer[cnt + del_len],
                           (pmpriv->mgmt_ie[index].ie_length -
                            (cnt + del_len)));
                memset(&pmpriv->mgmt_ie[index].ie_buffer, 0,
                       sizeof(pmpriv->mgmt_ie[index].ie_buffer));
                memcpy(&pmpriv->mgmt_ie[index].ie_buffer, ie,
                       pmpriv->mgmt_ie[index].ie_length - del_len);
                pmpriv->mgmt_ie[index].ie_length -= del_len;
                insert = 1;
                tmp_ie = pmpriv->mgmt_ie[index].ie_buffer;
                tmp_len = pmpriv->mgmt_ie[index].ie_length;
                cnt = 0;
                continue;
            }
            tmp_ie++;
            tmp_len--;
            cnt++;
        }
        if (MLAN_CUSTOM_IE_AUTO_IDX_MASK != idx)
            break;
    }
    if (index == MAX_MGMT_IE_INDEX && !insert) {
        PRINTM(MERROR, "Failed to Clear IE buffer\n");
        ret = MLAN_STATUS_FAILURE;
        goto done;
    }
  done:
    LEAVE();
    return ret;
}

/**
 *  @brief Set the custom IE
 *
 *  @param pmadapter	A pointer to mlan_adapter structure
 *  @param pioctl_req	A pointer to ioctl request buffer
 *
 *  @return		MLAN_STATUS_SUCCESS --success, otherwise fail
 */
mlan_status
wlan_misc_ioctl_custom_ie_list(IN pmlan_adapter pmadapter,
                               IN pmlan_ioctl_req pioctl_req)
{
    mlan_private *pmpriv = pmadapter->priv[pioctl_req->bss_num];
    mlan_status ret = MLAN_STATUS_SUCCESS;
    mlan_ds_misc_cfg *misc = (mlan_ds_misc_cfg *) pioctl_req->pbuf;
    custom_ie *ie_data = MNULL;
    t_u16 cmd_action, index, mask, i, len;
    t_u8 *tmp_ie;

    ENTER();

    if ((misc->param.cust_ie.len == 0) ||
        (misc->param.cust_ie.len == sizeof(t_u16))) {
        pioctl_req->action = MLAN_ACT_GET;
        /* Get the IE */
        cmd_action = HostCmd_ACT_GEN_GET;
    } else {
        ie_data = misc->param.cust_ie.ie_data_list;
        index = ie_data->ie_index;
        mask = ie_data->mgmt_subtype_mask;
        if (MLAN_CUSTOM_IE_AUTO_IDX_MASK == index) {    /* Need to be
                                                           Autohandled */
            if (mask == 0) {    /* Automatic Deletion */
                ret =
                    wlan_custom_ioctl_auto_delete(pmadapter, pioctl_req,
                                                  ie_data, index);
                memset(ie_data, 0, sizeof(pmpriv->mgmt_ie));
                len = 0;
                for (i = 0; i < MAX_MGMT_IE_INDEX; i++) {
                    memcpy((t_u8 *) ie_data + len, &i,
                           sizeof(ie_data->ie_index));
                    len += sizeof(ie_data->ie_index);
                    memcpy((t_u8 *) ie_data + len,
                           &pmpriv->mgmt_ie[i].mgmt_subtype_mask,
                           sizeof(ie_data->mgmt_subtype_mask));
                    len += sizeof(ie_data->mgmt_subtype_mask);
                    memcpy((t_u8 *) ie_data + len,
                           &pmpriv->mgmt_ie[i].ie_length,
                           sizeof(ie_data->ie_length));
                    len += sizeof(ie_data->ie_length);
                    if (pmpriv->mgmt_ie[i].ie_length) {
                        memcpy((t_u8 *) ie_data + len,
                               &pmpriv->mgmt_ie[i].ie_buffer,
                               pmpriv->mgmt_ie[i].ie_length);
                        len += pmpriv->mgmt_ie[i].ie_length;
                    }
                }
                misc->param.cust_ie.len = len;
                pioctl_req->action = MLAN_ACT_SET;
                cmd_action = HostCmd_ACT_GEN_SET;
            } else {            /* Automatic Addition */
                if (MLAN_CUSTOM_IE_AUTO_IDX_MASK == mask) {
                    mask = 0;
                    ie_data->mgmt_subtype_mask = 0;
                }
                if (MLAN_STATUS_FAILURE ==
                    wlan_custom_ioctl_get_autoidx(pmadapter, pioctl_req, mask,
                                                  ie_data, &index)) {
                    PRINTM(MERROR, "Failed to Set the IE buffer\n");
                    ret = MLAN_STATUS_FAILURE;
                    goto done;
                }
                if (MLAN_CUSTOM_IE_AUTO_IDX_MASK == index) {
                    ret = MLAN_STATUS_SUCCESS;
                    goto done;
                }
                tmp_ie = (t_u8 *) & pmpriv->mgmt_ie[index].ie_buffer;
                memcpy(tmp_ie + pmpriv->mgmt_ie[index].ie_length,
                       &ie_data->ie_buffer, ie_data->ie_length);
                pmpriv->mgmt_ie[index].ie_length += ie_data->ie_length;
                pmpriv->mgmt_ie[index].ie_index = index;
                pmpriv->mgmt_ie[index].mgmt_subtype_mask = mask;
                pioctl_req->action = MLAN_ACT_SET;
                cmd_action = HostCmd_ACT_GEN_SET;
                ie_data->ie_index = index;
                ie_data->ie_length = pmpriv->mgmt_ie[index].ie_length;
                memcpy(&ie_data->ie_buffer, &pmpriv->mgmt_ie[index].ie_buffer,
                       sizeof(custom_ie));
                misc->param.cust_ie.len = pmpriv->mgmt_ie[index].ie_length + 6;
            }
        } else {
            /* Set/Clear the IE and save it */
            if (ie_data->mgmt_subtype_mask == 0 && ie_data->ie_length) {
                PRINTM(MINFO, "Clear the IE buffer\n");
                ret =
                    wlan_custom_ioctl_auto_delete(pmadapter, pioctl_req,
                                                  ie_data, index);
                memset(ie_data, 0, sizeof(pmpriv->mgmt_ie));
                memcpy((t_u8 *) ie_data, &pmpriv->mgmt_ie[index],
                       pmpriv->mgmt_ie[index].ie_length + 6);
                misc->param.cust_ie.len = pmpriv->mgmt_ie[index].ie_length + 6;
                pioctl_req->action = MLAN_ACT_SET;
                cmd_action = HostCmd_ACT_GEN_SET;
            } else {
                PRINTM(MINFO, "Set the IE buffer\n");
                pioctl_req->action = MLAN_ACT_SET;
                cmd_action = HostCmd_ACT_GEN_SET;
                if (ie_data->mgmt_subtype_mask == 0)
                    ie_data->ie_length = 0;
                memset(&pmpriv->mgmt_ie[index], 0, sizeof(custom_ie));
                memcpy(&pmpriv->mgmt_ie[index], ie_data, sizeof(custom_ie));
            }
        }
    }

    /* Send command to firmware */
    if (pmpriv->bss_type == MLAN_BSS_TYPE_STA) {
        ret = wlan_prepare_cmd(pmpriv,
                               HostCmd_CMD_MGMT_IE_LIST,
                               cmd_action,
                               0, (t_void *) pioctl_req, &misc->param.cust_ie);
    }
#ifdef UAP_SUPPORT
    else if (pmpriv->bss_type == MLAN_BSS_TYPE_UAP) {
        ret = wlan_prepare_cmd(pmpriv,
                               HOST_CMD_APCMD_SYS_CONFIGURE,
                               cmd_action, 0, (t_void *) pioctl_req, MNULL);
    }
#endif
    if (ret == MLAN_STATUS_SUCCESS)
        ret = MLAN_STATUS_PENDING;
  done:
    LEAVE();
    return ret;
}
