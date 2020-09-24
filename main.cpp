#
/*
 *    Copyright (C) 2015, 2016, 2017
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

#include	<unistd.h>
#include	<signal.h>
#include	<getopt.h>
#include        <cstdio>
#include        <iostream>
#include	<complex>
#include	<vector>
#include	"audiosink.h"
#include	<sndfile.h>
#include	"dab-api.h"
#include	"dab-processor.h"
#include	"band-handler.h"
#include	"ringbuffer.h"
#ifdef	HAVE_PLUTO
#include	"pluto-handler.h"
#elif	HAVE_SDRPLAY_V2
#include	"sdrplay-handler.h"
#elif	HAVE_SDRPLAY_V3
#include	"sdrplay-handler-v3.h"
#elif	HAVE_RTLSDR
#include	"rtlsdr-handler.h"
#endif
#include	"service-printer.h"
#include	<locale>
#include	<codecvt>
#include	<atomic>
#include	<string>
using std::cerr;
using std::endl;

void    printOptions (void);	// forward declaration
void	handleChannel (deviceHandler	*theDevice,
	               RingBuffer<std::complex<float>> * _I_Buffer,
	               uint8_t	Mode,
	               uint8_t		theBand,
	               std::string	theChannel,
	               int	timeSyncTime,
	               int	freqSyncTime,
	               int	duration,
	               FILE	*outFile,
	               bool	jsonOutput,
	               bool	firstEnsemble,
	               bool	dumping);
//	we deal with callbacks from different threads. So, if you extend
//	the functions, take care and add locking whenever needed
static
std::atomic<bool> run;

static
std::atomic<bool>timeSynced;

static
std::atomic<bool>timesyncSet;

static
std::atomic<bool>ensembleRecognized;
std::string     ensembleName;
uint32_t        ensembleId;
static
void    ensemblenameHandler (std::string name, int Id, void *userData) {
        fprintf (stderr, "ensemble %s is (%X) recognized\n",
                                  name. c_str (), (uint32_t)Id);
        ensembleRecognized. store (true);
        ensembleName    = name;
        ensembleId      = Id;
}

static
FILE	*outFile	= stdout;

static void sighandler (int signum) {
	fprintf (stderr, "Signal caught, terminating!\n");
	run. store (false);
}

std::vector<std::string> channelList;
static
void	syncsignalHandler (bool b, void *userData) {
	timeSynced. store (b);
	timesyncSet. store (true);
	(void)userData;
}
//
std::vector<std::string> programNames;
std::vector<int> programSIds;

#include	<bits/stdc++.h>

std::unordered_map <int, std::string> ensembleContents;
static
void	programnameHandler (std::string s, int SId, void *userdata) {
	for (std::vector<std::string>::iterator it = programNames.begin();
	             it != programNames. end(); ++it)
	   if (*it == s)
	      return;
	ensembleContents. insert (pair <int, std::string> (SId, s));
	programNames. push_back (s);
	programSIds . push_back (SId);
	std::cerr << "program " << s << " is part of the ensemble\n";
}

static
void	programdataHandler (audiodata *d, void *ctx) {
	(void)ctx;
	std::cerr << "\tstartaddress\t= " << d -> startAddr << "\n";
	std::cerr << "\tlength\t\t= "     << d -> length << "\n";
	std::cerr << "\tsubChId\t\t= "    << d -> subchId << "\n";
	std::cerr << "\tprotection\t= "   << d -> protLevel << "\n";
	std::cerr << "\tbitrate\t\t= "    << d -> bitRate << "\n";
}

//
//	The function is called from within the library with
//	a string, the so-called dynamic label
static
void	dataOut_Handler (std::string dynamicLabel, void *ctx) {
	(void)ctx;
	std::cerr << dynamicLabel << "\r";
}
//
//	The function is called from the MOT handler, with
//	as parameters the filename where the picture is stored
//	d denotes the subtype of the picture 
//	typedef void (*motdata_t)(std::string, int, void *);
void	motdataHandler (std::string s, int d, void *ctx) {
	(void)s; (void)d; (void)ctx;
//	fprintf (stderr, "plaatje %s\n", s. c_str ());
}

//
static
void	pcmHandler (int16_t *buffer, int size, int rate,
	                              bool isStereo, void *ctx) {
	(void)buffer;
	(void)size;
	(void)isStereo;
	(void)ctx;
}

static
void	systemData (bool flag, int16_t snr, int32_t freqOff, void *ctx) {
//	fprintf (stderr, "synced = %s, snr = %d, offset = %d\n",
//	                    flag? "on":"off", snr, freqOff);
}

static
void	fibQuality	(int16_t q, void *ctx) {
//	fprintf (stderr, "fic quality = %d\n", q);
}

static
void	mscQuality	(int16_t fe, int16_t rsE, int16_t aacE, void *ctx) {
//	fprintf (stderr, "msc quality = %d %d %d\n", fe, rsE, aacE);
}

int	main (int argc, char **argv) {
// Default values
uint8_t		theMode		= 1;
uint8_t		theBand		= BAND_III;
int		duration	= 10;		// seconds, default
#ifdef	HAVE_PLUTO
int16_t		gain		= 60;
bool		autogain	= false;
const char	*optionsString	= "RT:F:D:d:M:B:C:G:Q";
const char	deviceString	= "Compiled for Adalm Pluto";
#elif	HAVE_SDRPLAY_V2
int16_t		GRdB		= 30;
int16_t		lnaState	= 4;
bool		autogain	= true;
int16_t		ppmOffset	= 0;
const char	deviceString	= "Compiled for SDRPlay (2.13 library)";
const char	*optionsString	= "RF:T:D:d:M:B:C:G:L:Qp:";
#elif	HAVE_SDRPLAY_V3
int16_t		GRdB		= 30;
int16_t		lnaState	= 4;
bool		autogain	= true;
int16_t		ppmOffset	= 0;
const char	*deviceString	= "Compiled for SDRPlay (3.06/7 library)";
const char	*optionsString	= "RF:T:D:d:M:B:C:G:L:Qp:";
#elif	HAVE_AIRSPY
int16_t		gain		= 20;
bool		autogain	= false;
bool		rf_bias		= false;
int16_t		ppmOffset	= 0;
const char	deviceString	= "Compiled for AIRspy";
const char	*optionsString	= "RT:F:D:d:M:B:C:G:p:";
#elif	HAVE_RTLSDR
int16_t		gain		= 20;
bool		autogain	= false;
int16_t		ppmOffset	= 0;
const char	deviceString	= "Compiled for rtlsdr sticks";
const char	*optionsString	= "F:T:D:d:M:B:C:G:p:QR";
#endif
bool		dumping		= false;
int16_t		timeSyncTime	= 10;
int16_t		freqSyncTime	= 5;
bool		jsonOutput	= false;
int		opt;
struct sigaction sigact;
deviceHandler	*theDevice	= nullptr;
bool		firstEnsemble	= true;
RingBuffer<std::complex<float>> _I_Buffer (16 * 32768);

	std::cerr << "dab_channelScanner,\n \
	                Copyright 2020 J van Katwijk, Lazy Chair Computing\n";
	std::cerr << deviceString << "\n\
	          Software is provided AS IS and licensed under GPL V2\n";
	timeSynced.	store (false);
	timesyncSet.	store (false);
	run.		store (false);
//	std::wcout.imbue(std::locale("en_US.utf8"));
	if (argc == 1) {
	   printOptions ();
	   exit (1);
	}

	std::setlocale (LC_ALL, "en-US.utf8");

	fprintf (stderr, "options are %s\n", optionsString);
	while ((opt = getopt (argc, argv, optionsString)) != -1) {
	   switch (opt) {
	      case 'F':
	         outFile	= fopen (optarg, "w");
	         if (outFile == nullptr) {
	            fprintf (stderr, "cannot open %s\n", optarg);
	            exit (1);
	         }
	         break;

	      case 'D':
	         freqSyncTime	= atoi (optarg);
	         break;

	      case 'd':
	         timeSyncTime	= atoi (optarg);
	         break;

	      case 'M':
	         theMode	= atoi (optarg);
	         if (!((theMode == 1) || (theMode == 2) || (theMode == 4)))
	            theMode = 1; 
	         break;

	      case 'B':
	         theBand = std::string (optarg) == std::string ("L_BAND") ?
	                                                 L_BAND : BAND_III;
	         break;

	      case 'T':
	         duration	= atoi (optarg);
	         break;

	      case 'R':
	         dumping	= true;
	         break;

	      case 'C':
	         channelList. push_back (std::string (optarg));
	         fprintf (stderr, "%s \n", optarg);
	         break;

#ifdef	HAVE_PLUTO
	      case 'G':
	         gain		= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;


#elif	HAVE_SDRPLAY_V2
	      case 'G':
	         GRdB		= atoi (optarg);
	         break;

	      case 'L':
	         lnaState	= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;

	      case 'p':
	         ppmOffset	= atoi (optarg);
	         break;

#elif	HAVE_SDRPLAY_V3
	      case 'G':
	         GRdB		= atoi (optarg);
	         break;

	      case 'L':
	         lnaState	= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;

	      case 'p':
	         ppmOffset	= atoi (optarg);
	         break;

#elif	HAVE_RTLSDR
	      case 'G':
	         gain		= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;

	      case 'p':
	         ppmOffset	= atoi (optarg);
	         break;

#elif	HAVE_AIRSPY
	      case 'G':
	         gain		= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;

	      case 'b':
	         rf_bias	= true;
	         break;

	      case 'p':
	         ppmOffset	= atoi (optarg);
	         break;

#endif
	      default:
	         fprintf (stderr, "Option %c not understood\n", opt);
	         printOptions ();
	         exit (1);
	   }
	}
//
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;

	int32_t frequency	= MHz (220);	// just a dummy value
	try {
#ifdef	HAVE_SDRPLAY_V2
	   theDevice	= new sdrplayHandler (&_I_Buffer,
	                                      frequency,
	                                      ppmOffset,
	                                      GRdB,
	                                      lnaState,
	                                      autogain,
	                                      0,
	                                      0);
#elif	HAVE_SDRPLAY_V3
	   theDevice	= new sdrplayHandler_v3 (&_I_Buffer,
	                                      frequency,
	                                      ppmOffset,
	                                      GRdB,
	                                      lnaState,
	                                      autogain,
	                                      0,
	                                      0);
#elif	HAVE_AIRSPY
	   theDevice	= new airspyHandler (&_I_Buffer,
	                                     frequency,
	                                     ppmOffset,
	                                     gain,
	                                     rf_bias);
#elif	HAVE_PLUTO
	   theDevice	= new plutoHandler	(&_I_Buffer,
	                                         frequency,
	                                         gain,
	                                         autogain);
#elif	HAVE_RTLSDR
	   theDevice	= new rtlsdrHandler	(&_I_Buffer,
	                                         frequency,
	                                         ppmOffset,
	                                         gain,
	                                         autogain);
#endif

	}
	catch (int e) {
	   std::cerr << "allocating device failed (" << e << "), fatal\n";
	   exit (32);
	}
	if (theDevice == nullptr) {
	   fprintf (stderr, "no device selected, fatal\n");
	   exit (33);
	}
//
	for (uint16_t i = 0; i < channelList. size (); i ++) {
	   std::string theChannel = channelList. at (i);
	   handleChannel (theDevice,
	                  &_I_Buffer,
	                  theMode,
	                  theBand,
	                  theChannel,
	                  timeSyncTime,
	                  freqSyncTime,
	                  duration,
	                  outFile,
	                  jsonOutput,
	                  firstEnsemble,
	                  dumping
	                 );
	}

	theDevice	-> stopReader	();
	delete theDevice;	
}


void	handleChannel (deviceHandler *theDevice,
	               RingBuffer<std::complex<float>> *_I_Buffer,
	               uint8_t		theMode,
	               uint8_t		theBand,
	               std::string	theChannel,
	               int		timeSyncTime,
	               int		freqSyncTime,
	               int		duration,
	               FILE		*outFile,
	               bool		jsonOutput,
	               bool		firstEnsemble,
	               bool		dumping){
bool		firstService	= true;
bandHandler     dabBand;
int32_t frequency	= dabBand. Frequency (theBand, theChannel);
dabProcessor theRadio (_I_Buffer,
	               theMode,
	               syncsignalHandler,
	               systemData,
	               ensemblenameHandler,
	               programnameHandler,
	               fibQuality,
	               pcmHandler,
	               dataOut_Handler,
	               programdataHandler,
	               mscQuality,
	               motdataHandler,	// MOT in PAD
	               nullptr		// Ctx
	              );

	programNames. resize (0);
	programSIds. resize (0);

	theRadio. start ();
	theDevice	-> restartReader (frequency);

	print_fileHeader (outFile, jsonOutput);
	timesyncSet.		store (false);
	ensembleRecognized.	store (false);
	
	while (!timeSynced. load () && (--timeSyncTime >= 0))
	      sleep (1);

	if (!timeSynced. load ()) {
	   cerr << "There does not seem to be a DAB signal here" << endl;
	   theDevice -> stopReader ();
	   sleep (1);
	   theRadio. stop ();
	   return;
	}

	std::cerr << "there might be a DAB signal here" << endl;

	while ((--freqSyncTime >= 0)) {
	   std::cerr << freqSyncTime + 1 << "\r";
	   sleep (1);
	}
	std::cerr << "\n";

	if (!ensembleRecognized. load ()) {
	   std::cerr << "no ensemble data found, fatal\n";
	   theDevice -> stopReader ();
	   sleep (1);
	   theRadio. stop ();
	   return;
	}

	int avg_snr	= 0;
	std::vector<int> tii_data;
	if (dumping)
	   theDevice -> startDumping (theChannel, ensembleId);
	run. store (true);

	for (int i = 0; i < duration; i ++) {
	   fprintf (stderr, "we sleep\n");
	   sleep (1);
	   avg_snr	+= theRadio. get_snr ();
	   int tii	= theRadio. get_tiiData ();
	   if (tii != 0) {
	      uint16_t tii_index = 0;
	      for (tii_index = 0; tii_index < tii_data. size (); tii_index ++)
	         if (tii_data. at (tii_index) == tii)
	            break;
	      if (tii_index == tii_data. size ())
	         tii_data. push_back (tii);
	   }
	}


//	print ensemble data here
	print_ensembleData (outFile,
	                    jsonOutput,
                            &theRadio,
	                    theChannel,
	                    ensembleName,
	                    ensembleId,
	                    frequency / 1000,
	                    avg_snr / duration,	
	                    tii_data,
	                    &firstEnsemble);

	print_audioheader (outFile, jsonOutput);
	for (int i = 0; i < (int)(programNames. size ()); i ++) {
	   audiodata ad;
	   theRadio. dataforAudioService (programNames [i]. c_str (),
	                                                        &ad, 0);
	   if (ad. defined) {
	      print_audioService (outFile, jsonOutput, &theRadio,
	                          programNames [i]. c_str (), &ad,
	                          &firstService);
	      for (int j = 1; j < 5; j ++) {
	            packetdata pd;
	            theRadio. dataforDataService (programNames [i]. c_str (),
                                                                      &pd, j);
	            if (pd. defined)
	               print_dataService (outFile, jsonOutput, &theRadio,
                                          programNames [i]. c_str (), j, &pd,
	                                  &firstService);
	      }
	   }
	   firstService	= true;
	}
	for (int i = 0; i < (int)(programNames. size ()); i ++) {
	   packetdata pd;
	   theRadio. dataforDataService (programNames [i]. c_str (),
	                                                        &pd, 0);
	   if (pd. defined && firstService) {
	      print_dataHeader (outFile, jsonOutput);
	      firstService = false;
	   }

	   if (pd. defined) 
	      print_dataService (outFile, jsonOutput, &theRadio,
	                          programNames [i]. c_str (), i, &pd,
	                          &firstService);
	}

	print_ensembleFooter (outFile, jsonOutput);
	print_fileFooter (outFile, jsonOutput);
	theRadio. stop		();
	theDevice	-> stopDumping	();
	theDevice	-> stopReader	();
}

void    printOptions (void) {
	std::cerr << 
"                          dab-cmdline options are\n"
"	                  -T Duration\tstop after <Duration> seconds\n"
"	                  -M Mode\tMode is 1, 2 or 4. Default is Mode 1\n"
"	                  -D number\tamount of time to look for an ensemble\n"
"	                  -d number\tseconds to reach time sync\n"
"	for rtlsdr:\n"
"	                  -B Band\tBand is either L_BAND or BAND_III (default)\n"
"	                  -C Channel\n"
"	                  -G Gain in dB (range 0 .. 100)\n"
"	                  -Q autogain (default off)\n"
"	for pluto:\n"
"	                  -B Band\tBand is either L_BAND or BAND_III (default)\n"
"	                  -C Channel\n"
"	                  -G Gain in dB (range 0 .. 70)\n"
"	                  -Q autogain (default off)\n"
	
"	for SDRplay:\n"
"	                  -B Band\tBand is either L_BAND or BAND_III (default)\n"
"	                  -C Channel\n"
"	                  -G Gain reduction in dB (range 20 .. 59)\n"
"	                  -L lnaState (depends on model chosen)\n"
"	                  -Q autogain (default off)\n"
"	                  -p number\t ppm offset\n";
}
