#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <spdlog/spdlog.h>
#include "utils.h"
namespace cycles {
using Id = sf::Uint8;
constexpr auto SERVER_IP = "127.0.0.1";

struct Player {
  std::string name;
  sf::Color color;
  sf::Vector2i position;
  Id id;
};

// Forward declaration for friend declaration in GameState
class Connection;

struct GameState {
  std::vector<Id> grid;
  int gridWidth, gridHeight;
  std::vector<Player> players;
  int frameNumber;

  GameState() = default;

  Id getGridCell(int x, int y) const { return grid[y * gridWidth + x]; }

private:
  friend Connection;
  GameState(sf::Packet &packet);
};

class Connection {
  std::shared_ptr<sf::TcpSocket> socket;
  int frameNumber = 0;
  int lastFrameSent = -1;
  std::string playerName;

public:
  sf::Color connect(std::string playerName);

  void sendMove(Direction direction);

  GameState receiveGameState();

  bool isActive();
};

bool isInsideGrid(sf::Vector2i position, int gridWidth, int gridHeight);

bool isCellEmpty(sf::Vector2i position, const GameState &state);

} // namespace cycles
