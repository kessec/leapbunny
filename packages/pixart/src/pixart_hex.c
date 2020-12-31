/*
 * This file is responsible for parsing the .hex files supplied by PixArt
 * and converting them into usable binary form.
 */
#include "common.h"
#include "pixart_hex.h"
#define GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define ID_IMG_PROG_OFFSET	0x0000
#define ID_IMG_MAP_OFFSET	0x0000

#define UI_IMG_PROG_OFFSET	0x0100
#define UI_IMG_MAP_OFFSET	0x0100

#define FW_IMG_PROG_OFFSET	0x0300
#define FW_IMG_MAP_OFFSET	0x8200

/* return offset (address) encoded in Intel HEX line */
static off_t offset_of(const char *line)
{
	off_t off = 0;

	sscanf(&line[3], "%4hX", &off);
	return off;
}

/* return size (byte count) encoded in Intel HEX line */
static size_t size_of(const char *line)
{
	size_t size = 0;

	sscanf(&line[1], "%2hhX", &size);
	return size;
}

/* return record type encoded in Intel HEX line */
static unsigned char record_of(const char *line)
{
	unsigned char rec = 0;

	sscanf(&line[7], "%2hhX", &rec);
	return rec;
}

/* copy/convert Intel HEX to binary */
static int mem_cpy(unsigned char *dest, const char *src, size_t size)
{
	int i, j, val;

	for(i = j = 0; i < size; i++, j+= 2)
	{
		//sscanf(&src[j+9], "%2hhX", &dest[i]);
		sscanf(&src[j+9], "%2X", &val);
		dest[i] = val;
	}
}

static unsigned short checksum(const struct fw_chunk *fw_img, const struct fw_chunk *ui_img)
{
	int i;
	unsigned short total = 0;

	for(i = 0; i < ui_img->len; i++)
	{
		total += ui_img->data[i];
	}

	for(i = 0; i < fw_img->len; i++)
	{
		total += fw_img->data[i];
	}

	return total;
}

/* load malformed Intel HEX file fd starting at map_offset into img */
static int parse_chunk_id(int fd, struct fw_chunk *img, const off_t map_offset, const struct fw_chunk *fw_img, const struct fw_chunk *ui_img)
{
	int i, j, ret	= 0;
	FILE *file	= NULL;
	char *line	= NULL;
	size_t len	= 0;	/* size of line read from HEX file */
	ssize_t read;
	off_t offset	= 0;
	unsigned char rec = 0;
	unsigned short length, cksum, pid, vid, vers;
	unsigned char flags[6];
	void *tmp;

	file = fdopen(fd, "r");
	if(file == NULL)
	{
		ret = errno;
		goto out;
	}

	while ((read = getline(&line, &len, file)) != -1)
	{
		offset = offset_of(line);
		rec = record_of(line);
		if((offset == map_offset) && (rec != 0x1))
		{
			sscanf(&line[7], "%4hX", &pid);
			sscanf(&line[11], "%4hX", &vid);
			sscanf(&line[15], "%4hX", &vers);
			for(i = j = 0; i < NUM_ELEMS(flags); i++, j+=2)
			{
				sscanf(&line[19+j], "%2hhX", &flags[i]);
			}

			tmp	= realloc(img->data, 0xF);
			if(tmp == NULL)
			{
				ret = -ENOMEM;
				if(img->data)
				{
					free(img->data);
					img->data = NULL;
					img->len = 0;
				}
				goto err;
			}
			img->data	= tmp;
			img->len	= 0xF;

			length = fw_img->len + ui_img->len;
			cksum = checksum(fw_img, ui_img);

			img->data[0x0] = length >> 8;
			img->data[0x1] = length & 0xFF;
			img->data[0x2] = cksum >> 8;
			img->data[0x3] = cksum & 0xFF;
			img->data[0x4] = vers >> 8;
			img->data[0x5] = vers & 0xFF;
			img->data[0x6] = vid >> 8;
			img->data[0x7] = vid & 0xFF;
			img->data[0x8] = pid >> 8;
			img->data[0x9] = pid & 0xFF;
			memcpy(&img->data[0xA], flags, NUM_ELEMS(flags));

			break;
		}
	}
err:
	if(line)
	{
		free(line);
	}
	fclose(file);
out:
	return ret;
}


/* load Intel HEX file fd starting at map_offset into img */
static int parse_chunk_hex(int fd, struct fw_chunk *img, const off_t map_offset)
{
	int ret	= 0;
	FILE *file	= NULL;
	char *line	= NULL;
	size_t len	= 0;	/* size of line read from HEX file */
	size_t size	= 0;	/* growing size of binary file */
	ssize_t read;
	off_t offset	= 0;
	void *tmp	= NULL;

	file = fdopen(fd, "r");
	if(file == NULL)
	{
		ret = errno;
		goto out;
	}

	while ((read = getline(&line, &len, file)) != -1)
	{
		if((offset = offset_of(line)) >= map_offset)
		{
			offset -= map_offset;
			size = size_of(line);

			if(offset + size > img->len)
			{

				tmp	= realloc(img->data, offset + size);
				if(tmp == NULL)
				{
					ret = -ENOMEM;
					if(img->data)
					{
						free(img->data);
						img->data = NULL;
						img->len = 0;
					}

					goto err;
				}
				img->data	= tmp;
				img->len	= offset + size;
			}

			mem_cpy(&img->data[offset], line, size); 
		}
	}
err:
	if(line)
	{
		free(line);
	}

	fclose(file);
out:
	return ret;
}

int parse_ui_hex(int fd, struct fw_chunk *id_img, struct fw_chunk *ui_img, const struct fw_chunk *fw_img)
{
	int ret = 0;
	int fd2;

	fd2 = dup(fd);
	if(fd == -1)
	{
		ret = errno;
		goto out;
	}

	ui_img->offset = UI_IMG_PROG_OFFSET;
	ret = parse_chunk_hex(fd, ui_img, UI_IMG_MAP_OFFSET);
	if(ret == 0)
	{
		lseek(fd2, 0, SEEK_SET);
		/*
		 * the final record (VID, PID, version, flags) @ offset 0 is
		 * malformed, so it requires special handling
		 */
		id_img->offset = ID_IMG_PROG_OFFSET;
		ret = parse_chunk_id(fd2, id_img, ID_IMG_MAP_OFFSET, fw_img, ui_img);
	}
out:
	return ret;
}

int parse_fw_hex(int fd, struct fw_chunk *img)
{
	img->offset = FW_IMG_PROG_OFFSET;
	return parse_chunk_hex(fd, img, FW_IMG_MAP_OFFSET);
}
