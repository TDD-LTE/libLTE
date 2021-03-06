/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2014 The libLTE Developers. See the
 * COPYRIGHT file at the top-level directory of this distribution.
 *
 * \section LICENSE
 *
 * This file is part of the libLTE library.
 *
 * libLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */


#include <string.h>

#include "liblte/phy/utils/vector.h"
#include "liblte/phy/sync/sss.h"

void corr_all_zs(cf_t *z, cf_t s[N_SSS+1][N_SSS+1], cf_t *output) {
  uint32_t m;
  for (m = 0; m < N_SSS; m++) {
    output[m] = vec_dot_prod_ccc(z, s[m], N_SSS - 1);
  }
}


/* Assumes input points to the beginning of the SSS symbol. The SSS symbol start is
 * given by SSS_SYMBOL_ST() macro in sss.h.
 * Estimates the m0 and m1 values and saves in m0_value and m1_value
 * the resulted correlation (higher is more likely)
 *
 *
 * Source: "SSS Detection Method for Initial Cell Search in 3GPP LTE FDD/TDD Dual Mode Receiver"
 *       Jung-In Kim, Jung-Su Han, Hee-Jin Roh and Hyung-Jin Choi

 *
 */
int sss_synch_m0m1(sss_synch_t *q, cf_t *input, uint32_t *m0, float *m0_value,
    uint32_t *m1, float *m1_value) 
{

  int ret = LIBLTE_ERROR_INVALID_INPUTS; 

  if (q                 != NULL   &&
      input             != NULL   &&
      m0                != NULL   && 
      m1                != NULL)
  {
    
    /* Consider moving to sss_synch_t?? */
    cf_t zdelay[N_SSS+1],zconj[N_SSS+1],zprod[N_SSS+1];
    cf_t y[2][N_SSS+1], z[N_SSS+1], tmp[N_SSS+1];
    float tmp_real[N_SSS+1];
    cf_t input_fft[SSS_MAX_FFT_LEN];
    uint32_t i;

    dft_run_c(&q->dftp_input, input, input_fft);
    
    for (i = 0; i < N_SSS; i++) {
      y[0][i] = input_fft[q->fft_size/2-N_SSS + 2 * i];
      y[1][i] = input_fft[q->fft_size/2-N_SSS + 2 * i + 1];
    }
    
    vec_prod_ccc(y[0], q->fc_tables[q->N_id_2].c[0], z, N_SSS);
    memcpy(zdelay, &z[1], (N_SSS - 1) * sizeof(cf_t));
    vec_conj_cc(z, zconj, N_SSS - 1);
    vec_prod_ccc(zdelay, zconj, zprod, N_SSS - 1);

    corr_all_zs(zprod, q->fc_tables[q->N_id_2].s, tmp);
    vec_abs_cf(tmp, tmp_real, N_SSS);
    *m0 = vec_max_fi(tmp_real, N_SSS);
    if (m0_value) {
      *m0_value = tmp_real[*m0];
    }    

    vec_prod_ccc(y[1], q->fc_tables[q->N_id_2].c[1], tmp, N_SSS);
    vec_prod_ccc(tmp, q->fc_tables[q->N_id_2].z1[*m0], z, N_SSS);
    memcpy(zdelay, &z[1], (N_SSS - 1) * sizeof(cf_t));
    vec_conj_cc(z, zconj, N_SSS - 1);
    vec_prod_ccc(zdelay, zconj, zprod, N_SSS - 1);

    corr_all_zs(zprod, q->fc_tables[q->N_id_2].s, tmp);
    vec_abs_cf(tmp, tmp_real, N_SSS);
    *m1 = vec_max_fi(tmp_real, N_SSS);
    if (m1_value) {
      *m1_value = tmp_real[*m1];
    }    
    ret = LIBLTE_SUCCESS;
  } 
  return ret; 
}

void convert_tables(struct fc_tables *fc_tables, struct sss_tables *in) {
  uint32_t i, j;

  for (i = 0; i < N_SSS; i++) {
    for (j = 0; j < N_SSS; j++) {
      __real__ fc_tables->z1[i][j] = (float) in->z1[i][j];
    }
  }
  for (i = 0; i < N_SSS; i++) {
    for (j = 0; j < N_SSS - 1; j++) {
      __real__ fc_tables->s[i][j] = (float) in->s[i][j + 1] * in->s[i][j];
    }
  }
  for (i = 0; i < 2; i++) {
    for (j = 0; j < N_SSS; j++) {
      __real__ fc_tables->c[i][j] = (float) in->c[i][j];
    }
  }
}
