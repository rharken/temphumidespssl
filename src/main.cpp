#include <temphumidespssl.h>

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
  // Set up Wifi connection info from the config file
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

  // Load Certificates
  String certificate = getFile(CERTIFICATE);
  String privkey = getFile(CERTPRIVKEY);

  // Load Template
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

  // Set the certificate and private key
  server.getServer().setRSACert(new BearSSL::X509List(certificate.c_str()),
                                new BearSSL::PrivateKey(privkey.c_str()));
  // Cache SSL sessions to accelerate the TLS handshake.
  server.getServer().setCache(&serverCache);

  server.on("/", handleRoot);
  server.on("/json", handleJSON);

  // Upload handling for config file replacement and update of certificates
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
