#ifdef _WIN32 
#define _WIN32_WINNT 0x0601
#endif

#include <iostream>
#include <vector>
#include <thread>
#include <Boost/asio.hpp>
#include <Boost/bind.hpp>
using namespace boost;
using namespace std::chrono_literals;
std::vector<uint8_t> mBuff(1024 * 20);

void TakeData(asio::ip::tcp::socket& sock) {
	sock.async_read_some(asio::buffer(mBuff.data(), mBuff.size()),
		[&](system::error_code err, size_t length) 
		{
			if (err) {
				std::cout << err.what() << std::endl;
			}
			else {
				std::cout << "Read " << length << " bytes\n";
				for (int i = 0; i < length; ++i) {
					std::cout.put(mBuff[i]);
				}
				std::cout << std::endl;
			}
		}
	);
}


int main()
{
	asio::io_context context;
	asio::ip::tcp::socket sock(context);
	system::error_code err;
	asio::ip::tcp::resolver resolver(context);

	std::string url;      // = "www.yandex.ru"; // = "example.com"
	do {

		std::cout << "Enter the domain you want to connect (q for quit)" << std::endl;
		std::cin >> url;
		if (url[0] == 'q')
			break;

		asio::ip::tcp::resolver::query quer(url, "80"); //port 80 (http)

		auto iter = resolver.resolve(quer, err);

		if (err) {
			std::cout << err.what() << std::endl;
			continue;
		}
		//we are trying to connect only to the first adress in list given by resolver
		sock.connect(*iter, err);

		if (err) {
			std::cout << err.what() << std::endl;
			continue;
		}

		std::string request =
			"GET /index.html HTTP/1.1\r\n"
			"Host: " + url + "\r\n"
			"Connection: close\r\n\r\n";

		context.post(boost::bind(TakeData, boost::ref(sock))); //adding async task

		sock.write_some(boost::asio::buffer(request.data(), request.size()));

		context.run(); //blocking until all async task are finished
		context.restart(); //preparing for next 'run' call

		std::cout << "\nRequest done!" << std::endl;
		sock.close();

	} while (true);


	return 0;
}