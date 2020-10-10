#
/*
 *    Copyright (C) 2016, 2017
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
#
#ifndef	__DAB_PROCESSOR__
#define	__DAB_PROCESSOR__
/*
 *
 */
#include	"dab-constants.h"
#include	<thread>
#include	<atomic>
#include	<stdint.h>
#include	<vector>
#include	"dab-params.h"
#include	"phasereference.h"
#include	"ofdm-decoder.h"
#include	"fic-handler.h"
#include	"ringbuffer.h"
#include	"dab-api.h"
#include	"sample-reader.h"
#include	"tii_detector.h"
//
class	deviceHandler;

class dabProcessor {
public:
		dabProcessor  	(RingBuffer<std::complex<float>> *,
	                         uint8_t,		// Mode
	                         callbacks	*,
	                         void		*);
	virtual ~dabProcessor	(void);
	void		reset			(void);
	void		stop			(void);
	void		start			(void);
	void		clearEnsemble           (void);
	uint16_t	get_tiiData		();
	uint16_t	get_snr			();
	void		startDumping		(SNDFILE *, int);
	void		stopDumping		();
	void		dataforAudioService	(std::string,   audiodata *);
        void		dataforAudioService	(std::string,
                                                     audiodata *, int16_t);
        void            dataforDataService      (std::string,   packetdata *);
        void            dataforDataService      (std::string,
                                                     packetdata *, int16_t);
	int32_t		get_SId			(std::string);
private:
//
	RingBuffer<std::complex<float>>	*_I_Buffer;
	callbacks	*the_callBacks;
	dabParams	params;
	sampleReader	myReader;
	phaseReference	phaseSynchronizer;
	ofdmDecoder	my_ofdmDecoder;
	ficHandler	my_ficHandler;
	tiiDetector	my_tiiDetector;
	
	std::thread	threadHandle;
	void		*userData;
	std::atomic<bool>	running;
	bool		isSynced;
	int		snr;
	int32_t		T_null;
	int32_t		T_u;
	int32_t		T_s;
	int32_t		T_g;
	int32_t		T_F;
	int32_t		nrBlocks;
	int32_t		carriers;
	int32_t		carrierDiff;
	bool		wasSecond	(int16_t, dabParams *);
	int16_t		mainId;
	int16_t		subId;
virtual	void		run		(void);
};
#endif

