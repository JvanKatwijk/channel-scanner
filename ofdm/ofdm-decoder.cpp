#
/*
 *    Copyright (C) 2013 .. 2017
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
 *
 *	Once the bits are "in", interpretation and manipulation
 *	should reconstruct the data blocks.
 *	Ofdm_decoder is called once every Ts samples, and
 *	its invocation results in 2 * Tu bits
 */
#include	"ofdm-decoder.h"
#include	"phasetable.h"
#include	"freq-interleaver.h"
#include	"dab-params.h"
#include	"fft_handler.h"

/**
  */
	ofdmDecoder::ofdmDecoder	(uint8_t	dabMode):
	                                     params (dabMode),
	                                     my_fftHandler (dabMode),
	                                     myMapper    (dabMode) {

	this	-> T_s			= params. get_T_s ();
	this	-> T_u			= params. get_T_u ();
	this	-> nrBlocks		= params. get_L ();
	this	-> carriers		= params. get_carriers ();
	this	-> T_g			= T_s - T_u;
	fft_buffer			= my_fftHandler. getVector ();
	phaseReference. resize (T_u);
}

	ofdmDecoder::~ofdmDecoder	(void) {
}

void	ofdmDecoder::processBlock_0 (std::complex<float> *buffer) {
	memcpy (fft_buffer, buffer,
	                      T_u * sizeof (std::complex<float>));

	my_fftHandler. do_FFT ();
/**
  *	we are now in the frequency domain, and we keep the carriers
  *	as coming from the FFT as phase reference.
  */
	memcpy (phaseReference. data (),
	               fft_buffer, T_u * sizeof (std::complex<float>));
}

void	ofdmDecoder::decode (std::complex<float> *buffer,
	                             int32_t blkno, int16_t *ibits) {
int16_t	i;
std::complex<float> conjVector [T_u];

      memcpy (fft_buffer, &(buffer[T_g]),
                                       T_u * sizeof (std::complex<float>));

//fftlabel:
/**
  *	first step: do the FFT
  */
	my_fftHandler. do_FFT ();
/**
  *	a little optimization: we do not interchange the
  *	positive/negative frequencies to their right positions.
  *	The de-interleaving understands this
  */
//toBitsLabel:
/**
  *	Note that from here on, we are only interested in the
  *	"carriers" useful carriers of the FFT output
  */
	for (i = 0; i < carriers; i ++) {
	   int16_t	index	= myMapper. mapIn (i);
	   if (index < 0) 
	      index += T_u;
/**
  *	decoding is computing the phase difference between
  *	carriers with the same index in subsequent blocks.
  *	The carrier of a block is the reference for the carrier
  *	on the same position in the next block
  */
	   std::complex<float>	r1 = fft_buffer [index] * conj (phaseReference [index]);
           conjVector [index] = r1;
//	The viterbi decoder expects values in the range 0 .. 255,
//	we present values -127 .. 127 (easy with depuncturing)
	   float ab1		= abs (r1);
	   ibits [i]		= - real (r1) / ab1 * 127.0;
	   ibits [carriers + i] = - imag (r1) / ab1 * 127.0;
	}

	memcpy (phaseReference. data (),
	          fft_buffer, T_u * sizeof (std::complex<float>));
}

