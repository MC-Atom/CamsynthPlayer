/*
  ==============================================================================

    SocketServer.h
    Created: 18 Jul 2025 9:16:59pm
    Author:  Devyn Oh

  ==============================================================================
*/

#pragma once
#include <atomic>
#include <cstdio>
#include <memory>

#include <JuceHeader.h>

#define MAX_URL_SIZE 20000

class SocketServer final : public juce::Thread {
public:
  std::atomic<bool>*readyToSwap;
  std::atomic<int>*synthMode;
  std::atomic<float>*bias;
  std::shared_ptr<juce::AudioBuffer<float>> nextWaveform;
  juce::Label *httpState;

  SocketServer (std::atomic<bool>*readyToSwap_input, std::atomic<int>*synthMode_input, std::atomic<float>*bias_input,
    const std::shared_ptr<juce::AudioBuffer<float>> &nextWaveform_input,
    juce::Label *httpState_input)
  : Thread("Socket Server") {

    readyToSwap = readyToSwap_input;
    synthMode = synthMode_input;
    bias = bias_input;
    nextWaveform = nextWaveform_input;
    httpState = httpState_input;
    startThread();
  }

  ~SocketServer() override
  {
    // allow the thread 1 second to stop cleanly - should be plenty of time.
    stopThread (1000);
  }

  void run() override;

  std::function<void()> onSocketStatusChange;

private:
  static juce::AudioBuffer<float> processJsonDropNeg(const char* body, int wavetableNumChannels, int wavetableSize, float bias);
  static juce::AudioBuffer<float> processJsonCombineNeg(const char* body, int wavetableNumChannels, int wavetableSize, float bias);
  static juce::AudioBuffer<float> processJsonStereo(const char* body, int wavetableNumChannels, int wavetableSize, float bias);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SocketServer)

  ;
};
