#include "../include/synthesizer.h"

#include "../include/delta.h"

#include <cassert>
#include <cmath>

#undef min

Synthesizer::Synthesizer() {
    m_audioSampleToTimeOffset = 0.0;

    m_inputChannels = nullptr;
    m_inputChannelCount = 0;
    m_inputBufferSize = 0;
    m_inputWriteOffset = 0;
    m_inputSafetyBoundary = 0;

    m_audioBuffer = nullptr;
    m_audioBufferSize = 0;
    m_audioBufferWriteOffset = 0;
    m_audioBufferSafetyBoundary = 0;

    m_inputSampleRate = 0.0;
    m_audioSampleRate = 0.0;
    m_latency = 0.0;

    m_run = true;
    m_thread = nullptr;
}

Synthesizer::~Synthesizer()
{
    assert(m_inputChannels == nullptr);
    assert(m_audioBuffer == nullptr);
    assert(m_thread == nullptr);
}

void Synthesizer::initialize(const Parameters &p) {
    m_inputChannelCount = p.InputChannelCount;
    m_inputBufferSize = p.InputBufferSize;
    m_inputWriteOffset = p.InputBufferSize;
    m_audioBufferSize = p.AudioBufferSize;
    m_inputSampleRate = p.InputSampleRate;
    m_audioSampleRate = p.AudioSampleRate;
    m_latency = p.Latency;

    m_inputWriteOffset = 0;
    m_audioBufferWriteOffset = 0;
    m_processed = true;

    m_audioBuffer = new int16_t[p.AudioBufferSize];
    m_inputChannels = new InputChannel[p.InputChannelCount];
    for (int i = 0; i < p.InputChannelCount; ++i) {
        m_inputChannels[i].Data = new double[p.InputBufferSize];
    }

    // temp
    ysWindowsAudioWaveFile waveFile0;
    waveFile0.OpenFile("../assets/test_engine_03_16.wav");
    waveFile0.InitializeInternalBuffer(waveFile0.GetSampleCount());
    waveFile0.FillBuffer(0);
    waveFile0.CloseFile();

    ysWindowsAudioWaveFile waveFile1;
    waveFile1.OpenFile("../assets/test_engine_16.wav");
    waveFile1.InitializeInternalBuffer(waveFile1.GetSampleCount());
    waveFile1.FillBuffer(0);
    waveFile1.CloseFile();

    const unsigned int sampleCount0 = std::min((unsigned int)9000, waveFile0.GetSampleCount());
    const unsigned int sampleCount1 = std::min((unsigned int)9000, waveFile1.GetSampleCount());

    temp_filter_0.initialize(sampleCount0);
    for (int i = 0; i < sampleCount0; ++i) {
        //temp_filter_0.getImpulseResponse()[i] =
        //    /*std::exp(-i * 0.1) **/ 0.01 * (
        //        std::cos(20 * 3.14159 * i / 8000.0)
        //        + std::cos(50 * 3.14159 * i / 8000.0)
        //        + std::cos(90 * 3.14159 * i / 8000.0)
        //        + std::cos(170 * 3.14159 * i / 8000.0));*/
        temp_filter_0.getImpulseResponse()[i] = 0.025 * ((int16_t *)waveFile0.GetBuffer())[i] / INT16_MAX;
    }

    temp_filter_1.initialize(sampleCount1);
    for (int i = 0; i < sampleCount1; ++i) {
        temp_filter_1.getImpulseResponse()[i] = 0.025 * ((int16_t *)waveFile1.GetBuffer())[i] / INT16_MAX;
    }
}

void Synthesizer::startAudioRenderingThread() {
    m_run = true;
    m_thread = new std::thread(&Synthesizer::audioRenderingThread, this);
}

void Synthesizer::endAudioRenderingThread() {
    m_run = false;
    endInputBlock();

    m_thread->join();
    delete m_thread;

    m_thread = nullptr;
}

void Synthesizer::destroy() {
    delete[] m_audioBuffer;

    for (int i = 0; i < m_inputChannelCount; ++i) {
        delete[] m_inputChannels[i].Data;
    }

    delete[] m_inputChannels;

    m_audioBuffer = nullptr;
    m_inputChannels = nullptr;
}

int Synthesizer::readAudioOutput(int samples, int16_t *buffer) {
    std::lock_guard<std::mutex> lock(m_lock0);

    const int newDataLength = m_audioBufferWriteOffset;
    if (newDataLength >= samples) {
        memcpy(
            buffer,
            m_audioBuffer,
            sizeof(int16_t) * samples);
    }
    else {
        memcpy(
            buffer,
            m_audioBuffer,
            sizeof(int16_t) * newDataLength);
        memset(
            buffer + newDataLength,
            0,
            sizeof(int16_t) * ((size_t)samples - newDataLength));
    }
    
    const int samplesConsumed = std::min(samples, newDataLength);
    trim(samplesConsumed);

    return samplesConsumed;
}

void Synthesizer::waitProcessed() {
    {
        std::unique_lock<std::mutex> lk(m_lock0);
        m_cv0.wait(lk, [this] { return m_processed; });
    }
}

void Synthesizer::writeInput(const double *data) {
    assert(m_inputWriteOffset < m_inputBufferSize);

    if (m_inputWriteOffset >= m_inputBufferSize) return;

    for (int i = 0; i < m_inputChannelCount; ++i) {
        m_inputChannels[i].Data[m_inputWriteOffset] = data[i];
    }

    ++m_inputWriteOffset;
}

void Synthesizer::endInputBlock() {
    {
        std::lock_guard<std::mutex> lk(m_lock0);
        m_inputSafetyBoundary = m_inputWriteOffset - 1;
        m_processed = false;
    }

    m_cv0.notify_one();
}

void Synthesizer::audioRenderingThread() {
    while (m_run) {
        renderAudio();
    }
}

void Synthesizer::renderAudio() {
    std::unique_lock<std::mutex> lk0(m_lock0);

    m_cv0.wait(lk0, [this] {
        const double timeOffset = audioSampleToTimeOffset(m_audioBufferWriteOffset);
        return !m_run || timeOffsetInBounds(timeOffset, m_inputSafetyBoundary);
        });

    for (; m_audioBufferWriteOffset < m_audioBufferSize; ++m_audioBufferWriteOffset) {
        const double timeOffset = audioSampleToTimeOffset(m_audioBufferWriteOffset);
        if (!timeOffsetInBounds(timeOffset, m_inputSafetyBoundary)) break;
        else if (!timeOffsetGreaterZero(timeOffset)) {
            m_audioBuffer[m_audioBufferWriteOffset] = 0;
            continue;
        }
        else {
            m_audioBuffer[m_audioBufferWriteOffset] = renderAudio(timeOffset);
        }
    }

    m_processed = true;

    lk0.unlock();
    m_cv0.notify_one();
}

double Synthesizer::audioSampleToTimeOffset(int sample) const {
    return (sample / m_audioSampleRate) - m_latency + m_audioSampleToTimeOffset;
}

double Synthesizer::audioSamplesToTimeDelta(int samples) const {
    return samples / m_audioSampleRate;
}

double Synthesizer::timeOffsetToInputSample(double timeOffset) const {
    return (timeOffset * m_inputSampleRate);
}

double Synthesizer::inputSampleToTimeOffset(double inputSample) const {
    return (inputSample / m_inputSampleRate);
}

double Synthesizer::sampleInput(double timeOffset, int channel) const {
    const double index = timeOffsetToInputSample(timeOffset);

    const int index0 = (int)std::floor(index);
    const int index1 = (int)std::ceil(index);
    const double s = index - index0;

    const double *data = m_inputChannels[channel].Data;
    return (1 - s) * data[index0] + s * data[index1];
}

bool Synthesizer::timeOffsetGreaterZero(double timeOffset) const {
    const double index = timeOffsetToInputSample(timeOffset);
    const int index0 = (int)std::floor(index);

    return index0 >= 0;
}

bool Synthesizer::timeOffsetInBounds(double timeOffset, int safetyBoundary) const {
    const double index = timeOffsetToInputSample(timeOffset);
    const int index1 = (int)std::ceil(index);

    return index1 <= safetyBoundary;
}

void Synthesizer::trim(int audioSamples) {
    if (audioSamples > 0) {
        const int samplesLeft = m_audioBufferWriteOffset - audioSamples;
        memmove(
            m_audioBuffer,
            m_audioBuffer + audioSamples,
            sizeof(int16_t) * samplesLeft);

        m_audioBufferWriteOffset -= audioSamples;
    }

    const double startTimeOffset = audioSampleToTimeOffset(audioSamples);
    trimInput(startTimeOffset);

    m_audioSampleToTimeOffset += audioSamplesToTimeDelta(audioSamples);
}

void Synthesizer::trimInput(double startTimeOffset, bool move) {
    const double index = timeOffsetToInputSample(startTimeOffset);
    const double lowerBound = std::floor(index);

    if (lowerBound < 0) return;

    if (move) {
        for (int i = 0; i < m_inputChannelCount; ++i) {
            memmove(
                m_inputChannels[i].Data,
                m_inputChannels[i].Data + (int)lowerBound,
                sizeof(double) * (m_inputWriteOffset - (size_t)lowerBound));
        }
    }

    const double timeOffsetIndex = inputSampleToTimeOffset(index);
    const double timeOffset0 = inputSampleToTimeOffset(0.0);
    m_audioSampleToTimeOffset -= lowerBound / m_inputSampleRate;

    m_inputWriteOffset -= (int)lowerBound;
    m_inputSafetyBoundary -= (int)lowerBound;
}

int16_t Synthesizer::renderAudio(double timeOffset) {
    const double d0 = sampleInput(timeOffset, 0) * 10 - temp_prev[0];
    temp_prev[0] = sampleInput(timeOffset, 0) * 10;

    const double d1 = sampleInput(timeOffset, 1) * 10 - temp_prev[1];
    temp_prev[1] = sampleInput(timeOffset, 1) * 10;

    return temp_filter_0.f(d0 * 50) + temp_filter_1.f(d1 * 50);

    return sampleInput(timeOffset, 0) * 10;
}
