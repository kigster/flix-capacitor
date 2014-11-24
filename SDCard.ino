/*
 * SSCard
 *
 *  Created on: Nov 20, 2014
 *      Author: Konstantin Gredeskoul
 *        Code: https://github.com/kigster
 *
 *  (c) 2014 All rights reserved, MIT License.
 */
#include "FlixCapacitor.h"

Sd2Card card;
SdVolume volume;
SdFile root;

const static int filesAllocSize = 20;

void nextFileInList(FileList *fileList, char* fullPath, direction dir) {
    if (fileList->size > 0) {
        fileList->currentIndex += (int) dir;
        if (fileList->currentIndex < 0) fileList->currentIndex = fileList->size - 1;
        fileList->currentIndex = fileList->currentIndex % fileList->size;
        sprintf(fullPath, "%s/%s", fileList->parentFolder, fileList->files[fileList->currentIndex]);
    }
}
void listDirectory(FileList *fileList, File dir, int level, char *match) {
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) {
            break;
        }

        if (!entry.isDirectory()) {
            if (fileList->size >= fileList->allocated) {
                return;
            }
            if (strstr(entry.name(), match) != NULL) {
                strcpy(fileList->files[fileList->size++], entry.name());
            }
        }
        entry.close();
    }
}


FileList *findFilesMatchingExtension(char *folder, char *extension) {
    FileList *fileList = (FileList *) malloc(sizeof(FileList));
    fileList->parentFolder = folder;
    fileList->files = (char **) malloc(filesAllocSize * sizeof(char *));
    fileList->allocated = filesAllocSize;
    fileList->size = 0;
    fileList->currentIndex = 0;

    for (int i = 0; i < fileList->allocated ; ++i) {
        fileList->files[i] = (char *)malloc(FAT32_FILENAME_LENGTH);
    }

    File root = SD.open(folder);
    listDirectory(fileList, root, 0, extension);
    return fileList;
}



uint32_t FreeRamTeensy() { // for Teensy 3.0
    uint32_t stackTop;
    uint32_t heapTop;

    // current position of the stack.
    stackTop = (uint32_t) &stackTop;

    // current position of heap.
    void* hTop = malloc(1);
    heapTop = (uint32_t) hTop;
    free(hTop);

    // The difference is the free, available ram.
    return stackTop - heapTop;
}

void showDirectory() {
    Serial.println(
            "\nFiles found on the card (name, date and size in bytes): ");
    root.openRoot(volume);
    // list all files in the card with date and size
    root.ls(LS_R | LS_DATE | LS_SIZE);
}

bool initSDCard() {
    boolean status;

    if (root.isOpen())
        root.close();      // allows repeated calls

    // First, detect the card
    status = card.init(SD_CS); // Audio shield has SD card SD on pin 10
    if (status) {
        Serial.println("SD card is connected :-)");
    } else {
        Serial.println("SD card is not connected or unusable :-(");
        return false;
    }

    // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
    if (!volume.init(card)) {
        Serial.println(
                "Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
        return false;
    }

    root.openRoot(volume);

    // print the type and size of the first FAT-type volume
    Serial.print("\nVolume type is FAT");
    Serial.println(volume.fatType(), DEC);
    Serial.println();

    float size = volume.blocksPerCluster() * volume.clusterCount();
    size = size * (512.0 / 1e6); // convert blocks to millions of bytes
    Serial.print("File system space is ");
    Serial.print(size);
    Serial.println(" Mbytes.");

    status = SD.begin(SD_CS); // Audio shield has SD card CS on pin 10
    if (status) {
        Serial.println("SD library is able to access the filesystem");
    } else {
        Serial.println("SD library can not access the filesystem!");
        Serial.println(
                "Please report this problem, with the make & model of your SD card.");
        Serial.println(
                "  http://forum.pjrc.com/forums/4-Suggestions-amp-Bug-Reports");
    }
    return true;
}

