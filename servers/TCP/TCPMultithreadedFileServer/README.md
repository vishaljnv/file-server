#TCP Multithreaded File Server Program
This program receives request for download or upload of a file from a client and provides the requested service

Lab assignment for CMPE207-Network Programming and Application course, under Dr. Rod Fatoohi at SJSU

Questions in the file CmpE207.lab2.fal17.pdf hold the requirements. 
This program is strictly built for the specifications mentioned in the aforementioned document.

Compiling the code:
-Execute make command in the current directory.

 Eg. $make

Running the Server:

-commandline arguments
-p [Port]

 Eg. $ ./TCPMultithreadedFileServer -p port


Cleaning the build:
-Execute make clean command

 Eg. $make clean

NOTES:
-Service Port 1234 is assumed if not mentioned in the command line.
