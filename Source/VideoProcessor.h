/*
  ==============================================================================

    VideoProcessing.h
    Created: 29 Jun 2025 12:35:13pm
    Author:  Devyn Oh

  ==============================================================================
*/

#pragma once
#include <functional>
#include <memory>
#include <opencv2/opencv.hpp>

#include <JuceHeader.h>

class VideoProcessor : public juce::Thread, private juce::CameraDevice::Listener {
public:
  VideoProcessor() : juce::Thread("VideoProcessor") {}

  void startCamera()
  {
    auto camList = juce::CameraDevice::getAvailableDevices();
    if (camList.isEmpty()) {
      DBG("No camera devices available");
      return;
    }

    camera = juce::CameraDevice::openDevice(camList[0],
        640, 480, this); // resolution & listener

    if (camera)
    {
      camera->startRecordingToImageFile(juce::File::getSpecialLocation(juce::File::tempDirectory)
                                         .getChildFile("temp.jpg"));
    }
  }

  void prepareToPlay(const std::shared_ptr<juce::AudioBuffer<float>> &nWaveform, const std::atomic<bool>* rToSwap) {
    nextWaveform = nWaveform;
    readyToSwap = rToSwap;
  }

  void run() override;
  void process(std::shared_ptr<juce::AudioBuffer<float>> nextWaveform, std::atomic<bool> readyToSwap);

private:
  std::shared_ptr<juce::AudioBuffer<float>> nextWaveform;
  const std::atomic<bool> *readyToSwap = nullptr;
  cv::VideoCapture camera;

  cv::Mat resized;
  cv::Mat inputFrame;
  static constexpr int targetWidth = 800;
  // Frame to sum with for smear
  cv::Mat accumulatedFrame;
  double scale = static_cast<double>(targetWidth) / originalWidth;
  int originalWidth = inputFrame.cols;
  int originalHeight = inputFrame.rows;
  // Calculate the scale ratio and new height
  int newHeight = static_cast<int>(originalHeight * scale);
};
