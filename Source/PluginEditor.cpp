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

    int a = audioProcessor.isSocketConnected.load();

    std::string socketText;
    int port = -1;
    if (a == 0) {socketText = "Waiting to Connect";}
    else if (a >= 8080) {
        port = a;
        socketText = "Listening on port " + std::to_string(port);
    } else if (a == 2) {socketText = "Receiving on port " + std::to_string(port);}
    else if (a == -1){ socketText = "Failed to Connect";}
    else { socketText = "Unknown Error " + std::to_string(a);}

    g.drawFittedText (socketText, getLocalBounds(), juce::Justification::centred, 1);



}

void NewProjectAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
