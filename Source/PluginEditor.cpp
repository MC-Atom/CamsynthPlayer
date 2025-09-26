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

    addAndMakeVisible(audioProcessor.httpState);
    audioProcessor.httpState.setText("Waiting to Connect",juce::sendNotification);

    biasSlider.addListener(this);
    addAndMakeVisible(modeDropdown);
    addAndMakeVisible(biasSlider);
    modeDropdown.addItem("Drop Negatives",1);
    modeDropdown.addItem("Sum Negatives",2);
    modeDropdown.addItem("Stereo Negatives",3);
    modeDropdown.setSelectedId(1);
    modeDropdown.addListener(this);
    biasSlider.setRange(-1,1,0.01);

    addAndMakeVisible(modeLabel);
    addAndMakeVisible(biasLabel);
    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.attachToComponent(&modeDropdown,true);
    biasLabel.setText("Bias", juce::dontSendNotification);
    biasLabel.attachToComponent(&biasSlider,true);

    aSlider.addListener(this);
    sSlider.addListener(this);
    dSlider.addListener(this);
    rSlider.addListener(this);
    addAndMakeVisible(aSlider);
    addAndMakeVisible(sSlider);
    addAndMakeVisible(dSlider);
    addAndMakeVisible(rSlider);
    aSlider.setRange(0,2,0.01);
    sSlider.setRange(0,2,0.01);
    dSlider.setRange(0,2,0.01);
    rSlider.setRange(0,2,0.01);
    aSlider.setTextValueSuffix(" Sec");
    sSlider.setTextValueSuffix(" Sec");
    dSlider.setTextValueSuffix(" Sec");
    rSlider.setTextValueSuffix(" Sec");
    const double skewFactor = 0.6;
    aSlider.setSkewFactor(skewFactor);
    sSlider.setSkewFactor(skewFactor);
    dSlider.setSkewFactor(skewFactor);
    rSlider.setSkewFactor(skewFactor);


    addAndMakeVisible(aLabel);
    addAndMakeVisible(sLabel);
    addAndMakeVisible(dLabel);
    addAndMakeVisible(rLabel);

    aLabel.setText("A", juce::dontSendNotification);
    sLabel.setText("S", juce::dontSendNotification);
    dLabel.setText("D", juce::dontSendNotification);
    rLabel.setText("R", juce::dontSendNotification);
    aLabel.attachToComponent(&aSlider,true);
    sLabel.attachToComponent(&sSlider,true);
    dLabel.attachToComponent(&dSlider,true);
    rLabel.attachToComponent(&rSlider,true);


}

void NewProjectAudioProcessorEditor::sliderValueChanged (juce::Slider* slider) {
    if (slider == &aSlider) {
        audioProcessor.adsrParams.attack = slider->getValue();
    }
    if (slider == &rSlider) {
        audioProcessor.adsrParams.release = slider->getValue();
    }
    if (slider == &biasSlider) {
        //audioProcessor.adsrParams.attack = slider->getValue();
      audioProcessor.bias.store(slider->getValue());
        printf("Stored %f",audioProcessor.bias.load());
    }
}

void NewProjectAudioProcessorEditor::comboBoxChanged (juce::ComboBox* comboBox) {
    if (comboBox == &modeDropdown) {
        audioProcessor.synthMode.store(comboBox->getSelectedId());
        std::cout << "Mode: " << audioProcessor.synthMode.load() << std::endl;
    }
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor() = default;



//==============================================================================
void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));


}

void NewProjectAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    constexpr int padding = 20;

    audioProcessor.httpState.setBounds(padding, padding,getWidth()-(padding*2), 20);

    modeDropdown.setBounds(padding+30, padding*3, (getWidth()/2)-(padding*2)-30,20);
    biasSlider.setBounds((getWidth()/2)+padding, padding*3, (getWidth()/2)-(padding*2),20);

    aSlider.setBounds((getWidth()/2)+padding, padding*6, (getWidth()/2)-(padding*2),20);
    sSlider.setBounds((getWidth()/2)+padding, padding*8, (getWidth()/2)-(padding*2),20);
    dSlider.setBounds((getWidth()/2)+padding, padding*10, (getWidth()/2)-(padding*2),20);
    rSlider.setBounds((getWidth()/2)+padding, padding*12, (getWidth()/2)-(padding*2),20);

    aLabel.setBounds(padding, padding*4, (getWidth()/2)-(padding*2),20);
}
