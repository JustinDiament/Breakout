/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#include <glad/glad.h>
#include <SDL2/SDL.h>

#include "game.hpp"
#include "resource_manager.hpp"

#include <iostream>

// Third Party Libraries
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

// I'm using this with my setup -Justin
// #include "SDL2/SDL.h"

// C++ Standard Template Library (STL)
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

// vvvvvvvvvvvvvvvvvvvvvvvvvv Globals vvvvvvvvvvvvvvvvvvvvvvvvvv
// Globals generally are prefixed with 'g' in this application.

// Screen Dimensions
int gScreenWidth = 1000;
int gScreenHeight = 750;
SDL_Window *gGraphicsApplicationWindow = nullptr;
SDL_GLContext gOpenGLContext = nullptr;

// Main loop flag
bool gQuit = false; // If this is quit = 'true' then the program terminates.

// The Breakout game object
Game Breakout(gScreenWidth, gScreenHeight);

// The tick interval to advance a frame
// Used to make the framerate fixed intervals
int TICK_INTERVAL = 15;

// The next time to advance a frame
// Used to make the framerate fixed intervals
static Uint32 next_time;

// Track the time since last frame advanced
float deltaTime = 0.0f;
float lastFrame = 0.0f;

/**
 * Determine how much time is left until the frame should be advanced
 * Used to make the framerate fixed intervals
 *
 * @return The amount of ticks left until a frame should be advanced
 */
Uint32 time_left()
{
	Uint32 now;
	now = SDL_GetTicks();

	// If the next time to advance is before or equal to now, return 0 ticks until next advance
	if (next_time <= now)
		return 0;
	// Otherwise, return how much time is left until frame should be advanced
	else
		return next_time - now;
}

// ^^^^^^^^^^^^^^^^^^^^^^^^ Globals ^^^^^^^^^^^^^^^^^^^^^^^^^^^

// vvvvvvvvvvvvvvvvvvv Error Handling Routines vvvvvvvvvvvvvvv
static void GLClearAllErrors()
{
	while (glGetError() != GL_NO_ERROR)
	{
	}
}

// Returns true if we have an error
static bool GLCheckErrorStatus(const char *function, int line)
{
	while (GLenum error = glGetError())
	{
		std::cout << "OpenGL Error:" << error
				  << "\tLine: " << line
				  << "\tfunction: " << function << std::endl;
		return true;
	}
	return false;
}

#define GLCheck(x)      \
	GLClearAllErrors(); \
	x;                  \
	GLCheckErrorStatus(#x, __LINE__);
// ^^^^^^^^^^^^^^^^^^^ Error Handling Routines ^^^^^^^^^^^^^^^

/**
 * Initialization of the graphics application. Typically this will involve setting up a window
 * and the OpenGL Context (with the appropriate version)
 *
 * @return void
 */
void InitializeProgram()
{
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << "\n";
		exit(1);
	}
	// glEnable(GL_BLEND);

	// Setup the OpenGL Context
	// Use OpenGL 4.1 core or greater
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	// We want to request a double buffer for smooth updating.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Create an application window using OpenGL that supports SDL
	gGraphicsApplicationWindow = SDL_CreateWindow("Atari 2600 Breakout",
												  SDL_WINDOWPOS_UNDEFINED,
												  SDL_WINDOWPOS_UNDEFINED,
												  gScreenWidth,
												  gScreenHeight,
												  SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

	// Check if Window did not create.
	if (gGraphicsApplicationWindow == nullptr)
	{
		std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << "\n";
		exit(1);
	}

	// Create an OpenGL Graphics Context
	gOpenGLContext = SDL_GL_CreateContext(gGraphicsApplicationWindow);
	if (gOpenGLContext == nullptr)
	{
		std::cout << "OpenGL context could not be created! SDL Error: " << SDL_GetError() << "\n";
		exit(1);
	}

	// Initialize GLAD Library
	if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
	{
		std::cout << "glad did not initialize" << std::endl;
		exit(1);
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Initialize the breakout game
	Breakout.Init();
}

/**
 * PreDraw
 * Typically we will use this for setting some sort of 'state'
 * Note: some of the calls may take place at different stages (post-processing) of the
 * 		 pipeline.
 * @return void
 */
void PreDraw()
{
	// Set up timestamps for frame advancing
	float currentFrame = SDL_GetTicks();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Disable depth test and face culling.
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// manage user input
	// -----------------
	Breakout.ProcessInput(deltaTime);

	// update game state
	// -----------------
	Breakout.Update(deltaTime);

	// Initialize clear color
	// This is the background of the screen.
	glViewport(0, 0, gScreenWidth, gScreenHeight);
	glClearColor(0.5f, 9.f, 1.f, 1.f);

	// Clear color buffer and Depth Buffer
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

/**
 * Draw
 * The render function gets called once per loop.
 * Typically this includes 'glDraw' related calls, and the relevant setup of buffers
 * for those calls.
 *
 * @return void
 */
void Draw()
{
	// Render the breakout game every frame
	Breakout.Render();
}

/**
 * Function called in the Main application loop to handle user input
 *
 * @return void
 */
void Input()
{
	// Event handler that handles various events in SDL
	// that are related to input and output
	SDL_Event e;
	// Handle events on queue
	while (SDL_PollEvent(&e) != 0)
	{
		// If users posts an event to quit
		// An example is hitting the "x" in the corner of the window or pressing the Escape key
		if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
		{
			gQuit = true;
		}
		// If it was a non-ESC keypress, note that that key is being pressed
		else if (e.type == SDL_KEYDOWN)
		{
			// If an event happens greater than the valid keypress values, ignore it
			if (e.key.keysym.sym < 322)
			{
				Breakout.Keys[e.key.keysym.sym] = true;
			}
		}
		// When a key is released, remove note of it from keys being pressed array
		else if (e.type == SDL_KEYUP)
		{
			// If an event happens greater than the valid keypress values, ignore it
			if (e.key.keysym.sym < 322)
			{
				Breakout.Keys[e.key.keysym.sym] = false;
				Breakout.KeysProcessed[e.key.keysym.sym] = false;
			}
		}
	}
}

/**
 * Main Application Loop
 * This is an infinite loop in our graphics application
 *
 * @return void
 */
void MainLoop()
{

	// Determine the initial next time to advance a frame
	// Used to make the framerate fixed intervals
	next_time = SDL_GetTicks() + TICK_INTERVAL;

	// While application is running
	while (!gQuit)
	{
		// Handle Input
		Input();

		// Setup anything (i.e. OpenGL State) that needs to take
		// place before draw calls
		PreDraw();

		// Draw Calls in OpenGL
		// When we 'draw' in OpenGL, this activates the graphics pipeline.
		// i.e. when we use glDrawElements or glDrawArrays,
		//      The pipeline that is utilized is whatever 'glUseProgram' is
		//      currently binded.
		Draw();

		// Update screen of our specified window
		SDL_GL_SwapWindow(gGraphicsApplicationWindow);

		// Delay for the amount of time left until a frame should be advanced
		// Used to make the framerate fixed intervals
		SDL_Delay(time_left());

		// Set the next time a frame should be advanced to be the tick interval (15 ticks)
		next_time += TICK_INTERVAL;
	}
}

/**
 * The last function called in the program
 * This functions responsibility is to destroy any global
 * objects in which we have create dmemory.
 *
 * @return void
 */
void CleanUp()
{
	// Destroy our SDL2 Window
	ResourceManager::Clear();
	SDL_DestroyWindow(gGraphicsApplicationWindow);
	gGraphicsApplicationWindow = nullptr;

	// Quit SDL subsystems
	SDL_Quit();
}

/**
 * The entry point into our C++ programs.
 *
 * @return program status
 */
int main(int argc, char *args[])
{

	// 1. Setup the graphics program
	InitializeProgram();

	// 2. Call the main application loop
	MainLoop();

	// 3. Call the cleanup function when our program terminates
	CleanUp();

	return 0;
}
