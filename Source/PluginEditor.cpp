/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    audioProcessor.socketServer->onSocketStatusChange = [this]() {
        repaint();
    };

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor() = default;

//==============================================================================
void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));

    std::string socketText;
    switch (audioProcessor.isSocketConnected.load()) {
        case 0:
            socketText = "Waiting to Connect";
            break;
        case 1:
            socketText = "Listening";
            break;
        case 2:
            socketText = "Receiving";
            break;
        case -1:
            socketText = "Failed to Connect";
            break;
        default:
            socketText = "Unknown Error";
    }
    g.drawFittedText (socketText, getLocalBounds(), juce::Justification::centred, 1);



}

void NewProjectAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
