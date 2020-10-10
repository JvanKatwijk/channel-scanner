
-------------------------------------------------------------------------
dab-channelScanner
-------------------------------------------------------------------------

channelScanner is derived from the dab-cmdline scanner program.
The channel-scanner is used to scan some channels, channels specified in
the command line.

The output is (can be) twofold:

   a. a description of the content of the DAB data found in the scannel;

   b. a dump of a specified number of seconds of the raw input data.

![channel-scanner](/channel-scanner.png?raw=true)

**For a continuous scan over the (selected) channels in a given band,
one is advised to use the dab-scanner.**

---------------------------------------------------------------------
How to use the channel scanner
----------------------------------------------------------------------

channelscanner is a command line device, compiled for a single
input device (one may choose among RTLSDR sticks, Pluto and SDRplay devices).
Settings are done through parameters, e.g.

	sdrplay-channelScanner -C 12C -C 11C -C 5B -C 5C -C 8A -Q -F /tmp/datafile.txt -R

will scan the channels 12C, 11C, 5B, 5C and 8A, with the autogain set and

   a. the combined scandata will be written to the file /tmp/datafile.txt. The file is a text file, but can be read in with e.g. LibreOfficeCalc or similar programs

   b. in case the channel contains detectable DAB data, raw data of the channel will be written to a file for "duration" SECONDS. The filename is a combination of the channel, the EID and the date, e.g.  "5B 8181 2020-09-15 10:43:04.sdr"

The basic parameters are

	a. -C XX for the channel selection
	b. -G XX for selecting the gain (or gain reduction)
	c. -Q for setting the autogain
	d. -d xx and -D XX for setting the delay (see later on)
	e. -F filename for selecting an output file, default stdout
	f. -R, when used, the per channel input is dumped into a file
	g. -T xx, duration (in seconds), default 10

The -d xx flag sets the maximum waiting time in seconds for deciding whether or not time syncing can be achieved;
The -D xx flag sets the maximum waiting time in seconds  for the identification of an ensemble;

Use the -C XX flag for each channel that needs to be investigated,
i.e. -C 12C -C 11C tells the software that both channels "12C and "11C"
are to be inspected.

The -R flag, when used, instructs the software to dump the "per channel"
data - provided some DAB data is found in that channel - into a file.
The filename will be generated, and consists of the following elements

	channelName EnsembleIdentification Data. sdr

Example

	'12C 8001 2020-09-18 10:16:49.sdr'

indicates that a file with raw data (in ".sdr" format) is written
on the date as specified, where the input was from channel "12C",
with the Ensemble Identification 0x8001.

--------------------------------------------------------------------------
Supported devices
--------------------------------------------------------------------------

channel-scanner supports

	a. the RTLSDR devices
	b. PLUTO devices
	c. SDRplay devices (2.13 library only)

---------------------------------------------------------------------------
Building an executable
--------------------------------------------------------------------------

Assuming the required libraries are installed, building an executable
is using the cmake/make combination

	mkdir build
	cd build
	cmake .. -DXXX=ON
	make

where XXX is ONE of RTLSDR, SDRPLAY_V2, PLUTO

So, one generates an executable for a SINGLE device.

