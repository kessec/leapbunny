#ifndef _PIXART_CTRLS_H_
#define _PIXART_CTRLS_H_

#include "pixart_hex.h"

typedef enum {
	FW_CHECK_MATCH,
	FW_CHECK_MISMATCH,
	FW_CHECK_ERR
} FW_STATUS;

int install_xu_ctrls(int fd);
FW_STATUS check_cam_fw(int dev, const struct fw_chunk *ids_img);
int update_fw(int dev, const struct fw_chunk *img);

#endif /* _PIXART_CTRLS_H_ */
