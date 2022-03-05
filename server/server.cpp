#ifdef _WIN32 
#define _WIN32_WINNT 0x0601
#endif

#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <utility>
#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
using namespace boost;
using namespace std::chrono_literals;
constexpr uint16_t port = 54'000;

std::vector<uint8_t> input_buffer(1024 * 20);
std::vector<uint8_t> output_buffer(1024 * 20);
std::vector<std::shared_ptr<asio::ip::tcp::socket>> clients;

enum class MessageType : uint8_t {
	Ping,
	Hello,
	Text,
	Exit
};

void ReadData(asio::ip::tcp::socket&);
void WriteData(asio::ip::tcp::socket&, std::string&&);

void WaitConnection(asio::ip::tcp::acceptor& acceptor) {
	acceptor.async_accept(
		[&](system::error_code err, asio::ip::tcp::socket sock)
		{
			if (err) {
				std::cout << err.what() << std::endl;
			}
			else {
				std::cout << "Connection established! " << sock.remote_endpoint() << std::endl;
				clients.push_back(std::make_shared<asio::ip::tcp::socket>(std::move(sock)));
				WaitConnection(acceptor);
				ReadData(*clients.back());
			}
		}
	);
}

void ReadData(asio::ip::tcp::socket& sock) {
	sock.async_read_some(asio::buffer(input_buffer.data(), input_buffer.size()),
		[&](system::error_code err, size_t length)
		{
			if (err) {
				std::cout << err.what() << std::endl;
			}
			else {
				switch (static_cast<MessageType>(input_buffer[0]))
				{
				case MessageType::Ping:
				{
					uint64_t time = std::chrono::steady_clock::now().time_since_epoch().count();

					std::string str(input_buffer.begin() + 1, //ignoring the messageType byte
						std::find(input_buffer.begin(), input_buffer.end(), '\n'));

					std::istringstream ss(str);
					uint64_t millis;

					ss >> millis;
					std::cout << "Ping: " << std::chrono::duration<double>(time - millis).count() << std::endl;
					std::string s = "Ping took " + std::to_string(time - millis) + "microseconds\n";
					WriteData(sock, std::move(s));
				}
					break;

				case MessageType::Text:
					std::cout << "Message from " << sock.remote_endpoint() << std::endl;
					for (int i = 0; i < length; ++i) {
						std::cout.put(input_buffer[i]);
					}
					std::cout << std::endl;
					WriteData(sock, "Message recived\n");

					break; 
				case MessageType::Hello:
					std::cout << "Hello from " << sock.remote_endpoint() << std::endl;
					WriteData(sock, "Hello\n");
					break;

				case MessageType::Exit:
					std::cout << sock.remote_endpoint() << " ended the session" << std::endl;
					sock.close();
					break; 

				default:
					std::cout << "Error deducing message type" << std::endl;
					break;
				}
			}
		}
	);
}

void WriteData(asio::ip::tcp::socket& sock, std::string&& str) {
	sock.async_write_some(asio::buffer(str.data(), str.size()),
		[&](system::error_code err, size_t length)
		{
			if (err) {
				std::cout << err.what() << std::endl;
			}
			else {
				ReadData(sock);
			}
		}
	);
}


int main()
{
	asio::io_context context;
	asio::ip::tcp::acceptor acceptor(context, 
		asio::ip::tcp::endpoint(asio::ip::address_v4(), port));

	WaitConnection(acceptor);

	context.run();

	return 0;
}