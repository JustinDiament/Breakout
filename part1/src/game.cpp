/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/

#include <iostream>
#include <sstream>

#include <SDL2/SDL.h>

#include "game.hpp"
#include "resource_manager.hpp"
#include "sprite_renderer.hpp"
#include "game_object.hpp"
#include "text_renderer.hpp"

// Possible collision directions
enum Direction
{
    UP,
    RIGHT,
    DOWN,
    LEFT
};

// Define a type for collisions
typedef std::tuple<bool, Direction, glm::vec2> Collision;

// Game-related State data
SpriteRenderer *Renderer;
GameObject *Player;
GameObject *Player2;
BallObject *Ball;
BallObject *Ball2;
TextRenderer *TextLives;
TextRenderer *TextMenu;

// Track if each ball has fallen off the bottom of the screen yet
bool ballDead = false;
bool ball2Dead = false;

// Initial velocity of the Ball
const glm::vec2 INITIAL_BALL_VELOCITY(0.25f, -0.25f);

// Radius of the ball object
const float BALL_RADIUS = 6;

// The score of the current game
int score = 0;

// construct a game
Game::Game(unsigned int width, unsigned int height)
    : State(GAME_MENU), Keys(), Width(width), Height(height), Level(0), Lives(3)
{
}

// destruct a game
Game::~Game()
{
    delete Renderer;
    delete Player;
    delete Player2;
    delete Ball;
    delete Ball2;
    delete TextLives;
    delete TextMenu;
}

// initialize game state (load all shaders/textures/levels)
void Game::Init()
{
    // load shaders
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");

    // configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width),
                                      static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("particle").SetMatrix4("projection", projection);

    // set render-specific controls
    Shader shader = ResourceManager::GetShader("sprite");
    Renderer = new SpriteRenderer(shader);

    // set up text rendering for top bar text
    TextLives = new TextRenderer(this->Width, this->Height);
    TextLives->Load("fonts/FFFFORWA.TTF", 45);

    // set up text rendering for menu text
    TextMenu = new TextRenderer(this->Width, this->Height);
    TextMenu->Load("fonts/OCRAEXT.TTF", 24);

    // load textures
    ResourceManager::LoadTexture("textures/background.png", false, "background");
    ResourceManager::LoadTexture("textures/texture_sampler.png", false, "ball");
    ResourceManager::LoadTexture("textures/texture_sampler.png", false, "block");
    ResourceManager::LoadTexture("textures/texture_sampler.png", false, "block_solid");
    ResourceManager::LoadTexture("textures/texture_sampler.png", false, "paddle");

    // load levels
    GameLevel one;
    one.Load("levels/one.lvl", this->Width, this->Height / 3);
    GameLevel two;
    two.Load("levels/two.lvl", this->Width, this->Height / 3);
    GameLevel three;
    three.Load("levels/three.lvl", this->Width, this->Height / 3);
    GameLevel four;
    four.Load("levels/four.lvl", this->Width, this->Height / 3);
    this->Levels.push_back(one);
    this->Levels.push_back(two);
    this->Levels.push_back(three);
    this->Levels.push_back(four);

    // start at the first level in selection
    this->Level = 0;

    // set up player paddle 1
    glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"), 8);

    // set up player paddle 2
    glm::vec2 playerPos2 = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y - 100.0f);
    Player2 = new GameObject(playerPos2, PLAYER_SIZE, ResourceManager::GetTexture("paddle"), 8);

    // initialize all key press values to false/unpressed
    for (int i = 0; i < 322; i++)
    {
        // init them all to false
        Keys[i] = false;
    }

    // set up ball 1
    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS,
                                              -BALL_RADIUS * 2.0f);
    Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY,
                          ResourceManager::GetTexture("ball"));

    // set up ball 2
    glm::vec2 ballPos2 = playerPos2 + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS,
                                                -BALL_RADIUS * 2.0f);
    Ball2 = new BallObject(ballPos2, BALL_RADIUS, INITIAL_BALL_VELOCITY,
                           ResourceManager::GetTexture("ball"));
}

// loop every frame to update the game state
void Game::Update(float dt)
{
    // move the balls each frame
    Ball->Move(dt, this->Width);
    Ball2->Move(dt, this->Width);

    // check for collisions every frame
    this->DoCollisions();

    // did ball reach the bottom edge?
    if (Ball->Position.y >= this->Height)
    {
        ballDead = true;
    }

    // did ball2 reach bottom edge?
    if (Ball2->Position.y >= this->Height)
    {
        ball2Dead = true;
    }

    // if both ball(s) have fallen off the bottom, the player loses, so lose a life
    if (ballDead && ball2Dead)
    {
        // subtract a life
        --this->Lives;

        // did the player lose all their lives? if they did, it is game over, so reset to menu
        if (this->Lives == 0)
        {
            this->ResetLevel();
            this->State = GAME_MENU;
        }

        // reset the player paddles and balls
        this->ResetPlayer();
    }

    // if the player has won the game by clearing all breakable blocks, give them a win screen and reset the game
    if (this->State == GAME_ACTIVE && this->Levels[this->Level].IsCompleted())
    {
        this->ResetLevel();
        this->ResetPlayer();
        this->State = GAME_WIN;
    }
}

// loop every frame to check for player keyboard input
void Game::ProcessInput(float dt)
{
    // if game is on the menu, look for button presses to toggle level or start the game
    if (this->State == GAME_MENU)
    {
        // if player hits the enter key, start the game on the current level
        if (this->Keys[SDLK_RETURN] && !this->KeysProcessed[SDLK_RETURN])
        {
            // start the game
            this->State = GAME_ACTIVE;

            // if this is normal Breakout, disable Ball2 and Player2
            if (this->Level == 0)
            {
                // Move the ball offscreen (below screen so it counts as dead) and don't let it move
                Ball2->Position.x += 10000;
                Ball2->Position.y += 10000;
                Ball2->Velocity = glm::vec2(0, 0);

                // move player off screen so it can't affect game
                Player2->Position.y += 10000;
            }

            // done with enter key press
            this->KeysProcessed[SDLK_RETURN] = true;
        }
        // if the player presses w, scroll forward one in the levels list
        if (this->Keys[SDLK_w] && !this->KeysProcessed[SDLK_w])
        {
            // scroll one
            this->Level = (this->Level + 1) % 4;

            // done with w key press
            this->KeysProcessed[SDLK_w] = true;
        }

        // if the player presses s, scroll back one in the levels list
        if (this->Keys[SDLK_s] && !this->KeysProcessed[SDLK_s])
        {
            // scroll one
            if (this->Level > 0)
                --this->Level;
            else
                this->Level = 3;

            // done with s key press
            this->KeysProcessed[SDLK_s] = true;
        }
    }

    // if the game is in progress, check for left and right move presses with a and d keys
    if (this->State == GAME_ACTIVE)
    {
        // set the velocity value based on framerate
        float velocity = PLAYER_VELOCITY * dt;

        // if the player presses a, move the player paddle left
        if (this->Keys[SDLK_a])
        {
            // as long as the player isn't brushing up against the left wall, move the paddles
            if (Player->Position.x >= 64.0f)
            {
                // move both player paddles
                Player->Position.x -= velocity;
                Player2->Position.x -= velocity;

                // if the game hasn't started yet and the balls are fixed to the paddles, move them too
                if (Ball->Stuck)
                {
                    Ball->Position.x -= velocity;
                    Ball2->Position.x -= velocity;
                }
            }
        }
        // if the player presses d, move the player paddle right
        if (this->Keys[SDLK_d])
        {
            // as long as the player isn't brushing up against the right wall, move the paddles
            if (Player->Position.x <= this->Width - Player->Size.x - 64.0f)
            {
                // move both player paddles
                Player->Position.x += velocity;
                Player2->Position.x += velocity;

                // if the game hasn't started yet and the balls are fixed to the paddles, move them too
                if (Ball->Stuck)
                {
                    Ball->Position.x += velocity;
                    Ball2->Position.x += velocity;
                }
            }
        }

        // if the player presses the space bar, begin the game (if it has already started, this will do nothing)
        if (this->Keys[SDLK_SPACE])
        {
            Ball->Stuck = false;
            Ball2->Stuck = false;
        }
    }

    // if the game had been won, await an enter press to return to the menu
    if (this->State == GAME_WIN)
    {
        if (this->Keys[SDLK_RETURN])
        {
            this->KeysProcessed[SDLK_RETURN] = true;
            this->State = GAME_MENU;
        }
    }
}

// loop every frame to render the game window
void Game::Render()
{
    // if the game is active or at the menu, draw the game
    if (this->State == GAME_ACTIVE || this->State == GAME_MENU)
    {
        // draw background
        Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);

        // draw level
        this->Levels[this->Level].Draw(*Renderer);

        // draw player
        Player->Draw(*Renderer);

        // draw ball
        Ball->Draw(*Renderer);

        // if it is Super Breakout, also draw player2 and ball2
        if (Level > 0)
        {
            Player2->Draw(*Renderer);
            Ball2->Draw(*Renderer);
        }

        // set up a stream for the life count
        std::stringstream ss;
        ss << this->Lives;

        // convert the score remaining to 3 digits
        std::string scorestring = std::to_string(score);
        if (score < 10)
        {
            scorestring = "00" + std::to_string(score);
        }
        else if (score >= 10 && score < 100)
        {
            scorestring = "0" + std::to_string(score);
        }

        // render the text in the top bar for the lives remaining and the score
        TextLives->RenderText("" + ss.str(), 750.0f, 5.0f, 1.0f, glm::vec3(142 / 255.0f, 142 / 255.0f, 142 / 255.0f));
        TextLives->RenderText("" + scorestring, 200.0f, 5.0f, 1.0f, glm::vec3(142 / 255.0f, 142 / 255.0f, 142 / 255.0f));
    }

    // if the game is on the menu, render the menu how to play text
    if (this->State == GAME_MENU)
    {
        TextMenu->RenderText("Press ENTER to start", 360.0f, Height / 2, 1.0f);
        TextMenu->RenderText("Press W or S to select level", 350.0f, Height / 2 + 20.0f, 0.75f);
        TextMenu->RenderText("Once game starts, press SPACE to release ball", 170.0f, Height / 2 + 60.0f, 1.0f);
        TextMenu->RenderText("Use W and S to move paddle left and right", 270.0f, Height / 2 + 80.0f, 0.75f);
    }

    // if the game has been won, render the won text and instructions
    if (this->State == GAME_WIN)
    {
        TextMenu->RenderText(
            "You WON!!!", 400.0, Height / 2 - 20.0, 1.0, glm::vec3(0.0, 1.0, 0.0));
        TextMenu->RenderText(
            "Press ENTER to retry or ESC to quit", 280.0, Height / 2, 1.0, glm::vec3(1.0, 1.0, 0.0));
    }
}

// determine the direction of a collision
Direction VectorDirection(glm::vec2 target)
{
    glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),  // up
        glm::vec2(1.0f, 0.0f),  // right
        glm::vec2(0.0f, -1.0f), // down
        glm::vec2(-1.0f, 0.0f)  // left
    };
    float max = 0.0f;
    unsigned int best_match = -1;

    // find the closest cardinal direction
    for (unsigned int i = 0; i < 4; i++)
    {
        float dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max)
        {
            max = dot_product;
            best_match = i;
        }
    }

    // return the best match
    return (Direction)best_match;
}

// check to see if two objects have collided
Collision CheckCollision(BallObject &one, GameObject &two)
{
    // AABB - Circle collision
    // get center point circle first
    glm::vec2 center(one.Position + one.Radius);

    // calculate AABB info (center, half-extents)
    glm::vec2 aabb_half_extents(two.Size.x / 2.0f, two.Size.y / 2.0f);
    glm::vec2 aabb_center(
        two.Position.x + aabb_half_extents.x,
        two.Position.y + aabb_half_extents.y);

    // get difference vector between both centers
    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);

    // add clamped value to AABB_center and we get the value of box closest to circle
    glm::vec2 closest = aabb_center + clamped;

    // retrieve vector between center circle and closest point AABB and check if length <= radius
    difference = closest - center;
    if (glm::length(difference) <= one.Radius)
        return std::make_tuple(true, VectorDirection(difference), difference);
    else
        return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
}

// check if a box and a ball have collided
void Game::CheckBallBrickCollision(BallObject *Ball, GameObject &box)
{
    Collision collision = CheckCollision(*Ball, box);
    if (std::get<0>(collision)) // if collision is true
    {
        // destroy block if not solid
        if (!box.IsSolid)
        {
            score++;
            box.Destroyed = true;
        }

        // collision resolution
        Direction dir = std::get<1>(collision);
        glm::vec2 diff_vector = std::get<2>(collision);
        if (dir == LEFT || dir == RIGHT) // horizontal collision
        {
            Ball->Velocity.x = -Ball->Velocity.x; // reverse horizontal velocity
            // relocate
            float penetration = Ball->Radius - std::abs(diff_vector.x);
            if (dir == LEFT)
                Ball->Position.x += penetration; // move ball to right
            else
                Ball->Position.x -= penetration; // move ball to left;
        }
        else // vertical collision
        {
            Ball->Velocity.y = -Ball->Velocity.y; // reverse vertical velocity
            // relocate
            float penetration = Ball->Radius - std::abs(diff_vector.y);
            if (dir == UP)
                Ball->Position.y -= penetration; // move ball back up
            else
                Ball->Position.y += penetration; // move ball back down
        }
    }
}

// check if a player paddle and a ball have collided
void Game::CheckBallPlayerCollision(BallObject *Ball, GameObject *Player)
{
    // check for a collision between the given ball and player paddle
    Collision result = CheckCollision(*Ball, *Player);

    // if there is a collision result and the ball is not in the pre-game-start stuck state, handle it
    if (!Ball->Stuck && std::get<0>(result))
    {
        // check where the ball hit the paddle, and change velocity based on where it hit the paddle
        float centerBoard = Player->Position.x + Player->Size.x / 2.0f;
        float distance = (Ball->Position.x + Ball->Radius) - centerBoard;
        float percentage = distance / (Player->Size.x / 2.0f);

        // then move the ball accordingly
        float strength = 2.0f;
        glm::vec2 oldVelocity = Ball->Velocity;
        Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
        Ball->Velocity.y = -1.0f * abs(Ball->Velocity.y);
        Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity);
    }
}

// look for collisions every frame and resolve them
void Game::DoCollisions()
{
    // loop through every brick
    for (GameObject &brick : this->Levels[this->Level].Bricks)
    {
        // if the current brick has not been destroyed yet, check for collisions with it and both ball(s)
        // and handle them, if any
        if (!brick.Destroyed)
        {
            CheckBallBrickCollision(Ball, brick);
            CheckBallBrickCollision(Ball2, brick);
        }
    }

    // check for collisions between both ball(s) and player paddle(s) and handle them, if any
    CheckBallPlayerCollision(Ball, Player);
    CheckBallPlayerCollision(Ball, Player2);
    CheckBallPlayerCollision(Ball2, Player);
    CheckBallPlayerCollision(Ball2, Player2);
}

// reset a level after a game over
void Game::ResetLevel()
{
    // level 1
    if (this->Level == 0)
        this->Levels[0].Load("levels/one.lvl", this->Width, this->Height / 3);
    // level 2
    else if (this->Level == 1)
        this->Levels[1].Load("levels/two.lvl", this->Width, this->Height / 3);
    // level 3
    else if (this->Level == 2)
        this->Levels[2].Load("levels/three.lvl", this->Width, this->Height / 3);
    // level 4
    else if (this->Level == 3)
        this->Levels[3].Load("levels/four.lvl", this->Width, this->Height / 3);

    // reset lives to 3 and score to 0
    this->Lives = 3;
    score = 0;
}

// reset the player and ball after a loss of life or game over
void Game::ResetPlayer()
{
    // reset player
    Player->Size = PLAYER_SIZE;
    Player->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);

    // reset ball
    ballDead = false;
    Ball->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -(BALL_RADIUS * 2.0f)), INITIAL_BALL_VELOCITY);

    // if it is Super Breakout, also reset player 2 and ball 2
    if (Level > 0)
    {
        Player2->Size = PLAYER_SIZE;
        Player2->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y - 100);

        Ball2->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -(BALL_RADIUS * 2.0f) - 100), INITIAL_BALL_VELOCITY);
        ball2Dead = false;
    }
}