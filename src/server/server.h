#pragma once
#include "api.h"
#include <SFML/Main.hpp>
#include <list>

namespace cycles_server {
using cycles::Direction;
using cycles::Id;

struct Player {
  sf::Vector2i position;
  std::list<sf::Vector2i> tail;
  sf::Color color;
  std::string name;
  Id id;
  Player() : id(std::rand()) {}
};


struct Configuration {

  int maxClients = 60;
  int gridWidth = 100;
  int gridHeight = 100;
  int gameWidth = 1000;
  int gameHeight = 1000;
  int gameBannerHeight = 100;
  float cellSize = 10;

  Configuration(std::string configPath);
};
} // namespace cycles_server
