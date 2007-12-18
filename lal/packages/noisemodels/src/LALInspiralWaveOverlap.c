/*
*  Copyright (C) 2007 David Churches, Duncan Brown, Jolien Creighton, B.S. Sathyaprakash, Anand Sengupta, Craig Robinson , Thomas Cokelaer
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

/*  <lalVerbatim file="LALInspiralWaveOverlapCV">
Author: Sathyaprakash, B. S.
$Id$
</lalVerbatim>  */
/* <lalLaTeX>
\subsection{Module \texttt{LALInspiralWaveOverlap.c}}
Module to compute the overlap of a given data set with
two orthogonal inspiral signals of specified parameters 
with a weight specified in a psd array. The code also returns
in a parameter structure the maximum of the overlap, the bin
where the maximum occured and the phase at the maximum. 

\subsubsection*{Prototypes}
\vspace{0.1in}
\input{LALInspiralWaveOverlapCP}
\idx{LALInspiralWaveOverlap()}

\subsubsection*{Description}
\subsubsection*{Algorithm}
\subsubsection*{Uses}
\begin{verbatim}
LALInspiralWave
LALREAL4VectorFFT
LALInspiralWaveNormaliseLSO
LALInspiralWaveCorrelate
\end{verbatim}

\subsubsection*{Notes}

\vfill{\footnotesize\input{LALInspiralWaveOverlapCV}}
</lalLaTeX>  */
#include <lal/LALNoiseModels.h>

void LALInspiralGetOrthoNormalFilter(REAL4Vector *filter2, REAL4Vector *filter1);
NRCSID (LALINSPIRALWAVEOVERLAPC, "$Id$");

/*  <lalVerbatim file="LALInspiralWaveOverlapCP"> */
void
LALInspiralWaveOverlap 
   (
   LALStatus               *status,
   REAL4Vector             *output,
   InspiralWaveOverlapOut  *overlapout,
   InspiralWaveOverlapIn   *overlapin
   )
{  /*  </lalVerbatim>  */
   REAL4Vector filter1, filter2, output1, output2;
   InspiralWaveCorrelateIn corrin;
   REAL8 norm, x, y, z, phase;
   INT4 i, nBegin, nEnd;
   InspiralWaveNormaliseIn normin;

   INITSTATUS (status, "LALInspiralWaveOverlap", LALINSPIRALWAVEOVERLAPC);
   ATTATCHSTATUSPTR(status);

   ASSERT (output->data,  status, LALNOISEMODELSH_ENULL, LALNOISEMODELSH_MSGENULL);
   ASSERT (overlapin->psd.data,  status, LALNOISEMODELSH_ENULL, LALNOISEMODELSH_MSGENULL);
   ASSERT (overlapin->signal.data,  status, LALNOISEMODELSH_ENULL, LALNOISEMODELSH_MSGENULL);
   ASSERT (overlapin->ifExtOutput <= 1, status, LALNOISEMODELSH_ESIZE, LALNOISEMODELSH_MSGESIZE);

   output1.length = output2.length = overlapin->signal.length;
   filter1.length = filter2.length = overlapin->signal.length;

   /* Allocate memory to all temporary arrays */
   if (!(output1.data = (REAL4*) LALMalloc(sizeof(REAL4)*output1.length))) {
      ABORT (status, LALNOISEMODELSH_EMEM, LALNOISEMODELSH_MSGEMEM);
   }
   if (!(output2.data = (REAL4*) LALMalloc(sizeof(REAL4)*output2.length))) {
      LALFree(output1.data);
      output1.data = NULL;
      ABORT (status, LALNOISEMODELSH_EMEM, LALNOISEMODELSH_MSGEMEM);
   }
   if (!(filter1.data = (REAL4*) LALMalloc(sizeof(REAL4)*filter1.length))) {
      LALFree(output1.data);
      LALFree(output2.data);
      output1.data = NULL;
      output2.data = NULL;
      ABORT (status, LALNOISEMODELSH_EMEM, LALNOISEMODELSH_MSGEMEM);
   }
   if (!(filter2.data = (REAL4*) LALMalloc(sizeof(REAL4)*filter2.length))) {
      LALFree(output1.data);
      LALFree(output2.data);
      LALFree(filter1.data);
      output1.data = NULL;
      output2.data = NULL;
      filter1.data = NULL;
      ABORT (status, LALNOISEMODELSH_EMEM, LALNOISEMODELSH_MSGEMEM);
   }

   /* 
    * Generate the required template 
    */
   overlapin->param.nStartPad = 0;
   switch (overlapin->param.approximant)
   {
	case AmpCorPPN:
	case TaylorT1:
	case TaylorT2:
	case TaylorT3:
	case PadeT1:
	case EOB:
	case SpinTaylorT3: 
        case SpinTaylor:
        case Eccentricity:
		/*
		 * Generate only 0-phase template, pi/2-template can be generated by
		 * multiplying the 0-phase template with i=e^(i pi/2).
		 */
                overlapin->param.startPhase =  0;
                LALInspiralWave(status->statusPtr, &output1, &overlapin->param);
		CHECKSTATUSPTR(status);
		/* 
		 * When the approximant is BCV, TaylorT1, TaylorT2, TaylorT3, PadeT1 or EOB
		 * generate the template and take its FT
		 */
                LALREAL4VectorFFT(status->statusPtr, &filter1, &output1, overlapin->fwdp);
		CHECKSTATUSPTR(status);
		break;
          
	case TaylorF1:
	case TaylorF2:
	case BCV:
	case BCVSpin:
	   
		overlapin->param.startPhase = 0.;
		LALInspiralWave(status->statusPtr, &filter1, &overlapin->param);
		CHECKSTATUSPTR(status);
		
		/* 
		 * When the approximant is BCV, BCVSpin, TaylorF1 or TaylorF2 
		 * no FT is needed as the approximant generates signal in the Fourier domain.
		 */
	   
		/* print out for debugging
		 for (i=0; i<output1.length; i++) printf("%d %e %e\n", i, output1.data[i], output2.data[i]);
		 LALREAL4VectorFFT(status->statusPtr, &filter1, &output1, overlapin->fwdp);
		 CHECKSTATUSPTR(status);
		 LALREAL4VectorFFT(status->statusPtr, &filter2, &output2, overlapin->fwdp);
		 CHECKSTATUSPTR(status);
		 */
		break;
	default:
		if ( lalDebugLevel&LALINFO ) 
		{
			LALInfo(status, "Invalid choice of approximant\n");
			LALPrintError("\tInvalid choice of approximant\n");
		}
		break;

   }
   /* Normalise the template just created */
   normin.psd = &(overlapin->psd);
   normin.df = overlapin->param.tSampling / (REAL8) overlapin->signal.length;
   normin.fCutoff = overlapin->param.fFinal;
   normin.samplingRate = overlapin->param.tSampling;
   LALInspiralWaveNormaliseLSO(status->statusPtr, &filter1, &norm, &normin);
   CHECKSTATUSPTR(status);

   /* Get the template orthogonal to the normalized template just created */
   LALInspiralGetOrthoNormalFilter(&filter2, &filter1);
   CHECKSTATUSPTR(status);
   /* Compute the overlap of the two orthonormal templates with the signal in corrin */
   corrin.fCutoff = overlapin->param.fFinal;
   corrin.samplingRate = overlapin->param.tSampling;
   corrin.df = overlapin->param.tSampling / (REAL8) overlapin->signal.length;
   corrin.psd = overlapin->psd;
   corrin.signal1 = overlapin->signal;
   corrin.signal2 = filter1;
   corrin.revp = overlapin->revp;
   LALInspiralWaveCorrelate(status->statusPtr, &output1, corrin);
   CHECKSTATUSPTR(status);
   corrin.signal2 = filter2;
   LALInspiralWaveCorrelate(status->statusPtr, &output2, corrin);
   CHECKSTATUSPTR(status);

   /* Compute the maximum of the overlap by adding the two correlations in quadrature */
   nBegin = overlapin->nBegin;
   nEnd = output1.length - overlapin->nEnd;
   x = output1.data[nBegin];
   y = output2.data[nBegin];
   overlapout->max = x*x + y*y;
   overlapout->bin = nBegin;
   overlapout->phase = atan2(y,x);
   for (i=nBegin; i<nEnd; i++) {
       x = output1.data[i];
       y = output2.data[i];
       z = x*x + y*y;
       if (z>overlapout->max) {
          overlapout->max = z;
          overlapout->bin = i;
          overlapout->phase = atan2(y,x);
       }
   }

   phase = overlapout->phase;


   /*      For efficiency of the code, we replicate the loop as the case maybe 
    *      This is done so that the if ( ) conditions can be placed outside the
    *      loop 
    */

   /* If an extended output is not requested - then just fill out the output
    * (as before). This can be indicated by setting overlapin->ifExtOutput to zero
    * */

   if (  !(overlapin->ifExtOutput) ) 
   {
       /* No xcorrelation or filters requested on output - so just fill out
        * output vector and exit
        * */
       for (i=0; i<(int)output->length; i++) { 
           output->data[i] = cos(phase) * output1.data[i] 
                   + sin(phase) * output2.data[i];
       }
   }
   else { 

       /* Else we assume that all of  them are requested to be output. Eventually we 
        * would want the freedom to output any one of the 4 vectors but right now we
        * output all the four together
        * */

       /* Check that the space has been allocated to recv the xcorr and  filters */
       ASSERT (overlapout->xcorr1->length = overlapin->signal.length, 
               status, LALNOISEMODELSH_ESIZE, LALNOISEMODELSH_MSGESIZE);
       ASSERT (overlapout->xcorr1->data,  status, LALNOISEMODELSH_ENULL, LALNOISEMODELSH_MSGENULL);
      
       ASSERT (overlapout->xcorr2->length = overlapin->signal.length, 
               status, LALNOISEMODELSH_ESIZE, LALNOISEMODELSH_MSGESIZE);
       ASSERT (overlapout->xcorr2->data,  status, LALNOISEMODELSH_ENULL, LALNOISEMODELSH_MSGENULL);
      
       ASSERT (overlapout->filter1->length = overlapin->signal.length, 
               status, LALNOISEMODELSH_ESIZE, LALNOISEMODELSH_MSGESIZE);
       ASSERT (overlapout->filter1->data,  status, LALNOISEMODELSH_ENULL, LALNOISEMODELSH_MSGENULL);
       
       ASSERT (overlapout->filter2->length = overlapin->signal.length, 
               status, LALNOISEMODELSH_ESIZE, LALNOISEMODELSH_MSGESIZE);
       ASSERT (overlapout->filter2->data,  status, LALNOISEMODELSH_ENULL, LALNOISEMODELSH_MSGENULL);

       /* Now fill everything in the extended output */
       for (i=0; i<(int)output->length; i++) { 
           output->data[i] = cos(phase) * output1.data[i] 
                   + sin(phase) * output2.data[i];

           overlapout->xcorr1->data[i]  = output1.data[i];
           overlapout->xcorr2->data[i]  = output2.data[i];
           overlapout->filter1->data[i] = filter1.data[i];
           overlapout->filter2->data[i] = filter2.data[i];
       }
   }

   overlapout->max = sqrt(overlapout->max);


   if (filter1.data!=NULL) LALFree(filter1.data);
   if (filter2.data!=NULL) LALFree(filter2.data);
   if (output1.data!=NULL) LALFree(output1.data);
   if (output2.data!=NULL) LALFree(output2.data);

   DETATCHSTATUSPTR(status);
   RETURN(status);
}

/* Routine to compute a vector that is orthogonal to the input vector 
 * without changing its norm by multiplying with e^(i pi/2);  Assumes
 * that the input vector is the Fourier transform with the positive 
 * and negative frequencies pairs arranged as in fftw: f[i] <->f[ n-i].
 */

void LALInspiralGetOrthoNormalFilter(REAL4Vector *filter2, REAL4Vector *filter1)
{
    int i,n,nby2;

    n = filter1->length;
    nby2 = n/2;
    filter2->data[0] = filter1->data[0];
    filter2->data[nby2] = filter1->data[nby2];
    for (i=1; i<nby2; i++)
    {
        filter2->data[i] = -filter1->data[n-i];
        filter2->data[n-i] = filter1->data[i];
    }
}

