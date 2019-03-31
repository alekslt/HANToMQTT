#ifndef _HANREADER_h
#define _HANREADER_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "arduino.h"
  #include <HardwareSerial.h>
#else
  #include "WProgram.h"
#endif

#include <vector>
//#include <ArduinoJson.h>
#include "DlmsReader.h"

const byte TYPE_LIST = 0x01;
const byte TYPE_STRUCTURE = 0x02;

const byte TYPE_UINT32 = 0x06;
const byte TYPE_OCTET_STRING = 0x09;
const byte TYPE_STRING = 0x0a;
const byte TYPE_INT8 = 0x0f;
const byte TYPE_INT16 = 0x10;
const byte TYPE_UINT16 = 0x12;
const byte TYPE_ENUM = 0x16;
const byte TYPE_FLOAT = 0x17;
const byte TYPE_DATETIME = 0xf0; // Standard states that future interfaces will use date_time = 25(dec)

const byte kObisCodeSize = 0x06;
const byte kObisClockAndDeviation[] = { 0x00, 0x00, 0x01, 0x00, 0x00, 0xff };

struct ClockDeviation {
  char dateTime[20];
  uint16_t deviation;
  byte clockStatus;
};

struct Element {
  byte dataType;
  byte stringLength;
  union Value {
    char* value_string;
    uint32_t value_uint;
    int32_t value_int;
    float value_float;
    ClockDeviation* value_dateTime;
  } value;
};

struct ObisElement {
  byte* obisOctets;
  Element* data;
  uint8_t enumType;

  String ObisCodeString();
  String ValueString();
  String EnumString();

  void debugString(char* buf);
  //void toArduinoJson(JsonObject& json);

  void Reset();
};

//typedef uint32_t SerialConfig;

class HanReader
{
public:
  //const uint8_t dataHeader = 8;
  std::vector<ObisElement*> cosemObjectList;

  HanReader();
  void setup(HardwareSerial *hanPort);
  void setup(HardwareSerial *hanPort, Stream *debugPort);
  void setup(HardwareSerial *hanPort, unsigned long baudrate, SerialConfig config, Stream *debugPort);
  void setNetworkLogger(LOG_FN function) { netLog = function; }
  bool read();
  bool read(byte data);
  int getListSize();

private:
  uint8_t debugLevel;
  Stream *debug;
  LOG_FN netLog;
  HardwareSerial *han;
  //byte buffer[512];
  int bytesRead;
  DlmsReader reader;
  int listSize;
  byte* userData;
  int userDataLen;

  void Init();
  void printObjectStart(uint16_t pos);
  bool decodeClockAndDeviation(byte* buf, ClockDeviation& element);
  bool decodeAndApplyScalerElement(uint16_t& nextPos, ObisElement* element);
  bool decodeDataElement(uint16_t& nextPos, Element& element);
  bool decodeListElement(uint16_t& nextPos, ObisElement* obisElement);

  void debugPrint(byte *buffer, int start, int length);
};

/*
class HanReaderTest
{
public:
  //const uint8_t dataHeader = 8;
  std::vector<ObisElement*> cosemObjectList;

  HanReaderTest() {};

private:
  uint8_t debugLevel;
  Stream *debug;
  HardwareSerial *han;
  //byte buffer[512];
  int bytesRead;
  DlmsReader reader;
  int listSize;
  byte* userData;
  int userDataLen;
};
*/
#endif
