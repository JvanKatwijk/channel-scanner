#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the dab-cmdline
 *
 *    dab-cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dab-cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab-cmdline; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"sdrplay-handler.h"
#include	<unistd.h>

	sdrplayHandler::sdrplayHandler  (RingBuffer<std::complex<float>> *b,
	                                 int32_t	frequency,
	                                 int16_t	ppm,
	                                 int16_t	GRdB,
	                                 int16_t	lnaState,
	                                 bool		autoGain,
	                                 uint16_t	deviceIndex,
	                                 int16_t	antenna):
	                                    deviceHandler (b) {
int	err;
float	ver;
mir_sdr_DeviceT devDesc [4];
int	maxlna;

	this	-> _I_Buffer		= b;
        this    -> frequency            = frequency;
        this    -> ppmCorrection        = ppm;
        this    -> GRdB			= GRdB;
	if ((GRdB < 20) || (GRdB > 59))
	   GRdB = 30;
	this	-> lnaState		= lnaState;
        this    -> deviceIndex          = deviceIndex;
        this    -> agcMode		= autoGain ?
	                                     mir_sdr_AGC_100HZ :
	                                     mir_sdr_AGC_DISABLE;

	this	-> inputRate		= 2048000;
	err		= mir_sdr_ApiVersion (&ver);
	if (ver < 2.13) {
	   fprintf (stderr, "please upgrade to sdrplay library 2.13\n");
	   throw (24);
	}
	(void)err;

	mir_sdr_GetDevices (devDesc, &numofDevs, uint32_t (4));
	if (numofDevs == 0) {
	   fprintf (stderr, "Sorry, no device found\n");
	   throw (25);
	}

	if (deviceIndex >= numofDevs)
	   this -> deviceIndex = 0;
	hwVersion = devDesc [deviceIndex]. hwVer;
	fprintf (stderr, "sdrdevice found = %s, hw Version = %d\n",
	                              devDesc [deviceIndex]. SerNo, hwVersion);
	mir_sdr_SetDeviceIdx (deviceIndex);

	if (hwVersion == 2) {
	   if (antenna == 0) {
	      err = mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_A);
	   }
	   else {
	      err = mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_B);
	   }
	}
	if (hwVersion == 255) {
	   nrBits	= 14;
	   denominator	= 8192.0;
	   maxlna	= 9;
	}
        else 
	if (hwVersion == 1) {
           nrBits	= 12;
	   denominator	= 2048.0;
	   maxlna	= 3;
	}
	else
	if (hwVersion == 2) {
           nrBits	= 12;
	   denominator	= 2048.0;
	   maxlna	= 8;
	}
	else {
           nrBits	= 12;
	   denominator	= 2048.0;
	   maxlna	= 9;
	}

	if (lnaState < 0) 
	   lnaState = 0;
	if (lnaState > maxlna)
	   lnaState = maxlna;
	
        mir_sdr_AgcControl (autoGain ?
                         mir_sdr_AGC_100HZ :
                         mir_sdr_AGC_DISABLE, - GRdB, 0, 0, 0, 0, lnaState);
        if (!autoGain)
           mir_sdr_RSP_SetGr (GRdB, lnaState, 1, 0);

	dumping. store (false);
	dumpIndex	= 0;
	running. store (false);
}

	sdrplayHandler::~sdrplayHandler	(void) {
	stopReader ();
	if (numofDevs > 0)
	   mir_sdr_ReleaseDeviceIdx ();
}
//
static
void myStreamCallback (int16_t		*xi,
	               int16_t		*xq,
	               uint32_t		firstSampleNum, 
	               int32_t		grChanged,
	               int32_t		rfChanged,
	               int32_t		fsChanged,
	               uint32_t		numSamples,
	               uint32_t		reset,
	               uint32_t		hwRemoved,
	               void		*cbContext) {
int16_t	i;
sdrplayHandler	*p	= static_cast<sdrplayHandler *> (cbContext);
std::complex<float> localBuf [numSamples];
float	denominator		= p -> denominator;

	if (reset || hwRemoved)
	   return;
	for (i = 0; i <  (int)numSamples; i ++) {
	   localBuf [i] = std::complex<float> (float (xi [i]) / denominator,
	                                       float (xq [i]) / denominator);
	   if (p -> dumping. load ()) {
	      p -> dumpBuffer [2 * p -> dumpIndex ]	= xi [i];
	      p -> dumpBuffer [2 * p -> dumpIndex + 1]	= xq [i];
	      p -> dumpIndex ++;
	      if (p -> dumpIndex >= DUMP_SIZE / 2) {
	         sf_writef_short (p -> outFile,
	                          p -> dumpBuffer, p -> dumpIndex);
	         p -> dumpIndex = 0;
	      }
	   }
	}
	p -> _I_Buffer -> putDataIntoBuffer (localBuf, numSamples);
	(void)	firstSampleNum;
	(void)	grChanged;
	(void)	rfChanged;
	(void)	fsChanged;
	(void)	reset;
}

void	myGainChangeCallback (uint32_t	gRdB,
	                      uint32_t	lnaGRdB,
	                      void	*cbContext) {
	(void)gRdB;
	(void)lnaGRdB;	
	(void)cbContext;
}

bool	sdrplayHandler::restartReader	(int32_t frequency) {
int	gRdBSystem;
int	samplesPerPacket;
mir_sdr_ErrT	err;
int	localGRed	= GRdB;

	if (running. load ())
	   return true;

	this	-> frequency = frequency;
	err	= mir_sdr_StreamInit (&localGRed,
	                              double (inputRate) / 1000000.0,
	                              double (frequency) / 1000000.0,
	                              mir_sdr_BW_1_536,
	                              mir_sdr_IF_Zero,
	                              lnaState,	// lnaEnable do not know yet
	                              &gRdBSystem,
	                              mir_sdr_USE_RSP_SET_GR,
	                              &samplesPerPacket,
	                              (mir_sdr_StreamCallback_t)myStreamCallback,
	                              (mir_sdr_GainChangeCallback_t)myGainChangeCallback,
	                              this);
	if (err != mir_sdr_Success) {
	   fprintf (stderr, "Error %d on streamInit\n", err);
	   return false;
	}

//	mir_sdr_SetPpm       ((float)ppmCorrection);
        err             = mir_sdr_SetDcMode (4, 1);
        err             = mir_sdr_SetDcTrackTime (63);
        running. store (true);
        return true;
}

void	sdrplayHandler::stopReader	(void) {
	if (!running. load ())
	   return;

	mir_sdr_StreamUninit	();
	running. store (false);
}

void	sdrplayHandler::startDumping	(std::string s, uint32_t ensembleId) {
SF_INFO sf_info;
time_t now;
        time (&now);
        char buf [sizeof "2020-09-06-08T06:07:09Z"];
        strftime (buf, sizeof (buf), "%F %T", gmtime (&now));
//      strftime (buf, sizeof (buf), "%FT%TZ", gmtime (&now));
        std::string timeString = buf;
        std::string fileName = s + " " + toHex (ensembleId) + " " + timeString + ".sdr";

	if (dumping. load ())
	   return;
	sf_info. samplerate	= inputRate;
        sf_info. channels	= 2;
        sf_info. format		= SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        outFile = sf_open (fileName. c_str(), SFM_WRITE, &sf_info);
	if (outFile == nullptr) {
	   fprintf (stderr, "could not open %s\n", fileName. c_str ());
	   return;
	}
	dumping. store (true);
}

void	sdrplayHandler::stopDumping	() {
	if (!dumping. load ())
	   return;
	dumping. store (false);
	usleep (1000);
	sf_close (outFile);
}

