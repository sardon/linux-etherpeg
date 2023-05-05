
//#include <MacTypes.h>
//#include <Carbon/Carbon.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef int Boolean;
#define noErr  0
#define nil NULL
#define true  1
#define false  0

typedef struct {
  // no ethernet header; please remove it first.
  // IP Header:
  uint8_t	versionAndIHL;
  uint8_t	typeOfService;
  uint16_t	totalLength;
  uint32_t	ip1;
  uint8_t	timeToLive;
  uint8_t	protocol;
  uint16_t	headerChecksum;
  uint32_t	sourceIP;
  uint32_t	destIP;
  // TCP header:
  uint16_t	sourcePort;
  uint16_t	destPort;
  uint32_t	sequenceNumber;
  uint32_t	ackNumber;
  uint8_t	dataOffsetAndJunk;	// dataOffset is high 4 bits; dataOffset is number of uint32_s in TCP header
#define kFINBit 0x01
  uint8_t	moreFlagsAndJunk;
  // etc.
  // whatever
} Packet;

int createStash(void);
void destroyStash(void);

void ConsumePacket(const Packet *packet );

//static void searchForImageMarkers(const Packet *packet, int32_t *offsetOfSOI,  int32_t *offsetAfterEOI );

void harvestJPEGFromSinglePacket(const Packet *packet, int32_t offsetOfSOI, int32_t offsetAfterEOI );
void harvestJPEGFromStashAndThisPacket(const Packet *packet, int32_t offsetAfterEOI );
void addPacketToStashIfItContinuesASequence(const Packet *packet );
Boolean scanForAnotherImageMarker(char *imageData, int dataSize);

void DisplayImageAndDisposeHandle(char * jpeg, int size);
void showBlob(short n );
void eraseBlobArea(void );
