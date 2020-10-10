#
/*
 *    Copyright (C) 2013 .. 2017
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
/*
 * 	FIC data
 */
#ifndef	__FIC_HANDLER__
#define	__FIC_HANDLER__

#include	<stdio.h>
#include	<stdint.h>
#include	<vector>
#include	"viterbi-spiral.h"
#include	"fib-processor.h"
#include	<mutex>
#include	<string>
#include	"dab-api.h"
#include	"dab-params.h"

class ficHandler: public viterbiSpiral {
//class ficHandler: public viterbiHandler {
public:
		ficHandler		(uint8_t,	// dabMode
	                                 callbacks	*,
	                                 void		*);
		~ficHandler		(void);
	void	process_ficBlock	(std::vector<int16_t>, int16_t);
	void	clearEnsemble		(void);
	bool	syncReached		(void);
	void	dataforDataService	(std::string &, packetdata *, int);
        void	dataforAudioService	(std::string &, audiodata *, int);
	int32_t	get_CIFcount		() const;
	bool	has_CIFcount		() const;
	int32_t	SIdFor			(const std::string &);
	void	reset			();
private:
	callbacks	*the_callBacks;
	fib_processor	fibProcessor;
	dabParams	params;
	void		*userData;
	void		process_ficInput	(int16_t);
	uint8_t		bitBuffer_out	[768];
        int16_t		ofdm_input	[2304];
        bool		punctureTable	[4 * 768 + 24];

	int16_t		index;
	int16_t		BitsperBlock;
	int16_t		ficno;
	int16_t		ficBlocks;
	int16_t		ficMissed;
	int16_t		ficRatio;
	mutex		fibProtector;
	uint8_t		PRBS [768];
	uint8_t		shiftRegister [9];
};

#endif


