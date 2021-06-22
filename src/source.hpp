#include <algorithm>

#ifndef SOURCE_H
#define SOURCE_H
#define SOURCE_AMP_RESPONSE_RATE 2.0 // Determines the speed at which the amplitude can change

struct SourcePos {
    int x, y;   
};

class Source {
    int x, y;
    float freq, amplitude, phase, cur_amplitude;
    bool active;
    public:
        Source(int x, int y, float amplitude, float freq) {
            this->x = x;
            this->y = y;
            this->amplitude = amplitude;
            this->freq = freq;
            this->phase = 0;
            active = true;
        }

        void update(double delta) {
            phase+=delta;
            // approach desired amplitude
            float dAmp = amplitude - cur_amplitude; 
            cur_amplitude += dAmp * SOURCE_AMP_RESPONSE_RATE * delta;
        }
        
        void setInactive() {
            active = false;
            this->amplitude = 0.0;
        }

        void setActive() {
            active = true;
            this->phase = 0.0;
        }

        void setPos(int x, int y) {
            this->x = x;
            this->y = y;
        }
        SourcePos getPos() {
            return SourcePos {x, y};
        }
        float getPhase() {
            return phase;
        }
        void setPhase(float phase) {
            this->phase = phase;
        }
        float getAmplitude() {
            return cur_amplitude;
        }
        void setAmplitude(float amp) {
            this->amplitude = amp;
        }
        float getFreq() {
            return freq;
        }
        void setFreq(float freq) {
            this->freq = freq;
        }

};
#endif
