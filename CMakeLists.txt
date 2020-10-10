cmake_minimum_required( VERSION 2.8.11 )
set (objectName dab_channelScanner)
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -flto")
#set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11  -g")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g")
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "/usr/local/bin" CACHE PATH "default install path" FORCE )
endif()
#set (CMAKE_INSTALL_PREFIX /usr/local/bin)

if(MINGW)
    add_definitions ( -municode)
endif()

########################################################################
# select the release build type by default to get optimization flags
########################################################################
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

### make sure our local CMake Modules path comes first
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

######################################################################
#
########################################################################
OPTION(SDRPLAY  "Input: SDRPLAY_V2"  OFF)
OPTION(PLUTO	"Input: PLUTO"	  OFF)
OPTION(PLUTO	"Input: RTLSDR"	  OFF)
OPTION(X64_DEFINED "optimize for x64/SSE"  OFF)
OPTION(RPI_DEFINED "optimize for ARM/NEON" OFF)


if ( (NOT SDRPLAY_V2) AND (NOT PLUTO) AND (NOT RTLSDR) )
   message("None of the Input Options selected. Using default SDRPLAY_V2")
   set(SDRPLAY_V2 ON)
endif ()

set (objectName "")

if(SDRPLAY_V2)
   if (objectName STREQUAL "")
      set(SDRPLAY_V2 ON)
      set(objectName sdrplay-channelScanner)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(RTLSDR)
   if (objectName STREQUAL "")
      set(RTLSDR ON)
      set(objectName rtlsdr-channelScanner)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(PLUTO)
   if (objectName STREQUAL "")
      set(PLUTOSDR ON)
      set(objectName pluto-channelScanner)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

#########################################################################
	find_package (PkgConfig)

        find_package(FFTW3f)
        if (NOT FFTW3F_FOUND)
            message(FATAL_ERROR "please install FFTW3")
        endif ()

        find_package(Faad)
        if (NOT FAAD_FOUND )
            message(FATAL_ERROR "please install libfaad")
        endif ()


	find_package(LibSndFile)
	if (NOT LIBSNDFILE_FOUND)
	   message(FATAL_ERROR "please install libsndfile")
	endif ()
	list(APPEND extraLibs ${LIBSNDFILE_LIBRARY})

        find_package(LibSampleRate)
        if (NOT LIBSAMPLERATE_FOUND)
            message(FATAL_ERROR "please install libsamplerate")
        endif ()
        list(APPEND extraLibs ${LIBSAMPLERATE_LIBRARY})

#########################################################################
        find_package (PkgConfig)

##########################################################################
#	The devices
#

	if (SDRPLAY_V2)
	   find_path (SDRPLAYLIB_INCLUDE_DIR
                      NAMES mirsdrapi-rsp.h
                      PATHS
                      /usr/local/include/
	   )
           include_directories (${SDRPLAYLIB_INCLUDE_DIR})

           find_library (SDRPLAYLIB mirsdrapi-rsp)
           if(NOT(SDRPLAYLIB))
              message(FATAL_ERROR "please install -lmirsdrapi-rsp")
           else(NOT(SDRPLAYLIB))
              list (APPEND extraLibs ${SDRPLAYLIB})
           endif(NOT(SDRPLAYLIB))

	   include_directories (
	     ./devices/sdrplay-handler
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/sdrplay-handler/sdrplay-handler.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/sdrplay-handler/sdrplay-handler.cpp
	   )

	   add_definitions (-DHAVE_SDRPLAY_V2)
	endif (SDRPLAY)


	if (RTLSDR)
           find_package(LibRTLSDR)
           if (NOT LIBRTLSDR_FOUND)
               message(FATAL_ERROR "please install librtlsdr")
           endif ()
	   ###include_directories (${RTLSDR_INCLUDE_DIR})

	   include_directories (
	        ./devices/rtlsdr-handler/
	   )

	   set (${objectName}_HDRS
	        ${${objectName}_HDRS}
	        ./devices/rtlsdr-handler/rtlsdr-handler.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/rtlsdr-handler/rtlsdr-handler.cpp
	   )

	   add_definitions (-DHAVE_RTLSDR)
	endif()

	if (PLUTOSDR)
           find_library (PLUTOLIB iio)
           if(NOT(PLUTOLIB))
              message(FATAL_ERROR "please install libiio")
           else(NOT(PLUTOLIB))
              list (APPEND extraLibs ${PLUTOLIB})
           endif(NOT(PLUTOLIB))
	   find_library (AD9361LIB ad9361)
	   if (NOT(AD9361LIB))
	      message (FATAL_ERROR "please installed libad9361")
	   else(NOT(LIBAD9361))
	      list (APPEND extraLibs ${AD9361LIB})
	   endif(NOT(AD9361LIB))
	   include_directories (
	     ./devices/pluto-handler
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/pluto-handler/pluto-handler.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/pluto-handler/pluto-handler.cpp
	   )

	 add_definitions (-DHAVE_PLUTO)
	endif (PLUTOSDR)

	if (AIRSPY)
           find_package(LibAIRSPY)
           if (NOT LIBAIRSPY_FOUND)
               message(FATAL_ERROR "please install airspy library")
           endif ()
	   ### include_directories (${AIRSPYLIB_INCLUDE_DIR})
           list(APPEND extraLibs ${LIBAIRSPY_LIBRARIES})

	   include_directories (
	     ./devices/airspy-handler
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/airspy-handler/airspy-handler.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/airspy-handler/airspy-handler.cpp
	   )

	   add_definitions (-DHAVE_AIRSPY)
	endif (AIRSPY)

        find_package(Portaudio)
        if (NOT PORTAUDIO_FOUND)
            message(FATAL_ERROR "please install portaudio V19")
        endif ()
        list(APPEND extraLibs ${PORTAUDIO_LIBRARIES})

        find_package(zlib)
	if (NOT ZLIB_FOUND)
            message(FATAL_ERROR "please install libz")
        endif ()
	list(APPEND extraLibs ${ZLIB_LIBRARY})

	find_library (PTHREADS pthread)
	if (NOT(PTHREADS))
	   message (FATAL_ERROR "please install libpthread")
	else (NOT(PTHREADS))
	   set (extraLibs ${extraLibs} ${PTHREADS})
	endif (NOT(PTHREADS))

#######################################################################
#
#	Here we really start

	include_directories (
	           ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	           .
	           ./
	           ./devices
	           ./audio-sink
	           ./dab-streamer
	           ./ofdm
	           ./backend
	           ./backend/audio
	           ./backend/data
	           ./backend/data/mot
	           ./support
	           ./support/viterbi-spiral
	           /usr/include/
	)

	set (${objectName}_HDRS
	     ${${objectName}_HDRS}
	     ./dab-constants.h
	     ./dab-api.h
	     ./service-printer.h
	     ./dab_tables.h
	     ./audio-sink/audio-base.h
	     ./audio-sink/audiosink.h
	     ./audio-sink/newconverter.h
	     ./devices/device-handler.h
	     ./dab-processor.h
	     ./ofdm/phasereference.h
	     ./ofdm/phasetable.h
	     ./ofdm/freq-interleaver.h
	     ./ofdm/timesyncer.h
	     ./ofdm/fic-handler.h
	     ./ofdm/fib-processor.cpp
	     ./ofdm/sample-reader.h
	     ./ofdm/tii_detector.h
	     ./backend/firecode-checker.h
	     ./backend/backend-base.h
	     ./backend/charsets.h
	     ./backend/galois.h
	     ./backend/reed-solomon.h
	     ./backend/msc-handler.h
	     ./backend/virtual-backend.h
	     ./backend/audio-backend.h
	     ./backend/audio/faad-decoder.h
	     ./backend/audio/mp4processor.h 
	     ./backend/audio/mp2processor.h 
	     ./backend/data/virtual-datahandler.h 
	     ./backend/data/pad-handler.h 
	     ./backend/data/mot/mot-handler.h 
	     ./backend/data/mot/mot-dir.h 
	     ./backend/data/mot/mot-object.h 
	     ./support/ringbuffer.h
	     ./support/band-handler.h
#	     ./support/viterbi-handler.h
	     ./support/protTables.h
	     ./support/protection.h
	     ./support/uep-protection.h
	     ./support/eep-protection.h
	     ./support/fft_handler.h
	     ./support/dab-params.h
	     ./support/viterbi-spiral/viterbi-spiral.h
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./main.cpp
	     ./service-printer.cpp
	     ./dab_tables.cpp
	     ./audio-sink/audio-base.cpp
	     ./audio-sink/audiosink.cpp
	     ./audio-sink/newconverter.cpp
	     ./devices/device-handler.cpp
	     ./dab-processor.cpp
	     ./ofdm/ofdm-decoder.cpp
	     ./ofdm/phasereference.cpp
	     ./ofdm/phasetable.cpp
	     ./ofdm/freq-interleaver.cpp
	     ./ofdm/timesyncer.cpp
	     ./ofdm/sample-reader.cpp
	     ./ofdm/fib-processor.cpp
	     ./ofdm/fic-handler.cpp
	     ./ofdm/tii_detector.cpp
	     ./backend/firecode-checker.cpp
	     ./backend/backend-base.cpp
	     ./backend/charsets.cpp
	     ./backend/galois.cpp
	     ./backend/reed-solomon.cpp
	     ./backend/msc-handler.cpp
	     ./backend/virtual-backend.cpp
	     ./backend/audio-backend.cpp
	     ./backend/audio/mp4processor.cpp 
	     ./backend/audio/mp2processor.cpp 
	     ./backend/data/virtual-datahandler.cpp 
	     ./backend/data/pad-handler.cpp 
	     ./backend/data/mot/mot-handler.cpp 
	     ./backend/data/mot/mot-dir.cpp 
	     ./backend/data/mot/mot-object.cpp 
	     ./support/band-handler.cpp
#	     ./support/viterbi-handler.cpp
	     ./support/protTables.cpp
	     ./support/protection.cpp
	     ./support/eep-protection.cpp
	     ./support/uep-protection.cpp
	     ./support/fft_handler.cpp
	     ./support/dab-params.cpp
	     ./support/viterbi-spiral/viterbi-spiral.cpp
	)

	if (X64_DEFINED)
           set (${objectName}_SRCS
                ${${objectName}_SRCS}
              ./support/viterbi-spiral/spiral-sse.c
           )
           set (${objectName}_HDRS
                ${${objectName}_HDRS}
             ./support/viterbi-spiral/spiral-sse.h
           )
           add_definitions (-DSSE_AVAILABLE)
        elseif (RPI_DEFINED)
           set (${objectName}_SRCS
                ${${objectName}_SRCS}
              ./support/viterbi-spiral/spiral-neon.c
           )
           set (${objectName}_HDRS
                ${${objectName}_HDRS}
             ./support/viterbi-spiral/spiral-neon.h
           )
           add_definitions (-DNEON_AVAILABLE)
	# compiler options moved below add_executable .. and target specific
	#set (CMAKE_CXX_FLAGS "${CMAKE_XCC_FLAGS} -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4")
	#set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4")
        else (X64_DEFINED)
           set (${objectName}_SRCS
                ${${objectName}_SRCS}
              ./support/viterbi-spiral/spiral-no-sse.c
           )
           set (${objectName}_HDRS
                ${${objectName}_HDRS}
             ./support/viterbi-spiral/spiral-no-sse.h
           )
        endif (X64_DEFINED)
#
	include_directories (
	          ${FFTW_INCLUDE_DIRS}
	          ${PORTAUDIO_INCLUDE_DIRS}
	          ${FAAD_INCLUDE_DIRS}
	          ${SNDFILES_INCLUDE_DIRS}
	)

#####################################################################

	add_executable (${objectName} 
	                ${${objectName}_SRCS}
	)
       if (RPI_DEFINED)
           # compiler options seem changed with gcc versions. gcc 8.3.0 on Raspbian didn't accept -mcpu ..
           # test options with "make VERBOSE=1"
           target_compile_options(${objectName}  PRIVATE -march=armv7-a -mfloat-abi=hard -mfpu=neon-vfpv4 )
        endif()

	target_link_libraries (${objectName}
	                       ${FFTW3F_LIBRARIES}
	                       ${extraLibs}
	                       ${FAAD_LIBRARIES}
	                       ${CMAKE_DL_LIBS}
	)

	INSTALL (TARGETS ${objectName} DESTINATION .)

########################################################################
# Create uninstall target
########################################################################

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
