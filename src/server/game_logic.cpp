#include "game_logic.h"
#include <map>
#include <random>
#include <set>
#include <spdlog/spdlog.h>

namespace cycles_server {

namespace detail {

template <typename T>
std::map<Id, T> removeNonExistentPlayers(std::map<Id, T> directions,
                                         const std::map<Id, Player> &players) {
  for (auto [id, direction] : directions) {
    if (players.find(id) == players.end()) {
      directions.erase(id);
    }
  }
  return directions;
}

sf::Color getRandomColor(auto rng) {
  std::uniform_int_distribution<int> dist(0, 200);
  return sf::Color(dist(rng), dist(rng), dist(rng));
}

} // namespace detail

Id Game::addPlayer(const std::string &name) {
  gameStarted = true;
  Player newPlayer;
  newPlayer.name = name;
  newPlayer.color = detail::getRandomColor(rng);
  newPlayer.id = idCounter;
  std::uniform_real_distribution<float> dist(0, 1.0);
  do {
    newPlayer.position.x = conf.gridWidth * dist(rng);
    newPlayer.position.y = conf.gridHeight * dist(rng);
  } while (getCell(newPlayer.position.x, newPlayer.position.y));
  getCell(newPlayer.position.x, newPlayer.position.y) = newPlayer.id;
  players[idCounter] = newPlayer;
  idCounter++;
  return idCounter - 1;
}

void Game::removePlayer(Id id) {
  auto player_it = players.find(id);
  if (player_it == players.end()) {
    return;
  }
  auto &player = player_it->second;
  getCell(player.position.x, player.position.y) = 0;
  for (auto tail : player.tail) {
    getCell(tail.x, tail.y) = 0;
  }
  players.erase(id);
}

void Game::movePlayers(std::map<Id, Direction> directions) {
  if (directions.size() == 0) {
    return;
  }
  max_tail_length = 55 + frame / 100;
  // Sanitize directions
  directions = detail::removeNonExistentPlayers(directions, players);
  std::map<Id, sf::Vector2i> newPositions;
  // Transform directions to positions
  for (const auto &[id, direction] : directions) {
    auto it = players.find(id);
    if (it == players.end()) {
      continue;
    }
    const auto &player = it->second;
    const sf::Vector2i newPos = player.position + getDirectionVector(direction);
    spdlog::debug(
        "Game: Player {} trying to move to ({},{}) from ({},{}) in frame {}",
        player.name, newPos.x, newPos.y, player.position.x, player.position.y,
        frame);
    newPositions[id] = newPos;
  }
  // Check for collisions
  auto colliding = checkCollisions(newPositions);
  for (auto id : colliding) {
    removePlayer(id);
    newPositions.erase(id);
  }
  // Move remaining players
  for (const auto &[id, newPos] : newPositions) {
    auto it = players.find(id);
    if (it == players.end()) {
      continue;
    }
    auto &player = it->second;
    getCell(newPos.x, newPos.y) = player.id;
    if (player.tail.size() > max_tail_length) {
      getCell(player.tail.back().x, player.tail.back().y) = 0;
      player.tail.pop_back();
    }
    player.tail.push_front(player.position);
    player.position = newPos;
  }
}

bool Game::legalMove(sf::Vector2i newPos) {
  if (newPos.x < 0 || newPos.x >= conf.gridWidth || newPos.y < 0 ||
      newPos.y >= conf.gridHeight) {
    spdlog::debug("Game: Moved out of bounds");
    return false;
  }
  if (getCell(newPos.x, newPos.y) != 0) {
    spdlog::debug("Game: Moved where player {} is",
                  int(getCell(newPos.x, newPos.y)));
    return false;
  }
  return true;
}

std::set<Id> Game::checkCollisions(std::map<Id, sf::Vector2i> newPositions) {
  std::set<Id> colliding;
  // If two players are trying to go to the same position, remove both
  for (const auto &[id1, pos1] : newPositions) {
    for (const auto &[id2, pos2] : newPositions) {
      if (id1 < id2 && pos1 == pos2) {
        spdlog::debug("Game: Players {} and {} collided", id1, id2);
        colliding.insert(id1);
        colliding.insert(id2);
      }
    }
  }
  // If a player is trying to go to a position where another player is, remove
  // the player
  for (const auto &[id, newPos] : newPositions) {
    auto player = players[id];
    spdlog::debug(
        "Game: Player {} trying to move to ({},{}) from ({},{}) in frame {}",
        player.name, newPos.x, newPos.y, player.position.x, player.position.y,
        frame);
    if (!legalMove(newPos)) {
      spdlog::debug("Game: Player {} tried to move to an illegal position",
                    player.name);
      colliding.insert(id);
    }
  }
  return colliding;
}

} // namespace cycles_server
