#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>



GLuint simple_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > simple_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("simple.pnct"));
	simple_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});


Load< Scene > simple_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("simple.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = simple_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = simple_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});


void PlayMode::saveBegin(Scene::Transform *player1, Scene::Transform *enemy1){
	playerPos = player1->position;
	enemyPos = enemy1->position;
}

PlayMode::PlayMode() : scene(*simple_scene) {
	//get pointers to leg for convenience:
	// for (auto &transform : scene.transforms) {
	// 	if (transform.name == "Hip.FL") hip = &transform;
	// 	else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
	// 	else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
	// }
	// if (hip == nullptr) throw std::runtime_error("Hip not found.");
	// if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	// if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");

	// hip_base_rotation = hip->rotation;
	// upper_leg_base_rotation = upper_leg->rotation;
	// lower_leg_base_rotation = lower_leg->rotation;

	player = nullptr;
	enemy = nullptr;
	road = nullptr;

	for (auto &transform : scene.transforms) {
		if (transform.name == "player")  player = &transform;
		else if (transform.name == "enemy") enemy = &transform;
		else if (transform.name == "road") road = &transform;
	}

	saveBegin(player, enemy);

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
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_r) {
			reset.downs += 1;
			reset.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_j) {
			bl.downs += 1;
			bl.pressed = true;
			return true;
		}else if (evt.key.keysym.sym == SDLK_k) {
			br.downs += 1;
			br.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_r) {
			reset.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_j) {
			bl.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_k) {
			br.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			//SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}


bool PlayMode::checkCollision(Scene::Transform *first, Scene::Transform *second){
	constexpr float size = 0.6f;

	if ((std::abs(first->position.x - second->position.x) <= size) && (std::abs(first->position.y - second->position.y) <= size)){
		return true;
	}
	return false;
}

void PlayMode::update(float elapsed) {


	//move camera:
	{

		//combine inputs into a move:
		//constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-0.2f;
		if (!left.pressed && right.pressed) move.x = 0.2f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		//camera->transform->position += move.x * frame_right + move.y * frame_forward;
		
		enemySpeed += elapsed * 0.01f;
	    //collision checking

		if (checkCollision(player, enemy)&& !lost){
			//player->position+= glm::vec3(0.0f,0.0f, 1.0f);
			lost = true;
		}


		//enemy reset logic

		if(enemy->position.y <= 0.0f){

			// taken from https://stackoverflow.com/questions/686353/random-float-number-generation
			float r2 = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/4.0f));
			r2-=3.0f;
			enemy->position = enemyPos;
			enemy->position.x = r2;
			dodged+=1;
			
		}

		if (!lost){
			energy+= elapsed;
			canDash = (energy >= 5.0f);
			
			if (bl.pressed && canDash){
				(player->position).x-=1.5f;
				energy -= 5.0f;
				canDash = false;

			}else if(br.pressed&& canDash){
				(player->position).x+=1.5f;
				energy -= 5.0f;
				canDash = false;
			}
			player->position+= move.x * frame_right + move.y * frame_forward;
			enemy->position-=glm::vec3(0.0f,enemySpeed,0.0f);
			(player->position).x = std::clamp((player->position).x, -3.0f,1.0f);
			energy = std::clamp(energy, 0.0f,5.0f);
		}else{
			if (reset.pressed){
				lost = false;
				player->position = playerPos;
				enemy->position = enemyPos;
				enemySpeed = 0.1f;
				dodged = 0;
			}
			
		}

		//added keys
		if (reset.pressed){
				lost = false;
				player->position = playerPos;
				enemy->position = enemyPos;
				enemySpeed = 0.1f;
				dodged = 0;
		}

		


		
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	reset.downs = 0;
	bl.downs = 0;
	br.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;


		
		
		float ofsY = 2.0f / drawable_size.y;
		float ofsX = 2.0f / drawable_size.y;
		lines.draw_text("A/D for movement, J/K to dash (5s)",
			glm::vec3(-aspect + 0.1f * H + ofsY, -1.0 + 0.1f * H + ofsY, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));


		ofsY = 2200.0f / drawable_size.y;
		lines.draw_text(std::string("Dodged: " + std::to_string(dodged)) ,	
			glm::vec3(-aspect + 0.1f * H + ofsY, -1.0 + + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		if (lost) {
			ofsX = 1200.0f / drawable_size.x;
			ofsY = 1000.0f / drawable_size.y;
			lines.draw_text("You lose! Press R to restart",
			glm::vec3(-aspect + 0.1f * H + ofsY, -1.0 + + 0.1f * H + ofsX, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}

		{
			ofsX = 2200.0f / drawable_size.x;
			ofsY = 2300.0f / drawable_size.y;
			lines.draw_text(std::to_string(5.0f- energy) + "s",
			glm::vec3(-aspect + 0.1f * H + ofsY, -1.0 + + 0.1f * H + ofsX, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}


		if (canDash) {
			ofsX = 2000.0f / drawable_size.x;
			ofsY = 2200.0f / drawable_size.y;
			lines.draw_text("You can dash!",
			glm::vec3(-aspect + 0.1f * H + ofsY, -1.0 + + 0.1f * H + ofsX, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
		
	}
}
