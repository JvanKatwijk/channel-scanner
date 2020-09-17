#
/*
 *    Copyright (C) 2013, 2014, 2015, 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the dab library 
 *
 *    dab library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dab library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	<stdio.h>
#include	<vector>
#include	"dab_tables.h"
#include	"dab-api.h"
class	dabProcessor;

void	print_fileHeader (FILE *f, bool jsonOutput);
void	print_ensembleData (FILE *f, bool jsonOutput,
	                    dabProcessor *theRadio,
	                    std::string	channel,
	                    std::string ensembleLabel,
	                    uint32_t	ensembleId,
	                    float	frequency,
	                    int		snr,
	                    std::vector<int>	tiiData,
	                    bool	*firstEnsemble);
void	print_audioheader (FILE *f, bool jsonOutput);
void	print_audioService (FILE *f, bool jsonOutput,
	                    dabProcessor *theRadio,
	                    std::string serviceName,
	                    audiodata *d,
		            bool *firstService);
void	print_dataHeader (FILE *f, bool jsonOutput);
void	print_dataService (FILE		*f, bool jsonOutput,
	                   dabProcessor	*theRadio,
	                   std::string	serviceName,
	                   uint8_t	compnr,
	                   packetdata *d,
		           bool *firstService);
void	print_ensembleFooter (FILE *f, bool jsonOutput);
void	print_fileFooter (FILE *f, bool jsonOutput);
