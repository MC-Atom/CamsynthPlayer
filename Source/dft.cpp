//
// Created by Devyn Oh on 6/19/25.
//

#include "dft.h"
#include <iostream>
#include <cmath>

// Type alias for clarity
using Cmplx = std::complex<double>;

std::vector<FourierComponent> fourier(const std::vector<Cmplx>& path, const int depth) {


  const int N = path.size();
  constexpr double TAU = 2.0 * M_PI;
  std::vector<FourierComponent> dft;
  const double err = 0.01;

  const int mid = depth >> 1;
  const int minf = -mid; // The lowest frequency calculated
  const int maxf = depth - mid; // The highest frequency calculated

  for (int f = minf; f <= maxf; ++f) {
    // Define the function to integrate
    auto func = [&](double t) -> Cmplx {
      int i = std::round(t * (N - 1));
      if (i < 0) i = 0;
      if (i >= N) i = N - 1;
      Cmplx expTerm = std::polar(1.0, -f * TAU * t);  // same as Complex({abs:1, arg:...})
      return path[i] * expTerm;
    };

    Cmplx Cn = integrateC(func, 0.0, 1.0, err);

    dft.push_back({
        f,
        std::abs(Cn),
        std::arg(Cn)
    });

    double progress = 100.0 * (f - minf) / (maxf - minf);
    //std::cout << "progress " << progress << "%" << std::endl;
  }

  return dft;
}



Cmplx integrateC(std::function<Cmplx(double)> func, double a, double b, double err) {
  // simple trapezoidal implementation:
  const int steps = 1000; // crude fixed-step version
  double h = (b - a) / steps;
  Cmplx sum = 0.5 * (func(a) + func(b));

  for (int i = 1; i < steps; ++i) {
    double x = a + i * h;
    sum += func(x);
  }

  return sum * h;
}



double f(double x) {
  return 1/x;
}

void printFourierComponents(const std::vector<FourierComponent>& components) {
  for (const auto& comp : components) {
    std::cout << "Freq: " << comp.freq
              << ", Amp: " << comp.amp
              << ", Phase: " << comp.phase << '\n';
  }
}