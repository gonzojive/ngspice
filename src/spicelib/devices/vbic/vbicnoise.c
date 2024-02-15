/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1987 Gary W. Ng
Model Author: 1995 Colin McAndrew Motorola
Spice3 Implementation: 2003 Dietmar Warning DAnalyse GmbH
**********/

#include "ngspice/ngspice.h"
#include "vbicdefs.h"
#include "ngspice/cktdefs.h"
#include "ngspice/iferrmsg.h"
#include "ngspice/noisedef.h"
#include "ngspice/suffix.h"

/*
 * VBICnoise (mode, operation, firstModel, ckt, data, OnDens)
 *
 *    This routine names and evaluates all of the noise sources
 *    associated with VBIC's.  It starts with the model *firstModel and
 *    traverses all of its insts.  It then proceeds to any other models
 *    on the linked list.  The total output noise density generated by
 *    all of the VBIC's is summed with the variable "OnDens".
 */


int
VBICnoise (int mode, int operation, GENmodel *genmodel, CKTcircuit *ckt, Ndata *data, double *OnDens)
{
    NOISEAN *job = (NOISEAN *) ckt->CKTcurJob;

    VBICmodel *firstModel = (VBICmodel *) genmodel;
    VBICmodel *model;
    VBICinstance *inst;
    double tempOnoise;
    double tempInoise;
    double noizDens[VBICNSRCS];
    double lnNdens[VBICNSRCS];
    int i;

    /* define the names of the noise sources */

    static char *VBICnNames[VBICNSRCS] = {
        /* Note that we have to keep the order consistent with the
          strchr definitions in VBICdefs.h */
        "_rc",              /* noise due to rc */
        "_rci",             /* noise due to rci */
        "_rb",              /* noise due to rb */
        "_rbi",             /* noise due to rbi */
        "_re",              /* noise due to re */
        "_rbp",             /* noise due to rbp */
        "_rs",              /* noise due to rs */
        "_ic",              /* noise due to ic */
        "_ib",              /* noise due to ib */
        "_ibep",            /* noise due to ibep */
        "_iccp",            /* noise due to iccp */
        "_1overfbe",        /* flicker (1/f) noise ibe */
        "_1overfbep",       /* flicker (1/f) noise ibep */
        ""                  /* total transistor noise */
    };

    for (model=firstModel; model != NULL; model=VBICnextModel(model)) {
        for (inst=VBICinstances(model); inst != NULL;
                inst=VBICnextInstance(inst)) {

            switch (operation) {

            case N_OPEN:

                /* see if we have to to produce a summary report */
                /* if so, name all the noise generators */

                if (job->NStpsSm != 0) {
                    switch (mode) {

                    case N_DENS:
                        for (i=0; i < VBICNSRCS; i++) {
                            NOISE_ADD_OUTVAR(ckt, data, "onoise_%s%s", inst->VBICname, VBICnNames[i]);
                        }
                        break;

                    case INT_NOIZ:
                        for (i=0; i < VBICNSRCS; i++) {
                            NOISE_ADD_OUTVAR(ckt, data, "onoise_total_%s%s", inst->VBICname, VBICnNames[i]);
                            NOISE_ADD_OUTVAR(ckt, data, "inoise_total_%s%s", inst->VBICname, VBICnNames[i]);
                        }
                        break;
                    }
                }
                break;

            case N_CALC:
                switch (mode) {

                case N_DENS:
                    double dtemp;
                    if (inst->VBICtempGiven)
                        dtemp = inst->VBICtemp - ckt->CKTtemp + (model->VBICtnom-CONSTCtoK);
                    else
                        dtemp = inst->VBICdtemp;

                    NevalSrcInstanceTemp(&noizDens[VBICRCNOIZ],&lnNdens[VBICRCNOIZ],
                                 ckt,THERMNOISE,inst->VBICcollCXNode,inst->VBICcollNode,
                                 *(ckt->CKTstate0 + inst->VBICircx_Vrcx), dtemp);

                    NevalSrcInstanceTemp(&noizDens[VBICRCINOIZ],&lnNdens[VBICRCINOIZ],
                                 ckt,THERMNOISE,inst->VBICcollCXNode,inst->VBICcollCINode,
                                 *(ckt->CKTstate0 + inst->VBICirci_Vrci), dtemp);

                    NevalSrcInstanceTemp(&noizDens[VBICRBNOIZ],&lnNdens[VBICRBNOIZ],
                                 ckt,THERMNOISE,inst->VBICbaseBXNode,inst->VBICbaseNode,
                                 *(ckt->CKTstate0 + inst->VBICirbx_Vrbx), dtemp);

                    NevalSrcInstanceTemp(&noizDens[VBICRBINOIZ],&lnNdens[VBICRBINOIZ],
                                 ckt,THERMNOISE,inst->VBICbaseBXNode,inst->VBICbaseBINode,
                                 *(ckt->CKTstate0 + inst->VBICirbi_Vrbi), dtemp);

                    NevalSrcInstanceTemp(&noizDens[VBICRENOIZ],&lnNdens[VBICRENOIZ],
                                 ckt,THERMNOISE,inst->VBICemitEINode,inst->VBICemitNode,
                                 *(ckt->CKTstate0 + inst->VBICire_Vre), dtemp);

                    NevalSrcInstanceTemp(&noizDens[VBICRBPNOIZ],&lnNdens[VBICRBPNOIZ],
                                 ckt,THERMNOISE,inst->VBICemitEINode,inst->VBICemitNode,
                                 *(ckt->CKTstate0 + inst->VBICirbp_Vrbp), dtemp);

                    NevalSrcInstanceTemp(&noizDens[VBICRSNOIZ],&lnNdens[VBICRSNOIZ],
                                 ckt,THERMNOISE,inst->VBICsubsSINode,inst->VBICsubsNode,
                                 *(ckt->CKTstate0 + inst->VBICirs_Vrs), dtemp);


                    NevalSrc(&noizDens[VBICICNOIZ],&lnNdens[VBICICNOIZ],
                                 ckt,SHOTNOISE,inst->VBICcollCINode, inst->VBICemitEINode,
                                 *(ckt->CKTstate0 + inst->VBICitzf));

                    NevalSrc(&noizDens[VBICIBNOIZ],&lnNdens[VBICIBNOIZ],
                                 ckt,SHOTNOISE,inst->VBICbaseBINode, inst->VBICemitEINode,
                                 *(ckt->CKTstate0 + inst->VBICibe));

                    NevalSrc(&noizDens[VBICIBEPNOIZ],&lnNdens[VBICIBEPNOIZ],
                                 ckt,SHOTNOISE,inst->VBICbaseBXNode, inst->VBICbaseBPNode,
                                 *(ckt->CKTstate0 + inst->VBICibep));

                    NevalSrc(&noizDens[VBICICCPNOIZ],&lnNdens[VBICICCPNOIZ],
                                 ckt,SHOTNOISE,inst->VBICbaseBXNode, inst->VBICsubsSINode,
                                 *(ckt->CKTstate0 + inst->VBICiccp));


                    NevalSrc(&noizDens[VBICFLBENOIZ], NULL, ckt,
                                 N_GAIN,inst->VBICbaseBINode, inst->VBICemitEINode,
                                 (double)0.0);
                    noizDens[VBICFLBENOIZ] *= inst->VBICm * model->VBICfNcoef * 
                                 exp(model->VBICfNexpA *
                                 log(MAX(fabs(*(ckt->CKTstate0 + inst->VBICibe)/inst->VBICm),N_MINLOG))) /
                                 pow(data->freq, model->VBICfNexpB);
                    lnNdens[VBICFLBENOIZ] = 
                                 log(MAX(noizDens[VBICFLBENOIZ],N_MINLOG));

                    NevalSrc(&noizDens[VBICFLBEPNOIZ], NULL, ckt,
                                 N_GAIN,inst->VBICbaseBXNode, inst->VBICbaseBPNode,
                                 (double)0.0);
                    noizDens[VBICFLBEPNOIZ] *= inst->VBICm * model->VBICfNcoef * 
                                 exp(model->VBICfNexpA *
                                 log(MAX(fabs(*(ckt->CKTstate0 + inst->VBICibep)/inst->VBICm),N_MINLOG))) /
                                 pow(data->freq, model->VBICfNexpB);
                    lnNdens[VBICFLBEPNOIZ] = 
                                 log(MAX(noizDens[VBICFLBEPNOIZ],N_MINLOG));


                    noizDens[VBICTOTNOIZ] = noizDens[VBICRCNOIZ] +
                                            noizDens[VBICRCINOIZ] +
                                            noizDens[VBICRBNOIZ] +
                                            noizDens[VBICRBINOIZ] +
                                            noizDens[VBICRENOIZ] +
                                            noizDens[VBICRBPNOIZ] +
                                            noizDens[VBICICNOIZ] +
                                            noizDens[VBICIBNOIZ] +
                                            noizDens[VBICIBEPNOIZ] +
                                            noizDens[VBICFLBENOIZ] +
                                            noizDens[VBICFLBEPNOIZ];

                    lnNdens[VBICTOTNOIZ] = 
                                 log(noizDens[VBICTOTNOIZ]);

                    *OnDens += noizDens[VBICTOTNOIZ];

                    if (data->delFreq == 0.0) { 

                        /* if we haven't done any previous integration, we need to */
                        /* initialize our "history" variables                      */

                        for (i=0; i < VBICNSRCS; i++) {
                            inst->VBICnVar[LNLSTDENS][i] = lnNdens[i];
                        }

                        /* clear out our integration variables if it's the first pass */

                        if (data->freq == job->NstartFreq) {
                            for (i=0; i < VBICNSRCS; i++) {
                                inst->VBICnVar[OUTNOIZ][i] = 0.0;
                                inst->VBICnVar[INNOIZ][i] = 0.0;
                            }
                        }
                    } else {   /* data->delFreq != 0.0 (we have to integrate) */

/* In order to get the best curve fit, we have to integrate each component separately */

                        for (i=0; i < VBICNSRCS; i++) {
                            if (i != VBICTOTNOIZ) {
                                tempOnoise = Nintegrate(noizDens[i], lnNdens[i],
                                      inst->VBICnVar[LNLSTDENS][i], data);
                                tempInoise = Nintegrate(noizDens[i] * data->GainSqInv ,
                                      lnNdens[i] + data->lnGainInv,
                                      inst->VBICnVar[LNLSTDENS][i] + data->lnGainInv,
                                      data);
                                inst->VBICnVar[LNLSTDENS][i] = lnNdens[i];
                                data->outNoiz += tempOnoise;
                                data->inNoise += tempInoise;
                                if (job->NStpsSm != 0) {
                                    inst->VBICnVar[OUTNOIZ][i] += tempOnoise;
                                    inst->VBICnVar[OUTNOIZ][VBICTOTNOIZ] += tempOnoise;
                                    inst->VBICnVar[INNOIZ][i] += tempInoise;
                                    inst->VBICnVar[INNOIZ][VBICTOTNOIZ] += tempInoise;
                                }
                            }
                        }
                    }
                    if (data->prtSummary) {
                        for (i=0; i < VBICNSRCS; i++) {     /* print a summary report */
                            data->outpVector[data->outNumber++] = noizDens[i];
                        }
                    }
                    break;

                case INT_NOIZ:        /* already calculated, just output */
                    if (job->NStpsSm != 0) {
                        for (i=0; i < VBICNSRCS; i++) {
                            data->outpVector[data->outNumber++] = inst->VBICnVar[OUTNOIZ][i];
                            data->outpVector[data->outNumber++] = inst->VBICnVar[INNOIZ][i];
                        }
                    }    /* if */
                    break;
                }    /* switch (mode) */
                break;

            case N_CLOSE:
                return (OK);         /* do nothing, the main calling routine will close */
                break;               /* the plots */
            }    /* switch (operation) */
        }    /* for inst */
    }    /* for model */

return(OK);
}
