#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <random>

// GLuint pie_meshes_for_lit_color_texture_program = 0;
// Load< MeshBuffer > pie_meshes(LoadTagDefault, []() -> MeshBuffer const * {
// 	MeshBuffer const *ret = new MeshBuffer(data_path("pie_scene.pnct"));
// 	pie_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
// 	return ret;
// });
// Load< Scene > pie_scene(LoadTagDefault, []() -> Scene const * {
// 	return new Scene(data_path("pie_scene.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name) {
// 		Mesh const &mesh = pie_meshes->lookup(mesh_name);

// 		scene.drawables.emplace_back(transform);
// 		Scene::Drawable &drawable = scene.drawables.back();

// 		drawable.pipeline = lit_color_texture_program_pipeline;

// 		drawable.pipeline.vao = pie_meshes_for_lit_color_texture_program;
// 		drawable.pipeline.type = mesh.type;
// 		drawable.pipeline.start = mesh.start;
// 		drawable.pipeline.count = mesh.count;
// 	});
// });

PlayMode::PlayMode(Client &client_) : client(client_) {

	// ----- init game state ------
	player_pos = glm::vec2(0, 0);

	// { // font initialization

	// 	{ // vertex array mapping buffer for font_program

	// 		// tell OpenGl to create a vertex buffer object
	// 		glGenBuffers(1, &font_vertex_buffer);

	// 		// tell OpenGL to create a vertex attributes array
	// 		glGenVertexArrays(1, &font_vertex_attributes);

	// 		// set font_vertex_attributes a the current vertex array object
	// 		glBindVertexArray(font_vertex_attributes);
			
	// 		// set font_vertex_buffer as the source of glVertexAttribPointer
	// 		glBindBuffer(GL_ARRAY_BUFFER, font_vertex_buffer);

	// 		// set up the vertex array object to describe vertices
	// 		glVertexAttribPointer(
	// 			font_program.Position_vec4, // attribute
	// 			3, // size
	// 			GL_FLOAT, // type
	// 			GL_FALSE, // normalized QUESTION: what does normalized mean in this case
	// 			sizeof(Vertex), // stride QUESTION: what does stride do in this case
	// 			(GLbyte *)0 + 0 // offset
	// 		);
	// 		glEnableVertexAttribArray(font_program.Position_vec4);
	// 		// reminder: it's okay to bind a vec4 to a vec3 attribute, because the fourth 
	// 		// dimension will automatically get filled in with 1

	// 		glVertexAttribPointer(
	// 			font_program.Color_vec4, // attribute
	// 			4, 
	// 			GL_UNSIGNED_BYTE, // data type
	// 			GL_TRUE, // normalized
	// 			sizeof(Vertex), // stride
	// 			(GLbyte *)0 + 4*3 // offset
	// 		);
	// 		glEnableVertexAttribArray(font_program.Color_vec4);

	// 		glVertexAttribPointer(
	// 			font_program.TexCoord_vec2, // attribute
	// 			2, // size
	// 			GL_FLOAT, // type
	// 			GL_FALSE, // normalized
	// 			sizeof(Vertex), // stride
	// 			(GLbyte *)0 + 4*3 + 4*1 // offset
	// 		);
	// 		glEnableVertexAttribArray(font_program.TexCoord_vec2);

	// 		// we are now done talking about font_vertex_buffer, so we must unbind it
	// 		glBindBuffer(GL_ARRAY_BUFFER, 0);

	// 		// we are done setting up the vertex array object, so we must unbind it
	// 		glBindVertexArray(0);

	// 		GL_ERRORS(); // PARANOIA
	// 	}

	// 	{ // make a texture using freetype

	// 		// 1) Load font with Freetype
	// 		// copied from https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
	// 		FT_Library ft;
	// 		if (FT_Init_FreeType(&ft))
	// 		{
	// 			std::cout << "ERROR::FREETYPE:: Could not init FreeType Library " << std::endl;
	// 			exit(0);
	// 		}

	// 		std::string fontFile = data_path("font/Roboto/Roboto-Medium.ttf");
	// 		if (FT_New_Face(ft, fontFile.c_str(), 0, &face))
	// 		{
	// 			std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
	// 			exit(0);
	// 		}

	// 		// disable alignment since what we read from the face (font) is gray-scale
	// 		// this line was copied from https://github.com/ChunanGang/TextBasedGame/blob/main/TextRenderer.cpp
	// 		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// 		FT_Set_Pixel_Sizes(face, 0, 48);

	// 		// 2) load character with FreeType
	// 		if (FT_Load_Char(face, 'p', FT_LOAD_RENDER))
	// 		{
	// 			std::cout << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
	// 			exit(0);
	// 		}

	// 		glGenTextures(1, &test_texture);
	// 		glBindTexture(GL_TEXTURE_2D, test_texture);
	// 		// upload font data to the texture
	// 		glm::uvec2 size = glm::uvec2(face->glyph->bitmap.rows, face->glyph->bitmap.width);
	// 		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
	// 		assert(size.x == face->glyph->bitmap.rows);
	// 		assert(size.y == face->glyph->bitmap.width);
	// 		assert(data.size() == size.x*size.y);
	// 		for (size_t i = 0; i < size.y; i++)
	// 		{
	// 			for (size_t j = 0; j < size.x; j++)
	// 			{
	// 				size_t index = i * size.y + j;
	// 				assert(index < data.size());
	// 				uint8_t val = face->glyph->bitmap.buffer[j * std::abs(face->glyph->bitmap.pitch) + i]; // copied from professor McCan's code example for printing bitmap buffer
	// 				data[index].x = val;
	// 				data[index].y = 0;
	// 				data[index].z = 0;
	// 				data[index].w = 255;	
	// 			}
	// 		}

	// 		glTexImage2D(
	// 			GL_TEXTURE_2D, 
	// 			0, 
	// 			GL_RGBA,
	// 			size.x, 
	// 			size.y, 
	// 			0, 
	// 			GL_RGBA, 
	// 			GL_UNSIGNED_BYTE, 
	// 			data.data()
	// 		);

	// 		// set filtering and wrapping parameters
	// 		// parameters copied from https://github.com/ChunanGang/TextBasedGame/blob/main/TextRenderer.cpp
	// 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
	// 		// since the texture uses a mipmap and we haven't created one, tell OpenGL to make one for us
	// 		glGenerateMipmap(GL_TEXTURE_2D);

	// 		// alright, we've finished uploading the texture, so we can now unbind it
	// 		glBindTexture(GL_TEXTURE_2D, 0);

	// 		GL_ERRORS(); // PARANOIA
	// 	}
	// }
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
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			player_pos.x -= move_speed;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			player_pos.x += move_speed;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			player_pos.y += move_speed;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			player_pos.y -= move_speed;
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
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//queue data for sending to server:
	//TODO: send something that makes sense for your game
	if (left.downs || right.downs || down.downs || up.downs) {
		//send a five-byte message of type 'b':
		client.connections.back().send('b');
		client.connections.back().send(left.downs);
		client.connections.back().send(right.downs);
		client.connections.back().send(down.downs);
		client.connections.back().send(up.downs);

		// send player data to the server
		std::vector<char>player_pos_data;
		char* _player_pos_data = reinterpret_cast<char*>(&player_pos);
		size_t n = sizeof(player_pos) / sizeof(char);
		for (size_t i = 0; i < n; i++)
		{
			client.connections.back().send(_player_pos_data[i]);
		}
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
				// std::cout << c->recv_buffer[0] << std::endl;
				// step 2) interpret data about other players
				uint32_t other_players_data_size = (
					(uint32_t(c->recv_buffer[0]) << 16) | (uint32_t(c->recv_buffer[1]) << 8) | (uint32_t(c->recv_buffer[2]))
				);
				uint8_t other_players_size = c->recv_buffer[3];
				c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 4);
				size_t byte_count = 0; // counts the total number of bytes read in other_players_data
				for (size_t i = 0; i < other_players_size; i++) {
					glm::vec2* oplayer_position = reinterpret_cast<glm::vec2*>(&c->recv_buffer[0]);
					byte_count += sizeof(glm::vec2);
					uint8_t oplayer_namesize = c->recv_buffer[0 + sizeof(glm::vec2)];
					std::string oplayer_name = std::string(c->recv_buffer.begin() + sizeof(glm::vec2), c->recv_buffer.begin() + sizeof(glm::vec2) + oplayer_namesize);
					byte_count += oplayer_name.size();
					assert(oplayer_name.size() == oplayer_namesize);
					// if the other player is not in the other_players_data map, add them, and 
					// then set their data
					auto opd = other_players_data.find(oplayer_name);
					if (opd == other_players_data.end()) {
						other_players_data.insert(std::pair<std::string, OtherPlayersData>(oplayer_name, OtherPlayersData(*oplayer_position)));
					} else {
						opd->second.position = *oplayer_position;
					}
					c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + sizeof(glm::vec2) + oplayer_name.size());
				}
				assert(byte_count == other_players_data_size); // PARANOIA: the number of bytes read should equal the number of bytes
				// that was specified in the message
			}
		}
	}, 0.0);
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

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

		draw_text(glm::vec2(-aspect + 0.1f,-0.9f), "(press WASD to change your total)", 0.09f);

		draw_text(player_pos, "PLAYER", 0.09f);
	}

	// { // Use freetype and harfbuzz to draw fancier text
		 
	// 	// compute window aspect ratio
	// 	float aspect = drawable_size.x / float(drawable_size.y);

	// 	glm::vec2 center = drawable_size;
	// 	center.x *= 0.5f;
	// 	center.y *= 0.5f;

	// 	// create a matrix that scales and translates correctly
	// 	// TODO: double check that this is actually correct
	// 	glm::mat4 court_to_clip = glm::mat4(
	// 		glm::vec4(aspect, 0.0f, 0.0f, 0.0f),
	// 		glm::vec4(0.0f, aspect, 0.0f, 0.0f), 
	// 		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
	// 		glm::vec4(-center.x * aspect, -center.y * aspect, 0.0f, 1.0f)
	// 	);

	// 	std::vector <Vertex> vertices;
	// 	vertices.clear();
	// 	draw_rectangle(vertices, glm::vec2(drawable_size.x * 0.5f, drawable_size.y * 0.5f), glm::vec2(0.1, 0.1), glm::u8vec4(0xff, 0xff, 0xff, 0xff));

	// 	// use alpha blending
	// 	glEnable(GL_BLEND);
	// 	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// 	// do not use the depth test
	// 	// QUESTION: what is the GL depth test?
	// 	glDisable(GL_DEPTH_TEST);

	// 	// upload vertices to font_vertex buffer
	// 	glBindBuffer(GL_ARRAY_BUFFER, font_vertex_buffer); // set font_vertex_buffer as the current vertex buffer
	// 	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_DYNAMIC_DRAW); // upload data to the vertex buffer
	// 	glBindBuffer(GL_ARRAY_BUFFER, 0); // now that we are done with the buffer, unbind it

	// 	// set font program as the current program
	// 	glUseProgram(font_program.program);

	// 	// upload OBJECT_TO_CLIP to the proper uniform location
	// 	// glm::mat4 identity(
	// 	// 	glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
	// 	// 	glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
	// 	// 	glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
	// 	// 	glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	// 	// );
		
	// 	glUniformMatrix4fv(font_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));
	// 	// QUESTION: what does the 4fv in 'ulUniformMatrix4fv' mean?

	// 	// use the mapping font_vertex_attrivures ot fetch vertex data
	// 	glBindVertexArray(font_vertex_attributes); // QUESTION: what is this line acutally doing?

	// 	// bind the text texture to location zero 
	// 	glActiveTexture(GL_TEXTURE0);
	// 	glBindTexture(GL_TEXTURE_2D, test_texture);

	// 	// run the OpenGL pipeline
	// 	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	// 	// unbind the test texture
	// 	glBindTexture(GL_TEXTURE_2D, 0);

	// 	// reset the vertex array to none
	// 	glBindVertexArray(0);

	// 	// reset current program to none
	// 	glUseProgram(0);

	// }

	GL_ERRORS();
}
