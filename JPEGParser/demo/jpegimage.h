#ifndef JPEGIMAGE_H
#define JPEGIMAGE_H

class JpegImage
{
public:
    JpegImage();
    int open(const char* filename);
    int getWidth(){return width;}
    int getHeight(){return height;}
private:
    int width;
    int height;
};

#endif // JPEGIMAGE_H
