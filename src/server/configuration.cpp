#include"server.h"
#include <filesystem>
#include <set>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
namespace cycles_server{
  Configuration::Configuration(std::string configPath) {
    // Check if the file exists
    if (!std::filesystem::exists(configPath)) {
      spdlog::error("Configuration file does not exist, proceeding with "
                    "default configuration.");
      return;
    }
    // Reads from a yaml file
    YAML::Node config = YAML::LoadFile(configPath);
    if (config["maxClients"]) {
      maxClients = config["maxClients"].as<int>();
    }
    if (config["gridWidth"]) {
      gridWidth = config["gridWidth"].as<int>();
    }
    if (config["gridHeight"]) {
      gridHeight = config["gridHeight"].as<int>();
    }
    if (config["gameWidth"]) {
      gameWidth = config["gameWidth"].as<int>();
    }
    if (config["gameHeight"]) {
      gameHeight = config["gameHeight"].as<int>();
    }
    if (config["gameBannerHeight"]) {
      gameBannerHeight = config["gameBannerHeight"].as<int>();
    }
    std::set<std::string> knownParameters = {"maxClients", "gridWidth",
                                             "gridHeight", "gameWidth",
                                             "gameHeight", "gameBannerHeight"};
    // Warn if there are unknown parameters
    for (const auto &it : config) {
      if (knownParameters.find(it.first.as<std::string>()) ==
          knownParameters.end()) {
        spdlog::warn("Unknown parameter in configuration file: {}",
                     it.first.as<std::string>());
      }
    }
    cellSize = gameWidth / float(gridWidth);
  }


}
