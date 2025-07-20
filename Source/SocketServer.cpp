/*
  ==============================================================================

    SocketServer.cpp
    Created: 18 Jul 2025 9:16:59pm
    Author:  Devyn Oh

  ==============================================================================
*/

// lsof -i :8080

#include "SocketServer.h"

#define portNumber 8080
#define localHostName "0.0.0.0"

void SocketServer::run()
{
  const auto socket = std::make_unique<juce::StreamingSocket>();
  auto generatedWaveform = juce::AudioBuffer<float>();
  bool listening = socket->createListener(portNumber, localHostName);

  if (listening)
  {
    bool newWaveform = false;
    std::printf("Listening on %s:%d\n",localHostName,portNumber);

    while (!threadShouldExit())
    {
      std::unique_ptr<juce::StreamingSocket> connection(socket->waitForNextConnection());

      if (connection != nullptr) {
        char buffer[MAX_URL_SIZE] = {0};

        int bytesRead = connection->read(buffer, MAX_URL_SIZE - 1, true); // true = block
        if (bytesRead > 0) {
          buffer[bytesRead] = '\0';
          std::printf("📥 Received:\n%s\n", buffer);

          // === EXTRACT BODY ===
          // crude way to extract JSON body (assumes POST)
          char* body = strstr(buffer, "\r\n\r\n");

          // Process the json into the wavetable we want
          generatedWaveform = processJson(body,nextWaveform->getNumSamples());
          newWaveform = true;

          // Send minimal HTTP 200 OK back to client
          const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
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
    std::printf("Failed to create listener!\n");
  }
}

juce::AudioBuffer<float> SocketServer::processJson(char* body, int wavetableSize) {
  juce::AudioBuffer<float> wavetable = juce::AudioBuffer<float>();
  wavetable.setSize(1,wavetableSize);

  if (body != nullptr)
  {
    body += 4; // skip past the "\r\n\r\n"
    juce::String bodyStr(body);
    std::printf("Body:\n%s\n", bodyStr.toRawUTF8());

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
          float* writePtr = wavetable.getWritePointer(0);
          for (int i = 0; i < wavetableSize; ++i)
          {
            float t = static_cast<float>(i) / static_cast<float>(wavetableSize); // [0, 1)
            float value = amp * std::sin(2.0f * juce::MathConstants<float>::pi * freq * t + phase);
            writePtr[i] += value;
          }
        }
      }
      // Normalize data from -1 to 1
      float* data = wavetable.getWritePointer(0);
      float maxAmp = 0.0f;
      for (int i = 0; i < wavetable.getNumSamples(); ++i)
        maxAmp = std::max(maxAmp, std::abs(data[i]));

      if (maxAmp > 0.0f)
      {
        for (int i = 0; i < wavetable.getNumSamples(); ++i)
          data[i] /= maxAmp;
      }
    }
  }

  return wavetable;
}
