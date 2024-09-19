#include "renderer.h"
#include <SFML/Graphics.hpp>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

namespace detail {
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
} // namespace detail

using namespace cycles_server;
// Rendering Logic
GameRenderer::GameRenderer(Configuration conf)
    : conf(conf), window(sf::VideoMode(conf.gameWidth,
                                       conf.gameHeight + conf.gameBannerHeight),
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

void GameRenderer::render(std::shared_ptr<Game> game) {
  window.clear(sf::Color::White);
  // // Draw grid
  // sf::RectangleShape cell(sf::Vector2f(conf.cellSize - 1, conf.cellSize -
  // 1)); cell.setFillColor(sf::Color::Black); for (int y = 0; y <
  // conf.gridHeight; ++y) {
  //   for (int x = 0; x < conf.gridWidth; ++x) {
  // 	cell.setPosition(x * conf.cellSize, y * conf.cellSize);
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

void GameRenderer::handleEvents() {
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

void GameRenderer::renderPlayers(std::shared_ptr<Game> game) {
  const int offset = conf.gameBannerHeight;
  postProcessShader.setUniform("blur_radius", 10.0f);
  renderTexture.clear(sf::Color::White);
  for (const auto &[id, player] : game->getPlayers()) {
    sf::CircleShape playerShape(conf.cellSize);
    // Make the head of the player darker
    auto darkerColor = player.color;
    darkerColor.r = darkerColor.r * 0.8;
    darkerColor.g = darkerColor.g * 0.8;
    darkerColor.b = darkerColor.b * 0.8;
    playerShape.setFillColor(darkerColor);
    playerShape.setPosition(
        (player.position.x) * conf.cellSize - conf.cellSize / 2,
        (player.position.y) * conf.cellSize - conf.cellSize / 2 + offset);
    renderTexture.draw(playerShape);
    // Add a border to the head
    sf::CircleShape borderShape(conf.cellSize + 1);
    borderShape.setFillColor(sf::Color::Transparent);
    borderShape.setOutlineThickness(2);
    borderShape.setOutlineColor(player.color);
    borderShape.setPosition(
        (player.position.x) * conf.cellSize - conf.cellSize / 2 - 1,
        (player.position.y) * conf.cellSize - conf.cellSize / 2 - 1 + offset);
    renderTexture.draw(borderShape);
    // Draw tail
    for (auto tail : player.tail) {
      sf::RectangleShape tailShape(sf::Vector2f(conf.cellSize, conf.cellSize));
      tailShape.setFillColor(player.color);
      tailShape.setPosition(tail.x * conf.cellSize,
                            tail.y * conf.cellSize + offset);
      renderTexture.draw(tailShape);
      // Add a border to the tail
      sf::RectangleShape borderShape(
          sf::Vector2f(conf.cellSize + 2, conf.cellSize + 2));
      borderShape.setFillColor(sf::Color::Transparent);
      borderShape.setOutlineThickness(2);
      borderShape.setOutlineColor(player.color);
      borderShape.setPosition(tail.x * conf.cellSize - 1,
                              tail.y * conf.cellSize - 1 + offset);
      renderTexture.draw(borderShape);
    }
  }
  renderTexture.display();
  sf::Sprite sprite(renderTexture.getTexture());
  postProcessShader.setUniform("texture", sf::Shader::CurrentTexture);
  window.draw(sprite, &postProcessShader);

  for (const auto &[id, player] : game->getPlayers()) {
    sf::Text nameText(player.name, font, 22);
    nameText.setFillColor(sf::Color::Black);
    nameText.setPosition(player.position.x * conf.cellSize,
                         player.position.y * conf.cellSize - 20 + offset);
    window.draw(nameText);
  }
}

void GameRenderer::renderGameOver(std::shared_ptr<Game> game) {
  sf::Text gameOverText("Game Over", font, 60);
  gameOverText.setFillColor(sf::Color::Black);
  gameOverText.setPosition(conf.gameWidth / 2 - 150,
                           conf.gameHeight / 2 - 30 + conf.gameBannerHeight);
  if (game->getPlayers().size() > 0) {
    auto winner = game->getPlayers().begin()->second.name;
    sf::Text winnerText("Winner: " + winner, font, 40);
    winnerText.setFillColor(sf::Color::Black);
    winnerText.setPosition(conf.gameWidth / 2 - 150,
                           conf.gameHeight / 2 + 30 + conf.gameBannerHeight);
    window.draw(winnerText);
  }
  window.draw(gameOverText);
}

void GameRenderer::renderBanner(std::shared_ptr<Game> game) {
  // Draw a banner at the top
  sf::RectangleShape banner(
      sf::Vector2f(conf.gameWidth, conf.gameBannerHeight - 20));
  banner.setFillColor(sf::Color::Black);
  banner.setPosition(0, 0);
  window.draw(banner);
  // Draw the frame number
  sf::Text frameText("Frame: " + std::to_string(game->getFrame()), font, 22);
  frameText.setPosition(10, 10);
  frameText.setFillColor(sf::Color::White);
  window.draw(frameText);
  // Draw the number of players
  sf::Text playersText("Players: " + std::to_string(game->getPlayers().size()),
                       font, 22);
  playersText.setPosition(10, 40);
  playersText.setFillColor(sf::Color::White);
  window.draw(playersText);
}
