/*
 * San Jose State University
 * Department of Computer Engineering
 * 207 - Network Programming and Application
 * Lab Assignment-II
 * Question 3

 * Group 4
   * Akhilesh
   * Karthikeya Rao
   * Vishal Govindraddi Yarabandi
 
NOTES:
* This is a TCP Client for file upload/download to/from the TCP File Server.

@USAGE:
    Upload:
        $ ./TCPClient -h TCPFileServerIP -p port -u filename
    Download:
        $ ./TCPClient -h TCPFileServerIP -p port -d filename
*/

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<errno.h>
#include<unistd.h>
#include<string.h>
#include<netinet/in.h>
#include<fcntl.h>

#define REQUEST_MSGLEN        512
#define REPLY_MSGLEN          256
#define ERROR_MSGLEN          128

#define READY_FOR_FILE_TRANSFER          1
#define NOT_READY_FOR_FILE_TRANSFER      2

char error_msg[ERROR_MSGLEN];

void setErrorMessage(char *msg);
int connectToFileServer(char *ip, int port);

void constructDownloadRequestMessage(char *filename, char *msg);
void constructUploadRequestMessage(char *filename, size_t file_size, char *msg);

int downloadRemoteFile(int connectedSock, char *filename);
int downloadFile(int connectedSock, char *filename, size_t filesize, unsigned chunk_size);
int isFileReadyForDownload(char *reply, size_t *file_size, unsigned *chunk_size);

int uploadFileToRemote(int connectedSock, char *filename);
int uploadFile(int connectedSock, char *filename, size_t file_size, unsigned chunk_size);
int isServerReadyForUpload(char *reply, unsigned *chunk_size);

int parseData(char *str, char *start_marker, char *end_marker, char *data);

int main(int argc, char *argv[]){
    int sock;
    char *ip = NULL;
    char *upload_filename = NULL;
    char *download_filename = NULL;
    int upload = 0, download = 0;
    int port = -1;
    char ch;

    while((ch = getopt(argc, argv, "h:p:u:d:")) != -1){ //get command line arguments
        switch(ch){
            case 'h': ip = optarg;                     //-h option for file server IP address
                break;
            case 'p': port = atoi(optarg);             //-p option for file server service port
                break;
            case 'u': upload_filename = optarg;        //-u for upload
                      upload = 1;
                break;
            case 'd': download_filename = optarg;      //-d for download
                      download = 1;
        }
    }

    if(ip == NULL || port == -1 || (download == 0 && upload == 0)){ //exit if no command line arguments were given.
        printf("Parameters missing!\n");
        exit(EXIT_FAILURE);
    }

    printf("connecting...\n");
    if((sock = connectToFileServer(ip, port)) < 0){ //establish a connection to TCP file server
        perror("Could not connect to file server!\n");
        printf("ERROR: %s\n",error_msg);
        exit(EXIT_FAILURE);
    }

    printf("connected!\n");
    if(download){
        if(access(download_filename, F_OK) == 0){ //exit if a file with same name exists in current directory
            printf("Download failed: %s already exists\n",download_filename);
            exit(EXIT_FAILURE);
        }

        if(downloadRemoteFile(sock, download_filename) < 0){ //download file from server
            printf("Download failed: %s\n",error_msg);
            close(sock);
            exit(EXIT_FAILURE);
        }

        printf("Download complete!\n");
    }

    else if(upload){
        if(access(upload_filename, F_OK) != 0){ //check if file exists before sending upload request
            printf("Upload failed: %s does not exist\n",upload_filename);
            exit(EXIT_FAILURE);
        }
        if(uploadFileToRemote(sock, upload_filename) < 0){ //upload to remote server
            printf("Upload failed: %s\n",error_msg);
            close(sock);
            exit(EXIT_FAILURE);
        }
    
        printf("Upload complete!\n");
    }

    exit(EXIT_SUCCESS);
}

void setErrorMessage(char *msg){
    /*set global error_message; used across the program*/

    memset(error_msg, 0, ERROR_MSGLEN);
    strcpy(error_msg, msg);
}

int connectToFileServer(char *ip, int port){
    /*Establish a connection with the remote file server and return the connected socket*/

    int sock;
    struct sockaddr_in server;

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ //create TCP socket
        setErrorMessage("socket error!");
        return -1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;  //assuming IPv4
    server.sin_port = htons(port); //host to network byte order

    if(inet_pton(AF_INET, ip, &server.sin_addr) <= 0){ //convert IP address to network address structure
        setErrorMessage("inet_pton failed!");
        return -1;
    }

    if(connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0){ //request connection
        setErrorMessage("connect failed!");
        return -1;
    }

    return sock;
}

int downloadRemoteFile(int connectedSock, char *filename){
    /*Send initial download request, if server is ready to send the file, 
      start receiving file data over the socket*/

    int n;
    unsigned chunk_size = 0;
    size_t filesize = 0;
    char request[REQUEST_MSGLEN];
    char reply[REPLY_MSGLEN];

    constructDownloadRequestMessage(filename, request);

    if((n = send(connectedSock, request, (strlen(request) + 1), 0)) < (strlen(request) + 1)){ //send download request message
        setErrorMessage("send failed!");
        return -1;
    }

    if((n = recv(connectedSock, reply, REPLY_MSGLEN, 0)) <= 0){ //receive reply
        setErrorMessage("recv failed!");
        return -1;
    }

    if(!isFileReadyForDownload(reply, &filesize, &chunk_size)){ //parse reply and check if server is ready with the file
        return -1;
    }

    return downloadFile(connectedSock, filename, filesize, chunk_size); //start downloading the file
}

int uploadFileToRemote(int connectedSock, char *filename){
    /*Send initial upload request, if server is ready to receive,
      send file data over the socket*/

    int n;
    size_t file_size = 0;
    unsigned chunk_size = 0;
    char request[REQUEST_MSGLEN];
    char reply[REPLY_MSGLEN];
    FILE *fp;

    if((fp = fopen(filename, "rb")) == NULL){ //try to open file in binary read mode
        setErrorMessage("open file failed!");
        return -1;
    }

    fseek(fp, 0L, SEEK_END); //get file size
    file_size = ftell(fp);
    rewind(fp);

    constructUploadRequestMessage(filename, file_size, request);

    if((n = send(connectedSock, request, (strlen(request) + 1), 0)) < (strlen(request) + 1)){ //send upload request message
        setErrorMessage("send failed!");
        return -1;
    }

    if((n = recv(connectedSock, reply, REPLY_MSGLEN, 0)) <= 0){ //receive reply
        setErrorMessage("recv failed!");
        return -1;
    }

    if(!isServerReadyForUpload(reply, &chunk_size)){ //parse reply message and check if server is ready to receive the file
        return -1;
    }

    return uploadFile(connectedSock, filename, file_size, chunk_size); //start uploading the file
}

void constructDownloadRequestMessage(char *filename, char *msg){
    /* Construct download request message 
       Ex: REQ_START:R:REQ_END FILENAME_START:helloworld.mp4:FILENAME_END */

    memset(msg, 0, REQUEST_MSGLEN);
    sprintf(msg, "%s %s%s%s", "REQ_START:R:REQ_END", "FILENAME_START:",filename,":FILENAME_END");
}

void constructUploadRequestMessage(char *filename, size_t file_size, char *msg){
    /* Construct upload request message 
       Ex: REQ_START:W:REQ_END FILESIZE_START:271532:FILESIZE_END FILENAME_START:helloworld.mp4:FILENAME_END */

    memset(msg, 0, REQUEST_MSGLEN);
    sprintf(msg, "%s %s%zu%s %s%s%s", "REQ_START:W:REQ_END", "FILESIZE_START:",file_size,":FILESIZE_END",
                 "FILENAME_START:",filename,":FILENAME_END");
}

int isFileReadyForDownload(char *reply, size_t *filesize, unsigned *chunk_size){
    /* Parse reply msg and check if server is ready with the file,
       if yes then get the file size and the chunk size to use in every recv/send call.*/

    char result[4];
    char filesize_str[22];
    char chunksize_str[12];
    
    *chunk_size = 0;
    *filesize = 0;

    if (parseData(reply, "RES_START:",":RES_END",result)){  //Getting result
        setErrorMessage("result unknown");
        return 0;
    }

    parseData(reply, "ERR_START:",":ERR_END",error_msg);  //Getting error message

    if(atoi(result) != READY_FOR_FILE_TRANSFER)
        return 0;

    if(parseData(reply, "FILESIZE_START:",":FILESIZE_END",filesize_str)){ //Getting file size
        setErrorMessage("file size not specified");
        return 0;
    }

    *filesize = atoi(filesize_str);

    if(parseData(reply, "CHUNKSIZE_START:",":CHUNKSIZE_END",chunksize_str)){ //Getting chunk size
        setErrorMessage("chunksize not specified");
        return 0;
    }

    *chunk_size = atoi(chunksize_str);
    return 1;
}

int isServerReadyForUpload(char *reply, unsigned *chunk_size){
    /*Parse the reply message and check if server is ready to receive the file*/

    char result[4];
    char chunksize_str[12];

    if (parseData(reply, "RES_START:",":RES_END",result)){  //Getting result
        setErrorMessage("result unknown");
        return 0;
    }

    parseData(reply, "ERR_START:",":ERR_END",error_msg);  //Getting error message

    if(atoi(result) != READY_FOR_FILE_TRANSFER)
        return 0;

    if(parseData(reply, "CHUNKSIZE_START:",":CHUNKSIZE_END",chunksize_str)){ //Getting chunk size
        setErrorMessage("chunksize not specified");
        return 0;
    }

    *chunk_size = atoi(chunksize_str);
    return 1;
}

int parseData(char *str, char *start_marker, char *end_marker, char *data){
    /*Parse data between two markers and store in data buffer*/

    char *start_addr, *end_addr;
    int data_len;

    if((start_addr = strstr(str, start_marker)) == NULL)
        return -1;

    if((end_addr = strstr(str, end_marker)) == NULL)
        return -1;

    data_len = end_addr - start_addr - strlen(start_marker);
    if(end_addr != NULL){
        strncpy(data, start_addr + strlen(start_marker), data_len);
    }

    data[data_len] = '\0';
    return 0;
}

int downloadFile(int connectedSock, char *filename, size_t file_size, unsigned chunk_size){
    /*Download the file data in a local file*/

    int i;
    unsigned bytes_received, bytes_written;
    size_t total_bytes_received = 0;
    FILE *fp;
    char *buf = NULL;

    if((fp = fopen(filename, "ab")) == NULL){ //try opening file in binary append mode
        setErrorMessage("create file failed!");
        return -1;
    }

    buf = (char *)malloc(chunk_size);
    printf("Downloading file...\n");
    shutdown(connectedSock, SHUT_WR);
    while(total_bytes_received != file_size){ //read data as long as server is sending, in chunks, and store in local file.
        memset(buf, 0, chunk_size);

        if((bytes_received = recv(connectedSock, buf, chunk_size, 0)) < 0){
            setErrorMessage("receive data failed!");
            fclose(fp);
            free(buf);
            return -1;
        }

        if((bytes_written = fwrite(buf, 1, bytes_received, fp)) != bytes_received){
            setErrorMessage("write data failed!");
            fclose(fp);
            free(buf);
            return -1;
        }

        total_bytes_received += bytes_received;
    }

    fclose(fp);
    free(buf);
    return 0;
}

int uploadFile(int connectedSock, char *filename, size_t file_size, unsigned chunk_size){
    /*Upload file to the remote server*/

    int i;
    unsigned bytes_read, bytes_sent;
    unsigned bytes_to_send = 0;
    size_t total_bytes_sent = 0;
    FILE *fp;
    char *buf = NULL;

    if((fp = fopen(filename, "rb")) == NULL){ //try opening the file in binary read mode
        setErrorMessage("create file failed!");
        return -1;
    }

    buf = (char *)malloc(chunk_size);
    printf("Uploading file...\n");
    shutdown(connectedSock, SHUT_RD);
    while(total_bytes_sent != file_size){ //send file data until all the file data is sent
        memset(buf, 0, chunk_size);
        bytes_to_send = ((file_size - total_bytes_sent) > chunk_size)? chunk_size : file_size - total_bytes_sent ;

        if((bytes_read = fread(buf, 1, bytes_to_send, fp)) <= 0){
            setErrorMessage("read failed");
            fclose(fp);
            free(buf);
            return -1;
        }
        if((bytes_sent = send(connectedSock, buf, bytes_read, 0)) != bytes_read){
            setErrorMessage("sending data failed");
            fclose(fp);
            free(buf);
            return -1;
        }

        total_bytes_sent += bytes_sent;
    }

    fclose(fp);
    free(buf);
    return 0;
}
