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

/**
 * @brief Constructs a shader program for the water.
 */
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

/**
 * @brief Constructs a flat square with a water shader.
 */
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

/**
 * @brief Constructs a scene of a lake surrounded by cliffs. Does include the water plane.
 */
Scene lake() {
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

/**
 * @brief Constructs a scene of a bass swimming up to eat a duck.
 */
Scene bass(ShaderProgram shaderProgram) {
    Scene scene{ shaderProgram };

    auto bass = assimpLoad("models/bass/scene.gltf", true);
    bass.grow(glm::vec3(7, 7, 7));
    bass.move(glm::vec3(-5, -2, 0));
    bass.rotate(glm::vec3(0, M_PI/2, 0));
    scene.objects.push_back(std::move(bass));

    auto duck = assimpLoad("models/duck/source/Yellow rubber duck/Rubbish_Duck.gltf", true);
    duck.grow(glm::vec3(.25, 0.25, 0.25));
    duck.rotate(glm::vec3(0, M_PI/4, 0));
    duck.move(glm::vec3(-3, 0, -3));
    scene.objects.push_back(std::move(duck));

    // Duck slowly moving to center of lake
    Animator moveDuck;
    moveDuck.addAnimation(std::make_unique<TranslationAnimation>(scene.objects[1], 10.0, glm::vec3(3, 0, 3)));
    scene.animators.push_back(std::move(moveDuck));
    // Bass rotating up to eat duck
    Animator rotateBass;
    rotateBass.addAnimation(std::make_unique<PauseAnimation>(scene.objects[0], 7.0));
    rotateBass.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 3, glm::vec3(0, 0, M_PI/4)));
    rotateBass.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 1, glm::vec3(0, 0, -M_PI/4)));
    rotateBass.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 2, glm::vec3(0, 0, -M_PI/4)));
    rotateBass.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 2, glm::vec3(0, 0, M_PI/4)));

    scene.animators.push_back(std::move(rotateBass));
    Animator quadraticBass;
    quadraticBass.addAnimation(std::make_unique<PauseAnimation>(scene.objects[0], 5.0));
    quadraticBass.addAnimation(std::make_unique<QuadraticBezierAnimation>(scene.objects[0], 5.0,
                                                                          glm::vec3(-5, -2, 0),
                                                                          glm::vec3(-2, -1.75, 0),
                                                                          glm::vec3(-0.5, -0.25, 0)));
    quadraticBass.addAnimation(std::make_unique<QuadraticBezierAnimation>(scene.objects[0], 5.0,
                                                                          glm::vec3(-0.5, -0.25, 0),
                                                                          glm::vec3(2, -0.5, 0),
                                                                          glm::vec3(4, -2, 0)));
    scene.animators.push_back(std::move(quadraticBass));


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

    auto bassScene = bass(myScene.program);

    // Activate the shader program.
	myScene.program.activate();

	// Set up the view and projection matrices.
    // Top View
	glm::vec3 cameraPos = glm::vec3(0, 18, 1);
    glm::vec3 center = glm::vec3(0, 0, 0);
    glm::vec3 up = glm::vec3(0, 0, -1);
    glm::mat4 camera = glm::lookAt(cameraPos, center, up);
    // flat view outside waterScene
    //cameraPos = glm::vec3(0, 0.5, 15);
    //center = glm::vec3(0, 0, 0);
    //up = glm::vec3(0, 1, 0);
    //camera = glm::lookAt(cameraPos, center, up);

    // flat view above waterScene
    cameraPos = glm::vec3(5, 3, 5);
    center = glm::vec3(0, 0, 0);
    up = glm::vec3(0, 1, 0);
    camera = glm::lookAt(cameraPos, center, up);

    // flat view below waterScene
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

    // Generate and bind a custom framebuffer for the waterScene's reflection.
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

    // Generate and bind a custom framebuffer for the waterScene's refraction.
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

    auto waterScene = water(reflectionBufferId, refractionBufferId);

    waterScene.program.activate();
    waterScene.program.setUniform("view", camera);
    waterScene.program.setUniform("projection", perspective);
    waterScene.program.setUniform("viewPos", cameraPos);
    waterScene.program.setUniform("moveFactor", 0.0f);

    myScene.program.activate();

    // Values for calculating the waterScene's wave movement that will be passed to the shader
    const float WAVE_SPEED = 0.03f;
    float moveFactor = 0.0f;

    // Ready, set, go!
	bool running = true;
	sf::Clock c;
	auto last = c.getElapsedTime();

	// Start the animators.
	for (auto& anim : bassScene.animators) {
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
		for (auto& anim : bassScene.animators) {
			anim.tick(diff.asSeconds());
		}

		// Clear the OpenGL "context".
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.65f, 0.8f, 0.92f, 1.0f); // set the background to sky color

        glEnable(GL_CLIP_DISTANCE0);
        // First render:
        // Will render to the screen, just renders to the frame buffer
        // Render reflection texture
        glBindFramebuffer(GL_FRAMEBUFFER, myFbo1);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionBufferId, 0);
        float distance = 2 * cameraPos.y;
        cameraPos.y -= distance; // change the camera position to be below the waterScene
        camera = glm::lookAt(cameraPos, center, up);
        myScene.program.setUniform("plane", glm::vec4(0, 1, 0, 0));
        myScene.program.setUniform("view", camera);
        for (auto& o : myScene.objects) {
            o.render(myScene.program);
        }
        for (auto& o : bassScene.objects) {
            o.render(bassScene.program);
        }

        // Undo the camera position change
        cameraPos.y += distance;
        camera = glm::lookAt(cameraPos, center, up);
        myScene.program.setUniform("view", camera);

        // Second render:
        // Render refraction texture
        glBindFramebuffer(GL_FRAMEBUFFER, myFbo2);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, refractionBufferId, 0);
        myScene.program.setUniform("plane", glm::vec4(0, -1, 0, 0));
        for (auto& o : myScene.objects) {
            o.render(myScene.program);
        }
        for (auto& o : bassScene.objects) {
            o.render(bassScene.program);
        }

        // Switch back to the default framebuffer. Scene will now render to the display.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Render the scene objects.
        glDisable(GL_CLIP_DISTANCE0);
        for (auto& o : myScene.objects) {
			o.render(myScene.program);
        }
        for (auto& o : bassScene.objects) {
            o.render(bassScene.program);
        }

        // Render the waterScene
        waterScene.program.activate();
        moveFactor += WAVE_SPEED * diff.asSeconds();
        moveFactor = fmod(moveFactor, 1.0);
        waterScene.program.setUniform("moveFactor", moveFactor);
        waterScene.program.setUniform("reflectionTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, reflectionBufferId);
        waterScene.program.setUniform("refractionTexture", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, refractionBufferId);
        waterScene.objects[0].render(waterScene.program);

        // reactivate main shader
        myScene.program.activate();

        // Remove the duck after 10.0 seconds since it has been eaten by the bass
        if(c.getElapsedTime().asSeconds() > 10.0){
            if(bassScene.objects.size() > 1) {
                bassScene.objects.pop_back();
            }
        }

		window.display();
	}

	return 0;
}


