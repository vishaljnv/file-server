program FILESERVER {
    version FILESERVERVERS {
        int GET_FILE_SIZE(string) = 1;
        string REQUEST_FILE(string, int) = 2;
    } = 1;
} = 0x20000001;
