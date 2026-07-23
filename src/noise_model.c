#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "../include/common.h"

// Quantum vacuum noise sigma.
// Source: Heisenberg uncertainty -- resonator ground state has
// zero-point energy hbar*omega/2 even with zero photons.
// This is the Standard Quantum Limit -- cannot be removed.
double noise_quantum_sigma(const ResonatorConfig *rc) {
    return sqrt(HBAR * rc->omega_probe / 2.0);
}

// HEMT electronic noise sigma.
// Source: thermal agitation of electrons in HEMT at 4K.
double noise_hemt_sigma(const MaterialConfig *cfg) {
    double T_N = (cfg->amp_type == 0) ? GAAS_HEMT_TN : INP_HEMT_TN;
    return sqrt(KB * T_N * cfg->bandwidth_hz);
}

// Generates n Gaussian noise samples using Box-Muller transform.
// Box-Muller: Z = sqrt(-2*ln(U1)) * cos(2*pi*U2)
// Guaranteed finite -- loops until U1 > 0, catches non-finite Z.
void noise_generate(double *noise, int n, double sigma) {
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    for (int i = 0; i < n; i++) {
        double U1, U2, Z;
        int attempts = 0;
        do {
            U1 = (double)rand() / RAND_MAX;
            U2 = (double)rand() / RAND_MAX;
            attempts++;
        } while (U1 <= 0.0 && attempts < 100);

        if (U1 <= 0.0) U1 = 1e-10;  // safety fallback

        Z = sqrt(-2.0 * log(U1)) * cos(2.0 * PI * U2);

        // Safety catch -- should never trigger with above guard
        if (!isfinite(Z)) Z = 0.0;

        noise[i] = sigma * Z;
    }
}

// Adds noise array to signal in-place.
void noise_add(double *signal, const double *noise, int n) {
    for (int i = 0; i < n; i++) {
        signal[i] += noise[i];
    }
}