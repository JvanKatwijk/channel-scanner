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
#include	"dab-processor.h"
#include	"device-handler.h"
#include	"timesyncer.h"
#include	"dab-api.h"

/**
  *	\brief dabProcessor
  *	The dabProcessor class is the driver of the processing
  *	of the samplestream.
  */

	dabProcessor::dabProcessor	(RingBuffer<std::complex<float>> *buffer,
	                                 uint8_t	dabMode,
	                                 callbacks	*the_callBacks,
	                                 void		*userData):
	                                    params (dabMode),
	                                    myReader (this, buffer),
	                                    phaseSynchronizer (dabMode,
	                                                       DIFF_LENGTH),
	                                    my_ofdmDecoder (dabMode),
	                                    my_ficHandler (dabMode,
	                                                   the_callBacks, 
	                                                   userData),
	                                    my_tiiDetector (dabMode) {
	this	-> the_callBacks	= the_callBacks;
	this	-> userData		= userData;
	this	-> T_null		= params. get_T_null ();
	this	-> T_s			= params. get_T_s ();
	this	-> T_u			= params. get_T_u ();
	this	-> T_g			= params. get_T_g();
	this	-> T_F			= params. get_T_F ();
	this	-> nrBlocks		= params. get_L ();
	this	-> carriers		= params. get_carriers ();
	this	-> carrierDiff		= params. get_carrierDiff ();
	isSynced			= false;
	snr				= 0;
	mainId				= -1;
	subId				= -1;
	running. store (false);
}

	dabProcessor::~dabProcessor	() {
	stop ();
}

void	dabProcessor::start	(void) {
	if (running. load ())
	   return;
	threadHandle	= std::thread (&dabProcessor::run, this);
}

void	dabProcessor::run	(void) {
std::complex<float>	FreqCorr;
timeSyncer      myTimeSyncer (&myReader);
int32_t		i;
float		fineOffset		= 0;
float		coarseOffset		= 0;
bool		correctionNeeded	= true;
std::vector<complex<float>>	ofdmBuffer (T_null);
int		dip_attempts		= 0;
int		index_attempts		= 0;
int		startIndex		= -1;
int		tii_counter		= 0;
int		tii_delay		= 4;

	isSynced	= false;
	snr		= 0;
	running. store (true);
	my_ficHandler. reset ();
	myReader. setRunning (true);

	try {
	   myReader. reset ();
	   for (i = 0; i < T_F / 2; i ++) {
	      jan_abs (myReader. getSample (0));
	   }

notSynced:
//Initing:
           switch (myTimeSyncer. sync (T_null, T_F)) {
              case TIMESYNC_ESTABLISHED:
                 break;                 // yes, we are ready

              case NO_DIP_FOUND:
                 if  (++ dip_attempts >= 10) {
                    the_callBacks -> signalHandler (false, userData);
                    dip_attempts = 0;
                 }
                 goto notSynced;

              default:                  // does not happen
              case NO_END_OF_DIP_FOUND:
                 goto notSynced;
           }

	   myReader. getSamples (ofdmBuffer. data (),
	                         T_u, coarseOffset + fineOffset);

	   startIndex = phaseSynchronizer.
	                        findIndex (ofdmBuffer. data (), THRESHOLD);
	   if (startIndex < 0) { // no sync, try again
	      isSynced	= false;
	      if (++index_attempts > 10) {
	         the_callBacks -> signalHandler (false, userData);
	         index_attempts	= 0;
	      }
//	      fprintf (stderr, "startIndex %d\n", startIndex);
	      goto notSynced;
	   }

	   index_attempts	= 0;
	   goto SyncOnPhase;

Check_endofNull:
//	when we are here, we had a (more or less) decent frame,
//	and we are ready for the new one.
//	we just check that we are around the end of the null period

	   myReader. getSamples (ofdmBuffer. data (),
	                         T_u, coarseOffset + fineOffset);
	   startIndex = phaseSynchronizer.
	                       findIndex (ofdmBuffer. data (), 4 * THRESHOLD);
	   if (startIndex < 0) { // no sync, try again
	      isSynced	= false;
	      goto notSynced;
	   }

SyncOnPhase:
	   index_attempts	= 0;
	   dip_attempts		= 0;
	   isSynced		= true;
	   the_callBacks -> signalHandler (isSynced, userData);

//	Once here, we are synchronized, we need to copy the data we
//	used for synchronization for block 0

	   memmove (ofdmBuffer. data (),
	            &((ofdmBuffer. data ()) [startIndex]),
	                  (T_u - startIndex) * sizeof (std::complex<float>));
	   int ofdmBufferIndex	= T_u - startIndex;

//	Block 0 is special in that it is used for coarse time synchronization
//	and its content is used as a reference for decoding the
//	first datablock.
//	We read the missing samples in the ofdm buffer
	   myReader. getSamples (&((ofdmBuffer. data ()) [ofdmBufferIndex]),
	                  T_u - ofdmBufferIndex,
	                  coarseOffset + fineOffset);
	   my_ofdmDecoder. processBlock_0	(ofdmBuffer. data ());
//
//	if correction is needed (known by the fic handler)
//	we compute the coarse offset in the phaseSynchronizer
	   correctionNeeded = !my_ficHandler. syncReached ();
	   if (correctionNeeded) {
	      int correction  = phaseSynchronizer.
	                                  estimateOffset (ofdmBuffer. data ());
	      if (correction != 100) {
	         coarseOffset += correction * carrierDiff;
	         if (abs (coarseOffset) > Khz (35))
	            coarseOffset = 0;
	      }
	   }
//
//	after block 0, we will just read in the other (params -> L - 1) blocks
//	The first ones are the FIC blocks. We immediately
//	start with building up an average of the phase difference
//	between the samples in the cyclic prefix and the
//	corresponding samples in the datapart.
///	and similar for the (params. L - 4) MSC blocks
	   FreqCorr		= std::complex<float> (0, 0);
	   std::vector<int16_t> ibits (2 * params. get_carriers ());
	   for (int ofdmSymbolCount = 1;
	        ofdmSymbolCount < (uint16_t)nrBlocks; ofdmSymbolCount ++) {	
	      myReader. getSamples (ofdmBuffer. data (),
	                               T_s, coarseOffset + fineOffset);
	      for (i = (int)T_u; i < (int)T_s; i ++) 
	         FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
//
//	Note that only the first few blocks are handled locally
//	The FIC/FIB handling is in this thread, so that there is
//	no delay is "knowing" that we are synchronized
	      if (ofdmSymbolCount < 4) {
	         my_ofdmDecoder. decode (ofdmBuffer. data (),
	                                 ofdmSymbolCount, ibits. data ());
	         my_ficHandler. process_ficBlock (ibits, ofdmSymbolCount);
	      }
	   }

//	we integrate the newly found frequency error with the
//	existing frequency error.
	   fineOffset += 0.1 * arg (FreqCorr) / M_PI * (carrierDiff);

//	at the end of the frame, just skip Tnull samples
	   myReader. getSamples (ofdmBuffer. data (),
	                         T_null, coarseOffset + fineOffset);
	   float sum	= 0;
	   for (i = 0; i < T_null; i ++)
	      sum += abs (ofdmBuffer [i]);
	   sum /= T_null;

	   float sum2 = myReader. get_sLevel ();
	   snr	= 0.9 * snr + 0.1 * 20 * log10 ((sum2 + 0.005) / sum);

	   if (wasSecond (my_ficHandler. get_CIFcount(), &params)) {
	      my_tiiDetector. addBuffer (ofdmBuffer);
	      if (++tii_counter >= tii_delay) {
	         my_tiiDetector. processNULL (&mainId, &subId);
	         tii_counter = 0;
	         my_tiiDetector. reset ();
	      }
	   }

	   if (fineOffset > carrierDiff / 2) {
	      coarseOffset += carrierDiff;
	      fineOffset -= carrierDiff;
	   }
	   else
	   if (fineOffset < - carrierDiff / 2) {
	      coarseOffset -= carrierDiff;
	      fineOffset += carrierDiff;
	   }
	   goto Check_endofNull;
	}
	
	catch (int e) {
	   fprintf (stderr, "dab processor will stop\n");
	}

//	fprintf (stderr, "dabProcessor is shutting down\n");
}

void	dabProcessor:: reset	(void) {
	stop  ();
	start ();
}

void	dabProcessor::stop	(void) {	
	if (running. load ()) {
	   running. store (false);
	   myReader. setRunning (false);
	   sleep (1);
	   threadHandle. join ();
	}
}

void    dabProcessor::dataforAudioService	(std::string s,audiodata *dd) {
        my_ficHandler. dataforAudioService (s, dd, 0);
}

void    dabProcessor::dataforAudioService	(std::string s,
                                                  audiodata *d, int16_t c) {
        my_ficHandler. dataforAudioService (s, d, c);
}

void    dabProcessor::dataforDataService	(std::string s,
	                                         packetdata *d, int16_t c) {
        my_ficHandler. dataforDataService (s, d, c);
}

int32_t	dabProcessor::get_SId		(std::string s) {
	return my_ficHandler. SIdFor (s);
}

uint16_t	dabProcessor::get_tiiData	() {
	if ((subId == -1) || (mainId == -1))
	   return 0;
	return (mainId << 8) | subId;
}

uint16_t	dabProcessor::get_snr	() {
	return snr;
}

void    dabProcessor::clearEnsemble     (void) {
	my_ficHandler. reset ();
}

bool    dabProcessor::wasSecond (int16_t cf, dabParams *p) {
        switch (p -> get_dabMode ()) {
           default:
           case 1:
              return (cf & 07) >= 4;
           case 2:
           case 3:
              return (cf & 02);
           case 4:
              return (cf & 03) >= 2;
        }
}

void    dabProcessor::startDumping      (SNDFILE *f, int dumpScale) {
        myReader. startDumping (f, dumpScale);
}

void    dabProcessor::stopDumping() {
        myReader. stopDumping();
}

