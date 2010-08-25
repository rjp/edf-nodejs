#include <stdio.h>
#include <string.h>

#include "GameFactory.h"

#include "Demon.h"

Game *FindGame(char *szContentType)
{
	STACKTRACE
	Game *pReturn = NULL;

	debug("FindGame %s\n", szContentType);

	if(szContentType == NULL)
	{
		return NULL;
	}

	if(strcmp(szContentType, CT_DEMON) == 0)
	{
		debug("FindGame create Demon\n");
		pReturn = new Demon();
	}

	return pReturn;
}
