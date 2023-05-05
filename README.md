### this code is unmaintained and provided here for archival purposes. Check out driftnet For a modern alternative.  

$Id: README,v 1.0.0 2002/06/20 18:24:58 seb Exp $

(c) Sebastien Ardon 2002

## About

linux-etherpeg is a packet sniffer that looks for JPEG pictures in TCP streams, reassembles the JPEG files and display them in real-time using a basic GTK-based UI. It is a crude re-implementation of EtherPeg for MacOSX (http://www.etherpeg.org/)

Useful as a demonstrator for the need of end-to-end encryption, especially over wireless links such as 802.11


## Limitations

At the moment it is pretty much a hack. Only JPEG pictures are retrieved. The reassembly is pretty basic, and may fail under (improbable) circumstances. The GUI is very basic.


## Compiling

To use, you will need the following librairies:

	- gtk > 1.2
	- gdk-imlib
	- pcap

To compile, you will need the following tools:
	
	- automake
	- autoconf

As an indication, for those looking for packages names (RPMs, debs...), 
on my debian woody system I had to install the following packages:

	- gdk-imlib1
	- libgtk1.2
	- libgtk1.2-common
	- libpcap0

And in addition, to compile:
	- libpcap-dev
	- gdk-imlib-dev
	- libgtk1.2-dev


## TODO

- Feature where viewer click on a picture, and it opens a tab for 
  this particular IP address, where all images coming from and to this
  address are displayed, along with information such as hostname, mac
  address, network interface vendor name. 


## See also 

- (2018 Update) https://www.kali.org/tools/driftnet/ a lot more complete
- http://www.etherpeg.org

## Disclaimer

There is no warranty, expressed or implied, associated with this product.
Use at your own risk.

