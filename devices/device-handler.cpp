#
/*
 *    Copyright (C) 2010, 2011, 2012
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
 *
 * 	Default (void) implementation of
 * 	virtual input class
 */
#include	"device-handler.h"

	deviceHandler::deviceHandler (RingBuffer<std::complex<float>> *b) {
	_I_Buffer = b;
}

	deviceHandler::~deviceHandler (void) {
}

bool	deviceHandler::restartReader	(int32_t) {
	return true;
}

void	deviceHandler::stopReader	(void) {
}

void	deviceHandler::startDumping	(std::string s, uint32_t id) {
	(void)s; (void)id;
}

void	deviceHandler::stopDumping	() {
}

void	deviceHandler::run		(void) {
}


std::string  deviceHandler::toHex (uint32_t ensembleId) {
char t [4];
std::string res;
uint8_t c [2];
int     i;
        for (i = 0; i < 4; i ++) {
           t [3 - i] = ensembleId & 0xF;
           ensembleId >>= 4;
        }
        for (i = 0; i < 4; i ++) {
           c [0] = t [i] <= 9 ? (char) ('0' + t [i]) : (char)('A'+ t [i] - 10);
	   c [1] = 0;
           res. append ((const char *) (&c));
        }
        return res;
}
