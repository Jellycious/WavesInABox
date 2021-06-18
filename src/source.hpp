#include <algorithm>

#ifndef SOURCE_H
#define SOURCE_H
#define SOURCE_SHRINK_FACTOR 2.0

struct SourcePos {
    int x, y;   
};

class Source {
    int x, y;
    float freq, amplitude, phase;
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
            if(!active) {
                // decrease amplitude.
                amplitude = std::max(0.0, amplitude - delta * SOURCE_SHRINK_FACTOR);
            }
        }
        
        void setInactive() {
            active = false;
        }

        void setActive(float amplitude) {
            active = true;
            this->amplitude = amplitude;
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
            return amplitude;
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
