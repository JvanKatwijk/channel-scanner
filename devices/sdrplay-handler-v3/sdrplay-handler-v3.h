#
/*
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of channel scanner
 *
 *    channel scanner is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    channel scanner is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with channel scanner; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SDRPLAY_HANDLER_V3__
#define	__SDRPLAY_HANDLER_V3__

#include        <stdint.h>
#include        <thread>
#include        <atomic>
#include        <vector>
#include	<sndfile.h>
#include	"dab-constants.h"
#include	"ringbuffer.h"
#include	"device-handler.h"
#include	<sdrplay_api.h>


#define	DUMP_SIZE	4096
class	sdrplayHandler_v3: public deviceHandler {
public:
		sdrplayHandler_v3 (RingBuffer<std::complex<float>> *,
	                           uint32_t	frequency,
                                   int16_t	ppmCorrection,
                                   int16_t	GRdB,
                                   int16_t	lnaState,
                                   bool		autogain,
                                   uint16_t	deviceIndex,
                                   int16_t	antenna);

			~sdrplayHandler_v3	();
	bool		restartReader           (int32_t);
        void		stopReader              (void);
	void		startDumping		(std::string,
	                                              uint32_t ensembleId);
	void		stopDumping		();
//	The following items should be visible from outsize
//	the callback functions refer to them
        RingBuffer<std::complex<float>> *_I_Buffer;
        float		denominator;
	SNDFILE		*outFile;
	int16_t		dumpBuffer [DUMP_SIZE];
	int		dumpIndex;
        void		update_PowerOverload (sdrplay_api_EventParamsT *params);
        std::atomic<bool>       running;
	std::atomic<bool>	dumping;
private:
        void                    run             ();
        sdrplay_api_DeviceT             *chosenDevice;
        sdrplay_api_DeviceParamsT       *deviceParams;
        sdrplay_api_CallbackFnsT        cbFns;
        sdrplay_api_RxChannelParamsT    *chParams;
        std::thread                     threadHandle;

	int			inputRate;
        bool                    failFlag;
        int16_t                 hwVersion;
        int32_t                 vfoFrequency;
        int16_t                 ppmCorrection;
        int16_t                 GRdB;
        int16_t                 lnaState;
        bool                    autogain;
        uint16_t                deviceIndex;
	int16_t			antenna;
	int16_t			nrBits;
};
#endif

