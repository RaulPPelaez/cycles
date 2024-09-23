#pragma once
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(cycles_resources);
namespace cycles_resources{
  auto getResourceFile(std::string path){
    auto fs = cmrc::cycles_resources::get_filesystem();
    if(!fs.exists(path)){
      throw std::runtime_error("Failed to load " + path + " from resources");
    }
    return fs.open(path);
  }
}
