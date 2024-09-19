#pragma once
#include "utils.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
namespace cycles {

using Id = sf::Uint8; ///< The type of the player's unique identifier

constexpr auto SERVER_IP = "127.0.0.1";

/**
 * @brief A representation of a player
 */
struct Player {
  std::string name; ///< The name of the player
  sf::Color color;  ///< The color of the player
  sf::Vector2i position; ///< The position of the player's head in the grid (in cells)
  Id id; ///< The unique identifier of the player
};

// Forward declaration for friend declaration in GameState
class Connection;

/**
 * @brief A representation of the state of the game
 */
struct GameState {
  /**
   * @brief The grid of the game
   *
   * Each cell is represented by the unique identifier of the player that
   * occupies it. The value 0 represents an empty cell. The grid is stored in
   * row-major order and has dimensions gridWidth x gridHeight.
   */
  std::vector<Id> grid;

  int gridWidth;  ///< The width of the grid (in cells)
  int gridHeight; ///< The height of the grid (in cells)

  /**
   * @brief A vector with the players in the game
   */
  std::vector<Player> players;

  int frameNumber; ///< The number of the current frame

  GameState() = default;

  /**
   * @brief Get the value of a cell in the grid
   *
   * @param position The position of the cell
   * @return Id The identifier of the player occupying the cell (0 if empty)
   */
  Id getGridCell(sf::Vector2i position) const {
    return grid[position.y * gridWidth + position.x];
  }

  /**
   * @brief Check if a cell is empty
   *
   * @param position The position of the cell
   * @return true if the cell is empty
   * @return false if the cell is not empty
   */
  bool isCellEmpty(sf::Vector2i position) const {
    return getGridCell(position) == 0;
  }

  /**
   * @brief Check if a position is inside the grid
   *
   * @param position The position to check
   * @return true if the position is inside the grid
   * @return false if the position is outside the grid
   */
  bool isInsideGrid(sf::Vector2i position) const {
    return position.x >= 0 && position.x < gridWidth && position.y >= 0 &&
           position.y < gridHeight;
  }

private:
  friend Connection;
  GameState(sf::Packet &packet);
};

/**
 * @brief A connection to the server. Allows to receive the game state and send
 * the player's moves.
 */
class Connection {
  std::shared_ptr<sf::TcpSocket> socket;
  int frameNumber = 0;
  int lastFrameSent = -1;
  std::string playerName;

public:
  /**
   * @brief Construct a new Connection object
   *
   * @param playerName The name of the player that is trying to connect
   * @return sf::Color The color assigned to the player
   */
  sf::Color connect(std::string playerName);

  /**
   * @brief Send the player's move to the server
   *
   * Can only be called once per frame, after receiving the game state.
   * Will block until the move is sent.
   * Will return without doing nothing if the user is trying to send a move
   * twice in the same frame.
   *
   * @param direction The direction of the move
   */
  void sendMove(Direction direction);

  /**
   * @brief Receive the game state from the server
   *
   * Will block until the game state is received.
   * Can only be called once per frame.
   *
   * @return GameState The game state
   */
  GameState receiveGameState();

  /**
   * @brief Check if the connection is active
   *
   * @return true if the connection is active
   * @return false if the connection is not active
   */
  bool isActive();
};

} // namespace cycles
