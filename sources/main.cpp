#include <filesystem>
#include <bsl/log.hpp>
#include "printer_proxy.hpp"


int main(int argc, char* argv[])
{
    PrinterProxy proxy;
    proxy.Listen(2228);

    proxy.RunAsync();

    Async::Run();

    return 0;
}