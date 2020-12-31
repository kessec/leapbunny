/** @file moal_sdio_mmc.c
 *
 *  @brief This file contains SDIO MMC IF (interface) module
 *  related functions.
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
/****************************************************
Change log:
	02/25/09: Initial creation -
		  This file supports SDIO MMC only
****************************************************/

#include <linux/firmware.h>

#include "moal_sdio.h"

/** define marvell vendor id */
#define MARVELL_VENDOR_ID 0x02df

/********************************************************
		Local Variables
********************************************************/

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/

/** 
 *  @brief This function handles the interrupt.
 *  
 *  @param func	   A pointer to the sdio_func structure
 *  @return 	   None
 */
static void
woal_sdio_interrupt(struct sdio_func *func)
{
    moal_handle *handle;
    struct sdio_mmc_card *card;

    ENTER();

    card = sdio_get_drvdata(func);
    if (!card || !card->handle) {
        PRINTM(MINFO,
               "sdio_mmc_interrupt(func = %p) card or handle is NULL, card=%p\n",
               func, card);
        LEAVE();
        return;
    }
    handle = card->handle;

    PRINTM(MINFO, "*** IN SDIO IRQ ***\n");
    woal_interrupt(handle);

    LEAVE();
}

/**  @brief This function handles client driver probe.
 *  
 *  @param func	   A pointer to sdio_func structure.
 *  @param id	   A pointer to sdio_device_id structure.
 *  @return 	   MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
static int
woal_sdio_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
    int ret = MLAN_STATUS_SUCCESS;
    struct sdio_mmc_card *card = NULL;
    moal_handle *handle;

    ENTER();

    PRINTM(MINFO, "vendor=0x%4.04X device=0x%4.04X class=%d function=%d\n",
           func->vendor, func->device, func->class, func->num);

    card = kzalloc(sizeof(struct sdio_mmc_card), GFP_KERNEL);
    if (!card) {
        PRINTM(MFATAL, "Failed to allocate memory in probe function!\n");
        return -ENOMEM;
    }

    card->func = func;

#ifdef MMC_QUIRK_BLKSZ_FOR_BYTE_MODE
    /* The byte mode patch is available in kernel MMC driver which fixes one
       issue in MP-A transfer. bit1: use func->cur_blksize for byte mode */
    func->card->quirks |= MMC_QUIRK_BLKSZ_FOR_BYTE_MODE;
#endif

    if (NULL == (handle = woal_add_card(card))) {
        PRINTM(MERROR, "woal_add_card failed\n");
        kfree(card);
        ret = MLAN_STATUS_FAILURE;
    }

    LEAVE();
    return ret;
}

/**  @brief This function handles client driver remove.
 *  
 *  @param func	   A pointer to sdio_func structure.
 *  @return 	   n/a
 */
static void
woal_sdio_remove(struct sdio_func *func)
{
    struct sdio_mmc_card *card;

    ENTER();

    PRINTM(MINFO, "SDIO func=%d\n", func->num);

    if (func) {
        card = sdio_get_drvdata(func);
        if (card) {
            woal_remove_card(card);
            kfree(card);
        }
    }

    LEAVE();
}

#ifdef SDIO_SUSPEND_RESUME
/**  @brief This function handles client driver suspend
 *  
 *  @param dev	   A pointer to device structure
 *  @return 	   MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
int
woal_sdio_suspend(struct device *dev)
{
    struct sdio_func *func = dev_to_sdio_func(dev);
    mmc_pm_flag_t pm_flags = 0;
    moal_handle *handle = NULL;
    struct sdio_mmc_card *cardp;
    int i;
#ifdef STA_SUPPORT
    mlan_ioctl_req *req = NULL;
    mlan_ds_pm_cfg *pmcfg = NULL;
    int hs_actived = 0;
#endif /* STA_SUPPORT */

    ENTER();

    if (func) {
        pm_flags = sdio_get_host_pm_caps(func);
        PRINTM(MCMND, "%s: suspend: PM flags = 0x%x\n", sdio_func_id(func),
               pm_flags);
        if (!(pm_flags & MMC_PM_KEEP_POWER)) {
            PRINTM(MERROR, "%s: cannot remain alive while host is suspended\n",
                   sdio_func_id(func));
            LEAVE();
            return -ENOSYS;
        }
        cardp = sdio_get_drvdata(func);
        if (!cardp || !cardp->handle) {
            PRINTM(MERROR, "Card or moal_handle structure is not valid\n");
            LEAVE();
            return MLAN_STATUS_SUCCESS;
        }
    } else {
        PRINTM(MERROR, "sdio_func is not specified\n");
        LEAVE();
        return MLAN_STATUS_SUCCESS;
    }
    handle = cardp->handle;

#ifdef STA_SUPPORT
    if (handle->hs_activated == MTRUE) {
        PRINTM(MCMND, "HS Already actived\n");
        hs_actived = MTRUE;
        goto done;
    }

    /* Enable Host Sleep */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_pm_cfg));
    if (req == NULL) {
        PRINTM(MERROR, "Falied to allocate buffer for IOCTL\n");
        goto done;
    }
    pmcfg = (mlan_ds_pm_cfg *) req->pbuf;
    pmcfg->sub_command = MLAN_OID_PM_CFG_HS_CFG;
    req->req_id = MLAN_IOCTL_PM_CFG;
    req->action = MLAN_ACT_SET;

    woal_get_hs_params(woal_get_priv(handle, MLAN_BSS_TYPE_STA),
                       MOAL_IOCTL_WAIT, &(pmcfg->param.hs_cfg));
    if (pmcfg->param.hs_cfg.conditions == HOST_SLEEP_CFG_CANCEL)
        goto done;
    handle->hs_activate_wait_q_woken = MFALSE;
    pmcfg->param.hs_cfg.is_invoke_hostcmd = MTRUE;
    if (woal_request_ioctl
        (woal_get_priv(handle, MLAN_BSS_TYPE_STA), req,
         MOAL_IOCTL_WAIT) != MLAN_STATUS_SUCCESS) {
        PRINTM(MERROR, "IOCTL request failed\n");
        goto done;
    }
    if (pmcfg->param.hs_cfg.conditions != HOST_SLEEP_CFG_CANCEL) {
        handle->hs_activate_wait_q_woken = MFALSE;
        wait_event_interruptible(handle->hs_activate_wait_q,
                                 handle->hs_activate_wait_q_woken);
        hs_actived = MTRUE;
    }
  done:
    if (req)
        kfree(req);
#endif /* STA_SUPPORT */
    /* Indicate device suspended */
    handle->is_suspended = MTRUE;
    for (i = 0; i < handle->priv_num; i++)
        netif_carrier_off(handle->priv[i]->netdev);

    LEAVE();
#ifdef STA_SUPPORT
    if (hs_actived) {
        PRINTM(MCMND, "suspend with MMC_PM_KEEP_POWER\n");
        return sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
    } else
#endif /* STA_SUPPORT */
        return MLAN_STATUS_SUCCESS;
}

/**  @brief This function handles client driver resume
 *  
 *  @param dev	   A pointer to device structure
 *  @return 	   MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
int
woal_sdio_resume(struct device *dev)
{
    struct sdio_func *func = dev_to_sdio_func(dev);
    mmc_pm_flag_t pm_flags = 0;
    moal_handle *handle = NULL;
    struct sdio_mmc_card *cardp;
    int i;

    ENTER();

    if (func) {
        pm_flags = sdio_get_host_pm_caps(func);
        cardp = sdio_get_drvdata(func);
        if (!cardp || !cardp->handle) {
            PRINTM(MERROR, "Card or moal_handle structure is not valid\n");
            LEAVE();
            return MLAN_STATUS_SUCCESS;
        }
    } else {
        PRINTM(MERROR, "sdio_func is not specified\n");
        LEAVE();
        return MLAN_STATUS_SUCCESS;
    }
    handle = cardp->handle;

    if (handle->is_suspended == MFALSE) {
        PRINTM(MWARN, "Device already resumed\n");
        LEAVE();
        return MLAN_STATUS_SUCCESS;
    }
    handle->is_suspended = MFALSE;
    for (i = 0; i < handle->priv_num; i++)
        netif_carrier_on(handle->priv[i]->netdev);

#ifdef STA_SUPPORT
    /* Disable Host Sleep */
    woal_hs_cfg_cancel(woal_get_priv(handle, MLAN_BSS_TYPE_STA), MOAL_NO_WAIT);
#endif /* STA_SUPPORT */

    LEAVE();
    return MLAN_STATUS_SUCCESS;

}
#endif

/** Device ID for SD8786 */
#define SD_DEVICE_ID_8786   (0x9116)

/** WLAN IDs */
static const struct sdio_device_id wlan_ids[] = {
    {SDIO_DEVICE(MARVELL_VENDOR_ID, SD_DEVICE_ID_8786)},
    {},
};

#ifdef SDIO_SUSPEND_RESUME
static struct dev_pm_ops wlan_sdio_pm_ops = {
    .suspend = woal_sdio_suspend,
    .resume = woal_sdio_resume,
};
#endif
static struct sdio_driver wlan_sdio = {
    .name = "wlan_sdio",
    .id_table = wlan_ids,
    .probe = woal_sdio_probe,
    .remove = woal_sdio_remove,
#ifdef SDIO_SUSPEND_RESUME
    .drv = {
            .pm = &wlan_sdio_pm_ops,
            }
#endif
};

/********************************************************
		Global Functions
********************************************************/

/** 
 *  @brief This function writes data into card register
 *
 *  @param handle   A Pointer to the moal_handle structure
 *  @param reg      Register offset
 *  @param data     Value
 *
 *  @return    		MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_write_reg(moal_handle * handle, t_u32 reg, t_u32 data)
{
    mlan_status ret = MLAN_STATUS_FAILURE;
    sdio_writeb(((struct sdio_mmc_card *) handle->card)->func, (t_u8) data, reg,
                (int *) &ret);
    return ret;
}

/** 
 *  @brief This function reads data from card register
 *
 *  @param handle   A Pointer to the moal_handle structure
 *  @param reg      Register offset
 *  @param data     Value
 *
 *  @return    		MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_read_reg(moal_handle * handle, t_u32 reg, t_u32 * data)
{
    mlan_status ret = MLAN_STATUS_FAILURE;
    t_u8 val;

    val =
        sdio_readb(((struct sdio_mmc_card *) handle->card)->func, reg,
                   (int *) &ret);
    *data = val;

    return ret;
}

/**
 *  @brief This function writes multiple bytes into card memory
 *
 *  @param handle   A Pointer to the moal_handle structure
 *  @param pmbuf	Pointer to mlan_buffer structure
 *  @param port		Port
 *  @param timeout 	Time out value
 *
 *  @return    		MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_write_data_sync(moal_handle * handle, mlan_buffer * pmbuf, t_u32 port,
                     t_u32 timeout)
{
    mlan_status ret = MLAN_STATUS_FAILURE;
    t_u8 *buffer = (t_u8 *) (pmbuf->pbuf + pmbuf->data_offset);
    t_u8 blkmode = (port & MLAN_SDIO_BYTE_MODE_MASK) ? BYTE_MODE : BLOCK_MODE;
    t_u32 blksz = (blkmode == BLOCK_MODE) ? MLAN_SDIO_BLOCK_SIZE : 1;
    t_u32 blkcnt =
        (blkmode ==
         BLOCK_MODE) ? (pmbuf->data_len /
                        MLAN_SDIO_BLOCK_SIZE) : pmbuf->data_len;
    t_u32 ioport = (port & MLAN_SDIO_IO_PORT_MASK);

    if (!sdio_writesb
        (((struct sdio_mmc_card *) handle->card)->func, ioport, buffer,
         blkcnt * blksz))
        ret = MLAN_STATUS_SUCCESS;

    return ret;
}

/**
 *  @brief This function reads multiple bytes from card memory
 *
 *  @param handle   A Pointer to the moal_handle structure
 *  @param pmbuf	Pointer to mlan_buffer structure
 *  @param port		Port
 *  @param timeout 	Time out value
 *
 *  @return    		MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_read_data_sync(moal_handle * handle, mlan_buffer * pmbuf, t_u32 port,
                    t_u32 timeout)
{
    mlan_status ret = MLAN_STATUS_FAILURE;
    t_u8 *buffer = (t_u8 *) (pmbuf->pbuf + pmbuf->data_offset);
    t_u8 blkmode = (port & MLAN_SDIO_BYTE_MODE_MASK) ? BYTE_MODE : BLOCK_MODE;
    t_u32 blksz = (blkmode == BLOCK_MODE) ? MLAN_SDIO_BLOCK_SIZE : 1;
    t_u32 blkcnt =
        (blkmode ==
         BLOCK_MODE) ? (pmbuf->data_len /
                        MLAN_SDIO_BLOCK_SIZE) : pmbuf->data_len;
    t_u32 ioport = (port & MLAN_SDIO_IO_PORT_MASK);

    if (!sdio_readsb
        (((struct sdio_mmc_card *) handle->card)->func, buffer, ioport,
         blkcnt * blksz))
        ret = MLAN_STATUS_SUCCESS;

    return ret;
}

/** 
 *  @brief This function registers the IF module in bus driver
 *  
 *  @return	   MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_bus_register(void)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;

    ENTER();

    /* SDIO Driver Registration */
    if (sdio_register_driver(&wlan_sdio)) {
        PRINTM(MFATAL, "SDIO Driver Registration Failed \n");
        LEAVE();
        return MLAN_STATUS_FAILURE;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief This function de-registers the IF module in bus driver
 *  
 *  @return 	   None
 */
void
woal_bus_unregister(void)
{
    ENTER();

    /* SDIO Driver Unregistration */
    sdio_unregister_driver(&wlan_sdio);

    LEAVE();
}

/** 
 *  @brief This function de-registers the device
 *  
 *  @param handle A pointer to moal_handle structure
 *  @return 	  None
 */
void
woal_unregister_dev(moal_handle * handle)
{
    ENTER();
    if (handle->card) {
        /* Release the SDIO IRQ */
        sdio_claim_host(((struct sdio_mmc_card *) handle->card)->func);
        sdio_release_irq(((struct sdio_mmc_card *) handle->card)->func);
        sdio_disable_func(((struct sdio_mmc_card *) handle->card)->func);
        sdio_release_host(((struct sdio_mmc_card *) handle->card)->func);

        sdio_set_drvdata(((struct sdio_mmc_card *) handle->card)->func, NULL);

        PRINTM(MWARN, "Making the sdio dev card as NULL\n");
    }

    LEAVE();
}

/** 
 *  @brief This function registers the device
 *  
 *  @param handle  A pointer to moal_handle structure
 *  @return 	   MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_register_dev(moal_handle * handle)
{
    int ret = MLAN_STATUS_SUCCESS;
    struct sdio_mmc_card *card = handle->card;
    struct sdio_func *func;

    ENTER();

    func = card->func;
    sdio_claim_host(func);
    ret = sdio_enable_func(func);
    if (ret) {
        PRINTM(MFATAL, "sdio_enable_func() failed: ret=%d\n", ret);
        goto release_host;
    }

    /* Request the SDIO IRQ */
    ret = sdio_claim_irq(func, woal_sdio_interrupt);
    if (ret) {
        PRINTM(MFATAL, "sdio_claim_irq failed: ret=%d\n", ret);
        goto disable_func;
    }

    /* Set block size */
    ret = sdio_set_block_size(card->func, MLAN_SDIO_BLOCK_SIZE);
    if (ret) {
        PRINTM(MERROR, "sdio_set_block_seize(): cannot set SDIO block size\n");
        ret = MLAN_STATUS_FAILURE;
        goto release_irq;
    }

    sdio_release_host(func);
    sdio_set_drvdata(func, card);

    handle->hotplug_device = &func->dev;

    LEAVE();
    return MLAN_STATUS_SUCCESS;

  release_irq:
    sdio_release_irq(func);
  disable_func:
    sdio_disable_func(func);
  release_host:
    sdio_release_host(func);
    handle->card = NULL;

    LEAVE();
    return MLAN_STATUS_FAILURE;
}

/** 
 *  @brief This function set bus clock on/off
 *  
 *  @param handle    A pointer to moal_handle structure
 *  @param option    TRUE--on , FALSE--off
 *  @return 	   MLAN_STATUS_SUCCESS
 */
int
woal_sdio_set_bus_clock(moal_handle * handle, t_u8 option)
{
    return MLAN_STATUS_SUCCESS;
}
