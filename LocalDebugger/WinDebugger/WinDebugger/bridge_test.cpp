
#include <inttypes.h>

#include "ObisElement.h"
#include "HanReader.h"

//#include "Aidon.h"

uint8_t open_serial(char* a, int b, char* c) { return 0; }

unsigned long getEpochTime() {
  return 12345;
}

void sendBuffer(byte* buf, int len) {
  if (true) {
    int16_t bytesRemaining = len;
    int16_t bytesToWrite;
    int16_t bytesWritten = 0;
    size_t rc;
    bool result = true;

    while ((bytesRemaining > 0) && result) {
      bytesToWrite = bytesRemaining;
      rc = printf("%x", &(buf[bytesWritten]));
      //rc = mqttClient.write(&(buf[bytesWritten]), bytesToWrite);
      //result = (rc == bytesToWrite);
      bytesRemaining -= rc;
      bytesWritten += rc;
    }
  }
}

int main(int argc, char *argv[]) {
  // The HAN Port reader
  HanReader hanReader;
  HardwareSerial Serial(0);
  Stream Serial1;

  hanReader.setSendBuffer(sendBuffer);
  hanReader.setGetEpochTime(getEpochTime);
  hanReader.setup(&Serial, &Serial1);

  while (1) {
    // Read one byte from the port, and see if we got a full package
    if (hanReader.read()) {

      for (auto&& elem : hanReader.cosemObjectList) { // access by forwarding reference, the type of i is int&
        //elem->debugString(buf);
        //printf("%s\n", buf);

        elem->ValueString();
        elem->EnumString();

        elem->Reset();
        delete elem;
      }
      hanReader.cosemObjectList.clear();

      // Write the json to the debug port
      //root.printTo(Serial1);
      //Serial1.println("");
    }
    fflush(stdout);
  }

  return 0;
}
