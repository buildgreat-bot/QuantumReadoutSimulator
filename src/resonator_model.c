#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../include/common.h"

// ================================================================
// GLOBAL RUNTIME PARAMETERS
// ================================================================
double OMEGA_PROBE;
double OMEGA_R;
double OMEGA_Q;
double KAPPA;
double CHI;
double G_COUPLING;
double KAPPA_EXT;
double V0;
double DT;
int    N_SAMPLES;
int    QUBIT_STATE;
double TAU;
double DELTA;
double PHI_0;
double PHI_1;
double DELTA_PHI;

// ================================================================
// resonator_init_defaults()
// ================================================================
void resonator_init_defaults(ResonatorConfig *rc, int qubit_state) {
    rc->omega_probe  = OMEGA_PROBE;
    rc->omega_r      = OMEGA_R;
    rc->omega_q      = OMEGA_Q;
    rc->g            = G_COUPLING;
    rc->kappa        = KAPPA;
    rc->kappa_ext    = KAPPA_EXT;
    rc->kappa_int    = KAPPA - KAPPA_EXT;
    rc->V0           = V0;
    rc->qubit_state  = qubit_state;
    rc->use_numerical = 0;   // default: analytic (safe)
    rc->dt           = DT;
    rc->n_samples    = N_SAMPLES;
    rc->chi          = CHI;
}

// ================================================================
// resonator_compute_derived()
//
// PHYSICS:
// Jaynes-Cummings Hamiltonian:
//   H_JC = hbar*w_r*(a_dag*a + 1/2) + hbar*w_q/2*sigma_z
//          + hbar*g*(a_dag*sigma_- + a*sigma_+)
//
// Dispersive regime (Schrieffer-Wolff transform):
//   chi = g^2 / Delta,  Delta = w_q - w_r
//
// Effective resonator frequencies:
//   |0>: w_r_eff = w_r + chi
//   |1>: w_r_eff = w_r - chi
//
// Phase imprinting (Input-Output theory):
//   phi_0 = +arctan(2*chi/kappa)
//   phi_1 = -arctan(2*chi/kappa)
// ================================================================
void resonator_compute_derived(ResonatorConfig *rc) {
    rc->delta = rc->omega_q - rc->omega_r;

    // Use CLI-provided chi if set, else derive from g and delta
    if (rc->chi <= 0.0 && fabs(rc->delta) > 0.0) {
        rc->chi = (rc->g * rc->g) / fabs(rc->delta);
    }

    rc->tau      = 2.0 / rc->kappa;
    rc->omega_r_0 = rc->omega_r + rc->chi;
    rc->omega_r_1 = rc->omega_r - rc->chi;
    rc->delta_eff_0 = rc->omega_probe - rc->omega_r_0;
    rc->delta_eff_1 = rc->omega_probe - rc->omega_r_1;

    // Phase shifts
    PHI_0     = +atan(2.0 * rc->chi / rc->kappa);
    PHI_1     = -atan(2.0 * rc->chi / rc->kappa);
    DELTA_PHI =  2.0 * atan(2.0 * rc->chi / rc->kappa);
    TAU       = rc->tau;

    rc->phi_qubit = (rc->qubit_state == 0) ? PHI_0 : PHI_1;

    // Steady-state intracavity field amplitude
    // |<alpha>_ss| = sqrt(kappa_ext) / sqrt(delta_eff^2 + (kappa/2)^2)
    double kh = rc->kappa / 2.0;
    double de0 = rc->delta_eff_0;
    double de1 = rc->delta_eff_1;
    rc->alpha_ss_0 = sqrt(rc->kappa_ext) / sqrt(de0*de0 + kh*kh);
    rc->alpha_ss_1 = sqrt(rc->kappa_ext) / sqrt(de1*de1 + kh*kh);
}

// ================================================================
// resonator_print_full_physics()
// Textbook-quality physics explanation for Stage 1.
// ================================================================
void resonator_print_full_physics(const ResonatorConfig *rc) {
    printf("\n");
    printf("  +============================================================+\n");
    printf("  |   STAGE 1 -- RESONATOR EXIT                               |\n");
    printf("  |   Location: 10-20 mK (on chip with qubit)                 |\n");
    printf("  +============================================================+\n");

    printf("  |\n");
    printf("  |  [1] SYSTEM HAMILTONIAN -- JAYNES-CUMMINGS MODEL\n");
    printf("  |\n");
    printf("  |  H_JC = hbar*w_r*(a_dag*a + 1/2) + hbar*w_q/2*sigma_z\n");
    printf("  |         + hbar*g*(a_dag*sigma_- + a*sigma_+)\n");
    printf("  |\n");
    printf("  |  where:\n");
    printf("  |    w_r = resonator frequency = %.4f GHz\n",
           rc->omega_r/(2.0*PI)/1e9);
    printf("  |    w_q = qubit frequency     = %.4f GHz\n",
           rc->omega_q/(2.0*PI)/1e9);
    printf("  |    g   = coupling strength   = %.4f MHz\n",
           rc->g/(2.0*PI)/1e6);
    printf("  |    a_dag, a = photon creation/annihilation operators\n");
    printf("  |    sigma_z, sigma_+/- = qubit Pauli operators\n");
    printf("  |\n");

    printf("  +------------------------------------------------------------+\n");
    printf("  |\n");
    printf("  |  [2] DISPERSIVE REGIME (|Delta| >> g)\n");
    printf("  |\n");
    printf("  |  Schrieffer-Wolff transform gives effective Hamiltonian:\n");
    printf("  |    H_disp = hbar*(w_r + chi*sigma_z)*a_dag*a + ...\n");
    printf("  |\n");
    printf("  |  Dispersive shift: chi = g^2 / Delta\n");
    printf("  |    Delta = w_q - w_r = %.4f GHz\n",
           rc->delta/(2.0*PI)/1e9);
    printf("  |    chi = (%.1f MHz)^2 / (%.4f GHz) = %.4f MHz\n",
           rc->g/(2.0*PI)/1e6,
           rc->delta/(2.0*PI)/1e9,
           rc->chi/(2.0*PI)/1e6);
    printf("  |\n");
    double ratio = fabs(rc->delta)/rc->g;
    printf("  |  VALIDITY: |Delta|/g = %.1f >> 1  [%s]\n",
           ratio, ratio > 10.0 ? "VALID" : "WEAK -- use caution");
    printf("  |\n");

    printf("  +------------------------------------------------------------+\n");
    printf("  |\n");
    printf("  |  [3] EFFECTIVE RESONATOR FREQUENCIES\n");
    printf("  |\n");
    printf("  |    |0> (sigma_z=+1): w_r_eff = w_r + chi = %.6f GHz\n",
           rc->omega_r_0/(2.0*PI)/1e9);
    printf("  |    |1> (sigma_z=-1): w_r_eff = w_r - chi = %.6f GHz\n",
           rc->omega_r_1/(2.0*PI)/1e9);
    printf("  |    Frequency separation = 2*chi = %.4f MHz\n",
           2.0*rc->chi/(2.0*PI)/1e6);
    printf("  |\n");
    printf("  |  Probe detunings:\n");
    printf("  |    Delta_eff(|0>) = %.4f MHz\n",
           rc->delta_eff_0/(2.0*PI)/1e6);
    printf("  |    Delta_eff(|1>) = %.4f MHz\n",
           rc->delta_eff_1/(2.0*PI)/1e6);
    printf("  |\n");

    printf("  +------------------------------------------------------------+\n");
    printf("  |\n");
    printf("  |  [4] PHASE IMPRINTING (Input-Output Theory)\n");
    printf("  |\n");
    printf("  |  Reflection coefficient:\n");
    printf("  |    Gamma(Delta) = (i*Delta - kappa/2) / (i*Delta + kappa/2)\n");
    printf("  |\n");
    printf("  |  Phase acquired by reflected probe:\n");
    printf("  |    phi_0 = +arctan(2*chi/kappa) = +%.4f rad = +%.2f deg\n",
           PHI_0, PHI_0*180.0/PI);
    printf("  |    phi_1 = -arctan(2*chi/kappa) = -%.4f rad = -%.2f deg\n",
           fabs(PHI_1), fabs(PHI_1)*180.0/PI);
    printf("  |    Delta_phi = %.4f rad = %.2f deg (total separation)\n",
           DELTA_PHI, DELTA_PHI*180.0/PI);
    printf("  |\n");
    printf("  |  Active qubit state : |%d>\n", rc->qubit_state);
    printf("  |  Phase used         : %+.4f rad (%+.2f deg)\n",
           rc->phi_qubit, rc->phi_qubit*180.0/PI);
    printf("  |\n");

    printf("  +------------------------------------------------------------+\n");
    printf("  |\n");
    printf("  |  [5] CAVITY DYNAMICS -- RING-DOWN EQUATION\n");
    printf("  |\n");
    printf("  |  d<alpha>/dt = -(i*w_r_eff + kappa/2)*<alpha>\n");
    printf("  |                + sqrt(kappa_ext)*alpha_in(t)\n");
    printf("  |\n");
    printf("  |  Output field:\n");
    printf("  |    alpha_out(t) = sqrt(kappa_ext)*<alpha>(t) - alpha_in(t)\n");
    printf("  |\n");
    printf("  |  Parameters:\n");
    printf("  |    kappa     = %.4f MHz (total linewidth)\n",
           rc->kappa/(2.0*PI)/1e6);
    printf("  |    kappa_ext = %.4f MHz (external coupling)\n",
           rc->kappa_ext/(2.0*PI)/1e6);
    printf("  |    kappa_int = %.4f MHz (internal loss)\n",
           rc->kappa_int/(2.0*PI)/1e6);
    printf("  |    tau       = 2/kappa = %.2f ns\n", rc->tau*1e9);
    printf("  |\n");

    printf("  +------------------------------------------------------------+\n");
    printf("  |\n");
    printf("  |  [6] FINAL SIGNAL EQUATION\n");
    printf("  |\n");
    printf("  |  s(t) = V0 * exp(-kappa*t/2) * cos(w_probe*t + phi_qubit)\n");
    printf("  |\n");
    printf("  |    V0        = %.4f\n", rc->V0);
    printf("  |    kappa/2   = %.4f MHz (amplitude decay rate)\n",
           rc->kappa/(2.0*PI)/2.0/1e6);
    printf("  |    w_probe   = %.4f GHz\n",
           rc->omega_probe/(2.0*PI)/1e9);
    printf("  |    phi_qubit = %+.4f rad\n", rc->phi_qubit);
    printf("  |\n");
    printf("  |  Steady-state field amplitudes:\n");
    printf("  |    |<alpha>_ss| for |0> = %.6f\n", rc->alpha_ss_0);
    printf("  |    |<alpha>_ss| for |1> = %.6f\n", rc->alpha_ss_1);
    printf("  |\n");
    printf("  |  Mode: %s\n",
           rc->use_numerical ? "Numerical RK4 integration" : "Analytic formula");
    printf("  +============================================================+\n\n");
}

// ================================================================
// resonator_generate_signal_numerical()
// RK4 integration in ROTATING FRAME at omega_probe.
// In rotating frame, field varies at MHz scale -- stable at dt=0.1ns.
// ================================================================
void resonator_generate_signal_numerical(const ResonatorConfig *rc,
                                          double *signal) {
    double w_eff = (rc->qubit_state == 0)
                 ? rc->omega_r_0 : rc->omega_r_1;

    // Detuning in rotating frame (MHz scale -- RK4 stable)
    double delta_rot = w_eff - rc->omega_probe;
    double kh        = rc->kappa / 2.0;
    double skext     = sqrt(rc->kappa_ext);
    double alpha_in  = rc->V0;

    // Drive-on: first 20% of samples (pulse build-up)
    int drive_samples = rc->n_samples / 5;

    // Intracavity field in rotating frame (complex)
    double ar = 0.0, ai = 0.0;

    for (int i = 0; i < rc->n_samples; i++) {
        double t  = i * rc->dt;
        double dr = (i < drive_samples) ? alpha_in : 0.0;
        double di = 0.0;

        // Output field in rotating frame
        double out_r = skext * ar - (i < drive_samples ? alpha_in : 0.0);
        double out_i = skext * ai;

        // Rotate back to lab frame
        double phase = rc->omega_probe * t + rc->phi_qubit;
        signal[i]    = out_r * cos(phase) - out_i * sin(phase);

        // RK4 step in rotating frame
        // d(ar)/dt = -kh*ar + delta_rot*ai + skext*dr
        // d(ai)/dt = -kh*ai - delta_rot*ar + skext*di
        #define FR(r,ii) (-kh*(r) + delta_rot*(ii) + skext*dr)
        #define FI(r,ii) (-kh*(ii) - delta_rot*(r) + skext*di)

        double k1r = FR(ar,ai);
        double k1i = FI(ar,ai);
        double k2r = FR(ar+0.5*rc->dt*k1r, ai+0.5*rc->dt*k1i);
        double k2i = FI(ar+0.5*rc->dt*k1r, ai+0.5*rc->dt*k1i);
        double k3r = FR(ar+0.5*rc->dt*k2r, ai+0.5*rc->dt*k2i);
        double k3i = FI(ar+0.5*rc->dt*k2r, ai+0.5*rc->dt*k2i);
        double k4r = FR(ar+rc->dt*k3r, ai+rc->dt*k3i);
        double k4i = FI(ar+rc->dt*k3r, ai+rc->dt*k3i);

        ar += (rc->dt/6.0)*(k1r + 2*k2r + 2*k3r + k4r);
        ai += (rc->dt/6.0)*(k1i + 2*k2i + 2*k3i + k4i);

        #undef FR
        #undef FI
    }
}

// ================================================================
// resonator_generate_signal_analytic()
// Pure analytic formula -- always stable, matched filter compatible.
// s(t) = V0 * exp(-kappa*t/2) * cos(omega_probe*t + phi_qubit)
// ================================================================
void resonator_generate_signal_analytic(const ResonatorConfig *rc,
                                         double *signal) {
    for (int i = 0; i < rc->n_samples; i++) {
        double t   = i * rc->dt;
        double env = rc->V0 * exp(-rc->kappa * t / 2.0);
        signal[i]  = env * cos(rc->omega_probe * t + rc->phi_qubit);
    }
}

// ================================================================
// resonator_generate_template()
// Clean matched filter template -- always analytic, always |0>.
// ================================================================
void resonator_generate_template(const ResonatorConfig *rc,
                                  double *tmpl) {
    for (int i = 0; i < rc->n_samples; i++) {
        double t  = i * rc->dt;
        double env = rc->V0 * exp(-rc->kappa * t / 2.0);
        tmpl[i]   = env * cos(rc->omega_probe * t + PHI_0);
    }
}

// ================================================================
// resonator_save_metadata()
// ================================================================
void resonator_save_metadata(const ResonatorConfig *rc,
                              const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) { printf("ERROR: Cannot open %s\n", path); return; }

    fprintf(f, "{\n");
    fprintf(f, "  \"qubit_state\": %d,\n", rc->qubit_state);
    fprintf(f, "  \"phi_qubit_rad\": %.8f,\n", rc->phi_qubit);
    fprintf(f, "  \"phi_0_rad\": %.8f,\n", PHI_0);
    fprintf(f, "  \"phi_1_rad\": %.8f,\n", PHI_1);
    fprintf(f, "  \"delta_phi_rad\": %.8f,\n", DELTA_PHI);
    fprintf(f, "  \"omega_probe_GHz\": %.6f,\n",
            rc->omega_probe/(2.0*PI)/1e9);
    fprintf(f, "  \"omega_r_GHz\": %.6f,\n",
            rc->omega_r/(2.0*PI)/1e9);
    fprintf(f, "  \"omega_q_GHz\": %.6f,\n",
            rc->omega_q/(2.0*PI)/1e9);
    fprintf(f, "  \"kappa_MHz\": %.6f,\n", rc->kappa/(2.0*PI)/1e6);
    fprintf(f, "  \"chi_MHz\": %.6f,\n", rc->chi/(2.0*PI)/1e6);
    fprintf(f, "  \"g_MHz\": %.6f,\n", rc->g/(2.0*PI)/1e6);
    fprintf(f, "  \"tau_ns\": %.4f,\n", rc->tau*1e9);
    fprintf(f, "  \"V0\": %.4f,\n", rc->V0);
    fprintf(f, "  \"dt_ns\": %.4f,\n", rc->dt*1e9);
    fprintf(f, "  \"n_samples\": %d,\n", rc->n_samples);
    fprintf(f, "  \"template_csv\": \"%s\",\n", CSV_STAGE1T);
    fprintf(f, "  \"signal_csv\": \"%s\"\n", CSV_STAGE1);
    fprintf(f, "}\n");
    fclose(f);
    printf("  Metadata saved to %s\n", path);
}