#include "../include/audio_buffer.h"

#include <assert.h>
#include <cmath>

AudioBuffer::AudioBuffer() {
    m_writePointer = 0;
    m_sampleRate = 0;
    m_samples = nullptr;
    m_bufferSize = 0;
    m_offsetToSeconds = 0;
}

AudioBuffer::~AudioBuffer() {
    assert(m_samples == nullptr);
}

void AudioBuffer::initialize(int sampleRate, int bufferSize) {
    m_writePointer = 0;
    m_sampleRate = sampleRate;
    m_samples = new int16_t[bufferSize];
    memset(m_samples, 0, sizeof(int16_t) * bufferSize);
    m_bufferSize = bufferSize;
    m_offsetToSeconds = 1 / (double)sampleRate;
}

void AudioBuffer::destroy() {
    delete[] m_samples;

    m_samples = nullptr;
    m_bufferSize = 0;
}

bool AudioBuffer::checkForDiscontinuitiy(int threshold) const {
    for (int i = 0; i < m_bufferSize - 1; ++i) {
        const int i0 = getBufferIndex(i + m_writePointer);
        const int i1 = getBufferIndex(i0 + 1);

        if (std::abs(m_samples[i0] - m_samples[i1]) >= threshold) {
            return true;
        }
    }

    return false;
}
