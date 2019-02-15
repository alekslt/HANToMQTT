
#include <inttypes.h>

#include "HanReader.h"
#include "Aidon.h"

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

			// Only care for the ACtive Power Imported, which is found in the first list
			if (listSize == (int)Aidon::List1)
			{
				int power = hanReader.getInt((int)Aidon_List1::ActivePowerImported);
				printf("Power consumtion is right now: %d W\n", power);
			}
		}
		fflush(stdout);
	}

	return 0;
}
