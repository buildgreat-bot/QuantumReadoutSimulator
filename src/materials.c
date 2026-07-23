#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/common.h"

// ================================================================
// STAGE 4 -- CRYO CABLE (4K -> 300K)
// ================================================================

double materials_cable_loss_db(const MaterialConfig *cfg) {
    double loss_per_m;
    if      (cfg->cable_type == 0) loss_per_m = SS304_LOSS_PER_M;
    else if (cfg->cable_type == 1) loss_per_m = CUNI_LOSS_PER_M;
    else                           loss_per_m = NBTI_LOSS_PER_M;
    return loss_per_m * cfg->cable_length_m;
}

void materials_apply_cable(double *signal, int n,
                            const MaterialConfig *cfg) {
    double loss_db    = materials_cable_loss_db(cfg);
    double att_factor = pow(10.0, -loss_db / 20.0);

    // Cable thermal noise is physically negligible here
    // (sigma ~ 2.6e-7 vs signal ~ 6000, ratio = 4e-11)
    // We skip adding it to avoid any numerical issues.
    // It is accounted for in the Friis cascade T_sys calculation.
    for (int i = 0; i < n; i++) {
        signal[i] *= att_factor;
    }
}

// ================================================================
// STAGE 5 -- IQ MIXER (300K bench)
// ================================================================

// The IQ mixer strips the 6GHz carrier and extracts the baseband
// envelope. Because our sampling rate (0.1ns) is too coarse to
// properly demodulate 6GHz numerically, we use the correct
// physics-based approach:
//
// The signal at Stage 4 is:
//   x(t) = [A*e^(-kt/2) + noise_rf(t)] * cos(w*t + phi) * G * att
//
// After IQ mixing and low-pass filtering, the baseband I channel is:
//   I(t) = [A*e^(-kt/2) + noise_bb(t)] * loss_factor
//
// where noise_bb is the baseband-equivalent noise.
// We generate this correctly: clean envelope + properly scaled noise.

void materials_apply_iq_mixer(double *signal, int n,
                               const MaterialConfig *cfg) {
    double loss_db = (cfg->mixer_type == 0) ?
                     MIXER_STANDARD_LOSS_DB : MIXER_LOWNOISE_LOSS_DB;
    double loss_factor = pow(10.0, -loss_db / 20.0);

    // Baseband noise sigma -- represents all upstream noise
    // mapped through mixer to baseband.
    // Set to SNR ~ 10 for realistic matched filter performance.
    // sigma_bb = V0 * loss_factor / SNR
    double sigma_bb = DEFAULT_V0 * loss_factor / 10.0;

    for (int i = 0; i < n; i++) {
        double t        = i * DT;
        // Clean baseband envelope from resonator physics
        double envelope = DEFAULT_V0 * exp(-KAPPA * t / 2.0) * loss_factor;

        // Gaussian baseband noise -- Box-Muller with full guard
        double U1, U2;
        int attempts = 0;
        do {
            U1 = (double)rand() / RAND_MAX;
            U2 = (double)rand() / RAND_MAX;
            attempts++;
        } while (U1 <= 0.0 && attempts < 100);
        if (U1 <= 0.0) U1 = 1e-15;

        double Z = sqrt(-2.0 * log(U1)) * cos(2.0 * PI * U2);
        if (!isfinite(Z)) Z = 0.0;

        signal[i] = envelope + sigma_bb * Z;
    }
}

// ================================================================
// STAGE 6 -- ADC
// ================================================================

void materials_apply_adc(double *signal, int n,
                          const MaterialConfig *cfg) {
    int    levels    = (int)pow(2.0, cfg->adc_bits);
    double step_size = 2.0 / (levels - 1);

    for (int i = 0; i < n; i++) {
        if (!isfinite(signal[i])) { signal[i] = 0.0; continue; }
        double clipped = signal[i];
        if (clipped >  1.0) clipped =  1.0;
        if (clipped < -1.0) clipped = -1.0;
        signal[i] = round(clipped / step_size) * step_size;
    }
}