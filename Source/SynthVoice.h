/*
  ==============================================================================

    SynthVoice.h
    Created: 21 Jun 2025 2:37:47pm
    Author:  Devyn Oh

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <opencv2/opencv.hpp>
#include "SynthSound.h"

class SynthVoice : public juce::SynthesiserVoice {
public:
  bool canPlaySound (juce::SynthesiserSound * sound) override;
  void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) override;
  void stopNote (float velocity, bool allowTailOff) override;
  void pitchWheelMoved (int newPitchWheelValue) override;
  void controllerMoved (int controllerNumber, int newControllerValue) override;
  void prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels, const std::shared_ptr<juce::AudioBuffer<float>> &nextWaveform,  std::atomic<bool>* readyToSwap);
  void renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;
private:
  const float ALPHA = 0.4;
  std::shared_ptr<juce::AudioBuffer<float>> currentWaveform;
  std::shared_ptr<juce::AudioBuffer<float>> nextWaveform;
  std::atomic<bool>*readyToSwap = nullptr;

  juce::ADSR adsr;
  juce::ADSR::Parameters adsrParams;

  juce::dsp::Oscillator<float> osc;
  juce::dsp::Gain<float> gain;
  bool isPrepared = false;

  float phase = 0.0f;
  float phaseIncrement = 0.0f;
  float level = 0.0f;
  float freq = 440.0f;
  double sampleRate = 44100.0;
    
};
