#include "utils.h"
namespace cycles {
std::string socketErrorToString(const sf::Socket::Status status) {
  switch (status) {
  case sf::Socket::Done:
    return "Done";
  case sf::Socket::NotReady:
    return "NotReady";
  case sf::Socket::Partial:
    return "Partial";
  case sf::Socket::Disconnected:
    return "Disconnected";
  case sf::Socket::Error:
    return "Error";
  default:
    return "Unknown";
  }
}

int getDirectionValue(Direction direction) {
  return static_cast<int>(direction);
}

Direction getDirectionFromValue(int value) {
  return static_cast<Direction>(value);
}

sf::Vector2i getDirectionVector(Direction direction) {
  sf::Vector2i vector = sf::Vector2i(0, 0);
  switch (direction) {
  case Direction::north:
    vector = sf::Vector2i(0, -1);
    break;
  case Direction::east:
    vector = sf::Vector2i(1, 0);
    break;
  case Direction::south:
    vector = sf::Vector2i(0, 1);
    break;
  case Direction::west:
    vector = sf::Vector2i(-1, 0);
    break;
  }
  return vector;
}

} // namespace cycles
