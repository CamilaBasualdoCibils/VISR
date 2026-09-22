#include "AruiDesktop.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    ARUI::Server::Manager::AruiDesktop server(argc, argv);
    return server.Run();
}
