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
 * 1. PubSubClient is included in the project due as I require to enlarge the buffer used and
 *    the Arduino IDE does not allow for an easy way to do this currently by setting a define here.
 *    The headers are compiled before this unit.
 * 2. Regarding the above. Currently we have enough RAM to not care about using both a string buffer to
 *    format the message and another for mqtt to send.
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

extern "C" {
//#include "user_interface.h"
}

#include <ESP8266WiFi.h> // Wifi
//#include <WiFi.h> // Wifi
#include <EEPROM.h> // ??
#include <ArduinoOTA.h> // For OTA Programming
#include <NTPClient.h> // For NTP Wall Clock Time Syncronization
#include <WiFiClientSecure.h>
#include <WiFiUdp.h> // For NTP Wall Clock Time Syncronization

//#include <ArduinoJson.h> // json

//#define MQTT_MAX_PACKET_SIZE 512
#include <PubSubClient.h> // MQTT

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
#define power_topic         power_topic_base"/obis"
#define power_topic_command power_topic_base"/commands"
#define power_topic_debug   power_topic_base"/debug"

// NTP
#define NTP_SERVER      CONFIG_NTP_SERVER

#define REPORTING_DEBUG_PERIOD 10000

#define RXD2 16
#define TXD2 17

////////////////////////////////
// Forward declerations ////////
////////////////////////////////
void onMqttMessage(char* topic, byte* payload, unsigned int length);

////////////////////////////////
// Complex Variables
////////////////////////////////
HardwareSerial* debugger = NULL;

// The HAN Port reader
//DlmsReader reader;
HanReader hanReader;
//HanReaderTest hanTest;
HardwareSerial HSerial1(1);
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


std::map<String, bool> obisCodesSeen;

//bool fncomp (byte* lhs, byte* rhs) {return memcmp(lhs, rhs, kObisCodeSize) == 0;}
//bool(*fn_pt)(byte*,byte*) = fncomp;
//std::set<byte*,bool(*)(byte*,byte*)> obisSet (fn_pt);

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
    //uint32_t freeHeap = system_get_free_heap_size();
    //char buf[64];
    //sprintf(buf, "Free heap: %u", freeHeap);
    //mqttClient.publish(power_topic_debug, buf);
  }
  delay(10);
}



char mqttLogBuf[100];
void mqttLogger(const char *fmt, ...) {
  va_list argptr;
  va_start(argptr, fmt);
  vsnprintf(mqttLogBuf, 100, fmt, argptr);
  va_end(argptr);
  mqttClient.publish(power_topic_debug, mqttLogBuf);
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
  hanPort = &HSerial1;

  if (debugger) debugger->print("HAN MBUS Serial Setup Initializing...");

  //if (debugger) {
  //  // Setup serial port for debugging
  //  debugger->begin(2400, SERIAL_8E1);
  //  while (!&debugger);
  //  debugger->println("Started...");
  //  debugger->setDebugOutput(true);
  //}

  if (hanPort) {
    // Setup serial port for debugging
    hanPort->begin(2400, SERIAL_8E1);
    //hanPort->begin(2400,SERIAL_8E1, RXD2, TXD2);
    //Serial.swap();
    //hanPort->begin(115200, SERIAL_8E1);
    //while (!&hanPort);
    //debugger->println("Started...");
    //debugger->setDebugOutput(true);
  }

  hanReader.setNetworkLogger(mqttLogger);
  // initialize the HanReader
  // (passing Serial as the HAN port and Serial1 for debugging)
  //hanReader.setup(&Serial, &Serial1);
  hanReader.setup(hanPort, 115200, SERIAL_8E1, debugger);
  if (debugger) debugger->println("Done");
}

void loopHAN() {
  // Read one byte from the port, and see if we got a full package
  if (hanReader.read()) {
    // Get the list identifier
    int listSize = hanReader.getListSize();

    if (debugger) debugger->println("Preparing to send message");
    if (debugger) debugger->print("List Size: ");
    if (debugger) debugger->println(listSize);

    // Any generic useful info here
    //root["id"] = "espdebugger";
    //root["up"] = millis();
    //root["t"] = time;

    unsigned long epoch = timeClient.getEpochTime();
    String epochString = String(epoch);

    // Add a sub-structure to the json object,
    // to keep the data from the meter itself
    //JsonArray& data = root.createNestedArray("data");
    //char msg[512];

    for (auto&& elem : hanReader.cosemObjectList) { // access by forwarding reference, the type of i is int&
      //elem->debugString(buf);
      //printf("%s\n", buf);
      //JsonObject& jsonElem = data.createNestedObject();
      // Define a json object to keep the data
      //StaticJsonBuffer<512> jsonBuffer;
      //JsonObject& root = jsonBuffer.createObject();
      //elem->toArduinoJson(root);


      //root["t"] = epoch;

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

      elem->Reset();
      delete elem;

      //root.printTo(msg, 512);
      //debugger->println(msg);
      //mqttClient.publish(subTopic.c_str(), msg);
    }
    hanReader.cosemObjectList.clear();

    String subTopicListUpdated = String(power_topic) + "/list_updated";
    mqttClient.publish(subTopicListUpdated.c_str(), epochString.c_str());

    // Write the json to the debug port
    //root.printTo(Serial1);
    //debugger->println("");

    // Publish the json to the MQTT server
  }
}
