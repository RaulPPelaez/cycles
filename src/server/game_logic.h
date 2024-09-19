#pragma once
#include "server.h"
#include <map>
#include <mutex>
#include <random>
#include <set>
#include <vector>

namespace cycles_server {

// Game Logic
class Game {
  const Configuration conf;
  uint max_tail_length = 55;
  Id idCounter = 1;
  int frame = 0;
  bool gameStarted = false;
  std::map<Id, Player> players;
  std::vector<sf::Uint8> grid;
  std::mt19937 rng;
  std::mutex gameMutex;

public:
  Game(Configuration conf)
      : conf(conf), grid(conf.gridWidth * conf.gridHeight, 0),
        rng(std::random_device()()) {}

  Id addPlayer(const std::string &name);

  void removePlayer(Id id);

  void movePlayers(std::map<Id, Direction> directions);

  const auto &getGrid() { return grid; }

  auto getPlayers() {
    std::scoped_lock lock(gameMutex);
    return players;
  }

  void setFrame(int frame) { this->frame = frame; }

  int getFrame() { return frame; }

  bool isGameOver() { return gameStarted && players.size() <= 1; }

private:

  Id &getCell(int x, int y) { return grid[y * conf.gridWidth + x]; }

  bool legalMove(sf::Vector2i newPos);

  std::set<Id> checkCollisions(std::map<Id, sf::Vector2i> newPositions);

};

} // namespace cycles_server
