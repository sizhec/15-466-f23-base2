#include "Mode.hpp"

#include "Scene.hpp"

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

	//helped i have 

	bool PlayMode::checkCollision(Scene::Transform *first, Scene::Transform *second);
	void PlayMode::saveBegin(Scene::Transform *player, Scene::Transform *enemy);

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, reset, bl, br;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	// Scene::Transform *hip = nullptr;
	// Scene::Transform *upper_leg = nullptr;
	// Scene::Transform *lower_leg = nullptr;


	Scene::Transform *road = nullptr;
	Scene::Transform *player = nullptr;
	Scene::Transform *enemy = nullptr;

	glm::vec3 playerPos = glm::vec3(0.0f, 0.0f,0.0f);
	glm::vec3 enemyPos = glm::vec3(0.0f, 0.0f,0.0f);


	float enemySpeed = 0.1f;

	int dodged = 0;

	glm::quat hip_base_rotation;
	glm::quat upper_leg_base_rotation;
	glm::quat lower_leg_base_rotation;
	float wobble = 0.0f;
	float enegyBound = 5.0f;
	float energy = 0.0f;

	
	//camera:
	Scene::Camera *camera = nullptr;

	//constant

	bool lost = false;
	bool canDash = false;

};
