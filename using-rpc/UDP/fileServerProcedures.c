/*
 * San Jose State University
 * Department of Computer Engineering
 * 207 - Network Programming and Application
 * Lab Assignment-IV

 * Group 4
   * Akhilesh
   * Karthikeya Rao
   * Vishal Govindraddi Yarabandi
 
NOTES:
* These are the procedures implmenting File Server operations

@USAGE:
    $ ./FileServer
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>

#include "file_server.h"

int *get_file_size_1_svc(char *file_name, struct svc_req *req){
   FILE *fp;
   static int file_size;

   if((fp = fopen(file_name, "rb")) == NULL){ //try opening file in binary read mode.
        file_size = 0;
        return &file_size;
    }
    else{
        fseek(fp, 0L, SEEK_END);  //Calculate the file size
        file_size = ftell(fp);
    }

    fclose(fp);
    return &file_size;
}

char **request_file_1_svc(char *file_name, int file_size, struct svc_req *cln_req){
    FILE *fp = NULL;
    int n;
    static char *buf = NULL;

    if((fp = fopen(file_name, "rb")) == NULL){ //try opening file in binary read mode.
        return &buf;
    }

    buf = (char *)malloc(file_size);
    memset(buf, 0, file_size);

    if(n = fread(buf, 1, file_size, fp) <= 0){ //Read entire data from file into buf
        fprintf(stderr,"read failed");
        buf = NULL;
        return &buf;
    }

    fclose(fp);
    return &buf;
}
