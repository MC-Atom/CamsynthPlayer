/*
  ==============================================================================

    VideoProcessing.cpp
    Created: 29 Jun 2025 12:35:13pm
    Author:  Devyn Oh

  ==============================================================================
*/

#include "VideoProcessor.h"

#include <complex>
#include <memory>
#include <JuceHeader.h>
#include <opencv2/opencv.hpp>
#include <opencv2/xphoto.hpp>

#include "dft.h"

void VideoProcessor::run() {
    // Used code from https://gist.github.com/priteshgohil/edce691cf557e7e3bb708ff100a18da3 for camera capture

    cv::VideoCapture camera(0);

    if (!camera.isOpened()) {
        DBG ("ERROR: Could not open camera");
        return;
    }
    // camera.set(cv::CAP_PROP_AUTO_WB, 3);

    // create a window to display the images from the webcam
    cv::namedWindow("Webcam", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("HSV", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Greyscale", cv::WINDOW_AUTOSIZE);



    // find the target width and height
    camera >> inputFrame;
    //inputFrame = cv::imread("../assets/star.webp", cv::IMREAD_COLOR);
    accumulatedFrame = inputFrame.clone();

    try {
        while (!threadShouldExit()) {
            process(nextWaveform, readyToSwap);
            wait(100);
        }
    }
    catch (...)
    {
        auto eptr = std::current_exception();
        try {
            if (eptr) std::rethrow_exception(eptr);
        } catch (const std::exception& e) {
            DBG("Caught std::exception in run(): " << e.what());
        } catch (const char* msg) {
            DBG("Caught const char* exception: " << msg);
        } catch (int code) {
            DBG("Caught int exception: " << code);
        } catch (...) {
            DBG("Still unknown exception type in run()");
        }
    }

};

void VideoProcessor::process(std::shared_ptr<juce::AudioBuffer<float>> nextWaveform, std::atomic<bool> readyToSwap) {

    const int desiredVertices = 10; // Change this to control the number of edges
    double time = 0.0;

    // array to hold the hueGray frames
    cv::Mat hsv, hue, hueGray;
    // array to hold the grayscale frames
    cv::Mat grayscale;
    // array to hold the last frame
    cv::Mat frame;

    // Edge Detection
    cv::Mat blur, edges, cleaned;

    // display the frame until you press a key
    while (true) {
        const double ALPHA = 0.4;
        constexpr double STEP = 0.1;
        constexpr int DEPTH = 100;
        // capture the next frame from the webcam
        camera >> inputFrame;
        //WhiteBalance
        cv::Ptr<cv::xphoto::SimpleWB> wb = cv::xphoto::createSimpleWB();
        wb ->balanceWhite(inputFrame, inputFrame);

        //inputFrame = cv::imread("../assets/star.webp", cv::IMREAD_COLOR);
        cv::addWeighted(accumulatedFrame, 1.0 - ALPHA, inputFrame, ALPHA, 0, accumulatedFrame);

        // Resize the frame
        cv::resize(accumulatedFrame, resized, cv::Size(targetWidth, newHeight));


        // 1. Convert BGR to HSV
        cv::cvtColor(resized, hsv, cv::COLOR_BGR2HSV);
        // 2. Split into H, S, V
        std::vector<cv::Mat> hsvChannels;
        cv::split(hsv, hsvChannels);
        // 3. Extract Hue channel
        hue = hsvChannels[1];  // Hue is in [0,179]
        // 4. Optional: scale hue to 0–255 for display
        hue.convertTo(hueGray, CV_8UC1, 255.0 / 179.0);

        // 1. Convert to Greyscale
        cv::cvtColor(resized, grayscale, cv::COLOR_BGR2GRAY);


        // show the image on the window
        cv::imshow("Webcam", accumulatedFrame);
        //cv::imshow("HSV", hueGray);
        //cv::imshow("Greyscale", grayscale);

        frame = grayscale;
        //cv::imshow("frame", frame);

        // 2. Up the contrast & brightness
        // const double contrast = 2;
        // const double brightness = 0.0;
        // cv::Mat contrastFrame = cv::Mat::zeros( frame.size(), frame.type() );
        // for( int y = 0; y < frame.rows; y++ ) {
        //     for( int x = 0; x < frame.cols; x++ ) {
        //         for( int c = 0; c < frame.channels(); c++ ) {
        //             contrastFrame.at<cv::Vec3b>(y,x)[c] =
        //               cv::saturate_cast<uchar>( contrast*frame.at<cv::Vec3b>(y,x)[c] + brightness );
        //         }
        //     }
        // }

        // Apply Gaussian blur to smooth the input image
        cv::blur(frame, blur, cv::Size(2, 2));
        cv::imshow("Blur", blur);
        // Apply Canny edge detection
        cv::Canny(blur, edges, 100, 180);
        // Morphological closing to close small holes in the edges
        cv::morphologyEx(edges, cleaned, cv::MORPH_CLOSE, cv::Mat::ones(4, 4, CV_8U));

        // display the result
        cv::imshow("Edges", cleaned);

        // Find contours
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(cleaned.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        if (contours.empty()) {
            std::cerr << "No contours found\n";
            if (cv::waitKey(1) == 27)
                break;
            continue;
        }

        // Find the largest contour
        size_t largestContourIdx = 0;
        double maxArea = 0.0;
        for (size_t i = 0; i < contours.size(); ++i) {
            double area = cv::contourArea(contours[i]);
            if (area > maxArea) {
                maxArea = area;
                largestContourIdx = i;
            }
        }
        std::vector<cv::Point> largestContour = contours[largestContourIdx];

        cv::Mat largestCont = cv::Mat::zeros(frame.size(), CV_8UC3);
        for (cv::Point& pt : largestContour) {

            cv::circle(largestCont, pt, 1, cv::Scalar(0, 0, 255), -1);
        }


        cv::imshow("largestCont", largestCont);


        // convert from cv Point to complex numbers
        std::vector<std::complex<double>> complexPoints;
        complexPoints.reserve(largestContour.size());
        for (const cv::Point& pt : largestContour) {
            complexPoints.emplace_back(static_cast<double>(pt.x), static_cast<double>(pt.y));
        }

        std::vector<FourierComponent> fourierSeries = fourier(complexPoints,DEPTH);

        jassert(readyToSwap);
        if (readyToSwap.load(std::memory_order_acquire) == false) {
            auto newBuffer = std::make_shared<juce::AudioBuffer<float>>(1, 2048);
            auto* data = newBuffer->getWritePointer(0);

            for (int i = 0; i < newBuffer->getNumSamples(); ++i) {
                float t = static_cast<float>(i) / newBuffer->getNumSamples();
                float sample = 0.0f;

                for (auto [freq, amp, phase] : fourierSeries) {
                    sample += amp * std::sin((juce::MathConstants<float>::twoPi * (freq + 1) * t)+phase);
                }

                data[i] = sample;
            }


            nextWaveform = newBuffer;
            readyToSwap.store(true,std::memory_order_release);
        }

        cv::Mat fourierImage = cv::Mat::zeros(frame.size(), CV_8UC3);
        int trailSize = 5000;
        for (int trail = -trailSize; trail <= 0; ++trail) {
            std::complex<double> sum(0, 0);
            for (auto [freq, amp, phase] : fourierSeries) {
                double angle = std::fmod(phase + freq * (time + STEP * trail / 50), M_PI * 2);
                std::complex<double> vec(std::cos(angle) * amp, std::sin(angle) * amp);
                sum += vec;
            }
            cv::Point pt(sum.real(), sum.imag());
            cv::circle(fourierImage, pt, 3, cv::Scalar(0, 0, static_cast<int>(255.0 * (trailSize+trail) / trailSize)), -1);
            //std::cout << sum.real() << " " << sum.imag() << std::endl;
        }

        cv::Point lastPoint(0,0);
        for (auto [freq, amp, phase] : fourierSeries) {
            double angle = std::fmod(phase + freq * time, M_PI * 2);
            cv::Point nextPoint(lastPoint.x + std::cos(angle) * amp, lastPoint.y + std::sin(angle) * amp);
            cv::line(fourierImage, lastPoint, nextPoint, cv::Scalar(0, 0, 255), 2);
            lastPoint = nextPoint;
        }

        std::cout << time << std::endl;
        time += STEP;

        cv::imshow("fourierImage", fourierImage);



        /*
        // Approximate contour to a polygon with roughly n edges
        double epsilon = 1.0;
        std::vector<cv::Point> approx;
        int attempts = 0;
        while (attempts++ < 100) {
            cv::approxPolyDP(largestContour, approx, epsilon, true);
            if ((int)approx.size() == desiredVertices)
                break;
            if ((int)approx.size() > desiredVertices)
                epsilon *= 1.2; // increase simplification
            else
                epsilon *= 0.8; // decrease simplification
        }
        // Draw the contour result for visualization
        cv::Mat result(frame.size(), CV_8UC3, cv::Scalar(0));
        std::vector<std::vector<cv::Point>> toDraw{approx};
        cv::drawContours(result, toDraw, -1, cv::Scalar(0, 255, 0), 2);
        cv::imshow("Polygon Approximation", result);
        */


        /*
        // Draw smoothed curve using interpolation between points
        cv::Mat curveImg(frame.size(), CV_8UC3, cv::Scalar(0));
        for (size_t i = 0; i < largestContour.size(); ++i) {
            cv::Point p0 = largestContour[i % largestContour.size()];
            cv::Point p1 = largestContour[(i + 1) % largestContour.size()];

            // interpolate between p0 and p1
            for (float t = 0; t <= 1.0; t += 0.01f) {
                float x = (1 - t) * p0.x + t * p1.x;
                float y = (1 - t) * p0.y + t * p1.y;
                cv::circle(curveImg, cv::Point(cvRound(x), cvRound(y)), 1, cv::Scalar(0, 255, 0), -1);
            }
        }
        cv::imshow("Curved Approximation", curveImg);
        */
        // wait (10ms) for esc key to be pressed to stop
        if (cv::waitKey(1) == 27)
            break;
    }
}

