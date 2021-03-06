#ifndef _WPROGRAM_h
#define _WPROGRAM_h

#include <stdint.h>
#include <string>
using namespace std;
#include <stdio.h>
#include <stdlib.h>


#ifdef _WIN32
static bool ioctl(int a, int b, int* c) { return false; }
static bool read(int a, unsigned char* b, int c) { return false; };
static bool open(char* a, int b) { return false; }
#define O_RDONLY 0
static void usleep(int a) {};
static void yield() {};
static void delay(uint32_t w) { }
static uint32_t system_get_free_heap_size() { return 0; }
#define FIONREAD 0
#else
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

typedef uint8_t byte;
typedef string String;
const int HEX = 123;

class SerialConfig;

class SerialConfig
{
public:
  bool parameter = 1;

};

class Stream
{
public:

  void print(int str, int type) {
    if (type == HEX) {
      printf("%x", str);
    }
    else {
      printf("%d", str);
    }
  }

  void print(byte str) {
    print((int)str, 0);
  }

  void print(int str) {
    printf("%d", str);
  }

  void print(char* str) {
    printf("%s", str);
  }

  void println(int str, int type) {
    print(str, type);
    printf("\n");
  }

  void println(int str) {
    print(str);
    printf("\n");
  }

  void println(char* str) {
    printf("%s", str);
    printf("\n");
  }

  void print(uint32_t value) {
    printf("%u", value);
  }

  void print(float value) {
    printf("%f", value);
  }

  void print(const char* str) {
    printf("%s", str);
  }

  void println(const char* str) {
    printf("%s", str);
    printf("\n");
  }

};

// Crc: 28197

static byte shortmessage[] = { 0x7e, 0xa0,0x2a,0x41,0x08,0x83,0x13,0x04,0x13,0xe6,0xe7,0x00,0x0f,0x40,0x00,0x00,0x00,0x00,0x01,0x01,0x02,
            03,0x09,0x06,0x01,0x00,0x01,0x07,0x00,0xff,0x06,0x00,0x00,0x0f,0xb3,0x02,0x02,0x0f,0x00,0x16,0x1b,0x25,0x6e, 0x7e };

// Invalid "00 48 07 00 ff 12 08 e2 fe 02 0f ff 16 23 06 f0"
// Invalid "00 48 07 00 ff 12 08 e3 02 02 0f ff 16 23 88 1b"
// Invalid "00 48 07 00 ff 12 08 e5 02 02 0f ff 16 23 42 cb"
// Valid   "00 48 07 00 ff 12 08 c9 02 02 0f ff 16 23 ed 09"
static const char* eror_list1 = "7E a0 2a 41 08 83 13 04 13 e6 e7 00 0f 40 00 00 00 00 01 01 02 03 09 06 01 00 01 07 00 ff 06 00 00 02 ae 02 02 0f 00 16 1b 7e 4f 7E";
static const char* eror_list2 = "7e a1 0b 41 08 83 13 fa 7c e6 e7 00 0f 40 00 00 00 00 01 0c 02 02 09 06 01 01 00 02 81 ff 0a 0b 41 49 44 4f 4e 5f 56 30 30 30 31 02 02 09 06 00 00 60 01 00 ff 0a 10 37 33 35 39 39 39 32 39 30 36 33 36 38 31 34 33 02 02 09 06 00 00 60 01 07 ff 0a 04 36 35 32 35 02 03 09 06 01 00 01 07 00 ff 06 00 00 09 35 02 02 0f 00 16 1b 02 03 09 06 01 00 02 07 00 ff 06 00 00 00 00 02 02 0f 00 16 1b 02 03 09 06 01 00 03 07 00 ff 06 00 00 00 00 02 02 0f 00 16 1d 02 03 09 06 01 00 04 07 00 ff 06 00 00 00 dc 02 02 0f 00 16 1d 02 03 09 06 01 00 1f 07 00 ff 10 00 62 02 02 0f ff 16 21 02 03 09 06 01 00 47 07 00 ff 10 00 0a 02 02 0f ff 16 21 02 03 09 06 01 00 20 07 00 ff 12 f8 eb 60 60 d0 59 23 02 03 09 06 01 00 34 07 00 ff 12 08 d8 02 02 0f ff 16 23 02 03 09 06 01 00 d2 00 ff 12 08 cf 02 02 0f ff 16 23 1e cc 7e 7e";
static const char* test_list2 = "7e a1 0b 41 08 83 13 fa 7c e6 e7 00 0f 40 00 00 00 00 01 0c 02 02 09 06 01 01 00 02 81 ff 0a 0b 41 49 44 4f 4e 5f 56 30 30 30 31 02 02 09 06 00 00 60 01 00 ff 0a 10 37 33 35 39 39 39 32 39 30 36 33 36 38 31 34 33 02 02 09 06 00 00 60 01 07 ff 0a 04 36 35 32 35 02 03 09 06 01 00 01 07 00 ff 06 00 00 02 e2 02 02 0f 00 16 1b 02 03 09 06 01 00 02 07 00 ff 06 00 00 00 00 02 02 0f 00 16 1b 02 03 09 06 01 00 03 07 00 ff 06 00 00 00 00 02 02 0f 00 16 1d 02 03 09 06 01 00 04 07 00 ff 06 00 00 00 f6 02 02 0f 00 16 1d 02 03 09 06 01 00 1f 07 00 ff 10 00 1e 02 02 0f ff 16 21 02 03 09 06 01 00 47 07 00 ff 10 00 0a 02 02 0f ff 16 21 02 03 09 06 01 00 20 07 00 ff 12 08 ae 02 02 0f ff 16 23 02 03 09 06 01 00 34 07 00 ff 12 08 d5 02 02 0f ff 16 23 02 03 09 06 01 00 48 07 00 ff 12 08 c9 02 02 0f ff 16 23 ed 09 7e";
static const char* test_list1 = "7e a0 2a 41 08 83 13 04 13 e6 e7 00 0f 40 00 00 00 00 01 01 02 03 09 06 01 00 01 07 00 ff 06 00 00 0f b3 02 02 0f 00 16 1b 25 6e 7e";
static const char* test_list3 = "7e a1 77 41 08 83 13 39 1e e6 e7 00 0f 40 00 00 00 00 01 11 02 02 09 06 01 01 00 02 81 ff 0a 0b 41 49 44 4f 4e 5f 56 30 30 30 31 02 02 09 06 00 00 60 01 00 ff 0a 10 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 02 02 09 06 00 00 60 01 07 ff 0a 04 36 35 32 35 02 03 09 06 01 00 01 07 00 ff 06 00 00 05 16 02 02 0f 00 16 1b 02 03 09 06 01 00 02 07 00 ff 06 00 00 00 00 02 02 0f 00 16 1b 02 03 09 06 01 00 03 07 00 ff 06 00 00 00 00 02 02 0f 00 16 1d 02 03 09 06 01 00 04 07 00 ff 06 00 00 00 d5 02 02 0f 00 16 1d 02 03 09 06 01 00 1f 07 00 ff 10 00 35 02 02 0f ff 16 21 02 03 09 06 01 00 47 07 00 ff 10 00 22 02 02 0f ff 16 21 02 03 09 06 01 00 20 07 00 ff 12 08 ac 02 02 0f ff 16 23 02 03 09 06 01 00 34 07 00 ff 12 08 d4 02 02 0f ff 16 23 02 03 09 06 01 00 48 07 00 ff 12 08 ca 02 02 0f ff 16 23 02 02 09 06 00 00 01 00 00 ff 09 0c 07 e3 02 09 06 16 00 00 ff 00 00 00 02 03 09 06 01 00 01 08 00 ff 06 00 0c 23 eb 02 02 0f 01 16 1e 02 03 09 06 01 00 02 08 00 ff 06 00 00 00 00 02 02 0f 01 16 1e 02 03 09 06 01 00 03 08 00 ff 06 00 00 00 46 02 02 0f 01 16 20 02 03 09 06 01 00 04 08 00 ff 06 00 01 b8 b6 02 02 0f 01 16 f3 81 db 7e";


class HardwareSerial : public Stream
{
public:
  int fd;
  uint32_t msgPos = 0;
  uint32_t elements;
  uint32_t msgLen = 44;
  unsigned char message[512];
  unsigned char *replay;

  void begin(unsigned long baudrate, SerialConfig config) {
    if (baudrate || config.parameter) {

    }
  }

  bool available() {
    if (this->msgPos < this->elements) {
      return true;
    }
    else {
      //this->msgPos = 0;
      //sleep(1000);
      return false;
    }

    //return true;
    int bytes;
    ioctl(fd, FIONREAD, &bytes);
    return bytes > 0;
  }

  byte read() {
    //return this->message[this->msgPos++];
    return this->replay[this->msgPos++];
        

    int n;
    unsigned char buf[2];
    int len = 1;
    while (1) {
      n = ::read(fd, buf, len);

      if (n >= 0) {
        break;
      }
      if (errno == EAGAIN) {
        usleep(1000);
      }
    }
    return buf[0];
  }


  HardwareSerial(int read_fd) {
    /*
    const char *pos = test_list2;
    // WARNING: no sanitization or error-checking whatsoever 
    uint32_t stringLength = strlen(test_list2);
    elements = (uint32_t)round((stringLength + 1) / 3);
    //elements += 2;
    for (size_t count = 0; count < elements; count++) {
      sscanf(pos, "%2hhx", &this->message[count]);
      pos += 3;
    }
    */

    fd = read_fd;

    FILE *f = fopen("U:/han/data/han-data-8144.dat", "rb");
    fseek(f, 0, SEEK_END);
    elements = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    replay = (unsigned char*)malloc(elements + 1);
    fread(replay, 1, elements, f);
    fclose(f);

    //string[fsize] = 0;

  }
};

extern void yield();

const SerialConfig SERIAL_8E1;



#endif
