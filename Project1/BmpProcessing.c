#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#pragma pack(1)    //!!!改变结构体的对齐方式，防止读取错位
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffsets;
}BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t Planners;
    uint16_t biBitCount;     //decide whether use the palette or not
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;    //2^bitcount
    uint32_t biClrImportant;
}BITMAPINFORMATIONHEADER;

///////////////////////////

void changeRGBtoYUV(uint8_t r, uint8_t g, uint8_t b, uint8_t* y, char * u, char * v)   //利用指针来改变具体数值
{
    *y = (uint8_t)(0.3 * r + 0.59 * g + 0.11 * b);
    if(0.3 * r + 0.59 * g + 0.11 * b > 255) *y = 255;       //越界判断，目的是减少噪点
    else if(0.3 * r + 0.59 * g + 0.11 * b < 0) *y = 0;

    *u = (char)(0.493 * (b - *y));
    if(0.493*(b - *y) > 127) *u = 127;
    else if (0.493 * (b - *y) < -128) *u = -128;

    *v = (char)(0.877 * (r - *y));
    if(0.877 * (r - *y) > 127) *v = 127;
    else if(0.877 * (r - *y) < -128) *v = -128;
}

void changeYUVtoRGB(uint8_t y, char u, char v, uint8_t* r, uint8_t* g, uint8_t* b)           
{
    *r = (uint8_t)(y + (double)v / 0.877);
    if(y + (double)v / 0.877 > 255) *r = 255;
    else if(y + (double)v / 0.877 < 0) *r = 0;

    *b = (uint8_t)(y + (double)u / 0.493);
    if(y + (double)u / 0.493 > 255) *b = 255;  //
    else if(y + (double)u / 0.493 < 0) *b = 0;

    *g = (uint8_t)(y - 0.3 * *r - 0.11 * *b)/0.59;
    if(y - 0.3 * *r - 0.11 * *b > 255) *g = 255;
    else if(y - 0.3 * *r - 0.11 * *b < 0) *g = 0;
}

void changeRGBtoGray(unsigned char* rgb_data, unsigned char* gray_data, int width, int height, int bytesCount)
{
    uint8_t r, g, b;
    uint8_t y;
    char u,v;    //不能写*y  因为如果先定义指针再对指针进行*y赋值会出现野指针  而应该先定义y再用&y取地址
    for(int i = 0; i < width * height; i++)
    {
        b = rgb_data[i * bytesCount + 0];    //注意，rgb在bmp图像中的存储顺序其实是bgr
        g = rgb_data[i * bytesCount + 1];
        r = rgb_data[i * bytesCount + 2];
        changeRGBtoYUV(r, g, b, &y, &u, &v);
        gray_data[i] = y;
    }
}

void changeGraytoRGB(unsigned char* rgb_data, unsigned char* gray_data, int width, int height, int bytesCount)
{
    uint8_t r, g, b;
    uint8_t y;
    char u,v;
    for(int i = 0; i < width * height; i++)
    {
        b = rgb_data[i * bytesCount + 0];
        g = rgb_data[i * bytesCount + 1];
        r = rgb_data[i * bytesCount + 2];
        changeRGBtoYUV(r, g, b, &y, &u, &v);
        
        //change luminance
        y = y / 2 ;   

        changeYUVtoRGB(y, u, v, &r, &g, &b);
        rgb_data[i * bytesCount + 2] = r;               //r
        rgb_data[i * bytesCount + 1] = g;
        rgb_data[i * bytesCount + 0] = b; 
    }
    
}


int findMax(unsigned char* gray_data, int width, int height)
    {
        int max=0;
        for(int i=0; i < width * height; i++)
        {
            if(gray_data[i] > max)max = gray_data[i];
        }
        return max;
    }
    
int findMin(unsigned char* gray_data, int width, int height)
    {
        int min=255;
        for(int i=0; i < width * height; i++)
        {
            if(gray_data[i] < min)min = gray_data[i];
        }
        return min;
    }

void increaseContrast(unsigned char* gray_data, int width, int height)  //增加对比度
{
    int max,min;
    max = findMax(gray_data, width, height);
    min = findMin(gray_data, width, height);
    int delta = max - min;
    for(int i = 0; i < width * height; i++)
    {
        gray_data[i] = (gray_data[i] - min) * 255/delta;
    }
}

//把要进行输出的数组与padding相结合
void gray_dataOut(unsigned char* out_data, unsigned char* gray_data, int width, int height, int bytesCount, int padding )
{
    for(int i = 0; i< (width * bytesCount + padding) * height; i++)out_data[i] = 0;
    for(int i = 0; i< height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            out_data[3 * j + i * (width * bytesCount + padding)] = gray_data[j + i * width];
            out_data[3 * j + i * (width * bytesCount + padding) + 1] = gray_data[j + i * width];
            out_data[3 * j + i * (width * bytesCount + padding) + 2] = gray_data[j + i * width];
            //rgb=yyy
        }
        
    }   
}

void rgb_dataOut(unsigned char* out_data, unsigned char* gray_data, int width, int height, int bytesCount, int padding )
{
    for(int i = 0; i< (width * bytesCount + padding) * height; i++)out_data[i] = 0;
    
    for(int i = 0; i< height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            out_data[3 * j + i * (width * bytesCount + padding)] = gray_data[j * 3 + i * width * 3];
            out_data[3 * j + i * (width * bytesCount + padding) + 1] = gray_data[j * 3 + i * width * 3 + 1];
            out_data[3 * j + i * (width * bytesCount + padding) + 2] = gray_data[j * 3 + i * width * 3+ 2];
            //rgb=yyy
        }
        
    }   
}

void writeBMP(const char* file_name, unsigned char* out_data, int width, int height, int bytesCount, int padding, BITMAPFILEHEADER* bitmapfileheader, BITMAPINFORMATIONHEADER* bitmapinformationheader)
{
    FILE* fp;
    fp = fopen(file_name, "wb");
    if(fp == NULL){
        printf("write gray image error!");
        return ;
    }
    
    fwrite(bitmapfileheader, sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(bitmapinformationheader, sizeof(BITMAPINFORMATIONHEADER), 1, fp);
    fwrite(out_data, (width * bytesCount + padding) * height, 1, fp);  
    fclose(fp);

}

void printBitmapFileHeader(BITMAPFILEHEADER fileHeader) {
    printf("bfType: %02x\n", fileHeader.bfType);
    printf("bfSize: %d\n", fileHeader.bfSize);
    printf("bfReserved1: %hu\n", fileHeader.bfReserved1);
    printf("bfReserved2: %hu\n", fileHeader.bfReserved2);
    printf("bfOffsets: %u\n", fileHeader.bfOffsets);
}

void printBitmapInfoHeader(BITMAPINFORMATIONHEADER infoHeader) {
    printf("biSize: %02x\n", infoHeader.biSize);
    printf("biWidth: %d\n", infoHeader.biWidth);
    printf("biHeight: %d\n", infoHeader.biHeight);
    printf("Planners: %hu\n", infoHeader.Planners);
    printf("biBitCount: %hu\n", infoHeader.biBitCount);
    printf("biCompression: %u\n", infoHeader.biCompression);
    printf("biSizeImage: %d\n", infoHeader.biSizeImage);
    printf("biXPelsPerMeter: %u\n", infoHeader.biXPelsPerMeter);
    printf("biYPelsPerMeter: %u\n", infoHeader.biYPelsPerMeter);
    printf("biClrUsed: %u\n", infoHeader.biClrUsed);
    printf("biClrImportant: %u\n", infoHeader.biClrImportant);
}

void printData(unsigned char* rgb_data, int width, int height, int bytesCount)
{
    
    for(int i=0; i<1425; i++)
    {
        printf("%02x ", rgb_data[i]);
    }
}

int main()
{
    const char* source_file = "mouse.bmp";
    const char* gray_test = "gray_test.bmp";
    const char* modifiedRGB_test = "modifiedRGB_test.bmp";
       
    
    FILE* fp;
    fp = fopen(source_file,"rb");      //open the source file in binary form
    if(fp == NULL) {
        printf("Error!");
        exit(0);
    }
    
    //
    BITMAPFILEHEADER bitmapfileheader;
    BITMAPINFORMATIONHEADER bitmapinformationheader;  //define the head of bmp
    
    //
    fread(&bitmapfileheader, sizeof(BITMAPFILEHEADER), 1, fp);
    fread(&bitmapinformationheader, sizeof(BITMAPINFORMATIONHEADER), 1, fp);  //get the informatin into head struct
    
    //check
    if(bitmapfileheader.bfType == 0X4D42)  printf("correct file type!\n");
    else {
        fclose(fp);
        exit(0);
    }
    
    //
    int width = bitmapinformationheader.biWidth;
    int height = bitmapinformationheader.biHeight;

    //
    if(bitmapinformationheader.biSizeImage == 0)bitmapinformationheader.biSizeImage = bitmapinformationheader.biSize - bitmapfileheader.bfOffsets;
    int dataSize = bitmapinformationheader.biSizeImage;

    //
    int bytesCount = bitmapinformationheader.biBitCount/8;
    int padding = (4-(width * bytesCount) % 4) % 4;   //to make the size of a line in the picture 4*k

    //create a space to store the actually useful data of the image, without the padding
    unsigned char* rgb_data = (unsigned char* ) malloc(width*height*bytesCount);  
    
/*
    // printf("width*height*bytesCount:%d\n",width*height*bytesCount);
    // printf("padding*height:%d\n",padding*height);
    // printf("height*(width*3+padding):%d\n",height*(width*3+padding));
    
    // why this sum does not equal biSizeImage??
*/   
    
    //get the actually useful data
    for(int i = 0; i < height; i++)
    {
        fread(&rgb_data[i * width * bytesCount], bytesCount, width, fp);   
        fseek(fp, padding, SEEK_CUR);  //jump the padding
    }

    fclose(fp);
 
    //  
    unsigned char* gray_data = (unsigned char*)malloc(width * height * bytesCount);   //gray_data stores the Y
    unsigned char* out_data = (unsigned char*)malloc((width * bytesCount + padding) * height);  //out_data contains the useful data and paddings
    
    //write gray BMP
    changeRGBtoGray(rgb_data, gray_data, width, height, bytesCount);  
    gray_dataOut(out_data, gray_data, width, height, bytesCount, padding);  //set the out_data
    
    //increase contrast
    increaseContrast(gray_data, width, height);
    writeBMP(gray_test ,out_data, width, height, bytesCount, padding, &bitmapfileheader, &bitmapinformationheader);
    
    //write rgb BMP
    changeGraytoRGB(rgb_data, gray_data, width, height, bytesCount);
    rgb_dataOut(out_data, rgb_data, width, height, bytesCount, padding);
    writeBMP(modifiedRGB_test, out_data, width, height, bytesCount, padding, &bitmapfileheader, &bitmapinformationheader);
    
    //test infomation
    printBitmapFileHeader(bitmapfileheader);
    printBitmapInfoHeader(bitmapinformationheader);
    // printData(rgb_data, width, height, bytesCount);

}

