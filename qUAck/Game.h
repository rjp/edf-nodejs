#ifndef _GAME_H_
#define _GAME_H_

#include "EDF/EDF.h"

class Game
{
public:
	Game();
	virtual ~Game();

   virtual char *ContentType() = 0;

	bool IsRunning();
	bool IsEnded();
	bool IsCreator();

	int ServiceID();

	virtual EDF *CreateOptions();
   virtual EDF *JoinOptions();
	virtual EDF *StartOptions();
   virtual EDF *KeyOptions() = 0;

	virtual bool Create(int iServiceID, EDF *pOptions);
	virtual bool Join(int iServiceID, EDF *pOptions);
	virtual bool Start(EDF *pOptions);
	virtual bool End();
	virtual bool Key(char cOption) = 0;

	virtual bool Loop() = 0;

	virtual bool Action(const char *szAction, EDF *pAction);

private:
	enum GameState { NOT_STARTED, STARTED, ENDED };

	GameState m_iState;
   bool m_bIsCreator;

	int m_iServiceID;

	bool Subscribe(int iServiceID);
	bool Request(const char *szAction, EDF *pEDF);
};

#endif
