/*
 * FileSystem.cpp
 *
 *  Created on: Dec 2, 2014
 *      Author: kig
 */

#include "FlixCapacitor.h"
#ifdef ENABLE_AUDIO_SD

#include "FileSystem.h"
#include "print_helpers.h"

Sd2Card card;
SdVolume volume;
SdFile root;


FileSystem::FileSystem(uint8_t pin) {
    pinCS = pin;
    sdCardInitialized = false;
}

bool FileSystem::hasInitialized() {
    return sdCardInitialized;
}

bool FileSystem::initSDCard() {
    boolean status;

    if (root.isOpen())
        root.close();      // allows repeated calls

    // First, detect the card
    status = card.init(pinCS); // Audio shield has SD card SD on pin 10
    if (status) {
        Serial.println("SD card is connected :-)");
    } else {
        Serial.println("SD card is not connected or unusable :-(");
        sdCardInitialized = false;
        return sdCardInitialized;
    }

    // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
    if (!volume.init(card)) {
        Serial.println(
                "Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
        sdCardInitialized = false;
        return sdCardInitialized;
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

    status = SD.begin(pinCS); // Audio shield has SD card CS on pin 10
    if (status) {
        Serial.println("SD library is able to access the filesystem");
    } else {
        Serial.println("SD library can not access the filesystem!");
        Serial.println(
                "Please report this problem, with the make & model of your SD card.");
        Serial.println(
                "  http://forum.pjrc.com/forums/4-Suggestions-amp-Bug-Reports");
    }
    sdCardInitialized = true;
    return sdCardInitialized;
}

bool FileSystem::nextFileInList(FileList *fileList, char* fullPath, direction dir) {
    if (fileList->size > 0) {
        fileList->currentIndex += (int) dir;
        if (fileList->currentIndex < 0)
            fileList->currentIndex = fileList->size - 1;
        fileList->currentIndex = fileList->currentIndex % fileList->size;
        sprintf(fullPath, "%s/%s", fileList->parentFolder,
                fileList->files[fileList->currentIndex]);
        return true;
    } else
        return false;
}

void FileSystem::saveDirectory(FileList *fileList, File dir, int level, char *match) {
    printv("Opening directory ", dir.name());
    int files = 0;
    // count how many files are there
    while (true) {
        File entry = dir.openNextFile();
        if (!entry)
            break;
        if (!entry.isDirectory())
            files++;
        entry.close();
    }
    printv("Files found: ", files);

    dir.rewindDirectory();
    delay(10);

    if ((fileList->files = (char **) malloc(files * sizeof(char *))) == NULL) {
        Serial.println("Can't allocate RAM");
        return;
    }
    fileList->allocated = files;

    for (int i = 0; i < fileList->allocated; i++) {
        if ((fileList->files[i] = (char *) malloc(FAT32_FILENAME_LENGTH))
                == NULL) {
            Serial.println("Can't allocate RAM");
            return;
        }
    }

    while (true) {
        File entry = dir.openNextFile();
        if (!entry)
            break;
        if (!entry.isDirectory()) {
            if (strstr(entry.name(), match) != NULL) {
                strcpy(fileList->files[fileList->size++], entry.name());
            }
        }
        entry.close();
    }
    sprintf(buf, "loaded %d files from directory %s", files, dir.name());
    Serial.println(buf);
}

FileList *FileSystem::findFilesMatchingExtension(char *folder, char *extension) {
    FileList *fileList = (FileList *) malloc(sizeof(FileList));
    fileList->parentFolder = folder;
    fileList->size = 0;
    fileList->currentIndex = 0;

    File root = SD.open(folder);
    saveDirectory(fileList, root, 0, extension);
    return fileList;
}

FileSystem::~FileSystem() {
}


#endif
