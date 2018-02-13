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
* This is a UDP Pre-threaded File Server for only Download of a file by client.

@USAGE:
    $ ./UDPPretheadedFileServer -p port -c 5
     OR
    $ ./UDPPretheadedFileServer -c 5
*/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>

#include<pthread.h>

#define DFAULT_PORT    1234

int passiveUDP(int port);
void childMainLoop(int);
int sendFile(int sock, char *filename, struct sockaddr_in client);

int main(int argc, char *argv[]){
    int port = 1234;
    int numOfThreads = 0;
    char ch;
    int i;
    int serverSock;
    pthread_t thread_id;

    while((ch = getopt(argc, argv, "p:c:")) != -1){
        switch(ch){
            case 'p': port = atoi(optarg);
                break;
            case 'c': numOfThreads = atoi(optarg);
        }
    }

    if(!numOfThreads){
        printf("Number of threads not mentioned!\n");
        exit(EXIT_FAILURE);
    }

    serverSock = passiveUDP(port);
    for(i = 0; i < numOfThreads; i++){
        if(pthread_create(&thread_id, NULL, (void *(*)(void *))childMainLoop, (void *)serverSock) < 0){
            printf("thread create faild!\n");
        }
    }

    while(1);
}

void childMainLoop(int serverSock){
    int len;
    char buf[256];
    struct sockaddr_in client;
 
    len = sizeof(client);
    while(1){
        if(recvfrom(serverSock, buf, sizeof(buf), 0, (struct sockaddr *)&client, &len) < 0){
            printf(" receive failed\n");
            continue;
        }
        sendFile(serverSock, buf, client);
    }
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
