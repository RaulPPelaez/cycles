#pragma once
#include"server.h"
#include "game_logic.h"
#include <SFML/Graphics.hpp>
#include <functional>


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
  std::unique_ptr<PostProcess> postProcess;

public:
  GameRenderer(Configuration conf);

  void render(std::shared_ptr<Game> game);

  bool isOpen() const { return window.isOpen(); }

  void handleEvents(std::vector<std::function<void(sf::Event &)>> extraEventHandlers = {});

  void renderSplashScreen(std::shared_ptr<Game> game);

private:
  void renderPlayers(std::shared_ptr<Game> game);

  void renderGameOver(std::shared_ptr<Game> game);

  void renderBanner(std::shared_ptr<Game> game);
};
}
