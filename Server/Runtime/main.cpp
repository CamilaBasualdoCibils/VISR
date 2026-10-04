#include "VISR/Core/Debug/AllowDebugger.hpp"
#include "VisrServer.hpp"

#include <exception>
#include <iostream>

int main(int argc, char **argv) {
  VISR::Debug::AllowConfiguredDebuggerAttach();
  try {
    VISR::Server::VisrServer server(argc, argv);
    return server.Run();
  } catch (const std::exception &error) {
    std::cerr << "visr-server: " << error.what() << '\n';
    return 1;
  }
}
