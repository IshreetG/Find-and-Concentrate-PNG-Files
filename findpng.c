#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "crc.h"
#include "zutil.h"
#include "lab_png.h"
#include <limits.h>
#include <dirent.h>
#include <stdbool.h>

int find_png(char *dirpath); 
bool ispng(FILE *fp);

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
    int count = 0;
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


int main (int argc, char **argv){
    if (argc < 2) {
        printf("Wrong number of arguments: given %d, expected 1\n", argc - 1);
        return -1;
    }

    find_png(argv[1]);

    return 0;

}