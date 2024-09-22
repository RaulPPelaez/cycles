#include "renderer.h"
#include <SFML/Graphics.hpp>
#include <filesystem>
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
} // namespace detail

using namespace cycles_server;

void PostProcess::create(sf::Vector2i windowSize) {
  auto this_source =
      std::filesystem::path(__FILE__).parent_path().string() + "/";
  postProcessShader.loadFromFile(this_source + "shaders/postprocess.frag",
                                 sf::Shader::Fragment);
  bloomShader.loadFromFile(this_source + "shaders/bloom.frag",
                           sf::Shader::Fragment);
  renderTexture.create(windowSize.x, windowSize.y);
  renderTexture.setSmooth(true);
  channel1.create(windowSize.x, windowSize.y);
  channel1.setSmooth(true);
  if (!postProcessShader.isAvailable()) {
    spdlog::critical("Shader not available");
    exit(1);
  }
  if (!bloomShader.isAvailable()) {
    spdlog::critical("Bloom shader not available");
    exit(1);
  }
}

void PostProcess::apply(sf::RenderWindow &window, sf::RenderTexture &channel0) {
  auto windowSize = sf::Glsl::Vec2(window.getSize().x, window.getSize().y);
  postProcessShader.setUniform("iResolution", windowSize);
  bloomShader.setUniform("iResolution", windowSize);
  renderTexture.clear(sf::Color::Black);
  channel1.clear(sf::Color::Black);
  sf::RectangleShape bkg(windowSize);
  bkg.setFillColor(sf::Color::Black);
  renderTexture.draw(bkg);
  channel1.draw(bkg);
  channel0.generateMipmap();
  postProcessShader.setUniform("iChannel0", channel0.getTexture());
  sf::Sprite sprite(channel0.getTexture());
  channel1.draw(sprite, &postProcessShader);
  channel1.display();
  channel1.generateMipmap();
  bloomShader.setUniform("iChannel0", channel0.getTexture());
  bloomShader.setUniform("iChannel1", channel1.getTexture());
  sf::Sprite sprite2(renderTexture.getTexture());
  window.draw(sprite2, &bloomShader);
}

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
  renderTexture.create(window.getSize().x, window.getSize().y);
  postProcess.create(sf::Vector2i(window.getSize().x, window.getSize().y));
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
  const int offset_y = conf.gameBannerHeight + 0;
  const int offset_x = 0;
  auto cellSize = conf.cellSize;
  auto windowSize = sf::Glsl::Vec2(window.getSize().x, window.getSize().y);
  renderTexture.clear(sf::Color::Black);
  sf::RectangleShape bkg(windowSize);
  bkg.setFillColor(sf::Color::Black);
  renderTexture.draw(bkg);

  for (const auto &[id, player] : game->getPlayers()) {
    sf::CircleShape playerShape(cellSize);
    // Make the head of the player darker
    auto darkerColor = player.color;
    darkerColor.r = darkerColor.r * 0.8;
    darkerColor.g = darkerColor.g * 0.8;
    darkerColor.b = darkerColor.b * 0.8;
    playerShape.setFillColor(darkerColor);
    playerShape.setPosition(
        (player.position.x) * cellSize - cellSize / 2 + offset_x,
        (player.position.y) * cellSize - cellSize / 2 + offset_y);
    renderTexture.draw(playerShape);
    // Add a border to the head
    sf::CircleShape borderShape(cellSize + 1);
    borderShape.setFillColor(sf::Color::Transparent);
    borderShape.setOutlineThickness(3);
    borderShape.setOutlineColor(player.color);
    borderShape.setPosition(
        (player.position.x) * cellSize - cellSize / 2 - 1 + offset_x,
        (player.position.y) * cellSize - cellSize / 2 - 1 + offset_y);
    renderTexture.draw(borderShape);
    // Draw tail
    for (auto tail : player.tail) {
      sf::RectangleShape tailShape(sf::Vector2f(cellSize, cellSize));
      tailShape.setFillColor(player.color);
      tailShape.setPosition(tail.x * cellSize + offset_x,
                            tail.y * cellSize + offset_y);
      renderTexture.draw(tailShape);
    }
  }
  renderTexture.display();
  postProcess.apply(window, renderTexture);
  for (const auto &[id, player] : game->getPlayers()) {
    sf::Text nameText(player.name, font, 30);
    nameText.setFillColor(sf::Color::White);
    nameText.setOutlineThickness(2);
    nameText.setOutlineColor(sf::Color::Black);
    nameText.setPosition(player.position.x * cellSize - 20 + offset_x,
                         player.position.y * cellSize - 20 + offset_y);
    window.draw(nameText);
  }
}

void GameRenderer::renderGameOver(std::shared_ptr<Game> game) {
  sf::Text gameOverText("Game Over", font, 60);
  gameOverText.setOutlineThickness(3);
  gameOverText.setOutlineColor(sf::Color::White);
  gameOverText.setFillColor(sf::Color::Black);
  gameOverText.setPosition(conf.gameWidth / 2 - 150,
                           conf.gameHeight / 2 - 30);
  if (game->getPlayers().size() > 0) {
    auto winner = game->getPlayers().begin()->second.name;
    sf::Text winnerText("Winner: " + winner, font, 40);
    winnerText.setFillColor(sf::Color::Black);
    winnerText.setOutlineThickness(3);
    winnerText.setOutlineColor(sf::Color::White);
    winnerText.setPosition(conf.gameWidth / 2 - 150,
                           conf.gameHeight / 2 + 30);
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
