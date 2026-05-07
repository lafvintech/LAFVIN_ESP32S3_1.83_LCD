#include "sd_browser.h"

#include <SD.h>
#include <SPI.h>
#include <ctype.h>

// SPI pins used by the SD card slot on this board.
static const int SD_SPI_SCLK_PIN = 14;
static const int SD_SPI_MISO_PIN = 16;
static const int SD_SPI_MOSI_PIN = 15;
static const int SD_SPI_CS_PIN = 21;
static const uint32_t SD_SPI_FREQUENCY = 10000000U;

// Prepare the SPI bus before mounting the card.
static bool configureSdSpiPins() {
  pinMode(SD_SPI_CS_PIN, OUTPUT);
  digitalWrite(SD_SPI_CS_PIN, HIGH);
  SPI.begin(SD_SPI_SCLK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
  return true;
}

// Extract the last part of a full path.
static String sdBaseName(const String &path) {
  if (path.length() == 0 || path == "/") {
    return "/";
  }

  int slash = path.lastIndexOf('/');
  if (slash < 0) {
    return path;
  }
  return path.substring(slash + 1);
}

// Join a folder path and a child name into one full path.
static String sdJoinPath(const String &dirname, const String &name) {
  if (dirname == "/") {
    return "/" + name;
  }
  return dirname + "/" + name;
}

// Compare two names without caring about upper/lower case.
static int sdCompareNoCase(const String &left, const String &right) {
  size_t leftLen = left.length();
  size_t rightLen = right.length();
  size_t minLen = leftLen < rightLen ? leftLen : rightLen;

  for (size_t i = 0; i < minLen; ++i) {
    char a = static_cast<char>(tolower(static_cast<unsigned char>(left[i])));
    char b = static_cast<char>(tolower(static_cast<unsigned char>(right[i])));
    if (a < b) {
      return -1;
    }
    if (a > b) {
      return 1;
    }
  }

  if (leftLen < rightLen) {
    return -1;
  }
  if (leftLen > rightLen) {
    return 1;
  }
  return 0;
}

// Sort directories first, then sort names alphabetically.
static void sdSortEntries(SdEntry *entries, size_t count) {
  for (size_t i = 1; i < count; ++i) {
    SdEntry key = entries[i];
    size_t j = i;

    while (j > 0) {
      const SdEntry &candidate = entries[j - 1];
      bool shouldShift = false;

      if (key.isDirectory != candidate.isDirectory) {
        shouldShift = key.isDirectory && !candidate.isDirectory;
      } else {
        shouldShift = sdCompareNoCase(key.name, candidate.name) < 0;
      }

      if (!shouldShift) {
        break;
      }

      entries[j] = entries[j - 1];
      --j;
    }

    entries[j] = key;
  }
}

// File types that are usually safe to preview as text.
static bool sdHasTextExtension(const String &path) {
  String lower = path;
  lower.toLowerCase();
  return lower.endsWith(".txt") || lower.endsWith(".log") ||
         lower.endsWith(".csv") || lower.endsWith(".json") ||
         lower.endsWith(".ini") || lower.endsWith(".md") ||
         lower.endsWith(".yaml") || lower.endsWith(".yml") ||
         lower.endsWith(".xml");
}

// Small check to see if the first bytes look like readable text.
static bool sdBufferLooksLikeText(const uint8_t *data, size_t length) {
  if (length == 0) {
    return true;
  }

  size_t suspicious = 0;
  for (size_t i = 0; i < length; ++i) {
    uint8_t c = data[i];
    if (c == 0) {
      return false;
    }
    if ((c < 32 && c != '\n' && c != '\r' && c != '\t') || c == 127) {
      suspicious++;
    }
  }

  return suspicious < (length / 12 + 1);
}

bool sdBegin(SdCardInfo &cardInfo, String &errorMessage) {
  sdEnd();

  cardInfo.cardType = CARD_NONE;
  cardInfo.cardSizeBytes = 0;
  cardInfo.totalBytes = 0;
  cardInfo.usedBytes = 0;
  errorMessage = "";

  if (!configureSdSpiPins()) {
    errorMessage = "SD SPI pin setup failed";
    return false;
  }

  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQUENCY)) {
    errorMessage = "SD card mount failed";
    return false;
  }

  cardInfo.cardType = SD.cardType();
  if (cardInfo.cardType == CARD_NONE) {
    errorMessage = "No SD card attached";
    SD.end();
    return false;
  }

  cardInfo.cardSizeBytes = SD.cardSize();
  cardInfo.totalBytes = SD.totalBytes();
  cardInfo.usedBytes = SD.usedBytes();
  return true;
}

void sdEnd() {
  SD.end();
}

const char *sdCardTypeLabel(uint8_t cardType) {
  if (cardType == CARD_MMC) {
    return "MMC";
  }
  if (cardType == CARD_SD) {
    return "SDSC";
  }
  if (cardType == CARD_SDHC) {
    return "SDHC";
  }
  if (cardType == CARD_NONE) {
    return "NONE";
  }
  return "UNKNOWN";
}

bool sdListDirectory(fs::FS &fs, const String &dirname, SdEntry *entries,
                     size_t maxEntries, size_t &entryCount, bool &truncated,
                     String &errorMessage) {
  entryCount = 0;
  truncated = false;
  errorMessage = "";

  File root = fs.open(dirname);
  if (!root) {
    errorMessage = "Failed to open directory: " + dirname;
    return false;
  }
  if (!root.isDirectory()) {
    errorMessage = "Not a directory: " + dirname;
    return false;
  }

  File file = root.openNextFile();
  while (file) {
    if (entryCount < maxEntries) {
      String entryPath = file.path();
      String entryName = sdBaseName(entryPath);
      if (entryPath.length() == 0) {
        entryName = file.name();
        entryPath = sdJoinPath(dirname, entryName);
      }

      entries[entryCount].name = entryName;
      entries[entryCount].path = entryPath;
      entries[entryCount].isDirectory = file.isDirectory();
      entries[entryCount].size = file.isDirectory() ? 0 : file.size();
      entryCount++;
    } else {
      truncated = true;
    }

    file = root.openNextFile();
  }

  sdSortEntries(entries, entryCount);
  return true;
}

bool sdReadPreview(fs::FS &fs, const String &path, size_t maxBytes,
                   String &preview, uint64_t &fileSize, bool &isText,
                   bool &truncated, String &errorMessage) {
  preview = "";
  fileSize = 0;
  isText = false;
  truncated = false;
  errorMessage = "";

  File file = fs.open(path);
  if (!file) {
    errorMessage = "Failed to open file: " + path;
    return false;
  }
  if (file.isDirectory()) {
    errorMessage = "Cannot preview a directory";
    return false;
  }

  fileSize = file.size();

  uint8_t sample[256];
  size_t sampleLength = 0;
  while (sampleLength < sizeof(sample) && file.available()) {
    sample[sampleLength++] = static_cast<uint8_t>(file.read());
  }

  bool extensionText = sdHasTextExtension(path);
  bool looksText = extensionText || sdBufferLooksLikeText(sample, sampleLength);
  if (!looksText) {
    truncated = fileSize > 0;
    return true;
  }

  isText = true;
  truncated = fileSize > maxBytes;
  file.seek(0);
  preview.reserve(maxBytes + 32);

  size_t bytesRead = 0;
  while (file.available() && bytesRead < maxBytes) {
    char c = static_cast<char>(file.read());
    if (c == '\r' || c == '\n' || c == '\t' || static_cast<unsigned char>(c) >= 32) {
      preview += c;
    } else {
      preview += ' ';
    }
    bytesRead++;
  }

  return true;
}
