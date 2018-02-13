#TCP client for File Server Program
This program requests for download or upload of a file from the file server, running in a remote host.

Lab assignment for CMPE207-Network Programming and Application course, under Dr. Rod Fatoohi at SJSU

Questions in the file CmpE207.lab2.fal17.pdf hold the requirements. 
This program is strictly built for the specifications mentioned in the aforementioned document.

Compiling the code:
-Execute make command in the current directory.

 Eg. $make

Running the Server:

-commandline options
 -h File server IP
 -p [port]
 -u file name of file to upload
 -d file name of file to download

Upload Example:
 Eg. $ ./TCPClient -h 127.0.0.1 -p port -u testFileForUpload

Download Example:
 Eg. $ ./TCPClient -h 127.0.0.1 -p port -d testFileToDownload

Cleaning the build:
-Execute make clean command

 Eg. $make clean

