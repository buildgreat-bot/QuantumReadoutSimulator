#include <stdio.h>
#include <math.h>
#include "../include/common.h"

// Generates the full physical signal:
// s(t) = A * e^(-kappa*t/2) * cos(omega_probe*t + phi_qubit)
// This is what comes out of the resonator — oscillating at 6GHz
// while the envelope decays at rate kappa.
// phi is PHI_STATE_0 for |0> or PHI_STATE_1 for |1>
void signal_generate(double *signal, double A, double phi) {
    for (int i = 0; i < N_SAMPLES; i++) {
        double t = i * DT;

        // Exponential decay envelope from resonator ring-down
        double envelope = A * exp(-KAPPA * t / 2.0);

        // Full oscillating signal: envelope * carrier * phase
        signal[i] = envelope * cos(OMEGA_PROBE * t + phi);
    }
}

// Saves signal to CSV: two columns — time(s) and signal value
void signal_save_csv(const double *signal, int n, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        printf("ERROR: Could not open %s\n", filename);
        return;
    }
    fprintf(f, "time_s,signal_value\n");
    for (int i = 0; i < N_SAMPLES; i++) {
        double t = i * DT;
        fprintf(f, "%.12f,%.12f\n", t, signal[i]);
    }
    fclose(f);
}