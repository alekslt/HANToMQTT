#ifndef _OBISELEMENT_h
#define _OBISELEMENT_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#include <HardwareSerial.h>
#else
#include "WProgram.h"
#endif

#include <map>

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

extern std::map<int, String> cosemTypeToString;
extern std::map<byte, String> cosemEnumTypeToString;

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

  void Reset();
};

#endif // OBISELEMENT_h
