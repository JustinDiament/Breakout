/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>

#include "game_level.hpp"
#include "ball_object.hpp"

// Represents the current state of the game
enum GameState
{
    GAME_ACTIVE,
    GAME_MENU,
    GAME_WIN
};

// Initial size of the player paddle
const glm::vec2 PLAYER_SIZE(80.0f, 15.0f);
// Initial velocity of the player paddle
const float PLAYER_VELOCITY(0.5f);

// Game holds all game-related state and functionality.
// Combines all game-related data into a single class for
// easy access to each of the components and manageability.
class Game
{
public:
    // game state
    GameState State;

    // track keypresses
    bool Keys[322];
    bool KeysProcessed[322];

    // width and height of window
    unsigned int Width, Height;

    // all game levels
    std::vector<GameLevel> Levels;

    // current game level
    unsigned int Level;

    // lives remaining
    unsigned int Lives;

    // constructor/destructor
    Game(unsigned int width, unsigned int height);
    ~Game();

    // initialize game state (load all shaders/textures/levels)
    void Init();

    // game loop
    void ProcessInput(float dt);
    void Update(float dt);
    void Render();
    void DoCollisions();
    void ResetLevel();
    void ResetPlayer();

private:
    // check if a box and a ball have collided
    void CheckBallBrickCollision(BallObject *Ball, GameObject &box);

    // check if a player and a ball have collided
    void CheckBallPlayerCollision(BallObject *Ball, GameObject *Player);
};

#endif