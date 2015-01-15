#include "jpegimage.h"

/*------------------------------------------------*/
/* TJpgDec Quick Evaluation Program for PCs       */
/*------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "tjpgdec/tjpgd.h"
#include <qDebug>

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

JpegImage::JpegImage() {
}
int JpegImage::open(const char* filename)
{
    void *work;       /* Pointer to the decompressor work area */
    JDEC jdec;        /* Decompression object */
    JRESULT res;      /* Result code of TJpgDec API */
    IODEV devid;      /* User defined device identifier */

    qDebug("Open file:%s",filename);
    /* Open a JPEG file */
    devid.fp = fopen(filename, "rb");
    if (!devid.fp) {
        qDebug("Fail to open file:%s",filename);
        return -1;
    }

    /* Allocate a work area for TJpgDec */
    work = malloc(3100);

    /* Prepare to decompress */
    res = jd_prepare(&jdec, in_func, work, 3100, &devid);
    if (res == JDR_OK) {
        /* Ready to dcompress. Image info is available here. */
        qDebug("Image dimensions: %u by %u. %u bytes used.\n", jdec.width, jdec.height, 3100 - jdec.sz_pool);

        devid.fbuf = (BYTE *)malloc(3 * jdec.width * jdec.height); /* Frame buffer for output image (assuming RGB888 cfg) */
        devid.wfbuf = jdec.width;

        res = jd_decomp(&jdec, out_func, 0);   /* Start to decompress with 1/1 scaling */
        if (res == JDR_OK) {
            /* Decompression succeeded. You have the decompressed image in the frame buffer here. */
            qDebug("\rOK  \n");

        } else {
            qDebug("Failed to decompress: rc=%d\n", res);
        }

        free(devid.fbuf);    /* Discard frame buffer */

    } else {
        qDebug("Failed to prepare: rc=%d\n", res);
    }

    free(work);             /* Discard work area */

    fclose(devid.fp);       /* Close the JPEG file */

    return res;
}
