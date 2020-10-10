#
/*
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the dab cmdline
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

#ifndef __PLUTO_HANDLER__
#define	__PLUTO_HANDLER__

#include	<atomic>
#include	<iio.h>
#include	<thread>
#include	<sndfile.h>
#include	"ringbuffer.h"
#include	"device-handler.h"

#define DUMP_SIZE       4096

#define	RX_RATE		2112000
#define	DAB_RATE	2048000
#define	FM_RATE		2112000
#define	DIVIDER		1000
#define	CONV_SIZE	(RX_RATE / DIVIDER)

struct stream_cfg {
	long long bw_hz;
	long long fs_hz;
	long long lo_hz;
	const char *rfport;
};

class	plutoHandler: public deviceHandler {
public:
			plutoHandler		(RingBuffer<std::complex<float>> *,
	                                         int	frequency,
	                                         int	gain,
	                                         bool	agc);
	    		~plutoHandler		();
	bool		restartReader		(int32_t);
	void		stopReader		();
	void		startDumping		(std::string, uint32_t);
	void		stopDumping		();
	int16_t		bitDepth		();
private:

	std::thread		threadHandle;
	void			run		();
	int32_t			inputRate;
	std::atomic<bool>	running;
	struct	iio_device	*rx;
	struct	iio_context	*ctx;
	struct	iio_channel	*rx0_i;
	struct	iio_channel	*rx0_q;
	struct	iio_buffer	*rxbuf;
	struct	iio_buffer	*txbuf;
	struct	stream_cfg	rx_cfg;
	std::complex<float>	convBuffer	[CONV_SIZE + 1];
	int			convIndex;
	int16_t			mapTable_int	[DAB_RATE / DIVIDER];
	float			mapTable_float	[DAB_RATE / DIVIDER];
	SNDFILE			*outFile;
        std::atomic<bool>       dumping;
        int16_t			dumpBuffer [DUMP_SIZE];
        int			dumpIndex;
};
#endif

