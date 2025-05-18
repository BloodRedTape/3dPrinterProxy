#include "async.hpp"

boost::asio::io_context& Async::Context(){
	static boost::asio::io_context s_Context;

	return s_Context;
}
void Async::Run() {
	Context().run();
}
