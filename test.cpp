#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

void print_var_info(const fb_var_screeninfo& fb_vinfo)
{
    printf("fb_var_screeninfo:\n");
    printf("xres: %d\n", fb_vinfo.xres);			/* visible resolution		*/
    printf("yres: %d\n", fb_vinfo.yres);
    printf("xres virtual: %d\n", fb_vinfo.xres_virtual);		/* virtual resolution		*/
    printf("yres virtual: %d\n", fb_vinfo.yres_virtual);
    printf("xoffset: %d\n", fb_vinfo.xoffset);			/* offset from virtual to visible */
    printf("yoffset: %d\n", fb_vinfo.yoffset);			/* resolution			*/

    printf("bits per pixel: %d\n", fb_vinfo.bits_per_pixel);		/* guess what			*/
    printf(": %d\n", fb_vinfo.grayscale);		/* 0 = color, 1 = grayscale,	*/
                    /* >1 = FOURCC			*/
    printf("red: %d, %d, %d\n", fb_vinfo.red.offset, fb_vinfo.red.length, fb_vinfo.red.msb_right);		/* bitfield in fb mem if true color, */
//    printf(": %d\n", fb_vinfo.green);	/* else only length is significant */
//    printf(": %d\n", fb_vinfo.blue);
//    printf(": %d\n", fb_vinfo.transp);	/* transparency			*/

    printf("nonstd: %d\n", fb_vinfo.nonstd);			/* != 0 Non standard pixel format */

    printf("activate: %d\n", fb_vinfo.activate);			/* see FB_ACTIVATE_*		*/

    printf("height: %d\n", fb_vinfo.height);			/* height of picture in mm    */
    printf("width: %d\n", fb_vinfo.width);			/* width of picture in mm     */

    printf("accel_flags: %d\n", fb_vinfo.accel_flags);		/* (OBSOLETE) see fb_info.flags */

    /* Timing: All values in pixclocks, except pixclock (of course) */
    printf("pixclock: %d\n", fb_vinfo.pixclock);			/* pixel clock in ps (pico seconds) */
    printf(": %d\n", fb_vinfo.left_margin);		/* time from sync to picture	*/
    printf(": %d\n", fb_vinfo.right_margin);		/* time from picture to sync	*/
    printf(": %d\n", fb_vinfo.upper_margin);		/* time from sync to picture	*/
    printf(": %d\n", fb_vinfo.lower_margin);
    printf("hsync_len: %d\n", fb_vinfo.hsync_len);		/* length of horizontal sync	*/
    printf("vsync_len: %d\n", fb_vinfo.vsync_len);		/* length of vertical sync	*/
    printf("sync: %d\n", fb_vinfo.sync);			/* see FB_SYNC_*		*/
    printf("vmode: %d\n", fb_vinfo.vmode);			/* see FB_VMODE_*		*/
    printf("rotate: %d\n", fb_vinfo.rotate);			/* angle we rotate counter clockwise */
    printf("colorspace: %d\n", fb_vinfo.colorspace);		/* colorspace for FOURCC-based modes */
}

int main(int argc, char **argv)
{
    const char* fb_device = nullptr;
    if (argc > 1)
        fb_device = argv[1];

    fb_fix_screeninfo fb_fixinfo;
    fb_var_screeninfo fb_vinfo;

    if (!fb_device)
        fb_device = "/dev/fb0";

    int fbfd = open(fb_device, O_RDWR);

    if (fbfd < 0) {
        perror("open failed");
        return -1;
    }

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fb_fixinfo) != 0) {
        perror("FBIOGET_FSCREENINFO failed");
        return -2;
    }

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fb_vinfo) != 0) {
        perror("FBIOGET_VSCREENINFO failed");
        return -3;
    }

    if (fb_fixinfo.visual != FB_VISUAL_TRUECOLOR) {
        printf("not true color");
        return -4;
    }

    const auto fb_size = fb_fixinfo.line_length * fb_vinfo.yres;
    uint8_t* buffer = (uint8_t*) mmap(NULL, fb_size,
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED, fbfd, 0);
    if (buffer == MAP_FAILED)
    {
        perror("mmap failed");
        return -5;
    }

    printf("fb_fix_screeninfo:\n");

    printf("id: %s\n", fb_fixinfo.id);			/* identification string eg "TT Builtin" */
    printf("smem_start: %lx\n", fb_fixinfo.smem_start);	/* Start of frame buffer mem */
                    /* (physical address) */
    printf("smem_len: %d\n", fb_fixinfo.smem_len);			/* Length of frame buffer mem */
    printf("type: %d\n", fb_fixinfo.type);			/* see FB_TYPE_*		*/
    printf("type_aux: %d\n", fb_fixinfo.type_aux);			/* Interleave for interleaved Planes */
    printf("visual: %d\n", fb_fixinfo.visual);			/* see FB_VISUAL_*		*/
    printf("xpanstep: %d\n", fb_fixinfo.xpanstep);			/* zero if no hardware panning  */
    printf("ypanstep: %d\n", fb_fixinfo.ypanstep);			/* zero if no hardware panning  */
    printf("ywrapstep: %d\n", fb_fixinfo.ywrapstep);		/* zero if no hardware ywrap    */
    printf("line_length: %d\n", fb_fixinfo.line_length);		/* length of a line in bytes    */
    printf("mmio_start: %ld\n", fb_fixinfo.mmio_start);	/* Start of Memory Mapped I/O   */
                    /* (physical address) */
    printf("mmio_len: %d\n", fb_fixinfo.mmio_len);			/* Length of Memory Mapped I/O  */
    printf("accel: %d\n", fb_fixinfo.accel);			/* Indicate to driver which	*/
                    /*  specific chip/card we have	*/
    printf("capabilities: %d\n", fb_fixinfo.capabilities);		/* see FB_CAP_*			*/

    print_var_info(fb_vinfo);

    printf("doubling yres_virtual\n");
    fb_vinfo.yres_virtual *= 2;
    fb_vinfo.activate = FB_ACTIVATE_FORCE;
    print_var_info(fb_vinfo);
    if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &fb_vinfo) != 0) {
        perror("FBIOPUT_VSCREENINFO failed");
        return -99;
    }

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fb_vinfo) != 0) {
        perror("FBIOGET_VSCREENINFO failed");
        return -3;
    }
    print_var_info(fb_vinfo);

    return 0;
}
