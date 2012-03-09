                                README
                   Sangoma JSR309 API version 1.0.0
              =========================================
                                   
Supported OS
------------
This version of the JSR309 API supports the following Linux version:

* CentOS 5.x 32-bit - kernel 2.6.18-194.8.1.v5

Supported Sangoma D-series cards
--------------------------------

This version of the JSR309 supports the following D-series cards:

* Sangoma D100
* Sangoma D150
* Sangoma D500

Supported Features
-------------------

* IPv4 only
* RTP/RTCP as per IETF RFC3550 and RFC3551
* Codecs
  - PCMU and PCMA
  - G726 16/24/32/40kbps
  - G729AB
  - AMR-NB
  - AMR-WB
  - GSM-FR
  - GSM-EFR
  - iLBC 13.3/15.2kpbs
  - G722
  - G722.1C 32kbps
* Audio Packet size from 10ms to 40ms
* VAD and CNG as per ITU-T G711 annexe 2
* DTMF releay as per IETF RFC2833
* Transcoding any to any supported audio codecs and packet size
* Transcoding capacity
  - PCMU 20ms to/from G729 30ms
    # up to 400 sessions per D100/D150
    # up to 2000 sessions per D500 fully populated
  - PCMU 20ms to/from AMR 30ms
    # up to 250 sessions per D100/D150
    # up to 1250 sessions per D500 fully populated
* Adaptive jitter buffer with up to 400 ms of packet delay variation.


Install instructions
--------------------

The JSR309 API is packaged as and RPM. To install the JSR309 API on
your development system:

1) Copy ther RPM package on your drive (for example your home folder)

2) Install the RPM package with the root privileges: 
   sudo rpm -i sangoma-jsr309-1.0.0-alpha-i386.rpm
 
The installer install the files to various locations:

* /usr/include
* /usr/lib
* /usr/bin
* /usr/share/sangoma/jsr309


Uninstall instructions
----------------------

To uninstall the JSR309 API simply install the RPM package with the
root privilages:

     sudo rpm -e sangoma-jsr309


Where to start 
-------------- 

The JSR309 API comes with samples code and HTML documentation. We
invite the developper to read the API documentation and sample code
before starting to write any code that will use the JSR309 API

* /usr/share/sangoma/jsr309/doc/api 

  This folder contains the HTML documentation for the API.

* /usr/share/sangoma/jsr309/samples

  This folder contains the samples code that shows how to use the API
  to build various applications.  Please consult the API documentation
  to get more information about how to use and build the samples.


Known limitations
-----------------

1) This API works firmware 1.6.2 for the D-series cards.  Please
update the firmware on your cards.  Consult the section "Update
D-series firmware" of this documentation to get a procedure to update
your cards.

2) The JSR309 API uses a non-routable protocol to control the D-series
cards.  Please make sure to connect the cards and the control server
onto the same Ethernet Network.

3) The JSR309 API communicates with D-series card by using the RAW sockets
mode which requires the application to run with root previleges.

4) The JSR309 "media.codec.audio" section in the configuration file
(config.xml) is limited to a maximum of 7 types of codecs.

Update D-series firmware
------------------------

This section explains how to update the firmware on the D-series
cards. This is a 5 steps process for each card (except D150).

Step 1) Reset your media modules on the card.

     sngtc_tool -dev ethX -reset

Refer to note 1 below for more details about ethX

Step 2) Update the firmware of all media modules of your card.

     sngtc_tool -dev ethX -firmware

The application lists one line per media module discoverred behind the
specified Ethernet interface.  For each module the application shows
the current firmware ersion and the new version that will be installed
on the module. Enter 'y' and press the ENTER key to proceed to the
firmware update.  Enter 'n' and press the ENTER key to quit the
application without updating the firmware on the card.

Step 3) Reset your media modules on the card again.

     sngtc_tool -dev ethX -reset

step 4) Update your license with work with the new firmware version

     sngtc_tool -dev ethX -license

Step 5) Reset your media modules on the card again.

     sngtc_tool -dev ethX -reset


Notes:

1) ethX could eth0, eth1...  It depends on the type of D-series
cards that you use with your system.

      * D100: ethX should be the name of the Ethernet interface given
      to the Ethernet controller of the card.

      * D150: ethX should be the name of the Ethernet interface
      connected to the same LAN as the D150 card.

      * D500: ethX should be the name of the Ethernet interface given
      to the Ethernet controller of the card.
