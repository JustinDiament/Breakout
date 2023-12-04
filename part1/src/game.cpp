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
#include "ball_object.hpp"
#include "text_renderer.hpp"

enum Direction
{
    UP,
    RIGHT,
    DOWN,
    LEFT
};

// Game-related State data
SpriteRenderer *Renderer;
GameObject *Player;
GameObject *Player2;
BallObject *Ball;
BallObject *Ball2;
TextRenderer *TextLives;
TextRenderer *TextMenu;

float ShakeTime = 0.0f;

bool ballDead = false;
bool ball2Dead = false;

Game::Game(unsigned int width, unsigned int height)
    : State(GAME_MENU), Keys(), Width(width), Height(height), Level(0), Lives(3)
{
}

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

// Initial velocity of the Ball
const glm::vec2 INITIAL_BALL_VELOCITY(0.3f, -0.3f);
// Radius of the ball object
const float BALL_RADIUS = 6;

int score = 0;

void Game::Init()
{
    // load shaders
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
    //  ResourceManager::LoadShader("shaders/particle.vs", "shaders/particle.frag", nullptr, "particle");
    //  ResourceManager::LoadShader("shaders/post_processing.vs", "shaders/post_processing.frag", nullptr, "postprocessing");
    // configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width),
                                      static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("particle").SetMatrix4("projection", projection);
    // ResourceManager::GetShader("particle").Use().SetMatrix4("projection", projection);

    // set render-specific controls
    Shader shader = ResourceManager::GetShader("sprite");
    Renderer = new SpriteRenderer(shader);
    TextLives = new TextRenderer(this->Width, this->Height);
    TextLives->Load("fonts/FFFFORWA.TTF", 45);

    TextMenu = new TextRenderer(this->Width, this->Height);
    TextMenu->Load("fonts/OCRAEXT.TTF", 24);
    // load textures
    ResourceManager::LoadTexture("textures/background.png", false, "background");
    ResourceManager::LoadTexture("textures/ball.png", true, "face");
    ResourceManager::LoadTexture("textures/block.png", false, "block");
    ResourceManager::LoadTexture("textures/block_solid.png", false, "block_solid");
    ResourceManager::LoadTexture("textures/paddle.png", true, "paddle");
    ResourceManager::LoadTexture("textures/particle.png", true, "particle");
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
    this->Level = 0;
    // configure game objects
    glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    glm::vec2 playerPos2 = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y - 100.0f);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"), glm::vec3(146 / 255.0f, 68 / 255.0f, 58 / 255.0f));

    Player2 = new GameObject(playerPos2, PLAYER_SIZE, ResourceManager::GetTexture("paddle"), glm::vec3(146 / 255.0f, 68 / 255.0f, 58 / 255.0f));

    for (int i = 0; i < 322; i++)
    { // init them all to false
        Keys[i] = false;
    }

    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS,
                                              -BALL_RADIUS * 2.0f);
    Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY,
                          ResourceManager::GetTexture("face"));

    glm::vec2 ballPos2 = playerPos2 + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS,
                                                -BALL_RADIUS * 2.0f);
    Ball2 = new BallObject(ballPos2, BALL_RADIUS, INITIAL_BALL_VELOCITY,
                           ResourceManager::GetTexture("face"));
}

void Game::Update(float dt)
{
    // update objects
    Ball->Move(dt, this->Width);
    Ball2->Move(dt, this->Width);
    // check for collisions
    this->DoCollisions();

    // update particles
    // Particles->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2.0f));

    if (Ball->Position.y >= this->Height) // did ball reach bottom edge?
    {
        ballDead = true;
    }

    if (Ball2->Position.y >= this->Height) // did ball reach bottom edge?
    {
        ball2Dead = true;
    }

    if (ballDead && ball2Dead)
    {
        ballDead = false;
        ball2Dead = false;
        --this->Lives;
        // did the player lose all his lives? : Game over
        if (this->Lives == 0)
        {
            this->ResetLevel();
            this->State = GAME_MENU;
        }
        this->ResetPlayer();
    }

    // if (ShakeTime > 0.0f)
    // {
    //     ShakeTime -= dt;
    //     if (ShakeTime <= 0.0f)
    //         Effects->Shake = false;
    // }

    if (this->State == GAME_ACTIVE && this->Levels[this->Level].IsCompleted())
    {
        this->ResetLevel();
        this->ResetPlayer();
        // Effects->Chaos = true;
        this->State = GAME_WIN;
    }
}

void Game::ProcessInput(float dt)
{
    if (this->State == GAME_MENU)
    {
        if (this->Keys[SDLK_RETURN] && !this->KeysProcessed[SDLK_RETURN])
        {
            this->State = GAME_ACTIVE;
            if (this->Level == 0)
            {
                Ball2->Position.x += 10000;
                Ball2->Position.y += 10000;
                Ball2->Velocity = glm::vec2(0, 0);
                Player2->Position.x += 10000;
            }
            this->KeysProcessed[SDLK_RETURN] = true;
        }
        if (this->Keys[SDLK_w] && !this->KeysProcessed[SDLK_w])
        {
            this->Level = (this->Level + 1) % 4;
            this->KeysProcessed[SDLK_w] = true;
        }
        if (this->Keys[SDLK_s] && !this->KeysProcessed[SDLK_s])
        {
            if (this->Level > 0)
                --this->Level;
            else
                this->Level = 3;
            this->KeysProcessed[SDLK_s] = true;
        }
    }

    if (this->State == GAME_ACTIVE)
    {
        float velocity = PLAYER_VELOCITY * dt;
        // move playerboard
        if (this->Keys[SDLK_a])
        {
            if (Player->Position.x >= 64.0f)
            {
                Player->Position.x -= velocity;
                Player2->Position.x -= velocity;

                if (Ball->Stuck)
                    Ball->Position.x -= velocity;

                if (Ball2->Stuck)
                    Ball2->Position.x -= velocity;
            }
        }
        if (this->Keys[SDLK_d])
        {
            if (Player->Position.x <= this->Width - Player->Size.x - 64.0f)
            {
                Player->Position.x += velocity;
                Player2->Position.x += velocity;
                if (Ball->Stuck)
                    Ball->Position.x += velocity;

                if (Ball2->Stuck)
                    Ball2->Position.x += velocity;
            }
        }
        if (this->Keys[SDLK_SPACE])
        {
            Ball->Stuck = false;
            Ball2->Stuck = false;
        }
    }

    if (this->State == GAME_WIN)
    {
        if (this->Keys[SDLK_RETURN])
        {
            this->KeysProcessed[SDLK_RETURN] = true;
            // Effects->Chaos = false;
            this->State = GAME_MENU;
        }
    }
}

void Game::Render()
{
    if (this->State == GAME_ACTIVE || this->State == GAME_MENU)
    {
        // draw background
        Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
        // draw level
        this->Levels[this->Level].Draw(*Renderer);
        // draw player
        Player->Draw(*Renderer);

        if (Level > 0)
        {
            Player2->Draw(*Renderer);
        }

        // draw ball
        Ball->Draw(*Renderer);

        if (Level > 0)
        {
            Ball2->Draw(*Renderer);
        }

        std::stringstream ss;
        ss << this->Lives;
        // lives
        std::string scorestring = std::to_string(score);
        if (score < 10)
        {
            scorestring = "00" + std::to_string(score);
        }
        else if (score >= 10 && score < 100)
        {
            scorestring = "0" + std::to_string(score);
        }

        TextLives->RenderText("" + ss.str(), 600.0f, 5.0f, 1.0f, glm::vec3(142 / 255.0f, 142 / 255.0f, 142 / 255.0f));
        TextLives->RenderText("" + scorestring, 200.0f, 5.0f, 1.0f, glm::vec3(142 / 255.0f, 142 / 255.0f, 142 / 255.0f));
    }
    if (this->State == GAME_MENU)
    {
        TextMenu->RenderText("Press ENTER to start", 360.0f, Height / 2, 1.0f);
        TextMenu->RenderText("Press W or S to select level", 350.0f, Height / 2 + 20.0f, 0.75f);
        TextMenu->RenderText("Once game starts, press SPACE to release ball", 170.0f, Height / 2 + 60.0f, 1.0f);
        TextMenu->RenderText("Use W and S to move paddle left and right", 270.0f, Height / 2 + 80.0f, 0.75f);
    }
    if (this->State == GAME_WIN)
    {
        TextMenu->RenderText(
            "You WON!!!", 400.0, Height / 2 - 20.0, 1.0, glm::vec3(0.0, 1.0, 0.0));
        TextMenu->RenderText(
            "Press ENTER to retry or ESC to quit", 280.0, Height / 2, 1.0, glm::vec3(1.0, 1.0, 0.0));
    }
}

typedef std::tuple<bool, Direction, glm::vec2> Collision;

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
    for (unsigned int i = 0; i < 4; i++)
    {
        float dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max)
        {
            max = dot_product;
            best_match = i;
        }
    }
    return (Direction)best_match;
}

Collision CheckCollision(BallObject &one, GameObject &two) // AABB - Circle collision
{
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

void Game::DoCollisions()
{
    for (GameObject &box : this->Levels[this->Level].Bricks)
    {
        if (!box.Destroyed)
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
                else
                { // if block is solid, enable shake effect
                    ShakeTime = 0.05f;
                }

                // // destroy block if not solid
                // if (!box.IsSolid)
                //     box.Destroyed = true;
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

            Collision collision2 = CheckCollision(*Ball2, box);
            if (std::get<0>(collision2)) // if collision is true
            {
                // destroy block if not solid
                if (!box.IsSolid)
                {
                    score++;
                    box.Destroyed = true;
                }
                else
                { // if block is solid, enable shake effect
                }

                // // destroy block if not solid
                // if (!box.IsSolid)
                //     box.Destroyed = true;
                // collision resolution
                Direction dir = std::get<1>(collision2);
                glm::vec2 diff_vector = std::get<2>(collision2);
                if (dir == LEFT || dir == RIGHT) // horizontal collision
                {
                    Ball2->Velocity.x = -Ball2->Velocity.x; // reverse horizontal velocity
                    // relocate
                    float penetration = Ball2->Radius - std::abs(diff_vector.x);
                    if (dir == LEFT)
                        Ball2->Position.x += penetration; // move ball to right
                    else
                        Ball2->Position.x -= penetration; // move ball to left;
                }
                else // vertical collision
                {
                    Ball2->Velocity.y = -Ball2->Velocity.y; // reverse vertical velocity
                    // relocate
                    float penetration = Ball2->Radius - std::abs(diff_vector.y);
                    if (dir == UP)
                        Ball2->Position.y -= penetration; // move ball back up
                    else
                        Ball2->Position.y += penetration; // move ball back down
                }
            }
        }
    }

    Collision result = CheckCollision(*Ball, *Player);
    if (!Ball->Stuck && std::get<0>(result))
    {
        // check where it hit the board, and change velocity based on where it hit the board
        float centerBoard = Player->Position.x + Player->Size.x / 2.0f;
        float distance = (Ball->Position.x + Ball->Radius) - centerBoard;
        float percentage = distance / (Player->Size.x / 2.0f);
        // then move accordingly
        float strength = 2.0f;
        glm::vec2 oldVelocity = Ball->Velocity;
        Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
        Ball->Velocity.y = -1.0f * abs(Ball->Velocity.y);
        Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity);
    }

    Collision result2 = CheckCollision(*Ball, *Player2);
    if (!Ball->Stuck && std::get<0>(result2))
    {
        // check where it hit the board, and change velocity based on where it hit the board
        float centerBoard = Player2->Position.x + Player2->Size.x / 2.0f;
        float distance = (Ball->Position.x + Ball->Radius) - centerBoard;
        float percentage = distance / (Player2->Size.x / 2.0f);
        // then move accordingly
        float strength = 2.0f;
        glm::vec2 oldVelocity = Ball->Velocity;
        Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
        Ball->Velocity.y = -1.0f * abs(Ball->Velocity.y);
        Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity);
    }

    result = CheckCollision(*Ball2, *Player);
    if (!Ball2->Stuck && std::get<0>(result))
    {
        // check where it hit the board, and change velocity based on where it hit the board
        float centerBoard = Player->Position.x + Player->Size.x / 2.0f;
        float distance = (Ball2->Position.x + Ball2->Radius) - centerBoard;
        float percentage = distance / (Player->Size.x / 2.0f);
        // then move accordingly
        float strength = 2.0f;
        glm::vec2 oldVelocity = Ball2->Velocity;
        Ball2->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
        Ball2->Velocity.y = -1.0f * abs(Ball2->Velocity.y);
        Ball2->Velocity = glm::normalize(Ball2->Velocity) * glm::length(oldVelocity);
    }

    result2 = CheckCollision(*Ball2, *Player2);
    if (!Ball2->Stuck && std::get<0>(result2))
    {
        // check where it hit the board, and change velocity based on where it hit the board
        float centerBoard = Player2->Position.x + Player2->Size.x / 2.0f;
        float distance = (Ball2->Position.x + Ball2->Radius) - centerBoard;
        float percentage = distance / (Player2->Size.x / 2.0f);
        // then move accordingly
        float strength = 2.0f;
        glm::vec2 oldVelocity = Ball2->Velocity;
        Ball2->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
        Ball2->Velocity.y = -1.0f * abs(Ball2->Velocity.y);
        Ball2->Velocity = glm::normalize(Ball2->Velocity) * glm::length(oldVelocity);
    }
}

void Game::ResetLevel()
{
    if (this->Level == 0)
        this->Levels[0].Load("levels/one.lvl", this->Width, this->Height / 3);
    else if (this->Level == 1)
        this->Levels[1].Load("levels/two.lvl", this->Width, this->Height / 3);
    else if (this->Level == 2)
        this->Levels[2].Load("levels/three.lvl", this->Width, this->Height / 3);
    else if (this->Level == 3)
        this->Levels[3].Load("levels/four.lvl", this->Width, this->Height / 3);

    this->Lives = 3;
    score = 0;
}

void Game::ResetPlayer()
{
    // reset player/ball stats
    Player->Size = PLAYER_SIZE;
    Player->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);

    Ball->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -(BALL_RADIUS * 2.0f)), INITIAL_BALL_VELOCITY);

    if (Level > 0)
    {
        Player2->Size = PLAYER_SIZE;
        Player2->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y - 100);
        Ball2->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -(BALL_RADIUS * 2.0f) - 100), INITIAL_BALL_VELOCITY);
    }
}