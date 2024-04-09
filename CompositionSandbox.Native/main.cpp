// Experiments list
#define EXPERIMENT_NONE 0
#define EXPERIMENT_CROSS_PROCESS_VISUALS 1

// Experiment to be compiled
#define EXPERIMENT EXPERIMENT_CROSS_PROCESS_VISUALS

#if EXPERIMENT == EXPERIMENT_CROSS_PROCESS_VISUALS
#include "CrossProcessVisuals.h"
#endif