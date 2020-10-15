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
 *    the Free Software Foundation version 2 of the License.
 *
 *    channelScanner is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with channelScanner if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"pluto-handler.h"
#include	"xml-filewriter.h"
#include	<unistd.h>
#include	"ad9361.h"

/* static scratch mem for strings */
static char tmpstr[64];

/* helper function generating channel names */
static
char*	get_ch_name (const char* type, int id) {
        snprintf (tmpstr, sizeof(tmpstr), "%s%d", type, id);
        return tmpstr;
}

enum iodev { RX, TX };

/* returns ad9361 phy device */
static
struct iio_device* get_ad9361_phy (struct iio_context *ctx) {
struct iio_device *dev = iio_context_find_device (ctx, "ad9361-phy");
	return dev;
}

/* finds AD9361 streaming IIO devices */
static
bool get_ad9361_stream_dev (struct iio_context *ctx,
	                    enum iodev d, struct iio_device **dev) {
	switch (d) {
	case TX:
	   *dev = iio_context_find_device (ctx, "cf-ad9361-dds-core-lpc");
	   return *dev != NULL;

	case RX:
	   *dev = iio_context_find_device (ctx, "cf-ad9361-lpc");
	   return *dev != NULL;

	default: 
	   return false;
	}
}

/* finds AD9361 streaming IIO channels */
static
bool get_ad9361_stream_ch (__notused struct iio_context *ctx,
	                   enum iodev d, struct iio_device *dev,
	                   int chid, struct iio_channel **chn) {
	*chn = iio_device_find_channel (dev,
	                                get_ch_name ("voltage", chid),
	                                d == TX);
	if (!*chn)
	   *chn = iio_device_find_channel (dev,
	                                   get_ch_name ("altvoltage", chid),
	                                   d == TX);
	return *chn != NULL;
}

/* finds AD9361 phy IIO configuration channel with id chid */
static
bool	get_phy_chan (struct iio_context *ctx,
	              enum iodev d, int chid, struct iio_channel **chn) {
	switch (d) {
	   case RX:
	      *chn = iio_device_find_channel (get_ad9361_phy (ctx),
	                                      get_ch_name ("voltage", chid),
	                                      false);
	      return *chn != NULL;

	   case TX:
	      *chn = iio_device_find_channel (get_ad9361_phy (ctx),
	                                      get_ch_name ("voltage", chid),
	                                      true);
	      return *chn != NULL;

	   default: 
	      return false;
	}
}

/* finds AD9361 local oscillator IIO configuration channels */
static
bool	get_lo_chan (struct iio_context *ctx,
	             enum iodev d, struct iio_channel **chn) {
// LO chan is always output, i.e. true
	switch (d) {
	   case RX:
	      *chn = iio_device_find_channel (get_ad9361_phy (ctx),
	                                      get_ch_name ("altvoltage", 0),
	                                      true);
	      return *chn != NULL;

	   case TX:
	      *chn = iio_device_find_channel (get_ad9361_phy (ctx),
	                                      get_ch_name ("altvoltage", 1),
	                                      true);
	      return *chn != NULL;

	   default: 
	      return false;
	}
}

/* applies streaming configuration through IIO */
bool cfg_ad9361_streaming_ch (struct iio_context *ctx,
	                      struct stream_cfg *cfg,
	                      enum iodev type, int chid) {
struct iio_channel *chn = NULL;
int	ret;

// Configure phy and lo channels
	printf("* Acquiring AD9361 phy channel %d\n", chid);
	if (!get_phy_chan (ctx, type, chid, &chn)) {
	   return false;
	}
	ret = iio_channel_attr_write (chn,
	                              "rf_port_select", cfg -> rfport);
	if (ret < 0)
	   return false;
	ret = iio_channel_attr_write_longlong (chn,
	                                       "rf_bandwidth", cfg -> bw_hz);
	ret = iio_channel_attr_write_longlong (chn,
	                                       "sampling_frequency",
	                                       cfg -> fs_hz);

// Configure LO channel
	printf("* Acquiring AD9361 %s lo channel\n", type == TX ? "TX" : "RX");
	if (!get_lo_chan (ctx, type, &chn)) {
	   return false;
	}
	ret = iio_channel_attr_write_longlong (chn,
	                                       "frequency", cfg -> lo_hz);
	return true;
}

static inline
std::complex<float> cmul (std::complex<float> x, float y) {
	return std::complex<float> (real (x) * y, imag (x) * y);
}

	plutoHandler::plutoHandler  (RingBuffer<std::complex<float>>*b,
	                             const std::string	 &recorderVersion,
	                             int32_t	frequency,
	                             int	gainValue,
	                             bool	agcMode):
	                               deviceHandler (b) {
	this	-> _I_Buffer		= b;
	this	-> recorderVersion	= recorderVersion;
	this	-> ctx			= nullptr;
	this	-> rxbuf		= nullptr;
	this	-> rx0_i		= nullptr;
	this	-> rx0_q		= nullptr;

	rx_cfg. bw_hz			= 1536000;
	rx_cfg. fs_hz			= RX_RATE;
	rx_cfg. lo_hz			= frequency;
	rx_cfg. rfport			= "A_BALANCED";

//
//	step 1: establish a context
//
	ctx	= iio_create_default_context ();
	if (ctx == nullptr) {
	   ctx = iio_create_local_context ();
	}

	if (ctx == nullptr) {
	   ctx = iio_create_network_context ("pluto.local");
	}

	if (ctx == nullptr) {
	   ctx = iio_create_network_context ("192.168.2.1");
	}

	if (ctx == nullptr) {
	   fprintf (stderr, "No pluto found, fatal\n");
	   throw (24);
	}
//

	if (iio_context_get_devices_count (ctx) <= 0) {
	   fprintf (stderr, "no devices, fatal");
	   throw (25);
	}

	fprintf (stderr, "* Acquiring AD9361 streaming devices\n");
	if (!get_ad9361_stream_dev (ctx, RX, &rx)) {
	   fprintf (stderr, "No RX device found\n");
	   throw (27);
	}

	fprintf (stderr, "* Configuring AD9361 for streaming\n");
	if (!cfg_ad9361_streaming_ch (ctx, &rx_cfg, RX, 0)) {
	   fprintf (stderr, "RX port 0 not found\n");
	   throw (28);
	}

	struct iio_channel *chn;
	if (get_phy_chan (ctx, RX, 0, &chn)) {
	   int ret;
	   if (agcMode)
	      ret = iio_channel_attr_write (chn,
	                                    "gain_control_mode",
	                                    "slow_attack");
	   else {
	      ret = iio_channel_attr_write (chn,
	                                    "gain_control_mode",
	                                    "manual");
	      ret = iio_channel_attr_write_longlong (chn,
	                                             "hardwaregain",
	                                             gainValue);
	   }

	   if (ret < 0)
	      fprintf (stderr, "setting agc/gain did not work\n");
	}

	fprintf (stderr, "* Initializing AD9361 IIO streaming channels\n");
	if (!get_ad9361_stream_ch (ctx, RX, rx, 0, &rx0_i)) {
	   fprintf (stderr, "RX chan i not found");
	   throw (30);
	}
	
	if (!get_ad9361_stream_ch (ctx, RX, rx, 1, &rx0_q)) {
	   fprintf (stderr,"RX chan q not found");
	   throw (31);
	}
	
        iio_channel_enable (rx0_i);
        iio_channel_enable (rx0_q);

        rxbuf = iio_device_create_buffer (rx, 256*1024, false);
	if (rxbuf == nullptr) {
	   fprintf (stderr, "could not create RX buffer, fatal");
	   iio_context_destroy (ctx);
	   throw (35);
	}

	iio_buffer_set_blocking_mode (rxbuf, true);
//
//	and set up interpolation table
	float	divider		= (float)DIVIDER;
	float	denominator	= DAB_RATE / divider;
	for (int i = 0; i < DAB_RATE / DIVIDER; i ++) {
           float inVal  = float (RX_RATE / divider);
           mapTable_int [i]     =  int (floor (i * (inVal / denominator)));
           mapTable_float [i]   = i * (inVal / denominator) - mapTable_int [i];
        }
        convIndex       = 0;

	(void)  ad9361_set_bb_rate_custom_filter_manual (get_ad9361_phy (ctx),
	                                                 RX_RATE,
	                                                 1540000 / 2,
	                                                 1.1 * 1540000 / 2,
	                                                 1920000,
	                                                 1536000);
	dumping. store (false);
	xmlFile		= nullptr;
	running. store (false);
}

	plutoHandler::~plutoHandler () {
	stopReader ();
	iio_buffer_destroy (rxbuf);
	iio_context_destroy (ctx);
}

bool	plutoHandler::restartReader	(int32_t freq) {
struct	iio_channel *lo_channel;

	if (running. load())
	   return true;		// should not happen

	get_lo_chan (ctx, RX, &lo_channel);
	rx_cfg. lo_hz	= freq;
	int ret	= iio_channel_attr_write_longlong
	                             (lo_channel,
	                                   "frequency", rx_cfg. lo_hz);
	if (ret < 0) {
	   fprintf (stderr, "error in selected frequency\n");
	   return false;
	}

	threadHandle	= std::thread (&plutoHandler::run, this);
	return true;
}

void	plutoHandler::stopReader() {
	if (!running. load())
	   return;
	running. store (false);
	stopDumping	();
	usleep (50000);
	threadHandle. join ();
}

void	plutoHandler::run	() {
char	*p_end, *p_dat;
int	p_inc;
int	nbytes_rx;
std::complex<float> localBuf [DAB_RATE / DIVIDER];
std::complex<int16_t> dumpBuffer [CONV_SIZE + 1];
	running. store (true);
	while (running. load ()) {
	   nbytes_rx	= iio_buffer_refill	(rxbuf);
	   p_inc	= iio_buffer_step	(rxbuf);
	   p_end	= (char *)(iio_buffer_end  (rxbuf));

	   for (p_dat = (char *)iio_buffer_first (rxbuf, rx0_i);
	        p_dat < p_end; p_dat += p_inc) {
	      const int16_t i_p = ((int16_t *)p_dat) [0];
	      const int16_t q_p = ((int16_t *)p_dat) [1];
	      std::complex<float>sample = std::complex<float> (i_p / 2048.0,
	                                                       q_p / 2048.0);
	      dumpBuffer [convIndex] = std::complex<int16_t> (i_p, q_p);
	      convBuffer [convIndex ++] = sample;
	      if (convIndex > CONV_SIZE) {
	         if (dumping. load ())
	            xmlWriter -> add (&dumpBuffer [1], CONV_SIZE);
	         for (int j = 0; j < DAB_RATE / DIVIDER; j ++) {
	            int16_t inpBase	= mapTable_int [j];
	            float   inpRatio	= mapTable_float [j];
	            localBuf [j]	= cmul (convBuffer [inpBase + 1],
	                                                          inpRatio) +
                                     cmul (convBuffer [inpBase], 1 - inpRatio);
                 }

	         _I_Buffer ->  putDataIntoBuffer (localBuf,
	                                          DAB_RATE / DIVIDER);
	         convBuffer [0] = convBuffer [CONV_SIZE];
	         convIndex = 1;
	      }
	   }
	}
}
int16_t	plutoHandler::bitDepth		() {
	return 12;
}

std::string	plutoHandler::deviceName	() {
	return "pluto";
}

void	plutoHandler::startDumping	(const std::string &fileName) {
        xmlFile	= fopen (fileName. c_str (), "w");
	if (xmlFile == nullptr)
	   return;
	
	xmlWriter	= new xml_fileWriter (xmlFile,
	                                      12,
	                                      "int16",
	                                      RX_RATE,
	                                      rx_cfg. lo_hz,
	                                      "pluto",
	                                      "adalm",
	                                      recorderVersion);
	dumping. store (true);
}

void	plutoHandler::stopDumping	() {
	if (!dumping. load ())
	   return;
	if (xmlFile == nullptr)	// this can happen !!
	   return;
	dumping. store (false);
	usleep (1000);
	xmlWriter	-> print_xmlHeader ();
	delete xmlWriter;
	fclose (xmlFile);
	xmlFile		= nullptr;
}

