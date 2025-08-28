// testhumidespssl.h
#ifndef TESTHUMIDESPSSL_H
#define TESTHUMIDESPSSL_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServerSecure.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "DHT.h"
// Uncomment one of the lines below for whatever DHT sensor type you're using!
// #define DHTTYPE DHT11   // DHT 11
// #define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321

// Global declaration of server and cache
extern BearSSL::ESP8266WebServerSecure server;
extern BearSSL::ServerSessions serverCache;

// DHT Sensor
extern uint8_t DHTPin;

// Initialize DHT sensor.
extern DHT dht;

// Global constants including file names and locations
extern const char *WIFISECRETSFILENAME;
extern const char *CONFIGFILENAME;
extern const char *CERTIFICATE;
extern const char *CERTPRIVKEY;
extern const char *TEMPLATEFILENAME;

// Global key declarations for JSON
extern const char *LOCKEY;
extern const char *TEMPADJUSTKEY;
extern const char *HUMIDADJUSTKEY;

// Globals used across functions
extern String RespLocation;
extern float Temperature;
extern float Temptweak;
extern float RespTempC;
extern float RespTempF;
extern float Humidity;
extern float Humidtweak;
extern float RespHumid;
extern File fsUploadFile;
extern String HTMLTemplate;

// Helper functions

extern bool isAuthorized(String f);
extern bool isViewAuthorized(String f);
extern float tempCtoF(float c);
extern String getFile(const String &fileName);
extern JsonDocument getConfig(const String &configFileName);
extern String templateProcessor(String var);
extern void getDHT22();
extern String getContentType(String filename);
extern void replyOK();
extern void handleRoot();
extern void handleJSON();
extern bool handleFileRead(String path);
extern void handleFileUpload();

#endif // TESTHUMIDESPSSL_H