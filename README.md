# ESP8266-WemosRelay
Simple project for making an Amazon Echo/Alexa compatible IoT device with a Wemos D1 + Relay Shied.

Features
--------
* Amazon Echo/Alexa Support (fauxmoESP)
* Easy Wifi Configuration (ESPAsyncWifiManager)
* Async Web Server support (ESPAsyncWebServer)
 * Built-in Configuration Web site
 * AngularJS site with REST based API
 * Static site is stored in SPIFFS, utilizes CDNs to reduce footprint
* ArduinoOTA to support OTA updates plus loading SPIFFS
* Built using PlatformIO


3rd Party Libraries
-------------------
This project is designed to serve as a starter project for controlling a Wemos D1 + Relay Shield.  Out of the box it is configured with the following libraries
* ESPAsyncWifiManager
  * Provides an simple WiFi setup process for configuring a new device and assigning a name for within Alexa.
* fauxmoESP
  * To add simple support discovery plus On/Off commands from Alexa and other IoT platforms
* ESPAsyncTCP & ESPAsyncWebServer
  * Add support for a light-weight web server
* AruidnoJSON
 * Utilized for Configuration storage plus REST based API

SPIFFS
------
The application does utilize SPIFFS for storing configuration as well as the static files for the web site.

* Configuration is stored within /config.json
* Web application file system
 * index.html
 * main.js

Don't forget to load SPIFFS into the device.
pio run -t uploadfs

Web Server
----------
Supported URLs
* Static site (from SPIFFS)
 * index.html
 * main.js
* /relayon
* /relayoff
* /reset
 * Triggers a CPU reset
* /resetwifi
 * Reset Wifi Configuration (which will trigger setup Hotspot upon restarting)
* /heap
 * Return available HEAP
Used for the AngularJS site
* /info
 * HTTP_GET, REST based API used to retrieve configuration
* /settings
 * HTTP_POST, REST based API used to update configuration   

TODO
----
* Split source code into multiple files
* TBD
