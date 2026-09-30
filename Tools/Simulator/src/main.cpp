#include "ARUI/Tools/Simulator/SimulatorApplication.hpp"

#include <exception>
#include <iostream>

int main() {
  try {
    ARUI::Tools::Simulator::SimulatorApplication simulator;
    return simulator.Run();
  } catch (const std::exception &error) {
    std::cerr << "Simulator failed: " << error.what() << '\n';
    return 1;
  }
}
