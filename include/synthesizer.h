#ifndef ATG_ENGINE_SIM_ENGINE_SYNTHESIZER_H
#define ATG_ENGINE_SIM_ENGINE_SYNTHESIZER_H

#include "convolution_filter.h"
#include "leveling_filter.h"
#include "derivative_filter.h"
#include "low_pass_filter.h"

#include <cinttypes>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

class Synthesizer {
    public:
        struct AudioParameters {
            double Volume = 1.0;
            double Convolution = 1.0;
            double dF_F_mix = 0.01; //0.00990099
            double InputSampleNoise = 0.5; //0.125
            double InputSampleNoiseFrequencyCutoff = 80000.0;
            double AirNoise = 1.0;
            double AirNoiseFrequencyCutoff = 10000.0;
            double LevelerTarget = 30000;
            double LevelerMaxGain = 1.9;
            double LevelerMinGain = 0.00001;
        };

        struct Parameters {
            int InputChannelCount;
            int InputBufferSize;
            int AudioBufferSize;
            double InputSampleRate;
            double AudioSampleRate;
            AudioParameters InitialAudioParameters;
        };

        struct InputChannel {
            double *Data;
        };

        struct ProcessingFilters {
            ConvolutionFilter Convolution;
            DerivativeFilter Derivative;
            LowPassFilter AirNoiseLowPass;
            LowPassFilter SampleNoiseLowPass;
            LowPassFilter InputDcFilter;
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

        void setInputSampleRate(double sampleRate);
        double getInputSampleRate() const { return m_inputSampleRate; }

        void trim(int audioSamples);
        void trimInput(double startTimeOffset, bool move=true);
        int16_t renderAudio(double timeOffset);

        int getInputWriteOffset() const { return m_inputWriteOffset; }

        double getLevelerGain();
        AudioParameters getAudioParameters();
        void setAudioParameters(const AudioParameters &params);

    protected:
        LevelingFilter m_levelingFilter;
        InputChannel *m_inputChannels;
        AudioParameters m_audioParameters;
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

        ProcessingFilters *m_filters;
};

#endif /* ATG_ENGINE_SIM_ENGINE_SYNTHESIZER_H */
