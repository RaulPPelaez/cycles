#include "api.h"
#include "utils.h"
#include <iostream>
#include <random>
#include <spdlog/spdlog.h>
#include <string>

using namespace cycles;

class BotClient {
  Connection connection;
  std::string name;
  GameState state;
  Player my_player;
  std::mt19937 rng;
  int previousDirection = -1;
  int inertia = 30;

  bool is_valid_move(Direction direction) {
    // Check that the move does not overlap with any grid cell that is set to
    // not 0
    auto new_pos = my_player.position + getDirectionVector(direction);
    if (!state.isInsideGrid(new_pos)) {
      return false;
    }
    if (state.getGridCell(new_pos) != 0) {
      return false;
    }
    return true;
  }

  Direction decideMove() {
    constexpr int max_attempts = 200;
    int attempts = 0;
    const auto position = my_player.position;
    const int frameNumber = state.frameNumber;
    float inertialDamping = 1.0;
    auto dist = std::uniform_int_distribution<int>(
        0, 3 + static_cast<int>(inertia * inertialDamping));
    Direction direction;
    do {
      if (attempts >= max_attempts) {
        spdlog::error("{}: Failed to find a valid move after {} attempts", name,
                      max_attempts);
        exit(1);
      }
      // Simple random movement
      int proposal = dist(rng);
      if (proposal > 3) {
        proposal = previousDirection;
        inertialDamping =
            0; // Remove inertia if the previous direction is not valid
      }
      direction = getDirectionFromValue(proposal);
      attempts++;
    } while (!is_valid_move(direction));
    spdlog::debug("{}: Valid move found after {} attempts, moving from ({}, "
                  "{}) to ({}, {}) in frame {}",
                  name, position.x, position.y, attempts,
                  position.x + getDirectionVector(direction).x,
                  position.y + getDirectionVector(direction).y, frameNumber);
    return direction;
  }

  void receiveGameState() {
    state = connection.receiveGameState();
    for (const auto &player : state.players) {
      if (player.name == name) {
        my_player = player;
        break;
      }
    }
  }

  void sendMove() {
    spdlog::debug("{}: Sending move", name);
    auto move = decideMove();
    previousDirection = getDirectionValue(move);
    connection.sendMove(move);
  }

public:
  BotClient(const std::string &botName) : name(botName) {
    std::random_device rd;
    rng.seed(rd());
    std::uniform_int_distribution<int> dist(0, 50);
    inertia = dist(rng);
    connection.connect(name);
    if (!connection.isActive()) {
      spdlog::critical("{}: Connection failed", name);
      exit(1);
    }
  }

  void run() {
    while (connection.isActive()) {
      receiveGameState();
      sendMove();
    }
  }

};

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <bot_name>" << std::endl;
    return 1;
  }
#if SPDLOG_ACTIVE_LEVEL == SPDLOG_LEVEL_TRACE
  spdlog::set_level(spdlog::level::debug);
#endif
  std::string botName = argv[1];
  BotClient bot(botName);
  bot.run();
  return 0;
}
