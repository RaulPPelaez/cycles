#include "api.h"

namespace cycles {

GameState::GameState(sf::Packet &packet) {
  packet >> gridWidth >> gridHeight;
  sf::Uint32 playerCount;
  packet >> playerCount;
  players.resize(playerCount);
  for (sf::Uint32 i = 0; i < playerCount; ++i) {
    int x, y;
    sf::Uint8 r, g, b;
    Id playerId;
    std::string playerName;
    packet >> x >> y >> r >> g >> b >> playerName >> playerId >> frameNumber;
    players[i] = {playerName, sf::Color(r, g, b), sf::Vector2i(x, y), playerId};
  }
  grid.resize(gridWidth * gridHeight);
  for (auto &cell : grid) {
    packet >> cell;
  }
  //Check that the whole packet was read
  if (!packet.endOfPacket()) {
    spdlog::critical("There is still data left in the packet");
    exit(1);
  }
}

namespace detail {
std::shared_ptr<sf::TcpSocket> establishLink() {
  spdlog::debug("Trying to connect");
  auto socket = std::make_shared<sf::TcpSocket>();
  const char *port = std::getenv("CYCLES_PORT");
  if (port == nullptr) {
    spdlog::critical("Environment variable CYCLES_PORT not set");
    exit(1);
  }
  const unsigned short SERVER_PORT = std::stoi(port);
  spdlog::info("Connecting to server at {}:{}", SERVER_IP, SERVER_PORT);
  if (socket->connect(SERVER_IP, SERVER_PORT) != sf::Socket::Done) {
    spdlog::critical("Failed to connect to server");
    exit(1);
  }
  return socket;
}

void sendPacket(std::shared_ptr<sf::TcpSocket> socket, sf::Packet &packet,
                bool blocking = true) {
  int attempts = 0;
  bool blockingState = socket->isBlocking();
  socket->setBlocking(blocking);
  auto status = sf::Socket::NotReady;
  while (status != sf::Socket::Done && attempts < 10) {
    status = socket->send(packet);
    if (status == sf::Socket::NotReady) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    attempts++;
  }
  if (status != sf::Socket::Done) {
    spdlog::critical("Failed to send packet to server");
    spdlog::critical("Reason: {}", socketErrorToString(status));
    exit(1);
  }
  socket->setBlocking(blockingState);
}

sf::Packet receivePacket(std::shared_ptr<sf::TcpSocket> socket,
                         bool blocking = true) {
  sf::Packet packet;
  bool blockingState = socket->isBlocking();
  socket->setBlocking(blocking);
  sf::Socket::Status status = sf::Socket::NotReady;
  int attempts = 0;
  while (status != sf::Socket::Done && attempts < 10) {
    status = socket->receive(packet);
    if (status == sf::Socket::NotReady) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    attempts++;
  }
  if (status != sf::Socket::Done) {
    spdlog::critical("Failed to receive packet from server");
    spdlog::critical("Reason: {}", socketErrorToString(status));
    exit(1);
  }
  socket->setBlocking(blockingState);
  return packet;
}

std::shared_ptr<sf::TcpSocket> connectToServer(std::string playerName) {
  auto socket = detail::establishLink();
  // Send name to server
  sf::Packet namePacket;
  namePacket << playerName;
  detail::sendPacket(socket, namePacket);
  return socket;
}

}; // namespace detail

sf::Color Connection::connect(std::string playerName) {
  this->playerName = playerName;
  if (socket != nullptr) {
    spdlog::critical("Connection already established");
  }
  socket = detail::connectToServer(playerName);
  sf::Color color;
  sf::Packet colorPacket = detail::receivePacket(socket);
  sf::Uint8 r, g, b;
  if (!(colorPacket >> r >> g >> b)) {
    spdlog::critical("Failed to receive color from server");
    exit(1);
  }
  color = sf::Color(r, g, b);
  spdlog::info("{}: Assigned color: R={} G={} B={}", playerName,
               static_cast<int>(r), static_cast<int>(g), static_cast<int>(b));
  return color;
}

void Connection::sendMove(Direction direction) {
  if (frameNumber == lastFrameSent) {
    spdlog::warn("Trying to send move twice in the same frame, call "
                 "receiveGameState first");
    return;
  }
  spdlog::debug("Sending move");
  sf::Packet packet;
  packet << getDirectionValue(direction);
  detail::sendPacket(socket, packet);
  lastFrameSent = frameNumber;
}

GameState Connection::receiveGameState() {
  spdlog::debug("Receiving game state");
  auto packet = detail::receivePacket(socket);
  GameState state(packet);
  frameNumber = state.frameNumber;
  return state;
}

bool Connection::isActive() {
  return socket->getRemoteAddress() != sf::IpAddress::None;
}

bool isInsideGrid(sf::Vector2i position, int gridWidth, int gridHeight) {
  return position.x >= 0 && position.x < gridWidth && position.y >= 0 &&
         position.y < gridHeight;
}

bool isCellEmpty(sf::Vector2i position, const GameState &state) {
  return state.getGridCell(position.x, position.y) == 0;
}

} // namespace cycles
