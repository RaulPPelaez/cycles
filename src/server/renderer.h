#pragma once
#include"server.h"
#include "game_logic.h"
#include <SFML/Graphics.hpp>

namespace cycles_server{
// Rendering Logic
class PostProcess{
  sf::Shader postProcessShader;
  sf::Shader bloomShader;
  sf::RenderTexture renderTexture;
  sf::RenderTexture channel1;
public:
  PostProcess(){}
  void create(sf::Vector2i windowSize);
  void apply(sf::RenderWindow &window, sf::RenderTexture &target);
};

class GameRenderer {
  sf::RenderWindow window;
  sf::Font font;
  sf::RenderTexture renderTexture;
  const Configuration conf;
  PostProcess postProcess;

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
