#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "crc.h"
#include "zutil.h"
#include "lab_png.h"
#include <limits.h>
#include <dirent.h>
#include <stdbool.h>

// Functions for pnginfo:
int get_png_height(FILE *fp);
int get_png_width(FILE *fp);
bool check_crc(char *filepath);
bool ispng(FILE *fp);
void pnginfo(char *filepath);
int convertBufToHostInt(U8* buf);


int convertBufToHostInt(U8* buf) {
    u_int32_t value;
    memcpy(&value, buf, sizeof(u_int32_t));
    return ntohl(value);
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

void pnginfo(char *filepath){

    FILE *fp = fopen(filepath, "rb");

    if(fp == NULL){
        perror("fopen");
        return;
    }
    
    if (ispng(fp)==false){
        printf("%s: Not a PNG file\n", filepath);
        fclose(fp);
        return;
    }
    if(check_crc(filepath) == false){
        return;
    }
    printf("%s: %d x %d\n", filepath, get_png_width(fp), get_png_height(fp));



    fclose(fp);
    return;
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


int main (int argc, char **argv){
    if (argc < 2) {
        printf("Wrong number of arguments: given %d, expected 1\n", argc - 1);
        return -1;
    }

    pnginfo(argv[1]);

    return 0;

}

