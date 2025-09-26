/*
  ==============================================================================

    SocketServer.cpp
    Created: 18 Jul 2025 9:16:59pm
    Author:  Devyn Oh

  ==============================================================================
*/

// lsof -i :8080

#include "SocketServer.h"

#include <fstream>
#include <valarray>

#define InitPortNumber 8080
#define localHostName "0.0.0.0"

void SocketServer::run()
{
  while (!onSocketStatusChange) { // Make sure onSocketStatusChange is defined before continuing
    sleep(10);
  }

  const auto socket = std::make_unique<juce::StreamingSocket>();
  auto generatedWaveform = juce::AudioBuffer<float>();
  int portNumber = InitPortNumber;
  bool listening = socket->createListener(portNumber, localHostName);
  while (!listening) {
    portNumber += 1;
    listening = socket->createListener(portNumber, localHostName);
  }



  if (listening)
  {
    bool newWaveform = false;
    std::printf("Listening on %s:%d\n",localHostName,portNumber);
    if (true) {
      juce::MessageManagerLock mml (Thread::getCurrentThread());
      httpState->setText("Listening on port " + std::to_string(portNumber), juce::sendNotification);
    }

    while (!threadShouldExit())
    {
      std::unique_ptr<juce::StreamingSocket> connection(socket->waitForNextConnection());

      if (connection != nullptr) {
        char buffer[MAX_URL_SIZE] = {0};

        int bytesRead = connection->read(buffer, MAX_URL_SIZE - 1, false);
        if (bytesRead > 0) {
          buffer[bytesRead] = '\0';
          //std::printf("Received:\n%s\n", buffer);

          if (true) {
            juce::MessageManagerLock mml (Thread::getCurrentThread());
            httpState->setText("Recieving on port " + std::to_string(portNumber), juce::sendNotification);
          }

          // === EXTRACT BODY ===
          // crude way to extract JSON body (assumes POST)
          char* body = strstr(buffer, "\r\n\r\n");

          // Process the json into the wavetable we want
          switch (synthMode->load()) {
            case 1: // If in mode 0, drop negative harmonic values
              generatedWaveform = processJsonDropNeg(body,nextWaveform->getNumChannels(), nextWaveform->getNumSamples(), bias->load());
              break;
            case 2: // If in mode 1, combine the pos and neg values into mono
              generatedWaveform = processJsonCombineNeg(body,nextWaveform->getNumChannels(), nextWaveform->getNumSamples(), bias->load());
              break;
            case 3: //If in mode 2, map neg values to the right channel, pos to the left
              generatedWaveform = processJsonStereo(body,nextWaveform->getNumChannels(), nextWaveform->getNumSamples(), bias->load());
              std::cout << "hi" << std::endl;
              break;
            default: // Go to case 0 by default
              generatedWaveform = processJsonDropNeg(body,nextWaveform->getNumChannels(), nextWaveform->getNumSamples(), bias->load());
          }

          newWaveform = true;

          // Send minimal HTTP 200 OK back to client
          const char* response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
          connection->write(response, static_cast<int>(strlen(response)));
        }
      connection->close();
      }
      // Apply to shared buffer (guarded by readyToSwap being false)
      if (newWaveform && readyToSwap != nullptr && nextWaveform != nullptr && !readyToSwap->load()) {
        nextWaveform->clear();
        nextWaveform->makeCopyOf(generatedWaveform);
        readyToSwap->store(true);
        newWaveform = false;
      }

    }

    socket->close();
  }
  else
  {
    juce::MessageManagerLock mml (Thread::getCurrentThread());
    httpState->setText("Failed to create listener!", juce::sendNotification);
  }
}

juce::AudioBuffer<float> SocketServer::processJsonDropNeg(const char* body, const int wavetableNumChannels, const int wavetableSize, const float bias) {
  juce::AudioBuffer<float> wavetable = juce::AudioBuffer<float>();
  wavetable.setSize(wavetableNumChannels,wavetableSize);
  wavetable.clear();

  if (body != nullptr)
  {
    body += 4; // skip past the "\r\n\r\n"
    juce::String bodyStr(body);
    //std::printf("Body:\n%s\n", bodyStr.toRawUTF8());

    // parse JSON
    juce::var parsed = juce::JSON::parse(bodyStr);
    if (parsed.isObject())
    {
      juce::Array<juce::var> * harmonicsArray = parsed.getProperty ("harmonicSeries",juce::var()).getArray();
      for (auto& harmonic : *harmonicsArray) {
        float amp = harmonic.getProperty("Amp",juce::var());
        int freq = harmonic.getProperty("Freq",juce::var());
        float phase = harmonic.getProperty("Phase",juce::var());

        if (freq > 0) {
          //printf("%a\n",bias);
          amp *= std::pow(1 + (bias * 0.05f),freq-50);
          if (bias == -1) {
            amp = 0;
          }

          float* writePtr = wavetable.getWritePointer(0);

          for (int i = 0; i < wavetableSize; ++i)
          {
            float t = static_cast<float>(i) / static_cast<float>(wavetableSize); // [0, 1)
            float value = amp * std::sin(2.0f * juce::MathConstants<float>::pi * freq * t + phase);
            writePtr[i] += value;
          }
        }
      }

      std::string outstr = "";
      // Normalize data from -1 to 1
      float* data = wavetable.getWritePointer(0);
      float* channel2 = wavetable.getWritePointer(1);
      float maxAmp = 0.0f;
      for (int i = 0; i < wavetable.getNumSamples(); ++i)
        maxAmp = std::max(maxAmp, std::abs(data[i]));
      if (maxAmp > 0.0f || true)
      {
        for (int i = 0; i < wavetable.getNumSamples(); ++i) {
          data[i] /= maxAmp;
          channel2[i] = data[i];
          outstr.append(std::to_string(i) + "," + std::to_string(data[i]) + "\n");
        }
      }
    }
  }

  return wavetable;
}


juce::AudioBuffer<float> SocketServer::processJsonCombineNeg(const char* body, const int wavetableNumChannels, const int wavetableSize, const float bias) {
  juce::AudioBuffer<float> wavetable = juce::AudioBuffer<float>();
  wavetable.setSize(wavetableNumChannels,wavetableSize);
  wavetable.clear();

  if (body != nullptr)
  {
    body += 4; // skip past the "\r\n\r\n"
    juce::String bodyStr(body);
    //std::printf("Body:\n%s\n", bodyStr.toRawUTF8());

    // parse JSON
    juce::var parsed = juce::JSON::parse(bodyStr);
    if (parsed.isObject())
    {
      juce::Array<juce::var> * harmonicsArray = parsed.getProperty ("harmonicSeries",juce::var()).getArray();
      for (auto& harmonic : *harmonicsArray) {
        float amp = harmonic.getProperty("Amp",juce::var());
        int freq = harmonic.getProperty("Freq",juce::var());
        float phase = harmonic.getProperty("Phase",juce::var());

        if (freq < 0) freq = -freq;

        //printf("%a\n",bias);
        amp *= std::pow(1 + (bias * 0.05f),freq-50);
        if (bias == -1) {
          amp = 0;
        }

        float* writePtr = wavetable.getWritePointer(0);

        for (int i = 0; i < wavetableSize; ++i)
        {
          float t = static_cast<float>(i) / static_cast<float>(wavetableSize); // [0, 1)
          float value = amp * std::sin(2.0f * juce::MathConstants<float>::pi * freq * t + phase);
          writePtr[i] += value;
        }

      }

      std::string outstr = "";
      // Normalize data from -1 to 1
      float* data = wavetable.getWritePointer(0);
      float* channel2 = wavetable.getWritePointer(1);
      float maxAmp = 0.0f;
      for (int i = 0; i < wavetable.getNumSamples(); ++i)
        maxAmp = std::max(maxAmp, std::abs(data[i]));
      if (maxAmp > 0.0f || true) {
        for (int i = 0; i < wavetable.getNumSamples(); ++i) {
          data[i] /= maxAmp;
          channel2[i] = data[i];
          outstr.append(std::to_string(i) + "," + std::to_string(data[i]) + "\n");
        }
      }

    }
  }

  return wavetable;
}


juce::AudioBuffer<float> SocketServer::processJsonStereo(const char* body, const int wavetableNumChannels, const int wavetableSize, const float bias) {
  juce::AudioBuffer<float> wavetable = juce::AudioBuffer<float>();
  wavetable.setSize(wavetableNumChannels,wavetableSize);
  wavetable.clear();

  if (body != nullptr)
  {
    body += 4; // skip past the "\r\n\r\n"
    juce::String bodyStr(body);
    //std::printf("Body:\n%s\n", bodyStr.toRawUTF8());

    // parse JSON
    juce::var parsed = juce::JSON::parse(bodyStr);
    if (parsed.isObject())
    {
      juce::Array<juce::var> * harmonicsArray = parsed.getProperty ("harmonicSeries",juce::var()).getArray();
      for (auto& harmonic : *harmonicsArray) {
        float amp = harmonic.getProperty("Amp",juce::var());
        int freq = harmonic.getProperty("Freq",juce::var());
        float phase = harmonic.getProperty("Phase",juce::var());

        int channel;

        if (freq > 0) {
          channel = 0;
        } else {
          channel = 1;
          freq = -freq;
        }

        //printf("%a\n",bias);
        amp *= std::pow(1 + (bias * 0.05f),freq-50);
        if (bias == -1) {
          amp = 0;
        }

        float* writePtr = wavetable.getWritePointer(channel);

        for (int i = 0; i < wavetableSize; ++i)
        {
          float t = static_cast<float>(i) / static_cast<float>(wavetableSize); // [0, 1)
          float value = amp * std::sin(2.0f * juce::MathConstants<float>::pi * freq * t + phase);
          writePtr[i] += value;
        }

      }

      std::string outstr = "";
      // Normalize data from -1 to 1
      for (int channel = 0; channel < wavetable.getNumChannels(); ++channel) {
        float* data = wavetable.getWritePointer(channel);
        float maxAmp = 0.0f;
        for (int i = 0; i < wavetable.getNumSamples(); ++i)
          maxAmp = std::max(maxAmp, std::abs(data[i]));
        if (maxAmp > 0.0f || true)
        {
          for (int i = 0; i < wavetable.getNumSamples(); ++i) {
            data[i] /= maxAmp;
            outstr.append(std::to_string(i) + "," + std::to_string(data[i]) + "\n");
          }
        }
      }
    }
  }

  return wavetable;
}
