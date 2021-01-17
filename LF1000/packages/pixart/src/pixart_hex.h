#ifndef _PIXART_HEX_H_
#define _PIXART_HEX_H_

#include <sys/types.h>

struct fw_chunk {
	unsigned char	*data;	/* binary data ready for programming */
	size_t		len;	/* length of binary data in bytes */
	off_t		offset;	/* where binary data should be programmed */
};


/*
 * This function reads the Intel HEX file descriptor fd of a PixArt *UI.hex
 * file, and populates id_img and ui_img accordingly.  Since PixArts *UI.hex
 * files are malformed, the ui_img chunk would be populated by:
 *
 * # cat UI.hex | sed -e 's/.$//' -e 's/^:0[^0].*$//g' -e '/^$/d' | srec_cat - -Intel -ignore-checksums -Crop 0x0100 -Offset -0x0100 -o UI.bin -Binary
 *
 * The id_img chunk is present in *UI.hex as the last record.
 */
int parse_ui_hex(int fd, struct fw_chunk *id_img, struct fw_chunk *ui_img, const struct fw_chunk *fw_img);

/*
 * This function reads the Intel HEX file descriptor fd of a PixArt *FW.hex
 * file, and populates fw_img accordingly.  The equilavlent shell operation
 * would be:
 *
 * # srec_cat FW.hex -Intel -ignore-checksums -Crop 0x8200 -Offset -0x8200 -o FW.bin -Binary
 */
int parse_fw_hex(int fd, struct fw_chunk *fw_img);

#endif /* _PIXART_HEX_H_ */
