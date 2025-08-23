#include <ESP8266WiFi.h>
#include <ESP8266WebServerSecure.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "DHT.h"

// Uncomment one of the lines below for whatever DHT sensor type you're using!
// #define DHTTYPE DHT11   // DHT 11
// #define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321

//Global declaration of server and cache
BearSSL::ESP8266WebServerSecure server(443);
BearSSL::ServerSessions serverCache(5);

// DHT Sensor
uint8_t DHTPin = D2;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

//Global constants including file names and locations
const char *WIFISECRETSFILENAME = "/wifi.json";
const char *CONFIGFILENAME = "/config.json";
const char *CERTIFICATE = "/fullchain.pem";
const char *CERTPRIVKEY = "/privkey.pem";
const char *TEMPLATEFILENAME = "/temphumid.html";

//Global key declarations for JSON
const char *LOCKEY = "location";
const char *TEMPADJUSTKEY = "temp_adjust";
const char *HUMIDADJUSTKEY = "humidity_adjust";

//Globals used across functions
String RespLocation = "Unknown Location";
float Temperature = 0.0;
float Temptweak = 0.0;
float RespTempC = 0.0;
float RespTempF = 0.0;
float Humidity = 0.0;
float Humidtweak = 0.0;
float RespHumid = 0.0;
File fsUploadFile;
String HTMLTemplate;

// Helper functions

bool isAuthorized(String f) {
  if (f.endsWith(".pem"))
    return true;
  if (f.endsWith(".html"))
    return true;
  if (f.endsWith(".json"))
    return true;
  if (f.endsWith(".css"))
    return true;

  return false;
}

bool isViewAuthorized(String f) {
  if (f.endsWith("fullchain.pem"))
    return true;
  if (f.endsWith("config.json"))
    return true;
  if (f.endsWith(".html"))
    return true;
  if (f.endsWith(".css"))
    return true;

  return false;
}

//Convert Celcius to Farenheit
float tempCtoF(float c)
{
  return ((c * (9.0 / 5.0)) + 32.0);
}

//Read in a file
String getFile(const String& fileName)
{
  String ret = "";
  String msg = "";
  File file = LittleFS.open(fileName, "r");
  if (!file)
  {
    msg = "Failed to open ";
    msg += fileName;
    msg += " file for reading!";
    Serial.println(msg);
    return ret;
  }

  ret = file.readString();

  file.close();

  return ret;
}

//Read in the configuration file
JsonDocument getConfig(const String &configFileName)
{
  JsonDocument ret;

  File file = LittleFS.open(configFileName, "r");
  if (!file)
  {
    Serial.println("Failed to open config file for reading");
    return ret;
  }

  DeserializationError error = deserializeJson(ret, file);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    // Handle the error
  }
  file.close();

  return ret;
}

//Template processor for main web page
String templateProcessor(String var) {
    
  if (var == "LOCATION") {
   return RespLocation;
  }
  if (var == "TEMPERATUREC") {
   return String(RespTempC);
  }
  if (var == "TEMPERATUREF") {
   return String(RespTempF);
  }
  if (var == "HUMIDITY") {
   return String(RespHumid);
  }
  return emptyString;

}

//Get current readings from the DHT22
void getDHT22()
{
  Temperature = dht.readTemperature(); // Gets the values of the temperature
  Humidity = dht.readHumidity();       // Gets the values of the humidity
  // Set the updated server response values
  RespTempC = Temperature + Temptweak;
  RespTempF = tempCtoF(RespTempC);
  RespHumid = Humidity + Humidtweak;
}

//Determine file content type
String getContentType(String filename)
{ // convert the file extension to the MIME type
  if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
}

//Server Functions
void replyOK()
{
  server.send(200, "text/plain", "");
}

//HTML request handlers
void handleRoot()
{
  Serial.println("handleRoot");
  getDHT22();
  String resp = HTMLTemplate;
  resp.replace("%LOCATION%", templateProcessor("LOCATION"));
  resp.replace("%TEMPERATUREC%", templateProcessor("TEMPERATUREC"));
  resp.replace("%TEMPERATUREF%", templateProcessor("TEMPERATUREF"));
  resp.replace("%HUMIDITY%", templateProcessor("HUMIDITY"));
  server.send(200, "text/html", resp);
}

void handleJSON()
{
  Serial.println("handleJSON");
  getDHT22();
  JsonDocument ret;
  ret[LOCKEY] = RespLocation;
  ret["temp_celcius"] = RespTempC;
  ret["temp_farenheit"] = RespTempF;
  ret["temp_adjustment"] = Temptweak;
  ret["humidity_pct"] = RespHumid;
  ret["humidity_adjustment"] = Humidtweak;
  String resp;
  serializeJson(ret, resp);
  server.send(200, "application/json", resp);
}

bool handleFileRead(String path)
{ 
  // send the right file to the client (if it exists)
  if (isViewAuthorized(path)) {
    Serial.println("handleFileRead: " + path);
    String contentType = getContentType(path); // Get the MIME type
    if (LittleFS.exists(path))
    {                                                     // If the file exists, either as a compressed archive, or normal
      String fileContents = getFile(path);                 // Open the file
      server.send(200, contentType, fileContents);
      Serial.println(String("\tSent file: ") + path);
      return true;
    }
    Serial.println(String("\tFile Not Found: ") + path); // If the file doesn't exist, return false
    return false;
  }
  else
  {
    server.send(501, "text/plain", "File cannot be sent");
    return false;
  }
}

void handleFileUpload()
{ 
  Serial.println("Starting upload...");

  HTTPUpload &upload = server.upload();
  if (isAuthorized(upload.filename)) {
    if (upload.status == UPLOAD_FILE_START)
    {
      String filename = upload.filename;
      if (!filename.startsWith("/"))
        filename = "/" + filename;
      Serial.print("handleFileUpload Name: ");
      Serial.println(filename);
      fsUploadFile = LittleFS.open(filename, "w"); // Open the file for writing in LittleFS (create if it doesn't exist)
      filename = String();
      return;
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
      Serial.println("Status UPLOAD_FILE_WRITE");
      if (fsUploadFile)
        fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      Serial.println("Status UPLOAD_FILE_END");
      if (fsUploadFile)
      { // If the file was successfully created
        fsUploadFile.close(); // Close the file again
        Serial.print("handleFileUpload Size: ");
        Serial.println(upload.totalSize);
        server.sendHeader("Location", "/success.html", true); // Redirect the client to the success page
        server.send(303);
      }
      else
      {
        server.send(500, "text/plain", "500: couldn't create file");
      }
    }
  }
  else {
    server.send(501, "text/plain", "File cannot be uploaded");
  }
}

/////////////////////
// Setup
/////////////////////
void setup()
{
  Serial.begin(115200);
  delay(10000);

  if (!LittleFS.begin())
  {
    Serial.println("Failed to mount file system");
    return;
  }
  //Set up Wifi connection info from the config file
  JsonDocument wifi = getConfig(WIFISECRETSFILENAME);
  const char *ssid = wifi["SSID"].as<const char *>();
  const char *password = wifi["WiFi_Password"].as<const char *>();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  JsonDocument config = getConfig(CONFIGFILENAME);
  if (config[LOCKEY].is<JsonVariant>())
    RespLocation = config[LOCKEY].as<String>();
  if (config[TEMPADJUSTKEY].is<JsonVariant>())
    Temptweak = config[TEMPADJUSTKEY].as<float>();
  if (config[HUMIDADJUSTKEY].is<JsonVariant>())
    Humidtweak = config[HUMIDADJUSTKEY].as<float>();

  pinMode(DHTPin, INPUT);

  dht.begin();

  //Load Certificates
  String certificate = getFile(CERTIFICATE);
  String privkey     = getFile(CERTPRIVKEY);

  //Load Template
  HTMLTemplate = getFile(TEMPLATEFILENAME);

  // connect to your local wi-fi network
  WiFi.begin(ssid, password);

  // check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());

  //Set the certificate and private key
  server.getServer().setRSACert(new BearSSL::X509List(certificate.c_str()), 
                                new BearSSL::PrivateKey(privkey.c_str()));
  // Cache SSL sessions to accelerate the TLS handshake.
  server.getServer().setCache(&serverCache);

  server.on("/", handleRoot);
  server.on("/json", handleJSON);

  //Upload handling for config file replacement and update of certificates
  server.on("/upload", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/upload.html"))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on("/upload", HTTP_POST, []() {}, handleFileUpload); // Receive and save the file

  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.begin();
  Serial.println("HTTPS server started");
}

/////////////////////
// Main Loop
/////////////////////
void loop()
{

  server.handleClient();
}
