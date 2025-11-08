#include "dips.h"

int Dips_count(const double *samples, const double *avgs, int n, DipConfig cfg)
{
    if (!samples || !avgs || n <= 0) return 0;
    double trigger = cfg.trigger_drop_volts;
    double reset   = cfg.reset_drop_volts;

    enum { WAITING_FOR_DIP, WAITING_FOR_RESET } state = WAITING_FOR_DIP;
    int dips = 0;

    for (int i = 0; i < n; i++) {
        double s = samples[i];
        double a = avgs[i];

        double trigger_threshold = a - trigger;
        double reset_threshold   = a - reset;

        if (state == WAITING_FOR_DIP) {
            if (s <= trigger_threshold) {
                dips++;
                state = WAITING_FOR_RESET;
            }
        } else { // WAITING_FOR_RESET
            if (s >= reset_threshold) {
                state = WAITING_FOR_DIP;
            }
        }
    }
    return dips;
}