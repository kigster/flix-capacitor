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
    uint32_t frequencyMs;
    timerCallback callback;
    bool active;
    uint32_t lastCallMs;
} periodicCall;

enum direction { PREVIOUS = -1, CURRENT = 0, NEXT = 1};
#define MUSIC_ENABLED

#endif
