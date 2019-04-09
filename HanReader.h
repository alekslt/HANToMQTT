#ifndef _HANREADER_h
#define _HANREADER_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "arduino.h"
  #include <HardwareSerial.h>
#else
  #include "WProgram.h"
#endif

#include <vector>
#include "DlmsReader.h"



struct ClockDeviation;
struct Element;
struct ObisElement;

class HanReader
{
public:
  std::vector<ObisElement*> cosemObjectList;

  HanReader();
  void setup(HardwareSerial *hanPort, Stream *debugPort);
  void setGetEpochTime(GETEPOCHTIME_FN function) { getEpochTime = function; }
  void setNetworkLogger(LOG_FN function) { netLog = function; }
  bool read();
  bool read(byte data);
  unsigned long messageTimestamp() { return reader.messageTimestamp; }

private:
  uint8_t debugLevel;
  Stream *debug;
  GETEPOCHTIME_FN getEpochTime;
  LOG_FN netLog;
  HardwareSerial *han;
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

#endif
