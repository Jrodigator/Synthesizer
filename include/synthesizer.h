#include <time.h>

// Structs 
//--------------------------------------------------------------------------------------

typedef struct { /// Oscillator
    float phase;
    float phase_shift; 
    float frequency; 
} Oscillator;

typedef struct { /// Envolope
    float attack;
    float decay;
    float sustain;
    float release;
    float amplitude;
} Envolope;
//--------------------------------------------------------------------------------------

// Function prototypes
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------


// Function definitions
//--------------------------------------------------------------------------------------
void updateOsc(Oscillator * osc) {
    osc->phase += osc->phase_shift;
    if (osc->phase >= 1.0f) {
        osc->phase -= 1.0f;
    }
}

void updateEnv(Envolope* envolope, clock_t interval) {
        if (interval < envolope->attack) {
            envolope->amplitude = ( interval / envolope->attack );
        } else if (interval < envolope->attack + envolope->decay) {
            envolope->amplitude = 1.0f - (1.0f - envolope->sustain) * (interval - envolope->attack) / envolope->decay;
        } else if (interval < envolope->attack + envolope->decay + envolope->sustain) {
            envolope->amplitude = envolope->sustain;
        } else if (interval < envolope->attack + envolope->decay + envolope->sustain + envolope->release) {
            envolope->amplitude = envolope->amplitude * (1.0f - (interval - envolope->attack - envolope->decay - envolope->sustain) / envolope->release);
        } else {
            envolope->amplitude = 0.0f;
            //printf("should be off\n");
        }
}