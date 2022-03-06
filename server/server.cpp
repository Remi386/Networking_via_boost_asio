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
#include <boost/interprocess/streams/bufferstream.hpp>
#include <boost/noncopyable.hpp>
using namespace boost;
using namespace boost::interprocess;
using namespace std::chrono_literals;
constexpr uint16_t port = 54'000;
constexpr uint16_t buffer_size = 2048;
constexpr uint8_t threads_count = 4;
class Connection;

std::vector<std::shared_ptr<Connection>> clients;

enum class MessageType : uint8_t {
	Ping,
	Hello,
	Text,
	Exit
};

class Connection : public boost::noncopyable
{
public:
	using conn_ptr = std::shared_ptr<Connection>;

	explicit Connection(asio::ip::tcp::socket&& _sock)
		: sock(std::move(_sock))
	{
		write_buffer.reserve(buffer_size);
		read_buffer.reserve(buffer_size);
	}

	static conn_ptr CreateConnection(asio::ip::tcp::socket&& _sock) {
		conn_ptr connection = std::make_shared<Connection>(std::move(_sock));
		return connection;
	}

	void WriteData() {
		sock.async_write_some(asio::buffer(write_buffer.data(), write_buffer.size()),
			[&](system::error_code err, size_t length)
			{
				if (err) {
					std::cout << err.what() << std::endl;
				}
				else {
					ReadData();
				}
			}
		);
	}

	void ReadData() {
		sock.async_read_some(asio::buffer(read_buffer.data(), read_buffer.size()),
			[&](system::error_code err, size_t length)
			{
				if (err) {
					std::cout << err.what() << std::endl;
				}
				else {
					if (read_buffer.size() == 0) {
						std::cout << "Nothing to read" << std::endl;
						ReadData();
						return;
					}
					bufferstream istr(&read_buffer[0], read_buffer.size());
					bufferstream ostr(&write_buffer[0], write_buffer.size());
					
					switch (static_cast<MessageType>(istr.peek() - '0')) //converting char to int
					{
					case MessageType::Ping:
					{
						uint64_t time = std::chrono::steady_clock::now().time_since_epoch().count();

						uint64_t millis;
						istr >> millis;
						auto diff = std::to_string(double(time - millis) / std::chrono::steady_clock::period::den);
						std::cout << "Ping: " + diff << " seconds" << std::endl;
						ostr << "Ping took " << diff << " seconds\n";
						WriteData();
					}
					break;

					case MessageType::Text:
					{

						std::cout << "Message from " << sock.remote_endpoint() << std::endl;

						std::string line;
						std::getline(istr, line);
						std::cout.write(line.data(), line.size());

						ostr << "Message recived\n";
						WriteData();

					}
						break;
					case MessageType::Hello:
					{

						std::cout << "Hello from " << sock.remote_endpoint() << std::endl;
						ostr << "Hello from server\n";
						WriteData();
					}
						break;
					case MessageType::Exit:
						std::cout << sock.remote_endpoint() << " ended the session" << std::endl;
						sock.close();
						clients.erase(std::remove_if(clients.begin(), clients.end(), 
							[this](std::shared_ptr<Connection> ptr) 
							{
								if (ptr.get() == this)
								{
									std::cout << "Client removed succesfully\n";
									return true;
								}
								return false;
							})
						);
						break;

					default:
					{
						std::cout << "Error deducing message type\nMessage:" << std::endl;
						std::string line;
						while (!istr.eof()) {
							std::getline(istr, line);
							std::cout.write(line.data(), line.size());
						}
						break;
					}
					}
				}
			}
		);
	}

private:
	std::vector<char> write_buffer;
	std::vector<char> read_buffer;
	asio::ip::tcp::socket sock;
};

void WaitConnection(asio::ip::tcp::acceptor& acceptor) {
	acceptor.async_accept(
		[&](system::error_code err, asio::ip::tcp::socket sock)
		{
			if (err) {
				std::cout << err.what() << std::endl;
			}
			else {
				std::cout << "Connection established! " << sock.remote_endpoint() << std::endl;
				auto conn_ptr = Connection::CreateConnection(std::move(sock));
				clients.push_back(conn_ptr);
				WaitConnection(acceptor);
				conn_ptr->ReadData();
			}
		}
	);
}

int main()
{
	asio::io_context context;
	asio::ip::tcp::acceptor acceptor(context, 
		asio::ip::tcp::endpoint(asio::ip::address_v4(), port));

	boost::thread_group threads;

	WaitConnection(acceptor);

	/*for (int i = 0; i < threads_count; ++i) {
		threads.create_thread([&]() { context.run(); });
	}*/
	context.run(); //main thread also should handle events
	threads.join_all();

	return 0;
}