#include "../include/audio_buffer.h"

#include <assert.h>

AudioBuffer::AudioBuffer() {
    m_writePointer = 0;
    m_sampleRate = 0;
    m_samples = nullptr;
    m_bufferSize = 0;
    m_sampleToSeconds = 0;
}

AudioBuffer::~AudioBuffer() {
    assert(m_samples == nullptr);
}

void AudioBuffer::initialize(int sampleRate, int bufferSize) {
    m_writePointer = 0;
    m_sampleRate = sampleRate;
    m_samples = new int[bufferSize];
    m_bufferSize = bufferSize;
    m_sampleToSeconds = 1 / (double)sampleRate;
}

void AudioBuffer::destroy() {
    delete[] m_samples;

    m_samples = nullptr;
    m_bufferSize = 0;
}
