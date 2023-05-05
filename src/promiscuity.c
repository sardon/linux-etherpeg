/* $Id: promiscuity.c,v 1.3 2002/06/22 16:57:37 seb Exp $*/

#include <netinet/in.h>
#include <pcap.h> // see www.tcpdump.com for more information

#include "promiscuity.h"
#include "sort_frame.h"

static pcap_t *pcap_session = NULL;

extern void ConsumePacket(const Packet *packet );


void initPromiscuity(char *interface)
{
	char errorBuffer[PCAP_ERRBUF_SIZE];
	//tryAgain:
	pcap_session = pcap_open_live(interface, SNAPLEN, 1, 1, errorBuffer );
	if( NULL == pcap_session ) {
		
	  //int16_t itemHit;
		perror("pcap_open_live failed");
		exit(0);
	}
}

void idlePromiscuity(void)
{
	const unsigned char *ethernetPacket;
	const Packet *p;
	struct pcap_pkthdr header;
	
	ethernetPacket = pcap_next( pcap_session, &header );
	if( ethernetPacket ) {
	  //if( *(unsigned short *)(ethernetPacket+12) == EndianU16_NtoB(0x0800) ) { // ETHERTYPE_IP
	  if( *(unsigned short *)(ethernetPacket+12) == htons(0x0800) ) {
			// skip ethernet header: 6 byte source, 6 byte dest, 2 byte type
			p = (Packet *)( ethernetPacket + 6 + 6 + 2 );
	  } else if( *(unsigned short *)(ethernetPacket+12) == htons(0x8864) ) {
	    //else if( *(unsigned short *)(ethernetPacket+12) == EndianU16_NtoB(0x8864) ) { // ETHERTYPE_???
	    // skip ethernet header: 6 byte source, 6 byte dest, 2 byte type,
	    
	    // plus 8 bytes I don't know much about, but often seemed to be
	    // 11 00 07 fb 05 b0 00 21.  something about promiscuous mode?
	    p = (Packet *)( ethernetPacket + 6 + 6 + 2 + 8 );
	  }
	  else {
	    // some other kind of packet -- no concern of ours
	    return;
	  }
#if 0
	  if (p->protocol != 6)
	    printf("p->protocol != 6\n");
	  else if ((p->versionAndIHL & 0x0F) != 5)
	    printf("(p->versionAndIHL & 0x0F) != 5\n");
		else if ((p->totalLength < 40) && !(p->moreFlagsAndJunk & kFINBit))
		  printf("(p->totalLength < 40) && !(p->moreFlagsAndJunk & kFINBit)\n");
#endif
	  if ((p->protocol == 6) && ((p->versionAndIHL & 0x0F) == 5)) {
	    if ((p->totalLength > 40) || (p->moreFlagsAndJunk & kFINBit)) {
	      ConsumePacket( p );
	    }
	    else showBlob( 0 ); // yellow
	  }
	  else showBlob( 0 ); // yellow
	}
}

void termPromiscuity(void)
{
	if( pcap_session ) {
		pcap_close( pcap_session );
		pcap_session = NULL;
	}
}


