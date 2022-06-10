#ifndef ATG_ENGINE_SIM_ENGINE_SYNTHESIZER_H
#define ATG_ENGINE_SIM_ENGINE_SYNTHESIZER_H

#include "convolution_filter.h"
#include "leveling_filter.h"

#include <cinttypes>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

class Synthesizer {
    public:
        struct Parameters {
            int InputChannelCount;
            int InputBufferSize;
            int AudioBufferSize;
            double InputSampleRate;
            double AudioSampleRate;
        };

        struct InputChannel {
            double *Data;
        };

    public:
        Synthesizer();
        ~Synthesizer();

        void initialize(const Parameters &p);
        void startAudioRenderingThread();
        void endAudioRenderingThread();
        void destroy();

        int readAudioOutput(int samples, int16_t *buffer);

        void writeInput(const double *data);
        void endInputBlock();

        void waitProcessed();

        void audioRenderingThread();
        void renderAudio();

        double audioSampleToTimeOffset(int sample) const;
        double audioSamplesToTimeDelta(int samples) const;
        double timeOffsetToInputSample(double timeOffset) const;
        double inputSampleToTimeOffset(double inputSample) const;
        double sampleInput(double timeOffset, int channel) const;

        bool timeOffsetGreaterZero(double timeOffset) const;
        bool timeOffsetInBounds(double timeOffset, int safetyBoundary) const;

        void trim(int audioSamples);
        void trimInput(double startTimeOffset, bool move=true);
        int16_t renderAudio(double timeOffset);

        int getInputWriteOffset() const { return m_inputWriteOffset; }

    protected:
        LevelingFilter m_levelingFilter;
        InputChannel *m_inputChannels;
        int m_inputChannelCount;
        int m_inputBufferSize;
        int m_inputWriteOffset;
        int m_inputSafetyBoundary;

        int16_t *m_audioBuffer;
        int m_audioBufferSize;
        int m_audioBufferWriteOffset;
        int m_audioBufferSafetyBoundary;
        double m_audioSampleToTimeOffset;

        double m_inputSampleRate;
        double m_audioSampleRate;

        std::thread *m_thread;
        std::atomic<bool> m_run;
        bool m_processed;

        std::mutex m_lock0;
        std::condition_variable m_cv0;

        ConvolutionFilter temp_filter_0;
        ConvolutionFilter temp_filter_1;
        double temp_prev[2] = { 0, 0 };
};

#endif /* ATG_ENGINE_SIM_ENGINE_SYNTHESIZER_H */
