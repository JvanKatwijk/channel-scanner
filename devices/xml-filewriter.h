#
/*
 *    Copyright (C) 2020
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

#ifndef	__XML_FILEWRITER__
#define	__XML_FILEWRITER__


#include	<string>
#include	<stdint.h>
#include	<stdio.h>
#include	<complex>

class Blocks	{
public:
			Blocks		() {}
			~Blocks		() {}
	int		blockNumber;
	int		nrElements;
	std::string		typeofUnit;
	int		frequency;
	std::string		modType;
};

class xml_fileWriter {
public:
		xml_fileWriter	(FILE *,
	                         int,
	                         std::string,
	                         int,
	                         int,
	                         std::string,
	                         std::string,
	                         std::string);
	                         
			~xml_fileWriter		();
	void		add			(std::complex<int16_t> *, int);
	void		add			(std::complex<uint8_t> *, int);
	void		add			(std::complex<int8_t> *, int);
	void		print_xmlHeader		();
private:
	int		nrBits;
	std::string	container;
	int		sampleRate;
	int		frequency;
	std::string	deviceName;
	std::string	deviceModel;
	std::string	recorderVersion;
	FILE		*xmlFile;
	std::string	byteOrder;
	int		nrElements;
};

#endif
