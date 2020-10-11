#
/*
 *    Copyright (C) 2017 .. 2018
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

#ifndef __HACKRF_HANDLER__
#define	__HACKRF_HANDLER__

#include	"ringbuffer.h"
#include	<atomic>
#include	"device-handler.h"
#include	"libhackrf/hackrf.h"

class	xml_fileWriter;
typedef int (*hackrf_sample_block_cb_fn)(hackrf_transfer *transfer);


///////////////////////////////////////////////////////////////////////////
class	hackrfHandler: public deviceHandler {
public:
			hackrfHandler		(RingBuffer<std::complex<float>> *,
	                                         const std::string &,
	                                         int32_t frequency,
	                                         int16_t  ppm,
                                                 int16_t  lnaGain,
                                                 int16_t  vgaGain,
	                                         bool	ampEnable = false);
			~hackrfHandler		(void);
	bool		restartReader		(int32_t);
	void		stopReader		(void);
	void		resetBuffer		(void);
	int16_t		bitDepth		(void);
	void		startDumping		(const std::string &);
	void		stopDumping		();
//
//	The buffer should be visible by the callback function
	RingBuffer<std::complex<float>>	*_I_Buffer;
	hackrf_device	*theDevice;
	std::atomic<bool>	dumping;
	xml_fileWriter	*xmlWriter;
private:
	std::string	recorderVersion;
	FILE		*xmlFile;
	int32_t		vfoFrequency;
	int16_t		lnaGain;
	int16_t		vgaGain;
	bool		ampEnable;
	int32_t		inputRate;
	std::atomic<bool>	running;
	void		setLNAGain		(int);
	void		setVGAGain		(int);
};
#endif

