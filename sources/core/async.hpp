#pragma once

#include "pch/asio.hpp"

namespace Async{
extern boost::asio::io_context &Context();

extern void Run();
}