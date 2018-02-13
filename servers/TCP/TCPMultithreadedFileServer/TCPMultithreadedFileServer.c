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
* This is a TCP Multithreaded File Server for Upload and Download of a file, with one thread per client.

@USAGE:
    $ ./TCPMultithreadedFileServer -p port
     OR
    $ ./TCPMultithreadedFileServer
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<netinet/in.h>
#include<string.h>
#include<fcntl.h>

#include<sys/socket.h>
#include<sys/signal.h>
#include<sys/errno.h>

#include<pthread.h>

#define REQUEST_MSGLEN                  512
#define REPLY_MSGLEN                    256
#define ERROR_MSGLEN                    128

#define DEFAULT_PORT                    1234
#define BACKLOG                         5

#define READ_FILE_REQUEST               'R'
#define WRITE_FILE_REQUEST              'W'

#define READY_FOR_FILE_TRANSFER          1
#define NOT_READY_FOR_FILE_TRANSFER      2

#define DATA_TRANSFERE_CHUNK_SIZE       4096   //4KB

#define TRANSFERE_COMPLETED              0
#define TRANSFERE_FAILED                -1

char error_msg[ERROR_MSGLEN];

void setErrorMessage(char *msg);
int parseRequestMessage(char *request, char *request_type, char *filename, size_t *file_size);
int parseData(char *str, char *start_marker, char *end_marker, char *data);
void constructReplyMessage(char *msg, int result, char *err_msg, unsigned chunk_size, size_t file_size);
int serviceClient(int connectedSock);
int sendFile(int connectedSock, char *filename);
int receiveFile(int connectedSock, char *filename, size_t file_size);
int cleanUpAndReturn(int result, char *data_buf, FILE *fp, char *filename);

int main(int argc, char *argv[]){
    char ch;
    int port = DEFAULT_PORT;
    int listeningSock, connectedSock;
    struct sockaddr_in server;
    int enable = 1;
    pthread_attr_t thread_attrs;
    pthread_t thread_id;

    if((ch = getopt(argc, argv, "p:")) != -1){  //Get the port from -p option if mentioned
        port = atoi(optarg);
    }

    if((listeningSock = socket(AF_INET, SOCK_STREAM,0)) < -1){  //create a TCP socket
        perror("socket error: ");
        exit(EXIT_FAILURE);
    }

    setsockopt(listeningSock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)); //just want to be able to run immediately after Ctrl+c

    memset(&server, 0, sizeof(server));       //good programming practice!?
    server.sin_family = AF_INET;              //IPv4 only
    server.sin_port = htons(port);            //host-to-network byte order
    server.sin_addr.s_addr = htonl(INADDR_ANY);  //choose any interface

    if(bind(listeningSock, (struct sockaddr *)&server, sizeof(server)) < 0){ //bind socket to endpoint with details as in "server"
        perror("bind error: ");
        exit(EXIT_FAILURE);
    }

    if(listen(listeningSock, BACKLOG) < 0){ //listen for connections
        perror("listen error: ");
        exit(EXIT_FAILURE);
    }

    pthread_attr_init(&thread_attrs);
    pthread_attr_setdetachstate(&thread_attrs, PTHREAD_CREATE_DETACHED); //make thread detachable

    while(1){
        if((connectedSock = accept(listeningSock, NULL, NULL)) < 0){  //block for accepting incoming connections
            perror("accept error: ");
            exit(EXIT_FAILURE);
        }

        //create thread for every client connection
        if(pthread_create(&thread_id, &thread_attrs, (void * (*)(void *))serviceClient, (void *)connectedSock) < 0){
            perror("create thread failed!");
        }
    }
}

int serviceClient(int connectedSock){
    /*Receive a client request, get the request type and details and provide service.
      Return 0 on successfull completion -1 otherwise */

    int n;
    char request[REQUEST_MSGLEN];
    char reply[REPLY_MSGLEN];
    char request_type;
    char filename[256];
    size_t file_size = 0;

    if((n = recv(connectedSock, request, REQUEST_MSGLEN, 0)) < 0){ //receive request message
        perror("recv error!");
        return -1;
    }

    if(parseRequestMessage(request, &request_type, filename, &file_size) < 0){ //parse the request message to get details
        printf("Parsing request message failed! Error: %s\n",error_msg);
        return -1;
    }

    if(request_type == READ_FILE_REQUEST){ //if read requested, send the requested file.
        return sendFile(connectedSock, filename);
    }
    else if(request_type == WRITE_FILE_REQUEST){ //if write requested, receive the file.
        return receiveFile(connectedSock, filename, file_size);
    }

    close(connectedSock);
    return -1;
}

void setErrorMessage(char *msg){
    /*Set the global variable error_msg; used across the program.*/

    memset(error_msg, 0, ERROR_MSGLEN);
    strcpy(error_msg, msg);
}

int parseRequestMessage(char *request, char *request_type, char *filename, size_t *file_size){
    /*Parse the request message as in "request" and return results in value-result arguments
      request_type, filename, file_size. */

    char request_type_str[3];
    char file_size_str[22];

    *file_size = 0;

    memset(file_size_str, 0, 22);
    memset(request_type_str, 0, 2);

    if (parseData(request, "REQ_START:",":REQ_END",request_type_str)){  //Getting request type
        setErrorMessage("request unknown");
        return -1;
    }
    *request_type = request_type_str[0];

    if(parseData(request, "FILENAME_START:",":FILENAME_END",filename)){ //Getting file name
        setErrorMessage("file name not specified");
        return -1;
    }

    if(!parseData(request, "FILESIZE_START:",":FILESIZE_END",file_size_str)){ //getting file size
        *file_size = atoi(file_size_str);
    }
    
    return 0;
}

int sendFile(int connectedSock, char *filename){
    /*If file exists in the local directory, send the reply to start receiving data. 
      Otherwise send the reply with result value as F(failed) and error message as
      "file does not exist".*/

    FILE *fp = NULL;
    char reply[REPLY_MSGLEN];
    char *data;
    int result;
    size_t total_bytes_sent = 0;
    size_t file_size;
    unsigned bytes_read = 0, bytes_sent = 0;
    unsigned bytes_to_send = 0;
    unsigned chunk_size = DATA_TRANSFERE_CHUNK_SIZE;
    int i, n, reply_msg_len;

    if((fp = fopen(filename, "rb")) == NULL){ //try opening file in binary read mode.
        result = NOT_READY_FOR_FILE_TRANSFER;
        constructReplyMessage(reply, result, "file does not exist", 0, 0);
    }
    else{
        fseek(fp, 0L, SEEK_END);   //getting file size
        file_size = ftell(fp);
        rewind(fp);
        result = READY_FOR_FILE_TRANSFER;
        constructReplyMessage(reply, result, "Success", chunk_size, file_size);
    }

    reply_msg_len = strlen(reply) + 1;
    if((n = send(connectedSock, reply, reply_msg_len, 0)) != reply_msg_len){ //send reply message.
        setErrorMessage("sending reply failed");
        return -1;
    }

    if(result == NOT_READY_FOR_FILE_TRANSFER){
        return -1;
    }

    data = (char *)malloc(chunk_size);
    while(total_bytes_sent != file_size){ //Send file in chunks until all the data is sent successfully
        memset(data, 0, chunk_size);
        bytes_to_send = ((file_size - total_bytes_sent) > chunk_size)? chunk_size : file_size - total_bytes_sent ;

        if((bytes_read = fread(data, 1, bytes_to_send, fp)) <= 0){
            setErrorMessage("read failed");
            free(data);
            fclose(fp);
            return -1;
        }
        if((bytes_sent = send(connectedSock, data, bytes_read, 0)) != bytes_read){
            setErrorMessage("sending data failed");
            free(data);
            fclose(fp);
            return -1;
        }

        total_bytes_sent += bytes_sent;
    }

    free(data);
    fclose(fp);
    return 0;
}

void constructReplyMessage(char *msg, int result, char *err_msg, unsigned chunk_size, size_t file_size){
    /*construct reply message with start and end markers for every field.
      Ex: Success message format : "RES_START:S:RES_END ERR_START:Success:ERR_END
               FILESIZE_START:271532:FILESIZE_END CHUNKSIZE_START:4096:CHUNKSIZE_END"
          Failure message format: "RES_START:F:RES_END ERR_START:file does not exist:ERR_END
               FILESIZE_START:0:FILESIZE_END CHUNKSIZE_START:0:CHUNKSIZE_END" */
    memset(msg, 0, REPLY_MSGLEN);
    sprintf(msg, "%s%d%s %s%s%s %s%zu%s %s%u%s", "RES_START:",result, ":RES_END", "ERR_START:",err_msg,":ERR_END",
            "FILESIZE_START:",file_size,":FILESIZE_END", "CHUNKSIZE_START:",chunk_size, ":CHUNKSIZE_END");
}

int parseData(char *str, char *start_marker, char *end_marker, char *data){
    /*Get data between two markers. 
     Ex: for string like FILESIZE_START:523314:FILE_SIZE_END return 523314 in data buffer*/

    char *start_addr, *end_addr;
    int data_len = 0;

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

int receiveFile(int connectedSock, char *filename, size_t file_size){
    /*If a file with same name exists then reject upload, recceive file otherwise */

    FILE *fp = NULL;
    int result;
    char reply[REPLY_MSGLEN];
    char *data_buf = NULL;
    unsigned chunk_size = DATA_TRANSFERE_CHUNK_SIZE;
    unsigned bytes_received = 0, bytes_written = 0;
    size_t total_bytes_received = 0;
    int i, n, reply_msg_len;

    if(access(filename, F_OK) == 0){ //Check if file already exists locally
        result = NOT_READY_FOR_FILE_TRANSFER;
        constructReplyMessage(reply, result, "file already exists", 0, 0);
    }
    else{
        result = READY_FOR_FILE_TRANSFER;
        constructReplyMessage(reply, result, "Success", chunk_size, file_size);
    }

    reply_msg_len = strlen(reply) + 1;
    if((n = send(connectedSock, reply, reply_msg_len, 0)) != reply_msg_len){ //send reply
        setErrorMessage("sending reply failed");
        return -1;
    }

    if(result == NOT_READY_FOR_FILE_TRANSFER){
        return -1;
    }

    if((fp = fopen(filename, "ab")) == NULL){ //open file in binary append mode to start writing data
        setErrorMessage("file open failed");
        return -1;
    }

    data_buf = (char *)malloc(chunk_size);
    while(total_bytes_received != file_size){ //receive data as long as client is sending, in chunks, and write to the local file.
        memset(data_buf, 0, chunk_size);

        if((bytes_received = recv(connectedSock, data_buf, chunk_size, 0)) <= 0){
            setErrorMessage("receiving data failed");
            return cleanUpAndReturn(TRANSFERE_FAILED, data_buf, fp, filename);
        }

        if((bytes_written = fwrite(data_buf, 1, bytes_received, fp)) != bytes_received){
            setErrorMessage("writing to file failed");
            return cleanUpAndReturn(TRANSFERE_FAILED, data_buf, fp, filename);
        }

        total_bytes_received += bytes_received;
    }

    return cleanUpAndReturn(TRANSFERE_COMPLETED, data_buf, fp, filename);
}

int cleanUpAndReturn(int result, char *data_buf, FILE *fp, char *filename){
    /*Tasks to do here, in order.
      1. Free the allocated memory 
      2. Close FILE stream object
      3. On success, just return 0
      4. On failure, remove partially downloaded file and return -1.
    */

    if(data_buf != NULL){
        free(data_buf);
    }

    if(fp != NULL){
        fclose(fp);
    }

    if(result == TRANSFERE_COMPLETED){
        return 0;
    }
    else{
        if(!access(filename, F_OK)){ //check if file exists before removing it
            remove(filename);
        }
        return -1;
    }
}
