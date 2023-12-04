/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#include "game_level.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

// loads level from file
void GameLevel::Load(const char *file, unsigned int levelWidth, unsigned int levelHeight)
{
    // clear old data
    this->Bricks.clear();

    // load from file
    unsigned int tileCode;
    GameLevel level;
    std::string line;
    std::ifstream fstream(file);
    std::vector<std::vector<unsigned int>> tileData;
    if (fstream)
    {
        while (std::getline(fstream, line)) // read each line from level file
        {
            std::istringstream sstream(line);
            std::vector<unsigned int> row;
            while (sstream >> tileCode) // read each word separated by spaces
                row.push_back(tileCode);
            tileData.push_back(row);
        }

        // if there is any data in the level file, initialize a game level from it
        if (tileData.size() > 0)
            this->init(tileData, levelWidth, levelHeight);
    }
}

// render level
void GameLevel::Draw(SpriteRenderer &renderer)
{
    // for each tile in the level, unless it is an already destroyed block, draw it
    for (GameObject &tile : this->Bricks)
        if (!tile.Destroyed)
            tile.Draw(renderer);
}

// check if the level is completed (all non-solid tiles are destroyed)
bool GameLevel::IsCompleted()
{
    // look through the bricks to see if any are still not destroyed. if one is, level is not completed yet
    for (GameObject &tile : this->Bricks)
        if (!tile.IsSolid && !tile.Destroyed)
            return false;
    return true;
}

// initialize level from tile data
void GameLevel::init(std::vector<std::vector<unsigned int>> tileData, unsigned int levelWidth, unsigned int levelHeight)
{
    // calculate dimensions
    unsigned int height = tileData.size();
    unsigned int width = tileData[0].size();
    float unit_width = levelWidth / static_cast<float>(width), unit_height = levelHeight / 11;
    
    // initialize level tiles based on tileData
    for (unsigned int y = 0; y < height; ++y)
    {
        for (unsigned int x = 0; x < width; ++x)
        {
            // check block type from level data (2D level array)
            // solid block
            if (tileData[y][x] == 1 || tileData[y][x] > 7) 
            {
                glm::vec3 color = glm::vec3(1.0f); // original: white

                // if solid block is a type 1, it is a solid block from the middle of the screen, so color it
                if (tileData[y][x] == 1)
                    color = glm::vec3(0.8f, 0.8f, 0.7f);
                // if the solid block is a type 8, it is a screen border block, so color it grey
                else if (tileData[y][x] == 8)
                    color = glm::vec3(142 / 255.0f, 142 / 255.0f, 142 / 255.0f);

                // create the solid block and add it to the bricks vector
                glm::vec2 pos(unit_width * x, unit_height * y);
                glm::vec2 size(unit_width, unit_height);
                GameObject obj(pos, size, ResourceManager::GetTexture("block_solid"), color);
                obj.IsSolid = true;
                this->Bricks.push_back(obj);
            }
            // non-solid block
            else if (tileData[y][x] > 1) 
            {
                glm::vec3 color = glm::vec3(1.0f); // original: white

                // blue block
                if (tileData[y][x] == 2)
                    color = glm::vec3(64 / 255.0f, 79 / 255.0f, 205.0 / 255.0f);
                // green block
                else if (tileData[y][x] == 3)
                    color = glm::vec3(65 / 255.0f, 146 / 255.0f, 69 / 255.0f);
                // yellow block
                else if (tileData[y][x] == 4)
                    color = glm::vec3(164 / 255.0f, 154 / 255.0f, 36 / 255.0f);
                // gold block
                else if (tileData[y][x] == 5)
                    color = glm::vec3(187 / 255.0f, 123 / 255.0f, 43 / 255.0f);
                // orange block
                else if (tileData[y][x] == 6)
                    color = glm::vec3(209 / 255.0f, 115 / 255.0f, 56 / 255.0f);
                // red block
                else if (tileData[y][x] == 7)
                    color = glm::vec3(211 / 255.0f, 85 / 255.0f, 70 / 255.0f);

                // create the non-solid block and add it to the bricks vector
                glm::vec2 pos(unit_width * x, unit_height * y);
                glm::vec2 size(unit_width, unit_height);
                this->Bricks.push_back(GameObject(pos, size, ResourceManager::GetTexture("block"), color));
            }
        }
    }
}