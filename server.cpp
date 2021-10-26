
#include "Connection.hpp"

#include "hex_dump.hpp"

#include <chrono>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <unordered_map>
#include <glm/glm.hpp>

#ifdef _WIN32
extern "C" { uint32_t GetACP(); }
#endif
int main(int argc, char **argv) {
#ifdef _WIN32
	{ //when compiled on windows, check that code page is forced to utf-8 (makes file loading/saving work right):
		//see: https://docs.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
		uint32_t code_page = GetACP();
		if (code_page == 65001) {
			std::cout << "Code page is properly set to UTF-8." << std::endl;
		} else {
			std::cout << "WARNING: code page is set to " << code_page << " instead of 65001 (UTF-8). Some file handling functions may fail." << std::endl;
		}
	}

	//when compiled on windows, unhandled exceptions don't have their message printed, which can make debugging simple issues difficult.
	try {
#endif

	//------------ argument parsing ------------

	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}

	//------------ initialization ------------

	Server server(argv[1]);
	std::string status_message = "";


	//------------ main loop ------------
	constexpr float ServerTick = 1.0f / 30.0f; 

	//server state:

	//per-client state:
	struct PlayerInfo {
		PlayerInfo() {
			static uint32_t next_player_id = 1;
			name = "Player" + std::to_string(next_player_id);
			next_player_id += 1;
		}
		std::string name;

		uint32_t num_pies_collected = 0;
		uint32_t flag = 0;

		int32_t total = 0;

		glm::vec3 position;

	};
	std::unordered_map< Connection *, PlayerInfo > players;

	PlayerInfo *winner = NULL;
	while (true) {
		static auto next_tick = std::chrono::steady_clock::now() + std::chrono::duration< double >(ServerTick);
		//process incoming data from clients until a tick has elapsed:
		while (true) {
			auto now = std::chrono::steady_clock::now();
			double remain = std::chrono::duration< double >(next_tick - now).count();
			if (remain < 0.0) {
				next_tick += std::chrono::duration< double >(ServerTick);
				break;
			}
			server.poll([&](Connection *c, Connection::Event evt){
				if (evt == Connection::OnOpen) {
					//client connected:

					//create some player info for them:
					players.emplace(c, PlayerInfo());


				} else if (evt == Connection::OnClose) {
					//client disconnected:

					//remove them from the players list:
					auto f = players.find(c);
					assert(f != players.end());
					players.erase(f);


				} else { assert(evt == Connection::OnRecv);
					//got data from client:
					std::cout << "got bytes:\n" << hex_dump(c->recv_buffer); std::cout.flush();

					//look up in players list:
					auto f = players.find(c);
					assert(f != players.end());
					PlayerInfo &player = f->second;

					//handle messages from client:
					size_t message_size = 3 + sizeof(glm::vec3); // size in bytes
					while (c->recv_buffer.size() >= message_size) {
						//expecting five-byte messages 'b' (left count) (right count) (down count) (up count)
						char type = c->recv_buffer[0];
						if (type != 'b') {
							std::cout << " message of non-'b' type received from client!" << std::endl;
							//shut down client connection:
							c->close();
							return;
						}
						uint8_t num_pies_collected = c->recv_buffer[1];
						uint8_t flag = c->recv_buffer[2];
						if (flag == 1) {
							winner = &player;
						}

						glm::vec3* player_pos = reinterpret_cast<glm::vec3 *>(&(c->recv_buffer[5]));

						player.num_pies_collected = num_pies_collected;
						player.position = *player_pos;

						c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + message_size);
					}
				}
			}, remain);
		}

		//update current game state
		status_message = "";
		if (winner) {
			status_message = winner->name + " wins! ";
		} else {
			for (auto &[c, player] : players) {
				(void)c; //work around "unused variable" warning on whatever version of g++ github actions is running
				if (status_message != "") status_message += " + ";
				status_message += "( " + player.name + ": " + std::to_string(player.num_pies_collected) + " ) ";
			}
		}
		//std::cout << status_message << std::endl; //DEBUG

		//send updated game state to all clients
		// Each player in the game receives a message from the server
		// 
		// [m] - 1 byte
		// [status message size] - 3 bytes
		// [status message] - number of bytes given by previous field
		// [number of other players] - 1 byte
		// [other PlayerData] - number of bytes can depend on each player

		for (auto &[c, player] : players) {
			(void)player; //work around "unused variable" warning on whatever g++ github actions uses
			//send an update starting with 'm', a 24-bit size, and a blob of text:
			c->send('m');
			c->send(uint8_t(status_message.size() >> 16));
			c->send(uint8_t((status_message.size() >> 8) % 256));
			c->send(uint8_t(status_message.size() % 256));
			c->send_buffer.insert(c->send_buffer.end(), status_message.begin(), status_message.end());

			struct PlayerMessageData { // the data that gets sent to all of the players
				std::string name;
				glm::vec3 position;

				std::vector<char> getPackedData()
				{
					std::vector<char> result;
					// add the position data
					char* _player_pos_data = reinterpret_cast<char*>(&position);
					size_t _pos_len = sizeof(glm::vec3) / sizeof(char); // size in bytes of glm::vec3
					for (size_t i = 0; i < _pos_len; i++)
					{
						result.emplace_back(_player_pos_data[i]);
					}
					// add the name data
					uint8_t _name_len = name.size() + 1; // truncate the size of the name to 255 bytes. +1 is for the null terminator
					result.emplace_back(reinterpret_cast<unsigned char>(_name_len));
					result.insert(result.end(), name.begin(), name.end());
					
					return result;
				}

				size_t getPackedDataSize()
				{
					return sizeof(glm::vec3) + name.size() + 1;  // size of position + name.size() + 1
				}
			};
			// other players data begins with 1 byte, that tells the player how many 
			// PlayerData is being sent 
			std::vector<char> other_players_data;
			uint8_t n = players.size() - 1;
			(void)n;
			for (auto &[oc, oplayer] : players) { // other connection, other player
				if (oc == c) { // skip if we are visitng ourselves
					continue;
				}
				PlayerMessageData pmd {oplayer.name, oplayer.position};
				std::vector<char> player_data = pmd.getPackedData();
				other_players_data.insert(other_players_data.end(), player_data.begin(), player_data.end());
			}
			std::cout << uint8_t(other_players_data.size() >> 16) << std::endl;

			static_assert(sizeof(uint8_t(other_players_data.size() >> 16)) == 1, "Byte size in message is wrong!"); // static assert is checked at compile time
			c->send(uint8_t(other_players_data.size() >> 16));
			static_assert(sizeof(uint8_t((other_players_data.size() >> 8) % 256)) == 1, "Byte size in message is wrong!"); // static assert is checked at compile time
			c->send(uint8_t((other_players_data.size() >> 8) % 256));
			c->send(uint8_t(other_players_data.size() % 256));
			c->send(uint8_t(n));
			c->send_buffer.insert(c->send_buffer.end(), other_players_data.begin(), other_players_data.end());
		}

	}

	return 0;

#ifdef _WIN32
	} catch (std::exception const &e) {
		std::cerr << "Unhandled exception:\n" << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Unhandled exception (unknown type)." << std::endl;
		throw;
	}
#endif
}
