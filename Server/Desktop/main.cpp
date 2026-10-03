#include "ARUI/Core/Debug/AllowDebugger.hpp"
#include "AruiDesktop.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    ARUI::Debug::AllowConfiguredDebuggerAttach();
    ARUI::Server::Manager::AruiDesktop server(argc, argv);
    return server.Run();
}
