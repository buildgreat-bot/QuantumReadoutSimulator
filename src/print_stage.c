#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../include/common.h"

#define BOX_WIDTH 58

static void print_line(char c) {
    printf("  +");
    for (int i = 0; i < BOX_WIDTH; i++) printf("%c", c);
    printf("+\n");
}

static void print_row(const char *text) {
    printf("  | %-*s |\n", BOX_WIDTH - 2, text);
}

void print_stage_header(int stage, const char *name,
                         const char *location) {
    printf("\n");
    print_line('=');
    char buf[BOX_WIDTH];
    snprintf(buf, sizeof(buf), "  STAGE %d -- %s", stage, name);
    print_row(buf);
    snprintf(buf, sizeof(buf), "  Location: %s", location);
    print_row(buf);
    print_line('-');
}

void print_stage_properties(const char **keys,
                             const char **vals, int count) {
    print_row("  Properties Used:");
    char buf[BOX_WIDTH];
    for (int i = 0; i < count; i++) {
        snprintf(buf, sizeof(buf), "    %-22s: %s", keys[i], vals[i]);
        print_row(buf);
    }
    print_line('-');
}

void print_stage_happening(const char *text) {
    print_row("  What is happening:");
    char buf[BOX_WIDTH];
    char line[BOX_WIDTH];
    char word[64];
    line[0] = '\0';
    int len = 0;
    const char *p = text;
    while (*p) {
        int w = 0;
        while (*p && *p != ' ' && *p != '\n') {
            if (w < 63) word[w++] = *p;
            p++;
        }
        word[w] = '\0';
        if (*p) p++;
        if (len + w + 1 > 50 && len > 0) {
            snprintf(buf, sizeof(buf), "    %s", line);
            print_row(buf);
            line[0] = '\0';
            len = 0;
        }
        if (len > 0) { strncat(line, " ", sizeof(line)-len-1); len++; }
        strncat(line, word, sizeof(line)-len-1);
        len += w;
    }
    if (len > 0) {
        snprintf(buf, sizeof(buf), "    %s", line);
        print_row(buf);
    }
    print_line('-');
}

// ================================================================
// ROBUST RMS -- handles inf, nan, and extreme values safely.
//
// Strategy:
// 1. Scan for finite peak (skip inf/nan values).
// 2. If no finite values found, report 0.
// 3. Normalize each sample by peak before squaring.
//    This keeps values in [0,1] -- no overflow possible.
// 4. Final result = peak * sqrt(mean_of_normalized_squares).
// ================================================================
void print_stage_stats(const double *signal, int n,
                        const char *csv_path) {
    print_row("  Signal Stats:");

    // Step 1: find finite peak
    double peak = 0.0;
    int finite_count = 0;
    for (int i = 0; i < n; i++) {
        if (isfinite(signal[i])) {
            double a = fabs(signal[i]);
            if (a > peak) peak = a;
            finite_count++;
        }
    }

    // Step 2: compute normalized RMS over finite values only
    double rms = 0.0;
    if (peak > 0.0 && finite_count > 0) {
        for (int i = 0; i < n; i++) {
            if (isfinite(signal[i])) {
                double norm = signal[i] / peak;
                rms += norm * norm;
            }
        }
        rms = peak * sqrt(rms / (double)finite_count);
    }

    char buf[BOX_WIDTH];
    snprintf(buf, sizeof(buf), "    Peak amplitude : %.6f", peak);
    print_row(buf);
    snprintf(buf, sizeof(buf), "    RMS value      : %.6f", rms);
    print_row(buf);

    if (finite_count < n) {
        char warn[BOX_WIDTH];
        snprintf(warn, sizeof(warn),
                 "    WARNING: %d non-finite samples skipped",
                 n - finite_count);
        print_row(warn);
    }

    snprintf(buf, sizeof(buf), "    Saved to       : %s", csv_path);
    print_row(buf);
    print_line('=');
}

void print_stage_footer(void) {
    printf("\n");
}