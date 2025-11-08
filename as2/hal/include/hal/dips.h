#ifndef _DIPS_H_
#define _DIPS_H_

// Configuration for dip detection hysteresis:
// trigger_drop_volts: magnitude below average required to trigger a dip (e.g. 0.10)
// reset_drop_volts: magnitude below average which must be recovered to (e.g. 0.07)
// A new dip cannot trigger until sample >= average - reset_drop_volts.
typedef struct {
    double trigger_drop_volts;
    double reset_drop_volts;
} DipConfig;

// Create configuration object.
static inline DipConfig DipConfig_make(double trigger_drop, double reset_drop)
{
    DipConfig c;
    c.trigger_drop_volts = trigger_drop;
    c.reset_drop_volts   = reset_drop;
    return c;
}

// Count dips using hysteresis against per-sample averages.
// samples[i]: voltage sample
// avgs[i]:    smoothed average at time of sample i
// n: number of samples
// Returns the number of dip events detected.
int Dips_count(const double *samples, const double *avgs, int n, DipConfig cfg);

#endif