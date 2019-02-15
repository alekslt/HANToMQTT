#include "HanReader.h"
#include <map>

#ifdef _WIN32
#include <math.h>
#endif

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

std::map<int, String> cosemTypeToString;

HanReader::HanReader()
{
	cosemTypeToString[TYPE_LIST] = String("TYPE_LIST");
	cosemTypeToString[TYPE_STRUCTURE] = String("TYPE_STRUCTURE");
	cosemTypeToString[TYPE_UINT32] = String("TYPE_UINT32");
	cosemTypeToString[TYPE_OCTET_STRING] = String("TYPE_OCTET_STRING");
	cosemTypeToString[TYPE_STRING] = String("TYPE_STRING");
	cosemTypeToString[TYPE_INT8] = String("TYPE_INT8");
	cosemTypeToString[TYPE_INT16] = String("TYPE_INT16");
	cosemTypeToString[TYPE_UINT16] = String("TYPE_UINT16");
	cosemTypeToString[TYPE_ENUM] = String("TYPE_ENUM");
}

void HanReader::setup(HardwareSerial *hanPort, unsigned long baudrate, SerialConfig config, Stream *debugPort)
{
	// Initialize H/W serial port for MBus communication
	if (hanPort != NULL)
	{
		hanPort->begin(baudrate, config);
		while (!hanPort) {}
	}
	
	han = hanPort;
	bytesRead = 0;
	debug = debugPort;
	if (debug) debug->println("MBUS serial setup complete");
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

struct obis_element {
	byte* obisOctets;
	byte dataType;
	byte stringLength;
	union Value {
		char* value_string;
		uint32_t value_uint;
		int32_t value_int;
		float value_float;
	} value;
	uint8_t enumType;
};

bool HanReader::decodeListElement(uint16_t pos, uint16_t& nextPos)
{
	obis_element element;
	// Expect two or three elements. obis, value, [scaleval]
	byte dataType = userData[pos];
	uint8_t elements = userData[pos + 1];
	nextPos = pos + 2;

	if (dataType != TYPE_STRUCTURE) {
		debug->print("Object with invalid type. Expected 0x02(TYPE_STRUCTURE) got ");
		debug->print(dataType, HEX);
		debug->println(cosemTypeToString[dataType].c_str());
		return false;
	}

	printObjectStart(nextPos);
	uint8_t obisType = userData[nextPos];
	uint8_t obisLength = userData[nextPos + 1];
	if (obisType != TYPE_OCTET_STRING || obisLength != 6) {
		debug->print("OBIS element is invalid. Expected 0x09(OCTET_STRING) and length 0x06 got type:");
		debug->print(obisType, HEX);
		debug->print("(");
		debug->print(cosemTypeToString[obisType].c_str());
		debug->print(") and length:");
		debug->println(obisLength, HEX);
		return false;
	}
	byte* obis_code = &(userData[nextPos + 2]);
	nextPos += 8;
	
	element.obisOctets = obis_code;

	printObjectStart(nextPos);
	uint8_t valueType = userData[nextPos];
	uint8_t valueLength = userData[nextPos + 1];
	if (valueType == TYPE_UINT32) {
		element.value.value_uint = userData[nextPos + 1] << 24 | userData[nextPos + 2] << 16 | userData[nextPos + 3] << 8 | userData[nextPos + 4];
		nextPos += 5;
	}
	else if (valueType == TYPE_STRING) {
		element.stringLength = valueLength;
		element.value.value_string = (char*)&userData[nextPos + 2];
		nextPos += 2+ valueLength;
	}
	else if (valueType == TYPE_UINT16) {
		element.value.value_uint = userData[nextPos + 1] << 8 | userData[nextPos + 2];
		nextPos += 3;
	}
	else if (valueType == TYPE_INT16) {
		element.value.value_int = userData[nextPos + 1] << 8 | userData[nextPos + 2];
		nextPos += 3;
	}
	else if (valueType == TYPE_OCTET_STRING) {
		element.value.value_string = (char*)&userData[nextPos + 2];
		nextPos += 2 + valueLength;
	}

	element.dataType = valueType;
	
	if (elements == 3) {
		int8_t scaler;

		printObjectStart(nextPos);
		uint8_t scaleValStructType = userData[nextPos];
		uint8_t scaleValStructElements = userData[nextPos+1];
		if (scaleValStructType != TYPE_STRUCTURE || scaleValStructElements != 2) {
			debug->println("ScaleVal Struct element is invalid. Expected 0x02(TYPE_STRUCTURE) and elements 0x02");
			return false;
		}
		nextPos += 2;

		printObjectStart(nextPos);
		uint8_t scalerType = userData[nextPos];
		if (scalerType == TYPE_INT8) {
			scaler =userData[nextPos+1];
			int a = 34;
			nextPos += 2;
		}
		else {
			debug->println("ScaleVal Struct element is invalid. Expected TYPE_INT8 and TYPE_ENUM");
			return false;
		}

		printObjectStart(nextPos);
		uint8_t enumType = userData[nextPos];
		if (enumType == TYPE_ENUM) {
			element.enumType = userData[nextPos+1];
			nextPos += 2;
		}
		else {
			debug->println("ScaleVal Struct element is invalid. Expected TYPE_INT8 and TYPE_ENUM. Got Type:");
			debug->print(enumType, HEX);
			debug->print("(");
			debug->print(cosemTypeToString[enumType].c_str());
			debug->println(")");
			return false;
		}

		if (scaler < 0) {
			if (valueType == TYPE_UINT32 || valueType == TYPE_UINT16) {
				element.value.value_float = element.value.value_uint * pow(10, scaler);
				element.dataType = TYPE_FLOAT;
			}
			else if (/*valueType == TYPE_INT32 || */valueType == TYPE_INT16) {
				element.value.value_float = element.value.value_int * pow(10, scaler);
				element.dataType = TYPE_FLOAT;
			}
		}
		else {
			if (valueType == TYPE_UINT32 || valueType == TYPE_UINT16) {
				element.value.value_uint = element.value.value_uint * pow(10, scaler);
			}
			else if (/*valueType == TYPE_INT32 || */valueType == TYPE_INT16) {
				element.value.value_int = element.value.value_int * pow(10, scaler);
			}
		}
	}
	char buf[255];
	sprintf(buf, "Object: ObisCode:%d.%d.%d.%d.%d.%d  Type:%x(%s) Value: ",
		element.obisOctets[0], element.obisOctets[1], element.obisOctets[2],
		element.obisOctets[3], element.obisOctets[4], element.obisOctets[5],
		element.dataType, cosemTypeToString[element.dataType].c_str());

	/*
	debug->print("Object: ObisCode:");
	debug->print(element.obisOctets[0]);
	debug->print(".");
	debug->print(element.obisOctets[1]);
	debug->print(".");
	debug->print(element.obisOctets[2]);
	debug->print(".");
	debug->print(element.obisOctets[3]);
	debug->print(".");
	debug->print(element.obisOctets[4]);
	debug->print(".");
	debug->print(element.obisOctets[5]);
	debug->print(" Type:");
	debug->print(element.dataType, HEX);
	debug->print("(");
	debug->print(cosemTypeToString[element.dataType].c_str());
	debug->print(") Value: ");
	*/
	uint8_t bufLen = strlen(buf);
	switch (element.dataType) {
	case TYPE_FLOAT:
		sprintf(&buf[bufLen], "%f", element.value.value_float);
		//debug->print(element.value.value_float);
		break;
	case TYPE_UINT32:
		sprintf(&buf[bufLen], "%u", element.value.value_uint);
		//debug->print(element.value.value_uint);
		break;

	case TYPE_STRING:
		sprintf(&buf[bufLen], "%.*s", element.stringLength, element.value.value_string);
		//debug->print(element.value.value_uint);
		break;
	}
	debug->println(buf);
	/*
	if (obisType != TYPE_OCTET_STRING || obisLength != 6) {
		debug->print("OBIS element is invalid. Expected 0x09(OCTET_STRING) and length 0x06 got type:");
		debug->print(obisType, HEX);
		debug->print("(");
		debug->print(cosemTypeToString[obisType].c_str());
		debug->print(") and length:");
		debug->println(obisLength, HEX);
		return false;
	}*/

	return true;

}

bool HanReader::read(byte data)
{
	if (reader.Read(data))
	{
		printf("Got Message\n");
		bytesRead = reader.GetRawData(buffer, 0, 512);

		bool retUserBufferSuccess = reader.GetUserDataBuffer(userData, userDataLen);
		if (retUserBufferSuccess == false) {
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
		if (debug)
		{
			printObjectStart(firstStructurePos);
		}
		if (dataType != TYPE_LIST)
		{
			if (debug) debug->println("Error. We expect the root element to be a list.");
			return false;
		}
		int curPos = firstStructurePos + 2;
		for (int listItemIndex = 0; listItemIndex < listLength; listItemIndex++) {
			printObjectStart(curPos);

			uint16_t nextPos;
			if (decodeListElement(curPos, nextPos)) {
				curPos = nextPos;
			}
			
		}
		listSize = getInt(0, buffer, 0, bytesRead);
		return true;
	}
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

time_t HanReader::getPackageTime()
{
	int packageTimePosition = dataHeader 
		+ (compensateFor09HeaderBug ? 1 : 0);

	return getTime(buffer, packageTimePosition, bytesRead);
}

time_t HanReader::getTime(int objectId)
{
	return getTime(objectId, buffer, 0, bytesRead);
}

int HanReader::getInt(int objectId)
{
	return getInt(objectId, buffer, 0, bytesRead);
}

String HanReader::getString(int objectId)
{
	return getString(objectId, buffer, 0, bytesRead);
}


int HanReader::findValuePosition(int dataPosition, byte *buffer, int start, int length)
{
	// The first byte after the header gives the length 
	// of the extended header information (variable)
	int headerSize = dataHeader + (compensateFor09HeaderBug ? 1 : 0);
	int firstData = headerSize + buffer[headerSize] + 1;

	for (int i = start + firstData; i<length; i++)
	{
		if (dataPosition-- == 0)
			return i;
		else if (buffer[i] == 0x0A) // OBIS code value
			i += buffer[i + 1] + 1;
		else if (buffer[i] == 0x09) // string value
			i += buffer[i + 1] + 1;
		else if (buffer[i] == 0x02) // byte value (1 byte)
			i += 1;
		else if (buffer[i] == 0x12) // integer value (2 bytes)
			i += 2;
		else if (buffer[i] == 0x06) // integer value (4 bytes)
			i += 4;
		else
		{
			if (debug)
			{
				debug->print("Unknown data type found: 0x");
				debug->println(buffer[i], HEX);
			}
			return 0; // unknown data type found
		}
	}

	if (debug)
	{
		debug->print("Passed the end of the data. Length was: ");
		debug->println(length);
	}

	return 0;
}


time_t HanReader::getTime(int dataPosition, byte *buffer, int start, int length)
{
	// TODO: check if the time is represented always as a 12 byte string (0x09 0x0C)
	int timeStart = findValuePosition(dataPosition, buffer, start, length);
	timeStart += 1;
	return getTime(buffer, start + timeStart, length - timeStart);
}

time_t HanReader::getTime(byte *buffer, int start, int length)
{
	int pos = start;
	int dataLength = buffer[pos++];

	if (dataLength == 0x0C)
	{
		int year = buffer[pos] << 8 |
			buffer[pos + 1];

		int month = buffer[pos + 2];
		int day = buffer[pos + 3];
		int hour = buffer[pos + 5];
		int minute = buffer[pos + 6];
		int second = buffer[pos + 7];

		return toUnixTime(year, month, day, hour, minute, second);
	}
	else
	{
		// Date format not supported
		return (time_t)0L;
	}
}

int HanReader::getInt(int dataPosition, byte *buffer, int start, int length)
{
	int valuePosition = findValuePosition(dataPosition, buffer, start, length);

	if (valuePosition > 0)
	{
		int value = 0;
		int bytes = 0;
		switch (buffer[valuePosition++])
		{
			case 0x12: 
				bytes = 2;
				break;
			case 0x06:
				bytes = 4;
				break;
			case 0x02:
				bytes = 1;
				break;
		}

		for (int i = valuePosition; i < valuePosition + bytes; i++)
		{
			value = value << 8 | buffer[i];
		}

		return value;
	}
	return 0;
}

String HanReader::getString(int dataPosition, byte *buffer, int start, int length)
{
	int valuePosition = findValuePosition(dataPosition, buffer, start, length);
	if (valuePosition > 0)
	{
		String value = String("");
		for (int i = valuePosition + 2; i < valuePosition + buffer[valuePosition + 1] + 2; i++)
		{
			value += String((const char*)buffer[i]);
		}
		return value;
	}
	return String("");
}

time_t HanReader::toUnixTime(int year, int month, int day, int hour, int minute, int second)
{
	byte daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	long secondsPerMinute = 60;
	long secondsPerHour = secondsPerMinute * 60;
	long secondsPerDay = secondsPerHour * 24;

	long time = (year - 1970) * secondsPerDay * 365L;

	for (int yearCounter = 1970; yearCounter<year; yearCounter++)
		if ((yearCounter % 4 == 0) && ((yearCounter % 100 != 0) || (yearCounter % 400 == 0)))
			time += secondsPerDay;

	if (month > 2 && (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)))
		time += secondsPerDay;

	for (int monthCounter = 1; monthCounter<month; monthCounter++)
		time += daysInMonth[monthCounter - 1] * secondsPerDay;

	time += (day - 1) * secondsPerDay;
	time += hour * secondsPerHour;
	time += minute * secondsPerMinute;
	time += second;

	return (time_t)time;
}