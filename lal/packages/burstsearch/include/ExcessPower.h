/********************************** <lalVerbatim file="ExcessPowerHV">
Author: Flanagan, E
$Id$
**************************************************** </lalVerbatim> */

#ifndef _EXCESSPOWER_H
#define _EXCESSPOWER_H

#include <lal/LALDatatypes.h>
#include <lal/TFTransform.h>
#include <lal/LALRCSID.h>

#ifdef  __cplusplus   /* C++ protection. */
extern "C" {
#endif

NRCSID(EXCESSPOWERH, "$Id$");

/******** <lalErrTable file="ExcessPowerHErrTab"> ********/
#define EXCESSPOWERH_EORDER       32


#define EXCESSPOWERH_MSGEORDER    "Routines called in illegal order"
/******** </lalErrTable> ********/



typedef struct
tagTFTile
{
  INT4                             fstart;
  INT4                             fend;
  INT4                             tstart;
  INT4                             tend;
  INT4                             whichPlane;
  REAL8                            deltaT; /* deltaF will always be 1/deltaT */
  REAL8                            excessPower;
  REAL8                            alpha;
  REAL8                            weight;
  BOOLEAN                          firstCutFlag;
  struct tagTFTile                 *nextTile;
}
TFTile;


typedef struct
tagTFTiling
{
  TFTile                           *firstTile;  /* linked list of Tiles */
  INT4                             numTiles;
  COMPLEX8TimeFrequencyPlane       **tfp;       /* Vector of pointers */
  ComplexDFTParams                 **dftParams; /* Vector of pointers */
  INT4                             numPlanes;
  BOOLEAN                          planesComputed;
  BOOLEAN                          excessPowerComputed;
  BOOLEAN                          tilesSorted;
}
TFTiling;


typedef struct
tagCreateTFTilingIn
{
  INT4                             overlapFactor;
  INT4                             minFreqBins;
  INT4                             minTimeBins;
  REAL8                            flow;   /* lowest freq to search  */
  REAL8                            deltaF;
  INT4                             length;
  REAL8                            maxTileBand;
}
CreateTFTilingIn;


typedef struct
tagComputeExcessPowerIn
{
  REAL8                            numSigmaMin;
  REAL8                            alphaDefault;
}
ComputeExcessPowerIn;


void
LALAddWhiteNoise (
               LALStatus                               *status,
               COMPLEX8Vector                       *v,
               REAL8                                noiseLevel
               );


void
LALCreateTFTiling (
                LALStatus                              *status,
                TFTiling                            **tfTiling,
                CreateTFTilingIn                    *input
                );


void
LALDestroyTFTiling (
                 LALStatus                             *status,
                 TFTiling                           **tfTiling
                 );


void
LALComputeTFPlanes (
                 LALStatus                             *status,
                 TFTiling                           *tfTiling,
                 COMPLEX8FrequencySeries            *freqSeries
                 );


void
LALComputeExcessPower (
                    LALStatus                          *status,
                    TFTiling                        *tfTiling,
                    ComputeExcessPowerIn            *input
                    );


void
LALSortTFTiling (
              LALStatus                                *status,
              TFTiling                              *tfTiling
              );

void
LALComputeLikelihood (
                   LALStatus                           *status,
                   REAL8                            *lambda,
                   TFTiling                         *tfTiling
                   );


void
LALPrintTFTileList (
                 LALStatus                             *status,
                 FILE                               *fp,
                 TFTiling                           *tfTiling,
                 INT4                               maxTiles
                 );

void 
PrintTFTileList1 (
                 LALStatus                             *status,
                 FILE                               *fp,
                 TFTiling                           *tfTiling,
                 INT4                               maxTiles
                 );

#ifdef  __cplusplus
}
#endif  /* C++ protection. */

#endif
