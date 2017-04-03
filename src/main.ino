#include <fs.h>
#include <Arduino.h>
#include <hash.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>
#include <fauxmoESP.h>
#include "AsyncJson.h"
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

fauxmoESP fauxmo;
const int relayPin = D1;
DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWiFiManager wifiManager(&server, &dnsServer);
char deviceName[40];
bool shouldSaveConfig = false;

void relayOneOn();
void relayOneOff();
void setupWebServer();
void deviceReset();
void saveConfigCallback();
void LoadSettings();
void SaveSettings();
void SetupOTA();

void setup() {
  //configure the relay
  pinMode(relayPin, OUTPUT);

  //configure the serial port
  Serial.begin(115200);

  LoadSettings();

  //Setup WiFiManager and get us connected to wifi
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  // id/name, placeholder/prompt, default, length
  AsyncWiFiManagerParameter custom_devicename("DeviceName", "Device Name", deviceName, 40);
  wifiManager.addParameter(&custom_devicename);
  wifiManager.setTimeout(180);
  if(!wifiManager.autoConnect("AutoConnectAP")) {
    Serial.println("failed to connect and hit timeout");
    deviceReset();
  }
  Serial.println("Wifi Connection Successful");

  //save settings if we need to
  if (shouldSaveConfig)
  {
    strcpy(deviceName, custom_devicename.getValue());
    SaveSettings();
  }

  //web server reqeusts
  setupWebServer();

  //start the web server
  server.begin();

  //startup the device
  fauxmo.addDevice(deviceName);
  fauxmo.onMessage([](unsigned char device_id, const char * device_name, bool state) {
      //Serial.printf("[MAIN] Device #%d (%s) state: %s\n", device_id, device_name, state ? "ON" : "OFF");
      //Only one device, so we only care about state
      if (state)
      {
        relayOneOn();
      }
      else
      {
        relayOneOff();
      }
  });

  SetupOTA();

  //print out a directory list
  // Dir dir = SPIFFS.openDir("/");
  // while (dir.next()) {
  //     Serial.print(dir.fileName());
  //     File f = dir.openFile("r");
  //     Serial.println(f.size());
  // }
}

void loop() {
  fauxmo.handle();
  ArduinoOTA.handle();
}

void SetupOTA()
{
  ArduinoOTA.onStart([]() {
    // String type;
    // if (ArduinoOTA.getCommand() == U_FLASH)
    //   type = "sketch";
    // else // U_SPIFFS
    //   type = "filesystem";
    //
    // // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    // Serial.println("Start updating " + type);
    Serial.println("Starting OTA");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void setupWebServer()
{
  // Server with different default file
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  //Used for relay control
  server.on("/relayon", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("relay on");
    digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH
    request->send(200, "text/plain", "relay on");
  });
  server.on("/relayoff", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("relay off");
    digitalWrite(relayPin, LOW); // turn on relay with voltage HIGH
    request->send(200, "text/plain", "relay off");
  });

  //used to force a reset of the device
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "resetting device");
    deviceReset();
  });

  server.on("/resetwifi", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "resetting wifi settings ...");
    wifiManager.resetSettings();
    deviceReset();
  });

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncJsonResponse * response = new AsyncJsonResponse();

    JsonObject& root = response->getRoot();
    root["heap"] = ESP.getFreeHeap();
    root["deviceName"] = deviceName;
    response->setLength();
    request->send(response);
  });

  //handles the setting update
  server.on("/settings", HTTP_POST, [](AsyncWebServerRequest *request){

    if (request->hasParam("deviceName"))
    {
      strcpy(deviceName, request->getParam("deviceName")->value().c_str());
      SaveSettings();
      request->send(200,"text/plain","");
      return;
    }

    //failed request
    request->send(500,"text/plain","Error saving changes.");
  });
}

void relayOneOn()
{
  Serial.println("Command from Alex: Turn Relay One On");
  digitalWrite(relayPin, HIGH);
}

void relayOneOff()
{
  Serial.println("Command from Alex: Turn Relay One Off");
  digitalWrite(relayPin, LOW);
}

void deviceReset()
{
  delay(3000);
  ESP.reset();
  delay(5000);
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//Used to load up settings
void LoadSettings()
{
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          if (json.containsKey("device_name"))
          {
            strcpy(deviceName, json["device_name"]);
          }

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
}

//Used to save settings
void SaveSettings()
{
  Serial.println("saving config");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["device_name"] = deviceName;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();
  //end save
}
