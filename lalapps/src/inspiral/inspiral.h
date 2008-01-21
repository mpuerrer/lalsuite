/*
*  Copyright (C) 2007 Duncan Brown, Patrick Brady
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with with program; see the file COPYING. If not, write to the
*  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
*  MA  02111-1307  USA
*/

#ifndef _LALAPPS_INSPIRAL_H
#define _LALAPPS_INSPIRAL_H

#include <lalapps.h>
#include <lal/LALConfig.h>
#include <lal/LALStdio.h>
#include <lal/LALStdlib.h>
#include <lal/LALError.h>
#include <lal/LALDatatypes.h>
#include <lal/LIGOMetadataUtils.h>
#include <lal/LIGOMetadataTables.h>
#include <lal/AVFactories.h>
#include <lal/NRWaveIO.h>
#include <lal/NRWaveInject.h>
#include <lal/LIGOLwXMLRead.h>
#include <lal/Inject.h>
#include <lal/FileIO.h>
#include <lal/Units.h>
#include <lal/FrequencySeries.h>
#include <lal/TimeSeries.h>
#include <lal/TimeFreqFFT.h>
#include <lal/VectorOps.h>
#include <lal/LALDetectors.h>
#include <lal/LALFrameIO.h>
#include <lal/FrameStream.h>


#ifdef  __cplusplus   /* C++ protection. */
extern "C" {
#endif


#define INSPIRALH_ENULL   1
#define INSPIRALH_EFILE   2
#define INSPIRALH_ENONULL 3
#define INSPIRALH_ENOMEM  4
#define INSPIRALH_EVAL 	  5

#define INSPIRALH_MSGENULL    "Null pointer"
#define INSPIRALH_MSGEFILE    "Error in file-IO"
#define INSPIRALH_MSGENONULL  "Not a Null pointer"
#define INSPIRALH_MSGENOMEM   "Memory ellocation error"
#define INSPIRALH_MSGEVAL     "Invalid value"


REAL4 compute_candle_distance(
    REAL4 candleM1, 
    REAL4 candleM2,
    REAL4 snr, 
    REAL8 chanDeltaT, 
    INT4 nPoints, 
    REAL8FrequencySeries *spec, 
    UINT4 cut);

SummValueTable **add_summvalue_table(
    SummValueTable **newTable,
    LIGOTimeGPS gpsStartTime, 
    LIGOTimeGPS gpsEndTime, 
    const CHAR *programName, 
    const CHAR *ifoName, 
    const CHAR *summValueName, 
    const CHAR *comment, 
    REAL8 value
    );

void AddNumRelStrainModes( LALStatus              *status,
			   REAL4TimeVectorSeries  **outStrain,
			   SimInspiralTable *thisinj);

void InjectNumRelWaveforms (LALStatus              *status,
			    REAL4TimeSeries        *chan, 
			    SimInspiralTable       *injections,
			    CHAR                    ifo[3],    
			    REAL8                   dynRange); 


#ifdef  __cplusplus
}                /* Close C++ protection */
#endif

#endif           /* Close double-include protection */
