#include <iostream>

#include <stdio.h>

#include "logging.h"
#include "net.h"
#include "game.h"

#include "console.h"

Console *Console::myInstance = NULL;

Console *Console::getConsole()
{
	if (myInstance == NULL)
		myInstance = new Console();
	return myInstance;
}

void Console::mainLoop()
{
	Logger::getLogger()->info("Console ready");
	while (true) {
		// this is going to need a lot of work
		char key;
		int num;
		num = scanf("%c", &key);
		if (num != 1)
			break;
		if (key == 'q')
			break;
		if (key == 'h') {
			printf("q to quit\nh for help\nt to end turn\nn to stop network\nN to start network\nl to type file to load\n");
		}
		if (key == 't') {
			Logger::getLogger()->info("End Of Turn started");

		}
		if (key == 'n') {
			Network::getNetwork()->stop();
		}
		if (key == 'N') {
			Network::getNetwork()->start();
		}
		if (key == 'l') {
			char *file = new char[100];
			num = scanf("%s", file);
			if (num == 1) {
				Game::getGame()->loadGame(file);
			}
		}
	}
	Logger::getLogger()->info("Server starting shutdown");
}

void Console::close()
{
	Logger::getLogger()->info("Console closed");
}

Console::Console()
{
	Logger::getLogger()->info("Console opened");
}

Console::~Console()
{

}
