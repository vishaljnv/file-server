/*
 * San Jose State University
 * Department of Computer Engineering
 * 207 - Network Programming and Application
 * Lab Assignment-II
 * Question 1

 * Group 4
   * Akhilesh
   * Karthikeya Rao
   * Vishal Govindraddi Yarabandi
 
NOTES:
* This is a UDP Multiprocess File Server for only Download of a file by client.

@USAGE:
    $ ./UDPMultiprocessFileServer [port]
*/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>
#include<signal.h>
#define DFAULT_PORT    1234

int passiveUDP(int port);
void reaper(int signal);

int main(int argc, char *argv[]){
    int serverSock, len;
    char buf[256];
    struct sockaddr_in client;
    int port;
    int pid;

    if(argc == 2)
        port = atoi(argv[1]);
    else
        port = DFAULT_PORT;

    serverSock = passiveUDP(port);
    len = sizeof(client);

    signal(SIGCHLD, reaper);
    while(1){
        if(recvfrom(serverSock, buf, sizeof(buf), 0, (struct sockaddr*)&client, &len) < 0){
           printf(" receive failed\n");
           continue;
        }
        if((pid = fork()) != 0){
            //parent here!
        }
        else{  //child here
            exit(sendFile(serverSock, buf, client));
        }
    }

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

int sendFile(int sock, char *filename, struct sockaddr_in client){
    char file_buffer[2048];
    int bytes_read;
    size_t file_size;
    FILE *fp;

    fp = fopen(filename,"rb");
    if(fp == NULL){
        printf("file does not exist\n");
        exit(1);
    }

    fseek(fp,0,SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    memset(file_buffer,0, sizeof(file_buffer));
    if((bytes_read = fread(file_buffer, 1, file_size, fp)) <= 0){
       printf("unable to copy file into buffer\n");
       exit(1);
    }

    if(sendto(sock, file_buffer, bytes_read, 0, (struct sockaddr *)&client, sizeof(client))<0){
        printf("error in sending the file\n");
        exit(1);
    }

    fclose(fp);

    printf("The requested file is %s\n",filename);
    return EXIT_SUCCESS;
}

void reaper(int signal){
    /*Cleaning up zombies*/

    int status;
    while(wait3(&status, WNOHANG, NULL) >= 0)
        /*do nothing but wait*/;
}
