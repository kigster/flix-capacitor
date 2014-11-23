#ifndef _FlixCapacitor__H_
#define _FlixCapacitor__H_

// 8 for filename, 1 for ".", 3 for extension, 1 for \0
#define FAT32_FILENAME_LENGTH 13

typedef struct FileListStruct {
    char *parentFolder;
    char **files;
    int size;
    int currentIndex;
    int allocated;
} FileList;

typedef void (*timerCallback)(void);

typedef struct periodicCallStruct {
    uint16_t frequencyMs;
    uint32_t lastCallMs;
    timerCallback callback;
} periodicCall;

#endif
