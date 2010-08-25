#ifndef _DEMON_H_
#define _DEMON_H_

#include "Game.h"

#define CT_DEMON "application/x-game-demon"

class Demon : public Game
{
public:
	Demon();

   char *ContentType();

	EDF *StartOptions();
   EDF *KeyOptions();

	bool Start(EDF *pOptions);
	bool Key(char cKey);

	bool Loop();

	bool Action(const char *szAction, EDF *pAction);
};

#endif
