#include "jpegimage.h"

JpegImage::JpegImage()
{
}

/*------------------------------------------------*/
/* TJpgDec Quick Evaluation Program for PCs       */
/*------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "tjpgd.h"


/* User defined device identifier */
typedef struct {
    FILE *fp;      /* File pointer for input function */
    BYTE *fbuf;    /* Pointer to the frame buffer for output function */
    UINT wfbuf;    /* Width of the frame buffer [pix] */
} IODEV;

/*------------------------------*/
/* User defined input funciton  */
/*------------------------------*/

UINT in_func (JDEC* jd, BYTE* buff, UINT nbyte)
{
    IODEV *dev = (IODEV*)jd->device;   /* Device identifier for the session (5th argument of jd_prepare function) */


    if (buff) {
        /* Read bytes from input stream */
        return (UINT)fread(buff, 1, nbyte, dev->fp);
    } else {
        /* Remove bytes from input stream */
        return fseek(dev->fp, nbyte, SEEK_CUR) ? 0 : nbyte;
    }
}


/*------------------------------*/
/* User defined output funciton */
/*------------------------------*/

UINT out_func (JDEC* jd, void* bitmap, JRECT* rect)
{
    IODEV *dev = (IODEV*)jd->device;
    BYTE *src, *dst;
    UINT y, bws, bwd;


    /* Put progress indicator */
    if (rect->left == 0) {
        printf("\r%lu%%", (rect->top << jd->scale) * 100UL / jd->height);
    }

    /* Copy the decompressed RGB rectanglar to the frame buffer (assuming RGB888 cfg) */
    src = (BYTE*)bitmap;
    dst = dev->fbuf + 3 * (rect->top * dev->wfbuf + rect->left);  /* Left-top of destination rectangular */
    bws = 3 * (rect->right - rect->left + 1);     /* Width of source rectangular [byte] */
    bwd = 3 * dev->wfbuf;                         /* Width of frame buffer [byte] */
    for (y = rect->top; y <= rect->bottom; y++) {
        memcpy(dst, src, bws);   /* Copy a line */
        src += bws; dst += bwd;  /* Next line */
    }

    return 1;    /* Continue to decompress */
}


/*------------------------------*/
/* Program Main                 */
/*------------------------------*/

int main (int argc, char* argv[])
{
    void *work;       /* Pointer to the decompressor work area */
    JDEC jdec;        /* Decompression object */
    JRESULT res;      /* Result code of TJpgDec API */
    IODEV devid;      /* User defined device identifier */

    /* Open a JPEG file */
    if (argc < 2) return -1;
    devid.fp = fopen(argv[1], "rb");
    if (!devid.fp) return -1;

    /* Allocate a work area for TJpgDec */
    work = malloc(3100);

    /* Prepare to decompress */
    res = jd_prepare(&jdec, in_func, work, 3100, &devid);
    if (res == JDR_OK) {
        /* Ready to dcompress. Image info is available here. */
        printf("Image dimensions: %u by %u. %u bytes used.\n", jdec.width, jdec.height, 3100 - jdec.sz_pool);

        devid.fbuf = malloc(3 * jdec.width * jdec.height); /* Frame buffer for output image (assuming RGB888 cfg) */
        devid.wfbuf = jdec.width;

        res = jd_decomp(&jdec, out_func, 0);   /* Start to decompress with 1/1 scaling */
        if (res == JDR_OK) {
            /* Decompression succeeded. You have the decompressed image in the frame buffer here. */
            printf("\rOK  \n");

        } else {
            printf("Failed to decompress: rc=%d\n", res);
        }

        free(devid.fbuf);    /* Discard frame buffer */

    } else {
        printf("Failed to prepare: rc=%d\n", res);
    }

    free(work);             /* Discard work area */

    fclose(devid.fp);       /* Close the JPEG file */

    return res;
}
