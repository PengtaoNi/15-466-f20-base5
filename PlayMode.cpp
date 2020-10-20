#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>

GLuint phonebank_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > phonebank_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("phone-bank.pnct"));
	phonebank_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > phonebank_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("phone-bank.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = phonebank_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = phonebank_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

WalkMesh const *walkmesh = nullptr;
Load< WalkMeshes > phonebank_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	WalkMeshes *ret = new WalkMeshes(data_path("phone-bank.w"));
	walkmesh = &ret->lookup("WalkMesh");
	return ret;
});

PlayMode::PlayMode() : scene(*phonebank_scene) {
	for (auto& transform : scene.transforms) {
		if (transform.name == "Player") player1.transform = &transform;
		if (transform.name == "Player2") player2.transform = &transform;
	}

	//start player walking at nearest walk point:
	player1.at = walkmesh->nearest_walk_point(player1.transform->position);
	player2.at = walkmesh->nearest_walk_point(player2.transform->position);
	start1 = player1.at;
	start2 = player2.at;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left1.downs += 1;
			left1.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right1.downs += 1;
			right1.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up1.downs += 1;
			up1.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down1.downs += 1;
			down1.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_LEFT) {
			left2.downs += 1;
			left2.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right2.downs += 1;
			right2.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up2.downs += 1;
			up2.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down2.downs += 1;
			down2.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_r) {
			if (game_over) restart = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left1.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right1.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up1.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down1.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_LEFT) {
			left2.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right2.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up2.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down2.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	//player walking:
	{
		//combine inputs into a move:
		if (game_over) return;

		if (restart) {
			player1.at = start1;
			player2.at = start2;
			move1 = glm::vec2(0.0f, 0.0f);
			move2 = glm::vec2(0.0f, 0.0f);
			colliding = false;

			game_over = false;
			restart = false;
			return;
		}

		if (!colliding) {
			move1 += glm::vec2(player1.transform->position.x, -player1.transform->position.y) * 0.1f * elapsed;
			move2 += glm::vec2(player2.transform->position.x, -player2.transform->position.y) * 0.1f * elapsed;
			if (left1.pressed && !right1.pressed) move1.x += elapsed * 0.1f;
			if (!left1.pressed && right1.pressed) move1.x -= elapsed * 0.1f;
			if (down1.pressed && !up1.pressed) move1.y -= elapsed * 0.1f;
			if (!down1.pressed && up1.pressed) move1.y += elapsed * 0.1f;
			if (left2.pressed && !right2.pressed) move2.x += elapsed * 0.1f;
			if (!left2.pressed && right2.pressed) move2.x -= elapsed * 0.1f;
			if (down2.pressed && !up2.pressed) move2.y -= elapsed * 0.1f;
			if (!down2.pressed && up2.pressed) move2.y += elapsed * 0.1f;
		}

		//get move in world coordinate system:
		glm::vec3 remain1 = player1.transform->make_local_to_world() * glm::vec4(move1.x, move1.y, 0.0f, 0.0f);
		glm::vec3 remain2 = player2.transform->make_local_to_world() * glm::vec4(move2.x, move2.y, 0.0f, 0.0f);

		//using a for() instead of a while() here so that if walkpoint gets stuck in
		//some awkward case, code will not infinite loop:
		for (uint32_t iter = 0; iter < 10; ++iter) {
			if (remain1 == glm::vec3(0.0f)) break;
			WalkPoint end;
			float time;
			walkmesh->walk_in_triangle(player1.at, remain1, &end, &time);
			player1.at = end;
			if (time == 1.0f) {
				//finished within triangle:
				remain1 = glm::vec3(0.0f);
				break;
			}
			//some step remains:
			remain1 *= (1.0f - time);
			//try to step over edge:
			glm::quat rotation;
			if (walkmesh->cross_edge(player1.at, &end, &rotation)) {
				//stepped to a new triangle:
				player1.at = end;
				//rotate step to follow surface:
				remain1 = rotation * remain1;
			} else {
				//ran into a wall, bounce / slide along it:
				glm::vec3 const &a = walkmesh->vertices[player1.at.indices.x];
				glm::vec3 const &b = walkmesh->vertices[player1.at.indices.y];
				glm::vec3 const &c = walkmesh->vertices[player1.at.indices.z];
				glm::vec3 along = glm::normalize(b-a);
				glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));
				glm::vec3 in = glm::cross(normal, along);

				//check how much 'remain' is pointing out of the triangle:
				float d = glm::dot(remain1, in);
				if (d < 0.0f) {
					//bounce off of the wall:
					remain1 += (-1.25f * d) * in;
				} else {
					//if it's just pointing along the edge, bend slightly away from wall:
					remain1 += 0.01f * d * in;
				}
			}
		}
		if (remain1 != glm::vec3(0.0f)) {
			std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
		}
		for (uint32_t iter = 0; iter < 10; ++iter) {
			if (remain2 == glm::vec3(0.0f)) break;
			WalkPoint end;
			float time;
			walkmesh->walk_in_triangle(player2.at, remain2, &end, &time);
			player2.at = end;
			if (time == 1.0f) {
				//finished within triangle:
				remain2 = glm::vec3(0.0f);
				break;
			}
			//some step remains:
			remain2 *= (1.0f - time);
			//try to step over edge:
			glm::quat rotation;
			if (walkmesh->cross_edge(player2.at, &end, &rotation)) {
				//stepped to a new triangle:
				player2.at = end;
				//rotate step to follow surface:
				remain2 = rotation * remain2;
			}
			else {
				//ran into a wall, bounce / slide along it:
				glm::vec3 const& a = walkmesh->vertices[player2.at.indices.x];
				glm::vec3 const& b = walkmesh->vertices[player2.at.indices.y];
				glm::vec3 const& c = walkmesh->vertices[player2.at.indices.z];
				glm::vec3 along = glm::normalize(b - a);
				glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
				glm::vec3 in = glm::cross(normal, along);

				//check how much 'remain' is pointing out of the triangle:
				float d = glm::dot(remain2, in);
				if (d < 0.0f) {
					//bounce off of the wall:
					remain2 += (-1.25f * d) * in;
				}
				else {
					//if it's just pointing along the edge, bend slightly away from wall:
					remain2 += 0.01f * d * in;
				}
			}
		}
		if (remain2 != glm::vec3(0.0f)) {
			std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
		}

		//update player's position to respect walking:
		player1.transform->position = walkmesh->to_world_point(player1.at);
		player2.transform->position = walkmesh->to_world_point(player2.at);
		std::cout << player1.transform->position.x << ' ' << player1.transform->position.y << '\n';

		float separation = glm::length(player1.transform->position - player2.transform->position);
		if (colliding && separation > 2.0f) colliding = false;

		if (!colliding && separation <= 2.0f) {
			colliding = true;

			glm::vec2 pos1 = glm::vec2(player1.transform->position.x, player1.transform->position.y);
			glm::vec2 pos2 = glm::vec2(player2.transform->position.x, player2.transform->position.y);
			glm::vec2 norm = glm::normalize(pos1 - pos2);

			glm::vec2 move_sum = move1 + move2;
			glm::vec2 move_of_2_wrt_1 = move2 - move1;
			move1 = glm::dot(norm, move_of_2_wrt_1) * norm + move1;
			move2 = move_sum - move1;
		}

		{ //update player's rotation to respect local (smooth) up-vector:			
			glm::quat adjust = glm::rotation(
				player1.transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f), //current up vector
				walkmesh->to_world_smooth_normal(player1.at) //smoothed up vector at walk location
			);
			player1.transform->rotation = glm::normalize(adjust * player1.transform->rotation);
			adjust = glm::rotation(
				player2.transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f), //current up vector
				walkmesh->to_world_smooth_normal(player2.at) //smoothed up vector at walk location
			);
			player2.transform->rotation = glm::normalize(adjust * player2.transform->rotation);
		}
	}

	glm::vec2 pos1 = glm::vec2(player1.transform->position.x, player1.transform->position.y);
	glm::vec2 pos2 = glm::vec2(player2.transform->position.x, player2.transform->position.y);
	if (glm::length(pos1) > 6.9) {
		game_over = true;
		winner = 2;
	}
	else if (glm::length(pos1) > 6.9) {
		game_over = true;
		winner = 1;
	}

	//reset button press counters:
	left1.downs = 0;
	right1.downs = 0;
	up1.downs = 0;
	down1.downs = 0;
	left2.downs = 0;
	right2.downs = 0;
	up2.downs = 0;
	down2.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1000.0f, 1000.0f, 1000.0f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);
	if (game_over) { //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Game over! Press R to restart.",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Game over! Press R to restart.",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}
