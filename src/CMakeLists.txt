
find_package(spdlog REQUIRED)
find_package(SFML 2.6 COMPONENTS graphics window system network REQUIRED)

link_libraries(spdlog::spdlog)
link_libraries(sfml-graphics sfml-window sfml-system sfml-network pthread)

add_library(utils OBJECT utils.cpp)
link_libraries(utils)
add_library(api OBJECT api.cpp)
link_libraries(api)

add_executable(server server.cpp)
add_executable(randomio client_randomio.cpp)
