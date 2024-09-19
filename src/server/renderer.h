#pragma once
#include"server.h"
#include "game_logic.h"
#include <SFML/Graphics.hpp>

namespace cycles_server{
// Rendering Logic
class GameRenderer {
  sf::RenderWindow window;
  sf::Font font;
  sf::Shader postProcessShader;
  sf::RenderTexture renderTexture;
  const Configuration conf;

public:
  GameRenderer(Configuration conf);

  void render(std::shared_ptr<Game> game);

  bool isOpen() const { return window.isOpen(); }

  void handleEvents();

private:
  void renderPlayers(std::shared_ptr<Game> game);

  void renderGameOver(std::shared_ptr<Game> game);

  void renderBanner(std::shared_ptr<Game> game);
};
}
