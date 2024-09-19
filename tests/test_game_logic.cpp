//GTest tests for game logic
#include"server/game_logic.h"
#include"gtest/gtest.h"
#include<fstream>
using cycles::Id;
using namespace cycles_server;
// Game Logic
// class Game {
//   const Configuration conf;
//   uint max_tail_length = 55;
//   Id idCounter = 1;
//   int frame = 0;
//   bool gameStarted = false;
//   std::map<Id, Player> players;
//   std::vector<sf::Uint8> grid;
//   std::mt19937 rng;
//   std::mutex gameMutex;

// public:
//   Game(Configuration conf)
//       : conf(conf), grid(conf.gridWidth * conf.gridHeight, 0),
//         rng(std::random_device()()) {}

//   Id addPlayer(const std::string &name);

//   void removePlayer(Id id);

//   void movePlayers(std::map<Id, Direction> directions);

//   const auto &getGrid() { return grid; }

//   auto getPlayers() {
//     std::scoped_lock lock(gameMutex);
//     return players;
//   }

//   void setFrame(int frame) { this->frame = frame; }

//   int getFrame() { return frame; }

//   bool isGameOver() { return gameStarted && players.size() = 1; }

// private:

//   Id &getCell(int x, int y) { return grid[y * conf.gridWidth + x]; }

//   bool legalMove(sf::Vector2i newPos);

//   std::set<Id> checkCollisions(std::map<Id, sf::Vector2i> newPositions);

// };

std::string writeConfig(){
  std::string conf_yaml = R"(
gameHeight: 1000
gameWidth: 1000
gameBannerHeight: 100
gridHeight: 100
gridWidth: 100
maxClients: 60
)";
  auto temp_file = std::tmpnam(nullptr);
  std::ofstream out(temp_file);
  out<<conf_yaml;
  return temp_file;
}

bool test_grid(std::vector<sf::Uint8> grid, std::map<Id, Player> players, Configuration conf) {
  int GRID_HEIGHT = conf.gridHeight;
  int GRID_WIDTH = conf.gridWidth;
  std::vector<sf::Uint8> true_grid(GRID_HEIGHT * GRID_WIDTH, 0);
  for (auto &[id, player] : players) {
    true_grid[player.position.y * GRID_WIDTH + player.position.x] = id;
    for (auto tail : player.tail) {
      true_grid[tail.y * GRID_WIDTH + tail.x] = id;
    }
  }
  for (int i = 0; i < GRID_HEIGHT * GRID_WIDTH; i++) {
    if (grid[i] != true_grid[i]) {
      return false;
    }
  }
  return true;
}

TEST(GameLogicTest, AddPlayer) {
  // Write some yaml conf to a temp file
  std::string conf_file = writeConfig();
  Configuration conf(conf_file);
  Game game(conf);
  Id id = game.addPlayer("player1");
  auto players = game.getPlayers();
  EXPECT_EQ(players.size(), 1);
  EXPECT_EQ(players[id].name, "player1");
}

TEST(GameLogicTest, RemovePlayer) {
  // Write some yaml conf to a temp file
  std::string conf_file = writeConfig();
  Configuration conf(conf_file);
  Game game(conf);
  Id id = game.addPlayer("player1");
  Id id2 = game.addPlayer("player2");
  game.removePlayer(id);
  auto players = game.getPlayers();
  EXPECT_EQ(players.size(), 1);
  EXPECT_EQ(players[id2].name, "player2");
  game.removePlayer(id2);
  players = game.getPlayers();
  EXPECT_EQ(players.size(), 0);
}

TEST(GameLogicTest, MovePlayers) {
  // Write some yaml conf to a temp file
  std::string conf_file = writeConfig();
  Configuration conf(conf_file);
  Game game(conf);
  Id id = game.addPlayer("player1");
  Id id2 = game.addPlayer("player2");
  std::map<Id, Direction> directions;
  directions[id] = Direction::north;
  directions[id2] = Direction::south;
  auto players_before = game.getPlayers();
  game.movePlayers(directions);
  auto players = game.getPlayers();
  EXPECT_EQ(players[id].position, players_before[id].position + sf::Vector2i(0, -1));
  EXPECT_EQ(players[id2].position, players_before[id2].position + sf::Vector2i(0, 1));
}

TEST(GameLogicTest, GameOver){
  // Write some yaml conf to a temp file
  std::string conf_file = writeConfig();
  Configuration conf(conf_file);
  Game game(conf);
  EXPECT_FALSE(game.isGameOver());
  Id id = game.addPlayer("player1");
  Id id2 = game.addPlayer("player2");
  game.removePlayer(id);
  game.removePlayer(id2);
  EXPECT_TRUE(game.isGameOver());
}

TEST(GameLogicTest, Grid){
  // Write some yaml conf to a temp file
  std::string conf_file = writeConfig();
  Configuration conf(conf_file);
  Game game(conf);
  Id id = game.addPlayer("player1");
  Id id2 = game.addPlayer("player2");
  std::map<Id, Direction> directions;
  directions[id] = Direction::north;
  directions[id2] = Direction::south;
  game.movePlayers(directions);
  auto grid = game.getGrid();
  auto players = game.getPlayers();
  EXPECT_TRUE(test_grid(grid, players, conf));
}
