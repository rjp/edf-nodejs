#include <stdio.h>

#include "ua.h"

#include "qUAck.h"

#include "CmdIO.h"

#include "Game.h"

Game::Game()
{
	m_iState = NOT_STARTED;
	m_bIsCreator = false;

	m_iServiceID = 0;
}

Game::~Game()
{
	STACKTRACE
	EDF *pRequest = NULL;

	debug("Game::~Game entry\n");

	pRequest = new EDF();
	pRequest->AddChild("serviceid", m_iServiceID);

	CmdRequest(MSG_SERVICE_UNSUBSCRIBE, pRequest);

	debug("Game::~Game exit\n");
}

bool Game::IsRunning()
{
	return m_iState == STARTED;
}

bool Game::IsEnded()
{
	return m_iState == ENDED;
}

bool Game::IsCreator()
{
	return m_bIsCreator;
}

int Game::ServiceID()
{
	return m_iServiceID;
}

EDF *Game::CreateOptions()
{
	STACKTRACE
	EDF *pReturn = new EDF();

	return pReturn;
}

EDF *Game::JoinOptions()
{
	STACKTRACE
	EDF *pReturn = new EDF();

	return pReturn;
}

EDF *Game::StartOptions()
{
	STACKTRACE
	EDF *pReturn = new EDF();

	return pReturn;
}

bool Game::Create(int iServiceID, EDF *pOptions)
{
	STACKTRACE

	Subscribe(iServiceID);

	return Request("game_create", pOptions);
}

bool Game::Join(int iServiceID, EDF *pOptions)
{
	STACKTRACE

	Subscribe(iServiceID);

	return Request("game_join", pOptions);
}

bool Game::Start(EDF *pOptions)
{
	STACKTRACE

	return Request("game_start", pOptions);
}

bool Game::End()
{
	STACKTRACE

	return Request("game_end", NULL);
}

bool Game::Action(const char *szAction, EDF *pAction)
{
	bool bReturn = true;

	if(stricmp(szAction, "game_create") == 0)
	{
		CmdWrite("Game created\n");

		m_bIsCreator = true;
	}
	else if(stricmp(szAction, "game_start") == 0)
	{
		CmdWrite("Game started\n");

		m_iState = STARTED;
	}
	else if(stricmp(szAction, "game_join") == 0)
	{
		CmdWrite("Game joined\n");
	}
	else if(stricmp(szAction, "game_end") == 0)
	{
		CmdWrite("Game ended\n");

		m_iState = ENDED;
	}

	return bReturn;
}

bool Game::Subscribe(int iServiceID)
{
	STACKTRACE
	EDF *pRequest = NULL;

	debug("Game::Subscribe entry %d\n", iServiceID);

	pRequest = new EDF();
	pRequest->AddChild("serviceid", iServiceID);
	pRequest->AddChild("active", true);

	if(CmdRequest(MSG_SERVICE_SUBSCRIBE, pRequest) == false)
	{
		debug("Game::Subscribe exit false\n");
		return false;
	}

	m_iServiceID = iServiceID;

	debug("Game::Subscribe exit true\n");
	return true;
}

bool Game::Request(const char *szAction, EDF *pEDF)
{
	STACKTRACE
	bool bReturn = false;
	EDF *pRequest = NULL;

	debug("Game::Request entry %s\n", szAction);
	if(pEDF != NULL)
	{
		debugEDFPrint(pEDF, false, false);
	}

	pRequest = new EDF();
	pRequest->AddChild("serviceid", m_iServiceID);
	pRequest->Add("serviceaction", szAction);
	if(pEDF != NULL && pEDF->Children() > 0)
	{
		pRequest->Copy(pEDF, false);
	}

	bReturn = CmdRequest(MSG_USER_CONTACT, pRequest);

	debug("Game::Request exit %s\n", BoolStr(bReturn));
	return bReturn;
}
