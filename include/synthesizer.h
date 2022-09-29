#ifndef ATG_ENGINE_SIM_ENGINE_SYNTHESIZER_H
#define ATG_ENGINE_SIM_ENGINE_SYNTHESIZER_H

#include "convolution_filter.h"
#include "leveling_filter.h"
#include "derivative_filter.h"
#include "low_pass_filter.h"
#include "jitter_filter.h"
#include "ring_buffer.h"
#include "butterworth_low_pass_filter.h"

#include <cinttypes>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

class Synthesizer {
    public:
        struct AudioParameters {
            float Volume = 1.0f;
            float Convolution = 1.0f;
            float dF_F_mix = 0.01f;
            float InputSampleNoise = 0.5f;
            float InputSampleNoiseFrequencyCutoff = 10000.0f;
            float AirNoise = 1.0f;
            float AirNoiseFrequencyCutoff = 2000.0f;
            float LevelerTarget = 30000.0f;
            float LevelerMaxGain = 1.9f;
            float LevelerMinGain = 0.00001f;
        };

        struct Parameters {
            int InputChannelCount = 1;
            int InputBufferSize = 1024;
            int AudioBufferSize = 44100;
            float InputSampleRate = 10000;
            float AudioSampleRate = 44100;
            AudioParameters InitialAudioParameters;
        };

        struct InputChannel {
            RingBuffer<float> Data;
            float *TransferBuffer = nullptr;
            double LastInputSample = 0.0f;
        };

        struct ProcessingFilters {
            ConvolutionFilter Convolution;
            DerivativeFilter Derivative;
            JitterFilter JitterFilter;
            ButterworthLowPassFilter<float> AirNoiseLowPass;
            LowPassFilter InputDcFilter;
            ButterworthLowPassFilter<double> antialiasing;
        };

    public:
        Synthesizer();
        ~Synthesizer();

        void initialize(const Parameters &p);
        void initializeImpulseResponse(
            const int16_t *impulseResponse,
            unsigned int samples,
            float volume,
            int index);
        void startAudioRenderingThread();
        void endAudioRenderingThread();
        void destroy();

        int readAudioOutput(int samples, int16_t *buffer);

        void writeInput(const double *data);
        void endInputBlock();

        void waitProcessed();

        void audioRenderingThread();
        void renderAudio();

        double getLatency() const;

        int inputDelta(int s1, int s0) const;
        double inputDistance(double s1, double s0) const;

        void setInputSampleRate(double sampleRate);
        double getInputSampleRate() const { return m_inputSampleRate; }

        int16_t renderAudio(int inputOffset);

        double getLevelerGain();
        AudioParameters getAudioParameters();
        void setAudioParameters(const AudioParameters &params);

    //protected:
        ButterworthLowPassFilter<float> m_antialiasing;
        LevelingFilter m_levelingFilter;
        InputChannel *m_inputChannels;
        AudioParameters m_audioParameters;
        int m_inputChannelCount;
        int m_inputBufferSize;
        int m_inputSamplesRead;
        int m_latency;
        double m_inputWriteOffset;
        double m_lastInputSampleOffset;

        RingBuffer<int16_t> m_audioBuffer;
        int m_audioBufferSize;

        float m_inputSampleRate;
        float m_audioSampleRate;

        std::thread *m_thread;
        std::atomic<bool> m_run;
        bool m_processed;

        std::mutex m_inputLock;
        std::mutex m_lock0;
        std::condition_variable m_cv0;

        ProcessingFilters *m_filters;
};

#endif /* ATG_ENGINE_SIM_ENGINE_SYNTHESIZER_H */
