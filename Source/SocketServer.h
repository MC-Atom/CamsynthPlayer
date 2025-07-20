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

#define MAX_URL_SIZE 2000

class SocketServer final : public juce::Thread {
public:
  std::atomic<bool>*readyToSwap;
  std::shared_ptr<juce::AudioBuffer<float>> nextWaveform;
  std::atomic<int>*isSocketConnected;

  SocketServer (std::atomic<bool>*readyToSwap_input,
    const std::shared_ptr<juce::AudioBuffer<float>> &nextWaveform_input,
    std::atomic<int>*socketText)
  : Thread("Socket Server") {

    readyToSwap = readyToSwap_input;
    nextWaveform = nextWaveform_input;
    isSocketConnected = socketText;
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
  static juce::AudioBuffer<float> processJson(char* body, int wavetableSize);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SocketServer);
};
