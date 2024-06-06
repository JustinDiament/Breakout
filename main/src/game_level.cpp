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
                int colorIndex = 1; // original: black

                // if solid block is a type 1, it is a solid block from the middle of the screen, so color it white
                if (tileData[y][x] == 1)
                    colorIndex = 10;
                // if the solid block is a type 8, it is a screen border block, so color it light grey
                else if (tileData[y][x] == 8)
                    colorIndex = 9;

                // create the solid block and add it to the bricks vector
                glm::vec2 pos(unit_width * x, unit_height * y);
                glm::vec2 size(unit_width, unit_height);
                GameObject obj(pos, size, ResourceManager::GetTexture("block_solid"), colorIndex);
                obj.IsSolid = true;
                this->Bricks.push_back(obj);
            }
            // non-solid block
            else if (tileData[y][x] > 1)
            {
                int colorIndex = 1.0; // original: black

                // blue block
                if (tileData[y][x] == 2)
                    colorIndex = 2;
                // green block
                else if (tileData[y][x] == 3)
                    colorIndex = 3;
                // yellow block
                else if (tileData[y][x] == 4)
                    colorIndex = 4;
                // gold block
                else if (tileData[y][x] == 5)
                    colorIndex = 5;
                // orange block
                else if (tileData[y][x] == 6)
                    colorIndex = 6;
                // red block
                else if (tileData[y][x] == 7)
                    colorIndex = 7;

                // create the non-solid block and add it to the bricks vector
                glm::vec2 pos(unit_width * x, unit_height * y);
                glm::vec2 size(unit_width, unit_height);
                this->Bricks.push_back(GameObject(pos, size, ResourceManager::GetTexture("block"), colorIndex));
            }
        }
    }
}