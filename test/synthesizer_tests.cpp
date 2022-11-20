#include <gtest/gtest.h>

#include "../include/synthesizer.h"

#include <chrono>

using namespace std::chrono_literals;

void setupStandardSynthesizer(Synthesizer &synth) {
    Synthesizer::Parameters params;
    params.audioBufferSize = 512 * 16;
    params.audioSampleRate = 16;
    params.inputBufferSize = 256;
    params.inputChannelCount = 8;
    params.inputSampleRate = 32;

    Synthesizer::AudioParameters audioParams;
    audioParams.airNoise = 0.0;
    audioParams.inputSampleNoise = 0.0;
    audioParams.levelerMaxGain = 1.0;
    audioParams.levelerMinGain = 1.0;
    audioParams.dF_F_mix = 0.0;
    params.initialAudioParameters = audioParams;

    synth.initialize(params);
}

void setupSynchronizedSynthesizer(Synthesizer &synth) {
    Synthesizer::Parameters params;
    params.audioBufferSize = 512 * 16;
    params.audioSampleRate = 32;
    params.inputBufferSize = 1024;
    params.inputChannelCount = 8;
    params.inputSampleRate = 32;

    Synthesizer::AudioParameters audioParams;
    audioParams.airNoise = 0.0;
    audioParams.inputSampleNoise = 0.0;
    audioParams.levelerMaxGain = 1.0;
    audioParams.levelerMinGain = 1.0;
    audioParams.dF_F_mix = 0.0;
    params.initialAudioParameters = audioParams;

    synth.initialize(params);
}

TEST(SynthesizerTests, SynthesizerSanityCheck) {
    Synthesizer synth;
    setupStandardSynthesizer(synth);
    synth.destroy();
}
/*
TEST(SynthesizerTests, SynthesizerConversionTest) {
    Synthesizer synth;
    setupStandardSynthesizer(synth);

    EXPECT_NEAR(synth.inputSampleToTimeOffset(0.0), 0.0, 1E-6);
    EXPECT_NEAR(synth.inputSampleToTimeOffset(1.0), 1 / 32.0, 1E-6);

    EXPECT_NEAR(synth.audioSampleToTimeOffset(0), -0.5, 1E-6);

    synth.destroy();
}

TEST(SynthesizerTests, SynthesizerTrimTest) {
    Synthesizer synth;
    setupStandardSynthesizer(synth);

    const double timeOffset0 = synth.audioSampleToTimeOffset(0);

    synth.trimInput(0.5, false);

    const double timeOffset1 = synth.audioSampleToTimeOffset(8);

    EXPECT_NEAR(timeOffset1, timeOffset0, 1E-6);

    synth.destroy();
}

TEST(SynthesizerTests, SynthesizerSampleTest) {
    Synthesizer synth;
    setupStandardSynthesizer(synth);

    for (int i = 0; i < 1024; ++i) {
        const double v = (double)i;
        const double data[] = { v, v, v, v, v, v, v, v };
        synth.writeInput(data);
    }

    const double end_t = 1023 / 32.0;

    const double v0 = synth.sampleInput(end_t, 0);
    const double v1 = synth.sampleInput(end_t - 1 / 64.0, 0);

    EXPECT_NEAR(v0, 1023.0, 1E-6);
    EXPECT_NEAR(v1, 1022.5, 1E-6);

    synth.trimInput(0.5);

    const double v0_trim = synth.sampleInput(end_t, 0);
    const double v1_trim = synth.sampleInput(end_t - 1 / 64.0, 0);

    EXPECT_NEAR(v0, v0_trim, 1E-6);
    EXPECT_NEAR(v1, v1_trim, 1E-6);

    synth.destroy();
}
*/

TEST(SynthesizerTests, SynthesizerSystemTestSingleThread) {
    constexpr int inputSamples = 64;
    constexpr int outputSamples = 63;

    Synthesizer synth;
    setupSynchronizedSynthesizer(synth);

    int16_t *output = new int16_t[outputSamples];
    int totalSamples = 0;

    for (int i = 0; i < inputSamples;) {
        for (int j = 0; j < 16; ++j, ++i) {
            const double v = (double)i;
            const double data[] = { v, v, v, v, v, v, v, v };
            synth.writeInput(data);
        }

        synth.endInputBlock();
        synth.renderAudio();

        totalSamples += synth.readAudioOutput(16, output + totalSamples);
        int a = 0;
    }

    const int rem = synth.readAudioOutput(outputSamples - totalSamples, output + totalSamples);

    EXPECT_EQ(rem, outputSamples - totalSamples);

    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(output[i], 0);
    }

    for (int i = 16; i < outputSamples; ++i) {
        EXPECT_EQ(output[i], (i - 16) * 10 * 8);
    }

    synth.destroy();
    delete[] output;
}

TEST(SynthesizerTests, SynthesizerSystemTest) {
    constexpr int inputSamples = 1024;
    constexpr int outputSamples = 1023;

    Synthesizer synth;
    setupSynchronizedSynthesizer(synth);
    synth.startAudioRenderingThread();

    int16_t *output = new int16_t[outputSamples];
    int totalSamples = 0;

    for (int i = 0; i < inputSamples;) {
        for (int j = 0; j < 16; ++j, ++i) {
            const double v = (double)i;
            const double data[] = { v, v, v, v, v, v, v, v };
            synth.writeInput(data);
        }

        const int samplesReturned = synth.readAudioOutput(8, output + totalSamples);
        totalSamples += samplesReturned;
    }

    synth.endInputBlock();
    synth.waitProcessed();

    const int rem = synth.readAudioOutput(outputSamples - totalSamples, output + totalSamples);
    EXPECT_EQ(rem, outputSamples - totalSamples);

    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(output[i], 0);
    }

    for (int i = 16; i < outputSamples; ++i) {
        EXPECT_EQ(output[i], std::min(32767, (i - 16) * 10 * 8));
    }

    synth.endAudioRenderingThread();
    synth.destroy();

    delete[] output;
}
