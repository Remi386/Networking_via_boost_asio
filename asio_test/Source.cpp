#ifdef _WIN32 
#define _WIN32_WINNT 0x0601
#endif

#include <iostream>
#include <vector>
#include <thread>
#include <Boost/asio.hpp>
using namespace boost;
using namespace std::chrono_literals;

int main()
{
	asio::io_context context;

	asio::ip::tcp::socket sock(context);

	system::error_code err;

	asio::ip::tcp::resolver resolver(context);
	std::string url = "www.yandex.ru";
	asio::ip::tcp::resolver::query quer(url, "80");

	auto iter = resolver.resolve(quer, err);

	sock.connect(*iter);

	std::string request =
		"GET /index.html HTTP/1.1\r\n"
		"Host: " + url + "\r\n"
		"Connection: close\r\n\r\n";

	sock.write_some(boost::asio::buffer(request.data(), request.size()));
	
	std::this_thread::sleep_for(200ms); //waiting for response

	size_t bytes = sock.available();
	
	if (bytes > 0) {
		std::vector<uint8_t> buff(bytes);
		sock.read_some(boost::asio::buffer(buff.data(), buff.size()));

		for (int i = 0; i < buff.size(); ++i) {
			std::cout.put(buff[i]);
		}
		std::cout << std::endl;

	}


	std::cout << "Request done!" << std::endl;

	return 0;
}