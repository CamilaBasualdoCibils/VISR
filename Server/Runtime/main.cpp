#include "ARUI/Core/Debug/AllowDebugger.hpp"
#include "AruiServer.hpp"

#include <exception>
#include <iostream>

int main(int argc, char **argv) {
  ARUI::Debug::AllowConfiguredDebuggerAttach();
  try {
    ARUI::Server::AruiServer server(argc, argv);
    return server.Run();
  } catch (const std::exception &error) {
    std::cerr << "arui-server: " << error.what() << '\n';
    return 1;
  }
}
