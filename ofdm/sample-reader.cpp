#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library
 *
 *    DAB library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#
#include	"sample-reader.h"
#include	"device-handler.h"
#include	"dab-processor.h"

static
std::complex<float> oscillatorTable [INPUT_RATE];

	sampleReader::sampleReader (dabProcessor *parent,
	                            RingBuffer<std::complex<float>> *buffer
	                           ) {
int	i;
	theParent		= parent;
	this	-> _I_Buffer	= buffer;
	currentPhase		= 0;
	sLevel			= 0;
	sampleCount		= 0;
	for (i = 0; i < INPUT_RATE; i ++)
	   oscillatorTable [i] = std::complex<float>
	                            (cos (2.0 * M_PI * i / INPUT_RATE),
	                             sin (2.0 * M_PI * i / INPUT_RATE));

	corrector	= 0;
	running. store (true);
}

	sampleReader::~sampleReader (void) {
}

void	sampleReader::reset	(void) {
	currentPhase            = 0;
	sLevel                  = 0;
	sampleCount             = 0;
}

void	sampleReader::setRunning (bool b) {
	running. store (b);
}

float	sampleReader::get_sLevel (void) {
	return sLevel;
}

std::complex<float> sampleReader::getSample (int32_t phaseOffset) {
std::complex<float> temp;

	if (!running. load ())
	   throw 21;

	while (running. load () &&
	      (_I_Buffer -> GetRingBufferReadAvailable () < 2048))
	      usleep (100);

	if (!running. load ())	
	   throw 20;
//
	_I_Buffer -> getDataFromBuffer (&temp, 1);

//	OK, we have a sample!!
//	first: adjust frequency. We need Hz accuracy
	if (phaseOffset != 0) {
	   currentPhase	-= phaseOffset;
	   currentPhase	= (currentPhase + INPUT_RATE) % INPUT_RATE;

	   temp		*= oscillatorTable [currentPhase];
	}
	sLevel		= 0.00001 * jan_abs (temp) + (1 - 0.00001) * sLevel;
#define	N	5
	sampleCount	++;
	if (++ sampleCount > INPUT_RATE / N) {
	   sampleCount = 0;
	   theParent -> show_Corrector (phaseOffset);
	}
	return temp;
}

void	sampleReader::getSamples (std::complex<float>  *v,
	                          int32_t n, int32_t Offset) {
int32_t		i;

	while (running. load () &&
	       (_I_Buffer -> GetRingBufferReadAvailable () < n))
	   usleep (100);

	if (!running. load ())	
	   throw 20;
//
	n = _I_Buffer -> getDataFromBuffer (v, n);

//	OK, we have samples!!
//	first: adjust frequency. We need Hz accuracy
	for (i = 0; i < n; i ++) {
	   if (Offset != 0) {
	      currentPhase	-= Offset;
//
//	Note that "phase" itself might be negative
	      currentPhase	= (currentPhase + INPUT_RATE) % INPUT_RATE;
	      v [i]	*= oscillatorTable [currentPhase];
	   }
	   sLevel	= 0.00001 * jan_abs (v [i]) + (1 - 0.00001) * sLevel;
	}

	sampleCount	+= n;
	if (sampleCount > INPUT_RATE / N) {
	   theParent -> show_Corrector (Offset);
	   sampleCount = 0;
	}
}

