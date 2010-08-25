#include <stdio.h>

#include "Demon.h"

Demon::Demon() : Game()
{
}

char *Demon::ContentType()
{
	return CT_DEMON;
}

EDF *Demon::StartOptions()
{
	STACKTRACE
	EDF *pReturn = new EDF();

	return pReturn;
}

EDF *Demon::KeyOptions()
{
	STACKTRACE
	EDF *pReturn = new EDF();

	return pReturn;
}

bool Demon::Start(EDF *pOptions)
{
	return true;
}

bool Demon::Key(char cKey)
{
	STACKTRACE

	return false;
}

bool Demon::Loop()
{
	STACKTRACE

	return false;
}

bool Demon::Action(const char *szAction, EDF *pAction)
{
	return Game::Action(szAction, pAction);
}
