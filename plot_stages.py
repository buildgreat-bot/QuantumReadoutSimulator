import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

# ================================================================
# QUANTUM READOUT SIMULATOR — Stage Plots
# Reads all 6 stage CSVs and plots them in one figure
# ================================================================

# Load all stage CSVs
def load_stage(filename):
    data = np.loadtxt(filename, delimiter=',', skiprows=1)
    return data[:, 0], data[:, 1]  # time, signal

# Load data
t1, s1 = load_stage('data/stage1_resonator.csv')
t2, s2 = load_stage('data/stage2_hemt_input.csv')
t3, s3 = load_stage('data/stage3_hemt_output.csv')
t4, s4 = load_stage('data/stage4_cable.csv')
t5, s5 = load_stage('data/stage5_iq_mixer.csv')
t6, s6 = load_stage('data/stage6_adc.csv')

# ================================================================
# PLOT SETTINGS
# ================================================================

plt.style.use('dark_background')  # looks great for presentations
# plt.style.use('seaborn-v0_8-paper')  # use this for publications

fig = plt.figure(figsize=(18, 10))
fig.suptitle('Quantum Readout Signal Chain — All 6 Stages',
             fontsize=16, fontweight='bold', y=0.98)

gs = gridspec.GridSpec(2, 3, figure=fig,
                       hspace=0.45, wspace=0.35)

# Color palette — one per stage
colors = ['#4FC3F7',   # Stage 1 — light blue
          '#EF5350',   # Stage 2 — red
          '#8B0000',   # Stage 3 — dark red
          '#FF8C00',   # Stage 4 — orange
          '#66BB6A',   # Stage 5 — green
          '#CE93D8']   # Stage 6 — purple

# Zoom windows
RF_ZOOM  = 50e-9    # 50ns for RF stages 1-4
BB_ZOOM  = 500e-9   # 500ns for baseband stages 5-6

# ================================================================
# STAGE 1 — Resonator Exit
# ================================================================
ax1 = fig.add_subplot(gs[0, 0])
mask = t1 <= RF_ZOOM
ax1.plot(t1[mask]*1e9, s1[mask], color=colors[0],
         linewidth=0.8, label='Clean signal')
ax1.set_title('Stage 1 — Resonator Exit\n(10-20 mK)',
              fontsize=10, fontweight='bold')
ax1.set_xlabel('Time (ns)', fontsize=8)
ax1.set_ylabel('Amplitude', fontsize=8)
ax1.legend(fontsize=7)
ax1.tick_params(labelsize=7)
ax1.text(0.98, 0.95,
         r'$A \cdot e^{-\kappa t/2} \cdot \cos(\omega t + \phi)$',
         transform=ax1.transAxes,
         fontsize=7, ha='right', va='top',
         color='white', alpha=0.8)

# ================================================================
# STAGE 2 — HEMT Input (noise added)
# ================================================================
ax2 = fig.add_subplot(gs[0, 1])
mask = t2 <= RF_ZOOM
ax2.plot(t2[mask]*1e9, s2[mask], color=colors[1],
         linewidth=0.8, label='Signal + noise')
ax2.set_title('Stage 2 — HEMT Input (4K)\n+Quantum + HEMT noise before gain',
              fontsize=10, fontweight='bold')
ax2.set_xlabel('Time (ns)', fontsize=8)
ax2.set_ylabel('Amplitude', fontsize=8)
ax2.legend(fontsize=7)
ax2.tick_params(labelsize=7)

# ================================================================
# STAGE 3 — HEMT Output (gain applied)
# ================================================================
ax3 = fig.add_subplot(gs[0, 2])
mask = t3 <= RF_ZOOM
ax3.plot(t3[mask]*1e9, s3[mask], color=colors[2],
         linewidth=0.8, label='Amplified signal')
ax3.set_title('Stage 3 — HEMT Output (4K)\n+Gain ×6310 applied',
              fontsize=10, fontweight='bold')
ax3.set_xlabel('Time (ns)', fontsize=8)
ax3.set_ylabel('Amplitude', fontsize=8)
ax3.legend(fontsize=7)
ax3.tick_params(labelsize=7)

# ================================================================
# STAGE 4 — Cryo Cable
# ================================================================
ax4 = fig.add_subplot(gs[1, 0])
mask = t4 <= RF_ZOOM
ax4.plot(t4[mask]*1e9, s4[mask], color=colors[3],
         linewidth=0.8, label='After cable')
ax4.set_title('Stage 4 — Cryo Cable (4K→300K)\n+Attenuation + thermal noise',
              fontsize=10, fontweight='bold')
ax4.set_xlabel('Time (ns)', fontsize=8)
ax4.set_ylabel('Amplitude', fontsize=8)
ax4.legend(fontsize=7)
ax4.tick_params(labelsize=7)

# ================================================================
# STAGE 5 — IQ Mixer (baseband)
# ================================================================
ax5 = fig.add_subplot(gs[1, 1])
mask = t5 <= BB_ZOOM
ax5.plot(t5[mask]*1e9, s5[mask], color=colors[4],
         linewidth=1.0, label='Baseband envelope')
ax5.set_title('Stage 5 — IQ Mixer (300K)\n6GHz stripped — baseband',
              fontsize=10, fontweight='bold')
ax5.set_xlabel('Time (ns)', fontsize=8)
ax5.set_ylabel('Amplitude', fontsize=8)
ax5.set_ylim(-0.6, 0.6)
ax5.legend(fontsize=7)
ax5.tick_params(labelsize=7)

# ================================================================
# STAGE 6 — ADC (digitized)
# ================================================================
ax6 = fig.add_subplot(gs[1, 2])
mask = t6 <= BB_ZOOM
ax6.plot(t6[mask]*1e9, s6[mask], color=colors[5],
         linewidth=1.0, label='Digitized signal',
         drawstyle='steps-post')  # shows quantization steps
ax6.set_title('Stage 6 — ADC (300K)\nQuantized — ready for matched filter',
              fontsize=10, fontweight='bold')
ax6.set_xlabel('Time (ns)', fontsize=8)
ax6.set_ylabel('Amplitude', fontsize=8)
ax6.set_ylim(-0.6, 0.6)
ax6.legend(fontsize=7)
ax6.tick_params(labelsize=7)

# ================================================================
# SAVE AND SHOW
# ===================================python plot_stages.pypython plot_stages.py=============================
plt.savefig('data/signal_chain_plot.png',
            dpi=150, bbox_inches='tight',
            facecolor=fig.get_facecolor())
print("Plot saved to data/signal_chain_plot.png")
plt.show()