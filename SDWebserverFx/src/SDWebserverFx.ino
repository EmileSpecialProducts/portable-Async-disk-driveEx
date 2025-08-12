/*                                                                                                       \
   SDWebServer - Example WebServer with SD Card backend for esp8266 ESP32 Clasic/C3/C6/S2/S3                                                      \
                                                                                                                         \
   Copyright (c) 2015 Hristo Gochkov. All rights reserved.                                                               \
   This file is part of the ESP8266WebServer library for Arduino environment.                                            \
                                                                                                                         \
   This library is free software; you can redistribute it and/or                                                         \
   modify it under the terms of the GNU Lesser General Public                                                            \
   License as published by the Free Software Foundation; either                                                          \
   version 2.1 of the License, or (at your option) any later version.                                                    \
                                                                                                                         \
   This library is distributed in the hope that it will be useful,                                                       \
   but WITHOUT ANY WARRANTY; without even the implied warranty of                                                        \
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                                                     \
   Lesser General Public License for more details.                                                                       \
                                                                                                                         \
   You should have received a copy of the GNU Lesser General Public                                                      \
   License along with this library; if not, write to the Free Software                                                   \
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA                                            \
                                                                                                                         \
   Have a FAT Formatted SD Card connected to the SPI port of the ESP8266                                                 \
   The web root is the SD Card root folder                                                                               \
   File extensions with more than 3 charecters are not supported by the SD Library                                       \
   File Names longer than 8 charecters will be truncated by the SD library, so keep filenames shorter                    \
   index.htm is the default index (works on subfolders as well)                                                          \
                                                                                                                         \
   upload the contents of SdRoot to the root of the SDcard and access the editor 
                                                                                                                         \
 */

#include <Arduino.h> //  https://github.com/espressif/arduino-esp32/tree/master/cores/esp32

#if defined(ESP8266)
#include <ESP8266WiFi.h>
//#include <ESP8266WebServer.h>
#include <ESPAsyncTCP.h> //https://github.com/ESP32Async/ESPAsyncTCP
#include <ESP8266mDNS.h>
#else
#include <AsyncTCP.h> // https://github.com/ESP32Async/AsyncTCP
#include <WiFi.h>      // https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi
#include <esp_wifi.h>
#include <ESPmDNS.h>   // https://github.com/espressif/arduino-esp32/tree/master/libraries/ESPmDNS
#endif

#include <NTPClient.h>   //  https://github.com/arduino-libraries/NTPClient

#include <ArduinoOTA.h>  // ArduinoOTA by Arduino, Juraj  https://github.com/JAndrassy/ArduinoOTA
#if defined(ESP8266)
// https://github.com/tzapu/WiFiManager/issues/1530
#define WEBSERVER_H
#endif 

#define USEWIFIMANAGER

#ifdef USEWIFIMANAGER
#include <WiFiManager.h> // WifiManager by tzapu  https://github.com/tzapu/WiFiManager
#endif

#include <ESPAsyncWebServer.h> // https://github.com/ESP32Async/ESPAsyncWebServer
#include <SPI.h>
#include "SdFat.h" // https://github.com/greiman/SdFat

#include "NTP.hpp"
#if not defined(ESP8266)
#include <rom/rtc.h>
#endif

// https://github.com/tzapu/WiFiManager/issues/1530
typedef enum {
  MY_HTTP_GET = 0b00000001,
  MY_HTTP_POST = 0b00000010,
  MY_HTTP_DELETE = 0b00000100,
  MY_HTTP_PUT = 0b00001000,
  MY_HTTP_PATCH = 0b00010000,
  MY_HTTP_HEAD = 0b00100000,
  MY_HTTP_OPTIONS = 0b01000000,
  MY_HTTP_ANY = 0b01111111,
} MyWebRequestMethod;

#define DBG_OUTPUT_PORT Serial
 // USBSerial Serial Serial1 Serial2

#if defined(ESP8266)
//#include "SDFS.h" 
#warning "ESP8266 Pins You can not change them"

/*
WeMos D1 Mini pinout
Label GPIO    Input        Output Notes
RTS   RESET
A0    ADC0    Analog Input X
D0    GPIO16  no interrupt no     PWM or I2C support HIGH at boot used to wake up from deep sleep
D5    GPIO14  OK           OK     SPI (SCLK)
D6    GPIO12  OK           OK     SPI (MISO)
D7    GPIO13  OK           OK     SPI (MOSI)
D8    GPIO15  pulled toGND OK     SPI (CS) Boot fails if pulled HIGH
3V3

TX    GPIO1   TX pin       OK     HIGH at boot debug output at boot, boot fails if pulled LOW
RX    GPIO3   OK           RX pin HIGH at boot
D1    GPIO5   OK           OK     often used as SCL (I2C)
D2    GPIO4   OK           OK     often used as SDA (I2C)
D3    GPIO0   pulled up    OK     connected to FLASH button, boot fails if pulled LOW
D4    GPIO2   pulled up    OK     HIGH at boot connected to on-board LED, boot fails if pulled LOW
GND
5V
*/

#define SD_PIN_SCK 14  // D5 GPIO14 WeMos D1 Mini
#define SD_PIN_MOSI 13 // D7 GPIO13 WeMos D1 Mini
#define SD_PIN_MISO 12 // D6 GPIO12 WeMos D1 Mini
#define SD_PIN_CS 15   // D8 GPIO15 WeMos D1 Mini
#define BOOTPIN 16     // D0 GPIO16 WeMos D1 Mini

#define LED_RED_PIN 2    // D4 GPIO2 WeMos D1 Mini on-board LED
#define LED_ORANGE_PIN 5 // D1 GPIO5 WeMos D1 Mini
#define LED_GREEN_PIN 4  // D2 GPIO4 WeMos D1 Mini

#define RXD 3
#define TXD 1

#elif defined(CONFIG_IDF_TARGET_ESP32S2)
#warning "TARGET_ESP32S2"
/*
https://deguez07.medium.com/esp32-with-sd-card-modules-the-master-guide-5d391f6785d7
https://github.com/Xinyuan-LilyGO/LilyGo-esp32s2-base
https://github.com/Xinyuan-LilyGO/ESP32_S2
*/

#define SD_PIN_SCK 12
#define SD_PIN_MOSI 11
#define SD_PIN_MISO 13
#define SD_PIN_CS 10
#define BOOTPIN 0

#define LED_RED_PIN 2
#define LED_ORANGE_PIN 3
#define LED_GREEN_PIN 4

#define RXD 40
#define TXD 39

#elif defined(CONFIG_IDF_TARGET_ESP32S3)
#warning "TARGET_ESP32S3"
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitm-1.html
// https://learn.adafruit.com/adafruit-metro-esp32-s3/pinouts
// gpio.0 Boot Mode. Weak pullup during reset. (Boot Mode 0=Boot from Flash, 1=Download)
// gpio.3 JTAG Mode. Weak pull down during reset. (JTAG Config)
// gpio.45 SPI voltage. Weak pull down during reset. (SPI Voltage 0=3.3v 1=1.8v)
// gpio.46 Boot mode. Weak pull down during reset. (Enabling/Disabling ROM Messages Print During Booting)

// ESP32-S3 DEV Board / Lolin S3 https://www.wemos.cc/en/latest/s3/s3_pro.html / Lilygo T Display S3 :
// SS=10; MOSI=11; MISO=13; SCK=12
// Adafruit Feather S3 / QTPY S3 :
// SS=42; MOSI=35; MISO=37; SCK=36
// XIAO_ESP32S3 :
// SS=44; MOSI=9; MISO=8; SCK=7
// DFRobot Firebeetle S3:
// SS=10; MOSI=15; MISO=16; SCK=17

// The SPI2 (FSPI) default pins are
#define SD_PIN_SCK 12
#define SD_PIN_MOSI 11
#define SD_PIN_MISO 13
#define SD_PIN_CS 10

//#define SD_PIN_SCK 9
//#define SD_PIN_MOSI 10
//#define SD_PIN_MISO 11
//#define SD_PIN_CS 8

#define BOOTPIN 0

#define LED_RED_PIN 4
#define LED_ORANGE_PIN 5
#define LED_GREEN_PIN 6
#define LED_RGB_PIN 48 // GPIO 38

#define RXD 44
#define TXD 43

#elif defined(CONFIG_IDF_TARGET_ESP32C3)

#define SD_PIN_SCK 3
#define SD_PIN_MOSI 4
#define SD_PIN_MISO 5
#define SD_PIN_CS 10
#define BOOTPIN 9

#define LED_RED_PIN 2
#define LED_ORANGE_PIN 6
#define LED_GREEN_PIN 7

#define RXD 20
#define TXD 21

#elif defined(CONFIG_IDF_TARGET_ESP32C6)
// https://github.com/wuxx/nanoESP32-C6

#define SD_PIN_SCK 21
#define SD_PIN_MOSI 19
#define SD_PIN_MISO 20
#define SD_PIN_CS 18

//#define SD_PIN_SCK 19
//#define SD_PIN_MOSI 18
//#define SD_PIN_MISO 20
//#define SD_PIN_CS 23

#define PIN_NEOPIXEL 8
#define BOOTPIN 9

#define LED_RED_PIN 4
#define LED_ORANGE_PIN 5
#define LED_GREEN_PIN 6

#define RXD 17
#define TXD 16

#elif defined(CONFIG_IDF_TARGET_ESP32)
/****************************************************************************
    pin setup
    Left
    1  - 3.3 V                                        1
    2  - EN - RESET                                   2
    3  - GPIO36 - ADC0  - INPUT ONLY                  3
    4  - GPIO39 - ADC3  - INPUT ONLY                  4
    5  - GPIO34 - ADC6  - INPUT ONLY                  5
    6  - GPIO35 - ADC7  - INPUT ONLY                  6
    7  - GPIO32 - ADC4  - TOUCH9                      7
    8  - GPIO33 - ADC5  - TOUCH8                      8
    9  - GPIO25 - ADC18 - DAC1                        9
    10 - GPIO26 - ADC19 - DAC2                        10
    11 - GPIO27 - ADC17 - TOUCH7                      11
    12 - GPIO14 - ADC16 - TOUCH6 - SPICLK             12
    13 - GPIO12 - ADC15 - TOUCH5 - SPIMISO            13
    14 - GND                                          14 - GND
    15 - GPIO13 - ADC14 - TOUCH4 - SPIMOSI            15
    16 - GPIO9  - RX1 - Do not USE Connected to Flash 16
    17 - GPIO10 - TX1 - Do not USE Connected to Flash 17
    18 - GPIO11       - Do not USE Connected to Flash 18
    19 - Vin                                          19

    1  - GND                                          1  - GND
    2  - GPIO23 - V SPIMOSI                           2  - MOSI
    3  - GPIO22 - I2C SCL                             3
    4  - TX0    - GPIO1                               4  - USB-TX
    5  - RX0    - GPIO3                               5  - USB-RX
    6  - GPIO21 - I2CSDA                              6
    7  - GND                                          7  - GND
    8  - GPIO19 - V SPIMISO                           8  - MISO
    9  - GPIO18 - V SPISCK                            9  - SCK
    10 - GPIO5  - V SPI SS                            10 - CSN
    11 - GPIO17 - TX2                                 11
    12 - GPIO16 - RX2                                 12
    13 - GPIO4  - ADC10 - TOUCH0                      13 -
    14 - GPIO0  - ADC11 - TOUCH1 - BOOT               14
    15 - GPIO2  - ADC12 - TOUCH2 - OnBoardLed         15 -- LEDPIN
    16 - GPIO15 - ADC13 - TOUCH3 - SPISS              16
    17 - GPIO8  - Do not USE Connected to Flash       17
    18 - GPIO7  - Do not USE Connected to Flash       18
    19 - GPIO6  - Do not USE Connected to Flash       19

    GPIO2 = Debug led on Board LOW = OFF        HIGH = on
*/

#define SD_PIN_SCK 18
#define SD_PIN_MOSI 23
#define SD_PIN_MISO 19
#define SD_PIN_CS 5
#define BOOTPIN 0

#define LED_RED_PIN 15
#define LED_ORANGE_PIN 2
#define LED_GREEN_PIN 32

#define RXD 3
#define TXD 1

#endif

#define USEWIFIMANAGER
#ifndef USEWIFIMANAGER
const char *ssid = "SSID_ROUTER";
const char *password = "Password_Router";
#endif

#if defined(ESP8266)
const char *host = "ESP-NASEX-12E";
#elif defined(CONFIG_IDF_TARGET_ESP32)
const char *host = "ESP-NASEX-ESP32"; // CONFIG_IDF_TARGET=ESP32
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
const char *host = "ESP-NASEX-C3";
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
const char *host = "ESP-NASEX-C6";
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
const char *host = "ESP-NASEX-S2";
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
const char *host = "ESP-NASEX-S3";
#endif

AsyncWebServer server(80); 

#if not defined(ESP8266) && not defined(CONFIG_IDF_TARGET_ESP32)
SPIClass *sd_spi = NULL;
#endif

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

unsigned long PreviousTimeSeconds;
unsigned long PreviousTimeMinutes;
unsigned long PreviousTimeHours;
unsigned long PreviousTimeDay;
uint16_t Config_Reset_Counter = 0;
int OTAUploadBusy = 0;
static bool hasSD = false;
int SDmaxSpeed = 0;

//------------------------------------------------------------------------------
// This example was designed for exFAT but will support FAT16/FAT32.
// Note: Uno will not support SD_FAT_TYPE = 3.
// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
//------------------------------------------------------------------------------

#define SD_FAT_TYPE 3

#if SD_FAT_TYPE == 0
typedef SdFat sd_t;
typedef File file_t;
#elif SD_FAT_TYPE == 1
typedef SdFat32 sd_t;
typedef File32 file_t;
#elif SD_FAT_TYPE == 2
typedef SdExFat sd_t;
typedef ExFile file_t;
#elif SD_FAT_TYPE == 3
typedef SdFs sd_t;
typedef FsFile file_t;
#else // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif // SD_FAT_TYPE

#define MAX_FILENAME_SIZE 256
#define MAX_PATHNAME_SIZE (MAX_FILENAME_SIZE * 3)

// Try to select the best SD card configuration.
#if ENABLE_DEDICATED_SPI
#define SD_SPI_TYPE DEDICATED_SPI
#else
#define SD_SPI_TYPE SHARED_SPI
#endif

const uint8_t Index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
  <head>
    <title>LittleFS Async Editor</title>
    <script src="https://emilespecialproducts.github.io/ESP-SD-Async-FatEx-Web-Server/editor.js" type="text/javascript"></script> 
  </head>
  <body onload="onBodyLoad();">
    <div id="uploader"></div>
    <div id="tree" class="css-tree"></div>
    <div id="editor"></div>
    <div id="preview" style="display:none;"></div>
    <iframe id=download-frame style='display:none;'></iframe>
  </body>
</html>
)rawliteral";


const uint8_t error404_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
  <head>
    <title>Async Web Server </title>
  </head>
  <body>
    Page not found
  </body>
</html>
)rawliteral";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

sd_t sd;



void reply(AsyncWebServerRequest *request, int code, const char *type, const uint8_t *data, size_t len)
{
    DBG_OUTPUT_PORT.printf("reply Len = %d code = %d Type= %s\n ", len, code, type);  
    //request->send(code, type, data, len);
        AsyncWebServerResponse *response =
            request->beginResponse(code, type, data, len);
        // response->addHeader("Content-Encoding", "gzip");
        // response->addHeader("Content-Encoding", "7zip");
        request->send(response);
}

void Log(String Str)
{
  if (sd.exists("/Log.txt"))
  {
    String message = "";
    file_t LogFile = sd.open("/Log.txt", O_RDWR  | O_APPEND);
    message += String(getFormattedDateTime(timeClient.getEpochTime())) + ",";
    message += Str;
    LogFile.println(message);
    LogFile.close();
  }
}

#if not defined(ESP8266)
String reset_reason(int reason)
{
  switch (reason)
  {
  case 1:
    return ("Vbat power on reset");
    break;
  case 3:
    return ("Software reset digital core");
    break;
  case 4:
    return ("Legacy watch dog reset digital core");
    break;
  case 5:
    return ("Deep Sleep reset digital core");
    break;
  case 6:
    return ("Reset by SLC module, reset digital core");
    break;
  case 7:
    return ("Timer Group0 Watch dog reset digital core");
    break;
  case 8:
    return ("Timer Group1 Watch dog reset digital core");
    break;
  case 9:
    return ("RTC Watch dog Reset digital core");
    break;
  case 10:
    return ("Instrusion tested to reset CPU");
    break;
  case 11:
    return ("Time Group reset CPU");
    break;
  case 12:
    return ("Software reset CPU");
    break;
  case 13:
    return ("RTC Watch dog Reset CPU");
    break;
  case 14:
    return ("for APP CPU, reseted by PRO CPU");
    break;
  case 15:
    return ("Reset when the vdd voltage is not stable");
    break;
  case 16:
    return ("RTC Watch dog reset digital core and rtc module");
    break;
  default:
    return ("NO_MEAN");
  }
  return ("NO_MEAN");
}
#endif


void setup(void)
{
  pinMode(BOOTPIN, INPUT_PULLUP);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(SD_PIN_CS, OUTPUT);
  digitalWrite(SD_PIN_CS, HIGH);

#if   ARDUINO_USB_CDC_ON_BOOT || defined(ESP8266) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32)
  DBG_OUTPUT_PORT.begin(115200);
#else
  DBG_OUTPUT_PORT.begin(115200, SERIAL_8N1, RXD, TXD);
#endif
  DBG_OUTPUT_PORT.setDebugOutput(true);
  DBG_OUTPUT_PORT.print("\n");
#ifdef USEWIFIMANAGER
  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  //  wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect(DeviceName); // anonymous ap
  // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
  if (!res)
  {
    DBG_OUTPUT_PORT.println("Failed to connect Restarting");
    delay(5000);
    if (digitalRead(BOOTPIN) == LOW)
    {
      wm.resetSettings();
    }
    ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
  }
#else
  WiFi.begin(ssid, password);
  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(ssid);
  // Wait for connection
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20)
  { // wait 10 seconds
    delay(500);
  }
  if (i == 21)
  {
    DBG_OUTPUT_PORT.print("Could not connect to");
    DBG_OUTPUT_PORT.println(ssid);
    while (1)
      delay(500);
  }
#endif
  WiFi.setSleep(false);

#if not defined(ESP8266)
  esp_wifi_set_ps(WIFI_PS_NONE); // Esp32 enters the power saving mode by default,
#endif

  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());
  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(WiFi.SSID());

  if (MDNS.begin(host))
  {
    MDNS.addService("http", "tcp", 80);
    DBG_OUTPUT_PORT.println("MDNS responder started");
    DBG_OUTPUT_PORT.print("You can now connect to http://");
    DBG_OUTPUT_PORT.print(host);
    DBG_OUTPUT_PORT.print(".local or http://");
    DBG_OUTPUT_PORT.println(WiFi.localIP());
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(host);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    OTAUploadBusy=20; // only do a update for 20 sec;
    DBG_OUTPUT_PORT.println("Start updating " + type); });
  ArduinoOTA.onEnd([]()
                   {
      OTAUploadBusy=0;
      DBG_OUTPUT_PORT.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { DBG_OUTPUT_PORT.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
    OTAUploadBusy=0;
    DBG_OUTPUT_PORT.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      DBG_OUTPUT_PORT.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      DBG_OUTPUT_PORT.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      DBG_OUTPUT_PORT.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      DBG_OUTPUT_PORT.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      DBG_OUTPUT_PORT.println("End Failed");
    } });
  ArduinoOTA.begin();

  
server.on("/list", MY_HTTP_GET, [](AsyncWebServerRequest *request)
    {
        if (!request->hasArg("dir"))
          return request->send(500, "text/plain", "BAD ARGS\r\n"); 
        String path = request->arg("dir");
        String output="[";
        if(path != "/" && !sd.exists(path)) return request->send(500, "text/plain", "BAD PATH\r\n"); 
         file_t dir = sd.open(path);
        path = String();
        if (!dir.isDirectory())
        {
          dir.close();
          return request->send(500, "text/plain", "NOT DIR\r\n"); 
        }
        dir.rewindDirectory();
        char filename[MAX_FILENAME_SIZE];
        for (int cnt = 0; true; ++cnt)
        {  
           file_t entry = dir.openNextFile();
          if (!entry)
            break;          
          output += String(cnt++>0?",":"") +"{\"type\":\"" + String((entry.isDirectory()) ? "dir" : "file")+"\"";
          entry.getName(filename, sizeof(filename));
          output += ",\"name\":\"" + String(filename)+"\"" 
          + String((entry.isDirectory()) ?"":",\"size\":\"" + String(entry.size())+"\"") 
          + "}";
          entry.close();
        }
        output+="]";
        request->send(200, "text/json", output);
        dir.close();
      });
      
server.on("/edit", MY_HTTP_PUT, [](AsyncWebServerRequest *request)
{
  DBG_OUTPUT_PORT.println("Create ");
  if (request->args() == 0)
  return request->send(500, "text/plain", "BAD ARGS\r\n");  
  String path = request->arg(0);
  DBG_OUTPUT_PORT.println("Create: " + path);
  if (path == "/" || sd.exists(path))
  {
    request->send(500, "text/plain", "BAD PATH\r\n"+ path); 
    return;
  }
  if(path.indexOf('.') > 0){
    DBG_OUTPUT_PORT.println("CreateFile: " + path);
     file_t file = sd.open(path, O_RDWR  | O_CREAT);
    if (file)
    {
      file.write((const uint8_t *)" ", 1); // must write one char
      file.close();
    }
  }
  else
  {
    DBG_OUTPUT_PORT.println("CreateDir: " + path);
    sd.mkdir(path);
  }
  request->send(200, "text/plain", ""); 
});

server.on("/edit", MY_HTTP_DELETE, [](AsyncWebServerRequest *request)
{
  DBG_OUTPUT_PORT.println("Delete ");
  if (request->args() == 0)
    return request->send(500, "text/plain", "BAD ARGS\r\n");  
  String path = request->arg(0);
  DBG_OUTPUT_PORT.println("Delete: " + path);
  if(path.indexOf('.') > 0){
    if (path == "/" || !sd.exists(path))
    {
      request->send(500, "text/plain", "BAD PATH\r\n" + path); 
      return;
    }
    DBG_OUTPUT_PORT.println("Delete file "+path);
    
    sd.remove(path);
    request->send(200, "text/plain", "");
  } else
  {
    DBG_OUTPUT_PORT.println("Delete Dir "+path);
    sd.rmdir(path);
    request->send(200, "text/plain", "");
  } 
});

server.on("/edit", MY_HTTP_POST, 
        [](AsyncWebServerRequest *request)
          {
            DBG_OUTPUT_PORT.println("File upload completed " + request->url());
            //request->send(200, "text/plain; chartset=\"UTF-8\"", "File upload completed");
            //request->send(200);
            request->redirect("/"); 
          }, 
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
          {
            typedef struct Uploadf {int count;file_t uploadFile;} Uploadf;
             if (!index) {
              request->_tempObject = malloc(sizeof(Uploadf));
              ((Uploadf *)(request->_tempObject))->count=1 ;
              ((Uploadf *)(request->_tempObject))->uploadFile =sd.open("/"+ filename, O_RDWR  | O_CREAT);
            }
            if (len) {
              ((Uploadf *)(request->_tempObject))->count++;
              //DBG_OUTPUT_PORT.printf("%x Count\n",((Uploadf *)(request->_tempObject))->count);
              ((Uploadf *)(request->_tempObject))->uploadFile.write(data, len); 
            }
            if (final) {
              ((Uploadf *)(request->_tempObject))->uploadFile.close();
              //DBG_OUTPUT_PORT.printf("final Uploaded %s in %d blocks ",filename.c_str(),((Uploadf *)(request->_tempObject))->count);
              free(request->_tempObject); // this free is also done in the AsyncWebServerRequest destructor
              request->_tempObject=NULL;
             }
          }
    );
    server.on("/", MY_HTTP_GET, [](AsyncWebServerRequest *request)
    { request->redirect("/index.html"); });
    
    server.onNotFound([](AsyncWebServerRequest *request)
    { 
      struct DownLoadFile {file_t File; size_t index;unsigned long time; };
      static DownLoadFile DownLoadFiles[5];       
      DBG_OUTPUT_PORT.printf("url NotFound %s , Method =%s\n", request->url().c_str(), request->methodToString());
      if (request->method() == HTTP_GET)
      {
          if (sd.exists(request->url())) 
          {
            String dataType = "text/plain";
            String url= request->url();
            url.toLowerCase();
                 if (url.endsWith(".htm"))   dataType = "text/html";
            else if (url.endsWith(".html"))  dataType = "text/html";
            else if (url.endsWith(".css"))   dataType = "text/css";
            else if (url.endsWith(".js"))    dataType = "application/javascript";
            else if (url.endsWith(".png"))   dataType = "image/png";
            else if (url.endsWith(".gif"))   dataType = "image/gif";
            else if (url.endsWith(".jpg"))   dataType = "image/jpeg";
            else if (url.endsWith(".bmp"))   dataType = "image/bmp";
            else if (url.endsWith(".ico"))   dataType = "image/x-icon";
            else if (url.endsWith(".xml"))   dataType = "text/xml";
            else if (url.endsWith(".pdf"))   dataType = "application/pdf";
            else if (url.endsWith(".zip"))   dataType = "application/zip";

            // clean up is time out
            for( int f=0;f<(int )(sizeof(DownLoadFiles)/sizeof(DownLoadFiles[0]));f++)
            {
                if( DownLoadFiles[f].File && DownLoadFiles[f].File.isOpen() && DownLoadFiles[f].time < millis() )
                {   DownLoadFiles[f].File.close(); 
                    DownLoadFiles[f].index=0;
                    //DBG_OUTPUT_PORT.printf("clean up %d timeout\n",f);
                }  
            }

            for( int f=0;f<(int )(sizeof(DownLoadFiles)/sizeof(DownLoadFiles[0]));f++)
              {  
                if(!DownLoadFiles[f].File.isOpen())
                {
                  DownLoadFiles[f].File=sd.open(request->url());
                  DownLoadFiles[f].index=0;
                  //DBG_OUTPUT_PORT.printf("fileindex %d Created\n",f);
                  break;
                }
              }
        
            AsyncWebServerResponse *response = request->beginChunkedResponse(dataType, [](uint8_t *buffer, size_t maxLen, size_t index)-> size_t
            {
              int len =0;
              int fileindex=-1;
              for( int f=0;f<(int )(sizeof(DownLoadFiles)/sizeof(DownLoadFiles[0]));f++)
              {
                if(DownLoadFiles[f].File && DownLoadFiles[f].File.isOpen() && DownLoadFiles[f].index==index )
                {
                  fileindex=f;
                  //DBG_OUTPUT_PORT.printf("fileindex %d found\n",f);
                  break;
                }  
              }
              if( fileindex ==-1)
              { 
                //DBG_OUTPUT_PORT.printf("fileindex not found index = %d \n",index);
              } 
              else
              { 
              
              len=DownLoadFiles[fileindex].File.read(buffer,maxLen);
              DownLoadFiles[fileindex].time= millis()+3000; // add 3000 ms for the time out of the connection.
              DownLoadFiles[fileindex].index=index+len;
              if( len == 0)  {
                              DownLoadFiles[fileindex].File.close();
                              DownLoadFiles[fileindex].index=0;
                              //DBG_OUTPUT_PORT.printf("fileindex %d Closed\n",fileindex);
                              } 
              }
              return len;
            });
            request->send(response);
            }
            else
            {
            if (request->url() == "/index.html")
                    reply(request, 200, "text/html", Index_html, sizeof(Index_html) - 1);
            else if (request->url() == "/error404.html")
                    reply(request, 404, "text/html", error404_html, sizeof(error404_html) - 1);
            else 
                request->redirect("/error404.html");
            }
        }
        else if (request->method() == MY_HTTP_POST)
        {
            reply(request, 404, "text/html", error404_html, sizeof(error404_html) - 1);
        }
    });
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*"); 
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, PUT, POST, DELETE, HEAD");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "content-type");
  server.begin();  
  DBG_OUTPUT_PORT.println("HTTP server started");

#if defined(ESP8266) || defined(CONFIG_IDF_TARGET_ESP32)
  SDmaxSpeed = 16;
  hasSD = false;
  while (hasSD == false && SDmaxSpeed > 8)
  {
    if (sd.begin(SD_PIN_CS, 1000000UL * SDmaxSpeed ))
    {
      DBG_OUTPUT_PORT.println("SD Card initialized.");
      hasSD = true;
    }
    else
      SDmaxSpeed -= 4; // Reduce the speed
  }
  if (hasSD == false)
  {
    DBG_OUTPUT_PORT.println("SD Card initialized failed.");
  }
#else

#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
  sd_spi = new SPIClass(FSPI);
#else
  sd_spi = new SPIClass(); // HSPI
#endif

  sd_spi->begin(SD_PIN_SCK, SD_PIN_MISO, SD_PIN_MOSI, SD_PIN_CS);

  SDmaxSpeed = 50;
  hasSD = false;
  while (hasSD == false && SDmaxSpeed > 8)
  {
    if (sd.begin(SdSpiConfig(SD_PIN_CS, SD_SPI_TYPE, 1000000UL * SDmaxSpeed, sd_spi)))
    {
      DBG_OUTPUT_PORT.println("SD Card initialized.");
      hasSD = true;
    }
    else
      SDmaxSpeed -= 4; // Reduce the speed
  }
  if (hasSD == false)
  {
    DBG_OUTPUT_PORT.println("SD Card initialized failed.");
  }
#endif

  timeClient.begin();
  timeClient.setUpdateInterval(1000 * 60 * 60 * 24); // 24 uur
  timeClient.update();

  String message = "Reboot from: ";
#if defined(ESP8266)
  message += ESP.getChipId();
#else
  message += ESP.getChipModel();
#endif
  message += "_";
  message += WiFi.macAddress();
  message += " LocalIpAddres: " + WiFi.localIP().toString();
  message += " SSID: " + String(WiFi.SSID());
  message += " Rssi: " + String(WiFi.RSSI());

#if not defined(ESP8266)
  message += " Total heap: " + String(ESP.getHeapSize() / 1024);
  message += " Free heap: " + String(ESP.getFreeHeap() / 1024);
  message += " Total PSRAM: " + String(ESP.getPsramSize() / 1024);
  message += " Free PSRAM: " + String(ESP.getFreePsram() / 1024);
  message += " Temperature: " + String(temperatureRead()) + " °C "; // internal TemperatureSensor
#else
  message += " FlashChipId: " + String(ESP.getFlashChipId());
  message += " FlashChipRealSize: " + String(ESP.getFlashChipRealSize());
#endif
  message += " FlashChipSize: " + String(ESP.getFlashChipSize());
  message += " FlashChipSpeed: " + String(ESP.getFlashChipSpeed());
#if not defined(ESP8266)
#if ESP_ARDUINO_VERSION != ESP_ARDUINO_VERSION_VAL(2, 0, 17)
  // [ESP::getFlashChipMode crashes on ESP32S3 boards](https://github.com/espressif/arduino-esp32/issues/9816) 
  message += " FlashChipMode: ";
  switch( ESP.getFlashChipMode()){
    case FM_QIO :message += "FM_QIO"; break;
    case FM_QOUT:message += "FM_QOUT";break;
    case FM_DIO :message += "FM_DIO"; break;
    case FM_DOUT:message += "FM_DOUT";break;
    case FM_FAST_READ:message += "FM_FAST_READ";break;
    case FM_SLOW_READ:message += "FM_SLOW_READ";break;
    default:
      message += String(ESP.getFlashChipMode());
  }
#endif 
#endif
  // message += " CardSize: " + String(sd.card(). / 1000 / 1000) + "MB ";
  message += " CardSpeed: " + String(SDmaxSpeed);
  {
    cid_t cid;
    csd_t csd;
    scr_t scr;
    uint32_t ocr;
    if (!sd.card()->readCID(&cid) || !sd.card()->readCSD(&csd) ||
        !sd.card()->readOCR(&ocr) || !sd.card()->readSCR(&scr))
    {
      message += F(" readSDInfo failed\n");
    }
    else
    {
      message += F(" SDType=");
      switch (sd.card()->type())
      {
      case SD_CARD_TYPE_SD1:
        message += "SD1";
        break;
      case SD_CARD_TYPE_SD2:
        message += "SD2";
        break;
      case SD_CARD_TYPE_SDHC:
        if (csd.capacity() < 70000000)
        {
          message += F("SDHC");
        }
        else
        {
          message += F("SDXC");
        }
        break;
      default:
        message += F("Unknown");
      }
      if (sd.fatType() <= 32)
      {
        message += " FAT" + String(sd.fatType());
      }
      else
      {
        message += F(" exFAT");
      }
      message += " CardSize: " + String(0.000512 * csd.capacity());
    }
  }
#if not defined(ESP8266)
  message += " " + reset_reason(rtc_get_reset_reason(0));  
  message += " esp_idf_version: " + String(esp_get_idf_version());
  message += " arduino_version: " + String(ESP_ARDUINO_VERSION_MAJOR) + "." + String(ESP_ARDUINO_VERSION_MINOR) + "." + String(ESP_ARDUINO_VERSION_PATCH);
#endif
  message += " Build Date: " + String(__DATE__ " " __TIME__);
  DBG_OUTPUT_PORT.print(message);
  Log(message);
}

void loop(void)
{
  unsigned long currentTimeSeconds = timeClient.getEpochTime();
  yield();
  ArduinoOTA.handle();
  if (OTAUploadBusy == 0)
  { // Do not do things that take time when OTA is busy
   
  }

  if (PreviousTimeDay != (currentTimeSeconds / (60 * 60 * 24)))
  { // Day Loop
    PreviousTimeDay = (currentTimeSeconds / (60 * 60 * 24));
    timeClient.update();
  }
  if (PreviousTimeHours != (currentTimeSeconds / (60 * 60)))
  { // Hours Loop
    PreviousTimeHours = (currentTimeSeconds / (60 * 60));
  }

  if (PreviousTimeMinutes != (currentTimeSeconds / 60))
  { // Minutes loop
    PreviousTimeMinutes = (currentTimeSeconds / 60);
    if ((WiFi.status() != WL_CONNECTED))
    { // if WiFi is down, try reconnecting
      WiFi.disconnect();
      WiFi.reconnect();
    }
  }

  if (PreviousTimeSeconds != currentTimeSeconds)
  { // 1 second loop
    PreviousTimeSeconds = currentTimeSeconds;
    if (digitalRead(LED_RED_PIN) == 0)
      digitalWrite(LED_RED_PIN, 1);
    else
      digitalWrite(LED_RED_PIN, 0);
    if (OTAUploadBusy > 0)
      OTAUploadBusy--;
#ifdef USEWIFIMANAGER
    if (digitalRead(BOOTPIN) == LOW)
    {
      if (++Config_Reset_Counter > 5)
      {                 // press the BOOT 5 sec to reset the WifiManager Settings
        WiFiManager wm; // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
        delay(500);
        wm.resetSettings();
        ESP.restart();
      }
    }
    else
    {
      Config_Reset_Counter = 0;
    }
#endif
  }
}
