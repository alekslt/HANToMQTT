#include "DlmsReader.h"

DlmsReader::DlmsReader()
{
    //this->Clear();
}

void DlmsReader::Clear()
{
    this->position = 0;
    this->dataLength = 0;
    this->destinationAddressLength = 0;
    this->sourceAddressLength = 0;
    this->frameFormatType = 0;
}


bool DlmsReader::ReadOld(byte data)
{
//    if (debug && debugLevel > 1) debug->print(position, HEX);
//    if (debug && debugLevel > 1) debug->print("\t");
//    if (debug && debugLevel > 1) debug->println(data, HEX);

    if (position == 0 && data != 0x7E)
    {
        // we haven't started yet, wait for the start flag (no need to capture any data yet)
        return false;
    }
    else
    {
        // We have completed reading of one package, so clear and be ready for the next
        if (dataLength > 0 && position >= dataLength + 2)
            Clear();

        // Check if we're about to run into a buffer overflow
        if (position >= DLMS_READER_BUFFER_SIZE)
            Clear();

        // Check if this is a second start flag, which indicates the previous one was a stop from the last package
        if (position == 1 && data == 0x7E)
        {
            // just return, we can keep the one byte we had in the buffer
            return false;
        }

        // We have started, so capture every byte
        buffer[position++] = data;

        if (position == 1)
        {
            if (debug && debugLevel > 1) debug->print("Start: ");
            if (debug && debugLevel > 1) debug->println(data, HEX);
            messageTimestamp = getEpochTime();
            // This was the start flag, we're not done yet
            return false;
        }
        else if (position == 2)
        {
            // Capture the Frame Format Type
            frameFormatType = (byte)(data & 0xF0);
            printf(" Frameformat: %x Valid: %d\n", frameFormatType, IsValidFrameFormat(frameFormatType));
            if (!IsValidFrameFormat(frameFormatType)) {
                if (netLog && debugLevel > 1) netLog("FF: %x, V:%d", frameFormatType, IsValidFrameFormat(frameFormatType));
                Clear();
            }

            return false;
        }
        else if (position == 3)
        {
            // Capture the length of the data package
            dataLength = ((buffer[1] & 0x0F) << 8) | buffer[2];
            printf(" DataLength: %x\n", dataLength);
            return false;
        }
        else if (destinationAddressLength == 0)
        {
            // Capture the destination address
            destinationAddressLength = GetAddress(3, destinationAddress, 0, DLMS_READER_MAX_ADDRESS_SIZE);
            if (destinationAddressLength > 3)
                Clear();
            return false;
        }
        else if (sourceAddressLength == 0)
        {
            // Capture the source address
            sourceAddressLength = GetAddress(3 + destinationAddressLength, sourceAddress, 0, DLMS_READER_MAX_ADDRESS_SIZE);
            if (sourceAddressLength > 3)
                Clear();
            return false;
        }
        else if (position == 4 + destinationAddressLength + sourceAddressLength + 2)
        {
            if (debug && debugLevel > 1) debugPrint(buffer, 0, position);

            // Verify the header checksum
            uint16_t headerChecksum = GetChecksum(position - 2);
            uint16_t calculatedChecksum = Crc16.ComputeChecksum(buffer, 1, position - 3);
            printf(" Header CheckSum: %x, Calculated : %x\n", headerChecksum, calculatedChecksum);
            if (headerChecksum != calculatedChecksum) {
                if (netLog && debugLevel > 1) netLog("Mismatched header checksum. Reset");
                Clear();
            }

            return false;
        }
        else if (position == dataLength + 1)
        {
            if (debug && debugLevel > 1) debugPrint(buffer, 0, position);

            // Verify the data package checksum
            uint16_t checksum = this->GetChecksum(position - 2);
            uint16_t calculatedChecksum = Crc16.ComputeChecksum(buffer, 1, position-3);
            printf(" CheckSum: %x, CalcCheck: %x, Pos:%x, dataLength:%x \n", checksum, calculatedChecksum, position, dataLength);
            if (checksum != calculatedChecksum) {
                if (netLog && debugLevel > 1) netLog("Mismatched frame checksum. Reset. Pos: %x. CheckP:%x, CheckC:%x, DL: %x", position, checksum, calculatedChecksum, dataLength);
                if (false && netLog && debugLevel > 1) {
                    char debuf[128];
                    int kChunkSize = 45;
                    int chunks = position / kChunkSize;
                    int remainder = position % kChunkSize;
                    for (int chunkIndex = 0; chunkIndex <= chunks; chunkIndex++) {
                        int fromPos = chunkIndex * kChunkSize;
                        int toPos = (chunkIndex == chunks) ? remainder : kChunkSize;
                        for (int i = 0; i < toPos; i++)
                        {
                            sprintf(&debuf[i * 2], "%02x", buffer[fromPos+i]);
                        }
                        debuf[toPos*2] = 0x00;
                        netLog("%s", debuf);
                    }
                }
                Clear();
            }

            return false;
        }
        else if (position == dataLength + 2)
        {
            // We're done, check the stop flag and signal we're done
            if (data == 0x7E) {
                printf("Stop: %x, Pos:%x, dataLength:%x \n", data, position, dataLength);
                return true;
            }
            else
            {
                printf("Stop Error: %x, Pos:%x, dataLength:%x \n", data, position, dataLength);
                Clear();
                return false;
            }
        }
    }
    return false;
}

bool DlmsReader::Read(byte data)
{
/*
        # Waiting for packet start
        if (self.state == WAITING):
            if (c == FLAG):
                self.state = DATA
                self.pkt = ""

        elif (self.state == DATA):
            if (c == FLAG):
                # Minimum length check
                if (len(self.pkt) >= 19):
                    # Check CRC
                    crc = self.crc_func(self.pkt[:-2])
                    crc ^= 0xffff
                    if (crc == struct.unpack("<H", self.pkt[-2:])[0]):
                        self.parse(self.pkt)
                self.pkt = ""
            elif (c == ESCAPE):
                self.state = ESCAPED
            else:
                self.pkt += c

        elif (self.state == ESCAPED):
            self.pkt += chr(ord(c) ^ 0x20)
            self.state = DATA
            */
    if (netLog && debugLevel > 1 && data == 0x7E) netLog("Frame Flag. Pos: %x, State: %x", position, this->state);

    if (this->state == kHDLCState_Waiting)
    {
        if (data == 0x7E)
        {
            if (debug && debugLevel > 1) debug->println(" Frame Start Flag");
            this->state = kHDLCState_Data;
            Clear();
        }
    }
    else if (this->state == kHDLCState_Data)
    {
        if (data == 0x7E)
        {
            // Get out of invalid state.
            if (position == 0) {
              if (debug && debugLevel > 1) debug->println("Expected Flag to indicate stop, but pos=0. Resetting");
              if (netLog && debugLevel > 1) netLog("State_Data. pos=0. data=0x7e. Invalid state. Resetting");
              Clear();
              return false;
            }

            if (debug && debugLevel >1 ) debugPrint(buffer, 0, position);

            if (debug && debugLevel > 1) debug->print(" Frame Stop Flag. Pos: ");
            if (debug && debugLevel > 1) debug->println(position, HEX);

            // Minimum length check
            if (position <= 18)
            {
                this->state = kHDLCState_Data;
                Clear();
                return false;
            }

            // Capture the Frame Format Type
            frameFormatType = (byte)(buffer[0] & 0xF0);
            if (debug && debugLevel > 1) debug->print(" Frameformat: ");
            if (debug && debugLevel > 1) debug->print(frameFormatType, HEX);
            if (debug && debugLevel > 1) debug->print(" Valid: ");
            if (debug && debugLevel > 1) debug->println(IsValidFrameFormat(frameFormatType));

            if (!IsValidFrameFormat(frameFormatType))
            {
              if (netLog && debugLevel > 1) netLog("FF: %x, V:%d", frameFormatType, IsValidFrameFormat(frameFormatType));
              Clear();
              return false;
            }

            // Capture the destination address
            destinationAddressLength = GetAddress(2, destinationAddress, 0, DLMS_READER_MAX_ADDRESS_SIZE);
            if (destinationAddressLength > 3)
            {
              Clear();
              return false;
            }

            // Capture the source address
            sourceAddressLength = GetAddress(2 + destinationAddressLength, sourceAddress, 0, DLMS_READER_MAX_ADDRESS_SIZE);
            if (sourceAddressLength > 3)
            {
              Clear();
              return false;
            }

            // uint8_t control = buffer[2 + destinationAddressLength + sourceAddressLength];

            // Capture the length of the data package
            dataLength = ((buffer[0] & 0x0F) << 8) | buffer[1];
            if (debug && debugLevel > 1) debug->print(" DataLength: ");
            if (debug && debugLevel > 1) debug->println(dataLength, HEX);

            // CRC Check
            uint8_t headerLength = 2 + destinationAddressLength + sourceAddressLength + 1;
            uint16_t headerChecksum = GetChecksum(headerLength);
            uint16_t calculatedHeaderChecksum = Crc16.ComputeChecksum(buffer, 0, headerLength);

            if (debug && debugLevel > 1) debug->print(" Header CheckSum: ");
            if (debug && debugLevel > 1) debug->print(headerChecksum, HEX);
            if (debug && debugLevel > 1) debug->print(", Calculated : ");
            if (debug && debugLevel > 1) debug->print(calculatedHeaderChecksum, HEX);
            if (debug && debugLevel > 1) debug->print(", Last: ");
            if (debug && debugLevel > 1) debug->println(buffer[position-3], HEX);

            // CRC Check
            uint16_t frameChecksum = GetChecksum(position - 2);
            uint16_t calculatedframeChecksum = Crc16.ComputeChecksum(buffer, 0, position - 2);

            if (debug && debugLevel > 1) debug->print(" Frame CheckSum: ");
            if (debug && debugLevel > 1) debug->print(frameChecksum, HEX);
            if (debug && debugLevel > 1) debug->print(", Calculated : ");
            if (debug && debugLevel > 1) debug->println(calculatedframeChecksum, HEX);

            if (headerChecksum != calculatedHeaderChecksum) {
              if (debug && debugLevel > 1) debug->println("Mismatched header checksum. Reset");
              if (netLog && debugLevel > 1) netLog("Mismatched header checksum. Reset");
              Clear();
              this->state = kHDLCState_Waiting;
              return false;
            }

            if (frameChecksum != calculatedframeChecksum) {
              if (debug && debugLevel > 1) debug->println("Mismatched frame checksum. Reset");
              if (netLog && debugLevel > 1) netLog("Mismatched frame checksum. Reset");
/*
                if (netLog && debugLevel > 1 && position < 64) {
                    char debuf[128];
                    for (int i = 0; i < position; i++)
                    {
                        sprintf(&debuf[i*2], "%02x", buffer[i]);
                    }
                    debuf[position * 2] = 0x00;
                    netLog("%s", debuf);
                }
*/
              Clear();
              this->state = kHDLCState_Waiting;
              return false;
            }


            // HDLC Header                  7e .... FCS
            // LLC Header                   E6 (format identifier) E7 (group identifier) 00 (group length)
            // Service Type                 0F Data-Notification
            // Long Invoke-Id and Priority  40

            // Prepare for further reading.
            this->state = kHDLCState_Waiting;
            return true;
        }
        else if (data == 0x7D)
        {
            this->state = kHDLCState_Escaped;
        }
        else {
            // Check if we're about to run into a buffer overflow
            if (position >= DLMS_READER_BUFFER_SIZE) {
                if (debug && debugLevel > 1) debug->println("Error. Would have buffer overflowed.");
                Clear();
            }

            //if (debug && debugLevel > 1) debug->print(position, HEX);
            //if (debug && debugLevel > 1) debug->print("\t");
            //if (debug && debugLevel > 1) debug->println(data, HEX);

            buffer[position++] = data;
        }
    }
    else if (this->state == kHDLCState_Escaped)
    {
        if (netLog && debugLevel > 1) netLog("Escape Flag. For Byte: %x, Pos: %x, State: %x", data ^ 0x20, position, this->state);
        buffer[position++] = data ^ 0x20;
        this->state = kHDLCState_Data;
    }

    return false;
}

bool DlmsReader::IsValidFrameFormat(byte frameFormatType)
{
    return frameFormatType == 0xA0;
}

bool DlmsReader::GetUserDataBuffer(byte *&dataBuffer, int& length)
{
  if (debug && debugLevel > 1) debug->print(" DataLength: ");
  if (debug && debugLevel > 1) debug->println(dataLength, HEX);
  if (debug && debugLevel > 1) debug->print(" position: ");
  if (debug && debugLevel > 1) debug->println(position, HEX);

  if (netLog && debugLevel > 1) netLog("DL: %x, Pos:%x", dataLength, position);

  if (dataLength > 0 && position - 2 == dataLength)
  {
    int headerLength = 4 + destinationAddressLength + sourceAddressLength + 2;
    // int bytesWritten = 0;
    dataBuffer = &(buffer[0+ headerLength]);
    length = dataLength- headerLength -2;

    return true;
  }
  else {
    if (debug) debug->println("Invalid buffer length and position. Resetting.");
    Clear();
    return false;
  }
}

/*
bool DlmsReader::GetUserDataBuffer(byte *&dataBuffer, int& length)
{
    if (debug && debugLevel > 1) debug->print(" DataLength: ");
    if (debug && debugLevel > 1) debug->println(dataLength, HEX);
    if (debug && debugLevel > 1) debug->print(" position: ");
    if (debug && debugLevel > 1) debug->println(position, HEX);

    if (netLog && debugLevel > 1) netLog("DL: %x, Pos:%x", dataLength, position);

    if (dataLength > 0 && position == dataLength)
    {
        int headerLength = 3 + destinationAddressLength + sourceAddressLength + 2;
        int bytesWritten = 0;
        dataBuffer = &(buffer[0 + headerLength]);
        length = dataLength - headerLength - 2;

        return true;
    }
    else {
        if (debug) debug->println("Invalid buffer length and position. Resetting.");
        Clear();
        return false;
    }
}
*/
/*
int DlmsReader::GetRawData(byte *dataBuffer, int start, int length)
{
    if (dataLength > 0 && position == dataLength + 2)
    {
        int headerLength = 3 + destinationAddressLength + sourceAddressLength + 2;
        int bytesWritten = 0;
        for (int i = headerLength + 1; i < dataLength - 1; i++)
        {
          dataBuffer[i + start - headerLength - 1] = buffer[i];
          bytesWritten++;
        }
        return bytesWritten;
    }
    else
        return 0;
}
*/
int DlmsReader::GetAddress(int addressPosition, byte* addressBuffer, int start, int length)
{
    int addressBufferPos = start;
    for (int i = addressPosition; i < position; i++)
    {
        addressBuffer[addressBufferPos++] = buffer[i];

        // LSB=1 means this was the last address byte
        if ((buffer[i] & 0x01) == 0x01)
            break;

        // See if we've reached last byte, try again when we've got more data
        else if (i == position - 1)
            return 0;
    }
    return addressBufferPos - start;
}

uint16_t DlmsReader::GetChecksum(int checksumPosition)
{
    return (uint16_t)(buffer[checksumPosition + 1] << 8 |
        buffer[checksumPosition]);
}

void DlmsReader::debugPrint(byte *buffer, int start, int length)
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
  }
  debug->println("");
}
