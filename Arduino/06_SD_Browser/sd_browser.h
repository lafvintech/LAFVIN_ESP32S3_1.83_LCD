#ifndef SD_BROWSER_H
#define SD_BROWSER_H

#include <Arduino.h>
#include <FS.h>

// One item from a directory listing.
struct SdEntry {
  String name;
  String path;
  bool isDirectory;
  uint64_t size;
};

// Basic card information shown on the UI.
struct SdCardInfo {
  uint8_t cardType;
  uint64_t cardSizeBytes;
  uint64_t totalBytes;
  uint64_t usedBytes;
};

// Mount the SD card and fill card information.
bool sdBegin(SdCardInfo &cardInfo, String &errorMessage);

// Unmount the SD card.
void sdEnd();

// Convert the low-level card type value to readable text.
const char *sdCardTypeLabel(uint8_t cardType);

// Read one directory and return items for the UI.
bool sdListDirectory(fs::FS &fs, const String &dirname, SdEntry *entries,
                     size_t maxEntries, size_t &entryCount, bool &truncated,
                     String &errorMessage);

// Read only the first part of a file for preview.
bool sdReadPreview(fs::FS &fs, const String &path, size_t maxBytes,
                   String &preview, uint64_t &fileSize, bool &isText,
                   bool &truncated, String &errorMessage);

#endif
