/*
  ==============================================================================

    SynthSound.h
    Created: 21 Jun 2025 2:37:58pm
    Author:  Devyn Oh

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SynthSound : public juce::SynthesiserSound {
  
public:
    bool appliesToNote (int midiNoteNumber) override { return true;}
    bool appliesToChannel (int midiChannel) override { return true;}

private:

};
