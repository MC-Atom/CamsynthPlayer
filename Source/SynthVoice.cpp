/*
  ==============================================================================

    SynthVoice.cpp
    Created: 21 Jun 2025 2:37:47pm
    Author:  Devyn Oh

  ==============================================================================
*/

#include "SynthVoice.h"
#include <utility>

std::shared_ptr<juce::AudioBuffer<float>> SynthVoice::currentWaveform = std::make_shared<juce::AudioBuffer<float>>();

bool SynthVoice::canPlaySound (juce::SynthesiserSound * sound) {
  return dynamic_cast<juce::SynthesiserSound*>(sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) {
  freq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
  adsr.setParameters(adsrParams);
  tailOff = 0.0;
  adsr.noteOn();
}
void SynthVoice::stopNote (float velocity, bool allowTailOff) {
  adsr.noteOff();
  //if (! allowTailOff || ! adsr.isActive())
  //  clearCurrentNote();

  if (allowTailOff)
  {
    clearCurrentNote();
    // start a tail-off by setting this flag. The render callback will pick up on
    // this and do a fade out, calling clearCurrentNote() when it's finished.

    if (juce::approximatelyEqual (tailOff, 0.0)) // we only need to begin a tail-off if it's not already doing so - the
      tailOff = 1.0;                     // stopNote method could be called more than once.
  }
  else
  {
    // we're being told to stop playing immediately, so reset everything..
    clearCurrentNote();
    phase = 0.0f;
  }

}
void SynthVoice::pitchWheelMoved (int newPitchWheelValue) {

}
void SynthVoice::controllerMoved (int controllerNumber, int newControllerValue) {

}
void SynthVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels, juce::ADSR::Parameters asdrParams,  const std::shared_ptr<juce::AudioBuffer<float>> &nextWaveform,std::atomic<bool>* readyToSwap) {

  this->nextWaveform = nextWaveform;
  this->readyToSwap = readyToSwap;

  this->sampleRate = sampleRate;

  this->adsrParams = asdrParams;

  //this->adsrParams.release = 5;
  //this->adsrParams.attack = 0.1;
  //this->adsrParams.decay = 0.3;
  //this->adsrParams.sustain = 0.5;

  adsr.setSampleRate(sampleRate);
  adsr.setParameters(asdrParams);
  //adsrParams.attack = 0.01;
  //adsrParams.release = 0.1;

  juce::dsp::ProcessSpec spec;
  spec.maximumBlockSize = samplesPerBlock;
  spec.sampleRate = sampleRate;
  spec.numChannels = outputChannels;

  gain.prepare(spec);
  gain.setGainDecibels(-6);

  //currentWaveform = std::make_shared<juce::AudioBuffer<float>>();

  int waveformSize = this->nextWaveform->getNumSamples();
  currentWaveform->setSize(nextWaveform->getNumChannels(),waveformSize);

  // Initialize to a sin wave of amplitude 1
  currentWaveform->clear();
  {
    float* writePtr = currentWaveform->getWritePointer(0);
    float* channel2 = currentWaveform->getWritePointer(1);
    for (int i = 0; i < waveformSize; ++i) {
        float value = std::sin(juce::MathConstants<float>::twoPi * i / (float)waveformSize);
      writePtr[i] = value;
      channel2[i] = value;
    }
  }


  isPrepared = true;
}


void SynthVoice::renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) {
  jassert(isPrepared);

  //if (!adsr.isActive())
  //    return;

  auto numChannels = outputBuffer.getNumChannels();

  // Ensure temp buffer is sized for this block
  if (renderBuffer.getNumChannels() != numChannels || renderBuffer.getNumSamples() < numSamples)
      renderBuffer.setSize ((int) numChannels, numSamples, false, true, true);

  renderBuffer.clear();

  if (readyToSwap->load(std::memory_order_acquire))
  {
      currentWaveform->makeCopyOf(*nextWaveform);
      readyToSwap->store(false, std::memory_order_release);
  }

  juce::AudioBuffer<float> waveform;
  waveform.makeCopyOf(*currentWaveform);

  juce::dsp::AudioBlock<float> audioBlock { outputBuffer };

  // Make sure the current waveform is valid
  if (waveform.getNumSamples() == 0)
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
    
    auto tableSizeOverSampleRate = (float) waveform.getNumSamples() / sampleRate;
    auto tableDelta = freq * tableSizeOverSampleRate;


    auto tableSize = (unsigned int) waveform.getNumSamples();
    for (auto sample = 0; sample < numSamples; ++sample) {
      for (int channel = 0; channel < waveform.getNumChannels(); channel++) {
        auto* table = waveform.getReadPointer (channel);
        auto index0 = (unsigned int) phase;
        auto index1 = index0 == (tableSize - 1) ? (unsigned int) 0 : index0 + 1;
        auto frac = phase - (float) index0;
        auto value0 = table[index0];
        auto value1 = table[index1];
        auto currentSample = value0 + frac * (value1 - value0);

        renderBuffer.addSample(channel, sample, currentSample);
      }
      phase = std::fmod(phase + tableDelta, (float) tableSize);
    }
        

  {
    juce::dsp::AudioBlock<float> block { juce::dsp::AudioBlock<float> (renderBuffer) };
    gain.process (juce::dsp::ProcessContextReplacing<float> (block));
  }
  // Control the ADSR
  adsr.applyEnvelopeToBuffer(renderBuffer, 0, numSamples);

  if (tailOff > 0.0) {
    for (auto sample = 0; sample < numSamples; ++sample) {
      for (int channel = 0; channel < numChannels; ++channel)
        renderBuffer.setSample(channel, sample, renderBuffer.getSample(channel, sample) * tailOff);

      tailOff *= 0.999;

      if (tailOff <= 0.005)
      {
        clearCurrentNote();
        phase = 0.0;
        break;
      }
    }
  }

  for (int ch = 0; ch < renderBuffer.getNumChannels(); ++ch)
  {
    outputBuffer.addFrom (ch, 0, renderBuffer.getReadPointer (ch), numSamples, 1.0f);
  }
}
