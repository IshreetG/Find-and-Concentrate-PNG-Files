#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "crc.h"
#include "zutil.h"
#include "lab_png.h"

#include <limits.h>
#include <dirent.h>

#define BUF_LEN  (256*16)
#define BUF_LEN2 (256*32)

int find_png(char *dir_path); 

//  Functions for pnginfo:
bool ispng(FILE *fp); 
int get_png_height(FILE *fp);
int get_png_width(FILE *fp);
void pnginfo(char *filepath);
bool check_crc(char *filepath);

// Functions for catpng:
void catpng(char** filePaths, int n);
int get_width_height_from_IHDR(char *filepath, int* width);
U8* get_IDAT_Data(char* filepath, int length);
int get_IDAT_length(char* filepath);

// Helper Functions
void print_buf(U8* buf, int size);
int convertBufToHostInt(U8* buf);

int main(){

    int width = 0;

    // /Array of strings (size of 3, max size of 20 char)
    char *filePaths[] = {
   //"images/WEEF_1.png",
   "images/pic_cropped_0.png",
  // "images/pic_cropped_1.png",
   "images/pic_cropped_2.png",
   "images/pic_cropped_3.png",
   "images/pic_cropped_4.png",
    //"images/uweng.png"
    };

    int n = sizeof(filePaths) / sizeof(filePaths[1]);


    // Tester function that loops through each array element
    // for(int i = 0; i < sizeof(filePaths)/20; i++)
    // {
    // pnginfo(filePaths[i]);
    // }

    // find_png("../starter/images");


    // int height = get_width_height_from_IHDR("images/WEEF_1.png", &width);
    // printf("Height %d\n", height);
    // printf("width %d\n", width);

    // catpng(filePaths, n);
    pnginfo("images/red-green-16x16-corrupted.png");

    return 0;
}

bool check_crc(char *filepath){

    // crc is backwards after generating (make sure both crcs are in big endian)

    FILE *fp = fopen(filepath, "rb");
    U8* IHDR_data = malloc(4+13); 
    U8* IHDR_crc = malloc(4); 

    // check crc of IHDR

    // Get IHDR data & type
    fseek(fp, 8+4, SEEK_SET); // skip to type
    fread(IHDR_data, 4+13, 1, fp);  // get IHDR data & type
    fread(IHDR_crc, 4, 1, fp);  // get IHDR crc

    int IHDR_crc_generated = crc(IHDR_data, 4+13);
    int IHDR_crc_int = convertBufToHostInt(IHDR_crc);

    free(IHDR_data);
    free(IHDR_crc);

    if (IHDR_crc_generated != IHDR_crc_int){
        printf("IHDR chunk CRC error: computed %X, expected %X\n", IHDR_crc_generated, IHDR_crc_int);
        fclose(fp);
        return false;
    }

    // check crc of IDAT

    U8* IDAT_data_length = malloc(4);
    fread(IDAT_data_length, 4, 1, fp); // read IDAT length
    int IDAT_data_length_int = convertBufToHostInt(IDAT_data_length); 

    U8* IDAT_data = malloc(IDAT_data_length_int+4); 
    U8* IDAT_crc = malloc(4); 

    fread(IDAT_data, IDAT_data_length_int+4, 1, fp); // read IDAT data & type
    fread(IDAT_crc, 4, 1, fp); // Read IDAT crc

    int IDAT_crc_generated = crc(IDAT_data, IDAT_data_length_int+4);
    int IDAT_crc_int = convertBufToHostInt(IDAT_crc);

    free(IDAT_data_length);
    free(IDAT_data);
    free(IDAT_crc);

    if (IDAT_crc_generated != IDAT_crc_int){
        printf("IDAT chunk CRC error: computed %X, expected %X\n", IDAT_crc_generated, IDAT_crc_int);
        fclose(fp);
        return false;
    }

    // check crc of IEND

    U8* IEND_type = malloc(4); 
    U8* IEND_crc = malloc(4); 

    fseek(fp, 4, SEEK_CUR); // skip length
    fread(IEND_type, 4, 1, fp); // Read IEND type
    fread(IEND_crc, 4, 1, fp); // Read IEND crc

    int IEND_crc_generated = crc(IEND_type, 4);
    int IEND_crc_int = convertBufToHostInt(IEND_crc);
    
    free(IEND_type);
    free(IEND_crc);

    if (IEND_crc_generated != IEND_crc_int){
        printf("IEND chunk CRC error: computed %X, expected %X\n", IEND_crc_generated, IEND_crc_int);
        fclose(fp);
        return false;
    }

    fclose(fp);

    return true;
}

 // Covnerts to big endian and convert to int
int convertBufToHostInt(U8* buf) {
    u_int32_t value;
    memcpy(&value, buf, sizeof(u_int32_t));
    return ntohl(value);
}


int get_width_height_from_IHDR(char* filepath, int* width){

    FILE *fp = fopen(filepath, "rb");
    U8* buf = malloc(4); // first 4 bits is width, second 4 bits is height

    // get height & width
    fseek(fp, 8+4+4, SEEK_SET); // skip to width
    fread(buf, 4, 1, fp); // width

    *width = convertBufToHostInt(buf);

    fread(buf, 4, 1, fp); //height

    u_int32_t height = convertBufToHostInt(buf);

    fclose(fp);
    free(buf);

    return height;
}

U8* get_IDAT_Data(char* filepath, int length){

    FILE *fp = fopen(filepath, "rb");
    U8* dataBuf = malloc(length);

    fseek(fp, 8+4+4+13+4+4+4, SEEK_SET); // skip to data
    fread(dataBuf, length, 1, fp); // read data
    fclose(fp);

    return dataBuf;
}

int get_IDAT_length(char* filepath){

    FILE *fp = fopen(filepath, "rb");
    U8* buf = malloc(4); 
    int length = 0;

    // get data length first
    fseek(fp, 8+4+4+13+4, SEEK_SET); // skip to data length 8+4+4
    fread(buf, 4, 1, fp); // data length
    length = convertBufToHostInt(buf);

    fclose(fp);
    free(buf);

    return length;
}

void catpng(char** filePaths, int n){

    if ((n == 0) || (filePaths[0] == NULL)){
        printf("Error filePaths is empty\n");
        return;
    }

    printf("n is %d\n", n);

    int heightSum = 0;
    int width = 0;
    int* heights = malloc(n*sizeof(int));

    // Get the height & Width

    for(int i = 0; i < n; i++){
        heights[i] = get_width_height_from_IHDR(filePaths[i], &width);
        heightSum += heights[i];
        printf("heights: %d\n", heights[i]);
        printf("width: %d\n", width);
    }

      printf("heightSum: %d\n", heightSum);

    // Get the IDAT data & uncompress
    int length = 0;
    int totalLength = 0;
    int ret = 0;
    U64 len_inf = 0;      /* uncompressed data length                      */
    U8* bufTotal = NULL;
    size_t oldLength = 0;
    int length1 = 0;

    int number_of_channels = 4; // RGBA

      for(int i = 0; i < n; i++){
        length = get_IDAT_length(filePaths[i]);
        totalLength += length;

        if (i == 0){
            length1 = get_IDAT_length(filePaths[i]);
        }

        printf("length %d\n", length);

        U8* buf = get_IDAT_Data(filePaths[i], length);

        // Need to use uncompressed_size to initlize gp_buf_inf
        int uncompressed_size = heights[i] * (width * number_of_channels + 1);
        U8* gp_buf_inf = malloc(uncompressed_size * sizeof(U8));

        printf("uncompressed_size %d\n", uncompressed_size);

        // Uncompress buf using mem_inf:
        ret = mem_inf(gp_buf_inf, &len_inf, buf, length);
        if (ret == 0) { /* success */
            printf("original len = %d, length = %lu, len_inf = %lu\n", \
                uncompressed_size, length, len_inf);
        } else { /* failure */
            fprintf(stderr,"mem_inf failed. ret = %d., len_inf = %d\n", ret, len_inf);
        }
        free(buf);

        printf("TEST\n");

        // Create new buffer with length+oldLength and then copy contents

        size_t newLength = len_inf + oldLength;
        U8* newBuf = malloc(newLength);

        // Copy over old data to the new buffer
        if (oldLength != 0) {
        memcpy(newBuf, bufTotal, oldLength);
        free(bufTotal);
        }

        // Copy uncompressed data to bufTotal
        memcpy(newBuf + oldLength, gp_buf_inf, len_inf);
        free(gp_buf_inf);
        if (ret != 0) {
            free(gp_buf_inf);
            free(newBuf);
            return;
        } else {
            bufTotal = newBuf;
        }

        oldLength = newLength;
    }

    // Recompress data in bufTotal
    U64 len_def = 0;  
    U8 gp_buf_def[totalLength];
    printf("totalLength %d\n", totalLength);

    ret = mem_def(gp_buf_def, &len_def, bufTotal, oldLength, Z_DEFAULT_COMPRESSION);
    if (ret == 0) { /* success */
        printf("original len = %d, len_def = %lu\n", oldLength, len_def);
    } else { /* failure */
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
        return ret;
    }

    // Create all.png with compressed data

    FILE *fp = fopen("all.png", "wb");

    u_int32_t widthNetworkOrder = htonl(width);

    // Copy over everything until IDAT chunk from first image

    FILE *fp2 = fopen(filePaths[0], "rb");
    int untilHeight = 8+4+4+4;
    U8* buf1 = malloc(untilHeight); // everything until Height
    fread(buf1, untilHeight, 1, fp2);
    fwrite(buf1 , 1, untilHeight, fp );
    free(buf1);

    u_int32_t heightSumNetworkOrder = htonl(heightSum);
    fwrite(&heightSumNetworkOrder, 1, 4, fp);

    U8* buf2 = malloc(5); // allocate for the rest of Data in IHDR
    fseek(fp2, 4, SEEK_CUR); // skip height
    fread(buf2, 5, 1, fp2);
    fwrite(buf2 , 1, 5, fp );

    // Calc crc IHDR
    U8 IHDRdata[17]; 
    memcpy(IHDRdata, "IHDR", 4); 
    memcpy(IHDRdata + 4, &widthNetworkOrder, 4);
    memcpy(IHDRdata + 8, &heightSumNetworkOrder, 4); 
    memcpy(IHDRdata + 12, buf2, 5); 
    free(buf2);

    u_int32_t IHDRcrc = htonl(crc(IHDRdata, 17));
    fwrite(&IHDRcrc, 1, 4, fp);

    fclose(fp2);

    // Need to calculate crc for IDAT for all.png

    char chunkType[4] = { 'I', 'D', 'A', 'T' };
    // Need to add type to IDAT Data:
    U8* IDATDataWithType = malloc(4 + totalLength);

    memcpy(IDATDataWithType, chunkType, 4);
    memcpy(IDATDataWithType + 4, gp_buf_def, totalLength);

    int crcAllPng = htonl(crc(IDATDataWithType, 4 + totalLength));
    free(IDATDataWithType);

    // IDAT

    u_int32_t totalLengthHost = htonl(totalLength);

    fwrite(&totalLengthHost, 1, 4, fp ); // length
    fwrite(chunkType, 1, 4, fp ); // Type
    fwrite(gp_buf_def, 1, totalLength, fp ); // write compressed data to all.png
    fwrite(&crcAllPng, 1, 4, fp ); // crc

    // IEND

    FILE *fp3 = fopen(filePaths[0], "rb");
    int IENDChunk = 8+4+4+13+4+4+4+length1+4;
    U8* buf3 = malloc(IENDChunk); 
    fseek(fp3, IENDChunk, SEEK_SET); // skip to IEND chunk
    fread(buf3, 4+4+0+4, 1, fp3); // read IEND chunk
    fwrite(buf3 , 1, 4+4+0+4, fp ); // write it to all.png
    free(buf3);
    fclose(fp3);

    printf("Done!\n");

    // Free memory:
    fclose(fp);
    free(bufTotal);
    free(heights);

}


void print_buf(U8* buf, int size){
    printf("Buffer: ");
    for(int i = 0; i < size; i++){
        printf("%X", buf[i]);
    }
    printf("\n");
}

int get_png_height(FILE *fp){
    U8* buf = malloc(4);
    int height = 0;

    fseek(fp, 20, SEEK_SET); // Skip 20 byte to get to IHDR Data
    fread(buf, 4, 1, fp); // Read the height bits

    height = convertBufToHostInt(buf);
    free(buf);
    return height;
}

int get_png_width(FILE *fp){
    U8* buf = malloc(4);
    int width = 0;

    fseek(fp, 16, SEEK_SET); // Skip 16 byte to get to IHDR Data
    fread(buf, 4, 1, fp); // Read the width bits

    width = convertBufToHostInt(buf);
    free(buf);
    return width;
}

bool ispng(FILE *fp){
    U8 *buf = malloc(8); /*here we are dynaminally allocating the memory to use since we are the 8 bytes from the header*/
    fseek(fp, 0, SEEK_SET); // Go to start of file

    fread(buf, sizeof(buf), 1, fp);

    if(buf[0] == 0x89 && buf[1] == 0x50 && buf[2] == 0x4e && buf[3] == 0x47 && buf[4] == 0x0d && buf[5] == 0x0a && buf[6] == 0x1a && buf[7] == 0x0a ) {
        //this means that this is a png file so we will return 1
        free(buf);
        return true;
    }   
    free(buf);
    return false;
}

int find_png(char *dirpath){
    struct dirent *entry;
    DIR *dp;
    int count;
    char path[PATH_MAX+1]; //max number 
    FILE *f;


    if(!(dp = opendir(dirpath))){
        perror("opendir");
        return errno;
    }

    while((entry = readdir(dp)) != NULL){ //as long as the directory is not empty
     
        strcpy(path, dirpath); //here we are copying dirpath into path;
        strcat(path, "/"); //concating the path with / so that in will be path/
        strcat(path, entry->d_name); //now it will be path/name

        if(entry->d_type == DT_DIR){ //here we are checking if the type of file is in a directory 
          
            if(strcmp(entry->d_name, ".") != 0 &&  strcmp(entry->d_name, "..") != 0){ //if the name is not . and ..
                find_png(path); //repeat with updated directory path
            }

        }
        else if(entry->d_type == DT_REG){ //here we are checking if this is a regular file like weef.png 
            f = fopen(path, "rb");
            if (ispng(f) == true){
                    count++;
                    printf("%s\n", path);
                }
            fclose(f);

        }
    }

    closedir(dp);
    if(count == 0){ //this means that no png files are found
        printf("findpng: No PNG file found\n");
    }
    return 0;


}