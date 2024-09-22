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
  std::tuple<int, int, int> hslToRgb(float h, float s, float l) {
    float c = (1 - std::abs(2 * l - 1)) * s;
    float x = c * (1 - std::abs(std::fmod(h / 60.0, 2) - 1));
    float m = l - c / 2;

    float r = 0, g = 0, b = 0;
    if (h >= 0 && h < 60) {
      r = c; g = x; b = 0;
    } else if (h >= 60 && h < 120) {
      r = x; g = c; b = 0;
    } else if (h >= 120 && h < 180) {
      r = 0; g = c; b = x;
    } else if (h >= 180 && h < 240) {
      r = 0; g = x; b = c;
    } else if (h >= 240 && h < 300) {
      r = x; g = 0; b = c;
    } else {
      r = c; g = 0; b = x;
    }

    return std::make_tuple(
			   static_cast<int>((r + m) * 255),
			   static_cast<int>((g + m) * 255),
			   static_cast<int>((b + m) * 255)
			   );
  }

  // Function to generate the color palette
  std::vector<uint32_t> generateColorPalette(int numColors) {
    std::vector<uint32_t> palette;

    float goldenRatioConjugate = 0.618033988749895;
    float hue = 0;

    for (int i = 0; i < numColors; ++i) {
      hue = std::fmod(hue + goldenRatioConjugate, 1.0);
      float saturation = 0.5 + (std::sin(hue * 2 * M_PI) * 0.1);
      float lightness = 0.6 + (std::cos(hue * 2 * M_PI) * 0.1);

      auto [r, g, b] = hslToRgb(hue * 360, saturation, lightness);
      uint32_t color = (r << 24) | (g << 16) | (b << 8) | 0xFF;
      palette.push_back(color);
    }

    return palette;
  }
sf::Color getRandomColor(auto rng) {
  std::uniform_int_distribution<int> dist(0, 125);
  return sf::Color(dist(rng), dist(rng), dist(rng));
}

} // namespace detail

Id Game::addPlayer(const std::string &name) {
  static std::vector<uint32_t> palette = detail::generateColorPalette(300);
  gameStarted = true;
  Player newPlayer;
  newPlayer.name = name;
  newPlayer.color = sf::Color(palette[idCounter]);
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
