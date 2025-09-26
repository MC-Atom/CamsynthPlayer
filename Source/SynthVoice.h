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
#include "SynthSound.h"

class SynthVoice : public juce::SynthesiserVoice {
public:
  bool canPlaySound (juce::SynthesiserSound * sound) override;
  void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) override;
  void stopNote (float velocity, bool allowTailOff) override;
  void pitchWheelMoved (int newPitchWheelValue) override;
  void controllerMoved (int controllerNumber, int newControllerValue) override;
  void prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels, juce::ADSR::Parameters adsrParams, const std::shared_ptr<juce::AudioBuffer<float>> &nextWaveform,  std::atomic<bool>* readyToSwap);
  void renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;

  juce::ADSR adsr;
  juce::ADSR::Parameters adsrParams;

private:
  const float ALPHA = 0.4;
  juce::AudioBuffer<float> renderBuffer;
  juce::AudioBuffer<float> currentWaveform;
  std::shared_ptr<juce::AudioBuffer<float>> nextWaveform;
  std::atomic<bool>*readyToSwap = nullptr;
  double tailOff = 0.0;



  juce::dsp::Oscillator<float> osc;
  juce::dsp::Gain<float> gain;
  bool isPrepared = false;

  float phase = 0.0f;
  float phaseIncrement = 0.0f;
  float level = 0.0f;
  float freq = 440.0f;
  double sampleRate = 44100.0;
    
};
