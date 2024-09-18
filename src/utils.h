#pragma once
#include<string>
#include<SFML/Network.hpp>

namespace cycles{
  std::string socketErrorToString(const sf::Socket::Status status);

  enum class Direction { north = 0, east, south, west };

  int getDirectionValue(Direction direction);

  Direction getDirectionFromValue(int value);

  sf::Vector2i getDirectionVector(Direction direction);

}
