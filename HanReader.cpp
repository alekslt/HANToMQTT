#include "HanReader.h"
#include <ArduinoJson.h>
#include <map>

#ifdef _WIN32
#include <math.h>
#else
extern "C" {
#include "user_interface.h"
}
#endif



// http://www.cs.ru.nl/~marko/onderwijs/bss/SmartMeter/Excerpt_BB7.pdf

std::map<int, String> cosemTypeToString;
std::map<byte, String> cosemEnumTypeToString;

void ObisElement::debugString(char* buf) {
	sprintf(buf, "Object: ObisCode:%d.%d.%d.%d.%d.%d  Type:%x(%s) Value: ",
		this->obisOctets[0], this->obisOctets[1], this->obisOctets[2],
		this->obisOctets[3], this->obisOctets[4], this->obisOctets[5],
		this->data->dataType, cosemTypeToString[this->data->dataType].c_str());

	size_t bufLen = strlen(buf);
	switch (this->data->dataType) {
	case TYPE_FLOAT:
		sprintf(&buf[bufLen], "%f %s", this->data->value.value_float, cosemEnumTypeToString[this->enumType].c_str());
		break;
	case TYPE_UINT32:
		sprintf(&buf[bufLen], "%u %s", this->data->value.value_uint, cosemEnumTypeToString[this->enumType].c_str());
		break;

	case TYPE_STRING:
		sprintf(&buf[bufLen], "%.*s", this->data->stringLength, this->data->value.value_string);
		break;

	case TYPE_DATETIME:
		sprintf(&buf[bufLen], "%s Dev:%d ClockStatus: %u", this->data->value.value_dateTime->dateTime,
			this->data->value.value_dateTime->deviation, this->data->value.value_dateTime->clockStatus);
		break;
	}
}

String ObisElement::EnumString() {
  auto search = cosemEnumTypeToString.find(this->enumType);
  if (search != cosemEnumTypeToString.end()) {
    return search->second;
  } else {
    return cosemTypeToString[this->data->dataType];
  }
}

String ObisElement::ValueString() {
  char buf[64];
  switch (this->data->dataType) {
  case TYPE_FLOAT:
    sprintf(buf, "%f", this->data->value.value_float);
    break;
    
  case TYPE_UINT32:
    sprintf(buf, "%u", this->data->value.value_uint);
    break;

  case TYPE_STRING:
    sprintf(buf, "%.*s", this->data->stringLength, this->data->value.value_string);
    break;

  case TYPE_DATETIME:
    return String(this->data->value.value_dateTime->dateTime);
    break;
  }
  
  return String(buf);
}

String ObisElement::ObisCodeString() {
  char buf[64];
  sprintf(buf, "%d.%d.%d.%d.%d.%d",
    this->obisOctets[0], this->obisOctets[1], this->obisOctets[2],
    this->obisOctets[3], this->obisOctets[4], this->obisOctets[5]);

    return String(buf);
}

void ObisElement::toArduinoJson(JsonObject& json) {
	char buf[64];
	sprintf(buf, "%d.%d.%d.%d.%d.%d",
		this->obisOctets[0], this->obisOctets[1], this->obisOctets[2],
		this->obisOctets[3], this->obisOctets[4], this->obisOctets[5]);

	json["obis_code"] = String(buf);

	switch (this->data->dataType) {
	case TYPE_FLOAT:
		json["value"] = this->data->value.value_float;
		json["type"] = cosemEnumTypeToString[this->enumType];
		break;
	case TYPE_UINT32:
		json["value"] = this->data->value.value_uint;
		json["type"] = cosemEnumTypeToString[this->enumType];
		break;

	case TYPE_STRING:
		sprintf(buf, "%.*s", this->data->stringLength, this->data->value.value_string);
		json["value"] = String(buf);

		break;

	case TYPE_DATETIME:
		json["value"] = String(this->data->value.value_dateTime->dateTime);
		json["deviation"] = this->data->value.value_dateTime->deviation;
		json["clockStatus"] = this->data->value.value_dateTime->clockStatus;
		break;
	}
}

void ObisElement::Reset() {
	if (this->data->dataType == TYPE_DATETIME &&
		this->data->value.value_dateTime != NULL) {

		delete this->data->value.value_dateTime;
	}
	this->data->value.value_uint = 0;
	this->data->dataType = 0;
	this->data->stringLength = 0;
	delete this->data;

	this->enumType = 0;
	this->obisOctets = 0;
}



HanReader::HanReader() {
  debugLevel = 2;
}

void HanReader::Init() {
  cosemTypeToString[TYPE_LIST] = String("TYPE_LIST");
  cosemTypeToString[TYPE_STRUCTURE] = String("TYPE_STRUCTURE");
  cosemTypeToString[TYPE_UINT32] = String("TYPE_UINT32");
  cosemTypeToString[TYPE_OCTET_STRING] = String("TYPE_OCTET_STRING");
  cosemTypeToString[TYPE_STRING] = String("TYPE_STRING");
  cosemTypeToString[TYPE_INT8] = String("TYPE_INT8");
  cosemTypeToString[TYPE_INT16] = String("TYPE_INT16");
  cosemTypeToString[TYPE_UINT16] = String("TYPE_UINT16");
  cosemTypeToString[TYPE_FLOAT] = String("TYPE_FLOAT");
  cosemTypeToString[TYPE_ENUM] = String("TYPE_ENUM");
  cosemTypeToString[TYPE_DATETIME] = String("TYPE_DATETIME");

  cosemEnumTypeToString[27] = String("W");
  cosemEnumTypeToString[28] = String("VA");
  cosemEnumTypeToString[29] = String("var");
  cosemEnumTypeToString[30] = String("Wh");
  cosemEnumTypeToString[31] = String("VAh");
  cosemEnumTypeToString[32] = String("varh");
  cosemEnumTypeToString[33] = String("A");
  cosemEnumTypeToString[34] = String("C");
  cosemEnumTypeToString[35] = String("V");
}

void HanReader::setup(HardwareSerial *hanPort, unsigned long baudrate, SerialConfig config, Stream *debugPort)
{
  Init();
  
	// Initialize H/W serial port for MBus communication
	if (hanPort != NULL)
	{
		//hanPort->begin(baudrate, config);
		while (!hanPort) {}
	}
	
	han = hanPort;
	bytesRead = 0;
	debug = debugPort;
  reader.debug = debugPort;
  reader.debugLevel = debugLevel;
  reader.netLog = netLog;
}

void HanReader::setup(HardwareSerial *hanPort)
{
	setup(hanPort, 2400, SERIAL_8E1, NULL);
}

void HanReader::setup(HardwareSerial *hanPort, Stream *debugPort)
{
	setup(hanPort, 2400, SERIAL_8E1, debugPort);
}

void HanReader::printObjectStart(uint16_t pos) {
	byte dataType = userData[pos];
  if (debug) { 
  	debug->print("Datatype@");
  	debug->print(pos, HEX);
  	debug->print(":");
  	debug->print(dataType, HEX);
  	debug->print("(");
  	debug->print(cosemTypeToString[dataType].c_str());
  	debug->print(")");
  	debug->print(", Elements:");
  	debug->println(userData[pos + 1], HEX);
  }
}

bool HanReader::decodeClockAndDeviation(byte* buf, ClockDeviation& element) {

	uint16_t year = buf[0] << 8 | buf[1];

	uint8_t month = buf[2];
	uint8_t day = buf[3];
	uint8_t dayOfWeek = buf[4];
	uint8_t hour = buf[5];
	uint8_t minute = buf[6];
	uint8_t second = buf[7];
	uint8_t hundreths = buf[8]; // Can be 0xFF if not used

	sprintf(element.dateTime, "%04u.%02u.%02uT%02u:%02u:%02u", year, month, day, hour, minute, second);
	element.deviation = buf[9] << 8 | buf[10];
	element.clockStatus = buf[11];

	return true;
}

bool HanReader::decodeDataElement(uint16_t& nextPos, Element& element) {
	if (debugLevel > 2) printObjectStart(nextPos);
	element.dataType = userData[nextPos];
	uint8_t valueLength = userData[nextPos + 1];
	if (element.dataType == TYPE_UINT32) {
		element.value.value_uint = userData[nextPos + 1] << 24 | userData[nextPos + 2] << 16 | userData[nextPos + 3] << 8 | userData[nextPos + 4];
		nextPos += 5;
	}
	else if (element.dataType == TYPE_STRING) {
		element.stringLength = valueLength;
		element.value.value_string = (char*)&userData[nextPos + 2];
		nextPos += 2 + valueLength;
	}
	else if (element.dataType == TYPE_UINT16) {
		element.value.value_uint = userData[nextPos + 1] << 8 | userData[nextPos + 2];
		nextPos += 3;
	}
	else if (element.dataType == TYPE_INT16) {
		element.value.value_int = userData[nextPos + 1] << 8 | userData[nextPos + 2];
		nextPos += 3;
	}
	else if (element.dataType == TYPE_OCTET_STRING) {
		element.stringLength = valueLength;
		element.value.value_string = (char*)&userData[nextPos + 2];
		nextPos += 2 + valueLength;
	}
	else {
		return false;
	}

	return true;
}


bool HanReader::decodeAndApplyScalerElement(uint16_t& nextPos, ObisElement* obisElement) {
	int8_t scaler;

	if (debugLevel > 2) printObjectStart(nextPos);
	uint8_t scaleValStructType = userData[nextPos];
	uint8_t scaleValStructElements = userData[nextPos + 1];
	if (scaleValStructType != TYPE_STRUCTURE || scaleValStructElements != 2) {
    if (debug) debug->println("ScaleVal Struct element is invalid. Expected 0x02(TYPE_STRUCTURE) and elements 0x02");
		return false;
	}
	nextPos += 2;

	if (debugLevel > 2) printObjectStart(nextPos);
	uint8_t scalerType = userData[nextPos];
	if (scalerType == TYPE_INT8) {
		scaler = userData[nextPos + 1];
		nextPos += 2;
	}
	else {
    if (debug) debug->println("ScaleVal Struct element is invalid. Expected TYPE_INT8 and TYPE_ENUM");
		return false;
	}

	if (debugLevel > 2) printObjectStart(nextPos);
	uint8_t enumType = userData[nextPos];
	if (enumType == TYPE_ENUM) {
		obisElement->enumType = userData[nextPos + 1];
		nextPos += 2;
	}
	else {
    if (debug) { 
  		debug->println("ScaleVal Struct element is invalid. Expected TYPE_INT8 and TYPE_ENUM. Got Type:");
  		debug->print(enumType, HEX);
  		debug->print("(");
  		debug->print(cosemTypeToString[enumType].c_str());
  		debug->println(")");
    }
		return false;
	}

	if (scaler < 0) {
		if (obisElement->data->dataType == TYPE_UINT32 || obisElement->data->dataType == TYPE_UINT16) {
			obisElement->data->value.value_float = (float)(obisElement->data->value.value_uint * pow(10, scaler));
			obisElement->data->dataType = TYPE_FLOAT;
		}
		else if (/*obisElement->data->dataType == TYPE_INT32 || */obisElement->data->dataType == TYPE_INT16) {
			obisElement->data->value.value_float = (float)(obisElement->data->value.value_int * pow(10, scaler));
			obisElement->data->dataType = TYPE_FLOAT;
		}
	}
	else {
		if (obisElement->data->dataType == TYPE_UINT32 || obisElement->data->dataType == TYPE_UINT16) {
			obisElement->data->value.value_uint = (uint32_t)(obisElement->data->value.value_uint * pow(10, scaler));
		}
		else if (/*dataElement->dataType == TYPE_INT32 || */obisElement->data->dataType == TYPE_INT16) {
			obisElement->data->value.value_int = (int32_t)(obisElement->data->value.value_int * pow(10, scaler));
		}
	}

	return true;
}

bool HanReader::decodeListElement(uint16_t& nextPos, ObisElement* obisElement)
{
	Element* dataElement = new Element();

	if (debugLevel > 0) printObjectStart(nextPos);

	// Expect two or three elements. obis, value, [scaleval]
	byte dataType = userData[nextPos];
	uint8_t elements = userData[nextPos + 1];
	nextPos += 2;
  
	if (dataType != TYPE_STRUCTURE) {
    if (debug) { 
  		debug->print("Object with invalid type. Expected 0x02(TYPE_STRUCTURE) got ");
  		debug->print(dataType, HEX);
  		debug->println(cosemTypeToString[dataType].c_str());
    }
		return false;
	}

	if (debugLevel > 2) printObjectStart(nextPos);
	uint8_t obisType = userData[nextPos];
	uint8_t obisLength = userData[nextPos + 1];
	if (obisType != TYPE_OCTET_STRING || obisLength != kObisCodeSize) {
    if (debug) { 
  		debug->print("OBIS element is invalid. Expected 0x09(OCTET_STRING) and length 0x06 got type:");
  		debug->print(obisType, HEX);
  		debug->print("(");
  		debug->print(cosemTypeToString[obisType].c_str());
  		debug->print(") and length:");
  		debug->println(obisLength, HEX);
    }
		return false;
	}
	obisElement->obisOctets = &(userData[nextPos + 2]);
	nextPos += 8;

	if (!decodeDataElement(nextPos, *dataElement)) {
		return false;
	}

	obisElement->data = dataElement;

	// Handle conversion of time sent in an octet string type
	if (obisElement->data->dataType == TYPE_OCTET_STRING && obisElement->data->stringLength == 0x0c &&
		memcmp(obisElement->obisOctets, kObisClockAndDeviation, kObisCodeSize) == 0) {
		ClockDeviation* clockDeviation = new ClockDeviation();
		if (decodeClockAndDeviation((byte*)obisElement->data->value.value_string, *clockDeviation)) {
			obisElement->data->value.value_dateTime = clockDeviation;
			obisElement->data->dataType = TYPE_DATETIME;
		}
		else {
			delete clockDeviation;
		}

	}

	if (elements == 3) {
		decodeAndApplyScalerElement(nextPos, obisElement);
	}

	//char buf[255];
	//obisElement->debugString(buf);
	//debug->println(buf);
	return true;
}

bool HanReader::read(byte data)
{
	if (reader.Read(data))
	{
		if (debug) debug->println("Got message");
    if (debug) { debug->print("Free heap left: ");debug->println(system_get_free_heap_size()); }
		//bytesRead = reader.GetRawData(buffer, 0, 512);

		bool retUserBufferSuccess = reader.GetUserDataBuffer(userData, userDataLen);
		if (retUserBufferSuccess == false) {
      if (netLog) netLog("Error. Could not get APDU buffer");
      if (debug) debug->print("Error. Could not get APDU buffer");
			return false;
		}
		
		if (debug)
		{
			debug->print("Got valid DLMS data (");
			debug->print(userDataLen);
			debug->println(" bytes):");
			debugPrint(userData, 0, userDataLen);

			debug->print("First UserData Byte:");
			debug->print(userData[0], HEX);
			debug->print(", Last UserData Byte:");
			debug->println(userData[userDataLen - 1], HEX);
		}

		/*
			Data should start with E6 E7 00 0F
			and continue with four bytes for the InvokeId
		*/
		if (
			userData[0] != 0xE6 ||
			userData[1] != 0xE7 ||
			userData[2] != 0x00 ||
			userData[3] != 0x0F
		)
		{
			if (debug) debug->println("Invalid HAN data: Start should be E6 E7 00 0F");
			return false;
		}
		if (debug) debug->println("HAN dataheader is valid");
		int firstStructurePos = 9;
		byte dataType = userData[firstStructurePos];
		uint8_t listLength = userData[firstStructurePos+1];

		if (dataType != TYPE_LIST)
		{
			if (debug) debug->println("Error. We expect the root element to be a list.");
			return false;
		}
		uint16_t curPos = firstStructurePos + 2;
		for (int listItemIndex = 0; listItemIndex < listLength; listItemIndex++) {
      if (debug) debug->print("Decoding element: ");
      if (debug) debug->println(listItemIndex);
      
			ObisElement* obisElement = new ObisElement();
			if (decodeListElement(curPos, obisElement)) {
				this->cosemObjectList.push_back(obisElement);
			} else {
				obisElement->Reset();
				delete obisElement;
			}
		}
		listSize = listLength;
		return true;
	}
	return false;
}

void HanReader::debugPrint(byte *buffer, int start, int length)
{
	for (int i = start; i < start + length; i++)
	{
		if (buffer[i] < 0x10)
			debug->print("0");
		debug->print(buffer[i], HEX);
		debug->print(" ");
		if ((i - start + 1) % 16 == 0)
			debug->println("");
		else if ((i - start + 1) % 4 == 0)
			debug->print(" ");
		
		yield(); // Let other get some resources too
	}
	debug->println("");
}

bool HanReader::read()
{
	if (han->available())
	{
		byte newByte = han->read();
		return read(newByte);
	}
	return false;
}

int HanReader::getListSize()
{
	return listSize;
}
