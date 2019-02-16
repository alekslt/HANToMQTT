#ifndef _HANREADER_h
#define _HANREADER_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "arduino.h"
#else
  #include "WProgram.h"
#endif

#include <vector>
#include <ArduinoJson.h>
#include "DlmsReader.h"

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

	void debugString(char* buf);
	void toArduinoJson(JsonObject& json);
	
	void Reset();
};

class HanReader
{
public:
	const uint8_t dataHeader = 8;
	bool compensateFor09HeaderBug = false;
	std::vector<ObisElement*> cosemObjectList;

	HanReader();
	void setup(HardwareSerial *hanPort);
	void setup(HardwareSerial *hanPort, Stream *debugPort);
	void setup(HardwareSerial *hanPort, unsigned long baudrate, SerialConfig config, Stream *debugPort);
	bool read();
	bool read(byte data);
	int getListSize();

private:
	uint8_t debugLevel = 2;
	Stream *debug;
	HardwareSerial *han;
	byte buffer[512];
	int bytesRead;
	DlmsReader reader;
	int listSize;
	byte* userData;
	int userDataLen;

	void printObjectStart(uint16_t pos);
	bool decodeClockAndDeviation(byte* buf, ClockDeviation& element);
	bool decodeAndApplyScalerElement(uint16_t& nextPos, ObisElement* element);
	bool decodeDataElement(uint16_t& nextPos, Element& element);
	bool decodeListElement(uint16_t& nextPos, ObisElement* obisElement);

	void debugPrint(byte *buffer, int start, int length);
};


#endif