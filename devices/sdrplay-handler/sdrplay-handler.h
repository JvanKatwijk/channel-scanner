#
/*
 *    Copyright (C) 2017 .. 2020
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

#ifndef __SDRPLAY_HANDLER__
#define	__SDRPLAY_HANDLER__

#include	<dlfcn.h>
#include	<atomic>
#include	<string>
#include	<sndfile.h>
#include	"ringbuffer.h"
#include	"device-handler.h"
#include	"mirsdrapi-rsp.h"

#define	DUMP_SIZE	4096
typedef void (*mir_sdr_StreamCallback_t)(int16_t	*xi,
	                                 int16_t	*xq,
	                                 uint32_t	firstSampleNum, 
	                                 int32_t	grChanged,
	                                 int32_t	rfChanged,
	                                 int32_t	fsChanged,
	                                 uint32_t	numSamples,
	                                 uint32_t	reset,
	                                 uint32_t	hwRemoved,
	                                 void		*cbContext);
typedef	void	(*mir_sdr_GainChangeCallback_t)(uint32_t	gRdB,
	                                        uint32_t	lnaGRdB,
	                                        void		*cbContext);

///////////////////////////////////////////////////////////////////////////
class	sdrplayHandler: public deviceHandler {
public:
		sdrplayHandler          (RingBuffer<std::complex<float>> *,
	                                 int32_t        frequency,
	                                 int16_t        ppmCorrection,
	                                 int16_t	GRdB,
	                                 int16_t	lnaState,
	                                 bool		autogain,
	                                 uint16_t       deviceIndex,
	                                 int16_t        antenna);

		~sdrplayHandler		(void);

	bool	restartReader		(int32_t);
	void	stopReader		(void);
	void	startDumping		(std::string, uint32_t ensembleId);
	void	stopDumping		();
	int16_t	bitDepth		();
//	need to be visible, since being accessed from 
//	within the callback
	RingBuffer<std::complex<float>>	*_I_Buffer;
	float		denominator;
	SNDFILE		*outFile;
        std::atomic<bool>       dumping;
        int16_t		dumpBuffer [DUMP_SIZE];
        int             dumpIndex;
private:
	int16_t		hwVersion;
	int16_t		nrBits;
	uint16_t	deviceIndex;
	uint32_t	numofDevs;	// int32_t not my choice
	int32_t		inputRate;
	int32_t		frequency;
	int16_t		ppmCorrection;
	int16_t		GRdB;
	int16_t		lnaState;
	std::atomic<bool>	running;
	mir_sdr_AgcControlT agcMode;
};
#endif

