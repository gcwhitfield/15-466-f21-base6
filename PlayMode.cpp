#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>

GLuint pie_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > pie_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("twin-circles.pnct"));
	pie_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});
Load< Scene > phonebank(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("twin-circles.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name) {
		Mesh const &mesh = pie_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = pie_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;
	});
});

WalkMesh const *walkmesh = nullptr;
Load< WalkMeshes > phonebank_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	WalkMeshes *ret = new WalkMeshes(data_path("twin-circles.w"));
	walkmesh = &ret->lookup("WalkMesh");
	return ret;
});

PlayMode::PlayMode(Client &client_) : scene(*phonebank), client(client_) { 

	{ // initialize the scene
		scene.transforms.emplace_back(); // add player transform
		player.transform = &scene.transforms.back();

	 	// create a player camera attached to a child of the player transform
		scene.transforms.emplace_back();
		scene.cameras.emplace_back(&scene.transforms.back());
		player.camera = &scene.cameras.back();
		player.camera->fovy = glm::radians(60.0f);
		player.camera->near = 0.01;
		player.camera->transform->parent = player.transform;

		// player's eyes are 1.8 units above the ground
		player.camera->transform->position = glm::vec3(0.0f, 0.0f, 1.8f);

		// rotate camera facing direction (-z) to player facing direction (+y)
		player.camera->transform->position = glm::vec3(0.0f, 0.0f, 1.8f);

		// start player walking at nearest walkpint
		player.at = walkmesh->nearest_walk_point(player.transform->position);
		player.transform->position = walkmesh->to_world_point(player.at);
	
		// find the player base drawable, assign to variable
		// bool has_set_pie_base = false;
		for (auto d : scene.drawables) {
			if (d.transform->name == "Player") {
				other_player_base = d.pipeline;
			} else if (d.transform->name.find("PieSpawnPoint") < d.transform->name.size() - std::string("PieSpawnPoint").size()) {
				pie_spawn_locations.push_back(d.transform);
				pies.emplace_back(d);
			}
		}

		num_pies_total = pies.size();
		num_pies_collected = 0;
	}


	// ----- init game state ------
	player_pos = glm::vec3(0, 0, 0);

}

PlayMode::~PlayMode() {
	{ // deallocate font resouces
		FT_Done_Face(face);

		glDeleteTextures(1, &test_texture);
		test_texture = 0;

		glDeleteBuffers(1, &font_vertex_buffer);
		font_vertex_buffer = 0;

		glDeleteVertexArrays(1, &font_vertex_attributes);
		font_vertex_attributes = 0;
	}
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.repeat) {
			//ignore repeats
		} 

		// ------- 'unlock mouse when escape key is pressed' functionality copied from game 5 base code
		else if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
		} 
		
		else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}

	// ----- mouse input code copied from game 5 base code -----
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			glm::vec3 up = walkmesh->to_world_smooth_normal(player.at);
			player.transform->rotation = glm::angleAxis(-motion.x * player.camera->fovy, up) * player.transform->rotation;
			float pitch = glm::pitch(player.camera->transform->rotation);
			pitch += motion.y * player.camera->fovy;
			// camera looks down -z (basically at the player's feet) when pitch is at zero
			pitch = std::min(pitch, 0.95f * float(M_PI));
			pitch = std::max(pitch, 0.05f * float(M_PI));
			player.camera->transform->rotation = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));

			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	// ------ player walking code copied from game 5 base code -----
	// player walking 
	{
		// combine inputs into a move;
		constexpr float PlayerSpeed = 9.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x = -1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed & !up.pressed)     move.y = -1.0f;
		if (!down.pressed && up.pressed)	move.y = 1.0f;

		// make it so that moving diagonally doesn't go faster
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		// get move in world coordinate system
		glm::vec3 remain = player.transform->make_local_to_world() * glm::vec4(move.x, move.y, 0.0f, 0.0f);

		// using a for() instead of a while() here so that if walkpoint gets stuck i
		// some awkward case, code will not infinite loop:
		for (uint32_t iter = 0; iter < 10; ++iter) {
			if (remain == glm::vec3(0.0f)) break;
			WalkPoint end;
			float time;
			walkmesh->walk_in_triangle(player.at, remain, &end, &time);
			player.at = end;
			if (time == 1.0f) {
				// finished walking within triangle
				remain = glm::vec3(0.0f);
				break;
			}
			// some step remains
			remain *= (1.0f - time);
			// try to step over an edge
			glm::quat rotation;
			if (walkmesh->cross_edge(player.at, &end, &rotation)) {
				// stepped to a new triangle
				player.at = end;
				// rotate step to follow surface
				remain = rotation * remain;
			} else {
				// ran into a wall, bounce / slide along the wall
				glm::vec3 const &a = walkmesh->vertices[player.at.indices.x];
				glm::vec3 const &b = walkmesh->vertices[player.at.indices.y];
				glm::vec3 const &c = walkmesh->vertices[player.at.indices.z];
				glm::vec3 along = glm::normalize(b-a);
				glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));
				glm::vec3 in = glm::cross(normal, along);

				// check how much 'remain' is pointing out of th triangle
				float d = glm::dot(remain, in);
				if (d < 0.0f) {
					// bounce off the wall
					remain += (1.25f * d) * in;
				} else {
					// if it's just pointing along the edge, bend slightly away from wall:
					remain += 0.01f * d * in;
				}
			}
		}

		if (remain != glm::vec3(0.0f)) {
			std::cout << "NOTE: code used full iteration budget for walking! :(" << std::endl;
		}

		player.transform->position = walkmesh->to_world_point(player.at);
		
		{ // update player's rotation to respect local (smooth) up-vector:
			glm::quat adjust = glm::rotation(
				player.transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f), // current up vector
				walkmesh->to_world_smooth_normal(player.at) // smoothed up normal vector at walk location
			);
			player.transform->rotation = glm::normalize(adjust * player.transform->rotation);

		}
  	}

	// queue data for sending to server:
	// send a message of that starts with 'b', and contains button press data and
	// player location data
	client.connections.back().send('b');
	client.connections.back().send((uint8_t)num_pies_collected);
	client.connections.back().send((uint8_t)has_won);

	player_pos = player.transform->position;
	// send player data to the server
	std::vector<char>player_pos_data;
	char* _player_pos_data = reinterpret_cast<char*>(&player_pos);
	size_t n = sizeof(player_pos) / sizeof(char);
	for (size_t i = 0; i < n; i++)
	{
		client.connections.back().send(_player_pos_data[i]);
	}
	
	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush();
			//expecting message(s) like 'm' + 3-byte length + length bytes of text:
			while (c->recv_buffer.size() >= 4) {
				// step 1) interpret server message 
				std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush();
				char type = c->recv_buffer[0];
				if (type != 'm') {
					throw std::runtime_error("Server sent unknown message type '" + std::to_string(type) + "'");
				}
				uint32_t server_message_size = (
					(uint32_t(c->recv_buffer[1]) << 16) | (uint32_t(c->recv_buffer[2]) << 8) | (uint32_t(c->recv_buffer[3]))
				);
				if (c->recv_buffer.size() < 4 + server_message_size) break; //if whole message isn't here, can't process
				//whole message *is* here, so set current server message:
				server_message = std::string(c->recv_buffer.begin() + 4, c->recv_buffer.begin() + 4 + server_message_size);

				//and consume this part of the buffer:
				c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 4 + server_message_size);
				
				// step 2) interpret data about other players
				uint32_t other_players_data_size = (
					(uint32_t(c->recv_buffer[0]) << 16) | (uint32_t(c->recv_buffer[1]) << 8) | (uint32_t(c->recv_buffer[2]))
				);
				uint8_t other_players_size = c->recv_buffer[3];
				c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 4);
				size_t byte_count = 0; // counts the total number of bytes read in other_players_data

				for (size_t i = 0; i < other_players_size; i++) {
					glm::vec3* oplayer_position = reinterpret_cast<glm::vec3*>(&c->recv_buffer[0]);
					byte_count += sizeof(glm::vec3);
					uint8_t oplayer_namesize = c->recv_buffer[0 + sizeof(glm::vec3)];
					std::string oplayer_name = std::string(c->recv_buffer.begin() + sizeof(glm::vec3), c->recv_buffer.begin() + sizeof(glm::vec3) + oplayer_namesize);
					byte_count += oplayer_name.size();
					assert(oplayer_name.size() == oplayer_namesize);

					// if the other player is not in the other_players_data map, add them, and 
					// then set their data
					auto opd = other_players_data.find(oplayer_name);
					if (opd == other_players_data.end()) {
						scene.transforms.emplace_back();
						Scene::Transform *other_player_transform = &scene.transforms.back();
						other_player_transform->position = *oplayer_position;
						scene.drawables.emplace_back(Scene::Drawable(other_player_transform));
						Scene::Drawable *other_player = &scene.drawables.back();
						other_player->pipeline = other_player_base;
						OtherPlayersData oplayer_data = OtherPlayersData(*oplayer_position);
						oplayer_data.drawable = other_player;
						other_players_data.insert(std::pair<std::string, OtherPlayersData>(oplayer_name, oplayer_data));					
					} else {
						opd->second.position = *oplayer_position;
						std::cout << opd->first << ": " << to_string(opd->second.position) << std::endl;
						assert(opd->second.drawable);
						opd->second.drawable->transform->position = *oplayer_position;
					}
					c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(glm::vec3) + oplayer_name.size());
				}
				assert(byte_count == other_players_data_size); // PARANOIA: the number of bytes read should equal the number of bytes
				// that was specified in the message
			}
		}
	}, 0.0);

	{ // if the player collides with a  pie, remove it from the list of drawables and add score
		for (auto p : pies) {
			auto distance = [](glm::vec3 v1, glm::vec3 v2) {
				return sqrt(
					pow(v1.x - v2.x, 2) +
					pow(v1.y - v2.y, 2) +
					pow(v1.z - v2.z, 2) 
				);
			};
			float min_collection_dist = 1.0f;
			if (distance(player_pos, p.transform->position) < min_collection_dist) {
				num_pies_collected++;
				p.transform->position.y += 100000; // move the pie really far away so it 
				// appears like it has been removed from the drawables
			}
		}
	}
	// if you collect all the pies then you win the game
	if (num_pies_collected >= num_pies_total) {
		has_won = true; // has_won gets sent to the server
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	{ // draw the scene
		glUseProgram(lit_color_texture_program->program);
		glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
		glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
		glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0, 1.0, 0.95)));
		glUseProgram(0);
		glClearDepth(1.0f); // 1.0 is actuallt the default value to clear the depth buffer to, but FYI you can change it
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS); // this is the default depth compression function, but FYU you can change it

		scene.draw(*player.camera);
		
		GL_ERRORS();
	}

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		auto draw_text = [&](glm::vec2 const &at, std::string const &text, float H) {
			lines.draw_text(text,
				glm::vec3(at.x, at.y, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text(text,
				glm::vec3(at.x + ofs, at.y + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		};

		draw_text(glm::vec2(-aspect + 0.1f, 0.0f), server_message, 0.09f);

		draw_text(glm::vec2(-aspect + 0.1f,-0.9f), "Use WASD to move around, walk toward a pie to collect it. Whoever collects the most pies wins!", 0.09f);
		
	}

	GL_ERRORS();
}
