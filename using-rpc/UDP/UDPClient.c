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
* This is a UDP Client for file download from the RPC File Server.

@USAGE:
    $ ./UDPClient server filename
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "file_server.h"

CLIENT *connectToFileServer(char *host);

int downloadRemoteFile(char *filename, CLIENT *handle);
int storeFile(char *fil_name, int file_size, char *data);

int main(int argc, char *argv[]){
    CLIENT *handle;
    char *host = NULL;
    char *file_name = NULL;

    if(argc != 3){ //exit if no command line arguments are missing.
        printf("Invalid Parameters.!\n");
        exit(EXIT_FAILURE);
    }

    host = argv[1];
    file_name = argv[2];

    if(access(file_name, F_OK) == 0){ //exit if a file with same name exists in current directory
        fprintf(stderr, "Download failed: %s already exists\n",file_name);
        exit(EXIT_FAILURE);
    }

    printf("connecting...\n");
    if((handle = connectToFileServer(host)) == NULL){ //establish a connection to the UDP file server
        clnt_pcreateerror("Could not connect to file server!\n");
        exit(EXIT_FAILURE);
    }

    printf("connected!\n"); 
    if(downloadRemoteFile(file_name, handle) < 0){ //download file from server
        fprintf(stderr,"Download failed!\n");
        clnt_destroy(handle);
        exit(EXIT_FAILURE);
    }

    printf("Download complete!\n");

    clnt_destroy(handle);
    exit(EXIT_SUCCESS);
}

CLIENT *connectToFileServer(char *host){
    /*Create a client handle with FILESERVER program on the host.*/

    CLIENT *handle;

    handle = clnt_create(host, FILESERVER, FILESERVERVERS, "udp"); //Create a UDP client handle
    if(handle == NULL){
        return NULL;
    }

    return handle;
}

int downloadRemoteFile(char *file_name, CLIENT *handle){
    /*Send initial download request, if server is ready to send the file, 
      start receiving file data over the socket*/

    int ret;
    char **result;
    int *file_size;

    file_size = get_file_size_1(file_name, handle);
    if(file_size == NULL || *file_size == 0){
        return -1;
    }

    printf("File Size: %d\n",*file_size);
    if((result = request_file_1(file_name, *file_size, handle)) == NULL){ //send download request
        fprintf(stderr, "Download failed!\n");
        clnt_perror(handle, "localhost");
        return -1;
    }
    
    if(*result == NULL){
        fprintf(stderr, "Download failed!\n");
        return -1;
    }

    ret = storeFile(file_name, *file_size, *result);
    return ret;
}

int storeFile(char *file_name, int file_size, char *data){
    /*Download the file data in a local file*/

    int n;
    FILE *fp;

    if((fp = fopen(file_name, "ab")) == NULL){ //try opening file in binary append mode
        fprintf(stderr,"create file failed!");
        return -1;
    }

    if((n = fwrite(data, 1, file_size, fp)) <= 0){ //Write the received data to the file.
        fprintf(stderr, "Writing data to file failed!\n");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}
