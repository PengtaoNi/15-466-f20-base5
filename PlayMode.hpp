#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left1, right1, down1, up1, left2, right2, down2, up2;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//player info:
	struct Player {
		WalkPoint at;
		//transform is at player's feet and will be yawed by mouse left/right motion:
		Scene::Transform *transform = nullptr;
		//camera is at player's head and will be pitched by mouse up/down motion:
		Scene::Camera *camera = nullptr;
	} player1, player2;

	WalkPoint start1;
	WalkPoint start2;

	glm::vec2 move1 = glm::vec2(0.0f, 0.0f);
	glm::vec2 move2 = glm::vec2(0.0f, 0.0f);
	bool colliding = false;

	bool game_over = false;
	bool restart = false;

	int winner = 0;

	//camera:
	Scene::Camera* camera = nullptr;
};
