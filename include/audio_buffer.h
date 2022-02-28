#ifndef ATG_ENGINE_SIM_AUDIO_BUFFER_H
#define ATG_ENGINE_SIM_AUDIO_BUFFER_H

#include "scs.h"

class AudioBuffer {
    public:
        AudioBuffer();
        ~AudioBuffer();

        void initialize(int sampleRate, int bufferSize);
        void destroy();

        __forceinline double sampleToTime(int sample) const {
            return sample * m_sampleToSeconds;
        }

        __forceinline double timeDelta(int sample0, int sample1) const {
            if (sample1 == sample0) return 0;
            else if (sample1 < sample0) {
                return sampleToTime((m_bufferSize - sample0) + sample1);
            }
            else {
                return sampleToTime(sample1 - sample0);
            }
        }

        __forceinline int sampleDelta(int sample0, int sample1) const {
            if (sample1 == sample0) return 0;
            else if (sample1 < sample0) {
                return (m_bufferSize - sample0) + sample1;
            }
            else {
                return sample1 - sample0;
            }
        }

        __forceinline void writeSample(int sample, int offset, int index = 0) {
            m_samples[getBufferIndex(offset, index)] = sample;
        }

        __forceinline int readSample(int offset, int index) const {
            return m_samples[getBufferIndex(offset, index)];
        }

        __forceinline void commitBlock(int offset, int length) {
            m_writePointer = getBufferIndex(m_writePointer, offset + length);
        }

        __forceinline int getBufferIndex(int offset, int index = 0) const {
            return (((offset + index) % m_bufferSize) + m_bufferSize) % m_bufferSize;
        }

        __forceinline void copyBuffer(int *dest, int offset, int length) {
            const int start = getBufferIndex(offset, 0);
            const int end = getBufferIndex(offset, length);

            if (start == end) return;
            else if (start < end) {
                memcpy(dest, m_samples + start, length * sizeof(int));
            }
            else {
                memcpy(dest, m_samples + start, (m_bufferSize - start) * sizeof(int));
                memcpy(dest + m_bufferSize - start, m_samples + end, end * sizeof(int));
            }
        }

        int m_writePointer;

    protected:
        int m_sampleRate;
        int *m_samples;
        int m_bufferSize;

        double m_sampleToSeconds;
};

#endif /* ATG_ENGINE_SIM_AUDIO_BUFFER_H */
