# Camsynth Player

This is a Juce plugin that works in conjunction with the [Camsynth Sender](https://github.com/MC-Atom/CamsynthSender) to make music out of an outline in real time.

For a full demonstration and short walkthrough of how it works, check out my website: [https://devynoh.dev/portfolio/camsynth/](https://devynoh.dev/portfolio/camsynth/)

### Compiling and Running

This is a Juce application that doesn't have a compiled version on github. To compile this, download and compile through [Projucer](https://juce.com/download/). You may want to look at the [Juce getting started tutorial](https://juce.com/tutorials/tutorial_new_projucer_project/).
This will compile into VST and AU plugins that work in DAWs such as Logic Pro and Ableton.
This **NEEDS THE [CAMSYNTH SENDER](https://github.com/MC-Atom/CamsynthSender)** or it will just play a sine tone.

### See also:
* [Camsynth Sender](https://github.com/MC-Atom/CamsynthSender): The computation-heavy main application that drives this application
* [Camsynth Filter](https://github.com/MC-Atom/CamsynthFilter): A similar application to this that uses the ratio between the amplitudes of high and low frequencies to control a filter.
