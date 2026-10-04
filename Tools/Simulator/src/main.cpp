#include "VISR/Tools/Simulator/SimulatorApplication.hpp"

#include <exception>
#include <iostream>

int main() {
  try {
    VISR::Tools::Simulator::SimulatorApplication simulator;
    return simulator.Run();
  } catch (const std::exception &error) {
    std::cerr << "Simulator failed: " << error.what() << '\n';
    return 1;
  }
}
