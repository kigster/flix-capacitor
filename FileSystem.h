/*
 * FileSystem.h – various utilities for reading SD card and loading files
 * matching filename pattern or an extension.
 *
 *  Created on: Dec 2, 2014
 *      Author: kig
 */

#include "FlixCapacitor.h"

#ifdef ENABLE_AUDIO_SD

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <SD.h>

typedef struct FileListStruct {
    char *parentFolder;
    char **files;
    int size;
    int currentIndex;
    int allocated;
} FileList;

enum direction { PREVIOUS = -1, CURRENT = 0, NEXT = 1};

class FileSystem {
public:
	FileSystem(uint8_t pinCS);
	virtual ~FileSystem();

	bool initSDCard();
	bool hasInitialized();
	FileList *findFilesMatchingExtension(char *folder, char *extension);
	bool nextFileInList(FileList *fileList, char* fullPath, direction dir);

private:
	void saveDirectory(FileList *fileList, File dir, int level, char *match);
	bool sdCardInitialized;
	uint8_t pinCS; // pin where slave select on SD is located
	char buf[50];
};

#endif /* FILESYSTEM_H_ */
#endif /* ENABLE_AUDIO_SD */
