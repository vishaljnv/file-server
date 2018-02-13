#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

void downloadFileFromRemote(char *ip, int port, char *filename);

int main(int argc, char *argv[]){
    char *filename;
    char *ip = NULL;
    int download = 0;
    int port = -1;
    char ch;

    while((ch = getopt(argc, argv, "h:p:d:")) != -1){
        switch(ch){
            case 'h': ip = optarg;
                 break;
            case 'p': port = atoi(optarg);
                break;
            case 'd': download = 1;
                      filename = optarg;
        }
    }

    if(ip == NULL || port == -1 || !download){
        printf("Arguments missing!\n");
        exit(EXIT_FAILURE);
    }

    // create socket in client side
    downloadFileFromRemote(ip, port, filename);

    return 0;
}

void downloadFileFromRemote(char *ip, int port, char *filename){
    int received;
    int sockfd, len;
    char file_buffer[2048];
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        printf(" socket not created in client\n");
        exit(1);
    }

    memset(&servaddr,0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &servaddr.sin_addr.s_addr) ; //use specific address in cmd line
    servaddr.sin_port = htons(port);  // Port address

    len = sizeof(servaddr);
    if(sendto(sockfd, filename, strlen(filename) + 1, 0, (struct sockaddr *)&servaddr, len)<0){
      printf("sending failed \n");
      exit(1);
    }

    if((received = recvfrom(sockfd, file_buffer, 4096, 0, (struct sockaddr *)&servaddr, &len)) < 0){
      printf("error in recieving the file\n");
      exit(1);
    }

    FILE *fp;
    fp = fopen(filename,"ab");

    if(fwrite(file_buffer, 1, received, fp) < 0){
      printf("error writting file\n");
      exit(1);
    }
    else{
	printf("File received successfully!!!\n");
    }

    fclose(fp);
    close(sockfd);
}
