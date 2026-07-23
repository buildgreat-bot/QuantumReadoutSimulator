#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/common.h"

// ================================================================
// CLI ARGUMENT PARSERS
// ================================================================
static double parse_double(const char *arg, const char *flag) {
    int len = (int)strlen(flag);
    if (strncmp(arg, flag, len) == 0 && arg[len] == '=')
        return atof(arg + len + 1);
    return -1e99;
}

static int parse_int(const char *arg, const char *flag) {
    int len = (int)strlen(flag);
    if (strncmp(arg, flag, len) == 0 && arg[len] == '=')
        return atoi(arg + len + 1);
    return -999999;
}

// ================================================================
// CONFIG BANNER
// ================================================================
static void print_banner(const ResonatorConfig *rc,
                         const MaterialConfig  *mc) {
    printf("\n");
    printf("  +============================================================+\n");
    printf("  |     QUANTUM READOUT SIMULATOR -- PART 1                   |\n");
    printf("  |     Full Signal Chain: Resonator -> ADC                   |\n");
    printf("  +============================================================+\n");
    printf("  |\n");
    printf("  |  RESONATOR / QUBIT PARAMETERS:\n");
    printf("  |    Probe frequency  : %.4f GHz\n",
           rc->omega_probe/(2.0*PI)/1e9);
    printf("  |    Resonator freq   : %.4f GHz\n",
           rc->omega_r/(2.0*PI)/1e9);
    printf("  |    Qubit frequency  : %.4f GHz\n",
           rc->omega_q/(2.0*PI)/1e9);
    printf("  |    Coupling g       : %.2f MHz\n",
           rc->g/(2.0*PI)/1e6);
    printf("  |    Kappa (linewidth): %.4f MHz\n",
           rc->kappa/(2.0*PI)/1e6);
    printf("  |    Chi (disp shift) : %.4f MHz\n",
           rc->chi/(2.0*PI)/1e6);
    printf("  |    Tau (decay time) : %.2f ns\n", rc->tau*1e9);
    printf("  |    Qubit state      : |%d>\n", rc->qubit_state);
    printf("  |    Phase phi        : %+.4f rad (%+.2f deg)\n",
           rc->phi_qubit, rc->phi_qubit*180.0/PI);
    printf("  |\n");
    printf("  |  HARDWARE CONFIGURATION:\n");
    printf("  |    Amplifier  : %s\n",
           mc->amp_type==0 ? "GaAs HEMT (T_N=4.5K, 35dB)"
                           : "InP HEMT  (T_N=2.8K, 38dB)");
    printf("  |    Cable      : %s (%.1fm)\n",
           mc->cable_type==0 ? "SS304 (0.8dB/m)" :
           mc->cable_type==1 ? "CuNi  (0.5dB/m)" :
                               "NbTi  (0.08dB/m)",
           mc->cable_length_m);
    printf("  |    IQ Mixer   : %s\n",
           mc->mixer_type==0 ? "Standard  (8dB loss)"
                             : "Low-Noise (6dB loss)");
    printf("  |    ADC        : %d-bit (%d levels)\n",
           mc->adc_bits, (int)pow(2.0, mc->adc_bits));
    printf("  |    Bandwidth  : %.0f MHz\n", mc->bandwidth_hz/1e6);
    printf("  |\n");
    printf("  |  SAMPLING:\n");
    printf("  |    dt         : %.2f ns (%.1f samples/cycle at probe)\n",
           DT*1e9, 1.0/(DT*OMEGA_PROBE/(2.0*PI)));
    printf("  |    N_SAMPLES  : %d (covers %.1f ns)\n",
           N_SAMPLES, N_SAMPLES*DT*1e9);
    printf("  |    Mode       : %s\n",
           rc->use_numerical ? "RK4 numerical integration"
                             : "Analytic formula");
    printf("  +============================================================+\n\n");
}

// ================================================================
// MAIN
// ================================================================
int main(int argc, char **argv) {

    // Set global defaults
    OMEGA_PROBE = DEFAULT_OMEGA_PROBE;
    OMEGA_R     = DEFAULT_OMEGA_R;
    OMEGA_Q     = DEFAULT_OMEGA_Q;
    KAPPA       = DEFAULT_KAPPA;
    CHI         = DEFAULT_CHI;
    G_COUPLING  = DEFAULT_G;
    KAPPA_EXT   = DEFAULT_KAPPA_EXT;
    V0          = DEFAULT_V0;
    DT          = DEFAULT_DT;
    N_SAMPLES   = DEFAULT_N_SAMPLES;
    QUBIT_STATE = DEFAULT_QUBIT_STATE;

    int use_numerical = 0;   // default: analytic (safe and stable)
    int shots = 1;

    // Parse CLI arguments
    for (int i = 1; i < argc; i++) {
        double v; int iv;
        if ((v  = parse_double(argv[i], "--probe"))     != -1e99) OMEGA_PROBE = 2.0*PI*v;
        if ((v  = parse_double(argv[i], "--kappa"))     != -1e99) KAPPA       = 2.0*PI*v;
        if ((v  = parse_double(argv[i], "--chi"))       != -1e99) CHI         = 2.0*PI*v;
        if ((v  = parse_double(argv[i], "--g"))         != -1e99) G_COUPLING  = 2.0*PI*v;
        if ((v  = parse_double(argv[i], "--dt"))        != -1e99) DT          = v;
        if ((v  = parse_double(argv[i], "--tau"))       != -1e99) KAPPA       = 2.0/v;
        if ((iv = parse_int(argv[i],    "--qubit"))     != -999999) QUBIT_STATE = iv;
        if ((iv = parse_int(argv[i],    "--samples"))   != -999999) N_SAMPLES   = iv;
        if ((iv = parse_int(argv[i],    "--shots"))     != -999999) shots       = iv;
        if (strcmp(argv[i], "--numerical") == 0) use_numerical = 1;
        if (strcmp(argv[i], "--analytic")  == 0) use_numerical = 0;
    }

    // Build resonator config
    ResonatorConfig rc;
    resonator_init_defaults(&rc, QUBIT_STATE);
    rc.use_numerical = use_numerical;
    resonator_compute_derived(&rc);

    // Build material config (defaults)
    MaterialConfig mc;
    mc.amp_type       = 1;       // InP HEMT
    mc.cable_type     = 1;       // CuNi
    mc.cable_length_m = 1.5;
    mc.mixer_type     = 1;       // Low-Noise
    mc.adc_bits       = 12;
    mc.bandwidth_hz   = 100e6;

    // Parse material CLI args
    for (int i = 1; i < argc; i++) {
        double v; int iv;
        if (strstr(argv[i], "--amp=GaAs"))    mc.amp_type    = 0;
        if (strstr(argv[i], "--amp=InP"))     mc.amp_type    = 1;
        if (strstr(argv[i], "--cable=SS304")) mc.cable_type  = 0;
        if (strstr(argv[i], "--cable=CuNi"))  mc.cable_type  = 1;
        if (strstr(argv[i], "--cable=NbTi"))  mc.cable_type  = 2;
        if (strstr(argv[i], "--mixer_loss=8")) mc.mixer_type = 0;
        if (strstr(argv[i], "--mixer_loss=6")) mc.mixer_type = 1;
        if ((iv = parse_int(argv[i],    "--adc"))       != -999999) mc.adc_bits      = iv;
        if ((v  = parse_double(argv[i], "--bandwidth")) != -1e99)   mc.bandwidth_hz  = v;
        if ((v  = parse_double(argv[i], "--cable_len")) != -1e99)   mc.cable_length_m = v;
    }

    // Allocate signal arrays
    double *signal    = (double*)malloc(rc.n_samples * sizeof(double));
    double *tmpl      = (double*)malloc(rc.n_samples * sizeof(double));
    double *noise_arr = (double*)malloc(rc.n_samples * sizeof(double));
    if (!signal || !tmpl || !noise_arr) {
        printf("ERROR: memory allocation failed\n");
        return 1;
    }

    // Print banner
    print_banner(&rc, &mc);

    // ================================================================
    // STAGE 1 -- RESONATOR EXIT
    // ================================================================
    resonator_print_full_physics(&rc);

    if (rc.use_numerical)
        resonator_generate_signal_numerical(&rc, signal);
    else
        resonator_generate_signal_analytic(&rc, signal);

    resonator_generate_template(&rc, tmpl);
    signal_save_csv(tmpl, rc.n_samples, CSV_STAGE1T);
    printf("  Template saved to %s\n", CSV_STAGE1T);

    signal_save_csv(signal, rc.n_samples, CSV_STAGE1);
    resonator_save_metadata(&rc, JSON_META);
    print_stage_stats(signal, rc.n_samples, CSV_STAGE1);
    print_stage_footer();

    // ================================================================
    // STAGE 2 -- HEMT INPUT: quantum + electronic noise BEFORE gain
    // ================================================================
    {
        double sigma_q = noise_quantum_sigma(&rc);
        double sigma_e = noise_hemt_sigma(&mc);
        double sigma_t = sqrt(sigma_q*sigma_q + sigma_e*sigma_e);

        char sq[32], se[32], st[32], tn[32];
        snprintf(sq, sizeof(sq), "%.4e V", sigma_q);
        snprintf(se, sizeof(se), "%.4e V", sigma_e);
        snprintf(st, sizeof(st), "%.4e V", sigma_t);
        snprintf(tn, sizeof(tn), "%.1f K", amplifier_get_tn(&mc));

        const char *keys[] = {
            "Amplifier","Noise temp T_N",
            "Sigma quantum","Sigma electronic","Sigma total"
        };
        const char *vals[] = {
            mc.amp_type==0 ? "GaAs HEMT":"InP HEMT",
            tn, sq, se, st
        };

        print_stage_header(2,
            "HEMT Input -- Noise Added BEFORE Gain",
            "4K plate (dilution refrigerator)");
        print_stage_properties(keys, vals, 5);
        print_stage_happening(
            "Quantum vacuum noise and HEMT electronic noise both added "
            "to signal BEFORE gain. Physically critical -- noise added "
            "before gain gets amplified. Sets SNR floor for entire chain."
        );

        noise_generate(noise_arr, rc.n_samples, sigma_t);
        noise_add(signal, noise_arr, rc.n_samples);
        signal_save_csv(signal, rc.n_samples, CSV_STAGE2);
        print_stage_stats(signal, rc.n_samples, CSV_STAGE2);
        print_stage_footer();
    }

    // ================================================================
    // STAGE 3 -- HEMT OUTPUT: gain applied
    // ================================================================
    {
        double gain  = amplifier_get_gain(&mc);
        double T_sys = amplifier_friis_tsys(&mc);

        char cg[32], ct[32];
        snprintf(cg, sizeof(cg), "x%.0f (%.0f dB)", gain, 20.0*log10(gain));
        snprintf(ct, sizeof(ct), "%.4f K", T_sys);

        const char *keys[] = {"Amplifier","Gain applied","Friis T_sys"};
        const char *vals[] = {
            mc.amp_type==0 ? "GaAs HEMT (35dB)":"InP HEMT (38dB)",
            cg, ct
        };

        print_stage_header(3,
            "HEMT Output -- Gain Applied",
            "4K plate");
        print_stage_properties(keys, vals, 3);
        print_stage_happening(
            "HEMT amplifies signal and noise equally. SNR unchanged. "
            "Purpose: make signal survive cryo cable journey to 300K. "
            "Friis cascade confirms room-temp amp negligible after this gain."
        );

        amplifier_apply_gain(signal, rc.n_samples, gain);
        signal_save_csv(signal, rc.n_samples, CSV_STAGE3);
        print_stage_stats(signal, rc.n_samples, CSV_STAGE3);
        print_stage_footer();
    }

    // ================================================================
    // STAGE 4 -- CRYO CABLE
    // ================================================================
    {
        double loss_db = materials_cable_loss_db(&mc);
        double att     = pow(10.0, -loss_db/20.0);
        const char *cnames[] = {"SS304","CuNi","NbTi"};
        const char *closes[] = {"0.8 dB/m","0.5 dB/m","0.08 dB/m"};

        char cl[32], cd[32];
        snprintf(cl, sizeof(cl), "%.1f m", mc.cable_length_m);
        snprintf(cd, sizeof(cd), "%.3f dB (factor=%.4f)", loss_db, att);

        const char *keys[] = {
            "Cable material","Loss per meter","Length","Total attenuation"
        };
        const char *vals[] = {
            cnames[mc.cable_type],closes[mc.cable_type],cl,cd
        };

        print_stage_header(4,
            "Cryo Cable -- 4K to 300K",
            "Cryogenic coaxial cable");
        print_stage_properties(keys, vals, 4);
        print_stage_happening(
            "Signal travels up through cryo cable from 4K to 300K. "
            "Cable attenuates signal. Small thermal noise from cable walls added."
        );

        materials_apply_cable(signal, rc.n_samples, &mc);
        signal_save_csv(signal, rc.n_samples, CSV_STAGE4);
        print_stage_stats(signal, rc.n_samples, CSV_STAGE4);
        print_stage_footer();
    }

    // ================================================================
    // STAGE 5 -- IQ MIXER
    // ================================================================
    {
        double loss_db = (mc.mixer_type==0) ?
                         MIXER_STANDARD_LOSS_DB : MIXER_LOWNOISE_LOSS_DB;
        double factor  = pow(10.0, -loss_db/20.0);
        char cf[32];
        snprintf(cf, sizeof(cf), "%.3f", factor);

        const char *keys[] = {
            "Mixer type","Conversion loss","Loss factor","Output"
        };
        const char *vals[] = {
            mc.mixer_type==0 ? "Standard":"Low-Noise",
            mc.mixer_type==0 ? "8 dB":"6 dB",
            cf, "Baseband envelope (carrier stripped)"
        };

        print_stage_header(5,
            "IQ Mixer -- Carrier Stripped",
            "300K lab bench");
        print_stage_properties(keys, vals, 4);
        print_stage_happening(
            "6GHz carrier stripped. Baseband envelope extracted. "
            "Qubit state encoded as amplitude sign. Conversion loss applied."
        );

        materials_apply_iq_mixer(signal, rc.n_samples, &mc);
        signal_save_csv(signal, rc.n_samples, CSV_STAGE5);
        print_stage_stats(signal, rc.n_samples, CSV_STAGE5);
        print_stage_footer();
    }

    // ================================================================
    // STAGE 6 -- ADC
    // ================================================================
    {
        int    levels = (int)pow(2.0, mc.adc_bits);
        double step   = 2.0 / (levels - 1);
        char cl[32], cs[32];
        snprintf(cl, sizeof(cl), "%d levels", levels);
        snprintf(cs, sizeof(cs), "%.6f", step);

        const char *keys[] = {
            "ADC resolution","Levels","Step size","Input range"
        };
        const char *vals[] = {
            mc.adc_bits==8  ? "8-bit"  :
            mc.adc_bits==12 ? "12-bit" : "14-bit",
            cl, cs, "[-1.0, +1.0]"
        };

        print_stage_header(6,
            "ADC -- Signal Digitized",
            "300K lab bench (digitizer card)");
        print_stage_properties(keys, vals, 4);
        print_stage_happening(
            "Continuous analog signal quantized to discrete digital levels. "
            "Output x[n] is input to Part 2 matched filter."
        );

        materials_apply_adc(signal, rc.n_samples, &mc);
        signal_save_csv(signal, rc.n_samples, CSV_STAGE6);
        print_stage_stats(signal, rc.n_samples, CSV_STAGE6);
        print_stage_footer();
    }

    // ================================================================
    // DONE
    // ================================================================
    printf("  +============================================================+\n");
    printf("  |  PART 1 COMPLETE                                          |\n");
    printf("  |  6 stage CSVs saved to data/                              |\n");
    printf("  |  Template: data/stage1_template.csv                       |\n");
    printf("  |  Metadata: data/metadata.json                             |\n");
    printf("  |  Shots configured : %-3d (Monte Carlo ready)              |\n",
           shots);
    printf("  |  Run: gnuplot plot_stages.gp                              |\n");
    printf("  +============================================================+\n\n");

    free(signal);
    free(tmpl);
    free(noise_arr);
    return 0;
}