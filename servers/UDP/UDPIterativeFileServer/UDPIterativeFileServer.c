/*
 * San Jose State University
 * Department of Computer Engineering
 * 207 - Network Programming and Application
 * Lab Assignment-II
 * Question 2

 * Group 4
   * Akhilesh
   * Karthikeya Rao
   * Vishal Govindraddi Yarabandi
 
NOTES:
* This is a UDP Iterative File Server for only Download of a file by client.

@USAGE:
    $ ./UDPIterativeFileServer  [port]
*/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>

#define DFAULT_PORT    1234

int passiveUDP(int port);

int main(int argc, char *argv[]){
    int serverSock, len;
    char buf[256];
    char file_buffer[2048];
    int bytes_read;
    struct sockaddr_in client;
    size_t file_size;
    FILE *fp;
    int port;

    if(argc == 2)
        port = atoi(argv[1]);
    else
        port = DFAULT_PORT;

    serverSock = passiveUDP(port);
    len = sizeof(client);

    while(1){
        if(recvfrom(serverSock, buf, sizeof(buf), 0, (struct sockaddr*)&client, &len) < 0){
           printf(" receive failed\n");
           exit(1);
        }

        printf("The requested file is %s\n",buf);

        fp = fopen(buf,"rb");
        if(fp == NULL){
            printf("file does not exist\n");
            continue;
        }

        fseek(fp,0,SEEK_END);
        file_size = ftell(fp);
        rewind(fp);

        memset(file_buffer,0, sizeof(file_buffer));
        if((bytes_read = fread(file_buffer, 1, file_size, fp)) <= 0){
           printf("unable to copy file into buffer\n");
           exit(1);
        }

        if(sendto(serverSock, file_buffer, bytes_read, 0, (struct sockaddr *)&client, len)<0){
            printf("error in sending the file\n");
            exit(1);
        }
    
    }

    flose(fp);
    close(serverSock);
    return 0;
}

int passiveUDP(int port){
    int sock;
    struct sockaddr_in server;
 
    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    sock = socket(AF_INET,SOCK_DGRAM,0);

    if(sock < 0){
        printf("cant create socket\n");
        exit(EXIT_FAILURE);
    }

    if(bind(sock,(struct sockaddr *)&server, sizeof(server))<0){
        printf("bind failed \n");
        exit(EXIT_FAILURE);
    }

    return sock;
}
