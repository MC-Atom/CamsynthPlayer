/*
  ==============================================================================

    SynthVoice.cpp
    Created: 21 Jun 2025 2:37:47pm
    Author:  Devyn Oh

  ==============================================================================
*/

#include "SynthVoice.h"
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

  currentWaveform = juce::AudioBuffer<float>();
  int waveformSize = this->nextWaveform->getNumSamples();
  currentWaveform.setSize(1,waveformSize);

  // Initialize to a sin wave of amplitude 1
  currentWaveform.clear();
  float* writePtr = currentWaveform.getWritePointer(0);
  for (int i = 0; i < waveformSize; ++i)
  {
      float value = std::sin(juce::MathConstants<float>::twoPi * i / (float)waveformSize);
    writePtr[i] = value;
  }


  isPrepared = true;
}


void SynthVoice::renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) {
  jassert(isPrepared);

  if (readyToSwap->load(std::memory_order_acquire))
  {
      currentWaveform.makeCopyOf(*nextWaveform);
      readyToSwap->store(false, std::memory_order_release);
  }

  juce::dsp::AudioBlock<float> audioBlock { outputBuffer };

  // Make sure the current waveform is valid
  if (currentWaveform.getNumSamples() == 0)
    return;

//  // Get the read only pointer to the waveform
//  auto* wavetable = currentWaveform.getReadPointer(0);
//  int tableSize = currentWaveform.getNumSamples();
//  float tableSizeF = static_cast<float>(tableSize);
//
//  float phaseIncrement = tableSizeF / (static_cast<float>(sampleRate) / freq);
//
//  // For every sample in the buffer
//  for (int sample = 0; sample < numSamples; ++sample)
//  {
//    // Find sample by finding the current phase it's at and multiplying that by the table size
//    int index = static_cast<int>(phase * tableSize) % tableSize;
//    float value = wavetable[index];
//
//    // Write sample to all output channels
//    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
//      outputBuffer.addSample(channel, startSample + sample, value);
//
//    // Increment phase for the next sample
//    phase += phaseIncrement;
//    if (phase >= tableSizeF)
//      phase -= tableSizeF;
//  }
    
    auto tableSizeOverSampleRate = (float) currentWaveform.getNumSamples() / sampleRate;
        auto tableDelta = freq * tableSizeOverSampleRate;
    
    
         
    
        for (auto sample = 0; sample < numSamples; ++sample)
            {
                auto tableSize = (unsigned int) currentWaveform.getNumSamples();
                    auto index0 = (unsigned int) phase; // [6]
                    auto index1 = index0 == (tableSize - 1) ? (unsigned int) 0 : index0 + 1;
                    auto frac = phase - (float) index0; // [7]
                    auto* table = currentWaveform.getReadPointer (0); // [8]
                    auto value0 = table[index0];
                    auto value1 = table[index1];
                    auto currentSample = value0 + frac * (value1 - value0); // [9]
                phase = std::fmod(phase + tableDelta, (float) tableSize);
                for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
                outputBuffer.addSample(channel, startSample + sample, currentSample);
            }
        

  // Control the gain
  gain.process (juce::dsp::ProcessContextReplacing<float> (audioBlock));
  // Control the ASDR
  adsr.applyEnvelopeToBuffer(outputBuffer, startSample, numSamples);
}
