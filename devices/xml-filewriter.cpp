#
/*
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of channelScanner
 *
 *    channelScanner is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    channelScanner is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with channelScanner; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	"xml-filewriter.h"
#include	<stdio.h>
#include	<time.h>

struct kort_woord {
	uint8_t byte_1;
	uint8_t byte_2;
};

	xml_fileWriter::xml_fileWriter (FILE *f,
	                                int		nrBits,
	                                std::string	container,
	                                int		sampleRate,
	                                int		frequency,	
	                                std::string	deviceName,
	                                std::string	deviceModel,
	                                std::string	recorderVersion) {
uint8_t t	= 0;
	xmlFile		= f;
	this	-> nrBits	= nrBits;
	this	-> container	= container;
	this	-> sampleRate	= sampleRate;
	this	-> frequency	= frequency;
	this	-> deviceName	= deviceName;
	this	-> deviceModel	= deviceModel;
	this	-> recorderVersion	= recorderVersion;

	for (int i = 0; i < 5000; i ++)
	   fwrite (&t, 1, 1, f);
	int16_t testWord	= 0xFF;

	struct kort_woord *p	= (struct kort_woord *)(&testWord);
	if (p -> byte_1  == 0xFF)
	   byteOrder	= "LSB";
	else
	   byteOrder	= "MSB";
	nrElements	= 0;
}

	xml_fileWriter::~xml_fileWriter	() {
	print_xmlHeader ();
}

void	xml_fileWriter::print_xmlHeader	() {
time_t rawtime;
struct tm *timeinfo;
time (&rawtime);
timeinfo	= localtime (&rawtime);

	fseek (xmlFile, 0, SEEK_SET);
	fprintf (xmlFile, "<?xml version=\"1.0\" encoding =\"utf-8\"?>\n");
	fprintf (xmlFile, "<SDR>\n");
	fprintf (xmlFile, "<Recorder Name =\"%s\" Version =\"%s\"/>\n",
	                        "channelScanner", recorderVersion. c_str ());
	fprintf (xmlFile, "<Device Name=\"%s\" Model=\"%s\"/>\n",
	                         deviceName. c_str (), deviceModel. c_str ());
	fprintf (xmlFile, "<Time Value=\"%s\" Unit=\"UTC\"/>\n",
	                                   asctime (timeinfo));
	fprintf (xmlFile, "<!-- The Sample Information -->\n");
	
	fprintf (xmlFile, "<Sample>\n");
	   fprintf (xmlFile, "<Samplerate Value=\"%s\" Unit=\"Hz\"/>\n",
	                               std::to_string (sampleRate). c_str ());
	   fprintf (xmlFile, "<Channels Bits=\"%s\" Container=\"%s\" Ordering=\"%s\">\n",
	           std::to_string (nrBits). c_str (),
	                     container. c_str (), byteOrder. c_str ());
	      fprintf (xmlFile, "<Channel Value=\"I\"/>\n");
	      fprintf (xmlFile, "<Channel Value=\"Q\"/>\n");
	   fprintf (xmlFile, "</Channels>\n");
	fprintf (xmlFile, "</Sample>\n");
//	fprintf (xmlFile, "<!-- the Datablocks, here only 1 -->\n");
	fprintf (xmlFile, "<Datablocks>\n");
	fprintf (xmlFile, "   <Datablock Count=\"%d\" Number=\"1\" Unit=\"Channel\">\n",
	                                     nrElements);
	   fprintf (xmlFile, "<Frequency Value=\"%d\" Unit=\"KHz\"/>\n",
	                                     frequency / 1000);
	   fprintf (xmlFile, "Modulation Value=\"DAB\"/>\n");
	fprintf (xmlFile, "</Datablock>\n");
	fprintf (xmlFile, "</Datablocks>\n");
	fprintf (xmlFile, "</SDR>\n");
}

#define	BLOCK_SIZE	8192
static int16_t buffer_int16 [BLOCK_SIZE];
static int bufferP_int16	= 0;
void	xml_fileWriter::add	(std::complex<int16_t> * data, int count) {
	nrElements	+= 2 * count;
	for (int i = 0; i < count; i ++) {
	   buffer_int16 [bufferP_int16 ++] = real (data [i]);
	   buffer_int16 [bufferP_int16 ++] = imag (data [i]);
	   if (bufferP_int16 >= BLOCK_SIZE) {
	      fwrite (buffer_int16, sizeof (int16_t), BLOCK_SIZE, xmlFile);
	      bufferP_int16 = 0;
	   }
	}
}

static uint8_t buffer_uint8 [BLOCK_SIZE];
static int bufferP_uint8	= 0;
void	xml_fileWriter::add	(std::complex<uint8_t> * data, int count) {
	nrElements	+= 2 * count;
	for (int i = 0; i < count; i ++) {
	   buffer_uint8 [bufferP_uint8 ++] = real (data [i]);
	   buffer_uint8 [bufferP_uint8 ++] = imag (data [i]);
	   if (bufferP_uint8 >= BLOCK_SIZE) {
	      fwrite (buffer_uint8, sizeof (uint8_t), BLOCK_SIZE, xmlFile);
	      bufferP_uint8 = 0;
	   }
	}
}

static int8_t buffer_int8 [BLOCK_SIZE];
static int bufferP_int8	= 0;
void	xml_fileWriter::add	(std::complex<int8_t> * data, int count) {
	nrElements	+= 2 * count;
	for (int i = 0; i < count; i ++) {
	   buffer_int8 [bufferP_int8 ++] = real (data [i]);
	   buffer_int8 [bufferP_int8 ++] = imag (data [i]);
	   if (bufferP_int8 >= BLOCK_SIZE) {
	      fwrite (buffer_int8, sizeof (int8_t), BLOCK_SIZE, xmlFile);
	      bufferP_int8 = 0;
	   }
	}
}

