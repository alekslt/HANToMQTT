/*
 * PowerMeter HAN-port to MQTT program
 * WRT, Roar Fredriksen.
 * We are required to use the hardware serial
 * on the esp8266 as we need the even-parity support.
 * Serial1 can be used for low level debugging,
 * but I will also use a debug-mqtt topic.
 *
 * (Re)Programming should be done using OTA.
 *
 * Notes
 * 1. Using third party libraries: PubSubClient, NTPClient
 * 2. Using Serial.swap() for ESP8266 due to main TX, RX being connected to usb-serial (for NodeMCU) change if needed.
 * 3. Using UART1 on ESP8266 with pins defined below: RXD2, TXD2
 *
 * Aleksander Lygren Toppe
 * 2019.02.17
 * GPL? Public Domain? MIT?
 */


////////////////////////////////
// Headers /////////////////////
////////////////////////////////
#include <map>
//#include <set>




#if defined(ESP32)
#include <WiFi.h> // Wifi
#include <esp_task_wdt.h>
#else
#include <ESP8266WiFi.h> // Wifi
extern "C" {
#include "user_interface.h"
}
#endif


#include <EEPROM.h> // ??
#include <ArduinoOTA.h> // For OTA Programming
#include <NTPClient.h> // For NTP Wall Clock Time Syncronization
#include <WiFiClientSecure.h>
#include <WiFiUdp.h> // For NTP Wall Clock Time Syncronization

//#include <ArduinoJson.h> // json

//#define MQTT_MAX_PACKET_SIZE 512
#include <PubSubClient.h> // MQTT

#include "ObisElement.h"
#include "HanReader.h"

// Configuration - Sensitive defines/passwords/adresses
#include "secret.h"

////////////////////////////////
// Variables that you need to //
// change to make things work //
// for you. ////////////////////
////////////////////////////////
#define USE_ARDUINO_OTA
#define OTA_HOSTNAME "esp-powermeter-test2"
#define OTA_PASSWORD CONFIG_OTA_PASSWORD

// WiFi
const char* ssid = CONFIG_WIFI_SSID;
const char* password = CONFIG_WIFI_PASSWORD;

// MQTT
#define MQTT_SERVER      CONFIG_MQTT_SERVER
#define MQTT_SERVERPORT  CONFIG_MQTT_SERVERPORT                   // 8883 for MQTTS
#define MQTT_USERNAME    ""
#define MQTT_KEY         ""
#define MQTT_MYNAME      OTA_HOSTNAME" Client"

#define power_topic_base    "kdg/sensors/powermeterhan"
//#define power_topic_base    "test/powermeterhan"
#define power_topic         power_topic_base"/obis"
#define power_topic_command power_topic_base"/commands"
#define power_topic_debug   power_topic_base"/debug"
#define power_topic_raw    "raw/powermeterhan"

// NTP
#define NTP_SERVER      CONFIG_NTP_SERVER

#define REPORTING_DEBUG_PERIOD 60000
#define WATCHDOG_TIMEOUT_PERIOD 60000

#define RXD2 16
#define TXD2 17

//#define PC_SERIAL_DEBUGGING 1
////////////////////////////////
// Forward declerations ////////
////////////////////////////////
void onMqttMessage(char* topic, byte* payload, unsigned int length);



// Macros
#if defined(ESP32) or defined(ESP8266)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

////////////////////////////////
// Complex Variables
////////////////////////////////
HardwareSerial* debugger = NULL;

// The HAN Port reader
//DlmsReader reader;
HanReader hanReader;
//HanReaderTest hanTest;
//HardwareSerial HSerial1(1);
HardwareSerial* hanPort;

// Wifi and MQTT
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(
  ntpUDP,     // Connect using this UDP implementation
  NTP_SERVER, // Connect to this server
  0,          // 0 seconds timezone offset
  6000        // Update every 60sec
  );

unsigned long now;
unsigned long lastUpdate = 0;
unsigned long lastHANMessage = 0;


std::map<String, bool> obisCodesSeen;

//bool fncomp (byte* lhs, byte* rhs) {return memcmp(lhs, rhs, kObisCodeSize) == 0;}
//bool(*fn_pt)(byte*,byte*) = fncomp;
//std::set<byte*,bool(*)(byte*,byte*)> obisSet (fn_pt);


// Util functions
uint32_t getFreeHeap() {
  #if defined(ESP32)
  return ESP.getFreeHeap();
  #else
  return system_get_free_heap_size();
  #endif
}

unsigned long getEpochTime() {
  return timeClient.getEpochTime();
}

char mqttLogBuf[100];
void mqttLogger(const char *fmt, ...) {
  va_list argptr;
  va_start(argptr, fmt);
  vsnprintf(mqttLogBuf, 100, fmt, argptr);
  va_end(argptr);
  mqttClient.publish(power_topic_debug, mqttLogBuf);
}

void sendBuffer(byte* buf, int len) {
  if (mqttClient.beginPublish(power_topic_raw, len, false)) {
    uint16_t bytesRemaining = len;
    uint8_t kMaxBytesToWrite = 64;
    uint8_t bytesToWrite;
    uint16_t bytesWritten = 0;
    uint16_t rc;
    bool result = true;

    while((bytesRemaining > 0) && result) {
      bytesToWrite = min(bytesRemaining, kMaxBytesToWrite);
      //bytesToWrite = bytesRemaining;
      rc = mqttClient.write(&(buf[bytesWritten]), bytesToWrite);
      //result = (rc == bytesToWrite);
      bytesRemaining -= rc;
      bytesWritten += rc;
    }

    mqttClient.endPublish();
  }
}

void hard_restart() {
  #if defined(ESP32)
  esp_task_wdt_init(1,true);
  esp_task_wdt_add(NULL);
  #endif
  while(true);
}

////////////////////////////////
// Setup - Entry point /////////
////////////////////////////////

void setup() {
  setupDebugPort();
  setupWiFi();
  setupOTA();
  setupNTPClient();
  setupMqtt();
  setupHAN();
}

void loop() {
  loopOTA();
  loopNTPClient();
  loopMqtt();
  loopHAN();

  now = millis();
  if ((unsigned long)(now - lastUpdate) >= REPORTING_DEBUG_PERIOD) {
    lastUpdate = now;
    uint32_t freeHeap = getFreeHeap();
    char buf[64];
    sprintf(buf, "Free heap: %u", freeHeap);
    mqttClient.publish(power_topic_debug, buf);
  }

  if ((unsigned long)(now - lastHANMessage) >= WATCHDOG_TIMEOUT_PERIOD) {
    mqttClient.publish(power_topic_debug, "Critical error. No M-Bus data in 60 seconds. Rebooting.");
    delay(100);
    hard_restart();
  }

  delay(10);
}

void setupDebugPort() {
  // Uncomment to debug over the same port as used for HAN communication
  debugger = &Serial;

  // initialize serial communication at 9600 bits per second:
  Serial.begin(115200); //115200 2400
  //Serial.println("Serial debugging port initialized");
  // Initialize the Serial1 port for debugging
  // (This port is fixed to Pin2 of the ESP8266)
  //Serial1.begin(115200);
  //while (!Serial1) {}
  //Serial1.setDebugOutput(true);
  //Serial1.println("Serial1");
  //Serial1.println("Serial1 debugging port initialized");
}

void setupWiFi() {
  // Initialize wifi
  if (debugger) debugger->print("Connecting to ");
  if (debugger) debugger->println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (debugger) debugger->print(".");
  }

  if (debugger) {
    debugger->println("");
    debugger->println("WiFi connected");
    debugger->println("IP address: ");
    debugger->println(WiFi.localIP());
  }
}

////////////////////////////////
// OTA       ///////////////////
////////////////////////////////
void setupOTA() {
  #ifdef USE_ARDUINO_OTA
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  if (debugger) debugger->print("ArduinoOTA Initializing...");

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(OTA_HOSTNAME);

  // No authentication by default
  ArduinoOTA.setPassword((const char *)OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    if (debugger) debugger->println("Start");
  });
  ArduinoOTA.onEnd([]() {
    if (debugger) debugger->println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (debugger) {
      debugger->print("Progress: ");
      debugger->print((progress / (total / 100)));
      debugger->println("%%");
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if (debugger) {
      debugger->print("Error[");
      debugger->print(error);
      debugger->print("]: ");
    }

    if (error == OTA_AUTH_ERROR) {
      if (debugger) { debugger->println("Auth Failed");}
    }
    else if (error == OTA_BEGIN_ERROR) {
      if (debugger) { debugger->println("Begin Failed");}
    }
    else if (error == OTA_CONNECT_ERROR) {
      if (debugger) { debugger->println("Connect Failed");}
    }
    else if (error == OTA_RECEIVE_ERROR) {
      if (debugger) { debugger->println("Receive Failed");}
    }
    else if (error == OTA_END_ERROR) {
      if (debugger) { debugger->println("End Failed");}
    }
  });
  ArduinoOTA.begin();

  if (debugger) debugger->println("Done");
  #endif
}

void loopOTA() {
  #ifdef USE_ARDUINO_OTA
  ArduinoOTA.handle();
  #endif
}

////////////////////////////////
// NTP       ///////////////////
////////////////////////////////
void setupNTPClient() {
  if (debugger) debugger->print("NTPClient Initializing...");
  timeClient.begin();
  if (debugger) debugger->println("Done");
}

void loopNTPClient() {
  timeClient.update();
}

////////////////////////////////
// MQTT      ///////////////////
////////////////////////////////
void setupMqtt() {
  if (debugger) debugger->print("MQTT Initializing...");
  mqttClient.setServer(MQTT_SERVER, MQTT_SERVERPORT);
  mqttClient.setCallback(onMqttMessage);
  //espClient.setFingerprint(CONFIG_MQTT_FINGERPRINT);
  if (debugger) debugger->println("Done");
}

// Ensure the MQTT lirary gets some attention too
void loopMqtt()
{
  if (!mqttClient.connected()) {
    reconnectMqtt();
  }
  mqttClient.loop();
}

//char buf[64];

void reconnectMqtt() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    if (debugger) debugger->print("MQTT connection establishing...");
    // Attempt to connect
    if (mqttClient.connect(MQTT_MYNAME)) {
      if (debugger) debugger->println("Connected");
      mqttClient.subscribe(power_topic_command);
      mqttClient.publish(power_topic_debug, "Hello world");

      //uint32_t freeHeap = system_get_free_heap_size();

      //sprintf(buf, "Free heap: %u", freeHeap);
      //if (debugger) debugger->println(buf);
      //mqttClient.publish(power_topic_debug, buf);
    }
    else {
      if (debugger) debugger->print("Failed, rc=");
      if (debugger) debugger->print(mqttClient.state());
      if (debugger) debugger->println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  if (debugger) debugger->print("Message arrived [");
  if (debugger) debugger->print(topic);
  if (debugger) debugger->print("] ");
  for (uint32_t i = 0; i < length; i++) {
    char receivedChar = (char)payload[i];
    debugger->print(receivedChar);
    if (receivedChar == '0') {
    }
    if (receivedChar == '1') {
    }
  }
  if (debugger) debugger->println();
}

////////////////////////////////
// HAN       ///////////////////
////////////////////////////////

void setupHAN() {
  if (debugger) debugger->print("HAN MBUS Serial Setup Initializing...");

  #if defined(ESP8266)
  hanPort = &Serial;
  #else
  hanPort = new HardwareSerial(1);
  #endif

  //if (debugger) {
  //  // Setup serial port for debugging
  //  debugger->begin(2400, SERIAL_8E1);
  //  while (!&debugger);
  //  debugger->println("Started...");
  //  debugger->setDebugOutput(true);
  //}

  #if defined(ESP8266)
  hanPort->begin(2400, SERIAL_8E1);
  hanPort->swap();
  #else
  hanPort->begin(2400,SERIAL_8E1, RXD2, TXD2);
  #endif

  // Setup serial port for debugging

  //hanPort->begin(2400,SERIAL_8E1, RXD2, TXD2);
  //Serial.swap();
  //hanPort->begin(115200, SERIAL_8E1);
  //while (!&hanPort);
  //debugger->println("Started...");
  //debugger->setDebugOutput(true);

  #if defined(PC_SERIAL_DEBUGGING)
  hanPort = &Serial;
  #endif

  hanReader.setNetworkLogger(mqttLogger);
  hanReader.setSendBuffer(sendBuffer);
  hanReader.setGetEpochTime(getEpochTime);
  // initialize the HanReader
  // (passing Serial as the HAN port and Serial1 for debugging)
  //hanReader.setup(&Serial, &Serial1);
  hanReader.setup(hanPort, debugger);
  lastHANMessage = millis();
  if (debugger) debugger->println("Done");
}

void loopHAN() {
  // Read one byte from the port, and see if we got a full package
  if (hanReader.read()) {
    String epochString = String(hanReader.messageTimestamp());

    for (auto&& elem : hanReader.cosemObjectList) {
      String subTopicRoot = power_topic;
      subTopicRoot += "/" + elem->ObisCodeString();

      String subTopicValue = subTopicRoot + "/value";
      String subTopicUnit = subTopicRoot + "/unit";
      String subTopicUpdated = subTopicRoot + "/updated";

      mqttClient.publish(subTopicValue.c_str(), elem->ValueString().c_str());
      mqttClient.publish(subTopicUpdated.c_str(), epochString.c_str());

      if (obisCodesSeen[elem->ObisCodeString()] == false) {
        obisCodesSeen[elem->ObisCodeString()] = true;
        mqttClient.publish(subTopicUnit.c_str(), elem->EnumString().c_str(), true);
      }

      // Delete each element as we iterate.
      elem->Reset();
      delete elem;
    }

    // Clear the list
    hanReader.cosemObjectList.clear();

    // Signal that we're done updating a full notification list
    String subTopicListUpdated = String(power_topic) + "/list_updated";
    mqttClient.publish(subTopicListUpdated.c_str(), epochString.c_str());

    lastHANMessage = millis();
  }
}
