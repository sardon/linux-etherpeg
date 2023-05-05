/* $Id: sort_frame.c,v 1.3 2002/06/22 16:57:37 seb Exp $*/
#include <netinet/in.h>
#include <string.h>
#include "sort_frame.h"


enum { kFree, kCaptured, kDisplaying};

typedef struct StashedPacket StashedPacket;


struct StashedPacket{
	int state;				// Free, captured packet, or data being displayed
	Packet *data;
	int payloadoffset;
	int SOI;
	int EOI;
	StashedPacket *parent;
	StashedPacket *next;
	StashedPacket *following;
};

enum {
	kMaxPacketLength = 1500
};

enum {
	kStashSize = 1000
};

StashedPacket stash[kStashSize];
uint32_t nextStashEntry = 0;


void error(char *errstr){
  fprintf(stderr,"Error: %s\n",errstr);
}

int createStash(void)
{
	int i;
	for (i = 0; i < kStashSize; i++)
	{
		stash[i].state = kFree;
		stash[i].data = (Packet*)malloc(kMaxPacketLength);
		if (!stash[i].data) { 
		  error("out of memory, createStash"); 
		  return(-1); 
		}
	}
	return noErr;
}

void destroyStash(void)
{
	int16_t probe;
	for( probe = 0; probe < kStashSize; probe++ ) {
		free((void *)stash[probe].data);
		stash[probe].data = nil;
	}
}

/* //static void RecyclePacketChain(StashedPacket *parent) */
/* { */
/* 	StashedPacket *p; */
/* 	// Recycle the chain of packets chained off this parent packet */
/* 	for (p=parent; p; p=p->next) p->state = kFree; */
/* } */

static void TrimPacketChain(StashedPacket *p)
{
	StashedPacket *q;
	
	if (!p) return;
	
	// Free up to the next packet with a start marker
	do { p->state = kFree; p=p->next; } while (p && (p->SOI == -1));
	
	if (p)	// If we have packet with a start marker
	{
		for (q = p; q; q=q->next) q->parent = p;
		p->parent = NULL;
	}
}

static void harvestImage(StashedPacket *parent)
{
  int32_t totalSize;
  StashedPacket *p;
  char *h;
  
  if (parent->SOI == -1) { 
    error("ERROR! parent packet has no SOI"); 
    return; 
  }
  
  totalSize = parent->payloadoffset- parent->SOI;


  if (parent->EOI != -1 && parent->EOI < parent->SOI) parent->EOI = -1;  
  p = parent;
  while (p->EOI == -1)		// While we've not found the end, look for more in-order packets
    {
      StashedPacket *srch;
      int32_t targetseqnum;
      totalSize += ntohs(p->data->totalLength) - p->payloadoffset;
      targetseqnum = ntohl(p->data->sequenceNumber) + ntohs(p->data->totalLength) - p->payloadoffset;
      for (srch = parent; srch; srch=srch->next)
	if (ntohl(srch->data->sequenceNumber) == targetseqnum)		// We found the right packet
	  {
	    if (ntohs(srch->data->totalLength) <= srch->payloadoffset) {
	      // packets like this could cause us to hang, so skip 'em
	      //error("harvestImage: skipping empty payload");
	      //continue;
	    }
	    p->following = srch;	// Link it in
	    p = srch;		// Move p one packet forward
	    break;	        // and continue
	  }
      // If we couldn't find the desired sequence number, leave the chain in place
      // -- it might get completed later
      if (!srch) return;
    }

  totalSize += p->EOI - p->payloadoffset;
  h = (char *)malloc(totalSize);

  if(h) {
    void *imageData;
    void *ptr;
    int32_t size;
    
    ptr = h;
    size = ntohs(parent->data->totalLength) - parent->SOI;
    if (parent->following == NULL) size += (parent->EOI - ntohs(parent->data->totalLength));
    imageData = (void *)parent->data + parent->SOI;
    memcpy(ptr,imageData,size);
    // BlockMoveData( ((Ptr)(parent->data)) + parent->SOI, ptr, size );
    ptr += size;
    
    p = parent->following;
    while (p) {
      size = ntohs(p->data->totalLength) - p->payloadoffset;
      if (p->following == NULL) 
	size += p->EOI - ntohs(p->data->totalLength);
      imageData = (void *)p->data +  p->payloadoffset;
      memcpy(ptr,imageData,size);
      //BlockMoveData( ((Ptr)(p->data)) + p->payloadoffset, ptr, size );
      ptr += size;
      p = p->following;
    }
    DisplayImageAndDisposeHandle(h,totalSize);
  } else {
    error("out of memory, harvestImage");
  }

  TrimPacketChain(parent);
}

static int32_t getOffsetToPayload( const Packet *packet )
{
#define kIPHeaderLength	20
  short tcpHeaderLength = (packet->dataOffsetAndJunk >> 2) & ~3;
  return kIPHeaderLength + tcpHeaderLength;
}

static void ensureFreeSlotInStash()
{
  StashedPacket *p = &stash[nextStashEntry];
  
  if (p->state != kFree)
    {
      if (p->SOI != -1) harvestImage(p);
      while (p->state != kFree)		
	// If harvestImage was unable to pull a good image
	// out of the chain, then trash it anyway to make space
	{
	  TrimPacketChain(p->parent ? p->parent : p);
	}
    }
}

static StashedPacket *addPacketToStash(const Packet *packetdata, int32_t SOI, int32_t EOI, StashedPacket *parent)
{
  StashedPacket *p;



  if (!parent && SOI == -1) { error("addPacketToStash invalid packet"); return(NULL); }
  if (htons(packetdata->totalLength) > kMaxPacketLength) return(NULL);
  
  p = &stash[nextStashEntry];
  if (p->state != kFree) { error("addPacketToStash no free space"); return(NULL); }
  if (++nextStashEntry >= kStashSize) nextStashEntry = 0;
  
  p->state  = kCaptured;
  memcpy( p->data,packetdata, htons(packetdata->totalLength));
  //BlockMoveData(packetdata, p->data, packetdata->totalLength);
  p->payloadoffset = getOffsetToPayload(p->data);
  p->SOI    = SOI;
  p->EOI    = EOI;
  p->parent = parent;
  p->next   = NULL;
  p->following = NULL;
  
  if (parent){
    p->next = parent->next;
    parent->next = p;
  }
  
  return(p);
}

static StashedPacket *findParentPacket(const Packet *packet)
{
  int i;
  for (i = 0; i < kStashSize; i++)		// Search for matching packet
    {
      const Packet *p = stash[i].data;
      if (stash[i].state == kCaptured &&
	  p->sourceIP   == packet->sourceIP   && p->destIP   == packet->destIP &&
	  p->sourcePort == packet->sourcePort && p->destPort == packet->destPort)
	{
	  // If this packet already has a parent, we share the same parent
	  if (stash[i].parent) return (stash[i].parent);
	  else return(&stash[i]);		// Else this packet is our parent
	}
    }
  return (NULL);
}

static void searchForImageMarkers(const Packet *packet, int32_t *offsetOfSOI, int32_t *offsetAfterEOI)
{
  
  uint8_t *packetStart, *dataStart, *dataEnd, *data;
  packetStart = (uint8_t *) packet;
  dataStart = packetStart + getOffsetToPayload(packet);	// first byte that might contain actual payload
  dataEnd = packetStart + ntohs(packet->totalLength);	// byte after last byte that might contain actual payload
	
  *offsetOfSOI = -1;
  *offsetAfterEOI = -1;
  
  for( data = dataStart; data <= dataEnd-3; data++ ) {
    // JPEG SOI is FF D8, but it's always followed by another FF.
    if( ( 0xff == data[0] ) && ( 0xd8 == data[1] ) && ( 0xff == data[2] ) )
      *offsetOfSOI = data - packetStart;
		
    // GIF start marker is 'GIF89a' etc.
     if ('G' == data[0] && 'I' == data[1] && 'F' == data[2] && '8' == data[3]){
       *offsetOfSOI = data - packetStart;  
     } 
  }
  for( data = dataStart; data <= dataEnd-2; data++ ) {
    // JPEG EOI is always FF D9.
    if( ( 0xff == data[0] ) && ( 0xd9 == data[1] ) ){
      *offsetAfterEOI = data - packetStart + 2; // caller will need to grab 2 extra bytes.
    }
  }
  
  if (packet->moreFlagsAndJunk & kFINBit)
    *offsetAfterEOI = ntohs(packet->totalLength);
}

// look for image-start markers more than 4 bytes into imageData.
// if one is found, remove the portion of the handle before it and return true.
// if none found, return false.
Boolean scanForAnotherImageMarker(char *imageData, int dataSize)
{
  uint8_t *packetStart, *dataStart, *dataEnd, *data;
  int32_t offsetOfStart = -1L;
	
  packetStart = imageData;
  dataStart = packetStart + 4;
  dataEnd = packetStart + dataSize;
  
  for( data = dataStart; data <= dataEnd-3; data++ ) {
    // JPEG SOI is FF D8, but it's always followed by another FF.
    if( ( 0xff == data[0] ) && ( 0xd8 == data[1] ) && ( 0xff == data[2] ) ) {
      offsetOfStart = data - packetStart;
      break;
    }
    
    // GIF start marker is 'GIF89a' etc.
    if ('G' == data[0] && 'I' == data[1] && 'F' == data[2] && '8' == data[3]) {
      printf("found gif header\n");
      offsetOfStart = data - packetStart;
      break;
    }
  }
  
  if (offsetOfStart > 0 ) {
    //char mungerPleaseDelete;
    //Munger( imageData, 0, nil, offsetOfStart, &mungerPleaseDelete, 0 );
    memmove(imageData,imageData+offsetOfStart,dataSize-offsetOfStart);
     
    return true;
  }
  else {
    return false;
  }
}

void ConsumePacket( const Packet *packet )
{
	int32_t SOI, EOI;
	StashedPacket *p;
	StashedPacket *parent;
	Boolean addMe = false, harvestMe = false;
	
	if( packet->protocol != 6 ) goto toss; // only TCP packets please
	if( ( packet->versionAndIHL & 0x0f ) != 5 ) goto toss; // minimal IP headers only (lame?)
	
	ensureFreeSlotInStash();
	
	p = NULL;
	parent = findParentPacket(packet);

	searchForImageMarkers(packet, &SOI, &EOI);
	
	// If this packet contains an image start marker, or continues an existing sequence, then stash it
	if (parent || SOI != -1) addMe = true;
	if (addMe) p = addPacketToStash(packet, SOI, EOI, parent);
	
	// If this packet contains an image end marker, and we successfully stashed it, then harvest the packet
	if (p && EOI != -1) harvestMe = true;
	if (harvestMe) harvestImage(parent ? parent : p);

	if      (harvestMe) showBlob(3); // blue
	else if (addMe)     showBlob(2); // green
	else                showBlob(1); // black
	return;
 toss:
	showBlob( 0 ); // yellow
}
