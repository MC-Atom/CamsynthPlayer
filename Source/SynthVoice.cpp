/*
  ==============================================================================

    SynthVoice.cpp
    Created: 21 Jun 2025 2:37:47pm
    Author:  Devyn Oh

  ==============================================================================
*/

#include "SynthVoice.h"
#include <opencv2/xphoto.hpp>
#include <utility>

bool SynthVoice::canPlaySound (juce::SynthesiserSound * sound) {
  return dynamic_cast<juce::SynthesiserSound*>(sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) {
  freq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
  adsr.noteOn();
}
void SynthVoice::stopNote (float velocity, bool allowTailOff) {
  adsr.noteOff();
}
void SynthVoice::pitchWheelMoved (int newPitchWheelValue) {

}
void SynthVoice::controllerMoved (int controllerNumber, int newControllerValue) {

}
void SynthVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels, const std::shared_ptr<juce::AudioBuffer<float>> &nextWaveform,std::atomic<bool>* readyToSwap) {

  this->nextWaveform = nextWaveform;
  this->readyToSwap = readyToSwap;

  this->sampleRate = sampleRate;
  adsr.setSampleRate(sampleRate);

  juce::dsp::ProcessSpec spec;
  spec.maximumBlockSize = samplesPerBlock;
  spec.sampleRate = sampleRate;
  spec.numChannels = outputChannels;

  gain.prepare(spec);
  gain.setGainLinear(0.01f);

  isPrepared = true;
}


void SynthVoice::renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) {
  jassert(isPrepared);

  if (readyToSwap->load(std::memory_order_acquire))
  {
      currentWaveform = nextWaveform;
      readyToSwap->store(false, std::memory_order_release);
  }

  juce::dsp::AudioBlock<float> audioBlock { outputBuffer };

  // Make sure the current waveform is valid
  if (!currentWaveform || currentWaveform->getNumSamples() == 0)
    return;

  // Get the read only pointer to the waveform
  auto* table = currentWaveform->getReadPointer(0);
  int tableSize = currentWaveform->getNumSamples();

  // For every sample in the buffer
  for (int sample = 0; sample < numSamples; ++sample)
  {
    // Find sample by finding the current phase it's at and multiplying that by the table size
    int index = static_cast<int>(phase * tableSize) % tableSize;
    float value = table[index];

    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
      outputBuffer.addSample(channel, startSample + sample, value);

    // Increment phase for the next sample
    phase += freq/sampleRate;
    if (phase >= 1.0f)
      phase -= 1.0f;
  }

  // Control the gain
  gain.process (juce::dsp::ProcessContextReplacing<float> (audioBlock));
  // Control the ASDR
  adsr.applyEnvelopeToBuffer(outputBuffer, startSample, numSamples);
}