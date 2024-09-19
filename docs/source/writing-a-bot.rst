.. _writing_a_bot:
Writing a bot
-------------

To write a bot, you need to use the :cpp:class:`cycles::Connection` class to communicate with the server.
The bot code should continuously read the game state from the server and respond with the desired action by calling :cpp:func:`cycles::Connection::receiveGameState` and :cpp:func:`cycles::Connection::sendMove` respectively.

An example of a bot that moves in a random direction is provided in the `src/client/client_randomio.cpp` file.

.. doxygenclass:: cycles::Connection
   :members:

The receive method will return an instance of :cpp:class:`cycles::GameState` that contains the current game state.

.. doxygenstruct:: cycles::GameState
   :members:

.. doxygenstruct:: cycles::Player
   :members:

.. doxygentypedef:: cycles::Id      


Example
*******

A simple bot that always moves north is shown below:

.. code-block:: cpp

		#include "api.h"
		#include "utils.h"
		#include <string>
		#include <iostream>

		using namespace cycles;

		class BotClient {
		  Connection connection;
		  std::string name;
		  GameState state;

		  void sendMove() {
		    connection.sendMove(Direction::north);
		  }

		  void receiveGameState() {
		    state = connection.receiveGameState();
		    std::cout<<"There are "<<state.players.size()<<" players"<<std::endl;
		  }
		  
		public:
		  BotClient(const std::string &botName) : name(botName) {
		    connection.connect(name);
		    if (!connection.isActive()) {
		      exit(1);
		    }
		  }

		  void run() {
		    while (connection.isActive()) {
		      receiveGameState();
		      sendMove();
		    }
		  }

		};

		int main() {
		  BotClient bot("northton");
		  bot.run();
		return 0;
		}

A more sophisticated example can be found in the `src/client/client_randomio.cpp` file.


Other utilities
---------------


.. doxygenfile:: utils.h
