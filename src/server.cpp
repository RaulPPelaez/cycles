#include "api.h"
#include "utils.h"
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

constexpr int MAX_CLIENTS = 60;
constexpr int GRID_WIDTH = 100;
constexpr int GRID_HEIGHT = 100;
constexpr int GAME_WIDTH = 1000;
constexpr int GAME_HEIGHT = 1000;
constexpr int GAME_BANNER_HEIGHT = 100;

constexpr int CELL_SIZE = GAME_WIDTH / GRID_WIDTH;

using cycles::Direction;
using cycles::Id;

struct Player {
  sf::Vector2i position;
  std::list<sf::Vector2i> tail;
  sf::Color color;
  std::string name;
  Id id;
  Player() : id(std::rand()) {}
};

bool test_grid(std::vector<sf::Uint8> grid, std::map<Id, Player> players) {
  // Check the validity of the grid by comparing it to the players positions and
  // tails
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
// Game Logic
class Game {
  uint max_tail_length = 55;
  Id idCounter = 1;
  std::map<Id, Player> players;
  std::vector<sf::Uint8> grid;
  auto &getCell(int x, int y) { return grid[y * GRID_WIDTH + x]; }
  std::mt19937 rng;
  std::mutex gameMutex;

public:
  Game() : grid(GRID_HEIGHT * GRID_WIDTH, 0), rng(std::random_device()()) {}

  Id addPlayer(const std::string &name) {
    Player newPlayer;
    newPlayer.name = name;
    newPlayer.color = getRandomColor();
    newPlayer.id = idCounter;
    std::uniform_real_distribution<float> dist(0, 1.0);
    do {
      newPlayer.position.x = GRID_WIDTH * dist(rng);
      newPlayer.position.y = GRID_HEIGHT * dist(rng);
    } while (getCell(newPlayer.position.x, newPlayer.position.y));
    getCell(newPlayer.position.x, newPlayer.position.y) = newPlayer.id;
    players[idCounter] = newPlayer;
    idCounter++;
    return idCounter - 1;
  }

  void removePlayer(Id id) {
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

  void movePlayers(std::map<Id, Direction> directions) {
    if (directions.size() == 0) {
      return;
    }
    max_tail_length = 55 + frame / 100;
    // Sanitize directions
    directions = removeNonExistentPlayers(directions);
    std::map<Id, sf::Vector2i> newPositions;
    // Transform directions to positions
    for (const auto &[id, direction] : directions) {
      auto it = players.find(id);
      if (it == players.end()) {
        continue;
      }
      const auto &player = it->second;
      const sf::Vector2i newPos =
          player.position + getDirectionVector(direction);
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

  const auto &getGrid() { return grid; }

  auto getPlayers() {
    std::scoped_lock lock(gameMutex);
    return players;
  }

  void setFrame(int frame) { this->frame = frame; }

  int getFrame() { return frame; }

  bool isGameOver() { return players.size() == 1; }

private:
  int frame = 0;

  template <typename T>
  std::map<Id, T> removeNonExistentPlayers(std::map<Id, T> directions) {
    std::set<Id> toRemove;
    for (auto [id, direction] : directions) {
      if (players.find(id) == players.end()) {
        directions.erase(id);
      }
    }
    return directions;
  }

  std::set<Id> checkCollisions(std::map<Id, sf::Vector2i> newPositions) {
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

  bool legalMove(sf::Vector2i newPos) {
    if (newPos.x < 0 || newPos.x >= GRID_WIDTH || newPos.y < 0 ||
        newPos.y >= GRID_HEIGHT) {
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

  sf::Color getRandomColor() {
    std::uniform_int_distribution<int> dist(0, 200);
    return sf::Color(dist(rng), dist(rng), dist(rng));
  }
};

// Server Logic
class GameServer {
private:
  sf::TcpListener listener;
  std::map<Id, std::shared_ptr<sf::TcpSocket>> clientSockets;
  std::mutex serverMutex;
  std::shared_ptr<Game> game;
  bool running;

public:
  GameServer(std::shared_ptr<Game> game) : game(game), running(false) {
    const char *portenv = std::getenv("CYCLES_PORT");
    if (portenv == nullptr) {
      std::cerr << "Please set the CYCLES_PORT environment variable"
                << std::endl;
      exit(1);
    }
    std::cout << "Port: " << portenv << std::endl;
    const unsigned short PORT = std::stoi(portenv);
    listener.listen(PORT);
    listener.setBlocking(false);
    if (listener.getLocalPort() == 0) {
      spdlog::critical("Failed to bind to port {}", PORT);
      exit(1);
    }
  }

  void run() {
    running = true;
    std::thread acceptThread(&GameServer::acceptClients, this);
    std::thread gameLoopThread(&GameServer::gameLoop, this);
    acceptThread.join();
    gameLoopThread.join();
  }

  void stop() { running = false; }

  int getFrame() { return frame; }

private:
  int frame = 0;
  const int max_client_communication_time = 100; // ms

  bool acceptingClients = true;

  void acceptClients() {
    while (running && acceptingClients && clientSockets.size() < MAX_CLIENTS) {
      auto clientSocket = std::make_shared<sf::TcpSocket>();
      if (listener.accept(*clientSocket) == sf::Socket::Done) {
        clientSocket->setBlocking(
            true); // Set to blocking for initial communication
        // Receive player name
        sf::Packet namePacket;
        if (clientSocket->receive(namePacket) == sf::Socket::Done) {
          std::string playerName;
          namePacket >> playerName;
          auto id = game->addPlayer(playerName);
          // Send color to the client
          sf::Packet colorPacket;
          const auto &player = game->getPlayers().at(id);
          colorPacket << player.color.r << player.color.g << player.color.b;
          if (clientSocket->send(colorPacket) != sf::Socket::Done) {
            spdlog::critical("Failed to send color to client: {}", playerName);
          } else {
            // std::cout << "Color sent to client: " << playerName << std::endl;
            spdlog::info("Color sent to client: {}", playerName);
          }
          clientSocket->setBlocking(
              false); // Set back to non-blocking for game loop
          clientSockets[id] = clientSocket;
          spdlog::info("New client connected: {} with id {}", playerName, id);
        }
      }
    }
  }

  void checkPlayers() {
    // Remove sockets from players that have died or disconnected
    spdlog::debug("Server ({}): Checking players", frame);
    const auto &players = game->getPlayers();
    for (const auto &[id, socket] : clientSockets) {
      bool remove = false;
      if (players.find(id) == players.end()) {
        spdlog::info("Player {} has died", id);
        remove = true;
      }
      if (socket->getRemoteAddress() == sf::IpAddress::None) {
        spdlog::info("Player {} has disconnected", id);
        remove = true;
      }
      if (remove) {
        game->removePlayer(id);
        clientSockets.erase(id);
      }
    }
  }

  auto receiveClientInput(auto clientSockets) {
    spdlog::debug("Server ({}): Receiving client input from {} clients", frame,
                  clientSockets.size());
    if (clientSockets.size() == 0) {
      return std::map<Id, Direction>();
    }
    std::map<Id, Direction> successful;
    for (const auto &[id, clientSocket] : clientSockets) {
      auto name = game->getPlayers().at(id).name;
      spdlog::debug("Server ({}): Receiving input from player {} ({})", frame,
                    id, name);
      sf::Packet packet;
      auto status = clientSocket->receive(packet);
      if (status == sf::Socket::Done) {
        int direction;
        packet >> direction;
        spdlog::debug("Received direction {} from player {} ({})", direction,
                      id, name);
        successful[id] = static_cast<Direction>(direction);
      }
    }
    return successful;
  }

  auto sendGameState(auto clientSockets) {
    spdlog::debug("Server ({}): Sending game state to {} clients", frame,
                  clientSockets.size());
    if (clientSockets.size() == 0) {
      return std::vector<Id>();
    }
    sf::Packet packet;
    packet << GRID_WIDTH << GRID_HEIGHT;
    const auto &grid = game->getGrid();
    const auto &players = game->getPlayers();
    packet << static_cast<sf::Uint32>(players.size());
    for (const auto &[id, player] : players) {
      packet << player.position.x << player.position.y << player.color.r
             << player.color.g << player.color.b << player.name << id << frame;
    }
    for (auto &cell : grid) {
      packet << cell;
    }
    std::vector<Id> successful;
    for (const auto &[id, clientSocket] : clientSockets) {
      if (clientSocket->send(packet) != sf::Socket::Done) {
        spdlog::debug("Server ({}): Failed to send game state to player {}",
                      frame, id);
      } else {
        successful.push_back(id);
        spdlog::debug("Server ({}): Game state sent to player {}", frame, id);
      }
    }
    return successful;
  }

  void gameLoop() {
    sf::sleep(sf::milliseconds(2000)); // Wait for clients to connect
    acceptingClients = false;
    sf::Clock clock;
    sf::Clock clientCommunicationClock;
    while (running && !game->isGameOver()) {
      if (clock.getElapsedTime().asMilliseconds() >= 100) { // ~10 FPS
        clock.restart();
        std::scoped_lock lock(serverMutex);
        game->setFrame(frame);
        checkPlayers();
        auto clientsUnsent = clientSockets;
        decltype(clientSockets) toRecieve;
        std::map<Id, Direction> newDirs;
        std::set<Id> timedOutPlayers;
        clientCommunicationClock.restart();
        while (clientsUnsent.size() > 0 || toRecieve.size() > 0) {
          auto successful = sendGameState(clientsUnsent);
          for (auto s : successful) {
            clientsUnsent.erase(s);
            toRecieve[s] = clientSockets[s];
          }
          auto succesfulrec = receiveClientInput(toRecieve);
          for (auto s : succesfulrec) {
            toRecieve.erase(s.first);
            newDirs[s.first] = s.second;
          }
          spdlog::debug("Server ({}): Clients unsent: {}", frame,
                        clientsUnsent.size());
          spdlog::debug("Server ({}): Clients to recieve: {}", frame,
                        toRecieve.size());
          // Increase trials for clients that have not sent input this frame
          if (clientCommunicationClock.getElapsedTime().asMilliseconds() >
              max_client_communication_time) {
            // Mark all remaining clients for removal
            for (auto [id, socket] : clientsUnsent) {
              timedOutPlayers.insert(id);
            }
            for (auto [id, socket] : toRecieve) {
              timedOutPlayers.insert(id);
            }
            break;
          }
        }
        for (auto id : timedOutPlayers) {
          spdlog::info(
              "Server ({}): Client {} has not sent input for a long time",
              frame, id);
          game->removePlayer(id);
          clientSockets.erase(id);
          newDirs.erase(id);
        }
        game->movePlayers(newDirs);
        frame++;
      }
    }
  }
};

namespace detail{
    sf::Font loadFont() {
      sf::Font font;
    const std::vector<std::string> fontPaths = {
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
        "/usr/share/fonts/ttf/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/liberation-sans/LiberationSans-Regular.ttf"};

    for (const auto &path : fontPaths) {
      if (font.loadFromFile(path)) {
        spdlog::info("Font loaded from {}", path);
	return font;
      }
    }

    spdlog::error("Failed to load font");
    throw std::runtime_error("Failed to load font");
  }
  std::string bloomShader = R"(
uniform sampler2D texture;
uniform float blur_radius = 1.0;


void main() {
    vec2 textureCoordinates = gl_TexCoord[0].xy;
    vec4 color = vec4(0.0);
    color += texture2D(texture, textureCoordinates - 10.0 * blur_radius) * 0.0012;
    color += texture2D(texture, textureCoordinates - 9.0 * blur_radius) * 0.0015;
    color += texture2D(texture, textureCoordinates - 8.0 * blur_radius) * 0.0038;
    color += texture2D(texture, textureCoordinates - 7.0 * blur_radius) * 0.0087;
    color += texture2D(texture, textureCoordinates - 6.0 * blur_radius) * 0.0180;
    color += texture2D(texture, textureCoordinates - 5.0 * blur_radius) * 0.0332;
    color += texture2D(texture, textureCoordinates - 4.0 * blur_radius) * 0.0547;
    color += texture2D(texture, textureCoordinates - 3.0 * blur_radius) * 0.0807;
    color += texture2D(texture, textureCoordinates - 2.0 * blur_radius) * 0.1065;
    color += texture2D(texture, textureCoordinates - blur_radius) * 0.1258;
    color += texture2D(texture, textureCoordinates) * 0.1330;
    color += texture2D(texture, textureCoordinates + blur_radius) * 0.1258;
    color += texture2D(texture, textureCoordinates + 2.0 * blur_radius) * 0.1065;
    color += texture2D(texture, textureCoordinates + 3.0 * blur_radius) * 0.0807;
    color += texture2D(texture, textureCoordinates + 4.0 * blur_radius) * 0.0547;
    color += texture2D(texture, textureCoordinates + 5.0 * blur_radius) * 0.0332;
    color += texture2D(texture, textureCoordinates + 6.0 * blur_radius) * 0.0180;
    color += texture2D(texture, textureCoordinates - 7.0 * blur_radius) * 0.0087;
    color += texture2D(texture, textureCoordinates - 8.0 * blur_radius) * 0.0038;
    color += texture2D(texture, textureCoordinates - 9.0 * blur_radius) * 0.0015;
    color += texture2D(texture, textureCoordinates - 10.0 * blur_radius) * 0.0012;
    gl_FragColor = color*0;
}

)";
}

// Rendering Logic
class GameRenderer {
private:
  sf::RenderWindow window;
  sf::Font font;
  sf::Shader postProcessShader;
  sf::RenderTexture renderTexture;

public:
  GameRenderer()
      : window(sf::VideoMode(GAME_WIDTH, GAME_HEIGHT + GAME_BANNER_HEIGHT),
               "Game Server") {
    window.setFramerateLimit(60);
    try {
      font = detail::loadFont();
    } catch (const std::runtime_error &e) {
      spdlog::warn("No font loaded. Text rendering may not work correctly.");
    }
    postProcessShader.loadFromMemory(detail::bloomShader, sf::Shader::Fragment);
    renderTexture.create(window.getSize().x, window.getSize().y);
  }

  void render(std::shared_ptr<Game> game) {
    window.clear(sf::Color::White);
    // // Draw grid
    // sf::RectangleShape cell(sf::Vector2f(CELL_SIZE - 1, CELL_SIZE - 1));
    // cell.setFillColor(sf::Color::Black);
    // for (int y = 0; y < GRID_HEIGHT; ++y) {
    //   for (int x = 0; x < GRID_WIDTH; ++x) {
    // 	cell.setPosition(x * CELL_SIZE, y * CELL_SIZE);
    // 	window.draw(cell);
    //   }
    // }
    renderPlayers(game);
    if (game->isGameOver()) {
      renderGameOver(game);
    }
    renderBanner(game);
    window.display();
  }

  bool isOpen() const { return window.isOpen(); }

  void handleEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
      if (event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::Escape) {
        window.close();
      }
    }
  }

private:

  void renderPlayers(std::shared_ptr<Game> game) {
    const int offset = GAME_BANNER_HEIGHT;

    postProcessShader.setUniform("blur_radius", 10.0f);
    renderTexture.clear(sf::Color::White);
    for (const auto &[id, player] : game->getPlayers()) {
      sf::CircleShape playerShape(CELL_SIZE);
      // Make the head of the player darker
      auto darkerColor = player.color;
      darkerColor.r = darkerColor.r * 0.8;
      darkerColor.g = darkerColor.g * 0.8;
      darkerColor.b = darkerColor.b * 0.8;
      playerShape.setFillColor(darkerColor);
      playerShape.setPosition((player.position.x) * CELL_SIZE - CELL_SIZE / 2,
                              (player.position.y) * CELL_SIZE - CELL_SIZE / 2 +
                                  offset);
      renderTexture.draw(playerShape);
      // Draw tail
      for (auto tail : player.tail) {
        sf::RectangleShape tailShape(sf::Vector2f(CELL_SIZE, CELL_SIZE));
        tailShape.setFillColor(player.color);
        tailShape.setPosition(tail.x * CELL_SIZE, tail.y * CELL_SIZE + offset);
        renderTexture.draw(tailShape);
      }

      sf::Text nameText(player.name, font, 22);
      nameText.setFillColor(sf::Color::Black);
      nameText.setPosition(player.position.x * CELL_SIZE,
                           player.position.y * CELL_SIZE - 20 + offset);
      window.draw(nameText);
    }
    renderTexture.display();
    sf::Sprite sprite(renderTexture.getTexture());
    postProcessShader.setUniform("texture", sf::Shader::CurrentTexture);
    window.draw(sprite, &postProcessShader);

  }

  void renderGameOver(std::shared_ptr<Game> game) {
    sf::Text gameOverText("Game Over", font, 60);
    gameOverText.setFillColor(sf::Color::Black);
    gameOverText.setPosition(GAME_WIDTH / 2 - 150,
                             GAME_HEIGHT / 2 - 30 + GAME_BANNER_HEIGHT);
    if (game->getPlayers().size() > 0) {
      auto winner = game->getPlayers().begin()->second.name;
      sf::Text winnerText("Winner: " + winner, font, 40);
      winnerText.setFillColor(sf::Color::Black);
      winnerText.setPosition(GAME_WIDTH / 2 - 150,
                             GAME_HEIGHT / 2 + 30 + GAME_BANNER_HEIGHT);
      window.draw(winnerText);
    }
    window.draw(gameOverText);
  }

  void renderBanner(std::shared_ptr<Game> game) {
    // Draw a banner at the top
    sf::RectangleShape banner(
        sf::Vector2f(GAME_WIDTH, GAME_BANNER_HEIGHT - 20));
    banner.setFillColor(sf::Color::Black);
    banner.setPosition(0, 0);
    window.draw(banner);
    // Draw the frame number
    sf::Text frameText("Frame: " + std::to_string(game->getFrame()), font, 22);
    frameText.setPosition(10, 10);
    frameText.setFillColor(sf::Color::White);
    window.draw(frameText);
    // Draw the number of players
    sf::Text playersText(
        "Players: " + std::to_string(game->getPlayers().size()), font, 22);
    playersText.setPosition(10, 40);
    playersText.setFillColor(sf::Color::White);
    window.draw(playersText);
  }
};

int main() {
#if SPDLOG_ACTIVE_LEVEL == SPDLOG_LEVEL_TRACE
  spdlog::set_level(spdlog::level::debug);
#endif
  std::srand(static_cast<unsigned int>(std::time(nullptr)));
  auto game = std::make_shared<Game>();
  GameServer server(game);
  GameRenderer renderer;
  std::thread serverThread(&GameServer::run, &server);
  while (renderer.isOpen()) {
    renderer.handleEvents();
    renderer.render(game);
  }
  server.stop();
  serverThread.join();

  return 0;
}
