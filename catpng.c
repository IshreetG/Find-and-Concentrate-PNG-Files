#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "crc.h"
#include "zutil.h"
#include "lab_png.h"
#include <limits.h>
#include <dirent.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define BUF_LEN  (256*16)
#define BUF_LEN2 (256*32)

/* DECLARATIONS*/
void catpng(char** filePaths, int n);
int get_width_height_from_IHDR(char *filepath, int* width);
U8* get_IDAT_Data(char* filepath, int length);
int get_IDAT_length(char* filepath); 
bool ispng(char* filepath);


bool ispng(char* filepath){

    FILE *fp = fopen(filepath, "rb");

    U8 *buf = malloc(8); /*here we are dynaminally allocating the memory to use since we are the 8 bytes from the header*/
    fseek(fp, 0, SEEK_SET); // Go to start of file

    fread(buf, sizeof(buf), 1, fp);

    if(buf[0] == 0x89 && buf[1] == 0x50 && buf[2] == 0x4e && buf[3] == 0x47 && buf[4] == 0x0d && buf[5] == 0x0a && buf[6] == 0x1a && buf[7] == 0x0a ) {
        //this means that this is a png file so we will return 1
        free(buf);
        fclose(fp);
        return true;
    }   
    free(buf);
    fclose(fp);

    return false;
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

/*CATPNG FUNCTION*/
 void catpng(char** filePaths, int n){

    if ((n == 0) || (filePaths[0] == NULL)){
            printf("Error filePaths is empty\n");
            return;
        }
        for(int i = 0; i < n; i++){ //check if all are pngs
            if(ispng(filePaths[i]) == false){
                printf("File is not a valid png\n");
                return;
            }
        }
        
        int heightSum = 0;
        int width = 0;
        int* heights = malloc(n*sizeof(int));

        // Get the height & Width

        for(int i = 0; i < n; i++){
            heights[i] = get_width_height_from_IHDR(filePaths[i], &width);
            heightSum += heights[i];
        }

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


            U8* buf = get_IDAT_Data(filePaths[i], length);

            // Need to use uncompressed_size to initlize gp_buf_inf
            int uncompressed_size = heights[i] * (width * number_of_channels + 1);
            U8* gp_buf_inf = malloc(uncompressed_size * sizeof(U8));

            // Uncompress buf using mem_inf:
            ret = mem_inf(gp_buf_inf, &len_inf, buf, length);
            if (ret == 0) { /* success */
               // printf("original len = %d, length = %lu, len_inf = %lu\n", \
                    uncompressed_size, length, len_inf);
            } else { /* failure */
               // fprintf(stderr,"mem_inf failed. ret = %d., len_inf = %d\n", ret, len_inf);
            }
            free(buf);

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
                free(heights);
                return;
            } else {
                bufTotal = newBuf;
            }

            oldLength = newLength;
        }

        // Recompress data in bufTotal
        U64 len_def = 0;  
        int bufferRoom = totalLength*0.2; // Extra wiggle room for totalLength (compression is inconsistent)
        totalLength = totalLength + bufferRoom;
        U8* gp_buf_def = malloc(totalLength);

        ret = mem_def(gp_buf_def, &len_def, bufTotal, oldLength, Z_DEFAULT_COMPRESSION);
        if (ret == 0) { /* success */
           // printf("original len = %d, len_def = %lu\n", oldLength, len_def);
        } else { /* failure */
           // fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
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

        // Free memory:
        fclose(fp);
        free(bufTotal);
        free(heights);
        free(gp_buf_def);

    }

    int main(int argc, char **argv) {
        int n = argc - 1; //number of files 

        catpng(argv + 1, n); // skip first argv argument (./catpng)

        return 0;
    }