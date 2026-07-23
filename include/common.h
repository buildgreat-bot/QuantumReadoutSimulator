#ifndef COMMON_H
#define COMMON_H

#include <math.h>

// ================================================================
// PHYSICAL CONSTANTS
// ================================================================
#define HBAR        1.0545718e-34
#define KB          1.380649e-23
#define PI          3.14159265358979323846
#define SPEED_LIGHT 2.99792458e8

// ================================================================
// DEFAULT SIGNAL PARAMETERS
// ================================================================
#define DEFAULT_OMEGA_PROBE  (2.0 * PI * 6.0e9)
#define DEFAULT_OMEGA_R      (2.0 * PI * 6.0e9)
#define DEFAULT_OMEGA_Q      (2.0 * PI * 5.0e9)
#define DEFAULT_KAPPA        (2.0 * PI * 1.0e6)
#define DEFAULT_CHI          (2.0 * PI * 3.0e6)
#define DEFAULT_G            (2.0 * PI * 100.0e6)
#define DEFAULT_KAPPA_EXT    (2.0 * PI * 0.8e6)
#define DEFAULT_V0           1.0
#define DEFAULT_DT           1e-10
#define DEFAULT_N_SAMPLES    10240
#define DEFAULT_QUBIT_STATE  0

// ================================================================
// RUNTIME GLOBALS (defined in resonator_model.c)
// ================================================================
extern double OMEGA_PROBE;
extern double OMEGA_R;
extern double OMEGA_Q;
extern double KAPPA;
extern double CHI;
extern double G_COUPLING;
extern double KAPPA_EXT;
extern double V0;
extern double DT;
extern int    N_SAMPLES;
extern int    QUBIT_STATE;
extern double TAU;
extern double DELTA;
extern double PHI_0;
extern double PHI_1;
extern double DELTA_PHI;

// ================================================================
// RESONATOR CONFIG STRUCT
// ================================================================
typedef struct {
    double omega_probe;
    double omega_r;
    double omega_q;
    double g;
    double delta;
    double chi;
    double kappa;
    double kappa_ext;
    double kappa_int;
    double V0;
    double phi_qubit;
    double tau;
    double omega_r_0;
    double omega_r_1;
    double delta_eff_0;
    double delta_eff_1;
    double alpha_ss_0;
    double alpha_ss_1;
    int    qubit_state;
    int    use_numerical;
    double dt;
    int    n_samples;
} ResonatorConfig;

// ================================================================
// MATERIAL CONFIG STRUCT
// ================================================================
typedef struct {
    int    amp_type;
    int    cable_type;
    double cable_length_m;
    int    mixer_type;
    int    adc_bits;
    double bandwidth_hz;
} MaterialConfig;

// ================================================================
// AMPLIFIER PARAMETERS
// ================================================================
#define GAAS_HEMT_TN    4.5
#define GAAS_HEMT_GAIN  3162.0
#define INP_HEMT_TN     2.8
#define INP_HEMT_GAIN   6310.0
#define T_ROOM_AMP      290.0

// ================================================================
// CABLE PARAMETERS
// ================================================================
#define SS304_LOSS_PER_M    0.8
#define CUNI_LOSS_PER_M     0.5
#define NBTI_LOSS_PER_M     0.08

// ================================================================
// IQ MIXER PARAMETERS
// ================================================================
#define MIXER_STANDARD_LOSS_DB   8.0
#define MIXER_LOWNOISE_LOSS_DB   6.0

// ================================================================
// CSV OUTPUT PATHS
// ================================================================
#define CSV_STAGE1  "data/stage1_resonator.csv"
#define CSV_STAGE1T "data/stage1_template.csv"
#define CSV_STAGE2  "data/stage2_hemt_input.csv"
#define CSV_STAGE3  "data/stage3_hemt_output.csv"
#define CSV_STAGE4  "data/stage4_cable.csv"
#define CSV_STAGE5  "data/stage5_iq_mixer.csv"
#define CSV_STAGE6  "data/stage6_adc.csv"
#define JSON_META   "data/metadata.json"

// ================================================================
// FUNCTION DECLARATIONS
// ================================================================

// resonator_model.c
void   resonator_init_defaults(ResonatorConfig *rc, int qubit_state);
void   resonator_compute_derived(ResonatorConfig *rc);
void   resonator_print_full_physics(const ResonatorConfig *rc);
void   resonator_generate_signal_numerical(const ResonatorConfig *rc,
                                           double *signal);
void   resonator_generate_signal_analytic(const ResonatorConfig *rc,
                                          double *signal);
void   resonator_generate_template(const ResonatorConfig *rc,
                                   double *tmpl);
void   resonator_save_metadata(const ResonatorConfig *rc,
                               const char *path);

// signal_model.c
void signal_generate(double *signal, double A, double phi);
void signal_save_csv(const double *signal, int n, const char *filename);

// noise_model.c
double noise_quantum_sigma(const ResonatorConfig *rc);
double noise_hemt_sigma(const MaterialConfig *cfg);
void   noise_generate(double *noise, int n, double sigma);
void   noise_add(double *signal, const double *noise, int n);

// amplifier.c
double amplifier_get_gain(const MaterialConfig *cfg);
double amplifier_get_tn(const MaterialConfig *cfg);
double amplifier_friis_tsys(const MaterialConfig *cfg);
void   amplifier_apply_gain(double *signal, int n, double gain);

// materials.c
double materials_cable_loss_db(const MaterialConfig *cfg);
void   materials_apply_cable(double *signal, int n,
                              const MaterialConfig *cfg);
void   materials_apply_iq_mixer(double *signal, int n,
                                const MaterialConfig *cfg);
void   materials_apply_adc(double *signal, int n,
                           const MaterialConfig *cfg);

// print_stage.c
void print_stage_header(int stage, const char *name,
                        const char *location);
void print_stage_properties(const char **keys,
                             const char **vals, int count);
void print_stage_happening(const char *text);
void print_stage_stats(const double *signal, int n,
                       const char *csv_path);
void print_stage_footer(void);

#endif