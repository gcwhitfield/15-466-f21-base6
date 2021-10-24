#include "Mode.hpp"

#include "Connection.hpp"
#include "ColorTextureProgram.hpp"
#include "LitColorTextureProgram.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Sound.hpp"
#include "WalkMesh.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode(Client &client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----
	struct OtherPlayersData {
		OtherPlayersData(glm::vec3 p) : position(p) {};
		glm::vec3 position;
	};
	std::map<std::string, OtherPlayersData> other_players_data;

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	glm::vec3 player_pos;
	float move_speed = 0.01;
	
	Scene scene;
	
	struct Player {
		WalkPoint at;
		// transform is at player's feet and will be yawed by mouse left/right motion;
		Scene::Transform *transform = nullptr;
		// camera is at player's head and will be pitched by mouse up/down motion;
		Scene::Camera *camera = nullptr;
	} player;
	// ---------- 

	// ----- networking -----
	//last message from server:
	std::string server_message;

	//connection to server:
	Client &client;
	// ----------


	// ----- font rendering -----
	GLuint font_vertex_buffer = 0;
	GLuint font_vertex_attributes = 0;
	ColorTextureProgram font_program;
	FT_Face face; 
	GLuint test_texture = -1;

	// the Character struct and the characters map were inspired by the 
	// OpenGL documentation about text rendering: 
	// https://learnopengl.com/In-Practice/Text-Rendering
	struct Character {
		unsigned int TextureID;  // ID handle of the glyph texture
		glm::ivec2   Size;       // Size of glyph
		glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
	};

	std::map<char, Character> characters;
	// ----------

	// ----- misc ----- 
	Scene::Drawable *other_player_base; // base object from which to instantiate other players into the scene
	// ----------
};

// ----- helpful drawing stuff ----- 
// The vertex class was copied from the NEST framework
// draw functions will work on vectors of vertices, defined as follows:
struct Vertex {
    Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
        Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
    glm::vec3 Position;
    glm::u8vec4 Color;
    glm::vec2 TexCoord;
};

// HEX_TO_U8VEC4 was copied from the NEST framework
// some nice colors from the course web page:
#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))

// inline helper functions for drawing shapes. The triangles are being counter clockwise.
// draw_rectangle copied from NEST framework
inline void draw_rectangle (std::vector<Vertex> &verts, glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
    verts.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.0f, 0.0f));
    verts.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.0f, 1.0f));
    verts.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(1.0f, 1.0f));

    verts.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.0f, 0.0f));
    verts.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(1.0f, 1.0f));
    verts.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(1.0f, 0.0f));
};

inline void draw_quadrilateral (std::vector<Vertex> &verts, glm::vec2 const &top_left, glm::vec2 const &top_right, glm::vec2 const &bot_left, glm::vec2 const &bot_right, glm::u8vec4 const &color) {
    // the body of this function was copied largely from Professor McCann's start code for 'draw_rectangle's
    verts.emplace_back(glm::vec3(top_left.x, top_left.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
    verts.emplace_back(glm::vec3(top_right.x, top_right.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
    verts.emplace_back(glm::vec3(bot_right.x, bot_right.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

    verts.emplace_back(glm::vec3(top_left.x, top_left.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
    verts.emplace_back(glm::vec3(bot_right.x, bot_right.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
    verts.emplace_back(glm::vec3(bot_left.x, bot_left.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
};