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
* This is a UDP Pre-forked File Server for Download of a file by client.

@USAGE:
    $ ./UDPPreforkedFileServer -p port -c 6
     OR
    $ ./UDPPreforkedFileServer -c 6
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
    struct sockaddr_in client;
    int port = 1234;
    int pid;
    int numOfChildren = 0;
    char ch;
    int i;

    while((ch = getopt(argc, argv, "p:c:")) != -1){
        switch(ch){
            case 'p': port = atoi(optarg);
                break;
            case 'c': numOfChildren = atoi(optarg);
        }
    }

    if(!numOfChildren){
        printf("Number of children not mentioned!\n");
        exit(EXIT_FAILURE);
    }

    serverSock = passiveUDP(port);
    len = sizeof(client);

    for(i = 0; i < numOfChildren; i++){
        pid = fork();
        if(pid == 0){
            while(1){
                if(recvfrom(serverSock, buf, sizeof(buf), 0, (struct sockaddr*)&client, &len) < 0){
                   printf(" receive failed\n");
                   continue;
                }
                sendFile(serverSock, buf, client);
            }
        }
        else if(pid < 0){
            perror("fork failed!\n");
        }
        else{
            //parent here! Doing nothing
        }
    }

    wait(NULL);
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
