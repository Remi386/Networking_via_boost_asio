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

void WriteData(asio::ip::tcp::socket& sock, size_t mess_size) {
	sock.async_write_some(asio::buffer(output_buffer.data(), mess_size),
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

void ReadData(asio::ip::tcp::socket& sock) {
	sock.async_read_some(asio::buffer(input_buffer.data(), input_buffer.size()),
		[&](system::error_code err, size_t length)
		{
			if (err) {
				std::cout << err.what() << std::endl;
			}
			else {
				for (int i = 0; i < length; ++i) {
					std::cout.put(input_buffer[i]);
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

	sock.connect(asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), port));
	std::string str;
	do {
		std::cout << "Enter the option (1 - ping, 2 - hello, 3 - text, 4 - exit)" << std::endl;
		std::cin >> str;
		std::string message;
		
		switch (std::stoi(str))
		{
		case 1:
		{
			uint64_t time = std::chrono::steady_clock::now().time_since_epoch().count();
			message = std::to_string(static_cast<uint8_t>(MessageType::Ping)) +
				std::to_string(time) + "\n";
		}
			break;
		case 2:
			message = std::to_string(static_cast<uint8_t>(MessageType::Hello)) + "Hello";
			break;
		case 3:
		{
			std::string mess;
			std::cout << "enter a message: " << std::endl;
			std::cin >> mess;
			message = std::to_string(static_cast<uint8_t>(MessageType::Text)) + mess + "\n";
			
		}
			break;
		case 4:
		{

			std::cout << "Disconnecting..." << std::endl;
			std::string str = std::to_string(static_cast<uint8_t>(MessageType::Exit)) + "Bye\n";
			sock.write_some(asio::buffer(str.data(), str.size()));
			sock.close();
			exit(0);
		}
		default:
			break;
		}
		std::cout << message << std::endl;
		std::copy(message.begin(), message.end(), output_buffer.begin());
		WriteData(sock, message.size());
		context.run();
		context.restart();
	} while (true);



	return 0;
}