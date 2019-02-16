
#include <inttypes.h>

#include <ArduinoJson.h>

#include "HanReader.h"
//#include "Aidon.h"

uint8_t open_serial(char* a, int b, char* c) { return 0; }

int main(int argc, char *argv[]) {
	// The HAN Port reader
	HanReader hanReader;
	HardwareSerial Serial(0);
	Stream Serial1;

	hanReader.setup(&Serial, &Serial1);

	while (1) {
		// Read one byte from the port, and see if we got a full package
		if (hanReader.read()) {
			// Get the list identifier
			int listSize = hanReader.getListSize();

			printf("\n");
			printf("List size: %d: ", listSize);

			// Define a json object to keep the data
			StaticJsonBuffer<4096> jsonBuffer;
			JsonObject& root = jsonBuffer.createObject();

			// Any generic useful info here
			root["id"] = "espdebugger";
			//root["up"] = millis();
			//root["t"] = time;

			// Add a sub-structure to the json object, 
			// to keep the data from the meter itself
			JsonArray& data = root.createNestedArray("data");

			char buf[128];

			for (auto&& elem : hanReader.cosemObjectList) { // access by forwarding reference, the type of i is int&
				//elem->debugString(buf);
				//printf("%s\n", buf);
				JsonObject& jsonElem = data.createNestedObject();
				elem->toArduinoJson(jsonElem);
				elem->Reset();
				delete elem;
			}
			hanReader.cosemObjectList.clear();

			// Write the json to the debug port
			//root.printTo(Serial1);
			//Serial1.println("");

			// Publish the json to the MQTT server
			char msg[1024];
			root.printTo(msg, 1024);
			Serial1.println(msg);
		}
		fflush(stdout);
	}

	return 0;
}
