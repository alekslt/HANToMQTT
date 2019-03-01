#ifndef _DLMSREADER_h
#define _DLMSREADER_h

#include "Crc16.h"

#if defined(ARDUINO) && ARDUINO >= 100
  #include "arduino.h"
#else
  #include "WProgram.h"
#endif

typedef void(*LOG_FN)(const char *fmt, ...);

#define DLMS_READER_BUFFER_SIZE 512
#define DLMS_READER_MAX_ADDRESS_SIZE 5

enum HDLCState { kHDLCState_Waiting, kHDLCState_Data, kHDLCState_Escaped};

class DlmsReader
{
  public:
    uint8_t debugLevel;
    Stream *debug;
    LOG_FN netLog;
    
    DlmsReader();
    bool Read(byte data);
    bool ReadOld(byte data);
	  bool GetUserDataBuffer(byte *&dataBuffer, int& length);
    int GetRawData(byte *buffer, int start, int length);
    
  protected:
    Crc16Class Crc16;
    
  private:
    byte buffer[DLMS_READER_BUFFER_SIZE];
    int position;
    int dataLength;
    HDLCState state;

    byte frameFormatType;
    byte destinationAddress[DLMS_READER_MAX_ADDRESS_SIZE];
    byte destinationAddressLength;
    byte sourceAddress[DLMS_READER_MAX_ADDRESS_SIZE];
    byte sourceAddressLength;

    void Clear();
    int GetAddress(int addressPosition, byte* buffer, int start, int length);
    unsigned short GetChecksum(int checksumPosition);
    bool IsValidFrameFormat(byte frameFormatType);
    void WriteBuffer();
	void debugPrint(byte *buffer, int start, int length);
};

#endif
