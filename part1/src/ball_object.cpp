/******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#include "ball_object.hpp"

// construct a new ball
BallObject::BallObject(glm::vec2 pos, float radius, glm::vec2 velocity, Texture2D sprite)
    : GameObject(pos, glm::vec2(radius * 2.0f, radius * 2.0f), sprite, glm::vec3(146 / 255.0f, 68 / 255.0f, 58 / 255.0f), velocity), Radius(radius), Stuck(true) {}

// move the ball each frame
glm::vec2 BallObject::Move(float dt, unsigned int window_width)
{
    // if not stuck to player board
    if (!this->Stuck)
    {
        // move the ball
        this->Position += this->Velocity * dt;
        
        // then check if outside window bounds and if so, reverse velocity and restore at correct position
        // check on left side
        if (this->Position.x <= 63.0f)
        {
            this->Velocity.x = -this->Velocity.x;
            this->Position.x = 63.0f;
        }
        // check on right side
        else if (this->Position.x + this->Size.x >= window_width - 63.0f)
        {
            this->Velocity.x = -this->Velocity.x;
            this->Position.x = window_width - this->Size.x - 63.0f;
        }
        // check at top of screen
        if (this->Position.y <= 0.0f)
        {
            this->Velocity.y = -this->Velocity.y;
            this->Position.y = 0.0f;
        }
    }

    // return the new ball position after moving
    return this->Position;
}

// resets the ball to initial Stuck Position (if ball is outside window bounds)
void BallObject::Reset(glm::vec2 position, glm::vec2 velocity)
{
    this->Position = position;
    this->Velocity = velocity;
    this->Stuck = true;
}