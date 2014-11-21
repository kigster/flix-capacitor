/*
 * SSCard
 *
 *  Created on: Nov 20, 2014
 *      Author: Konstantin Gredeskoul
 *        Code: https://github.com/kigster
 *
 *  (c) 2014 All rights reserved, MIT License.
 */

void showDirectory() {
    Serial.println("\nFiles found on the card (name, date and size in bytes): ");
    root.openRoot(volume);
    // list all files in the card with date and size
    root.ls(LS_R | LS_DATE | LS_SIZE);
}

bool initSDCard() {
    boolean status;

    if (root.isOpen()) root.close();      // allows repeated calls

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
      Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
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
      Serial.println("Please report this problem, with the make & model of your SD card.");
      Serial.println("  http://forum.pjrc.com/forums/4-Suggestions-amp-Bug-Reports");
    }
    return true;
}

