/*
 * drivers/video/lf1000fb.c
 *
 * Framebuffer support for the LF1000/Pollux SoC.
 *
 * Copyright 2010 LeapFrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * Parts of this are based on the CL7700 FB driver which is (C) 2009 CoreLogic 
 * Inc. and written by Sungdae Choi.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/fb.h>
#include <linux/platform_device.h>

#include <mach/platform.h>
#include <mach/screen.h>

#define DRIVER_NAME		"lf1000-fb"

#define RESSIZE(res)		(((res)->end - (res)->start)+1)

#define PALETTE_CLEAR		0x80000000

/*
 * Settings
 */

#define LF1000_FB_NUM_LAYERS	3
#define LF1000_FB_RGB_LAYER	0	/* RGB 2D graphics */
#define LF1000_FB_YUV_LAYER	1	/* YUV video */
#define LF1000_FB_OGL_LAYER	2	/* 3D engine */

struct lf1000fb_layer {
	u8			index;
	struct lf1000fb_info	*parent;
	struct fb_info		*fbinfo;
	void __iomem		*mem;

	unsigned int pseudo_pal[16];
	unsigned int palette_buffer[256];
};

struct lf1000fb_info {
	struct platform_device		*pdev;
	struct resource			*fbres;
	struct resource			*mlcres;
	struct lf1000_screen_info	*screen;

	void __iomem			*fbmem;
	void __iomem			*mlc;

	struct fb_info *fbs[LF1000_FB_NUM_LAYERS];
};

/*
 * framebuffer interface 
 */

#define MLCSCREENSIZE	0x04

#define MLCCONTROL0	0x24
#define MLCHSTRIDE0	0x28
#define MLCVSTRIDE0	0x2C
#define MLCPALETTE0	0x3C

#define MLCCONTROL1     0x58
#define MLCHSTRIDE1     0x5C
#define MLCVSTRIDE1     0x60
#define MLCPALETTE1	0x70
#define MLCCONTROL2     0x7C
#define MLCVSTRIDE2     0x80

/* MLCCONTROLn register */
#define FORMAT		16	/* see Table 21-5 */
#define LOCKSIZE	12
#define DIRTYFLAG	4

static u32 get_fb_size(struct lf1000fb_info *info)
{
	u32 size = info->screen->xres * info->screen->yres * 4;

	if (size % 4096)
		return (size/4096 + 1)*4096;
	return size;
}

static u32 mlc_control(u8 index)
{
	if (index == 0)
		return MLCCONTROL0;
	if (index == 1)
		return MLCCONTROL1;
	return MLCCONTROL2;
}

static u32 mlc_hstride(u8 index)
{
	if (index == 0)
		return MLCHSTRIDE0;
	return MLCHSTRIDE1;
}

static u32 mlc_vstride(u8 index)
{
	if (index == 0)
		return MLCVSTRIDE0;
	if (index == 1)
		return MLCVSTRIDE1;
	return MLCVSTRIDE2;
}

static void mlc_set_format(struct fb_info *fbinfo, u16 format)
{
	struct lf1000fb_layer *fbi = fbinfo->par;
	struct fb_var_screeninfo *var = &fbinfo->var;
	void __iomem *mlc = fbi->parent->mlc;
	u8 i = fbi->index;
	u32 tmp;

	dev_dbg(&fbi->parent->pdev->dev, "setting BPP=%d, format=0x%X\n",
			var->bits_per_pixel, format);

	if (i < 2)
		writel(var->bits_per_pixel>>3, mlc + mlc_hstride(i));
	writel((var->bits_per_pixel>>3)*var->xres, mlc + mlc_vstride(i));
	
	tmp = readl(mlc + mlc_control(i)) & ~(0xFFFF<<FORMAT);
	writel(tmp | (format<<FORMAT), mlc + mlc_control(i));

	/* update */
	tmp = readl(mlc + mlc_control(i));
	writel(tmp | (1<<DIRTYFLAG), mlc + mlc_control(i));
}

static int set_rgb(struct fb_info *fbi, struct fb_var_screeninfo *var,
		bool apply)
{
	switch (var->bits_per_pixel) {
		case 1:
		case 2:
		case 4:
		case 8:
			/* indexed colors */
			var->red.length = var->bits_per_pixel;
			var->green.length = var->bits_per_pixel;
			var->blue.length = var->bits_per_pixel;

			if (apply)
				mlc_set_format(fbi, 0x443A);
			break;

		case 16:
			/* RGB 565 */
			var->red.length = 5;
			var->green.length = 6;
			var->blue.length = 5;
			var->transp.length = 0;
			
			var->red.offset = var->green.length + var->blue.length;
			var->green.offset = var->blue.length;
			var->blue.offset = 0;
			var->transp.offset = 0;

			if (apply)
				mlc_set_format(fbi, 0x4432);
			break;

		case 24:
			/* RGB 888 */
			var->red.length = 8;
			var->green.length = 8;
			var->blue.length = 8;
			var->transp.length = 0;

			var->blue.offset = 0;
			var->green.offset = var->blue.length;
			var->red.offset = var->blue.length + var->green.length;
			var->transp.offset = 0;

			if (apply)
				mlc_set_format(fbi, 0x4653);
			break;

		case 32:
			/* (A)BGR 888 */
			var->red.length = 8;
			var->green.length = 8;
			var->blue.length = 8;
			if (var->transp.length) {
				var->transp.offset = var->red.length +
					var->green.length + var->blue.length;
			}

			var->red.offset = 0;
			var->green.offset = var->red.length;
			var->blue.offset = var->red.length + var->green.length;

			if (apply)
				mlc_set_format(fbi, 0x8653);
			break;

		default:
			return -EINVAL;
	}	

	return 0;
}

static void schedule_palette_update(struct lf1000fb_layer *fbi,
		unsigned int regno, unsigned int val)
{
	unsigned long flags;
	
	local_irq_save(flags);

	fbi->palette_buffer[regno] = val;

	local_irq_restore(flags);
}

static unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	return ((chan & 0xffff) >> (16 - bf->length)) << bf->offset;
}

static u16 to_rgb888(u16 red, u16 green, u16 blue)
{
	red >>= 8;
	green >>= 8;
	blue >>= 8;

	return (red << 16) | (green << 8) | blue;
}

static u16 to_rgb565(u16 red, u16 green, u16 blue)
{
	return ((red>>0) & 0xF800)|((green>>5) & 0x07E0)|((blue>>11) & 0x001F);
}

static int lf1000fb_setcolreg(unsigned regno, unsigned red, unsigned green,
		unsigned blue, unsigned transp, struct fb_info *info)
{
	u16 val;
	struct lf1000fb_layer *fbi = info->par;

	if (fbi->index > 1)
		return -EINVAL;

	if (info->fix.visual == FB_VISUAL_TRUECOLOR && regno < 16) {
		val = chan_to_field(red, &info->var.red);
		val |= chan_to_field(green, &info->var.green);
		val |= chan_to_field(blue, &info->var.blue);
		
		fbi->pseudo_pal[regno] = val;

		return 0;
	}
	
	if (info->fix.visual == FB_VISUAL_PSEUDOCOLOR && regno < 256) {
		if (fbi->fbinfo->var.bits_per_pixel == 16)
			val = to_rgb565(red, green, blue);
		else
			val = to_rgb888(red, green, blue);

		schedule_palette_update(fbi, regno, val);

		return 0;
	}
	
	return -EINVAL;
}

static int lf1000fb_check_var(struct fb_var_screeninfo *var,
		struct fb_info *info)
{
	struct lf1000fb_layer *layer = info->par;

	if (var->xres != layer->parent->screen->xres ||
	    var->yres != layer->parent->screen->yres)
		return -EINVAL;

	return set_rgb(info, var, 0);
}

static int lf1000fb_set_par(struct fb_info *info)
{
	struct lf1000fb_layer *layer = info->par;
	struct fb_var_screeninfo *var = &info->var;

	if (var->xres != layer->parent->screen->xres ||
	    var->yres != layer->parent->screen->yres)
		return -EINVAL;

	switch (var->bits_per_pixel) {
		case 16:
			info->fix.visual = FB_VISUAL_TRUECOLOR;
			break;
		case 24:
		case 32:
			info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
			break;
		default:
			return -EINVAL;
	}

	info->fix.line_length = var->xres_virtual * (var->bits_per_pixel>>3);

	return set_rgb(info, var, 1);
}

static struct fb_ops lf1000fb_ops = {
	.owner		= THIS_MODULE,
	.fb_setcolreg	= lf1000fb_setcolreg,
	.fb_check_var	= lf1000fb_check_var,
	.fb_set_par	= lf1000fb_set_par,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

/*
 * platform device
 */

static int lf1000fb_map_resource(struct lf1000fb_info *info,
		struct resource **res, void __iomem **mem,
		u8 index, const char *name)
{
	*res = platform_get_resource(info->pdev, IORESOURCE_MEM, index);
	if (!res) {
		dev_err(&info->pdev->dev, "can't get %s resource\n", name);
		return -ENXIO;
	}

	if (!request_mem_region((*res)->start, RESSIZE(*res), name)) {
		dev_err(&info->pdev->dev, "can't request %s memory\n", name);
		release_resource(*res);
		*res = NULL;
		return -EBUSY;
	}

	*mem = ioremap_nocache((*res)->start, RESSIZE(*res));
	if(!mem) {
		dev_err(&info->pdev->dev, "can't remap %s memory\n", name);
		release_mem_region((*res)->start, RESSIZE(*res));
		return -ENOMEM;
	}

	return 0;
}

static void lf1000fb_unmap_resource(struct resource **res, void __iomem **mem)
{
	if (*mem) {
		iounmap(*mem);
		*mem = NULL;
	}

	if (*res) {
		release_mem_region((*res)->start, RESSIZE(*res));
		release_resource(*res);
		*res = NULL;
	}
}

static void __init lf1000fb_set_fixed(struct lf1000fb_info *info,
		struct fb_fix_screeninfo *fix, struct resource *fb, u8 index)
{
	fix->smem_start = fb->start;
	fix->smem_len = RESSIZE(fb);

	snprintf(fix->id, 16, DRIVER_NAME "%d", index);
	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->accel = FB_ACCEL_NONE;
}

static int __init lf1000fb_probe_layer(struct lf1000fb_info *info, u8 index)
{
	int ret;
	struct fb_info *fbi;
	struct lf1000fb_layer *layer;

	if (index >= LF1000_FB_NUM_LAYERS)
		return -EINVAL;

	fbi = framebuffer_alloc(sizeof(struct lf1000fb_layer),
			&info->pdev->dev);
	if (fbi == NULL) {
		dev_err(&info->pdev->dev, "can't allocate layer %d\n", index);
		return -ENOMEM;
	}

	layer = fbi->par;
	layer->parent = info;
	layer->fbinfo = fbi;
	layer->mem = info->fbmem + get_fb_size(info)*index;
	fbi->pseudo_palette = layer->pseudo_pal;

	memset(layer->pseudo_pal, PALETTE_CLEAR,
			ARRAY_SIZE(layer->pseudo_pal));

	info->fbs[index] = fbi;

	/* configure framebuffer */

	lf1000fb_set_fixed(info, &fbi->fix, info->fbres, index);

	fbi->var.xres		= info->screen->xres;
	fbi->var.yres		= info->screen->yres;
	fbi->var.xres_virtual	= fbi->var.xres;
	fbi->var.yres_virtual	= fbi->var.yres;
	fbi->var.xoffset	= 0;
	fbi->var.yoffset	= 0;
	fbi->var.bits_per_pixel	= 16;

	lf1000fb_set_par(fbi); /* set up the hardware */

	fbi->var.activate	= FB_ACTIVATE_FORCE | FB_ACTIVATE_NOW;

	fbi->fbops		= &lf1000fb_ops;
	fbi->flags		= FBINFO_DEFAULT;
	fbi->node		= -1;

	fbi->screen_size = RESSIZE(info->fbres);
	fbi->screen_base = layer->mem;

	ret = fb_alloc_cmap(&fbi->cmap, 256, 0);
	if (ret) {
		dev_err(&info->pdev->dev, "failed to allocate FB%d cmap\n",
				index);
		goto out_layer;
	}

	ret = register_framebuffer(fbi);
	if (ret) {
		dev_err(&info->pdev->dev, "failed to register FB%d device\n",
				index);
		goto out_layer;
	}

	return 0;

out_layer:
	framebuffer_release(fbi);
	info->fbs[index] = NULL;

	return ret;
}

static int __init lf1000fb_probe(struct platform_device *pdev)
{
	struct lf1000fb_info *info = NULL;
	struct fb_info *fbinfo;
	int i;
	int ret = 0;

	fbinfo = framebuffer_alloc(sizeof(struct lf1000fb_info), &pdev->dev);
	if (!fbinfo) {
		dev_err(&pdev->dev, "can't allocate FB\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, info);

	info = fbinfo->par;
	info->pdev = pdev;
	info->screen = lf1000_get_screen_info();

	ret = lf1000fb_map_resource(info, &info->fbres, &info->fbmem, 0,
			DRIVER_NAME);
	if (ret)
		goto out_fbmem;

	ret = lf1000fb_map_resource(info, &info->mlcres, &info->mlc, 1,
			DRIVER_NAME "-mlc");
	if (ret) {
		lf1000fb_unmap_resource(&info->fbres, &info->fbmem);
		goto out_mlc;
	}

	/* set MLC screen size */
	writel((info->screen->xres-1) | ((info->screen->yres-1)<<16),
	       info->mlc + MLCSCREENSIZE);

	for (i = 0; i < LF1000_FB_NUM_LAYERS; i++) {
		dev_dbg(&info->pdev->dev, "adding FB%d\n", i);
		ret = lf1000fb_probe_layer(info, i);
		if (ret)
			goto out_fb;
	}

	return 0;

out_fb:
	lf1000fb_unmap_resource(&info->mlcres, &info->mlc);
out_mlc:
	lf1000fb_unmap_resource(&info->fbres, &info->fbmem);
out_fbmem:
	framebuffer_release(fbinfo);
	platform_set_drvdata(pdev, NULL);

	return ret;;
}

static int lf1000fb_remove(struct platform_device *pdev)
{
	struct fb_info *fbinfo = platform_get_drvdata(pdev);
	struct lf1000fb_info *info = fbinfo->par;
	struct lf1000fb_layer *layer;
	int i;

	for (i = 0; i < LF1000_FB_NUM_LAYERS; i++) {
		if (info->fbs[i]) {
			dev_dbg(&info->pdev->dev, "removing FB%d\n", i);
			layer = (info->fbs[i])->par;
			fb_dealloc_cmap(&layer->fbinfo->cmap);
			unregister_framebuffer(layer->fbinfo);
			framebuffer_release(layer->fbinfo);
		}
	}

	lf1000fb_unmap_resource(&info->mlcres, &info->mlc);
	lf1000fb_unmap_resource(&info->fbres, &info->fbmem);

	framebuffer_release(fbinfo);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver lf1000fb_driver = {
	.probe		= lf1000fb_probe,
	.remove		= lf1000fb_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
};

static int lf1000fb_init(void)
{
	return platform_driver_register(&lf1000fb_driver);
}

static void __exit lf1000fb_exit(void)
{
	platform_driver_unregister(&lf1000fb_driver);
}

module_init(lf1000fb_init);
module_exit(lf1000fb_exit);

MODULE_DESCRIPTION("LF1000 Frame Buffer Driver");
MODULE_AUTHOR("Andrey Yurovsky");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:lf1000-fb");
