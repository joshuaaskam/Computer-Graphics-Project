/**
This application displays a mesh in wireframe using "Modern" OpenGL 3.0+.
The Mesh3D class now initializes a "vertex array" on the GPU to store the vertices
	and faces of the mesh. To render, the Mesh3D object simply triggers the GPU to draw
	the stored mesh data.
We now transform local space vertices to clip space using uniform matrices in the vertex shader.
	See "simple_perspective.vert" for a vertex shader that uses uniform model, view, and projection
		matrices to transform to clip space.
	See "uniform_color.frag" for a fragment shader that sets a pixel to a uniform parameter.
*/
#define _USE_MATH_DEFINES
#include <glad/glad.h>
#include <iostream>
#include <memory>
#include <filesystem>
#include <math.h>

#include "AssimpImport.h"
#include "Mesh3D.h"
#include "Object3D.h"
#include "Animator.h"
#include "ShaderProgram.h"
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>

struct Scene {
	ShaderProgram program;
	std::vector<Object3D> objects;
	std::vector<Animator> animators;
};

/**
 * @brief Constructs a shader program that applies the Phong reflection model.
 */
ShaderProgram phongLightingShader() {
	ShaderProgram shader;
	try {
		// These shaders are INCOMPLETE.
		shader.load("shaders/light_perspective.vert", "shaders/lighting.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;
}

/**
 * @brief Constructs a shader program that performs texture mapping with no lighting.
 */
ShaderProgram texturingShader() {
	ShaderProgram shader;
	try {
		shader.load("shaders/texture_perspective.vert", "shaders/texturing.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;
}

ShaderProgram waterShader() {
    ShaderProgram shader;
    try {
        shader.load("shaders/water.vert", "shaders/water.frag");
    }
    catch (std::runtime_error& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        exit(1);
    }
    return shader;
}

/**
 * @brief Loads an image from the given path into an OpenGL texture.
 */
Texture loadTexture(const std::filesystem::path& path, const std::string& samplerName = "baseTexture") {
	StbImage i;
	i.loadFromFile(path.string());
	return Texture::loadImage(i, samplerName);
}

/*****************************************************************************************
*  DEMONSTRATION SCENES
*****************************************************************************************/
Scene bunny() {
	Scene scene{ texturingShader() };

	// We assume that (0,0) in texture space is the upper left corner, but some artists use (0,0) in the lower
	// left corner. In that case, we have to flip the V-coordinate of each UV texture location. The last parameter
	// to assimpLoad controls this. If you load a model and it looks very strange, try changing the last parameter.
	auto bunny = assimpLoad("models/bunny_textured.obj", true);
	bunny.grow(glm::vec3(9, 9, 9));
	bunny.move(glm::vec3(0.2, -1, 0));
	
	// Move all objects into the scene's objects list.
	scene.objects.push_back(std::move(bunny));
	// Now the "bunny" variable is empty; if we want to refer to the bunny object, we need to reference 
	// scene.objects[0]

	Animator spinBunny;
	// Spin the bunny 360 degrees over 10 seconds.
	spinBunny.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(0, 2 * M_PI, 0)));
	
	// Move all animators into the scene's animators list.
	scene.animators.push_back(std::move(spinBunny));

	return scene;
}


/**
 * @brief Demonstrates loading a square, oriented as the "floor", with a manually-specified texture
 * that does not come from Assimp.
 */
Scene marbleSquare() {
	Scene scene{ texturingShader() };

	std::vector<Texture> textures = {
		loadTexture("models/White_marble_03/Textures_2K/white_marble_03_2k_baseColor.tga", "baseTexture"),
	};
	auto mesh = Mesh3D::square(textures);
	auto floor = Object3D(std::vector<Mesh3D>{mesh});
	floor.grow(glm::vec3(5, 5, 5));
	floor.move(glm::vec3(0, -1.5, 0));
	floor.rotate(glm::vec3(-M_PI / 2, 0, 0));

	scene.objects.push_back(std::move(floor));
	return scene;
}

/**
 * @brief Loads a cube with a cube map texture.
 */
Scene cube() {
	Scene scene{ texturingShader() };

	auto cube = assimpLoad("models/cube.obj", true);

	scene.objects.push_back(std::move(cube));

	Animator spinCube;
	spinCube.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(0, 2 * M_PI, 0)));
	// Then spin around the x axis.
	spinCube.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(2 * M_PI, 0, 0)));

	scene.animators.push_back(std::move(spinCube));

	return scene;
}

/**
 * @brief Constructs a scene of a tiger sitting in a boat, where the tiger is the child object
 * of the boat.
 * @return
 */
Scene lifeOfPi() {
	// This scene is more complicated; it has child objects, as well as animators.
	Scene scene{ texturingShader() };

	auto boat = assimpLoad("models/boat/boat.fbx", true);
	boat.move(glm::vec3(0, -0.7, 0));
	boat.grow(glm::vec3(0.01, 0.01, 0.01));
	auto tiger = assimpLoad("models/tiger/scene.gltf", true);
	tiger.move(glm::vec3(0, -5, 10));
	// Move the tiger to be a child of the boat.
	boat.addChild(std::move(tiger));

	// Move the boat into the scene list.
	scene.objects.push_back(std::move(boat));

	// We want these animations to referenced the *moved* objects, which are no longer
	// in the variables named "tiger" and "boat". "boat" is now in the "objects" list at
	// index 0, and "tiger" is the index-1 child of the boat.
	Animator animBoat;
	animBoat.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10, glm::vec3(0, 2 * M_PI, 0)));
	Animator animTiger;
	animTiger.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0].getChild(1), 10, glm::vec3(0, 0, 2 * M_PI)));

	// The Animators will be destroyed when leaving this function, so we move them into
	// a list to be returned.
	scene.animators.push_back(std::move(animBoat));
	scene.animators.push_back(std::move(animTiger));

	// Transfer ownership of the objects and animators back to the main.
	return scene;
}

Scene water(uint32_t reflectionId, uint32_t refractionID) {
    Scene scene{waterShader()};
    std::vector<Texture> textures = {
            Texture{reflectionId, "reflectionTexture"},
            Texture{refractionID, "refractionTexture"},
            loadTexture("models/water/waterDUDV.png", "dudvMap")
    };
    auto water = Mesh3D::square({textures});
    auto lake = Object3D(std::vector<Mesh3D>{water});
    lake.rotate(glm::vec3(-M_PI/2, 0, 0));
    lake.move(glm::vec3(0.5, 0, 0.1));
    lake.grow(glm::vec3(7.6, 8.8, 1));
    scene.objects.push_back(lake);

    return scene;
}

Scene lake() {
    // This scene is more complicated; it has child objects, as well as animators.
    Scene scene{phongLightingShader()};

    auto cliff1 = assimpLoad("models/cliff/Cliff.obj", true);
    cliff1.move(glm::vec3(0, -2.5, -5));
    cliff1.grow(glm::vec3(3, 1.5, 1));
    scene.objects.push_back(std::move(cliff1));

    auto cliff2 = assimpLoad("models/cliff/Cliff.obj", true);
    cliff2.move(glm::vec3(5, -2.5, 0));
    cliff2.grow(glm::vec3(3, 1.5, 1));
    cliff2.rotate(glm::vec3(0, -M_PI/2, 0));
    scene.objects.push_back(std::move(cliff2));

    auto cliff3 = assimpLoad("models/cliff/Cliff.obj", true);
    cliff3.move(glm::vec3(-5, -2.5, 0));
    cliff3.grow(glm::vec3(3, 1.5, 1));
    cliff3.rotate(glm::vec3(0, -M_PI/2, 0));
    scene.objects.push_back(std::move(cliff3));

    auto cliff4 = assimpLoad("models/cliff/Cliff.obj", true);
    cliff4.move(glm::vec3(0, -2.5, 5));
    cliff4.grow(glm::vec3(3, 1.5, 1));
    cliff4.rotate(glm::vec3(0, M_PI, 0));
    scene.objects.push_back(std::move(cliff4));

    auto lakeBottom = assimpLoad("models/Rock_terrain/Rock_terrain_retopo.obj", true);
    lakeBottom.move(glm::vec3(.5, -2.8, .5));
    lakeBottom.grow(glm::vec3(1.4, 1.4, 1.4));
    scene.objects.push_back(std::move(lakeBottom));

    auto tree = assimpLoad("models/tree/scene.gltf", true);
    tree.move(glm::vec3(-4, 3, -4));
    scene.objects.push_back(std::move(tree));

    auto torch = assimpLoad("models/torch/scene.gltf", true);
    //torch.grow(glm::vec3(3, 3, 3));
    torch.move(glm::vec3(0, 1, -4));
    torch.rotate(glm::vec3(M_PI/4, 0, 0));
    scene.objects.push_back(std::move(torch));

    return scene;
}

Scene bass() {
    Scene scene{ phongLightingShader() };

    auto bass = assimpLoad("models/bass/scene.gltf", true);
    bass.grow(glm::vec3(7, 7, 7));
    bass.move(glm::vec3(-4, -1, 0));
    bass.rotate(glm::vec3(0, M_PI/2, 0));
    scene.objects.push_back(std::move(bass));

    auto duck = assimpLoad("models/duck/source/Yellow rubber duck/Rubbish_Duck.gltf", true);
    duck.grow(glm::vec3(.25, 0.25, 0.25));
    duck.rotate(glm::vec3(0, M_PI/4, 0));
    duck.move(glm::vec3(-3, 0, -3));
    scene.objects.push_back(std::move(duck));

    /*Animator moveBass;
    moveBass.addAnimation(std::make_unique<TranslationAnimation>(scene.objects[0], 5.0, glm::vec3(0, 0, 12)));

    scene.animators.push_back(std::move(moveBass));*/

    return scene;
}

int main() {
	
	std::cout << std::filesystem::current_path() << std::endl;
	
	// Initialize the window and OpenGL.
	sf::ContextSettings settings;
	settings.depthBits = 24; // Request a 24 bits depth buffer
	settings.stencilBits = 8;  // Request a 8 bits stencil buffer
	settings.antialiasingLevel = 2;  // Request 2 levels of antialiasing
	settings.majorVersion = 3;
	settings.minorVersion = 3;
	sf::Window window(sf::VideoMode{ 1200, 800 }, "Modern OpenGL", sf::Style::Resize | sf::Style::Close, settings);

	gladLoadGL();
	glEnable(GL_DEPTH_TEST);

    // Initialize scene objects.
	auto myScene = lake();
    // You can directly access specific objects in the scene using references.
	auto& firstObject = myScene.objects[0];

    auto bassScene = bass();


    // Activate the shader program.
	myScene.program.activate();

	// Set up the view and projection matrices.
    // Top View
	glm::vec3 cameraPos = glm::vec3(0, 18, 0);
    glm::vec3 center = glm::vec3(0, 0, 0);
    glm::vec3 up = glm::vec3(0, 0, -1);
    glm::mat4 camera = glm::lookAt(cameraPos, center, up);
    // flat view outside lake
    //cameraPos = glm::vec3(0, 0.5, 15);
    //center = glm::vec3(0, 0, 0);
    //up = glm::vec3(0, 1, 0);
    //camera = glm::lookAt(cameraPos, center, up);

    // flat view above lake
    cameraPos = glm::vec3(5, 3, 5);
    center = glm::vec3(0, 0, 0);
    up = glm::vec3(0, 1, 0);
    camera = glm::lookAt(cameraPos, center, up);

    // flat view below lake
    //cameraPos = glm::vec3(0, -0.5, 4);
    //center = glm::vec3(0, 0, 0);
    //up = glm::vec3(0, 1, 0);
    //camera = glm::lookAt(cameraPos, center, up);

    //View looking inside from front right corner
    //cameraPos = glm::vec3(7, 7, 7);
    //center = glm::vec3(0, 2, 0);
    //up = glm::vec3(0, 1, 0);
    //camera = glm::lookAt(cameraPos, center, up);

    glm::mat4 perspective = glm::perspective(glm::radians(45.0), static_cast<double>(window.getSize().x) / window.getSize().y, 0.1, 100.0);
	myScene.program.setUniform("view", camera);
	myScene.program.setUniform("projection", perspective);
	myScene.program.setUniform("cameraPos", cameraPos); // I don't know where this is used
    myScene.program.setUniform("viewPos", cameraPos);
    myScene.program.setUniform("directionalLight", glm::vec3(0, -1, 0));
    myScene.program.setUniform("directionalColor", glm::vec3(1, 1, 1));
    myScene.program.setUniform("ambientColor", glm::vec3(1, 1, 1));
    myScene.program.setUniform("material", glm::vec4(0.3, 0.7, 1, 24));
    myScene.program.setUniform("light.position", glm::vec3(0, 1, -4));
    myScene.program.setUniform("light.ambient", glm::vec3(1, 0.84, 0.69));
    myScene.program.setUniform("light.diffuse", glm::vec3(1, 0.84, 0.69));
    myScene.program.setUniform("light.specular", glm::vec3(1, 0.84, 0.69));
    myScene.program.setUniform("light.constant", 1.0f);
    myScene.program.setUniform("light.linear", 0.7f);
    myScene.program.setUniform("light.quadratic", 1.8f);


    bassScene.program.activate();
    bassScene.program.setUniform("view", camera);
    bassScene.program.setUniform("projection", perspective);
    bassScene.program.setUniform("cameraPos", cameraPos); // I don't know where this is used
    bassScene.program.setUniform("viewPos", cameraPos);
    bassScene.program.setUniform("directionalLight", glm::vec3(0, -1, 0));
    bassScene.program.setUniform("directionalColor", glm::vec3(1, 1, 1));
    bassScene.program.setUniform("ambientColor", glm::vec3(1, 1, 1));
    bassScene.program.setUniform("material", glm::vec4(0.3, 0.7, 1, 24));
    bassScene.program.setUniform("light.position", glm::vec3(0, 1, -4));
    bassScene.program.setUniform("light.ambient", glm::vec3(1, 0.84, 0.69));
    bassScene.program.setUniform("light.diffuse", glm::vec3(1, 0.84, 0.69));
    bassScene.program.setUniform("light.specular", glm::vec3(1, 0.84, 0.69));
    bassScene.program.setUniform("light.constant", 1.0f);
    bassScene.program.setUniform("light.linear", 0.7f);
    bassScene.program.setUniform("light.quadratic", 1.8f);


    // Generate and bind a custom framebuffer.
    uint32_t myFbo1;
    glGenFramebuffers(1, &myFbo1);
    glBindFramebuffer(GL_FRAMEBUFFER, myFbo1);

    uint32_t reflectionBufferId;
    glGenTextures(1, &reflectionBufferId);
    glBindTexture(GL_TEXTURE_2D, reflectionBufferId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1200, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionBufferId, 0);
// Render commands will no longer render to the screen.
    // checks if frame buffer is working.
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

// Generate and bind a custom framebuffer.
    uint32_t myFbo2;
    glGenFramebuffers(1, &myFbo2);
    glBindFramebuffer(GL_FRAMEBUFFER, myFbo2);
// Render commands will no longer render to the screen.

    uint32_t refractionBufferId;
    glGenTextures(1, &refractionBufferId);
    glBindTexture(GL_TEXTURE_2D, refractionBufferId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1200, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, refractionBufferId, 0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    auto lake = water(reflectionBufferId, refractionBufferId);

    lake.program.activate();
    lake.program.setUniform("view", camera);
    lake.program.setUniform("projection", perspective);
    lake.program.setUniform("viewPos", cameraPos);
    lake.program.setUniform("moveFactor", 0.0f);

    myScene.program.activate();

    const float WAVE_SPEED = 0.03f;
    float moveFactor = 0.0f;

    // Ready, set, go!
	bool running = true;
	sf::Clock c;
	auto last = c.getElapsedTime();

	// Start the animators.
	for (auto& anim : myScene.animators) {
		anim.start();
	}
    glEnable(GL_CULL_FACE);
	while (running) {
		
		sf::Event ev;
		while (window.pollEvent(ev)) {
			if (ev.type == sf::Event::Closed) {
				running = false;
			}
		}
		auto now = c.getElapsedTime();
		auto diff = now - last;
		std::cout << 1 / diff.asSeconds() << " FPS " << std::endl;
		last = now;

		// Update the scene.
		for (auto& anim : myScene.animators) {
			anim.tick(diff.asSeconds());
		}

		// Clear the OpenGL "context".
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.65f, 0.8f, 0.92f, 1.0f); // set the background to sky color

        glEnable(GL_CLIP_DISTANCE0);
        // Will not have visual changes, just renders to the frame buffer
        // Render reflection texture
        glBindFramebuffer(GL_FRAMEBUFFER, myFbo1);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionBufferId, 0);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        float distance = 2 * cameraPos.y;
        cameraPos.y -= distance; // change the camera position to be below the water
        camera = glm::lookAt(cameraPos, center, up); // The view that is rendered is what will be reflected
        myScene.program.setUniform("plane", glm::vec4(0, 1, 0, 0));
        myScene.program.setUniform("view", camera);
        for (auto& o : myScene.objects) {
            o.render(myScene.program);
        }
        bassScene.program.activate();
        bassScene.program.setUniform("plane", glm::vec4(0, 1, 0, 0));
        bassScene.program.setUniform("view", camera);
        for (auto& o : bassScene.objects) {
            o.render(bassScene.program);
        }

        // Undo the camera position change
        cameraPos.y += distance;
        camera = glm::lookAt(cameraPos, center, up); // The view that is rendered is what will be reflected
        bassScene.program.setUniform("view", camera);
        myScene.program.activate();
        myScene.program.setUniform("view", camera);

        // Render refraction texture
        glBindFramebuffer(GL_FRAMEBUFFER, myFbo2);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, refractionBufferId, 0);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        myScene.program.setUniform("plane", glm::vec4(0, -1, 0, 0));
        for (auto& o : myScene.objects) {
            o.render(myScene.program);
        }
        bassScene.program.activate();
        bassScene.program.setUniform("plane", glm::vec4(0, -1, 0, 0));
        for (auto& o : bassScene.objects) {
            o.render(bassScene.program);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

		// Render the scene objects.
        glDisable(GL_CLIP_DISTANCE0);
        myScene.program.activate();
        for (auto& o : myScene.objects) {
			o.render(myScene.program);
        }
        bassScene.program.activate();
        for (auto& o : bassScene.objects) {
            o.render(bassScene.program);
        }

        // Render the water
        lake.program.activate();
        moveFactor += WAVE_SPEED * diff.asSeconds();
        moveFactor = fmod(moveFactor, 1.0);
        lake.program.setUniform("moveFactor", moveFactor);
        lake.program.setUniform("reflectionTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, reflectionBufferId);
        lake.program.setUniform("refractionTexture", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, refractionBufferId);
        lake.objects[0].render(lake.program);

        // reactivate main shader
        myScene.program.activate();

		window.display();


		
	}

	return 0;
}


