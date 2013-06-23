/* 
 *  LALInferenceLikelihood.c:  Bayesian Followup likelihood functions
 *
 *  Copyright (C) 2009 Ilya Mandel, Vivien Raymond, Christian Roever,
 *  Marc van der Sluys and John Veitch, Will M. Farr
 *
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

#include <lal/LALInferenceLikelihood.h>
#include <lal/LALInference.h>
#include <lal/DetResponse.h>
#include <lal/TimeDelay.h>
#include <lal/TimeSeries.h>
#include <lal/Units.h>
#include <lal/Sequence.h>
#include <lal/FrequencySeries.h>
#include <lal/TimeFreqFFT.h>
#include <gsl/gsl_sf_bessel.h>
#include <gsl/gsl_sf_bessel.h>

#include <lal/LALInferenceTemplate.h>

#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif


/** Internal functions which are not exported */
//static UINT4 LIGOTimeGPSToNearestIndex(const LIGOTimeGPS *tm, const REAL8TimeSeries *series);
//static UINT4 LIGOTimeGPSToNearestIndex(const LIGOTimeGPS *tm, const REAL8TimeSeries *series) {
//  REAL8 dT = XLALGPSDiff(tm, &(series->epoch));
//
//  return (UINT4) (round(dT/series->deltaT));
//}

/* The time-domain weight corresponding to the (two-sided) noise power
   spectrum S(f) is defined to be:

   s(tau) == \int_{-infty}^\infty df \frac{\exp(2 \pi i f tau)}{S(f)}

*/

//static UINT4 nextPowerOfTwo(const UINT4 n);
//static UINT4 nextPowerOfTwo(const UINT4 n) {
//  UINT4 np2 = 1;
//  
//  for (np2 = 1; np2 < n; np2 *= 2) ; /* Keep doubling until >= n */
//
//  return np2;
//}

//static void padREAL8Sequence(REAL8Sequence *padded, const REAL8Sequence *data);
//static void padREAL8Sequence(REAL8Sequence *padded, const REAL8Sequence *data) {
//  if (padded->length < data->length) {
//    fprintf(stderr, "padREAL8Sequence: padded sequence too short (in %s, line %d)", __FILE__, __LINE__);
//    exit(1);
//  }
//
//  memset(padded->data, 0, padded->length*sizeof(padded->data[0]));
//  memcpy(padded->data, data->data, data->length*sizeof(data->data[0]));
//}

//static void padWrappedREAL8Sequence(REAL8Sequence *padded, const REAL8Sequence *data);
//static void padWrappedREAL8Sequence(REAL8Sequence *padded, const REAL8Sequence *data) {
//  UINT4 i;
//  UINT4 np = padded->length;
//  UINT4 nd = data->length;
//
//  if (np < nd) {
//    fprintf(stderr, "padWrappedREAL8Sequence: padded sequence too short (in %s, line %d)", __FILE__, __LINE__);
//    exit(1);
//  }
//
//  memset(padded->data, 0, np*sizeof(padded->data[0]));
//
//  padded->data[0] = data->data[0];
//  for (i = 1; i <= (nd-1)/2; i++) {
//    padded->data[i] = data->data[i]; /* Positive times/frequencies. */
//    padded->data[np-i] = data->data[nd-i]; /* Wrapped, negative times/frequencies. */
//  }
//  if (nd % 2 == 0) { /* If even, take care of singleton positive frequency. */
//    padded->data[nd/2] = data->data[nd/2];
//  }
//}

void LALInferenceInitLikelihood(LALInferenceRunState *runState)
{
    char help[]="\
                 (--zeroLogLike)                  Use flat, null likelihood.\n\
                 (--studentTLikelihood)           Use the Student-T Likelihood that marginalizes over noise.\n\
                 (--correlatedGaussianLikelihood) Use analytic, correlated Gaussian for Likelihood.\n\
                 (--bimodalGaussianLikelihood)    Use analytic, bimodal correlated Gaussian for Likelihood.\n\
                 (--rosenbrockLikelihood)         Use analytic, Rosenbrock banana for Likelihood.\n\
                 (--noiseonly)                    Using noise-only likelihood.\n\
                 (--margphi)                      Using marginalised phase likelihood.\n";

    ProcessParamsTable *commandLine=runState->commandLine;
    LALInferenceIFOData *ifo=runState->data;
    ProcessParamsTable *ppt=LALInferenceGetProcParamVal(commandLine,"--template");

    /* Print command line arguments if help requested */
    if(LALInferenceGetProcParamVal(runState->commandLine,"--help"))
    {
        fprintf(stdout,"%s",help);
        while(ifo) {
            fprintf(stdout,"(--dof-%s DoF)\tDegrees of freedom for %s\n",ifo->name,ifo->name);
            ifo=ifo->next;
        }
        return;
    }

   if (LALInferenceGetProcParamVal(commandLine, "--zeroLogLike")) {
    /* Use zero log(L) */
    runState->likelihood=&LALInferenceZeroLogLikelihood;
   } else if (LALInferenceGetProcParamVal(commandLine, "--correlatedGaussianLikelihood")) {
    runState->likelihood=&LALInferenceCorrelatedAnalyticLogLikelihood;
   } else if (LALInferenceGetProcParamVal(commandLine, "--bimodalGaussianLikelihood")) {
    runState->likelihood=&LALInferenceBimodalCorrelatedAnalyticLogLikelihood;
   } else if (LALInferenceGetProcParamVal(commandLine, "--rosenbrockLikelihood")) {
    runState->likelihood=&LALInferenceRosenbrockLogLikelihood;
   } else if (LALInferenceGetProcParamVal(commandLine, "--studentTLikelihood")) {
    fprintf(stderr, "Using Student's T Likelihood.\n");
    runState->likelihood=&LALInferenceFreqDomainStudentTLogLikelihood;

    /* Set the noise model evidence to the student t model value */
    LALInferenceTemplateNullFreqdomain(runState->data);
    REAL8 noiseZ=LALInferenceFreqDomainStudentTLogLikelihood(runState->currentParams,runState->data,&LALInferenceTemplateNullFreqdomain);
    LALInferenceAddVariable(runState->algorithmParams,"logZnoise",&noiseZ,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_FIXED);
    fprintf(stdout,"Student-t Noise evidence %lf\n",noiseZ);

   } else if (LALInferenceGetProcParamVal(commandLine, "--noiseonly")) {
    fprintf(stderr, "Using noise-only likelihood.\n");
    runState->likelihood=&LALInferenceNoiseOnlyLogLikelihood;
   }  else if (ppt){
     if(!strcmp("SineGauss",ppt->value) || !strcmp("SineGaussF",ppt->value))
        runState->likelihood=&LALInferenceUndecomposedFreqDomainLogLikelihood_Burst;
      else if(!strcmp("RingdownF",ppt->value))
      {
          runState->likelihood=&LALInferenceUndecomposedFreqDomainLogLikelihood_RD;
          if(LALInferenceGetProcParamVal(commandLine,"--margphi")){
            printf("Using Marginalise Phase Likelihood\n");
            runState->likelihood=&LALInferenceMarginalisedPhaseLogLikelihood_RD;
          }
      }
      else if(!strcmp("HMNS",ppt->value) )
        runState->likelihood=&LALInferenceMarginalisedPhaseLogLikelihood_HMNS;
      else if(!strcmp("BestIFO",ppt->value))
        runState->likelihood=&LALInferenceUndecomposedFreqDomainLogLikelihood_BestIFO;
      else if(LALInferenceGetProcParamVal(commandLine,"--powerburst"))
        runState->likelihood=&LALInferenceExtraPowerLogLikelihood;
   } else if (LALInferenceGetProcParamVal(commandLine, "--margphi")) {
    fprintf(stderr, "Using marginalised phase likelihood.\n");
    runState->likelihood=&LALInferenceMarginalisedPhaseLogLikelihood;
   }else {
    runState->likelihood=&LALInferenceUndecomposedFreqDomainLogLikelihood;
   }

    return;
}

/* Scaling used for the analytic likelihood parameters */
  static const REAL8 scaling[15] = {
    1.0,
    1.0,
    20.0/M_PI,
    10.0/M_PI,
    20.0/M_PI,
    10.0/M_PI,
    10.0/M_PI,
    0.1,
    10.0,
    10.0,
    10.0,
    20.0/M_PI,
    20.0/M_PI,
    10.0/M_PI,
    10.0/M_PI};

/* Covariance matrix for use in analytic likelihoods */
  static const REAL8 CM[15][15] = {{0.045991865933182365, -0.005489748382557155, -0.01025067223674548, 0.0020087713726603213, -0.0032648855847982987, -0.0034218261781145264, -0.0037173401838545774, -0.007694897715679858, 0.005260905282822458, 0.0013607957548231718, 0.001970785895702776, 0.006708452591621081, -0.005107684668720825, 0.004402554308030673, -0.00334987648531921},
                              {-0.005489748382557152, 0.05478640427684032, -0.004786202916836846, -0.007930397407501268, -0.0005945107515129139, 0.004858466255616657, -0.011667819871670204, 0.003169780190169035, 0.006761345004654851, -0.0037599761532668475, 0.005571796842520162, -0.0071098291510566895, -0.004477773540640284, -0.011250694688474672, 0.007465228985669282},
                              {-0.01025067223674548, -0.004786202916836844, 0.044324704403674524, -0.0010572820723801645, -0.009885693540838514, -0.0048321205972943464, -0.004576186966267275, 0.0025107211483955676, -0.010126911913571181, 0.01153595152487264, 0.005773054728678472, 0.005286558230422045, -0.0055438798694137734, 0.0044772210361854765, -0.00620416958073918},
                              {0.0020087713726603213, -0.007930397407501268, -0.0010572820723801636, 0.029861342087731065, -0.007803477134405363, -0.0011466944120756021, 0.009925736654597632, -0.0007664415942207051, -0.0057593957402320385, -0.00027248233573270216, 0.003885350572544307, 0.00022362281488693097, 0.006609741178005571, -0.003292722856107429, -0.005873218251875897},
                              {-0.0032648855847982987, -0.0005945107515129156, -0.009885693540838514, -0.007803477134405362, 0.0538403407841302, -0.007446654755103316, -0.0025216534232170153, 0.004499568241334517, 0.009591034277125177, 0.00008612746932654223, 0.003386296829505543, -0.002600737873367083, 0.000621148057330571, -0.006603857049454523, -0.009241221870695395},
                              {-0.0034218261781145264, 0.004858466255616657, -0.004832120597294347, -0.0011466944120756015, -0.007446654755103318, 0.043746559133865104, 0.008962713024625965, -0.011099652042761613, -0.0006620240117921668, -0.0012591530037708058, -0.006899982952117269, 0.0019732354732442878, -0.002445676747004324, -0.006454778807421816, 0.0033303577606412765},
                              {-0.00371734018385458, -0.011667819871670206, -0.004576186966267273, 0.009925736654597632, -0.0025216534232170153, 0.008962713024625965, 0.03664582756831382, -0.009470328827284009, -0.006213741694945105, 0.007118775954484294, -0.0006741237990418526, -0.006003374957986355, 0.005718636997353189, -0.0005191095254772077, -0.008466566781233205},
                              {-0.007694897715679857, 0.0031697801901690347, 0.002510721148395566, -0.0007664415942207059, 0.004499568241334515, -0.011099652042761617, -0.009470328827284016, 0.057734267068088, 0.005521731225009532, -0.017008048805405164, 0.006749693090695894, -0.006348460110898, -0.007879244727681924, -0.005321753837620446, 0.011126783289057604},
                              {0.005260905282822458, 0.0067613450046548505, -0.010126911913571181, -0.00575939574023204, 0.009591034277125177, -0.0006620240117921668, -0.006213741694945106, 0.005521731225009532, 0.04610670018969681, -0.010427010812879566, -0.0009861561285861987, -0.008896020395949732, -0.0037627528719902485, 0.00033704453138913093, -0.003173552163182467},
                              {0.0013607957548231744, -0.0037599761532668475, 0.01153595152487264, -0.0002724823357326985, 0.0000861274693265406, -0.0012591530037708062, 0.007118775954484294, -0.01700804880540517, -0.010427010812879568, 0.05909125052583998, 0.002192545816395299, -0.002057672237277737, -0.004801518314458135, -0.014065326026662672, -0.005619012077114913},
                              {0.0019707858957027763, 0.005571796842520162, 0.005773054728678472, 0.003885350572544309, 0.003386296829505542, -0.006899982952117272, -0.0006741237990418522, 0.006749693090695893, -0.0009861561285862005, 0.0021925458163952988, 0.024417715762416557, -0.003037163447600162, -0.011173674374382736, -0.0008193127407211239, -0.007137012700864866},
                              {0.006708452591621083, -0.0071098291510566895, 0.005286558230422046, 0.00022362281488693216, -0.0026007378733670806, 0.0019732354732442886, -0.006003374957986352, -0.006348460110897999, -0.008896020395949732, -0.002057672237277737, -0.003037163447600163, 0.04762367868805726, 0.0008818947598625008, -0.0007262691465810616, -0.006482422704208912},
                              {-0.005107684668720825, -0.0044777735406402895, -0.005543879869413772, 0.006609741178005571, 0.0006211480573305693, -0.002445676747004324, 0.0057186369973531905, -0.00787924472768192, -0.003762752871990247, -0.004801518314458137, -0.011173674374382736, 0.0008818947598624995, 0.042639958466440225, 0.0010194948614718209, 0.0033872675386130637},
                              {0.004402554308030674, -0.011250694688474675, 0.004477221036185477, -0.003292722856107429, -0.006603857049454523, -0.006454778807421815, -0.0005191095254772072, -0.005321753837620446, 0.0003370445313891318, -0.014065326026662679, -0.0008193127407211239, -0.0007262691465810616, 0.0010194948614718226, 0.05244900188599414, -0.000256550861960499},
                              {-0.00334987648531921, 0.007465228985669282, -0.006204169580739178, -0.005873218251875899, -0.009241221870695395, 0.003330357760641278, -0.008466566781233205, 0.011126783289057604, -0.0031735521631824654, -0.005619012077114915, -0.007137012700864866, -0.006482422704208912, 0.0033872675386130632, -0.000256550861960499, 0.05380987317762257}};




/* ============ Likelihood computations: ========== */

/** For testing purposes (for instance sampling the prior), likelihood that returns 0.0 = log(1) every
 time.  Activated with the --zeroLogLike command flag. */
REAL8 LALInferenceZeroLogLikelihood(LALInferenceVariables UNUSED *currentParams, LALInferenceIFOData UNUSED *data, LALInferenceTemplateFunction UNUSED template) {
  return 0.0;
}

REAL8 LALInferenceNoiseOnlyLogLikelihood(LALInferenceVariables *currentParams, LALInferenceIFOData *data, LALInferenceTemplateFunction UNUSED template)
/***************************************************************/
/* (log-) likelihood function.                                 */
/* Returns the non-normalised logarithmic likelihood           */
/* for noise-only models of the data                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Required (`currentParams') parameters are:                  */
/*   - "psdscale"  (gslMatrix)                                 */
/***************************************************************/
{
  double diffRe, diffIm, diffSquared;
  double dataReal, dataImag;
  REAL8 loglikeli;
  int i, j, lower, upper, ifo;
  LALInferenceIFOData *dataPtr;
  double chisquared;
  double deltaT, TwoDeltaToverN, deltaF;

  //noise model meta parameters
  gsl_matrix *lines   = NULL;//pointer to matrix holding line centroids
  gsl_matrix *widths  = NULL;//pointer to matrix holding line widths
  gsl_matrix *nparams = NULL;//pointer to matrix holding noise parameters

  gsl_matrix *psdBandsMin  = NULL;//pointer to matrix holding min frequencies for psd model
  gsl_matrix *psdBandsMax = NULL;//pointer to matrix holding max frequencies for psd model

  //double dflog=1.0;        //logarithmic spacing of psd parameters

  int Nblock = 1;            //number of frequency blocks per IFO
  int Nlines = 1;            //number of lines to be removed
  int psdFlag;               //flag for including psd fitting
  int lineFlag;              //flag for excluding lines from integration
  int lineimin = 0;
  int lineimax = 0;
  int lineSwitch;          //switch inside integration to exclude bins

  //line removal parameters
  lineFlag = *((INT4 *)LALInferenceGetVariable(currentParams, "removeLinesFlag"));
  if(lineFlag)
  {
    //Add line matrices to variable lists
    lines  = *(gsl_matrix **)LALInferenceGetVariable(currentParams, "line_center");
    widths = *(gsl_matrix **)LALInferenceGetVariable(currentParams, "line_width");
    Nlines = (int)lines->size2;
  }
  int lines_array[Nlines];
  int widths_array[Nlines];

  //check if psd parameters are included in the model
  psdFlag = *((INT4 *)LALInferenceGetVariable(currentParams, "psdScaleFlag"));
  if(psdFlag)
  {
    //if so, store current noise parameters in easily accessible matrix
    nparams = *((gsl_matrix **)LALInferenceGetVariable(currentParams, "psdscale"));
    Nblock = (int)nparams->size2;

    //dflog = *(REAL8*) LALInferenceGetVariable(currentParams, "logdeltaf");

    psdBandsMin = *((gsl_matrix **)LALInferenceGetVariable(currentParams, "psdBandsMin"));
    psdBandsMax = *((gsl_matrix **)LALInferenceGetVariable(currentParams, "psdBandsMax"));

  }
  double alpha[Nblock];
  double lnalpha[Nblock];

  double psdBandsMin_array[Nblock];
  double psdBandsMax_array[Nblock];

  chisquared = 0.0;
  /* loop over data (different interferometers): */
  dataPtr = data;
  ifo=0;

  while (dataPtr != NULL) {
    /* The parameters the Likelihood function can handle by itself   */
    /* (and which shouldn't affect the template function) are        */
    /* sky location (ra, dec), polarisation and signal arrival time. */
    /* Note that the template function shifts the waveform to so that*/
    /* t_c corresponds to the "time" parameter in                    */
    /* IFOdata->modelParams (set, e.g., from the trigger value).     */

    /* Reset log-likelihood */
    dataPtr->loglikelihood = 0.0;

    /* determine frequency range & loop over frequency bins: */
    deltaT = dataPtr->timeData->deltaT;
    deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);

    lower = (UINT4)ceil(dataPtr->fLow / deltaF);
    upper = (UINT4)floor(dataPtr->fHigh / deltaF);
    TwoDeltaToverN = 2.0 * deltaT / ((double) dataPtr->timeData->data->length);

    //Set up noise PSD meta parameters
    for(i=0; i<Nblock; i++)
    {
      if(psdFlag)
      {
        alpha[i]   = gsl_matrix_get(nparams,ifo,i);
        lnalpha[i] = log(alpha[i]);

        psdBandsMin_array[i] = gsl_matrix_get(psdBandsMin,ifo,i);
        psdBandsMax_array[i] = gsl_matrix_get(psdBandsMax,ifo,i);

      }
      else
      {
        alpha[i]=1.0;
        lnalpha[i]=0.0;
      }
    }

    //Set up psd line arrays
    for(j=0;j<Nlines;j++)
    {
      if(lineFlag)
      {

        //find range of fourier fourier bins which are excluded from integration
        lines_array[j]  = (int)gsl_matrix_get(lines,ifo,j);//lineimin = (int)(gsl_matrix_get(lines,ifo,j) - gsl_matrix_get(widths,ifo,j));
        widths_array[j] = (int)gsl_matrix_get(widths,ifo,j);//lineimax = (int)(gsl_matrix_get(lines,ifo,j) + gsl_matrix_get(widths,ifo,j));
      }
      else
      {
        lines_array[j]=0;
        widths_array[j]=0;
      }
    }

    for (i=lower; i<=upper; ++i)
    {

      dataReal     = creal(dataPtr->freqData->data->data[i]) / deltaT;
      dataImag     = cimag(dataPtr->freqData->data->data[i]) / deltaT;
      
      /* compute squared difference & 'chi-squared': */
      diffRe       = dataReal;         // Difference in real parts...
      diffIm       = dataImag;         // ...and imaginary parts, and...
      diffSquared  = diffRe*diffRe + diffIm*diffIm ;  // ...squared difference of the 2 complex figures.
      
      REAL8 temp = ((TwoDeltaToverN * diffSquared) / dataPtr->oneSidedNoisePowerSpectrum->data->data[i]);

      /* Add noise PSD parameters to the model */
      if(psdFlag)
      {
        for(j=0; j<Nblock; j++)
        {
            if (i >= psdBandsMin_array[j] && i <= psdBandsMax_array[j])
            {
                temp  /= alpha[j];
                temp  += lnalpha[j];
            }
        }
      }

      /* Remove lines from model */
      lineSwitch=1;

      if(lineFlag)
      {
        for(j=0;j<Nlines;j++)
        {
          //find range of fourier fourier bins which are excluded from integration
          lineimin = lines_array[j] - widths_array[j];//gsl_matrix_get(widths,ifo,j));
          lineimax = lines_array[j] + widths_array[j];//(int)(gsl_matrix_get(lines,ifo,j) + gsl_matrix_get(widths,ifo,j));

          //if the current bin is inside the exluded region, set the switch to 0
          if(i>lineimin && i<lineimax) lineSwitch=0;
        }
      }

      /*only sum over bins which are outside of excluded regions */
      if(lineSwitch)
      {
        chisquared  += temp;
        dataPtr->loglikelihood -= temp;
      }
      
     }
    ifo++; //increment IFO counter for noise parameters
    dataPtr = dataPtr->next;
  }

  loglikeli = -1.0 * chisquared; // note (again): the log-likelihood is unnormalised!
  return(loglikeli);
}

REAL8 LALInferenceUndecomposedFreqDomainLogLikelihood(LALInferenceVariables *currentParams, LALInferenceIFOData * data,       LALInferenceTemplateFunction templt)
/***************************************************************/
/* (log-) likelihood function.                                 */
/* Returns the non-normalised logarithmic likelihood.          */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Required (`currentParams') parameters are:                  */
/*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
/*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
/*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
/*   - "distance"        (REAL8, Mpc, >0)                      */
/*   - "time"            (REAL8, GPS sec.)                     */
/***************************************************************/
{
  //static int timeDomainWarning = 0;
  double Fplus, Fcross;
  double FplusScaled, FcrossScaled;
  double diffRe, diffIm, diffSquared;
  double dataReal, dataImag;
  double dre, dim,newRe,newIm;
  REAL8 loglikeli;
  REAL8 plainTemplateReal, plainTemplateImag;
  REAL8 templateReal, templateImag;
  int i, j, lower, upper, ifo;
  LALInferenceIFOData *dataPtr;
  double ra, dec, psi, distMpc, gmst;
  double GPSdouble;
  LIGOTimeGPS GPSlal;
  double chisquared;
  double timedelay;  /* time delay b/w iterferometer & geocenter w.r.t. sky location */
  double timeshift;  /* time shift (not necessarily same as above)                   */
  double deltaT, TwoDeltaToverN, deltaF, twopit, f, re, im;
  double timeTmp;
	double mc;
  int different;
	UINT4 logDistFlag=0;
  LALStatus status;
  memset(&status,0,sizeof(status));
  LALInferenceVariables intrinsicParams;

  if(data==NULL) {XLAL_ERROR_REAL8(XLAL_EINVAL,"ERROR: Encountered NULL data pointer in likelihood\n");}

  //noise model meta parameters
  gsl_matrix *lines   = NULL;//pointer to matrix holding line centroids
  gsl_matrix *widths  = NULL;//pointer to matrix holding line widths
  gsl_matrix *nparams = NULL;//pointer to matrix holding noise parameters

  gsl_matrix *psdBandsMin  = NULL;//pointer to matrix holding min frequencies for psd model
  gsl_matrix *psdBandsMax = NULL;//pointer to matrix holding max frequencies for psd model

  //double dflog = 1.0;        //logarithmic spacing of psd parameters
  
  int Nblock = 1;            //number of frequency blocks per IFO
  int Nlines = 1;            //number of lines to be removed
  int psdFlag = 0;           //flag for including psd fitting
  int lineFlag = 0;          //flag for excluding lines from integration
  int lineimin = 0;
  int lineimax = 0;
  int lineSwitch;          //switch inside integration to exclude bins

  //line removal parameters
  if(LALInferenceCheckVariable(currentParams, "removeLinesFlag"))
    lineFlag = *((INT4 *)LALInferenceGetVariable(currentParams, "removeLinesFlag"));
  if(lineFlag)
  {
    //Add line matrices to variable lists
    lines  = *(gsl_matrix **)LALInferenceGetVariable(currentParams, "line_center");
    widths = *(gsl_matrix **)LALInferenceGetVariable(currentParams, "line_width");
    Nlines = (int)lines->size2;
  }
  int lines_array[Nlines];
  int widths_array[Nlines];

  //check if psd parameters are included in the model
  if(LALInferenceCheckVariable(currentParams, "psdScaleFlag"))
    psdFlag = *((INT4 *)LALInferenceGetVariable(currentParams, "psdScaleFlag"));
  if(psdFlag)
  {
    //if so, store current noise parameters in easily accessible matrix
    nparams = *((gsl_matrix **)LALInferenceGetVariable(currentParams, "psdscale"));
    Nblock = (int)nparams->size2;

    //dflog = *(REAL8*) LALInferenceGetVariable(currentParams, "logdeltaf");

    psdBandsMin = *((gsl_matrix **)LALInferenceGetVariable(currentParams, "psdBandsMin"));
    psdBandsMax = *((gsl_matrix **)LALInferenceGetVariable(currentParams, "psdBandsMax"));

  }
  double alpha[Nblock];
  double lnalpha[Nblock];

  double psdBandsMin_array[Nblock];
  double psdBandsMax_array[Nblock];

  logDistFlag=LALInferenceCheckVariable(currentParams, "logdistance");
  if(LALInferenceCheckVariable(currentParams,"logmc")){
    mc=exp(*(REAL8 *)LALInferenceGetVariable(currentParams,"logmc"));
    LALInferenceAddVariable(currentParams,"chirpmass",&mc,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
  }

  /* determine source's sky location & orientation parameters: */
  ra        = *(REAL8*) LALInferenceGetVariable(currentParams, "rightascension"); /* radian      */
  dec       = *(REAL8*) LALInferenceGetVariable(currentParams, "declination");    /* radian      */
  psi       = *(REAL8*) LALInferenceGetVariable(currentParams, "polarisation");   /* radian      */
  GPSdouble = *(REAL8*) LALInferenceGetVariable(currentParams, "time");           /* GPS seconds */
	if(logDistFlag)
		 distMpc = exp(*(REAL8*)LALInferenceGetVariable(currentParams,"logdistance"));
	else
		 distMpc   = *(REAL8*) LALInferenceGetVariable(currentParams, "distance");       /* Mpc         */

  /* figure out GMST: */
  //XLALINT8NSToGPS(&GPSlal, floor(1e9 * GPSdouble + 0.5));
  XLALGPSSetREAL8(&GPSlal, GPSdouble);
  //UandA.units    = MST_RAD;
  //UandA.accuracy = LALLEAPSEC_LOOSE;
  //LALGPStoGMST1(&status, &gmst, &GPSlal, &UandA);
  gmst=XLALGreenwichMeanSiderealTime(&GPSlal);
  intrinsicParams.head      = NULL;
  intrinsicParams.dimension = 0;
  LALInferenceCopyVariables(currentParams, &intrinsicParams);
  LALInferenceRemoveVariable(&intrinsicParams, "rightascension");
  LALInferenceRemoveVariable(&intrinsicParams, "declination");
  LALInferenceRemoveVariable(&intrinsicParams, "polarisation");
  LALInferenceRemoveVariable(&intrinsicParams, "time");
  
  /* Remove likelihood and prior params from intrinsicParams before equality with currentParams is done */
   if (LALInferenceCheckVariable(&intrinsicParams,"deltalogL"))
  LALInferenceRemoveVariable(&intrinsicParams, "deltalogL");
    if (LALInferenceCheckVariable(&intrinsicParams,"logL"))
  LALInferenceRemoveVariable(&intrinsicParams, "logL");
    if (LALInferenceCheckVariable(&intrinsicParams,"deltaloglV1"))
  LALInferenceRemoveVariable(&intrinsicParams, "deltaloglV1");
    if (LALInferenceCheckVariable(&intrinsicParams,"deltaloglL1"))
  LALInferenceRemoveVariable(&intrinsicParams, "deltaloglL1");
    if (LALInferenceCheckVariable(&intrinsicParams,"deltaloglH1"))
  LALInferenceRemoveVariable(&intrinsicParams, "deltaloglH1");
      if (LALInferenceCheckVariable(&intrinsicParams,"logw"))
  LALInferenceRemoveVariable(&intrinsicParams, "logw");
        if (LALInferenceCheckVariable(&intrinsicParams,"logPrior"))
  LALInferenceRemoveVariable(&intrinsicParams, "logPrior");
  
	if(logDistFlag)
			LALInferenceRemoveVariable(&intrinsicParams, "logdistance");
	else
			LALInferenceRemoveVariable(&intrinsicParams, "distance");
  // TODO: add pointer to template function here.
  // (otherwise same parameters but different template will lead to no re-computation!!)

  chisquared = 0.0;
  /* loop over data (different interferometers): */
  dataPtr = data;
  ifo=0;

  while (dataPtr != NULL) {
    /* The parameters the Likelihood function can handle by itself   */
    /* (and which shouldn't affect the template function) are        */
    /* sky location (ra, dec), polarisation and signal arrival time. */
    /* Note that the template function shifts the waveform to so that*/
	/* t_c corresponds to the "time" parameter in                    */
	/* IFOdata->modelParams (set, e.g., from the trigger value).     */
    
    /* Reset log-likelihood */
    dataPtr->loglikelihood = 0.0;

    /* Compare parameter values with parameter values corresponding  */
    /* to currently stored template; ignore "time" variable:         */
    if (LALInferenceCheckVariable(dataPtr->modelParams, "time")) {
      timeTmp = *(REAL8 *) LALInferenceGetVariable(dataPtr->modelParams, "time");
      LALInferenceRemoveVariable(dataPtr->modelParams, "time");
    }
    else timeTmp = GPSdouble;
    different = LALInferenceCompareVariables(dataPtr->modelParams, &intrinsicParams);
    /* "different" now may also mean that "dataPtr->modelParams" */
    /* wasn't allocated yet (as in the very 1st iteration).      */

    if (different) { /* template needs to be re-computed: */
    //printf("IM RECALCULATING THE TEMPLATE!\n");
      LALInferenceCopyVariables(&intrinsicParams, dataPtr->modelParams);
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
      templt(dataPtr);
      if(XLALGetBaseErrno()==XLAL_FAILURE) /* Template generation failed in a known way, set -Inf likelihood */
          return(-DBL_MAX);

      if (dataPtr->modelDomain == LAL_SIM_DOMAIN_TIME) {
	/* TD --> FD. */
	LALInferenceExecuteFT(dataPtr);
      }
    }
    else { /* no re-computation necessary. Return back "time" value, do nothing else: */
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
    }

    /* Template is now in dataPtr->timeFreqModelhPlus and hCross */

    /* determine beam pattern response (F_plus and F_cross) for given Ifo: */
    XLALComputeDetAMResponse(&Fplus, &Fcross,
                             dataPtr->detector->response,
			     ra, dec, psi, gmst);
    /* signal arrival time (relative to geocenter); */
    timedelay = XLALTimeDelayFromEarthCenter(dataPtr->detector->location,
                                             ra, dec, &GPSlal);
    /* (negative timedelay means signal arrives earlier at Ifo than at geocenter, etc.) */
    /* amount by which to time-shift template (not necessarily same as above "timedelay"): */
    timeshift =  (GPSdouble - (*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time"))) + timedelay;

    twopit    = LAL_TWOPI * timeshift;
//printf("Template's time %10.2lf time %10.2lf epoch=%10.2lf\n", GPSdouble,(*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time")),XLALGPSGetREAL8(&(dataPtr->freqData->epoch))    );

    /* include distance (overall amplitude) effect in Fplus/Fcross: */
    FplusScaled  = Fplus  / distMpc;
    FcrossScaled = Fcross / distMpc;

    if (LALInferenceCheckVariable(currentParams, "crazyInjectionHLSign") &&
        *((INT4 *)LALInferenceGetVariable(currentParams, "crazyInjectionHLSign"))) {
      if (strstr(dataPtr->name, "H") || strstr(dataPtr->name, "L")) {
        FplusScaled *= -1.0;
        FcrossScaled *= -1.0;
      }
    }

    dataPtr->fPlus = FplusScaled;
    dataPtr->fCross = FcrossScaled;
    dataPtr->timeshift = timeshift;

 //FILE *testout=fopen("test_likeliLAL.txt","w");
 //fprintf(testout, "f PSD dataRe dataIm signalRe signalIm\n");
    /* determine frequency range & loop over frequency bins: */
    deltaT = dataPtr->timeData->deltaT;
    deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);
    // printf("deltaF %g, Nt %d, deltaT %g\n", deltaF, dataPtr->timeData->data->length, dataPtr->timeData->deltaT);
    lower = (UINT4)ceil(dataPtr->fLow / deltaF);
    upper = (UINT4)floor(dataPtr->fHigh / deltaF);
    TwoDeltaToverN = 2.0 * deltaT / ((double) dataPtr->timeData->data->length);

    /* Employ a trick here for avoiding cos(...) and sin(...) in time
       shifting.  We need to multiply each template frequency bin by
       exp(-J*twopit*deltaF*i) = exp(-J*twopit*deltaF*(i-1)) +
       exp(-J*twopit*deltaF*(i-1))*(exp(-J*twopit*deltaF) - 1) .  This
       recurrance relation has the advantage that the error growth is
       O(sqrt(N)) for N repetitions. */
    
    /* Values for the first iteration: */
    re = cos(twopit*deltaF*lower);
    im = -sin(twopit*deltaF*lower);

    /* Incremental values, using cos(theta) - 1 = -2*sin(theta/2)^2 */
    dim = -sin(twopit*deltaF);
    dre = -2.0*sin(0.5*twopit*deltaF)*sin(0.5*twopit*deltaF);

    //Set up noise PSD meta parameters
    for(i=0; i<Nblock; i++)
    {
      if(psdFlag)
      {
        alpha[i]   = gsl_matrix_get(nparams,ifo,i);
        lnalpha[i] = log(alpha[i]);

        psdBandsMin_array[i] = gsl_matrix_get(psdBandsMin,ifo,i);
        psdBandsMax_array[i] = gsl_matrix_get(psdBandsMax,ifo,i);

      }
      else
      {
        alpha[i]=1.0;
        lnalpha[i]=0.0;
      }
    }

    //Set up psd line arrays
    for(j=0;j<Nlines;j++)
    {
      if(lineFlag)
      {

        //find range of fourier fourier bins which are excluded from integration
        lines_array[j]  = (int)gsl_matrix_get(lines,ifo,j);//lineimin = (int)(gsl_matrix_get(lines,ifo,j) - gsl_matrix_get(widths,ifo,j));
        widths_array[j] = (int)gsl_matrix_get(widths,ifo,j);//lineimax = (int)(gsl_matrix_get(lines,ifo,j) + gsl_matrix_get(widths,ifo,j));
      }
      else
      {
        lines_array[j]=0;
        widths_array[j]=0;
      }
    }

    for (i=lower; i<=upper; ++i){
      /* derive template (involving location/orientation parameters) from given plus/cross waveforms: */
      plainTemplateReal = FplusScaled * creal(dataPtr->freqModelhPlus->data->data[i])  
                          +  FcrossScaled * creal(dataPtr->freqModelhCross->data->data[i]);
      plainTemplateImag = FplusScaled * cimag(dataPtr->freqModelhPlus->data->data[i])  
                          +  FcrossScaled * cimag(dataPtr->freqModelhCross->data->data[i]);

      /* do time-shifting...             */
      /* (also un-do 1/deltaT scaling): */
      f = ((double) i) * deltaF;
      /* real & imag parts of  exp(-2*pi*i*f*deltaT): */
      re = cos(twopit * f);
      im = - sin(twopit * f);
      templateReal = (plainTemplateReal*re - plainTemplateImag*im) / deltaT;
      templateImag = (plainTemplateReal*im + plainTemplateImag*re) / deltaT;
      dataReal     = creal(dataPtr->freqData->data->data[i]) / deltaT;
      dataImag     = cimag(dataPtr->freqData->data->data[i]) / deltaT;
      /* compute squared difference & 'chi-squared': */
      diffRe       = dataReal - templateReal;         // Difference in real parts...
      diffIm       = dataImag - templateImag;         // ...and imaginary parts, and...
      diffSquared  = diffRe*diffRe + diffIm*diffIm ;  // ...squared difference of the 2 complex figures.
      REAL8 temp = ((TwoDeltaToverN * diffSquared) / dataPtr->oneSidedNoisePowerSpectrum->data->data[i]);
      chisquared  += temp;
      dataPtr->loglikelihood -= temp;
 //fprintf(testout, "%e %e %e %e %e %e\n",
 //        f, dataPtr->oneSidedNoisePowerSpectrum->data->data[i], 
 //        dataPtr->freqData->data->data[i].re, dataPtr->freqData->data->data[i].im,
 //        templateReal, templateImag);
      /* Add noise PSD parameters to the model */
      if(psdFlag)
      {
        for(j=0; j<Nblock; j++)
        {
            if (i >= psdBandsMin_array[j] && i <= psdBandsMax_array[j])
            {
                temp  /= alpha[j];
                temp  += lnalpha[j];
            }
        }
      }

      /* Remove lines from model */
      lineSwitch=1;
      
      if(lineFlag)
      {
        for(j=0;j<Nlines;j++)
        {
          //find range of fourier fourier bins which are excluded from integration
          lineimin = lines_array[j] - widths_array[j];//gsl_matrix_get(widths,ifo,j));
          lineimax = lines_array[j] + widths_array[j];//(int)(gsl_matrix_get(lines,ifo,j) + gsl_matrix_get(widths,ifo,j));

          //if the current bin is inside the exluded region, set the switch to 0
          if(i>lineimin && i<lineimax) lineSwitch=0;
        }
      }

      /*only sum over bins which are outside of excluded regions */
      if(lineSwitch)
      {
        chisquared  += temp;
        dataPtr->loglikelihood -= temp;
      }
 
      /* Now update re and im for the next iteration. */
      newRe = re + re*dre - im*dim;
      newIm = im + re*dim + im*dre;

      re = newRe;
      im = newIm;

    }
    ifo++; //increment IFO counter for noise parameters
    dataPtr = dataPtr->next;
 //fclose(testout);
  }
  loglikeli = -1.0 * chisquared; // note (again): the log-likelihood is unnormalised!
  LALInferenceClearVariables(&intrinsicParams);
  printf("%10.10e\n",loglikeli);
  return(loglikeli);
}

/***************************************************************/
/* Student-t (log-) likelihood function                        */
/* as described in Roever/Meyer/Christensen (2011):            */
/*   "Modelling coloured residual noise                        */
/*   in gravitational-wave signal processing."                 */
/*   Classical and Quantum Gravity, 28(1):015010.              */
/*   http://dx.doi.org/10.1088/0264-9381/28/1/015010           */
/*   http://arxiv.org/abs/0804.3853                            */
/* Returns the non-normalised logarithmic likelihood.          */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Required (`currentParams') parameters are:                  */
/*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
/*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
/*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
/*   - "distance"        (REAL8, Mpc, > 0)                     */
/*   - "time"            (REAL8, GPS sec.)                     */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* This function is essentially the same as the                */
/* "UndecomposedFreqDomainLogLikelihood()" function.           */
/* The additional parameter to be supplied is the (REAL8)      */
/* degrees-of-freedom parameter (nu) for each Ifo.             */
/* The additional "df" argument gives the corresponding        */
/* d.f. parameter for each element of the "*data" list.        */
/* The names of "df" must match the "->name" slot of           */
/* the elements of "data".                                     */
/*                                                             */
/* (TODO: allow for d.f. parameter to vary with frequency,     */
/*        i.e., to be a set of vectors corresponding to        */
/*        frequencies)                                         */
/***************************************************************/

REAL8 LALInferenceFreqDomainStudentTLogLikelihood(LALInferenceVariables *currentParams, LALInferenceIFOData *data, 
                                      LALInferenceTemplateFunction templt)
{
  //static int timeDomainWarning = 0;
  double Fplus, Fcross;
  double FplusScaled, FcrossScaled;
  double diffRe, diffIm, diffSquared;
  double dataReal, dataImag;
  REAL8 loglikeli;
  REAL8 plainTemplateReal, plainTemplateImag;
  REAL8 templateReal, templateImag;
  int i, lower, upper;
  LALInferenceIFOData *dataPtr;
  double ra, dec, psi, distMpc, gmst,mc;
  double GPSdouble;
  LIGOTimeGPS GPSlal;
  double chisquared;
  double timedelay;  /* time delay b/w iterferometer & geocenter w.r.t. sky location */
  double timeshift;  /* time shift (not necessarily same as above)                   */
  double deltaT, FourDeltaToverN, deltaF, twopit, f, re, im, singleFreqBinTerm;
  double degreesOfFreedom, nu;
  double timeTmp;
  int different;
  LALStatus status;
  memset(&status,0,sizeof(status));
  LALInferenceVariables intrinsicParams;
  
  /* Fill in derived parameters if necessary */
  if(LALInferenceCheckVariable(currentParams,"logdistance")){
    distMpc=exp(*(REAL8 *) LALInferenceGetVariable(currentParams,"logdistance"));
    LALInferenceAddVariable(currentParams,"distance",&distMpc,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
  }

  if(LALInferenceCheckVariable(currentParams,"logmc")){
    mc=exp(*(REAL8 *)LALInferenceGetVariable(currentParams,"logmc"));
    LALInferenceAddVariable(currentParams,"chirpmass",&mc,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
  }
  /* determine source's sky location & orientation parameters: */
  ra        = *(REAL8*) LALInferenceGetVariable(currentParams, "rightascension"); /* radian      */
  dec       = *(REAL8*) LALInferenceGetVariable(currentParams, "declination");    /* radian      */
  psi       = *(REAL8*) LALInferenceGetVariable(currentParams, "polarisation");   /* radian      */
  GPSdouble = *(REAL8*) LALInferenceGetVariable(currentParams, "time");           /* GPS seconds */
  distMpc   = *(REAL8*) LALInferenceGetVariable(currentParams, "distance");       /* Mpc         */

  /* figure out GMST: */
  /* XLALINT8NSToGPS(&GPSlal, floor(1e9 * GPSdouble + 0.5)); */
  XLALGPSSetREAL8(&GPSlal, GPSdouble);
  /* UandA.units    = MST_RAD;                       */
  /* UandA.accuracy = LALLEAPSEC_LOOSE;              */
  /* LALGPStoGMST1(&status, &gmst, &GPSlal, &UandA); */
  gmst=XLALGreenwichMeanSiderealTime(&GPSlal);
  intrinsicParams.head      = NULL;
  intrinsicParams.dimension = 0;
  LALInferenceCopyVariables(currentParams, &intrinsicParams);
  LALInferenceRemoveVariable(&intrinsicParams, "rightascension");
  LALInferenceRemoveVariable(&intrinsicParams, "declination");
  LALInferenceRemoveVariable(&intrinsicParams, "polarisation");
  LALInferenceRemoveVariable(&intrinsicParams, "time");
  LALInferenceRemoveVariable(&intrinsicParams, "distance");
  /*  TODO: add pointer to template function here.                                         */
  /*  (otherwise same parameters but different template will lead to no re-computation!!)  */

  chisquared = 0.0;
  /* loop over data (different interferometers): */
  dataPtr = data;

  while (dataPtr != NULL) {
    /* The parameters the Likelihood function can handle by itself    */
    /* (and which shouldn't affect the template function) are         */
    /* sky location (ra, dec), polarisation and signal arrival time.  */
    /* Note that the template function shifts the waveform to so that */
    /* t_c corresponds to the "time" parameter in                     */
    /* IFOdata->modelParams (set, e.g., from the trigger value).      */
    
    /* Reset log-likelihood */
    dataPtr->loglikelihood = 0.0;

    /* Compare parameter values with parameter values corresponding */
    /* to currently stored template; ignore "time" variable:        */
    if (LALInferenceCheckVariable(dataPtr->modelParams, "time")) {
      timeTmp = *(REAL8 *) LALInferenceGetVariable(dataPtr->modelParams, "time");
      LALInferenceRemoveVariable(dataPtr->modelParams, "time");
    }
    else timeTmp = GPSdouble;
    different = LALInferenceCompareVariables(dataPtr->modelParams, &intrinsicParams);
    /* "different" now may also mean that "dataPtr->modelParams" */
    /* wasn't allocated yet (as in the very 1st iteration).      */

    if (different) { /* template needs to be re-computed: */
      LALInferenceCopyVariables(&intrinsicParams, dataPtr->modelParams);
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
      templt(dataPtr);

      if (dataPtr->modelDomain == LAL_SIM_DOMAIN_TIME) {
	/* TD --> FD. */
	LALInferenceExecuteFT(dataPtr);
      }
    }
    else { /* no re-computation necessary. Return back "time" value, do nothing else: */
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
    }

    /* Template is now in dataPtr->freqModelhPlus hCross. */

    /* determine beam pattern response (F_plus and F_cross) for given Ifo: */
    XLALComputeDetAMResponse(&Fplus, &Fcross,
                             dataPtr->detector->response,
			     ra, dec, psi, gmst);
    /* signal arrival time (relative to geocenter); */
    timedelay = XLALTimeDelayFromEarthCenter(dataPtr->detector->location,
                                             ra, dec, &GPSlal);
    /* (negative timedelay means signal arrives earlier at Ifo than at geocenter, etc.)    */
    /* amount by which to time-shift template (not necessarily same as above "timedelay"): */
    timeshift =  (GPSdouble - (*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time"))) + timedelay;

    twopit    = LAL_TWOPI * timeshift;

    /* include distance (overall amplitude) effect in Fplus/Fcross: */
    FplusScaled  = Fplus  / distMpc;
    FcrossScaled = Fcross / distMpc;

    if (LALInferenceCheckVariable(currentParams, "crazyInjectionHLSign") &&
        *((INT4 *)LALInferenceGetVariable(currentParams, "crazyInjectionHLSign"))) {
      if (strstr(dataPtr->name, "H") || strstr(dataPtr->name, "L")) {
        FplusScaled *= -1.0;
        FcrossScaled *= -1.0;
      }
    }

    dataPtr->fPlus = FplusScaled;
    dataPtr->fCross = FcrossScaled;
    dataPtr->timeshift = timeshift;

    /* extract the element from the "df" vector that carries the current Ifo's name: */
    CHAR df_variable_name[64];
    sprintf(df_variable_name,"df_%s",dataPtr->name);
    if(LALInferenceCheckVariable(currentParams,df_variable_name)){
      degreesOfFreedom = *(REAL8*) LALInferenceGetVariable(currentParams,df_variable_name);
    }
    else {
      degreesOfFreedom = dataPtr->STDOF;
    }
    if (!(degreesOfFreedom>0)) {
      XLALPrintError(" ERROR in StudentTLogLikelihood(): degrees-of-freedom parameter must be positive.\n");
      XLAL_ERROR_REAL8(XLAL_EDOM);
    }

    /* determine frequency range & loop over frequency bins: */
    deltaT = dataPtr->timeData->deltaT;
    deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);
    lower = (UINT4)ceil(dataPtr->fLow / deltaF);
    upper = (UINT4)floor(dataPtr->fHigh / deltaF);
    FourDeltaToverN = 4.0 * deltaT / ((double) dataPtr->timeData->data->length);
    for (i=lower; i<=upper; ++i){
      /* degrees-of-freedom parameter (nu_j) for this particular frequency bin: */
      nu = degreesOfFreedom;
      /* (for now constant across frequencies)                                  */
      /* derive template (involving location/orientation parameters) from given plus/cross waveforms: */
      plainTemplateReal = FplusScaled * creal(dataPtr->freqModelhPlus->data->data[i])  
                          +  FcrossScaled * creal(dataPtr->freqModelhCross->data->data[i]);
      plainTemplateImag = FplusScaled * cimag(dataPtr->freqModelhPlus->data->data[i])  
                          +  FcrossScaled * cimag(dataPtr->freqModelhCross->data->data[i]);

      /* do time-shifting...            */
      /* (also un-do 1/deltaT scaling): */
      f = ((double) i) * deltaF;
      /* real & imag parts of  exp(-2*pi*i*f*deltaT): */
      re = cos(twopit * f);
      im = - sin(twopit * f);
      templateReal = (plainTemplateReal*re - plainTemplateImag*im) / deltaT;
      templateImag = (plainTemplateReal*im + plainTemplateImag*re) / deltaT;
      dataReal     = creal(dataPtr->freqData->data->data[i]) / deltaT;
      dataImag     = cimag(dataPtr->freqData->data->data[i]) / deltaT;
      /* compute squared difference & 'chi-squared': */
      diffRe       = dataReal - templateReal;         /* Difference in real parts...                     */
      diffIm       = dataImag - templateImag;         /* ...and imaginary parts, and...                  */
      diffSquared  = diffRe*diffRe + diffIm*diffIm ;  /* ...squared difference of the 2 complex figures. */
      singleFreqBinTerm = ((nu+2.0)/2.0) * log(1.0 + (FourDeltaToverN * diffSquared) / (nu * dataPtr->oneSidedNoisePowerSpectrum->data->data[i]));
      chisquared  += singleFreqBinTerm;   /* (This is a sum-of-squares, or chi^2, term in the Gaussian case, not so much in the Student-t case...)  */
      dataPtr->loglikelihood -= singleFreqBinTerm;
    }
    dataPtr = dataPtr->next;
  }
  loglikeli = -1.0 * chisquared; /* note (again): the log-likelihood is unnormalised! */
  LALInferenceClearVariables(&intrinsicParams);  
  return(loglikeli);
}



REAL8 LALInferenceFreqDomainLogLikelihood(LALInferenceVariables *currentParams, LALInferenceIFOData * data, 
                              LALInferenceTemplateFunction templt)
/***************************************************************/
/* (log-) likelihood function.                                 */
/* Returns the non-normalised logarithmic likelihood.          */
/* Slightly slower but cleaner than							   */
/* UndecomposedFreqDomainLogLikelihood().          `		   */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Required (`currentParams') parameters are:                  */
/*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
/*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
/*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
/*   - "distance"        (REAL8, Mpc, >0)                      */
/*   - "time"            (REAL8, GPS sec.)                     */
/***************************************************************/
{
  REAL8 loglikeli, totalChiSquared=0.0;
  LALInferenceIFOData *ifoPtr=data;
  COMPLEX16Vector *freqModelResponse=NULL;

  /* loop over data (different interferometers): */
  while (ifoPtr != NULL) {
    ifoPtr->loglikelihood = 0.0;

	if(freqModelResponse==NULL)
		freqModelResponse= XLALCreateCOMPLEX16Vector(ifoPtr->freqData->data->length);
	else
		freqModelResponse= XLALResizeCOMPLEX16Vector(freqModelResponse, ifoPtr->freqData->data->length);
	/*compute the response*/
	LALInferenceComputeFreqDomainResponse(currentParams, ifoPtr, templt, freqModelResponse);
	/*if(residual==NULL)
		residual=XLALCreateCOMPLEX16Vector(ifoPtr->freqData->data->length);
	else
		residual=XLALResizeCOMPLEX16Vector(residual, ifoPtr->freqData->data->length);
	
	COMPLEX16VectorSubtract(residual, ifoPtr->freqData->data, freqModelResponse);
	totalChiSquared+=ComputeFrequencyDomainOverlap(ifoPtr, residual, residual); 
	*/
        REAL8 temp = LALInferenceComputeFrequencyDomainOverlap(ifoPtr, ifoPtr->freqData->data, ifoPtr->freqData->data)
          -2.0*LALInferenceComputeFrequencyDomainOverlap(ifoPtr, ifoPtr->freqData->data, freqModelResponse)
          +LALInferenceComputeFrequencyDomainOverlap(ifoPtr, freqModelResponse, freqModelResponse);
	totalChiSquared+=temp;
        ifoPtr->loglikelihood -= 0.5*temp;

    ifoPtr = ifoPtr->next;
  }
  loglikeli = -0.5 * totalChiSquared; // note (again): the log-likelihood is unnormalised!
  XLALDestroyCOMPLEX16Vector(freqModelResponse);
  return(loglikeli);
}

REAL8 LALInferenceChiSquareTest(LALInferenceVariables *currentParams, LALInferenceIFOData * data, LALInferenceTemplateFunction templt)
/***************************************************************/
/* Chi-Square function.                                        */
/* Returns the chi square of a template:                       */
/* chisq= p * sum_i (dx_i)^2, with dx_i  =  <s,h>_i  - <s,h>/p */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Required (`currentParams') parameters are:                  */
/*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
/*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
/*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
/*   - "distance"        (REAL8, Mpc, >0)                      */
/*   - "time"            (REAL8, GPS sec.)                     */
/***************************************************************/
{
  REAL8 ChiSquared=0.0, dxp, xp, x, norm, binPower, nextBin;
  REAL8 lowerF, upperF, deltaT, deltaF;
  REAL8 *segnorm;
  INT4  i, chisqPt, imax,  kmax, numBins=0;//kmin - set but not used
  INT4  *chisqBin;
  LALInferenceIFOData *ifoPtr=data;
  COMPLEX16Vector *freqModelResponse=NULL;
  //COMPLEX16Vector *segmentFreqModelResponse-NULL;

  /* Allocate memory for local pointers */
  segnorm=XLALMalloc(sizeof(REAL8) * ifoPtr->freqData->data->length);
  chisqBin=XLALMalloc(sizeof(INT4) * (numBins + 1));

  /* loop over data (different interferometers): */
  while (ifoPtr != NULL) {
    if(freqModelResponse==NULL)
      freqModelResponse= XLALCreateCOMPLEX16Vector(ifoPtr->freqData->data->length);
    else
      freqModelResponse= XLALResizeCOMPLEX16Vector(freqModelResponse, ifoPtr->freqData->data->length);
    /*compute the response*/
    LALInferenceComputeFreqDomainResponse(currentParams, ifoPtr, templt, freqModelResponse);

    deltaT = ifoPtr->timeData->deltaT;
    deltaF = 1.0 / (((REAL8)ifoPtr->timeData->data->length) * deltaT);
    
    /* Store values of fLow and fHigh to use later */
    lowerF = ifoPtr->fLow;
    upperF = ifoPtr->fHigh;
   
    /* Generate bin boundaries */
    numBins = *(INT4*) LALInferenceGetVariable(currentParams, "numbins");
    //kmin = ceil(ifoPtr->fLow / deltaF); - set but not used
    kmax = floor(ifoPtr->fHigh / deltaF);
    imax = kmax > (INT4) ifoPtr->freqData->data->length-1 ? (INT4) ifoPtr->freqData->data->length-1 : kmax;
    
    memset(segnorm,0,sizeof(REAL8) * ifoPtr->freqData->data->length);
    norm = 0.0;
    
    for (i=1; i < imax; ++i){  	  	  
      norm += ((4.0 * deltaF * (creal(freqModelResponse->data[i])*creal(freqModelResponse->data[i])
              +cimag(freqModelResponse->data[i])*cimag(freqModelResponse->data[i]))) 
              / ifoPtr->oneSidedNoisePowerSpectrum->data->data[i]);
      segnorm[i] = norm;
    }


    memset(chisqBin,0,sizeof(INT4) * (numBins +1));

    binPower = norm / (REAL8) numBins;
    nextBin   = binPower;
    chisqPt   = 0;
    chisqBin[chisqPt++] = 0;

    for ( i = 1; i < imax; ++i )
    {
      if ( segnorm[i] >= nextBin )
      {
        chisqBin[chisqPt++] = i;
        nextBin += binPower;
        if ( chisqPt == numBins ) break;
      }
    }
    chisqBin[16]=imax;
    /* check that we have sucessfully allocated all the bins */
    if ( i == (INT4) ifoPtr->freqData->data->length && chisqPt != numBins )
    {
      /* if we have reaced the end of the template power vec and not
       * */
      /* allocated all the bin boundaries then there is a problem
       * */
      fprintf(stderr,"Error constructing frequency bins\n"); 
    }

    /* the last bin boundary is at can be at Nyquist since   */
    /* qtilde is zero above the ISCO of the current template */
    // chisqBin[numBins] = ifoPtr->freqData->data->length;

    /* end */
    
    x = LALInferenceComputeFrequencyDomainOverlap(ifoPtr, ifoPtr->freqData->data, freqModelResponse)/(sqrt(norm));
    
    ChiSquared=0.0;
    
    for (i=0; i < numBins; ++i){
      
      ifoPtr->fLow = chisqBin[i] * deltaF;
      ifoPtr->fHigh = chisqBin[i+1] * deltaF;
      
      xp = LALInferenceComputeFrequencyDomainOverlap(ifoPtr, ifoPtr->freqData->data, freqModelResponse)/(sqrt(norm));
      dxp = ((REAL8) numBins) * xp - x;
      ChiSquared += (dxp * dxp);
      
    }
    ChiSquared = ChiSquared / (REAL8) numBins;
    ifoPtr->fLow = lowerF;
    ifoPtr->fHigh = upperF;
    
    printf("Chi-Square for %s\t=\t%f\n",ifoPtr->detector->frDetector.name,ChiSquared);
    
    ifoPtr = ifoPtr->next;
  }
  XLALFree(chisqBin);
  XLALFree(segnorm);
  return(ChiSquared);
}

//REAL8 LALInferenceTimeDomainLogLikelihood(LALInferenceVariables *currentParams, LALInferenceIFOData * data, 
//                              LALInferenceTemplateFunction templt)
///***************************************************************/
///* Time domain (log-) likelihood function.                     */
///* Returns the non-normalised logarithmic likelihood.          */
///* Mathematically equivalent to Frequency domain likelihood.   */
///* Time domain version of LALInferenceFreqDomainLogLikelihood()*/
///* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
///* Required (`currentParams') parameters are:                  */
///*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
///*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
///*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
///*   - "distance"        (REAL8, Mpc, >0)                      */
///*   - "time"            (REAL8, GPS sec.)                     */
///***************************************************************/
//{
//  REAL8 loglikeli, totalChiSquared=0.0;
//  LALInferenceIFOData *ifoPtr=data;
//  REAL8TimeSeries *timeModelResponse=NULL;
//
//  /* loop over data (different interferometers): */
//  while (ifoPtr != NULL) {
//    ifoPtr->loglikelihood = 0.0;
//
//    if(timeModelResponse==NULL) {
//      timeModelResponse = 
//	XLALCreateREAL8TimeSeries("time detector response", &(ifoPtr->timeData->epoch), 
//				  0.0, ifoPtr->timeData->deltaT,
//				  &lalDimensionlessUnit,
//				  ifoPtr->timeData->data->length);
//    } else if (timeModelResponse->data->length != ifoPtr->timeData->data->length) {
//      /* Cannot resize *up* a time series, so just dealloc and XLALReallocate it. */
//      XLALDestroyREAL8TimeSeries(timeModelResponse);
//      timeModelResponse =                   
//	XLALCreateREAL8TimeSeries("time detector response", &(ifoPtr->timeData->epoch), 
//				  0.0, ifoPtr->timeData->deltaT,
//				  &lalDimensionlessUnit,
//				  ifoPtr->timeData->data->length);
//    }
//     
//    /*compute the response*/
//    LALInferenceComputeTimeDomainResponse(currentParams, ifoPtr, template, timeModelResponse);
//    REAL8 temp = (LALInferenceWhitenedTimeDomainOverlap(ifoPtr->whiteTimeData, ifoPtr->windowedTimeData)
//                  -2.0*LALInferenceWhitenedTimeDomainOverlap(ifoPtr->whiteTimeData, timeModelResponse)
//                  +LALInferenceTimeDomainOverlap(ifoPtr->timeDomainNoiseWeights, timeModelResponse, timeModelResponse));
//    totalChiSquared+=temp;
//    ifoPtr->loglikelihood -= 0.5*temp;
//    
//    ifoPtr = ifoPtr->next;
//  }
//  loglikeli = -0.5*totalChiSquared; 
//  XLALDestroyREAL8TimeSeries(timeModelResponse);
//  return(loglikeli);
//}

void LALInferenceComputeFreqDomainResponse(LALInferenceVariables *currentParams, LALInferenceIFOData * dataPtr, 
                              LALInferenceTemplateFunction templt, COMPLEX16Vector *freqWaveform)
/***************************************************************/
/* Frequency-domain single-IFO response computation.           */
/* Computes response for a given template.                     */
/* Will re-compute template only if necessary                  */
/* (i.e., if previous, as stored in data->freqModelhCross,     */
/* was based on different parameters or template function).    */
/* Carries out timeshifting for a given detector               */
/* and projection onto this detector.                          */
/* Result stored in freqResponse, assumed to be correctly      */
/* initialized												   */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Required (`currentParams') parameters are:                  */
/*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
/*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
/*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
/*   - "distance"        (REAL8, Mpc, >0)                      */
/*   - "time"            (REAL8, GPS sec.)                     */
/***************************************************************/							  
{
  //static int timeDomainWarning = 0;

	double ra, dec, psi, distMpc, gmst;
	
	double GPSdouble;
	double timeTmp;
	LIGOTimeGPS GPSlal;
	double timedelay;  /* time delay b/w iterferometer & geocenter w.r.t. sky location */
	double timeshift;  /* time shift (not necessarily same as above)                   */
	double deltaT, deltaF, twopit, f, re, im;

	int different;
	LALInferenceVariables intrinsicParams;
	LALStatus status;
	memset(&status,0,sizeof(status));
	
	double Fplus, Fcross;
	double FplusScaled, FcrossScaled;
	REAL8 plainTemplateReal, plainTemplateImag;
	UINT4 i;
	REAL8 mc;
	/* Fill in derived parameters if necessary */
	if(LALInferenceCheckVariable(currentParams,"logdistance")){
		distMpc=exp(*(REAL8 *) LALInferenceGetVariable(currentParams,"logdistance"));
		LALInferenceAddVariable(currentParams,"distance",&distMpc,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
	}

	if(LALInferenceCheckVariable(currentParams,"logmc")){
		mc=exp(*(REAL8 *)LALInferenceGetVariable(currentParams,"logmc"));
		LALInferenceAddVariable(currentParams,"chirpmass",&mc,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
	}
		
	
	/* determine source's sky location & orientation parameters: */
	ra        = *(REAL8*) LALInferenceGetVariable(currentParams, "rightascension"); /* radian      */
	dec       = *(REAL8*) LALInferenceGetVariable(currentParams, "declination");    /* radian      */
	psi       = *(REAL8*) LALInferenceGetVariable(currentParams, "polarisation");   /* radian      */
	GPSdouble = *(REAL8*) LALInferenceGetVariable(currentParams, "time");           /* GPS seconds */
	distMpc   = *(REAL8*) LALInferenceGetVariable(currentParams, "distance");       /* Mpc         */

		
	/* figure out GMST: */
	//XLALINT8NSToGPS(&GPSlal, floor(1e9 * GPSdouble + 0.5));
	XLALGPSSetREAL8(&GPSlal, GPSdouble);
	//UandA.units    = MST_RAD;
	//UandA.accuracy = LALLEAPSEC_LOOSE;
	//LALGPStoGMST1(&status, &gmst, &GPSlal, &UandA);
	gmst=XLALGreenwichMeanSiderealTime(&GPSlal);
	intrinsicParams.head      = NULL;
	intrinsicParams.dimension = 0;
	LALInferenceCopyVariables(currentParams, &intrinsicParams);
	LALInferenceRemoveVariable(&intrinsicParams, "rightascension");
	LALInferenceRemoveVariable(&intrinsicParams, "declination");
	LALInferenceRemoveVariable(&intrinsicParams, "polarisation");
	LALInferenceRemoveVariable(&intrinsicParams, "time");
	LALInferenceRemoveVariable(&intrinsicParams, "distance");


	// TODO: add pointer to template function here.
	// (otherwise same parameters but different template will lead to no re-computation!!)
      
	/* The parameters the response function can handle by itself     */
    /* (and which shouldn't affect the template function) are        */
    /* sky location (ra, dec), polarisation and signal arrival time. */
    /* Note that the template function shifts the waveform to so that*/
	/* t_c corresponds to the "time" parameter in                    */
	/* IFOdata->modelParams (set, e.g., from the trigger value).     */
    
    /* Compare parameter values with parameter values corresponding  */
    /* to currently stored template; ignore "time" variable:         */
    if (LALInferenceCheckVariable(dataPtr->modelParams, "time")) {
      timeTmp = *(REAL8 *) LALInferenceGetVariable(dataPtr->modelParams, "time");
      LALInferenceRemoveVariable(dataPtr->modelParams, "time");
    }
    else timeTmp = GPSdouble;
    different = LALInferenceCompareVariables(dataPtr->modelParams, &intrinsicParams);
    /* "different" now may also mean that "dataPtr->modelParams" */
    /* wasn't allocated yet (as in the very 1st iteration).      */

    if (different) { /* template needs to be re-computed: */
      LALInferenceCopyVariables(&intrinsicParams, dataPtr->modelParams);
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
      templt(dataPtr);

      if (dataPtr->modelDomain == LAL_SIM_DOMAIN_TIME) {
	/* TD --> FD. */
	LALInferenceExecuteFT(dataPtr);
      }
    }
    else { /* no re-computation necessary. Return back "time" value, do nothing else: */
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
    }
    /* Template is now in dataPtr->freqModelhPlus and
       dataPtr->freqModelhCross */

    /* determine beam pattern response (F_plus and F_cross) for given Ifo: */
    XLALComputeDetAMResponse(&Fplus, &Fcross, dataPtr->detector->response,
			     ra, dec, psi, gmst);
		 
    /* signal arrival time (relative to geocenter); */
    timedelay = XLALTimeDelayFromEarthCenter(dataPtr->detector->location,
                                             ra, dec, &GPSlal);
    /* (negative timedelay means signal arrives earlier at Ifo than at geocenter, etc.) */

    /* amount by which to time-shift template (not necessarily same as above "timedelay"): */
    timeshift =  (GPSdouble - (*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time"))) + timedelay;

    twopit    = LAL_TWOPI * timeshift;

    /* include distance (overall amplitude) effect in Fplus/Fcross: */
    FplusScaled  = Fplus  / distMpc;
    FcrossScaled = Fcross / distMpc;
    
    if (LALInferenceCheckVariable(currentParams, "crazyInjectionHLSign") &&
        *((INT4 *)LALInferenceGetVariable(currentParams, "crazyInjectionHLSign"))) {
      if (strstr(dataPtr->name, "H") || strstr(dataPtr->name, "L")) {
        FplusScaled *= -1.0;
        FcrossScaled *= -1.0;
      }
    }

	if(freqWaveform->length!=dataPtr->freqModelhPlus->data->length){
		printf("fW%d data%d\n", freqWaveform->length, dataPtr->freqModelhPlus->data->length);
		printf("Error!  Frequency data vector must be same length as original data!\n");
		exit(1);
	}
	
	deltaT = dataPtr->timeData->deltaT;
    deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);

#ifdef DEBUG
FILE* file=fopen("TempSignal.dat", "w");	
#endif
	for(i=0; i<freqWaveform->length; i++){
		/* derive template (involving location/orientation parameters) from given plus/cross waveforms: */
		plainTemplateReal = FplusScaled * creal(dataPtr->freqModelhPlus->data->data[i])  
                          +  FcrossScaled * creal(dataPtr->freqModelhCross->data->data[i]);
		plainTemplateImag = FplusScaled * cimag(dataPtr->freqModelhPlus->data->data[i])  
                          +  FcrossScaled * cimag(dataPtr->freqModelhCross->data->data[i]);

		/* do time-shifting...             */
		/* (also un-do 1/deltaT scaling): */
		f = ((double) i) * deltaF;
		/* real & imag parts of  exp(-2*pi*i*f*deltaT): */
		re = cos(twopit * f);
		im = - sin(twopit * f);

		freqWaveform->data[i]=crect((plainTemplateReal*re - plainTemplateImag*im),(plainTemplateReal*im + plainTemplateImag*re));		

#ifdef DEBUG
		fprintf(file, "%lg %lg \t %lg\n", f, freqWaveform->data[i].re, freqWaveform->data[i].im);
#endif
	}
#ifdef DEBUG
fclose(file);
#endif
	LALInferenceClearVariables(&intrinsicParams);
}

REAL8 LALInferenceComputeFrequencyDomainOverlap(LALInferenceIFOData * dataPtr,
                                    COMPLEX16Vector * freqData1, 
                                    COMPLEX16Vector * freqData2)
{
  if (dataPtr==NULL || freqData1 ==NULL || freqData2==NULL){
  	XLAL_ERROR_REAL8(XLAL_EFAULT); 
  	}
  	
  int lower, upper, i;
  double deltaT, deltaF;
  
  double overlap=0.0;
  
  /* determine frequency range & loop over frequency bins: */
  deltaT = dataPtr->timeData->deltaT;
  deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);
  lower = ceil(dataPtr->fLow / deltaF);
  upper = floor(dataPtr->fHigh / deltaF);
	
  for (i=lower; i<=upper; ++i){  	  	  
    overlap  += ((4.0*deltaF*(creal(freqData1->data[i])*creal(freqData2->data[i])+cimag(freqData1->data[i])*cimag(freqData2->data[i]))) 
                 / dataPtr->oneSidedNoisePowerSpectrum->data->data[i]);
  }

  return overlap;
}

REAL8 LALInferenceNullLogLikelihood(LALInferenceIFOData *data)
/*Identical to FreqDomainNullLogLikelihood                        */
{
	REAL8 loglikeli, totalChiSquared=0.0;
	LALInferenceIFOData *ifoPtr=data;
	
	/* loop over data (different interferometers): */
	while (ifoPtr != NULL) {
          ifoPtr->nullloglikelihood = 0.0;
          REAL8 temp = LALInferenceComputeFrequencyDomainOverlap(ifoPtr, ifoPtr->freqData->data, ifoPtr->freqData->data);
          totalChiSquared+=temp;
          ifoPtr->nullloglikelihood -= 0.5*temp;
		ifoPtr = ifoPtr->next;
	}
	loglikeli = -0.5 * totalChiSquared; // note (again): the log-likelihood is unnormalised!
	return(loglikeli);
}

REAL8 LALInferenceNullLogLikelihoodBestIFO(LALInferenceIFOData *data)
/*Identical to FreqDomainNullLogLikelihood                        */
{
	REAL8 loglikeli, totalChiSquared=0.0;
	LALInferenceIFOData *ifoPtr=data;
	
	/* loop over data (different interferometers): */
	while (ifoPtr != NULL && ifoPtr->skipIFO!=1) {
        printf("doing null logl\n");
          ifoPtr->nullloglikelihood = 0.0;
          REAL8 temp = LALInferenceComputeFrequencyDomainOverlap(ifoPtr, ifoPtr->freqData->data, ifoPtr->freqData->data);
          totalChiSquared+=temp;
          ifoPtr->nullloglikelihood -= 0.5*temp;
		ifoPtr = ifoPtr->next;
	}
	loglikeli = -0.5 * totalChiSquared; // note (again): the log-likelihood is unnormalised!
	return(loglikeli);
}
//REAL8 LALInferenceWhitenedTimeDomainOverlap(const REAL8TimeSeries *whitenedData, const REAL8TimeSeries *data) {
//  return 2.0*LALInferenceIntegrateSeriesProduct(whitenedData, data);
//}

//REAL8 LALInferenceTimeDomainNullLogLikelihood(LALInferenceIFOData *data) {
//  REAL8 logL = 0.0;
//  LALInferenceIFOData *ifoPtr = data;
//  /* UINT4 ifoIndex = 0; */
//  /* UINT4 i; */
//  /* char fileName[256]; */
//  /* FILE *out; */
//  
//  while (ifoPtr != NULL) {
//    ifoPtr->nullloglikelihood = 0.0;
//    REAL8 temp = LALInferenceWhitenedTimeDomainOverlap(ifoPtr->whiteTimeData, ifoPtr->windowedTimeData);
//    logL += temp;
//    ifoPtr->nullloglikelihood -= 0.5*temp;
// 
//    ifoPtr = ifoPtr->next;
//    /* ifoIndex++; */
//  }
//
//  logL *= -0.5;
//  
//  return logL;
//}



//REAL8 LALInferenceIntegrateSeriesProduct(const REAL8TimeSeries *s1, const REAL8TimeSeries *s2) {
//  if (s1 == NULL || s2 == NULL)
//      XLAL_ERROR_REAL8(XLAL_EFAULT, "Null arguments received.");
//
//  LIGOTimeGPS start, stop;
//  LIGOTimeGPS stopS1, stopS2;
//  UINT4 i1, i2;
//  REAL8 sum = 0.0;
//  REAL8 t1Start, t2Start, t, tStop;
//
//  /* Compute stop times. */
//  stopS1 = s1->epoch;
//  stopS2 = s2->epoch;
//  XLALGPSAdd(&stopS1, (s1->data->length-1)*s1->deltaT);
//  XLALGPSAdd(&stopS2, (s2->data->length-1)*s2->deltaT);
//
//  /* The start time is the max of the two start times, the stop time
//     is the min of the two stop times */
//  start = (XLALGPSCmp(&(s1->epoch), &(s2->epoch)) <= 0 ? s2->epoch : s1->epoch); /* Start at max start time. */
//  stop = (XLALGPSCmp(&stopS1, &stopS2) <= 0 ? stopS1 : stopS2); /* Stop at min end time. */
//
//  t = 0;
//  tStop = XLALGPSDiff(&stop, &start);
//  t1Start = XLALGPSDiff(&(s1->epoch), &start);
//  t2Start = XLALGPSDiff(&(s2->epoch), &start);
//  i1 = LIGOTimeGPSToNearestIndex(&start, s1);
//  i2 = LIGOTimeGPSToNearestIndex(&start, s2);
//  
//  REAL8 *data1 = s1->data->data;
//  REAL8 *data2 = s2->data->data;
//
//  do {
//    REAL8 nextTime1, nextTime2, nextTime;
//    REAL8 dt;
//    nextTime1 = t1Start + (i1+0.5)*s1->deltaT;
//    nextTime2 = t2Start + (i2+0.5)*s2->deltaT;
//
//    /* Whichever series needs updating first gives us the next time. */
//    nextTime = (nextTime1 <= nextTime2 ? nextTime1 : nextTime2);
//
//    /* Ensure we don't go past the stop time. */
//    nextTime = (tStop < nextTime ? tStop : nextTime);
//
//    dt = nextTime - t;
//
//    sum += dt*data1[i1]*data2[i2];
//
//    if (nextTime1 == nextTime) {
//      i1++;
//    }
//    if (nextTime2 == nextTime) {
//      i2++;
//    }
//
//    t = nextTime;    
//  } while (t < tStop);
//
//  return sum;
//}

//void LALInferenceConvolveTimeSeries(REAL8TimeSeries *conv, const REAL8TimeSeries *data, const REAL8TimeSeries *response) {
//  if (conv == NULL || data == NULL || response == NULL)
//    XLAL_ERROR_VOID(XLAL_EFAULT, "Null arguments received.");
//
//  UINT4 responseSpan = (response->data->length + 1)/2;
//  UINT4 paddedLength = nextPowerOfTwo(data->data->length + responseSpan);
//  REAL8FFTPlan *fwdPlan = XLALCreateForwardREAL8FFTPlan(paddedLength, 1); /* Actually measure---rely on FFTW to store the best plan for a given length. */
//  REAL8FFTPlan *revPlan = XLALCreateReverseREAL8FFTPlan(paddedLength, 1); /* Same. */
//  REAL8Sequence *paddedData, *paddedResponse, *paddedConv;
//  COMPLEX16Sequence *dataFFT, *responseFFT;
//  UINT4 i;
//
//  if (data->deltaT != response->deltaT) {
//    fprintf(stderr, "convolveTimeSeries: sample spacings differ: %g vs %g (in %s, line %d)\n", 
//            data->deltaT, response->deltaT, __FILE__, __LINE__);
//    exit(1);
//  }
//
//  if (conv->data->length < data->data->length) {
//    fprintf(stderr, "convolveTimeSeries: output length smaller than input length (in %s, line %d)", __FILE__, __LINE__);
//    exit(1);
//  }
//
//  paddedData = XLALCreateREAL8Sequence(paddedLength);
//  paddedResponse = XLALCreateREAL8Sequence(paddedLength);
//  paddedConv = XLALCreateREAL8Sequence(paddedLength);
//
//  dataFFT = XLALCreateCOMPLEX16Sequence(paddedLength/2 + 1); /* Exploit R -> C symmetry. */
//  responseFFT = XLALCreateCOMPLEX16Sequence(paddedLength/2 + 1);
//
//  padREAL8Sequence(paddedData, data->data);
//  padWrappedREAL8Sequence(paddedResponse, response->data);
//
//  XLALREAL8ForwardFFT(dataFFT, paddedData, fwdPlan);
//  XLALREAL8ForwardFFT(responseFFT, paddedResponse, fwdPlan);
//
//  for (i = 0; i < paddedLength/2 + 1; i++) {
//    /* Store product in dataFFT. */
//    double dataRe, dataIm, resRe, resIm;
//    dataRe = dataFFT->data[i].re;
//    dataIm = dataFFT->data[i].im;
//    resRe = responseFFT->data[i].re;
//    resIm = responseFFT->data[i].im;
//
//    dataFFT->data[i].re = dataRe*resRe - dataIm*resIm;
//    dataFFT->data[i].im = dataRe*resIm + resRe*dataIm;
//  }
//
//  XLALREAL8ReverseFFT(paddedConv, dataFFT, revPlan);
//
//  memset(conv->data->data, 0, conv->data->length*sizeof(conv->data->data[0]));
//  for (i = 0; i < data->data->length; i++) {
//    conv->data->data[i] = conv->deltaT*paddedConv->data[i]/paddedConv->length; /* Normalize */
//  }
//
//  strncpy(conv->name, "convolved", LALNameLength);
//  conv->epoch = data->epoch;
//  conv->deltaT = data->deltaT;
//  conv->f0 = data->f0;
//  conv->sampleUnits = data->sampleUnits;  
//
//  XLALDestroyREAL8FFTPlan(fwdPlan);
//  XLALDestroyREAL8FFTPlan(revPlan);
//
//  XLALDestroyREAL8Sequence(paddedData);
//  XLALDestroyREAL8Sequence(paddedResponse);
//  XLALDestroyREAL8Sequence(paddedConv);
//
//  XLALDestroyCOMPLEX16Sequence(dataFFT);
//  XLALDestroyCOMPLEX16Sequence(responseFFT);
//}

//void LALInferenceWrappedTimeSeriesToLinearTimeSeries(REAL8TimeSeries *linear, const REAL8TimeSeries *wrapped) {
//  UINT4 NNeg, NPos, N, i;
//
//  if (linear->data->length != wrapped->data->length) {
//    fprintf(stderr, "wrappedTimeSeriesToLinearTimeSeries: lengths differ (in %s, line %d)",
//            __FILE__, __LINE__);
//    exit(1);
//  }
//
//  N = linear->data->length;
//  NNeg = (N-1)/2;
//  NPos = N-NNeg-1; /* 1 for the zero component. */
//
//  for (i = 0; i < NNeg; i++) {
//    linear->data->data[i] = wrapped->data->data[N-i-1];
//  }
//  linear->data->data[NNeg] = wrapped->data->data[0];
//  for (i = 1; i <= NPos; i++) {
//    linear->data->data[NNeg+i] = wrapped->data->data[i];
//  }
//
//  linear->epoch = wrapped->epoch;
//  linear->deltaT = wrapped->deltaT;
//  linear->f0 = wrapped->f0;
//  linear->sampleUnits = wrapped->sampleUnits;
//  
//  /* Adjust start time for linear to account for negative components. */
//  XLALGPSAdd(&linear->epoch, -(NNeg*linear->deltaT));
//}

//void LALInferenceLinearTimeSeriesToWrappedTimeSeries(REAL8TimeSeries *wrapped, const REAL8TimeSeries *linear) {
//  UINT4 NNeg, NPos, N, i;
//
//  if (wrapped->data->length != linear->data->length) {
//    fprintf(stderr, "linearTimeSeriesToWrappedTimeSeries: lengths differ (in %s, line %d)",
//            __FILE__, __LINE__);
//    exit(1);
//  }
//
//  N = wrapped->data->length;
//  NNeg = (N-1)/2;
//  NPos = N-NNeg-1;
//
//  wrapped->data->data[0] = linear->data->data[NNeg];
//  for (i = 1; i <= NPos; i++) {
//    wrapped->data->data[i] = linear->data->data[NNeg+i];
//  }
//  for (i = 0; i < NNeg; i++) {
//    wrapped->data->data[N-i-1] = linear->data->data[i];
//  }
//
//  wrapped->epoch = linear->epoch;
//  wrapped->deltaT = linear->deltaT;
//  wrapped->f0 = linear->f0;
//  wrapped->sampleUnits = linear->sampleUnits;
//  
//  /* Adjust start time. */
//  XLALGPSAdd(&wrapped->epoch, NNeg*wrapped->deltaT);
//}

//REAL8 LALInferenceTimeDomainOverlap(const REAL8TimeSeries *TDW, const REAL8TimeSeries *A, const REAL8TimeSeries *B) {
//  REAL8TimeSeries *Bconv;
//  REAL8 overlap;
//
//  Bconv = XLALCreateREAL8TimeSeries(B->name, &(B->epoch), 0.0, B->deltaT, &(B->sampleUnits), B->data->length);
//
//  LALInferenceConvolveTimeSeries(Bconv, B, TDW);
//
//  overlap = LALInferenceIntegrateSeriesProduct(A, Bconv);
//
//  XLALDestroyREAL8TimeSeries(Bconv);
//
//  return 4.0*overlap; /* This is the overlap definition. */
//}

static void extractDimensionlessVariableVector(LALInferenceVariables *currentParams, REAL8 *x, INT4 mode) {
  REAL8 m1, m2, d, iota, phi, psi, ra, dec, t, a1, a2, theta1, theta2, phi1, phi2;
  
  REAL8 mean[15];
  REAL8 Mc;
  
  memset(x, 0, 15*sizeof(REAL8));
  memset(mean, 0, 15*sizeof(REAL8));

  if (mode==0) {
    mean[0] = 16.0;
    mean[1] = 7.0;
    mean[2] = M_PI/2.0;
    mean[3] = M_PI;
    mean[4] = M_PI/2.0;
    mean[5] = M_PI;
    mean[6] = 0.0;
    mean[7] = 50.0;
    mean[8] = 0.0;
    mean[9] =0.5;
    mean[10]=0.5;
    mean[11]=M_PI/2.0;
    mean[12]=M_PI/2.0;
    mean[13]=M_PI;
    mean[14]=M_PI;
  } else if (mode==1) {
    mean[0] = 16.0;
    mean[1] = 7.0;
    mean[2] = 1.0*M_PI/4.0;
    mean[3] = 1.0*M_PI/2.0;
    mean[4] = 1.0*M_PI/4.0;
    mean[5] = 1.0*M_PI/2.0;
    mean[6] = -M_PI/4.0;
    mean[7] = 25.0;
    mean[8] = -0.03;
    mean[9] =0.2;
    mean[10]=0.2;
    mean[11]=1.0*M_PI/4.0;
    mean[12]=1.0*M_PI/4.0;
    mean[13]=1.0*M_PI/2.0;
    mean[14]=1.0*M_PI/2.0;
  } else if (mode==2) {
    /* set means of second mode to be 8 sigma from first mode */
    mean[0] = 16.0 + 8.*sqrt(CM[0][0]);
    mean[1] = 7.0 + 8.*sqrt(CM[1][1]);
    mean[2] = 1.0*M_PI/4.0 + 8.*sqrt(CM[2][2]);
    mean[3] = 1.0*M_PI/2.0 + 8.*sqrt(CM[3][3]);
    mean[4] = 1.0*M_PI/4.0 + 8.*sqrt(CM[4][4]);
    mean[5] = 1.0*M_PI/2.0 + 8.*sqrt(CM[5][5]);
    mean[6] = -M_PI/4.0 + 8.*sqrt(CM[6][6]);
    mean[7] = 25.0 + 8.*sqrt(CM[7][7]);
    mean[8] = -0.03 + 8.*sqrt(CM[8][8]);
    mean[9] =0.2 + 8.*sqrt(CM[9][9]);
    mean[10]=0.2 + 8.*sqrt(CM[10][10]);
    mean[11]=1.0*M_PI/4.0 + 8.*sqrt(CM[11][11]);
    mean[12]=1.0*M_PI/4.0 + 8.*sqrt(CM[12][12]);
    mean[13]=1.0*M_PI/2.0 + 8.*sqrt(CM[13][13]);
    mean[14]=1.0*M_PI/2.0 + 8.*sqrt(CM[14][14]);
    /* mean[0] = 16.0;
    mean[1] = 7.0;
    mean[2] = 3.0*M_PI/4.0;
    mean[3] = 3.0*M_PI/2.0;
    mean[4] = 3.0*M_PI/4.0;
    mean[5] = 3.0*M_PI/2.0;
    mean[6] = M_PI/4.0;
    mean[7] = 75.0;
    mean[8] = 0.03;
    mean[9] =0.8;
    mean[10]=0.8;
    mean[11]=3.0*M_PI/4.0;
    mean[12]=3.0*M_PI/4.0;
    mean[13]=3.0*M_PI/2.0;
    mean[14]=3.0*M_PI/2.0; */
  } else {
    printf("Error!  Unrecognized mode in analytic likelihood!\n");
    exit(1);
  }


  if (LALInferenceCheckVariable(currentParams,"m1")&&LALInferenceCheckVariable(currentParams,"m2"))
  {
    m1=*(REAL8 *)LALInferenceGetVariable(currentParams,"m1");
    m2=*(REAL8 *)LALInferenceGetVariable(currentParams,"m2");
  }
  else
  {
  	if (LALInferenceCheckVariable(currentParams, "chirpmass")) {
    	Mc = *(REAL8 *)LALInferenceGetVariable(currentParams, "chirpmass");
  	} else if (LALInferenceCheckVariable(currentParams, "logmc")) {
    	Mc = exp(*(REAL8 *)LALInferenceGetVariable(currentParams, "logmc"));
  	} else {
    	fprintf(stderr, "Could not find chirpmass or logmc in LALInferenceCorrelatedAnalyticLogLikelihood (in %s, line %d)\n", 
        	    __FILE__, __LINE__);
    	exit(1);
  	}

  	if (LALInferenceCheckVariable(currentParams, "massratio")) {
    	REAL8 eta = *(REAL8 *)LALInferenceGetVariable(currentParams, "massratio");
    	LALInferenceMcEta2Masses(Mc, eta, &m1, &m2);
  	} else if (LALInferenceCheckVariable(currentParams, "asym_massratio")) {
    	REAL8 q = *(REAL8 *)LALInferenceGetVariable(currentParams, "asym_massratio");
    	LALInferenceMcQ2Masses(Mc, q, &m1, &m2);
  	} else {
    	fprintf(stderr, "Could not find eta or q in LALInferenceCorrelatedAnalyticLogLikelihood (in %s, line %d)\n",
        	    __FILE__, __LINE__);
    	exit(1);
  	}
  }

  if (LALInferenceCheckVariable(currentParams, "distance")) {
    d = *(REAL8 *)LALInferenceGetVariable(currentParams, "distance");
  } else if (LALInferenceCheckVariable(currentParams, "logdistance")) {
    d = exp(*(REAL8 *)LALInferenceGetVariable(currentParams, "logdistance"));
  } else {
    fprintf(stderr, "Could not find distance or log(d) in LALInferenceCorrelatedAnalyticLogLikelihood (in %s, line %d)\n",
            __FILE__, __LINE__);
    exit(1);
  }

  iota = *(REAL8 *)LALInferenceGetVariable(currentParams, "inclination");
  psi = *(REAL8 *)LALInferenceGetVariable(currentParams, "polarisation");
  phi = *(REAL8 *)LALInferenceGetVariable(currentParams, "phase");
  ra = *(REAL8 *)LALInferenceGetVariable(currentParams, "rightascension");
  dec = *(REAL8 *)LALInferenceGetVariable(currentParams, "declination");
  t = *(REAL8 *)LALInferenceGetVariable(currentParams, "time");
  
  if (LALInferenceCheckVariable(currentParams, "a_spin1")) {
    a1 = *(REAL8 *)LALInferenceGetVariable(currentParams, "a_spin1");
  } else {
    a1 = 0.0;
  }

  if (LALInferenceCheckVariable(currentParams, "a_spin2")) {
    a2 = *(REAL8 *)LALInferenceGetVariable(currentParams, "a_spin2");
  } else {
    a2 = 0.0;
  }

  if (LALInferenceCheckVariable(currentParams, "phi_spin1")) {
    phi1 = *(REAL8 *)LALInferenceGetVariable(currentParams, "phi_spin1");
  } else {
    phi1 = 0.0;
  }
  
  if (LALInferenceCheckVariable(currentParams, "phi_spin2")) {
    phi2 = *(REAL8 *)LALInferenceGetVariable(currentParams, "phi_spin2");
  } else {
    phi2 = 0.0;
  }
  
  if (LALInferenceCheckVariable(currentParams, "theta_spin1")) {
    theta1 = *(REAL8 *)LALInferenceGetVariable(currentParams, "theta_spin1");
  } else {
    theta1 = 0.0;
  }

  if (LALInferenceCheckVariable(currentParams, "theta_spin2")) {
    theta2 = *(REAL8 *)LALInferenceGetVariable(currentParams, "theta_spin2");
  } else {
    theta2 = 0.0;
  }

  x[0] = (m1    - mean[0]);
  x[1] = (m2    - mean[1]);
  x[2] = (iota  - mean[2])/(M_PI/20.0);
  x[3] = (phi   - mean[3])/(M_PI/10.0);
  x[4] = (psi   - mean[4])/(M_PI/20.0);
  x[5] = (ra    - mean[5])/(M_PI/10.0);
  x[6] = (dec   - mean[6])/(M_PI/10.0);
  x[7] = (d     - mean[7])/10.0;
  x[8] = (t     - mean[8])/0.1;
  x[9] =(a1     - mean[9])/0.1;
  x[10]=(a2     - mean[10])/0.1;
  x[11]=(theta1 - mean[11])/(M_PI/20.0);
  x[12]=(theta2 - mean[12])/(M_PI/20.0);
  x[13]=(phi1   - mean[13])/(M_PI/10.0);
  x[14]=(phi2   - mean[14])/(M_PI/10.0);
}

REAL8 LALInferenceCorrelatedAnalyticLogLikelihood(LALInferenceVariables *currentParams, 
                                                  LALInferenceIFOData UNUSED *data, 
                                                  LALInferenceTemplateFunction UNUSED template) {
  const INT4 DIM = 15;
  static gsl_matrix *LUCM = NULL;
  static gsl_permutation *LUCMPerm = NULL;
  INT4 mode = 0;
  
  REAL8 x[DIM];
  REAL8 xOrig[DIM];

  gsl_vector_view xView = gsl_vector_view_array(x, DIM);

  extractDimensionlessVariableVector(currentParams, x, mode);

  memcpy(xOrig, x, DIM*sizeof(REAL8));

  if (LUCM==NULL) {
    gsl_matrix_const_view CMView = gsl_matrix_const_view_array(&(CM[0][0]), DIM, DIM);
    int signum;

    LUCM = gsl_matrix_alloc(DIM, DIM);
    LUCMPerm = gsl_permutation_alloc(DIM);

    gsl_matrix_memcpy(LUCM, &(CMView.matrix));

    gsl_linalg_LU_decomp(LUCM, LUCMPerm, &signum);
  }

  gsl_linalg_LU_svx(LUCM, LUCMPerm, &(xView.vector));

  INT4 i;
  REAL8 sum = 0.0;
  for (i = 0; i < DIM; i++) {
    sum += xOrig[i]*x[i];
  }
  //gsl_matrix_free(LUCM);
  //gsl_permutation_free(LUCMPerm);
  return -sum/2.0;
}

REAL8 LALInferenceBimodalCorrelatedAnalyticLogLikelihood(LALInferenceVariables *currentParams,
                                                  LALInferenceIFOData UNUSED *data,
                                                  LALInferenceTemplateFunction UNUSED template) {
  const INT4 DIM = 15;
  const INT4 MODES = 2;
  INT4 i, mode;
  REAL8 sum = 0.0;
  REAL8 a, b;
  static gsl_matrix *LUCM = NULL;
  static gsl_permutation *LUCMPerm = NULL;
  gsl_vector_view xView;

  REAL8 x[DIM];
  REAL8 xOrig[DIM];
  REAL8 exps[MODES];

  if (LUCM==NULL) {
    gsl_matrix_const_view CMView = gsl_matrix_const_view_array(&(CM[0][0]), DIM, DIM);
    int signum;

    LUCM = gsl_matrix_alloc(DIM, DIM);
    LUCMPerm = gsl_permutation_alloc(DIM);

    gsl_matrix_memcpy(LUCM, &(CMView.matrix));

    gsl_linalg_LU_decomp(LUCM, LUCMPerm, &signum);
  }

  for(mode = 1; mode < 3; mode++) {
    xView = gsl_vector_view_array(x, DIM);

    extractDimensionlessVariableVector(currentParams, x, mode);

    memcpy(xOrig, x, DIM*sizeof(REAL8));

    gsl_linalg_LU_svx(LUCM, LUCMPerm, &(xView.vector));

    sum = 0.0;
    for (i = 0; i < DIM; i++) {
      sum += xOrig[i]*x[i];
    }
    exps[mode-1] = -sum/2.0;
  }

  /* Assumes only two modes used from here on out */
  if (exps[0] > exps[1]) {
    a = exps[0];
    b = exps[1];
  } else {
    a = exps[1];
    b = exps[0];
  }
  //gsl_matrix_free(LUCM);
  //gsl_permutation_free(LUCMPerm);

  /* attempt to keep returned values finite */
  return a + log1p(exp(b-a));
}

REAL8 LALInferenceRosenbrockLogLikelihood(LALInferenceVariables *currentParams,
                                          LALInferenceIFOData UNUSED *data,
                                          LALInferenceTemplateFunction UNUSED template) {
  const INT4 DIM = 15;
  REAL8 x[DIM];

  REAL8 sum = 0;
  INT4 mode = 0;
  INT4 i;

  extractDimensionlessVariableVector(currentParams, x, mode);

  for (i = 0; i < DIM-1; i++) {
    REAL8 oneMX = 1.0 - x[i];
    REAL8 dx = x[i+1] - x[i]*x[i];

    sum += oneMX*oneMX + 100.0*dx*dx;
  }

  return -sum;
}

//void LALInferencePSDToTDW(REAL8TimeSeries *TDW, const REAL8FrequencySeries *PSD, const REAL8FFTPlan *plan,
//              const REAL8 fMin, const REAL8 fMax) {
//  COMPLEX16FrequencySeries *CPSD = NULL;
//  UINT4 i;
//  UINT4 PSDLength = TDW->data->length/2 + 1;
//  
//  if (PSD->data->length != PSDLength) {
//    fprintf(stderr, "PSDToTDW: lengths of PSD and TDW do not match (in %s, line %d)", 
//            __FILE__, __LINE__);
//    exit(1);
//  }
//
//  CPSD = 
//    XLALCreateCOMPLEX16FrequencySeries(PSD->name, &(PSD->epoch), PSD->f0, PSD->deltaF, &(PSD->sampleUnits), PSD->data->length);
//
//  for (i = 0; i < PSD->data->length; i++) {
//    REAL8 f = PSD->f0 + i*PSD->deltaF;
//
//    if (fMin <= f && f <= fMax) {
//      CPSD->data->data[i].re = 1.0 / (2.0*PSD->data->data[i]);
//      CPSD->data->data[i].im = 0.0;
//    } else {
//      CPSD->data->data[i].re = 0.0;
//      CPSD->data->data[i].im = 0.0;
//    }
//  }
//
//  XLALREAL8FreqTimeFFT(TDW, CPSD, plan);
//
//  /* FILE *PSDf = fopen("PSD.dat", "w"); */
//  /* for (i = 0; i < PSD->data->length; i++) { */
//  /*   fprintf(PSDf, "%g %g\n", i*PSD->deltaF, PSD->data->data[i]); */
//  /* } */
//  /* fclose(PSDf); */
//
//  /* FILE *TDWf = fopen("TDW.dat", "w"); */
//  /* for (i = 0; i < TDW->data->length; i++) { */
//  /*   fprintf(TDWf, "%g %g\n", i*TDW->deltaT, TDW->data->data[i]); */
//  /* } */
//  /* fclose(TDWf); */
//}

REAL8 LALInferenceUndecomposedFreqDomainLogLikelihood_Burst(LALInferenceVariables *currentParams, LALInferenceIFOData * data, LALInferenceTemplateFunction templt)
/***************************************************************/
/* (log-) likelihood function.                                 */
/* Returns the non-normalised logarithmic likelihood.          */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Required (`currentParams') parameters are:                  */
/*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
/*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
/*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
/*   - "q"        (REAL8, q, >0)                      */
/*   - "time"            (REAL8, GPS sec.)                     */
/*   - "f0"            (REAL8, Hz.)                     */
/*   - "hrss"            (REAL8, strain.)                     */
/*   - "time"            (REAL8, GPS sec.)                     */

/***************************************************************/
{  
  //static int timeDomainWarning = 0;
  double Fplus, Fcross;
  double FplusScaled, FcrossScaled;
  double diffRe, diffIm, diffSquared;
  double dataReal, dataImag;
  REAL8 loglikeli;
  REAL8 plainTemplateReal, plainTemplateImag;
  REAL8 templateReal, templateImag;
  int i, lower, upper;
  LALInferenceIFOData *dataPtr;
  double ra, dec, psi;
  //,q,hrss,f0,delta_t,eccentricity,polar_angle;
  double GPSdouble,gmst;
  LIGOTimeGPS GPSlal;
  double chisquared;
  double timedelay;  /* time delay b/w iterferometer & geocenter w.r.t. sky location */
  double timeshift;  /* time shift (not necessarily same as above)                   */
  double deltaT, TwoDeltaToverN, deltaF, twopit, f, re, im;
  double timeTmp;
  int different;
  LALStatus status;
  memset(&status,0,sizeof(status));
  LALInferenceVariables intrinsicParams;
 // double norm=0;
  /* determine source's sky location & orientation parameters: */
  ra        = *(REAL8*) LALInferenceGetVariable(currentParams, "rightascension"); // radian      
  dec       = *(REAL8*) LALInferenceGetVariable(currentParams, "declination");    // radian      
  psi       = *(REAL8*) LALInferenceGetVariable(currentParams, "polarisation");   // radian      
  GPSdouble = *(REAL8*) LALInferenceGetVariable(currentParams, "time");           // GPS seconds 
 
  /* figure out GMST: */
  //XLALINT8NSToGPS(&GPSlal, floor(1e9 * GPSdouble + 0.5));
  XLALGPSSetREAL8(&GPSlal, GPSdouble);
  //UandA.units    = MST_RAD;
  //UandA.accuracy = LALLEAPSEC_LOOSE;
  //LALGPStoGMST1(&status, &gmst, &GPSlal, &UandA);
  gmst=XLALGreenwichMeanSiderealTime(&GPSlal);
  intrinsicParams.head      = NULL;
  intrinsicParams.dimension = 0;
  LALInferenceCopyVariables(currentParams, &intrinsicParams);
  LALInferenceRemoveVariable(&intrinsicParams, "rightascension");
  LALInferenceRemoveVariable(&intrinsicParams, "declination");
  LALInferenceRemoveVariable(&intrinsicParams, "polarisation");
  LALInferenceRemoveVariable(&intrinsicParams, "time");

  // TODO: add pointer to template function here.
  // (otherwise same parameters but different template will lead to no re-computation!!)

  chisquared = 0.0;
  /* loop over data (different interferometers): */
  dataPtr = data;
REAL8 net_snr=0.0,ifo_snr=0.0;

  while (dataPtr != NULL) {
    ifo_snr=0.0;
    /* The parameters the Likelihood function can handle by itself   */
    /* (and which shouldn't affect the template function) are        */
    /* sky location (ra, dec), polarisation and signal arrival time. */
    /* Note that the template function shifts the waveform to so that*/
	/* t_c corresponds to the "time" parameter in                    */
	/* IFOdata->modelParams (set, e.g., from the trigger value).     */
    
    /* Reset log-likelihood */
    dataPtr->loglikelihood = 0.0;

    /* Compare parameter values with parameter values corresponding  */
    /* to currently stored template; ignore "time" variable:         */
    if (LALInferenceCheckVariable(dataPtr->modelParams, "time")) {
      timeTmp = *(REAL8 *) LALInferenceGetVariable(dataPtr->modelParams, "time");
      LALInferenceRemoveVariable(dataPtr->modelParams, "time");
    }
    else timeTmp = GPSdouble;
    different = LALInferenceCompareVariables(dataPtr->modelParams, &intrinsicParams);
    /* "different" now may also mean that "dataPtr->modelParams" */
    /* wasn't allocated yet (as in the very 1st iteration).      */

    if (different) { /* template needs to be re-computed: */
      //  printf("IM RECALCULATING THE TEMPLATE!\n");

      LALInferenceCopyVariables(&intrinsicParams, dataPtr->modelParams);
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
      //LALInferenceAddVariable(dataPtr->modelParams, "polarisation", &psi, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);

      templt(dataPtr);
      if(XLALGetBaseErrno()==XLAL_FAILURE) /* Template generation failed in a known way, set -Inf likelihood */
          return(-DBL_MAX);

          if (dataPtr->modelDomain == LAL_SIM_DOMAIN_TIME) {
    //	if (!timeDomainWarning) {
//	  timeDomainWarning = 1;
//	  fprintf(stderr, "WARNING: using time domain template with frequency domain likelihood (in %s, line %d)\n", __FILE__, __LINE__);
//	} 
//printf("Im doing the FFT in likelihood burst\n");
        LALInferenceExecuteFT(dataPtr);
       // norm=sqrt(dataPtr->window->data->length/dataPtr->window->sumofsquares);
        /* note that the dataPtr->modelParams "time" element may have changed here!! */
        /* (during "template()" computation)  */
      }
    }
    else { /* no re-computation necessary. Return back "time" value, do nothing else: */
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
    }

    /*-- Template is now in dataPtr->freqModelhPlus and dataPtr->freqModelhCross. --*/
    /*-- (Either freshly computed or inherited.)                            --*/

    /* determine beam pattern response (F_plus and F_cross) for given Ifo: */
    XLALComputeDetAMResponse(&Fplus, &Fcross,
                             dataPtr->detector->response,
			     ra, dec, psi, gmst);
    /* signal arrival time (relative to geocenter); */
    timedelay = XLALTimeDelayFromEarthCenter(dataPtr->detector->location,
                                             ra, dec, &GPSlal);
    /* (negative timedelay means signal arrives earlier at Ifo than at geocenter, etc.) */
    /* amount by which to time-shift template (not necessarily same as above "timedelay"): */
    timeshift =  (GPSdouble - (*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time"))) + timedelay;
  //timeshift = timedelay;
    twopit    = LAL_TWOPI * timeshift;
    
    /* include distance (overall amplitude) effect in Fplus/Fcross: */
    FplusScaled  = Fplus  ;
    FcrossScaled = Fcross ;
    dataPtr->fPlus = FplusScaled;
    dataPtr->fCross = FcrossScaled;
    dataPtr->timeshift = timeshift;

    /*char fname[256];
    sprintf(fname,"%s_template.dat",dataPtr->name);
    FILE *testout=fopen(fname,"w");
    */
    
    /* determine frequency range & loop over frequency bins: */
    deltaT = dataPtr->timeData->deltaT;
    deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);
    //printf("deltaF %g, Nt %d, deltaT %g\n", deltaF, dataPtr->timeData->data->length, dataPtr->timeData->deltaT);
    lower = (UINT4)ceil(dataPtr->fLow / deltaF);
    upper = (UINT4)floor(dataPtr->fHigh / deltaF);
    TwoDeltaToverN = 2.0 * deltaT / ((double) dataPtr->timeData->data->length);
    for (i=lower; i<=upper; ++i){
      /* derive template (involving location/orientation parameters) from given plus/cross waveforms: */
      plainTemplateReal = FplusScaled * creal(dataPtr->freqModelhPlus->data->data[i])  
                          +  FcrossScaled * creal(dataPtr->freqModelhCross->data->data[i]);
      plainTemplateImag = FplusScaled * cimag(dataPtr->freqModelhPlus->data->data[i])  
                          +  FcrossScaled * cimag(dataPtr->freqModelhCross->data->data[i]);

      /* do time-shifting...             */
      /* (also un-do 1/deltaT scaling): */
      f = ((double) i) * deltaF;
      /* real & imag parts of  exp(-2*pi*i*f*deltaT): */
      re = cos(twopit * f);
      im = - sin(twopit * f);
      templateReal = (plainTemplateReal*re - plainTemplateImag*im) / deltaT; //salvo remove norm?
      templateImag = (plainTemplateReal*im + plainTemplateImag*re) / deltaT; //salvo remove norm?
      dataReal     = creal(dataPtr->freqData->data->data[i]) / deltaT;
      dataImag     = cimag(dataPtr->freqData->data->data[i]) / deltaT;
      /* compute squared difference & 'chi-squared': */
      diffRe       = dataReal - templateReal;         // Difference in real parts...
      diffIm       = dataImag - templateImag;         // ...and imaginary parts, and...
      diffSquared  = diffRe*diffRe + diffIm*diffIm ;  // ...squared difference of the 2 complex figures.
      REAL8 temp = ((TwoDeltaToverN * diffSquared) / dataPtr->oneSidedNoisePowerSpectrum->data->data[i]);
      ifo_snr+=TwoDeltaToverN*(templateReal*templateReal+templateImag*templateImag)/ dataPtr->oneSidedNoisePowerSpectrum->data->data[i];
      //fprintf(testout,"%10.10e %10.10e %10.10e \n",f,deltaT*templateReal,deltaT*templateImag);
      chisquared  += temp;
      dataPtr->loglikelihood -= temp;
 
    }
    dataPtr = dataPtr->next;
    net_snr+=4.0*ifo_snr;
    //fclose(testout);
  }
 
  net_snr=sqrt(net_snr);
  //{
  //    printf("logl %10.10e \n",-1.0 * chisquared);
 // printf("Q %10.2e f %10.2e hrss %10.2e time %10.10f eccentricity %10.10e polangle %10.10e norm %f \n",q,f0,hrss,GPSdouble,eccentricity,polar_angle,norm);
//}
  
  loglikeli = -1.0 * chisquared; // note (again): the log-likelihood is unnormalised!
  LALInferenceClearVariables(&intrinsicParams);
  //printf("%.10e \n",loglikeli);
  //exit(1);
  return(loglikeli);
}

REAL8 LALInferenceUndecomposedFreqDomainLogLikelihood_BestIFO(LALInferenceVariables *currentParams, LALInferenceIFOData * data,LALInferenceTemplateFunction templt)
/***************************************************************/
/* (log-) likelihood function.                                 */
/* Returns the non-normalised logarithmic likelihood.          */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Required (`currentParams') parameters are:                  */
/*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
/*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
/*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
/*   - "q"        (REAL8, q, >0)                      */
/*   - "time"            (REAL8, GPS sec.)                     */
/*   - "f0"            (REAL8, Hz.)                     */
/*   - "hrss"            (REAL8, strain.)                     */
/*   - "time"            (REAL8, GPS sec.)                     */

/***************************************************************/
{
  //static int timeDomainWarning = 0;
  (void) templt;
  double Fplus, Fcross, FplusBest, FcrossBest;
  double FplusScaled, FcrossScaled;
  double diffRe, diffIm, diffSquared;
  double dataReal, dataImag;
  REAL8 loglikeli;
  REAL8 plainTemplateReal, plainTemplateImag;
  REAL8 templateReal, templateImag;
  int i, lower, upper;
  LALInferenceIFOData *dataPtr;
  double ra, dec, psi;
  //,q,hrss,f0,delta_t,eccentricity,polar_angle;
  double GPSdouble,gmst;
  LIGOTimeGPS GPSlal;
  double chisquared;
  double timedelay;  /* time delay b/w iterferometer & geocenter w.r.t. sky location */
  double timeshift;  /* time shift (not necessarily same as above)                   */
  double deltaT, TwoDeltaToverN, deltaF, twopit, f, re, im;
  //double timeTmp;
  //double atan_shift;
  double ci=0.0,iota;
  //int different;
  LALStatus status;
  memset(&status,0,sizeof(status));
  //LALInferenceVariables intrinsicParams;
 // double norm=0;
  /* determine source's sky location & orientation parameters: */
  ra        = *(REAL8*) LALInferenceGetVariable(currentParams, "rightascension"); // radian      
  dec       = *(REAL8*) LALInferenceGetVariable(currentParams, "declination");    // radian      
  psi       = *(REAL8*) LALInferenceGetVariable(currentParams, "polarisation");   // radian      
  GPSdouble = *(REAL8*) LALInferenceGetVariable(currentParams, "time");           // GPS seconds 
  if (LALInferenceCheckVariable(currentParams, "inclination")){
    iota= *(REAL8*) LALInferenceGetVariable(currentParams, "inclination");
    ci=cos(iota);
    }   
  XLALGPSSetREAL8(&GPSlal, GPSdouble);
  gmst=XLALGreenwichMeanSiderealTime(&GPSlal);
  //intrinsicParams.head      = NULL;
  //intrinsicParams.dimension = 0;
  //LALInferenceCopyVariables(currentParams, &intrinsicParams);
  //LALInferenceRemoveVariable(&intrinsicParams, "rightascension");
  //LALInferenceRemoveVariable(&intrinsicParams, "declination");
  //LALInferenceRemoveVariable(&intrinsicParams, "polarisation");
  //LALInferenceRemoveVariable(&intrinsicParams, "time");

  // TODO: add pointer to template function here.
  // (otherwise same parameters but different template will lead to no re-computation!!)

  chisquared = 0.0;
  /* loop over data (different interferometers): */
  dataPtr = data;
    REAL8 net_snr=0.0,ifo_snr=0.0;
    UINT4 kappa=0;
  if (dataPtr->skipIFO==1) dataPtr=dataPtr->next;
  //while (dataPtr != NULL && dataPtr->skipIFO!=1) {
   while (dataPtr != NULL) {
   
     // printf("doing logL for ifo %s\n",dataPtr->name);
    ifo_snr=0.0;
    /* The parameters the Likelihood function can handle by itself   */
    /* (and which shouldn't affect the template function) are        */
    /* sky location (ra, dec), polarisation and signal arrival time. */
    /* Note that the template function shifts the waveform to so that*/
	/* t_c corresponds to the "time" parameter in                    */
	/* IFOdata->modelParams (set, e.g., from the trigger value).     */
    
    /* Reset log-likelihood */
    dataPtr->loglikelihood = 0.0;

  
    //timeTmp = dataPtr->injtime; 
	//LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
    
    /*-- Template is now in dataPtr->freqModelhPlus and dataPtr->freqModelhCross. --*/
    /*-- (Either freshly computed or inherited.)                            --*/

    /* determine beam pattern response (F_plus and F_cross) for given Ifo: */
    XLALComputeDetAMResponse(&Fplus, &Fcross,
                             dataPtr->detector->response,
			     ra, dec, psi, gmst);
    XLALComputeDetAMResponse(&FplusBest, &FcrossBest,
                             dataPtr->BestIFO->detector->response,
			     ra, dec, psi, gmst);           
    /* signal arrival time (relative to geocenter); */
    timedelay = XLALTimeDelayFromEarthCenter(dataPtr->detector->location,
                                             ra, dec, &GPSlal);
    REAL8 best_timedelay;
     best_timedelay = XLALTimeDelayFromEarthCenter(dataPtr->BestIFO->detector->location,
                                             ra, dec, &GPSlal);
    /* (negative timedelay means signal arrives earlier at Ifo than at geocenter, etc.) */
    /* amount by which to time-shift template (not necessarily same as above "timedelay"): */
    //timeshift =  (GPSdouble - (*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time"))-best_timedelay) + timedelay;
    
     //printf("IFO %s besttd %lf this td %lf diff %lf\n",dataPtr->name,best_timedelay,timedelay,best_timedelay-timedelay);
  timeshift = GPSdouble-dataPtr->injtime+timedelay-best_timedelay;
    twopit    = LAL_TWOPI * timeshift;
//printf("Template's time %10.5lf time %10.5lf epoch=%10.5lf\n", GPSdouble,(*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time")),XLALGPSGetREAL8(&(dataPtr->freqData->epoch))    );
    /* include distance (overall amplitude) effect in Fplus/Fcross: */
    FplusScaled  = Fplus  ;
    FcrossScaled = Fcross ;
    dataPtr->fPlus = FplusScaled;
    dataPtr->fCross = FcrossScaled;
    dataPtr->timeshift = timeshift;
    
    REAL8 atan_shift=atan2(-FcrossScaled*ci,FplusScaled*(ci*ci+1.0)/2.0);//Salvo: Remember to use differet cosiota here if not pinned!!!
//    printf("atan %lf ci %10.10e \n",atan_timeshift,ci  );
    REAL8 best_atan_shift=atan2(-FcrossBest*ci,FplusBest*(ci*ci+1.0)/2.0);
    REAL8 mu, best_mu;
    
    mu=sqrt(FplusScaled*FplusScaled*(0.5*(1.0+ci*ci))*(0.5*(1.0+ci*ci)) +FcrossScaled * FcrossScaled*ci*ci);
    best_mu=sqrt(FplusBest*FplusBest*(0.5*(1.0+ci*ci))*(0.5*(1.0+ci*ci))+FcrossBest*FcrossBest*ci*ci);
    //REAL8 best_amp, best_pha;
    
    /* determine frequency range & loop over frequency bins: */
    deltaT = dataPtr->timeData->deltaT;
    deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);
    lower = (UINT4)ceil(dataPtr->fLow / deltaF);
    upper = (UINT4)floor(dataPtr->fHigh / deltaF);
    TwoDeltaToverN = 2.0 * deltaT / ((double) dataPtr->timeData->data->length);
    
  //  printf("seg start in like %lf\n", (*(REAL8*) LALInferenceGetVariable(data->modelParams, "time")));
  //   FILE * testout = fopen("cippa.txt","w");
  
  /*remove time shift from best ifo 
   for (i=lower; i<=upper; ++i){
        f = ((double) i) * deltaF;
       
      re = cos(-LAL_TWOPI*best_timedelay * f-best_atan_shift);
      im = -sin(-LAL_TWOPI*best_timedelay  *f-best_atan_shift);
       dataPtr->BestIFO->TemplateFromInjection->data->data[i].re=dataPtr->BestIFO->TemplateFromInjection->data->data[i].re*re-dataPtr->BestIFO->TemplateFromInjection->data->data[i].im*im;
       dataPtr->BestIFO->TemplateFromInjection->data->data[i].im=dataPtr->BestIFO->TemplateFromInjection->data->data[i].re*im+dataPtr->BestIFO->TemplateFromInjection->data->data[i].im*re;
       }*/
  REAL8 tmp_re,tmp_im;
    for (i=lower; i<=upper; ++i){
      /* derive template (involving location/orientation parameters) from given plus/cross waveforms: */
            f = ((double) i) * deltaF;

      /*best_amp=sqrt(dataPtr->BestIFO->TemplateFromInjection->data->data[i].re*  dataPtr->BestIFO->TemplateFromInjection->data->data[i].re+ dataPtr->BestIFO->TemplateFromInjection->data->data[i].im*dataPtr->BestIFO->TemplateFromInjection->data->data[i].im);
      best_pha=atan2(dataPtr->BestIFO->TemplateFromInjection->data->data[i].im,dataPtr->BestIFO->TemplateFromInjection->data->data[i].re);
      
      plainTemplateReal=mu/best_mu*best_amp*cos(best_pha+atan_shift-best_atan_shift);
      plainTemplateImag=mu/best_mu*best_amp*sin(best_pha+atan_shift-best_atan_shift);
*/


       // re = cos(-LAL_TWOPI*((GPSdouble - (*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time")))+best_timedelay) * f-best_atan_shift);
     // im = sin(-LAL_TWOPI*((GPSdouble - (*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time")))+best_timedelay)  *f-best_atan_shift);
    //    re = cos(-LAL_TWOPI*best_timedelay* f-best_atan_shift);
     // im = sin(  -LAL_TWOPI*best_timedelay*f-best_atan_shift);
     re = cos(-best_atan_shift);
     im = sin(-best_atan_shift);
      tmp_re= creal(dataPtr->BestIFO->TemplateFromInjection->data->data[i])*re-cimag(dataPtr->BestIFO->TemplateFromInjection->data->data[i])*im;
       tmp_im=creal(dataPtr->BestIFO->TemplateFromInjection->data->data[i])*im+cimag(dataPtr->BestIFO->TemplateFromInjection->data->data[i])*re;

      plainTemplateReal = mu/best_mu*tmp_re;
      plainTemplateImag =mu/best_mu*tmp_im;

      /* do time-shifting...             */
      /* (also un-do 1/deltaT scaling): */
      f = ((double) i) * deltaF;
      /* real & imag parts of  exp(-2*pi*i*f*deltaT): */
      re = cos(twopit * f- atan_shift);
      im = - sin(twopit *f- atan_shift);
      
        
      templateReal = (plainTemplateReal*re - plainTemplateImag*im) / deltaT; //salvo remove norm?
      templateImag = (plainTemplateReal*im + plainTemplateImag*re) / deltaT; //salvo remove norm?
      dataReal     = creal(dataPtr->freqData->data->data[i]) / deltaT;
      dataImag     = cimag(dataPtr->freqData->data->data[i]) / deltaT;
      /* compute squared difference & 'chi-squared': */
      diffRe       = dataReal - templateReal;         // Difference in real parts...
      diffIm       = dataImag - templateImag;         // ...and imaginary parts, and...
      diffSquared  = diffRe*diffRe + diffIm*diffIm ;  // ...squared difference of the 2 complex figures.
      REAL8 temp = ((TwoDeltaToverN * diffSquared) / dataPtr->oneSidedNoisePowerSpectrum->data->data[i]);
      ifo_snr+=TwoDeltaToverN*(templateReal*templateReal+templateImag*templateImag)/ dataPtr->oneSidedNoisePowerSpectrum->data->data[i];
      chisquared  += temp;
      dataPtr->loglikelihood -= temp;
 //fprintf(testout, "%e %e %e %e %e\n",f, dataPtr->freqData->data->data[i].re, dataPtr->freqData->data->data[i].im,    templateReal, templateImag);
    }
    dataPtr = dataPtr->next;
    net_snr+=4.0*ifo_snr;
   // if(sqrt(ifo_snr)>1000.0)
    //printf("too large single IFOSNR %lf \n",sqrt(ifo_snr));
// fclose(testout);
 kappa++;
  }
  net_snr=sqrt(net_snr);
 // if (net_snr>=99991)
  {
 //     printf("net_snr %10.10e \n",net_snr);
 // printf("Q %10.2e f %10.2e hrss %10.2e time %10.10f eccentricity %10.10e polangle %10.10e norm %f \n",q,f0,hrss,GPSdouble,eccentricity,polar_angle,norm);
}
  loglikeli = -1.0 * chisquared; // note (again): the log-likelihood is unnormalised!
  //LALInferenceDestroyVariables(&intrinsicParams);
 // printf("got %10.10e\n",loglikeli);
  return(loglikeli);
}


REAL8 LALInferenceMarginalisedPhaseLogLikelihood_HMNS(LALInferenceVariables *currentParams, LALInferenceIFOData * data,LALInferenceTemplateFunction templt)
    /***************************************************************/
    /* (log-) likelihood function.                                 */
    /* Returns the non-normalised logarithmic likelihood.          */
    /* Analytically marginalised over phase and distance           */
    /* Attaches output variables for (h|h), (d|d) and (d|h^*)      */
    /* At a distance of 1 Mpc for phi_0=0                          */
    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    /* Required (`currentParams') parameters are:                  */
    /*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
    /*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
    /*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
    /*   - "time"            (REAL8, GPS sec.)                     */
    /***************************************************************/
{

    //static int timeDomainWarning = 0;
    double Fplus, Fcross;
    double FplusScaled, FcrossScaled;
    double dataReal, dataImag;
    REAL8 loglikeli=0.0;
    REAL8 plainTemplateReal, plainTemplateImag;
    REAL8 templateReal, templateImag;
    int i, lower, upper;
    LALInferenceIFOData *dataPtr;
    /* XXX: setting distMpc=1.0 in case it's not used */
    double ra, dec, psi, gmst;
    double GPSdouble;
    LIGOTimeGPS GPSlal;
    double timedelay;  /* time delay b/w iterferometer & geocenter w.r.t. sky location */
    double timeshift;  /* time shift (not necessarily same as above)                   */
    double deltaT, TwoDeltaToverN, deltaF, twopit, re, im, dre, dim, newRe, newIm;
    double timeTmp;
    int different;

    LALStatus status;
    memset(&status,0,sizeof(status));
    LALInferenceVariables intrinsicParams;

    /* determine source's sky location & orientation parameters: */
    ra        = *(REAL8*) LALInferenceGetVariable(currentParams, "rightascension"); /* radian      */
    dec       = *(REAL8*) LALInferenceGetVariable(currentParams, "declination");    /* radian      */
    psi       = *(REAL8*) LALInferenceGetVariable(currentParams, "polarisation");   /* radian      */
    GPSdouble = *(REAL8*) LALInferenceGetVariable(currentParams, "time");           /* GPS seconds */

    REAL8 phi0=0.0;

    /* figure out GMST: */
    XLALGPSSetREAL8(&GPSlal, GPSdouble);
    gmst=XLALGreenwichMeanSiderealTime(&GPSlal);

    /* Create parameter set to pass to the template function */
    intrinsicParams.head      = NULL;
    intrinsicParams.dimension = 0;
    LALInferenceCopyVariables(currentParams, &intrinsicParams);

    LALInferenceRemoveVariable(&intrinsicParams, "rightascension");
    LALInferenceRemoveVariable(&intrinsicParams, "declination");
    LALInferenceRemoveVariable(&intrinsicParams, "polarisation");
    LALInferenceRemoveVariable(&intrinsicParams, "time");
    LALInferenceRemoveVariable(&intrinsicParams, "phase");
    LALInferenceAddVariable(&intrinsicParams, "phase",&phi0,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);

    // TODO: add pointer to template function here.
    // (otherwise same parameters but different template will lead to no re-computation!!)

    /* loop over data (different interferometers): */
    dataPtr = data;
    REAL8 S=0.,D=0.,R=0., Rre=0., Rim=0.;
    /* Need to compute S=h*h/S_h, D=d*d/S_h, R=h*d/S_h^2 */
    while (dataPtr != NULL) {
        /* The parameters the Likelihood function can handle by itself   */
        /* (and which shouldn't affect the template function) are        */
        /* sky location (ra, dec), polarisation and signal arrival time. */
        /* Note that the template function shifts the waveform to so that*/
        /* t_c corresponds to the "time" parameter in                    */
        /* IFOdata->modelParams (set, e.g., from the trigger value).     */

        /* Reset log-likelihood */
        dataPtr->loglikelihood = 0.0;

        /* Compare parameter values with parameter values corresponding  */
        /* to currently stored template; ignore "time" variable:         */
        if (LALInferenceCheckVariable(dataPtr->modelParams, "time")) {
            timeTmp = *(REAL8 *) LALInferenceGetVariable(dataPtr->modelParams, "time");
            LALInferenceRemoveVariable(dataPtr->modelParams, "time");
        }
        else timeTmp = GPSdouble;
        different = LALInferenceCompareVariables(dataPtr->modelParams, &intrinsicParams);
        /* "different" now may also mean that "dataPtr->modelParams" */
        /* wasn't allocated yet (as in the very 1st iteration).      */

        if (different) { /* template needs to be re-computed: */
            LALInferenceCopyVariables(&intrinsicParams, dataPtr->modelParams);
            LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
            templt(dataPtr);
            if(XLALGetBaseErrno()==XLAL_FAILURE) /* Template generation failed in a known way, set -Inf likelihood */
                return(-DBL_MAX);

            if (dataPtr->modelDomain == LAL_SIM_DOMAIN_TIME) {
                LALInferenceExecuteFT(dataPtr);
                /* note that the dataPtr->modelParams "time" element may have changed here!! */
                /* (during "template()" computation)  */
            }
        }
        else { /* no re-computation necessary. Return back "time" value, do nothing else: */
            LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
        }

        /*-- Template is now in dataPtr->freqModelhPlus and dataPtr->freqModelhCross. --*/
        /*-- (Either freshly computed or inherited.)                            --*/

        /* determine beam pattern response (F_plus and F_cross) for given Ifo: */
        XLALComputeDetAMResponse(&Fplus, &Fcross,
                dataPtr->detector->response,
                ra, dec, psi, gmst);
        /* signal arrival time (relative to geocenter); */
        timedelay = XLALTimeDelayFromEarthCenter(dataPtr->detector->location,
                ra, dec, &GPSlal);
        /* (negative timedelay means signal arrives earlier at Ifo than at geocenter, etc.) */
        /* amount by which to time-shift template (not necessarily same as above "timedelay"): */
        timeshift =  (GPSdouble - (*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time"))) + timedelay;
        twopit    = LAL_TWOPI * timeshift;

        /* Allow for inclusion of distance (overall amplitude) effect in Fplus/Fcross: */
        FplusScaled  = Fplus;
        FcrossScaled = Fcross;

        dataPtr->fPlus = FplusScaled;
        dataPtr->fCross = FcrossScaled;
        dataPtr->timeshift = timeshift;

        /* determine frequency range & loop over frequency bins: */
        deltaT = dataPtr->timeData->deltaT;
        deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);
        lower = (UINT4)ceil(dataPtr->fLow / deltaF);
        upper = (UINT4)floor(dataPtr->fHigh / deltaF);
        TwoDeltaToverN = 2.0 * deltaT / ((double) dataPtr->timeData->data->length);

        /* Employ a trick here for avoiding cos(...) and sin(...) in time
         *       shifting.  We need to multiply each template frequency bin by
         *       exp(-J*twopit*deltaF*i) = exp(-J*twopit*deltaF*(i-1)) +
         *       exp(-J*twopit*deltaF*(i-1))*(exp(-J*twopit*deltaF) - 1) .  This
         *       recurrance relation has the advantage that the error growth is
         *       O(sqrt(N)) for N repetitions. */

        /* Values for the first iteration: */
        re = cos(twopit*deltaF*lower);
        im = -sin(twopit*deltaF*lower);

        /* Incremental values, using cos(theta) - 1 = -2*sin(theta/2)^2 */
        dim = -sin(twopit*deltaF);
        dre = -2.0*sin(0.5*twopit*deltaF)*sin(0.5*twopit*deltaF);

        for (i=lower; i<=upper; ++i){
            /* derive template (involving location/orientation parameters) from given plus/cross waveforms: */
            plainTemplateReal = FplusScaled * creal(dataPtr->freqModelhPlus->data->data[i]) 
                +  FcrossScaled * creal(dataPtr->freqModelhCross->data->data[i]);
            plainTemplateImag = FplusScaled * cimag(dataPtr->freqModelhPlus->data->data[i]) 
                +  FcrossScaled * cimag(dataPtr->freqModelhCross->data->data[i]);
            /* do time-shifting...             */
            /* (also un-do 1/deltaT scaling): */
            templateReal = (plainTemplateReal*re - plainTemplateImag*im) / deltaT;
            templateImag = (plainTemplateReal*im + plainTemplateImag*re) / deltaT;
            dataReal     = creal(dataPtr->freqData->data->data[i]) / deltaT;
            dataImag     = cimag(dataPtr->freqData->data->data[i])/ deltaT;

            REAL8 S_h = dataPtr->oneSidedNoisePowerSpectrum->data->data[i];

            S+=TwoDeltaToverN*((templateReal*templateReal)+(templateImag*templateImag)) / S_h;
            D+=TwoDeltaToverN*(dataReal*dataReal + dataImag*dataImag)/S_h;
            REAL8 dhstarRe=dataReal*templateReal+dataImag*templateImag; /* (-i^2=1) */
            REAL8 dhstarIm=dataImag*templateReal-dataReal*templateImag;

            Rre+=TwoDeltaToverN*dhstarRe/S_h;
            Rim+=TwoDeltaToverN*dhstarIm/S_h;

            /* Now update re and im for the next iteration. */
            newRe = re + re*dre - im*dim;
            newIm = im + re*dim + im*dre;

            re = newRe;
            im = newIm;
        }
        //printf("D: %.3e, S: %.3e, R: %.3e\n",D,S,R);
        dataPtr = dataPtr->next;
    }
    R=2.0*sqrt(Rre*Rre+Rim*Rim);
    gsl_sf_result result;
    REAL8 I0x=0.0;
    if(GSL_SUCCESS==gsl_sf_bessel_I0_scaled_e(R, &result))
    {
        I0x=result.val;
    }
    else printf("ERROR: Cannot calculate I0(%lf)\n",R);
    /* This is marginalised over phase only for now */
    REAL8 thislogL=-(S+D) + log(I0x) + R ;
    loglikeli=thislogL;
    LALInferenceClearVariables(&intrinsicParams);
    return(loglikeli);
}

REAL8 LALInferenceExtraPowerLogLikelihood(LALInferenceVariables *currentParams, LALInferenceIFOData * data,LALInferenceTemplateFunction templt)
/***************************************************************/
/* (log-) likelihood function.                                 */
/* Returns the non-normalised logarithmic likelihood.          */
/* Analytically marginalised over phase and distance           */
/* Attaches output variables for (h|h), (d|d) and (d|h^*)      */
/* At a distance of 1 Mpc for phi_0=0                          */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Required (`currentParams') parameters are:                  */
/*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
/*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
/*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
/*   - "time"            (REAL8, GPS sec.)                     */
/***************************************************************/
{
  //static int timeDomainWarning = 0;
  double Fplus, Fcross;
  double FplusScaled, FcrossScaled;
  double dataReal, dataImag;
  REAL8 loglikeli=0.0;
  REAL8 plainTemplateReal, plainTemplateImag;
  REAL8 templateReal, templateImag;
  int i, lower, upper;
  LALInferenceIFOData *dataPtr;
  /* XXX: setting distMpc=1.0 in case it's not used */
  //double ra, dec, psi, distMpc, gmst;
  double ra, dec, psi, amplitude=1.0, distMpc=1.0, gmst;
  double GPSdouble;
  LIGOTimeGPS GPSlal;
  double timedelay;  /* time delay b/w iterferometer & geocenter w.r.t. sky location */
  double timeshift;  /* time shift (not necessarily same as above)                   */
  double deltaT, TwoDeltaToverN, deltaF, twopit, re, im, dre, dim, newRe, newIm;
  double timeTmp;
  double mc;
  int different;
  int logDistFlag=0;
  
  LALStatus status;
  memset(&status,0,sizeof(status));
  LALInferenceVariables intrinsicParams;
  logDistFlag=LALInferenceCheckVariable(currentParams, "logdistance");
  if(LALInferenceCheckVariable(currentParams,"logmc")){
    mc=exp(*(REAL8 *)LALInferenceGetVariable(currentParams,"logmc"));
    LALInferenceAddVariable(currentParams,"chirpmass",&mc,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
  }
  
  /* determine source's sky location & orientation parameters: */
  ra        = *(REAL8*) LALInferenceGetVariable(currentParams, "rightascension"); /* radian      */
  dec       = *(REAL8*) LALInferenceGetVariable(currentParams, "declination");    /* radian      */
  psi       = *(REAL8*) LALInferenceGetVariable(currentParams, "polarisation");   /* radian      */
  GPSdouble = *(REAL8*) LALInferenceGetVariable(currentParams, "time");           /* GPS seconds */

  /* XXX: Burst hack to check for existence of 'distance'
   * parameters before trying to find their value */
	if(LALInferenceCheckVariable(currentParams,"distance") || LALInferenceCheckVariable(currentParams,"logdistance")){
		if(logDistFlag)
			distMpc = exp(*(REAL8*)LALInferenceGetVariable(currentParams,"logdistance"));
		else
			distMpc   = *(REAL8*) LALInferenceGetVariable(currentParams, "distance");       /* Mpc         */
	}
	if(LALInferenceCheckVariable(currentParams,"logamp")){
            amplitude   = exp(*(REAL8*) LALInferenceGetVariable(currentParams, "logamp"));
	}
  
  REAL8 phi0=0.0;

    /* figure out GMST: */
  XLALGPSSetREAL8(&GPSlal, GPSdouble);
  gmst=XLALGreenwichMeanSiderealTime(&GPSlal);
  
  /* Create parameter set to pass to the template function */
  intrinsicParams.head      = NULL;
  intrinsicParams.dimension = 0;
  LALInferenceCopyVariables(currentParams, &intrinsicParams);
  if(logDistFlag)
    LALInferenceRemoveVariable(&intrinsicParams, "logdistance");
  else
    LALInferenceRemoveVariable(&intrinsicParams, "distance");
  LALInferenceRemoveVariable(&intrinsicParams, "rightascension");
  LALInferenceRemoveVariable(&intrinsicParams, "declination");
  LALInferenceRemoveVariable(&intrinsicParams, "polarisation");
  LALInferenceRemoveVariable(&intrinsicParams, "time");
  LALInferenceRemoveVariable(&intrinsicParams, "phase");
  LALInferenceAddVariable(&intrinsicParams, "phase",&phi0,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
  //LALInferenceAddVariable(&intrinsicParams, "distance", &distMpc, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_OUTPUT);

  // TODO: add pointer to template function here.
    // (otherwise same parameters but different template will lead to no re-computation!!)
    

  /* Add an extra power burst with PSD slope f^-7/3, an amplitude powerburst_amp
  and upper termination frequency powerburst_fhigh */
  REAL8 burstamp=exp(*(REAL8 *)LALInferenceGetVariable(currentParams,"powerburst_logamp"));
  REAL8 burstfreq=*(REAL8 *)LALInferenceGetVariable(currentParams,"powerburst_fhigh");


    /* loop over data (different interferometers): */
    dataPtr = data;
    REAL8 S=0.,D=0.,R=0., Rre=0., Rim=0.;
    /* Need to compute S=h*h/S_h, D=d*d/S_h, R=h*d/S_h^2 */
    while (dataPtr != NULL) {
      /* The parameters the Likelihood function can handle by itself   */
      /* (and which shouldn't affect the template function) are        */
      /* sky location (ra, dec), polarisation and signal arrival time. */
      /* Note that the template function shifts the waveform to so that*/
      /* t_c corresponds to the "time" parameter in                    */
      /* IFOdata->modelParams (set, e.g., from the trigger value).     */
      
      /* Reset log-likelihood */
      dataPtr->loglikelihood = 0.0;
      
      /* Compare parameter values with parameter values corresponding  */
      /* to currently stored template; ignore "time" variable:         */
      if (LALInferenceCheckVariable(dataPtr->modelParams, "time")) {
	timeTmp = *(REAL8 *) LALInferenceGetVariable(dataPtr->modelParams, "time");
	LALInferenceRemoveVariable(dataPtr->modelParams, "time");
      }
      else timeTmp = GPSdouble;
      different = LALInferenceCompareVariables(dataPtr->modelParams, &intrinsicParams);
      /* "different" now may also mean that "dataPtr->modelParams" */
      /* wasn't allocated yet (as in the very 1st iteration).      */
      
      if (different) { /* template needs to be re-computed: */
	LALInferenceCopyVariables(&intrinsicParams, dataPtr->modelParams);
	LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
	templt(dataPtr);
	if(XLALGetBaseErrno()==XLAL_FAILURE) /* Template generation failed in a known way, set -Inf likelihood */
	  return(-DBL_MAX);
	
	if (dataPtr->modelDomain == LAL_SIM_DOMAIN_TIME) {
	  //	if (!timeDomainWarning) {
	  //	  timeDomainWarning = 1;
	  //	  fprintf(stderr, "WARNING: using time domain template with frequency domain likelihood (in %s, line %d)\n", __FILE__, __LINE__);
	  //	}
	  LALInferenceExecuteFT(dataPtr);
	  /* note that the dataPtr->modelParams "time" element may have changed here!! */
	  /* (during "template()" computation)  */
	}
    }
    else { /* no re-computation necessary. Return back "time" value, do nothing else: */
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
    }
    
    /*-- Template is now in dataPtr->freqModelhPlus and dataPtr->freqModelhCross. --*/
    /*-- (Either freshly computed or inherited.)                            --*/
    
    /* determine beam pattern response (F_plus and F_cross) for given Ifo: */
    XLALComputeDetAMResponse(&Fplus, &Fcross,
			    dataPtr->detector->response,
			    ra, dec, psi, gmst);
    /* signal arrival time (relative to geocenter); */
    timedelay = XLALTimeDelayFromEarthCenter(dataPtr->detector->location,
					    ra, dec, &GPSlal);
    /* (negative timedelay means signal arrives earlier at Ifo than at geocenter, etc.) */
    /* amount by which to time-shift template (not necessarily same as above "timedelay"): */
    timeshift =  (GPSdouble - (*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time"))) + timedelay;
    twopit    = LAL_TWOPI * timeshift;
    
    /* include distance (overall amplitude) effect in Fplus/Fcross: */
    FplusScaled  = Fplus  * amplitude / distMpc;
    FcrossScaled = Fcross * amplitude / distMpc;
    
    /* Check for wrong calibration sign */
    if (LALInferenceCheckVariable(currentParams, "crazyInjectionHLSign") &&
      *((INT4 *)LALInferenceGetVariable(currentParams, "crazyInjectionHLSign"))) {
	if (strstr(dataPtr->name, "H") || strstr(dataPtr->name, "L")) {
	  FplusScaled *= -1.0;
	  FcrossScaled *= -1.0;
      }
    }

dataPtr->fPlus = FplusScaled;
dataPtr->fCross = FcrossScaled;
dataPtr->timeshift = timeshift;

//FILE *testout=fopen("test_likeliLAL.txt","w");
//fprintf(testout, "f PSD dataRe dataIm signalRe signalIm\n");
/* determine frequency range & loop over frequency bins: */
deltaT = dataPtr->timeData->deltaT;
deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);
// printf("deltaF %g, Nt %d, deltaT %g\n", deltaF, dataPtr->timeData->data->length, dataPtr->timeData->deltaT);
lower = (UINT4)ceil(dataPtr->fLow / deltaF);
upper = (UINT4)floor(dataPtr->fHigh / deltaF);
TwoDeltaToverN = 2.0 * deltaT / ((double) dataPtr->timeData->data->length);

/* Employ a trick here for avoiding cos(...) and sin(...) in time
*       shifting.  We need to multiply each template frequency bin by
*       exp(-J*twopit*deltaF*i) = exp(-J*twopit*deltaF*(i-1)) +
*       exp(-J*twopit*deltaF*(i-1))*(exp(-J*twopit*deltaF) - 1) .  This
*       recurrance relation has the advantage that the error growth is
*       O(sqrt(N)) for N repetitions. */

/* Values for the first iteration: */
re = cos(twopit*deltaF*lower);
im = -sin(twopit*deltaF*lower);

/* Incremental values, using cos(theta) - 1 = -2*sin(theta/2)^2 */
dim = -sin(twopit*deltaF);
dre = -2.0*sin(0.5*twopit*deltaF)*sin(0.5*twopit*deltaF);

for (i=lower; i<=upper; ++i){
  /* derive template (involving location/orientation parameters) from given plus/cross waveforms: */
  plainTemplateReal = FplusScaled * creal(dataPtr->freqModelhPlus->data->data[i])  
  +  FcrossScaled * creal(dataPtr->freqModelhCross->data->data[i]);
  plainTemplateImag = FplusScaled * cimag(dataPtr->freqModelhPlus->data->data[i])
  +  FcrossScaled * cimag(dataPtr->freqModelhCross->data->data[i]);
  /* do time-shifting...             */
  /* (also un-do 1/deltaT scaling): */
  templateReal = (plainTemplateReal*re - plainTemplateImag*im) / deltaT;
  templateImag = (plainTemplateReal*im + plainTemplateImag*re) / deltaT;
  dataReal     = creal(dataPtr->freqData->data->data[i]) / deltaT;
  dataImag     = cimag(dataPtr->freqData->data->data[i])/ deltaT;
  
  REAL8 f=i*deltaF;

  REAL8 S_h = dataPtr->oneSidedNoisePowerSpectrum->data->data[i];
  
  if (f < burstfreq)
  {
    REAL8 fsq=f*f;
    REAL8 f4=fsq*fsq;
    REAL8 f7=f4*fsq*f;
    REAL8 f7by3=cbrt(f7);
    S_h += burstamp*burstamp / f7by3 ;
  }

  
  S+=TwoDeltaToverN*((templateReal*templateReal)+(templateImag*templateImag)) / S_h;
  D+=TwoDeltaToverN*(dataReal*dataReal + dataImag*dataImag)/S_h;
  REAL8 dhstarRe=dataReal*templateReal+dataImag*templateImag; /* (-i^2=1) */
  REAL8 dhstarIm=dataImag*templateReal-dataReal*templateImag;
  //REAL8 dhstar2=dhstarRe*dhstarRe + dhstarIm*dhstarIm;

  Rre+=TwoDeltaToverN*dhstarRe/S_h;
  Rim+=TwoDeltaToverN*dhstarIm/S_h;

  //R+=TwoDeltaToverN*(dataReal*dataReal+dataImag*dataImag)*(templateReal*templateReal+templateImag*templateImag)/S_h;

  //R+= sqrt(TwoDeltaToverN*TwoDeltaToverN*dhstar2 / (S_h*S_h));
  
  /* Now update re and im for the next iteration. */
  newRe = re + re*dre - im*dim;
  newIm = im + re*dim + im*dre;
  
  re = newRe;
  im = newIm;
}
//printf("D: %.3e, S: %.3e, R: %.3e\n",D,S,R);
dataPtr = dataPtr->next;
}
//LALInferenceAddVariable(currentParams,"D",&D,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
//LALInferenceAddVariable(currentParams,"S",&S,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
//LALInferenceAddVariable(currentParams,"R",&R,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
R=2.0*sqrt(Rre*Rre+Rim*Rim);
gsl_sf_result result;
REAL8 I0x=0.0;
if(GSL_SUCCESS==gsl_sf_bessel_I0_scaled_e(R, &result))
{
    I0x=result.val;
}
else printf("ERROR: Cannot calculate I0(%lf)\n",R);
/* This is marginalised over phase only for now */
REAL8 thislogL=-(S+D) + log(I0x) + R ;
//printf("%lf\n",logadd(0,R));
//REAL8 thislogL=log(0.5)-(S+D)+logadd(0.0,R);
//fclose(testout);
loglikeli=thislogL;
LALInferenceClearVariables(&intrinsicParams);
return(loglikeli);
}

REAL8 LALInferenceMarginalisedPhaseLogLikelihood_RD(LALInferenceVariables *currentParams, LALInferenceIFOData * data,LALInferenceTemplateFunction templt)
/***************************************************************/
/* (log-) likelihood function.                                 */
/* Returns the non-normalised logarithmic likelihood.          */
/* Analytically marginalised over phase and distance           */
/* Attaches output variables for (h|h), (d|d) and (d|h^*)      */
/* At a distance of 1 Mpc for phi_0=0                          */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Required (`currentParams') parameters are:                  */
/*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
/*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
/*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
/*   - "time"            (REAL8, GPS sec.)                     */
/***************************************************************/
{
	
  //static int timeDomainWarning = 0;
  double Fplus, Fcross;
  double FplusScaled, FcrossScaled;
  double dataReal, dataImag;
  REAL8 loglikeli=0.0;
  REAL8 plainTemplateReal, plainTemplateImag;
  REAL8 templateReal, templateImag;
  int i, lower, upper;
  LALInferenceIFOData *dataPtr;
  /* XXX: setting distMpc=1.0 in case it's not used */
  //double ra, dec, psi, distMpc, gmst;
  double ra, dec, psi, gmst;
  double amplitude=1.0, distMpc=1.0;
  //double hrss, frequency, Q;
  double GPSdouble;
  LIGOTimeGPS GPSlal;
  double timedelay;  /* time delay b/w iterferometer & geocenter w.r.t. sky location */
  double timeshift;  /* time shift (not necessarily same as above)                   */
  double deltaT, TwoDeltaToverN, deltaF, twopit, re, im, dre, dim, newRe, newIm;
  double timeTmp;
  double mc;
  int different;
  int logDistFlag=0;
  
  LALStatus status;
  memset(&status,0,sizeof(status));
  LALInferenceVariables intrinsicParams;
  logDistFlag=LALInferenceCheckVariable(currentParams, "logdistance");
  if(LALInferenceCheckVariable(currentParams,"logmc")){
    mc=exp(*(REAL8 *)LALInferenceGetVariable(currentParams,"logmc"));
    LALInferenceAddVariable(currentParams,"chirpmass",&mc,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
  }
  
  /* determine source's sky location & orientation parameters: */
  ra        = *(REAL8*) LALInferenceGetVariable(currentParams, "rightascension"); /* radian      */
  dec       = *(REAL8*) LALInferenceGetVariable(currentParams, "declination");    /* radian      */
  psi       = *(REAL8*) LALInferenceGetVariable(currentParams, "polarisation");   /* radian      */
  GPSdouble = *(REAL8*) LALInferenceGetVariable(currentParams, "time");           /* GPS seconds */

  /* XXX: Burst hack to check for existence of 'distance'
   * parameters before trying to find their value */
	if(LALInferenceCheckVariable(currentParams,"distance") || LALInferenceCheckVariable(currentParams,"logdistance")){
		if(logDistFlag)
			distMpc = exp(*(REAL8*)LALInferenceGetVariable(currentParams,"logdistance"));
		else
			distMpc   = *(REAL8*) LALInferenceGetVariable(currentParams, "distance");       /* Mpc         */
	}

//if(LALInferenceCheckVariable(currentParams,"loghrss")){
//        hrss = exp(*(REAL8*) LALInferenceGetVariable(currentParams, "loghrss"));
//        /* Now compute the peak amplitude from hrss, quality and frequency */
//        frequency = *(REAL8*) LALInferenceGetVariable(currentParams, "frequency");
//        Q = *(REAL8*) LALInferenceGetVariable(currentParams, "Q");
//        amplitude = 2.0*hrss*sqrt(LAL_SQRT2*LAL_PI*frequency/Q);
//}
  
  REAL8 phi0=0.0;
  
    /* figure out GMST: */
  XLALGPSSetREAL8(&GPSlal, GPSdouble);
  gmst=XLALGreenwichMeanSiderealTime(&GPSlal);
  
  /* Create parameter set to pass to the template function */
  intrinsicParams.head      = NULL;
  intrinsicParams.dimension = 0;
  LALInferenceCopyVariables(currentParams, &intrinsicParams);
  if(LALInferenceCheckVariable(currentParams,"distance") || LALInferenceCheckVariable(currentParams,"logdistance")){
      if(logDistFlag)
          LALInferenceRemoveVariable(&intrinsicParams, "logdistance");
      else
          LALInferenceRemoveVariable(&intrinsicParams, "distance");
  }
  LALInferenceRemoveVariable(&intrinsicParams, "rightascension");
  LALInferenceRemoveVariable(&intrinsicParams, "declination");
  LALInferenceRemoveVariable(&intrinsicParams, "polarisation");
  LALInferenceRemoveVariable(&intrinsicParams, "time");
  LALInferenceRemoveVariable(&intrinsicParams, "phase");
  LALInferenceAddVariable(&intrinsicParams, "phase",&phi0,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
  //LALInferenceAddVariable(&intrinsicParams, "distance", &distMpc, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_OUTPUT);

  // TODO: add pointer to template function here.
    // (otherwise same parameters but different template will lead to no re-computation!!)
    
    /* loop over data (different interferometers): */
    dataPtr = data;
    REAL8 S=0.,D=0.,R=0., Rre=0., Rim=0.;
    /* Need to compute S=h*h/S_h, D=d*d/S_h, R=h*d/S_h^2 */
    while (dataPtr != NULL) {
      /* The parameters the Likelihood function can handle by itself   */
      /* (and which shouldn't affect the template function) are        */
      /* sky location (ra, dec), polarisation and signal arrival time. */
      /* Note that the template function shifts the waveform to so that*/
      /* t_c corresponds to the "time" parameter in                    */
      /* IFOdata->modelParams (set, e.g., from the trigger value).     */
      
      /* Reset log-likelihood */
      dataPtr->loglikelihood = 0.0;
      
      /* Compare parameter values with parameter values corresponding  */
      /* to currently stored template; ignore "time" variable:         */
      if (LALInferenceCheckVariable(dataPtr->modelParams, "time")) {
	timeTmp = *(REAL8 *) LALInferenceGetVariable(dataPtr->modelParams, "time");
	LALInferenceRemoveVariable(dataPtr->modelParams, "time");
      }
      else timeTmp = GPSdouble;
      different = LALInferenceCompareVariables(dataPtr->modelParams, &intrinsicParams);
      /* "different" now may also mean that "dataPtr->modelParams" */
      /* wasn't allocated yet (as in the very 1st iteration).      */
      
      if (different) { /* template needs to be re-computed: */
	LALInferenceCopyVariables(&intrinsicParams, dataPtr->modelParams);
	LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
	templt(dataPtr);
	if(XLALGetBaseErrno()==XLAL_FAILURE) /* Template generation failed in a known way, set -Inf likelihood */
	  return(-DBL_MAX);
	
	if (dataPtr->modelDomain == LAL_SIM_DOMAIN_TIME) {
	  //	if (!timeDomainWarning) {
	  //	  timeDomainWarning = 1;
	  //	  fprintf(stderr, "WARNING: using time domain template with frequency domain likelihood (in %s, line %d)\n", __FILE__, __LINE__);
	  //	}
	  LALInferenceExecuteFT(dataPtr);
	  /* note that the dataPtr->modelParams "time" element may have changed here!! */
	  /* (during "template()" computation)  */
	}
    }
    else { /* no re-computation necessary. Return back "time" value, do nothing else: */
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
    }
    
    /*-- Template is now in dataPtr->freqModelhPlus and dataPtr->freqModelhCross. --*/
    /*-- (Either freshly computed or inherited.)                            --*/
    
    /* determine beam pattern response (F_plus and F_cross) for given Ifo: */
    XLALComputeDetAMResponse(&Fplus, &Fcross,
			    dataPtr->detector->response,
			    ra, dec, psi, gmst);
    /* signal arrival time (relative to geocenter); */
    timedelay = XLALTimeDelayFromEarthCenter(dataPtr->detector->location,
					    ra, dec, &GPSlal);
    /* (negative timedelay means signal arrives earlier at Ifo than at geocenter, etc.) */
    /* amount by which to time-shift template (not necessarily same as above "timedelay"): */
    timeshift =  (GPSdouble - (*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time"))) + timedelay;
    twopit    = LAL_TWOPI * timeshift;
    
    /* include distance (overall amplitude) effect in Fplus/Fcross: */
    FplusScaled  = amplitude * Fplus  / (distMpc);
    FcrossScaled = amplitude * Fcross / (distMpc);
    
    /* Check for wrong calibration sign */
    if (LALInferenceCheckVariable(currentParams, "crazyInjectionHLSign") &&
      *((INT4 *)LALInferenceGetVariable(currentParams, "crazyInjectionHLSign"))) {
	if (strstr(dataPtr->name, "H") || strstr(dataPtr->name, "L")) {
	  FplusScaled *= -1.0;
	  FcrossScaled *= -1.0;
      }
    }

dataPtr->fPlus = FplusScaled;
dataPtr->fCross = FcrossScaled;
dataPtr->timeshift = timeshift;

//FILE *testout=fopen("test_likeliLAL.txt","w");
//fprintf(testout, "f PSD dataRe dataIm signalRe signalIm\n");
/* determine frequency range & loop over frequency bins: */
deltaT = dataPtr->timeData->deltaT;
deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);
// printf("deltaF %g, Nt %d, deltaT %g\n", deltaF, dataPtr->timeData->data->length, dataPtr->timeData->deltaT);
lower = (UINT4)ceil(dataPtr->fLow / deltaF);
upper = (UINT4)floor(dataPtr->fHigh / deltaF);
TwoDeltaToverN = 2.0 * deltaT / ((double) dataPtr->timeData->data->length);

/* Employ a trick here for avoiding cos(...) and sin(...) in time
*       shifting.  We need to multiply each template frequency bin by
*       exp(-J*twopit*deltaF*i) = exp(-J*twopit*deltaF*(i-1)) +
*       exp(-J*twopit*deltaF*(i-1))*(exp(-J*twopit*deltaF) - 1) .  This
*       recurrance relation has the advantage that the error growth is
*       O(sqrt(N)) for N repetitions. */

/* Values for the first iteration: */
re = cos(twopit*deltaF*lower);
im = -sin(twopit*deltaF*lower);

/* Incremental values, using cos(theta) - 1 = -2*sin(theta/2)^2 */
dim = -sin(twopit*deltaF);
dre = -2.0*sin(0.5*twopit*deltaF)*sin(0.5*twopit*deltaF);

for (i=lower; i<=upper; ++i){
  /* derive template (involving location/orientation parameters) from given plus/cross waveforms: */
  plainTemplateReal = FplusScaled * creal(dataPtr->freqModelhPlus->data->data[i]) 
  +  FcrossScaled * creal(dataPtr->freqModelhCross->data->data[i]);
  plainTemplateImag = FplusScaled * cimag(dataPtr->freqModelhPlus->data->data[i]) 
  +  FcrossScaled * cimag(dataPtr->freqModelhCross->data->data[i]);
  /* do time-shifting...             */
  /* (also un-do 1/deltaT scaling): */
  templateReal = (plainTemplateReal*re - plainTemplateImag*im) / deltaT;
  templateImag = (plainTemplateReal*im + plainTemplateImag*re) / deltaT;
  dataReal     = creal(dataPtr->freqData->data->data[i]) / deltaT;
  dataImag     = cimag(dataPtr->freqData->data->data[i])/ deltaT;
  
  REAL8 S_h = dataPtr->oneSidedNoisePowerSpectrum->data->data[i];
  
  S+=TwoDeltaToverN*((templateReal*templateReal)+(templateImag*templateImag)) / S_h;
  D+=TwoDeltaToverN*(dataReal*dataReal + dataImag*dataImag)/S_h;
// printf("\n%e %e\n",dataReal,templateReal);
// printf("%e %e\n",dataImag,templateImag);
  REAL8 dhstarRe=dataReal*templateReal+dataImag*templateImag; /* (-i^2=1) */
  REAL8 dhstarIm=dataImag*templateReal-dataReal*templateImag;
  //REAL8 dhstar2=dhstarRe*dhstarRe + dhstarIm*dhstarIm;

  Rre+=TwoDeltaToverN*dhstarRe/S_h;
  Rim+=TwoDeltaToverN*dhstarIm/S_h;

  //R+=TwoDeltaToverN*(dataReal*dataReal+dataImag*dataImag)*(templateReal*templateReal+templateImag*templateImag)/S_h;

  //R+= sqrt(TwoDeltaToverN*TwoDeltaToverN*dhstar2 / (S_h*S_h));
  
  /* Now update re and im for the next iteration. */
  newRe = re + re*dre - im*dim;
  newIm = im + re*dim + im*dre;
  
  re = newRe;
  im = newIm;
}
//printf("D: %.3e, S: %.3e, R: %.3e\n",D,S,R);
dataPtr = dataPtr->next;
}
//LALInferenceAddVariable(currentParams,"D",&D,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
//LALInferenceAddVariable(currentParams,"S",&S,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
//LALInferenceAddVariable(currentParams,"R",&R,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
R=2.0*sqrt(Rre*Rre+Rim*Rim);
gsl_sf_result result;
REAL8 I0x=0.0;
if(GSL_SUCCESS==gsl_sf_bessel_I0_scaled_e(R, &result))
{
    I0x=result.val;
}
else printf("ERROR: Cannot calculate I0(%lf)\n",R);
/* This is marginalised over phase only for now */
REAL8 thislogL=-(S+D) + log(I0x) + R ;
//printf("%lf\n",logadd(0,R));
//REAL8 thislogL=log(0.5)-(S+D)+logadd(0.0,R);
//fclose(testout);
loglikeli=thislogL;
LALInferenceClearVariables(&intrinsicParams);
return(loglikeli);
}

REAL8 LALInferenceUndecomposedFreqDomainLogLikelihood_RD(LALInferenceVariables *currentParams, LALInferenceIFOData * data,                               LALInferenceTemplateFunction templt)
/***************************************************************/
/* (log-) likelihood function.                                 */
/* Returns the non-normalised logarithmic likelihood.          */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Required (`currentParams') parameters are:                  */
/*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
/*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
/*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
/*   - "q"        (REAL8, q, >0)                      */
/*   - "time"            (REAL8, GPS sec.)                     */
/*   - "f0"            (REAL8, Hz.)                     */
/*   - "hrss"            (REAL8, strain.)                     */
/*   - "time"            (REAL8, GPS sec.)                     */

/***************************************************************/
{
  //static int timeDomainWarning = 0;
  double Fplus, Fcross;
  double FplusScaled, FcrossScaled;
  double diffRe, diffIm, diffSquared;
  double dataReal, dataImag;
  REAL8 loglikeli;
  REAL8 plainTemplateReal, plainTemplateImag;
  REAL8 templateReal, templateImag;
  int i, lower, upper;
  LALInferenceIFOData *dataPtr;
  double ra, dec, psi;
  double GPSdouble,gmst;
  double amplitude=1.0;
  LIGOTimeGPS GPSlal;
  double chisquared;
  double timedelay;  /* time delay b/w iterferometer & geocenter w.r.t. sky location */
  double timeshift;  /* time shift (not necessarily same as above)                   */
  double deltaT, TwoDeltaToverN, deltaF, twopit, f, re, im;
  double timeTmp;
  int different;
  LALStatus status;
  memset(&status,0,sizeof(status));
  LALInferenceVariables intrinsicParams;
 // double norm=0;
  /* determine source's sky location & orientation parameters: */
  ra        = *(REAL8*) LALInferenceGetVariable(currentParams, "rightascension"); // radian      
  dec       = *(REAL8*) LALInferenceGetVariable(currentParams, "declination");    // radian      
  psi       = *(REAL8*) LALInferenceGetVariable(currentParams, "polarisation");   // radian      
  GPSdouble = *(REAL8*) LALInferenceGetVariable(currentParams, "time");           // GPS seconds 
    if(LALInferenceCheckVariable(currentParams,"logamp")){
            amplitude   = exp(*(REAL8*) LALInferenceGetVariable(currentParams, "logamp"));
	}
  
  /* figure out GMST: */
  //XLALINT8NSToGPS(&GPSlal, floor(1e9 * GPSdouble + 0.5));
  XLALGPSSetREAL8(&GPSlal, GPSdouble);
  //UandA.units    = MST_RAD;
  //UandA.accuracy = LALLEAPSEC_LOOSE;
  //LALGPStoGMST1(&status, &gmst, &GPSlal, &UandA);
  gmst=XLALGreenwichMeanSiderealTime(&GPSlal);
  intrinsicParams.head      = NULL;
  intrinsicParams.dimension = 0;
  LALInferenceCopyVariables(currentParams, &intrinsicParams);
  LALInferenceRemoveVariable(&intrinsicParams, "rightascension");
  LALInferenceRemoveVariable(&intrinsicParams, "declination");
  LALInferenceRemoveVariable(&intrinsicParams, "polarisation");
  LALInferenceRemoveVariable(&intrinsicParams, "time");

  // TODO: add pointer to template function here.
  // (otherwise same parameters but different template will lead to no re-computation!!)

  chisquared = 0.0;
  /* loop over data (different interferometers): */
  dataPtr = data;
REAL8 net_snr=0.0,ifo_snr=0.0;

  while (dataPtr != NULL) {
    ifo_snr=0.0;
    /* The parameters the Likelihood function can handle by itself   */
    /* (and which shouldn't affect the template function) are        */
    /* sky location (ra, dec), polarisation and signal arrival time. */
    /* Note that the template function shifts the waveform to so that*/
	/* t_c corresponds to the "time" parameter in                    */
	/* IFOdata->modelParams (set, e.g., from the trigger value).     */
    
    /* Reset log-likelihood */
    dataPtr->loglikelihood = 0.0;

    /* Compare parameter values with parameter values corresponding  */
    /* to currently stored template; ignore "time" variable:         */
    if (LALInferenceCheckVariable(dataPtr->modelParams, "time")) {
      timeTmp = *(REAL8 *) LALInferenceGetVariable(dataPtr->modelParams, "time");
      LALInferenceRemoveVariable(dataPtr->modelParams, "time");
    }
    else timeTmp = GPSdouble;
    different = LALInferenceCompareVariables(dataPtr->modelParams, &intrinsicParams);
    /* "different" now may also mean that "dataPtr->modelParams" */
    /* wasn't allocated yet (as in the very 1st iteration).      */

    if (different) { /* template needs to be re-computed: */
      //  printf("IM RECALCULATING THE TEMPLATE!\n");

      LALInferenceCopyVariables(&intrinsicParams, dataPtr->modelParams);
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
      //LALInferenceAddVariable(dataPtr->modelParams, "polarisation", &psi, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);

      templt(dataPtr);
      if(XLALGetBaseErrno()==XLAL_FAILURE) /* Template generation failed in a known way, set -Inf likelihood */
          return(-DBL_MAX);

          if (dataPtr->modelDomain == LAL_SIM_DOMAIN_TIME) {
    //	if (!timeDomainWarning) {
//	  timeDomainWarning = 1;
//	  fprintf(stderr, "WARNING: using time domain template with frequency domain likelihood (in %s, line %d)\n", __FILE__, __LINE__);
//	} 
//printf("Im doing the FFT in likelihood burst\n");
        LALInferenceExecuteFT(dataPtr);
       // norm=sqrt(dataPtr->window->data->length/dataPtr->window->sumofsquares);
        /* note that the dataPtr->modelParams "time" element may have changed here!! */
        /* (during "template()" computation)  */
      }
    }
    else { /* no re-computation necessary. Return back "time" value, do nothing else: */
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
    }

    /*-- Template is now in dataPtr->freqModelhPlus and dataPtr->freqModelhCross. --*/
    /*-- (Either freshly computed or inherited.)                            --*/

    /* determine beam pattern response (F_plus and F_cross) for given Ifo: */
    XLALComputeDetAMResponse(&Fplus, &Fcross,
                             dataPtr->detector->response,
			     ra, dec, psi, gmst);
    /* signal arrival time (relative to geocenter); */
    timedelay = XLALTimeDelayFromEarthCenter(dataPtr->detector->location,
                                             ra, dec, &GPSlal);
    /* (negative timedelay means signal arrives earlier at Ifo than at geocenter, etc.) */
    /* amount by which to time-shift template (not necessarily same as above "timedelay"): */
    timeshift =  (GPSdouble - (*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time"))) + timedelay;
  //timeshift = timedelay;
    twopit    = LAL_TWOPI * timeshift;

    /* include distance (overall amplitude) effect in Fplus/Fcross: */
    FplusScaled  = amplitude * Fplus;
    FcrossScaled = amplitude * Fcross;
    dataPtr->fPlus = FplusScaled;
    dataPtr->fCross = FcrossScaled;
    dataPtr->timeshift = timeshift;

 //sprintf(fname,"%s_template.dat",dataPtr->name);
 //FILE *testout=fopen(fname,"w");
/*
 for(i=0;i<(INT4) dataPtr->timeModelhPlus->data->length;i++){
     fprintf(testout,"%10.10e %10.10e %5.5f\n",(REAL8) i*dataPtr->timeData->deltaT, Fplus*dataPtr->timeModelhPlus->data->data[i]+Fcross*dataPtr->timeModelhCross->data->data[i],timeshift);
 }
 fclose(testout);*/
 //fprintf(testout, "f PSD dataRe dataIm signalRe signalIm\n");
    /* determine frequency range & loop over frequency bins: */
    deltaT = dataPtr->timeData->deltaT;
    deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);
    // printf("deltaF %g, Nt %d, deltaT %g\n", deltaF, dataPtr->timeData->data->length, dataPtr->timeData->deltaT);
    lower = (UINT4)ceil(dataPtr->fLow / deltaF);
    upper = (UINT4)floor(dataPtr->fHigh / deltaF);
    TwoDeltaToverN = 2.0 * deltaT / ((double) dataPtr->timeData->data->length);
    for (i=lower; i<=upper; ++i){
      /* derive template (involving location/orientation parameters) from given plus/cross waveforms: */
      plainTemplateReal = FplusScaled * creal(dataPtr->freqModelhPlus->data->data[i])  
                          +  FcrossScaled * creal(dataPtr->freqModelhCross->data->data[i]);
      plainTemplateImag = FplusScaled * cimag(dataPtr->freqModelhPlus->data->data[i]) 
                          +  FcrossScaled * cimag(dataPtr->freqModelhCross->data->data[i]);

      /* do time-shifting...             */
      /* (also un-do 1/deltaT scaling): */
      f = ((double) i) * deltaF;
      /* real & imag parts of  exp(-2*pi*i*f*deltaT): */
      re = cos(twopit * f);
      im = - sin(twopit * f);
      templateReal = (plainTemplateReal*re - plainTemplateImag*im) / deltaT; //salvo remove norm?
      templateImag = (plainTemplateReal*im + plainTemplateImag*re) / deltaT; //salvo remove norm?
      dataReal     = creal(dataPtr->freqData->data->data[i]) / deltaT;
      dataImag     = cimag(dataPtr->freqData->data->data[i]) / deltaT;
      /* compute squared difference & 'chi-squared': */
      diffRe       = dataReal - templateReal;         // Difference in real parts...
      diffIm       = dataImag - templateImag;         // ...and imaginary parts, and...
      diffSquared  = diffRe*diffRe + diffIm*diffIm ;  // ...squared difference of the 2 complex figures.
      REAL8 temp = ((TwoDeltaToverN * diffSquared) / dataPtr->oneSidedNoisePowerSpectrum->data->data[i]);
      ifo_snr+=TwoDeltaToverN*(templateReal*templateReal+templateImag*templateImag)/ dataPtr->oneSidedNoisePowerSpectrum->data->data[i];
      chisquared  += temp;
      dataPtr->loglikelihood -= temp;
 //fprintf(testout, "%e %e %e %e %e %e\n",     f, dataPtr->oneSidedNoisePowerSpectrum->data->data[i],          dataPtr->freqData->data->data[i].re, dataPtr->freqData->data->data[i].im,      templateReal, templateImag);
    }
    dataPtr = dataPtr->next;
    net_snr+=4.0*ifo_snr;
 //fclose(testout);
  }
  //exit(1);
  net_snr=sqrt(net_snr);
  //{
    //  printf("logl %10.10e \n",-1.0 * chisquared);
 // printf("Q %10.2e f %10.2e hrss %10.2e time %10.10f eccentricity %10.10e polangle %10.10e norm %f \n",q,f0,hrss,GPSdouble,eccentricity,polar_angle,norm);
//}
  
  loglikeli = -1.0 * chisquared; // note (again): the log-likelihood is unnormalised!
  LALInferenceClearVariables(&intrinsicParams);
  return(loglikeli);
}

REAL8 LALInferenceMarginalisedPhaseLogLikelihood(LALInferenceVariables *currentParams, LALInferenceIFOData * data,LALInferenceTemplateFunction templt)
/***************************************************************/
/* (log-) likelihood function.                                 */
/* Returns the non-normalised logarithmic likelihood.          */
/* Analytically marginalised over phase and distance           */
/* See LIGO-T1300326 for details                               */
/* At a distance of 1 Mpc for phi_0=0                          */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Required (`currentParams') parameters are:                  */
/*   - "rightascension"  (REAL8, radian, 0 <= RA <= 2pi)       */
/*   - "declination"     (REAL8, radian, -pi/2 <= dec <=pi/2)  */
/*   - "polarisation"    (REAL8, radian, 0 <= psi <= ?)        */
/*   - "time"            (REAL8, GPS sec.)                     */
/***************************************************************/
{
  //static int timeDomainWarning = 0;
  double Fplus, Fcross;
  double FplusScaled, FcrossScaled;
  double dataReal, dataImag;
  REAL8 loglikeli=0.0;
  REAL8 plainTemplateReal, plainTemplateImag;
  REAL8 templateReal, templateImag;
  int i, lower, upper;
  LALInferenceIFOData *dataPtr;
  double ra, dec, psi, distMpc, gmst;
  double GPSdouble;
  LIGOTimeGPS GPSlal;
  double timedelay;  /* time delay b/w iterferometer & geocenter w.r.t. sky location */
  double timeshift;  /* time shift (not necessarily same as above)                   */
  double deltaT, TwoDeltaToverN, deltaF, twopit, re, im, dre, dim, newRe, newIm;
  double timeTmp;
  double mc;
  int different;
  int logDistFlag=0;
  //noise model meta parameters
  gsl_matrix *lines   = NULL;//pointer to matrix holding line centroids
  gsl_matrix *widths  = NULL;//pointer to matrix holding line widths
  gsl_matrix *nparams = NULL;//pointer to matrix holding noise parameters
  
  gsl_matrix *psdBandsMin  = NULL;//pointer to matrix holding min frequencies for psd model
  gsl_matrix *psdBandsMax = NULL;//pointer to matrix holding max frequencies for psd model
  
  //double dflog = 1.0;        //logarithmic spacing of psd parameters
  
  int Nblock = 1;            //number of frequency blocks per IFO
  int Nlines = 1;            //number of lines to be removed
  int psdFlag = 0;           //flag for including psd fitting
  int lineFlag = 0;          //flag for excluding lines from integration
  int lineimin = 0;
  int lineimax = 0;
  int lineSwitch;          //switch inside integration to exclude bins
  
  //line removal parameters
  if(LALInferenceCheckVariable(currentParams, "removeLinesFlag"))
    lineFlag = *((INT4 *)LALInferenceGetVariable(currentParams, "removeLinesFlag"));
  if(lineFlag)
  {
    //Add line matrices to variable lists
    lines  = *(gsl_matrix **)LALInferenceGetVariable(currentParams, "line_center");
    widths = *(gsl_matrix **)LALInferenceGetVariable(currentParams, "line_width");
    Nlines = (int)lines->size2;
  }
  int lines_array[Nlines];
  int widths_array[Nlines];
  
  //check if psd parameters are included in the model
  if(LALInferenceCheckVariable(currentParams, "psdScaleFlag"))
    psdFlag = *((INT4 *)LALInferenceGetVariable(currentParams, "psdScaleFlag"));
  if(psdFlag)
  {
    //if so, store current noise parameters in easily accessible matrix
    nparams = *((gsl_matrix **)LALInferenceGetVariable(currentParams, "psdscale"));
    Nblock = (int)nparams->size2;
    
    //dflog = *(REAL8*) LALInferenceGetVariable(currentParams, "logdeltaf");
    
    psdBandsMin = *((gsl_matrix **)LALInferenceGetVariable(currentParams, "psdBandsMin"));
    psdBandsMax = *((gsl_matrix **)LALInferenceGetVariable(currentParams, "psdBandsMax"));
    
  }
  double alpha[Nblock];
  double lnalpha[Nblock];
  
  double psdBandsMin_array[Nblock];
  double psdBandsMax_array[Nblock];
  
  LALStatus status;
  memset(&status,0,sizeof(status));
  LALInferenceVariables intrinsicParams;
  logDistFlag=LALInferenceCheckVariable(currentParams, "logdistance");
  if(LALInferenceCheckVariable(currentParams,"logmc")){
    mc=exp(*(REAL8 *)LALInferenceGetVariable(currentParams,"logmc"));
    LALInferenceAddVariable(currentParams,"chirpmass",&mc,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
  }
  
  if(LALInferenceCheckVariable(currentParams,"LAL_AMPORDER") && LALInferenceCheckVariable(currentParams,"LAL_APPROXIMANT"))
  {
    INT4 apprx=*(INT4 *)LALInferenceGetVariable(currentParams,"LAL_APPROXIMANT");
    /* This is only valid at Newtonian amp order for most templates, and at all orders for
     * the F-domain templates listed here. This will need to be updated if the F-domain phase
     * handling changes. */
    if(!( (*(INT4 *)LALInferenceGetVariable(currentParams,"LAL_AMPORDER") == LAL_PNORDER_NEWTONIAN) \
      || (apprx == TaylorF2 || apprx==TaylorF2RedSpin || apprx==TaylorF2RedSpinTidal || 
          apprx==SpinTaylorF2 || apprx==IMRPhenomA || apprx==IMRPhenomB || apprx==IMRPhenomC) ))
    {
      XLALPrintError("Error: Cannot use non-Newtonian amplitude order and analytically marginalise over phase!\n");
      exit(1);
    }
  }
  
  /* determine source's sky location & orientation parameters: */
  ra        = *(REAL8*) LALInferenceGetVariable(currentParams, "rightascension"); /* radian      */
  dec       = *(REAL8*) LALInferenceGetVariable(currentParams, "declination");    /* radian      */
  psi       = *(REAL8*) LALInferenceGetVariable(currentParams, "polarisation");   /* radian      */
  GPSdouble = *(REAL8*) LALInferenceGetVariable(currentParams, "time");           /* GPS seconds */
  
  if(logDistFlag)
    distMpc = exp(*(REAL8*)LALInferenceGetVariable(currentParams,"logdistance"));
  else
    distMpc   = *(REAL8*) LALInferenceGetVariable(currentParams, "distance");       /* Mpc         */
  
  REAL8 phi0=0.0;
  
  /* figure out GMST: */
  XLALGPSSetREAL8(&GPSlal, GPSdouble);
  gmst=XLALGreenwichMeanSiderealTime(&GPSlal);
  
  /* Create parameter set to pass to the template function */
  intrinsicParams.head      = NULL;
  intrinsicParams.dimension = 0;
  LALInferenceCopyVariables(currentParams, &intrinsicParams);
  if(logDistFlag)
    LALInferenceRemoveVariable(&intrinsicParams, "logdistance");
  else
    LALInferenceRemoveVariable(&intrinsicParams, "distance");
  LALInferenceRemoveVariable(&intrinsicParams, "rightascension");
  LALInferenceRemoveVariable(&intrinsicParams, "declination");
  LALInferenceRemoveVariable(&intrinsicParams, "polarisation");
  LALInferenceRemoveVariable(&intrinsicParams, "time");
  LALInferenceRemoveVariable(&intrinsicParams, "phase");
  LALInferenceAddVariable(&intrinsicParams, "phase",&phi0,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_OUTPUT);
  //LALInferenceAddVariable(&intrinsicParams, "distance", &distMpc, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_OUTPUT);
  
  // TODO: add pointer to template function here.
  // (otherwise same parameters but different template will lead to no re-computation!!)
  
  /* loop over data (different interferometers): */
  dataPtr = data;
  UINT4 ifo=0;
  
  REAL8 S=0.,D=0.,R=0., Rre=0., Rim=0.;
  /* Need to compute S=h*h/S_h, D=d*d/S_h, R=h*d/S_h^2 */
  while (dataPtr != NULL) {
    /* The parameters the Likelihood function can handle by itself   */
    /* (and which shouldn't affect the template function) are        */
    /* sky location (ra, dec), polarisation and signal arrival time. */
    /* Note that the template function shifts the waveform to so that*/
    /* t_c corresponds to the "time" parameter in                    */
    /* IFOdata->modelParams (set, e.g., from the trigger value).     */
    
    /* Reset log-likelihood */
    dataPtr->loglikelihood = 0.0;
    
    /* Compare parameter values with parameter values corresponding  */
    /* to currently stored template; ignore "time" variable:         */
    if (LALInferenceCheckVariable(dataPtr->modelParams, "time")) {
      timeTmp = *(REAL8 *) LALInferenceGetVariable(dataPtr->modelParams, "time");
      LALInferenceRemoveVariable(dataPtr->modelParams, "time");
    }
    else timeTmp = GPSdouble;
    different = LALInferenceCompareVariables(dataPtr->modelParams, &intrinsicParams);
    /* "different" now may also mean that "dataPtr->modelParams" */
    /* wasn't allocated yet (as in the very 1st iteration).      */
    
    if (different) { /* template needs to be re-computed: */
      LALInferenceCopyVariables(&intrinsicParams, dataPtr->modelParams);
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
      templt(dataPtr);
      if(XLALGetBaseErrno()==XLAL_FAILURE) /* Template generation failed in a known way, set -Inf likelihood */
        return(-DBL_MAX);
      
      if (dataPtr->modelDomain == LAL_SIM_DOMAIN_TIME) {
        //	if (!timeDomainWarning) {
        //	  timeDomainWarning = 1;
        //	  fprintf(stderr, "WARNING: using time domain template with frequency domain likelihood (in %s, line %d)\n", __FILE__, __LINE__);
        //	}
        LALInferenceExecuteFT(dataPtr);
        /* note that the dataPtr->modelParams "time" element may have changed here!! */
        /* (during "template()" computation)  */
      }
    }
    else { /* no re-computation necessary. Return back "time" value, do nothing else: */
      LALInferenceAddVariable(dataPtr->modelParams, "time", &timeTmp, LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
    }
    
    /*-- Template is now in dataPtr->freqModelhPlus and dataPtr->freqModelhCross. --*/
    /*-- (Either freshly computed or inherited.)                            --*/
    
    /* determine beam pattern response (F_plus and F_cross) for given Ifo: */
    XLALComputeDetAMResponse(&Fplus, &Fcross,
                             dataPtr->detector->response,
                             ra, dec, psi, gmst);
    /* signal arrival time (relative to geocenter); */
    timedelay = XLALTimeDelayFromEarthCenter(dataPtr->detector->location,
                                             ra, dec, &GPSlal);
    /* (negative timedelay means signal arrives earlier at Ifo than at geocenter, etc.) */
    /* amount by which to time-shift template (not necessarily same as above "timedelay"): */
    timeshift =  (GPSdouble - (*(REAL8*) LALInferenceGetVariable(dataPtr->modelParams, "time"))) + timedelay;
    twopit    = LAL_TWOPI * timeshift;
    
    /* include distance (overall amplitude) effect in Fplus/Fcross: */
    FplusScaled  = Fplus  / distMpc;
    FcrossScaled = Fcross / distMpc;
    
    /* Check for wrong calibration sign */
    if (LALInferenceCheckVariable(currentParams, "crazyInjectionHLSign") &&
        *((INT4 *)LALInferenceGetVariable(currentParams, "crazyInjectionHLSign"))) {
      if (strstr(dataPtr->name, "H") || strstr(dataPtr->name, "L")) {
        FplusScaled *= -1.0;
        FcrossScaled *= -1.0;
      }
    }
    
    dataPtr->fPlus = FplusScaled;
    dataPtr->fCross = FcrossScaled;
    dataPtr->timeshift = timeshift;
    

    //Set up noise PSD meta parameters
    for(i=0; i<Nblock; i++)
    {
      if(psdFlag)
      {
	alpha[i]   = gsl_matrix_get(nparams,ifo,i);
	lnalpha[i] = log(alpha[i]);
	
	psdBandsMin_array[i] = gsl_matrix_get(psdBandsMin,ifo,i);
	psdBandsMax_array[i] = gsl_matrix_get(psdBandsMax,ifo,i);
	
      }
      else
      {
	alpha[i]=1.0;
	lnalpha[i]=0.0;
      }
    }
    
    //Set up psd line arrays
    for(INT4 j=0;j<Nlines;j++)
    {
      if(lineFlag)
      {
	
	//find range of fourier fourier bins which are excluded from integration
	lines_array[j]  = (int)gsl_matrix_get(lines,ifo,j);//lineimin = (int)(gsl_matrix_get(lines,ifo,j) - gsl_matrix_get(widths,ifo,j));
	widths_array[j] = (int)gsl_matrix_get(widths,ifo,j);//lineimax = (int)(gsl_matrix_get(lines,ifo,j) + gsl_matrix_get(widths,ifo,j));
      }
      else
      {
	lines_array[j]=0;
	widths_array[j]=0;
      }
    }
    //FILE *testout=fopen("test_likeliLAL.txt","w");
    //fprintf(testout, "f PSD dataRe dataIm signalRe signalIm\n");
    /* determine frequency range & loop over frequency bins: */
    deltaT = dataPtr->timeData->deltaT;
    deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);
    // printf("deltaF %g, Nt %d, deltaT %g\n", deltaF, dataPtr->timeData->data->length, dataPtr->timeData->deltaT);
    lower = (UINT4)ceil(dataPtr->fLow / deltaF);
    upper = (UINT4)floor(dataPtr->fHigh / deltaF);
    TwoDeltaToverN = 2.0 * deltaT / ((double) dataPtr->timeData->data->length);
    
    /* Employ a trick here for avoiding cos(...) and sin(...) in time
     *       shifting.  We need to multiply each template frequency bin by
     *       exp(-J*twopit*deltaF*i) = exp(-J*twopit*deltaF*(i-1)) +
     *       exp(-J*twopit*deltaF*(i-1))*(exp(-J*twopit*deltaF) - 1) .  This
     *       recurrance relation has the advantage that the error growth is
     *       O(sqrt(N)) for N repetitions. */
    
    /* Values for the first iteration: */
    re = cos(twopit*deltaF*lower);
    im = -sin(twopit*deltaF*lower);
    
    /* Incremental values, using cos(theta) - 1 = -2*sin(theta/2)^2 */
    dim = -sin(twopit*deltaF);
    dre = -2.0*sin(0.5*twopit*deltaF)*sin(0.5*twopit*deltaF);
    /* Loop over freq domain */
    for (i=lower; i<=upper; ++i){

      
      /* Remove lines from model */
      lineSwitch=1;
      
      if(lineFlag)
      {
	for(INT4 j=0;j<Nlines;j++)
	{
	  //find range of fourier fourier bins which are excluded from integration
	  lineimin = lines_array[j] - widths_array[j];//gsl_matrix_get(widths,ifo,j));
	  lineimax = lines_array[j] + widths_array[j];//(int)(gsl_matrix_get(lines,ifo,j) + gsl_matrix_get(widths,ifo,j));
	  
	  //if the current bin is inside the exluded region, set the switch to 0
	  if(i>lineimin && i<lineimax) lineSwitch=0;
	}
      }
      
      /*only sum over bins which are outside of excluded regions */
      if(lineSwitch)
      {
	
	/* derive template (involving location/orientation parameters) from given plus/cross waveforms: */
	plainTemplateReal = FplusScaled * creal(dataPtr->freqModelhPlus->data->data[i])
	+  FcrossScaled * creal(dataPtr->freqModelhCross->data->data[i]);
	plainTemplateImag = FplusScaled * cimag(dataPtr->freqModelhPlus->data->data[i])
	+  FcrossScaled * cimag(dataPtr->freqModelhCross->data->data[i]);
	/* do time-shifting...             */
	/* (also un-do 1/deltaT scaling): */
	templateReal = (plainTemplateReal*re - plainTemplateImag*im) / deltaT;
	templateImag = (plainTemplateReal*im + plainTemplateImag*re) / deltaT;
	dataReal     = creal(dataPtr->freqData->data->data[i]) / deltaT;
	dataImag     = cimag(dataPtr->freqData->data->data[i]) / deltaT;
	
	REAL8 S_h = dataPtr->oneSidedNoisePowerSpectrum->data->data[i];
	S+=TwoDeltaToverN*((templateReal*templateReal)+(templateImag*templateImag)) / S_h;
	D+=TwoDeltaToverN*(dataReal*dataReal + dataImag*dataImag)/S_h;
	REAL8 dhstarRe=dataReal*templateReal+dataImag*templateImag; /* (-i^2=1) */
	REAL8 dhstarIm=dataImag*templateReal-dataReal*templateImag;
	Rre+=TwoDeltaToverN*dhstarRe/S_h;
	Rim+=TwoDeltaToverN*dhstarIm/S_h;
	/* Add noise PSD parameters to the model */
	if(psdFlag)
	{
	  for(INT4 j=0; j<Nblock; j++)
	  {
	    if (i >= psdBandsMin_array[j] && i <= psdBandsMax_array[j])
	    {
	      S /= alpha[j];
	      S += lnalpha[j];
	      D /=alpha[j];
	      D+= lnalpha[j];
	      Rre /= alpha[j];
	      Rre += lnalpha[j];
	      Rim /= alpha[j];
	      Rim += lnalpha[j];
	    }
	  }
	}
      }

      /* Now update re and im for the next iteration. */
      newRe = re + re*dre - im*dim;
      newIm = im + re*dim + im*dre;
      
      re = newRe;
      im = newIm;
    }
    dataPtr = dataPtr->next;
  }
  R=2.0*sqrt(Rre*Rre+Rim*Rim);
  gsl_sf_result result;
  REAL8 I0x=0.0;
  if(GSL_SUCCESS==gsl_sf_bessel_I0_scaled_e(R, &result))
  {
    I0x=result.val;
  }
  else printf("ERROR: Cannot calculate I0(%lf)\n",R);
  /* This is marginalised over phase only for now */
  REAL8 thislogL=-(S+D) + log(I0x) + R ;
  loglikeli=thislogL;
  LALInferenceClearVariables(&intrinsicParams);
  return(loglikeli);
}

