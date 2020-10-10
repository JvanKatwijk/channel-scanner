#
/*
 *    Copyright (C) 2010, 2011, 2012
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
 *	We have to create a simple virtual class here, since we
 *	want the interface with different devices (including  filehandling)
 *	to be transparent
 */
#ifndef	__DEVICE_HANDLER__
#define	__DEVICE_HANDLER__

#include	<stdint.h>
#include	<complex>
#include	<thread>
#include	"ringbuffer.h"
using namespace std;

class	deviceHandler {
public:
			deviceHandler 	(RingBuffer<std::complex<float>> *);
virtual			~deviceHandler 	(void);
virtual		bool	restartReader	(int32_t);
virtual		void	stopReader	(void);
virtual		void	startDumping	(std::string, uint32_t);
virtual		void	stopDumping	();
virtual		int16_t	bitDepth	(void) { return 10;}
		std::string	toHex	(uint32_t);
//
protected:
		RingBuffer<std::complex<float>> *_I_Buffer;
		int32_t	vfoFrequency;
	        int32_t	vfoOffset;
	        int	theGain;
virtual		void	run		(void);
};
#endif

