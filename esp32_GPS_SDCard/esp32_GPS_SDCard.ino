//esp32 sd card - gps

#include "FS.h"
#include "SD.h"
#include "SPI.h"



#include <TinyGPS++.h>
static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;
HardwareSerial neogps(2);
TinyGPSPlus gps;

char gpsDataChar[138] = "";




void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char * path){
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path)){
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
      Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char * path){
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file){
    len = file.size();
    size_t flen = len;
    start = millis();
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for(i=0; i<2048; i++){
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}














void setup() {
  Serial.begin(115200);
  Serial.println("STRD");
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  
  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  listDir(SD, "/", 0);
  writeFile(SD, "/gpsData.txt", "Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum\n");
  appendFile(SD, "/gpsData.txt", "           (deg)      (deg)       Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail\n");
  appendFile(SD, "/gpsData.txt", "----------------------------------------------------------------------------------------------------------------------------------------\n");
  Serial.println(F("Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum"));
	Serial.println(F("           (deg)      (deg)       Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail"));
	Serial.println(F("----------------------------------------------------------------------------------------------------------------------------------------"));
  neogps.begin(9600, SERIAL_8N1, RXPin, TXPin);


}
































// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms) {
	unsigned long start = millis();
	do {
		while (neogps.available()) {
			gps.encode(neogps.read());
		}
	} while (millis() - start < ms);
}
/*
static void printFloat(float val, bool valid, int len, int prec) {
	if (!valid) {
		while (len-- > 1) {
			Serial.print('y');
      char tempCh[1]="";
      sprintf(tempCh, "y");
      strcat(gpsDataChar, tempCh);
		}
		Serial.print('b');
    char tempCh[1]="";
    sprintf(tempCh, "b");
    strcat(gpsDataChar, tempCh);
	} else {
		Serial.print(val, prec);
    //char t4t[32] = "";
    //sprintf(t4t, "%d", prec);
    //strcat(gpsDataChar, t4t);
    char tt[32] = "";
    sprintf(tt, "%f", val);
    strcat(gpsDataChar, tt);
		int vi = abs((int)val);
		int flen = prec + (val < 0.0 ? 2 : 1); // . and -
		flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
		for (int i=flen; i<len; ++i) {
			Serial.print('v');
      char tempCh[1]="";
      sprintf(tempCh, "v");
      strcat(gpsDataChar, tempCh);
		}
	}
	smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len) {
	char sz[32] = ".*****************.";
	if (valid) {
		sprintf(sz, "%ld", val);
	}
	sz[len] = 0;
	for (int i=strlen(sz); i<len; ++i) {
		sz[i] = ' ';
	}
	if (len > 0) {
		sz[len-1] = ' ';
	}
	Serial.print(sz);
  strcat(gpsDataChar, sz);
	smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t) {
	if (!d.isValid()) {
		Serial.print(F("p********** p"));
    strcat(gpsDataChar, "p********** p");
	} else {
		char sz[32];
		sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
		Serial.print(sz);
    strcat(gpsDataChar, sz);
	}

	if (!t.isValid()) {
		Serial.print(F("n******** n"));
    strcat(gpsDataChar, "n******** n");
	} else {
		char sz[32];
		sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
		Serial.print(sz);
    strcat(gpsDataChar, sz);
	}

	printInt(d.age(), d.isValid(), 5);
	smartDelay(0);
}

static void printStr(const char *str, int len) {
	int slen = strlen(str);
	for (int i=0; i<len; ++i) {
		Serial.print(i<slen ? str[i] : ' ');
	}
	smartDelay(0);
}

*/



static void printFloat(float val, bool valid, int len, int prec) {
	if (!valid) {
		while (len-- > 1) {
			Serial.print('*');
		}
		Serial.print(' ');
	} else {
		Serial.print(val, prec);
		int vi = abs((int)val);
		int flen = prec + (val < 0.0 ? 2 : 1); // . and -
		flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
		for (int i=flen; i<len; ++i) {
			Serial.print(' ');
		}
	}
	smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len) {
	char sz[32] = "*****************";
	if (valid) {
		sprintf(sz, "%ld", val);
	}
	sz[len] = 0;
	for (int i=strlen(sz); i<len; ++i) {
		sz[i] = ' ';
	}
	if (len > 0) {
		sz[len-1] = ' ';
	}
	Serial.print(sz);
	smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t) {
	if (!d.isValid()) {
		Serial.print(F("********** "));
	} else {
		char sz[32];
		sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
		Serial.print(sz);
	}

	if (!t.isValid()) {
		Serial.print(F("******** "));
	} else {
		char sz[32];
		sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
		Serial.print(sz);
	}

	printInt(d.age(), d.isValid(), 5);
	smartDelay(0);
}

static void printStr(const char *str, int len) {
	int slen = strlen(str);
	for (int i=0; i<len; ++i) {
		Serial.print(i<slen ? str[i] : ' ');
	}
	smartDelay(0);
}




void loop() {
  char gpsDataChar[138] = "";
  static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;

	printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
	printFloat(gps.hdop.hdop(), gps.hdop.isValid(), 6, 1);
	printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
	printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
	printInt(gps.location.age(), gps.location.isValid(), 5);
	printDateTime(gps.date, gps.time);
	printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2);
	printFloat(gps.course.deg(), gps.course.isValid(), 7, 2);
	printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);
	printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.deg()) : "*** ", 6);

 //SD Card Data
 if(gps.satellites.isValid()){
    char tt[32] = "";
    sprintf(tt, "%d", gps.satellites.value());
    strcat(gpsDataChar, tt);
    strcat(gpsDataChar, " - ");
 }else{
  strcat(gpsDataChar, "!s");
  strcat(gpsDataChar, " _ ");
 }
 if(gps.hdop.isValid()){
    char tt[32] = "";
    sprintf(tt, "%2.1f", gps.hdop.hdop());
    strcat(gpsDataChar, tt);
    strcat(gpsDataChar, " - ");
 }else{
  strcat(gpsDataChar, "!h");
  strcat(gpsDataChar, " _ ");
 }
 if(gps.location.isValid()){
  char tt[32] = "";
  sprintf(tt, "%4.6f", gps.location.lat());
  strcat(gpsDataChar, tt);
  strcat(gpsDataChar, ",");
 }else{
  strcat(gpsDataChar, "!LL");
  strcat(gpsDataChar, "_");
 }
 if(gps.location.isValid()){
  char tt[32] = "";
  sprintf(tt, "%4.6f", gps.location.lng());
  strcat(gpsDataChar, tt);
  strcat(gpsDataChar, " - ");
 }else{
  strcat(gpsDataChar, "!Ll");
  strcat(gpsDataChar, " _ ");
 }
 if(gps.location.isValid()){
    char tt[32] = "";
    sprintf(tt, "%d", gps.location.age());
    strcat(gpsDataChar, tt);
    strcat(gpsDataChar, " - ");
 }else{
  strcat(gpsDataChar, "!a");
  strcat(gpsDataChar, " _ ");
 }
 if(gps.date.isValid()){
  char sz[32];
  sprintf(sz, "%02d/%02d/%02d", gps.date.month(), gps.date.day(), gps.date.year());
  strcat(gpsDataChar, sz);
  strcat(gpsDataChar, " - ");
 }else{
  strcat(gpsDataChar, "!dd");
  strcat(gpsDataChar, " _ ");
 }
 if(gps.time.isValid()){
  char sz[32];
  sprintf(sz, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
  strcat(gpsDataChar, sz);
  strcat(gpsDataChar, " - ");
 }else{
  strcat(gpsDataChar, "!dt");
  strcat(gpsDataChar, " _ ");
 }
 if(gps.date.isValid()){
  char sz[32];
  sprintf(sz, "%d", gps.date.age());
  strcat(gpsDataChar, sz);
  strcat(gpsDataChar, " - ");
 }else{
  strcat(gpsDataChar, "!dda");
  strcat(gpsDataChar, " _ ");
 }
  if(gps.altitude.isValid()){
    char tt[32] = "";
    sprintf(tt, "%3.2f", gps.altitude.meters());
    strcat(gpsDataChar, tt);
    strcat(gpsDataChar, " - ");
 }else{
  strcat(gpsDataChar, "!alt");
  strcat(gpsDataChar, " _ ");
 }
  

	unsigned long distanceKmToLondon =
		(unsigned long)TinyGPSPlus::distanceBetween(
				gps.location.lat(),
				gps.location.lng(),
				LONDON_LAT, 
				LONDON_LON) / 1000;
	printInt(distanceKmToLondon, gps.location.isValid(), 9);

	double courseToLondon =
		TinyGPSPlus::courseTo(
				gps.location.lat(),
				gps.location.lng(),
				LONDON_LAT, 
				LONDON_LON);

	printFloat(courseToLondon, gps.location.isValid(), 7, 2);

	const char *cardinalToLondon = TinyGPSPlus::cardinal(courseToLondon);

	printStr(gps.location.isValid() ? cardinalToLondon : "*** ", 6);

	printInt(gps.charsProcessed(), true, 6);
	printInt(gps.sentencesWithFix(), true, 10);
	printInt(gps.failedChecksum(), true, 9);
	Serial.println();

  char tempCh[5]="";
  sprintf(tempCh, "\n");
  strcat(gpsDataChar, tempCh);
  appendFile(SD, "/gpsData.txt", gpsDataChar);
  Serial.println("Appended text:");
  Serial.println(gpsDataChar);

	smartDelay(1000);

	if (millis() > 5000 && gps.charsProcessed() < 10) {
		Serial.println(F("No GPS data received: check wiring"));
	}

}
