#include <stdio.h>
#include <math.h>
#include "../include/common.h"

// Returns HEMT gain based on amp_type in MaterialConfig.
double amplifier_get_gain(const MaterialConfig *cfg) {
    if (cfg->amp_type == 0) {
        return GAAS_HEMT_GAIN;   // GaAs: 35 dB = x3162
    } else {
        return INP_HEMT_GAIN;    // InP:  38 dB = x6310
    }
}

// Returns HEMT noise temperature based on amp_type.
double amplifier_get_tn(const MaterialConfig *cfg) {
    if (cfg->amp_type == 0) {
        return GAAS_HEMT_TN;     // GaAs: 4.5K
    } else {
        return INP_HEMT_TN;      // InP:  2.8K
    }
}

// Computes total system noise temperature using Friis cascade.
// T_sys = T_HEMT + T_room / G_HEMT
// Room temperature amplifier contribution is divided by HEMT gain
// because HEMT already amplified signal before room-temp stage.
double amplifier_friis_tsys(const MaterialConfig *cfg) {
    double T_N  = amplifier_get_tn(cfg);
    double G    = amplifier_get_gain(cfg);
    return T_N + (T_ROOM_AMP / G);
}

// Applies gain to every sample in signal array.
// Multiplies each sample by gain factor.
// Both signal AND noise get amplified equally —
// gain alone never improves SNR.
void amplifier_apply_gain(double *signal, int n, double gain) {
    for (int i = 0; i < n; i++) {
        signal[i] *= gain;
    }
}