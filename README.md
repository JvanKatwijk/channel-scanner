
-------------------------------------------------------------------------
dab-channelScanner
-------------------------------------------------------------------------

channelScanner is derived from the dab-cmdline scanner program.
The channel-scanner is used to scan some channels, channels specified in
the command line. The output is (can be) twofold:

   a. a description of the content of the DAB data found in the scannel;

   b. a dump of a specified number of seconds of the raw input data.

![channel-scanner](/channel-scanner.png?raw=true)

For a continuous scan over the (selected) channels in a given band,
one is advised to use the dab-scanner.

channel-scanner supports
	a. the RTLSDR devices
	b. PLUTO devices
	c. SDRplay devices (2.13 libraries)

Note that it contains a number of sources for handling the back end
that are not used at all, but were part of the sourcetree used
for setting up the program


---------------------------------------------------------------------------
Building an executable
--------------------------------------------------------------------------

Assuming the required libraries are installed, building an executable
is using cmake

mkdir build
cd build
cmake .. -DXXX=ON
make

where XXX is ONE of RTLSDR, SDRPLAY, PLUTO

So, one generates an executable for a single device

The basic parameters are

	a. -C XX for the channel selection
	b. -G for selecting the gain (or gain reduction)
	c. -Q for setting the autogain
	d. -d xx and -D XX for setting the delay (see later on)
	e. -F filename for selecting an output file, default stdout
	f. -R for dumping the per channel input into a file
	g. -T duration (in seconds), default 10

Important note:
The scanner now supports scanning multiple channels. Add a channel
to the list using the -C XX command.
As an example

	pluto-xxx -C 12C -C 11C -C 5B -C 5C -C 8A -Q -F /tmp/datafile.txt -R

will scan the channels 12C, 11C, 5B, 5C and 8A, with the autogain set and

   a. the combined scandata will be written to the file /tmp/datafile.txt
The file is a text file, but can be read in with e.g. LibreOfficeCalc
or similar programs
   b. in case the channel contains detectable DAB data, raw data of the
channel will be written to a file for "duration" SECONDS. The filename
is a combination of the channel, the EID and the date, e.g.
"5B 8181 2020-09-15 10:43:04.sdr"

The -d xx flag sets the maximum waiting time in seconds for deciding whether or not time syncing can be achieved;
The -D xx flag sets the maximum waiting time in seconds  for the identification of an ensemble;

