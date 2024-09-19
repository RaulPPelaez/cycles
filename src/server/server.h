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

// bool test_grid(std::vector<sf::Uint8> grid, std::map<Id, Player> players) {
//   // Check the validity of the grid by comparing it to the players positions
//   and
//   // tails
//   std::vector<sf::Uint8> true_grid(GRID_HEIGHT * GRID_WIDTH, 0);
//   for (auto &[id, player] : players) {
//     true_grid[player.position.y * GRID_WIDTH + player.position.x] = id;
//     for (auto tail : player.tail) {
//       true_grid[tail.y * GRID_WIDTH + tail.x] = id;
//     }
//   }
//   for (int i = 0; i < GRID_HEIGHT * GRID_WIDTH; i++) {
//     if (grid[i] != true_grid[i]) {
//       return false;
//     }
//   }
//   return true;
// }

struct Configuration {

  int maxClients = 60;
  int gridWidth = 100;
  int gridHeight = 100;
  int gameWidth = 1000;
  int gameHeight = 1000;
  int gameBannerHeight = 100;
  int cellSize = 10;

  Configuration(std::string configPath);
};
} // namespace cycles_server
