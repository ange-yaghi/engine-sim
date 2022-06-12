#include "..\include\synthesizer.h"
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
    waveFile0.OpenFile("../assets/test_engine_14_eq_adjusted_16.wav"); // test_engine_14_eq_adjusted_16.wav
    waveFile0.InitializeInternalBuffer(waveFile0.GetSampleCount());
    waveFile0.FillBuffer(0);
    waveFile0.CloseFile();

    ysWindowsAudioWaveFile waveFile1;
    waveFile1.OpenFile("../assets/test_engine_15_eq_adjusted_16.wav"); // ../assets/test_engine_16.wav
    waveFile1.InitializeInternalBuffer(waveFile1.GetSampleCount());
    waveFile1.FillBuffer(0);
    waveFile1.CloseFile();

    const unsigned int sampleCount0 = std::min((unsigned int)10000, waveFile0.GetSampleCount());
    const unsigned int sampleCount1 = std::min((unsigned int)10000, waveFile1.GetSampleCount());

    temp_filter_0.initialize(sampleCount0);
    for (int i = 0; i < sampleCount0; ++i) {
        temp_filter_0.getImpulseResponse()[i] = 2 * 0.025 * ((int16_t *)waveFile0.GetBuffer())[i] / INT16_MAX;
    }

    temp_filter_1.initialize(sampleCount1);
    for (int i = 0; i < sampleCount1; ++i) {
        temp_filter_1.getImpulseResponse()[i] = 0.1 * 2 * 0.025 * ((int16_t *)waveFile1.GetBuffer())[i] / INT16_MAX;
    }

    m_levelingFilter.p_target = 40000;
    m_levelingFilter.p_maxLevel = 10.0;
    m_levelingFilter.p_minLevel = 0.0001;
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

    temp_filter_0.destroy();
    temp_filter_1.destroy();
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
    return (sample / m_audioSampleRate) + m_audioSampleToTimeOffset;
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
    const double v0 = data[index0];
    const double v1 = data[index1];

    const double r = 0.5 * 0.5 * (((double)rand() / RAND_MAX) - 0.5);
    const double s_aug = std::fmax(std::fmin(s + r, 1.0), 0.0);

    if (std::abs(data[index1] - data[index0]) > 10000) {
        //return data[index0];
    }

    return (1 - s_aug) * data[index0] + s_aug * data[index1];
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

void Synthesizer::setInputSampleRate(double sampleRate) {
    if (sampleRate != m_inputSampleRate) {
        std::lock_guard<std::mutex> lock(m_lock0);
        m_inputSampleRate = sampleRate;

        m_inputWriteOffset = 0;
        m_inputSafetyBoundary = -1;
        m_audioSampleToTimeOffset = 0.0;
        m_audioBufferWriteOffset = 0;

        const double offset = audioSampleToTimeOffset(m_audioBufferWriteOffset);
        m_audioSampleToTimeOffset = -offset;
    }
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

#undef max

int16_t Synthesizer::renderAudio(double timeOffset) {
    const double d0 = sampleInput(timeOffset, 0) * 10 - temp_prev[0];
    temp_prev[0] = sampleInput(timeOffset, 0) * 10;

    const double d1 = sampleInput(timeOffset, 1) * 10 - temp_prev[1];
    temp_prev[1] = sampleInput(timeOffset, 1) * 10;

    const double r = 2.0 * ((double)rand() / RAND_MAX - 0.5);
    const double mix = 0.2;

    static double r_filtered = 0.0;
    r_filtered = 0.5 * r_filtered + 0.5 * r;

    //return 50 * (d0 + d1);
    //return temp_filter_0.f(std::min(0.0, d0 * 50)) + temp_filter_1.f(std::min(0.0, d1 * 50));

    //double v0 = 0.0, v1 = 0.0;
    //if ((rand() % 100000) == 0) {
    //    v0 = INT16_MAX * 0.5;
    //}
    //if ((rand() % 10000) == 0) {
     //   v1 = INT16_MAX * 0.5;
    //}

    const double v =
        temp_filter_0.f(d0 * 50 + temp_prev[0] * r_filtered * mix)
        + temp_filter_1.f(d1 * 50 + temp_prev[1] * r_filtered * mix);
    //const double v = temp_filter_0.f(v0) + temp_filter_1.f(v1);
    //const double v = temp_filter_0.f(temp_prev[0] * r) + temp_filter_1.f(temp_prev[1] * r);
    //const double v =
    //    temp_filter_0.f(0.1 * temp_prev[0] * r)
    //    + temp_filter_1.f(0.1 * temp_prev[1] * r);
    //const double v = temp_filter_0.f(temp_prev[0]) + temp_filter_1.f(temp_prev[1]);
    //const double v = temp_prev[0] + temp_prev[1];
    //const double v = d0 + d1;
    const double amplitude = std::abs(v);
    //return v;
    double v0 = m_levelingFilter.f(v * 0.08 * 12);
    
    int r_int = std::lround(v0);
    if (r_int > INT16_MAX) {
        r_int = INT16_MAX;
    }
    else if (r_int < INT16_MIN) {
        r_int = INT16_MIN;
    }

    return (int16_t)r_int;

    double log_v = std::log((amplitude * 0.0001) + 1) * 15000;
    log_v = (v0 < 0) ? -log_v : log_v;

    return log_v * 0.5;

    return sampleInput(timeOffset, 0) * 10;
    //return (d0 + d1) * 10;
}
