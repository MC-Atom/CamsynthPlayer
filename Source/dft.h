//
// Created by Devyn Oh on 6/19/25.
//

#ifndef DFT_H
#define DFT_H

#include <vector>
#include <complex>
#include <functional>

struct FourierComponent {
  int freq;
  double amp;
  double phase;
};

std::vector<FourierComponent> fourier(const std::vector<std::complex<double>>& path, int depth);
std::complex<double> integrateC(std::function<std::complex<double>(double)> func, double a, double b, double err);

void printFourierComponents(const std::vector<FourierComponent>& components);

#endif //DFT_H
