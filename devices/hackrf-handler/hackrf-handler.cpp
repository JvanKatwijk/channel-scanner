#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *    Copyright (C) 2019 Amplifier, antenna and ppm correctors
 *    Fabio Capozzi
 *
 *    This file is part of channelScanner
 *
 *    channelScanner is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation version 2 of the License.
 *
 *    channelScanner is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with channelScanner if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"hackrf-handler.h"
#include	"xml-filewriter.h"
#include	<unistd.h>

#define	DEFAULT_GAIN	30

	hackrfHandler::hackrfHandler  (RingBuffer<std::complex<float>> *b,	
	                               const std::string & recorderVersion,
	                               int32_t	frequency,
	                               int16_t	ppm,
	                               int16_t	lnaGain,
	                               int16_t	vgaGain,
	                               bool	ampEnable):
	                                 deviceHandler (b) {
int	res;
	this	-> _I_Buffer		= b;
	this	-> recorderVersion	= recorderVersion;
	vfoFrequency			= frequency;
	this	-> lnaGain		= lnaGain;
	this	-> vgaGain		= vgaGain;
	this	-> ampEnable		= ampEnable;

	this	-> inputRate		= 2048000;
//
	res	= hackrf_init ();
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_init:");
	   fprintf (stderr, "%s \n", hackrf_error_name (hackrf_error (res)));
	   throw (21);
	}

	res	= hackrf_open (&theDevice);
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_open:");
	   fprintf (stderr, "%s \n",
	                 hackrf_error_name (hackrf_error (res)));
	   throw (22);
	}

	res	= hackrf_set_sample_rate (theDevice, 2048000.0);
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_set_samplerate:");
	   fprintf (stderr, "%s \n", hackrf_error_name (hackrf_error (res)));
	   throw (23);
	}

	res	= hackrf_set_baseband_filter_bandwidth (theDevice,
	                                                        1750000);
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_set_bw:");
	   fprintf (stderr, "%s \n", hackrf_error_name (hackrf_error (res)));
	   throw (24);
	}

	res	= hackrf_set_freq (theDevice, frequency);
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_set_freq: ");
	   fprintf (stderr, "%s \n", hackrf_error_name (hackrf_error (res)));
	   throw (25);
	}

	hackrf_device_list_t *deviceList = hackrf_device_list ();
	if (deviceList != nullptr) {	// well, it should be
	   char *serial = deviceList -> serial_numbers [0];
	   enum hackrf_usb_board_id board_id =
	                 deviceList -> usb_board_ids [0];
	   (void) serial;
	   (void) board_id;
	}

	if ((vgaGain <= 62) && (vgaGain >= 0)) 
           (void)hackrf_set_vga_gain (theDevice, vgaGain);
	if ((lnaGain <= 40) && (lnaGain >= 0)) 
           (void)hackrf_set_lna_gain (theDevice, lnaGain);
	(void)hackrf_set_amp_enable (theDevice, ampEnable);

	dumping. store (false);
	running. store (false);
}

	hackrfHandler::~hackrfHandler	(void) {
	stopReader ();
	hackrf_close (theDevice);
	hackrf_exit ();
}
//

void	hackrfHandler::setLNAGain	(int newGain) {
int	res;
	if ((newGain <= 40) && (newGain >= 0)) {
	   res	= hackrf_set_lna_gain (theDevice, newGain);
	   if (res != HACKRF_SUCCESS) {
	      fprintf (stderr, "Problem with hackrf_lna_gain :\n");
	      fprintf (stderr, "%s \n", hackrf_error_name (hackrf_error (res)));
	      return;
	   }
	}
}

void	hackrfHandler::setVGAGain	(int newGain) {
int	res;
	if ((newGain <= 62) && (newGain >= 0)) {
	   res	= hackrf_set_vga_gain (theDevice, newGain);
	   if (res != HACKRF_SUCCESS) {
	      fprintf (stderr, "Problem with hackrf_vga_gain :\n");
	      fprintf (stderr, "%s \n",
	                 hackrf_error_name (hackrf_error (res)));
	      return;
	   }
	}
}
//
//	we use a static large buffer, rather than trying to allocate
//	a buffer on the stack
static std::complex<float>buffer [32 * 32768];
static
int	callback (hackrf_transfer *transfer) {
hackrfHandler *ctx = static_cast <hackrfHandler *>(transfer -> rx_ctx);
int	i;
uint8_t *p	= transfer -> buffer;
RingBuffer<std::complex<float> > * q = ctx -> _I_Buffer;

	for (i = 0; i < transfer -> valid_length / 2; i ++) {
	   float re	= (((int8_t *)p) [2 * i]) / 128.0;
	   float im	= (((int8_t *)p) [2 * i + 1]) / 128.0;
	   buffer [i]	= std::complex<float> (re, im);
	}
	q -> putDataIntoBuffer (buffer, transfer -> valid_length / 2);
	if (ctx -> dumping. load ())
	   ctx -> xmlWriter -> add ((std::complex<int8_t> *)p,
	                                      transfer -> valid_length / 2);
	return 0;
}

bool	hackrfHandler::restartReader	(int32_t newFrequency) {
int	res;

	if (running. load ())
	   return true;

	res     = hackrf_set_freq (theDevice, newFrequency);
        if (res != HACKRF_SUCCESS) {
           fprintf (stderr, "Problem with hackrf_set_freq: \n");
           fprintf (stderr, "%s \n", hackrf_error_name (hackrf_error (res)));
           return false;
        }
        vfoFrequency = newFrequency;

	res	= hackrf_start_rx (theDevice, callback, this);	
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_start_rx :\n");
	   fprintf (stderr, "%s \n", hackrf_error_name (hackrf_error (res)));
	   return false;
	}
//
//	reset the gain(s), since the amp-enable is to be reset
//	after each change in frequency
	if ((vgaGain <= 62) && (vgaGain >= 0)) 
           (void)hackrf_set_vga_gain (theDevice, vgaGain);
	if ((lnaGain <= 40) && (lnaGain >= 0)) 
           (void)hackrf_set_lna_gain (theDevice, lnaGain);
	(void)hackrf_set_amp_enable (theDevice, ampEnable);
	running. store (hackrf_is_streaming (theDevice));
	return running. load ();
}

void	hackrfHandler::stopReader	(void) {
int	res;

	if (!running. load ())
	   return;

	res	= hackrf_stop_rx (theDevice);
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_stop_rx :");
	   fprintf (stderr, "%s \n", hackrf_error_name (hackrf_error (res)));
	   return;
	}
	running. store (false);
}

void	hackrfHandler::resetBuffer	(void) {
	_I_Buffer	-> FlushRingBuffer ();
}

int16_t	hackrfHandler::bitDepth	(void) {
	return 8;
}

void	hackrfHandler::startDumping	(const std::string &fileName) {
        xmlFile	= fopen (fileName. c_str (), "w");
	if (xmlFile == nullptr)
	   return;
	
	xmlWriter	= new xml_fileWriter (xmlFile,
	                                      8,
	                                      "int8",
	                                      2048000,
	                                      vfoFrequency,
	                                      "Hackrf",
	                                      "--",
	                                      recorderVersion);
	dumping. store (true);
}

void	hackrfHandler::stopDumping	() {
	if (xmlFile == nullptr)	// this can happen !!
	   return;
	dumping. store (false);
	usleep (1000);
	xmlWriter	-> print_xmlHeader ();
	delete xmlWriter;
	fclose (xmlFile);
}

