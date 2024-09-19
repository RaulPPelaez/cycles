#pragma once
#include<SFML/Network.hpp>

namespace cycles{
  /**
   * @brief Converts a sf::Socket::Status to a string
   *
   * @param status The status to convert
   * @return std::string The string representation of the status
   */
  std::string socketErrorToString(const sf::Socket::Status status);

  /**
   * @brief Represents the four cardinal directions, which are the possible moves a player can make.
   */
  enum class Direction { north = 0, east, south, west };

  /**
   * @brief Transform a direction to an integer
   */
  int getDirectionValue(Direction direction);

  /**
   * @brief Transform an integer to a direction
   */
  Direction getDirectionFromValue(int value);

  /**
   * @brief Transform a Direction to a unit 2D vector in the corresponding direction.
   */
  sf::Vector2i getDirectionVector(Direction direction);

}
