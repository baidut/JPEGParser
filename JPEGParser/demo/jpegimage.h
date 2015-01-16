#ifndef JPEGIMAGE_H
#define JPEGIMAGE_H

#include <QList>

typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned int	u_int;

// printf("%d %d %d",sizeof(unsigned char),sizeof(unsigned short),sizeof(unsigned int)); //1 2 4
/*
// 方便定位位置 存储字节地址
// 标志所在段 无需存储两个
typedef int Addr;
//typedef struct{
//    int start;
//    int end;
//}Addr;

// 采用对齐的方式定义结构，解析用
typedef struct{
    Addr addr;
    u_char code; // 0XFF?? 0-255
}JpegMarker;

typedef struct{
    Addr addr;
    long value;
}JpegParm;

// Structurally, the compressed data formats consist of an ordered collection of
// parameters, markers, and entropy-coded data segments.

typedef struct{
    QList<Addr>  mcu;
}JpegECS;

typedef struct{
    Addr    Tables;
    Addr    ScanHeader;
    QList<JpegECS*>Ecs;  // entropy-coded segments.
    QList<Addr>    Rst;  // Restart
}JpegScan;

typedef struct{ // 注意成员对齐问题
    JpegMarker  SOFn;
    u_short    Lf;//PARM is short for parameter
    u_char     P;
    u_short    Y;
    u_char     X;
    JpegParm    Nf;
    // 数组

}JpegFrameHeader;

typedef struct{
    Addr    *Tables;
    JpegFrameHeader    *FrameHeader;
    QList<JpegScan*>    Scan;
}JpegFrame;

typedef struct{
    JpegMarker SOI;
    JpegFrame  *Frame;
    JpegMarker EOI;

    int getInfo(int addr){

    }
}JpegImage;
*/
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
