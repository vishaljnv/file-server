#UDP Prethreaded File Server Program

This program receives the file name from client and sends the file data over socket.

Lab assignment for CMPE207-Network Programming and Application course, under Dr. Rod Fatoohi at SJSU

Questions in the file CmpE207.lab2.fal17.pdf hold the requirements. 
This program is strictly built for the specifications mentioned in the aforementioned document.

Compiling the code:
-Execute make command in the current directory.

 Eg. $make


commandline options:
-p [port]
-c number of threads to create

Running the Server:
 Eg. $ ./UDPPrethreadedFileServer -p port -c 7


Cleaning the build:
-Execute make clean command

 Eg. $make clean

NOTES:
-Service Port 1234 is assumed if not mentioned in the command line.
