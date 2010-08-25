/*
** UNaXcess Conferencing System
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
**
** Concepts based on Bradford UNaXcess (c) 1984-87 Brandon S Allbery
** Extensions (c) 1989, 1990 Andrew G Minter
** Manchester UNaXcess extensions by Rob Partington, Gryn Davies,
** Michael Wood, Andrew Armitage, Francis Cook, Brian Widdas
**
** The look and feel was reproduced. No code taken from the original
** UA was someone else's inspiration. Copyright and 'nuff respect due
**
** qUAck.cpp: Header for qUAck client
*/

#ifndef _QUACK_H_
#define _QUACK_H_

#include "EDF/EDF.h"

#include "Conn/EDFConn.h"

#include "CmdInput.h"
#include "CmdMenu.h"

#define CLIENT_BASE "qUAck"
#define CLIENT_VERSION "v0.96c"

// Slight fudge for retro fit
#define VOTE_FLOATVALUES 1024 // Added in v2.7-beta8
#define VOTE_FLOATPERCENT 2048 // Added in v2.7-beta8

#define IS_INT_VOTE(x) \
(mask(x, VOTE_INTVALUES) == true || mask(x, VOTE_PERCENT) == true)

#define IS_FLOAT_VOTE(x) \
(mask(x, VOTE_FLOATVALUES) == true || mask(x, VOTE_FLOATPERCENT) == true)

#define IS_VALUE_VOTE(x) \
(IS_INT_VOTE(x) == true || IS_FLOAT_VOTE(x) == true || mask(x, VOTE_STRVALUES) == true)

// Show function flags
#define CMD_REDRAW 1
#define CMD_RESET 2

extern EDF *m_pUser, *m_pPaging;
extern EDF *m_pFolderList, *m_pFolderNav, *m_pMessageList, *m_pMessageView;
extern EDF *m_pChannelList, *m_pUserList;
extern EDF *m_pServiceList;
extern EDF *m_pSystemList;

extern EDFConn *m_pClient;

int BuildNum();
char *BuildTime();
char *BuildDate();

// int CmdServerVersion(const char *szVersion);

char *CLIENT_NAME();
char *CLIENT_SUFFIX();
char *CLIENT_PLATFORM();

void CmdStartup();
void CmdShutdown(const char *szError = NULL, bool bDeleteLog = false, bool bSupressError = false);
bool CmdRefreshFolders(bool bIdleReset = true);
bool CmdRefreshMessages(int iFolderID, bool bIdleReset = true);
bool CmdRefreshUsers(bool bIdleReset = true);
bool CmdRefreshServices();
void CmdRefresh(bool bUserReset);

bool CmdRequest(const char *szRequest, EDF **pReply = NULL, bool bSupressError = false);
bool CmdRequest(const char *szRequest, EDF *pRequest, EDF **pReply = NULL, bool bSupressError = false);
bool CmdRequest(const char *szRequest, EDF *pRequest, bool bDelete, EDF **pReply = NULL, bool bSupressError = false);

byte CmdMenu(CmdInput *pInput);
byte CmdMenu(int iMenuStatus);

bool CmdYesNo(const char *szTitle, bool bDefault);

char *CmdLineStr(const char *szTitle, int iMax = -1, int iOptions = 0, const char *szInit = NULL, CMDTABFUNC pTabFunc = NULL, EDF *pTabData = NULL);
char *CmdLineStr(const char *szTitle, const char *szExtra, int iMax = -1, int iOptions = 0, const char *szInit = NULL, CMDTABFUNC pTabFunc = NULL, EDF *pTabData = NULL);
char *CmdLineStr(int iMenuStatus);
int CmdLineTab(const char *szTitle, CMDTABFUNC pTabFunc, EDF *pData);
int CmdLineUser(CMDTABFUNC pTabFunc = CmdUserTab, int iInitID = -1, char **szReturn = NULL, bool bValid = true, const char *szDefaultPrompt = NULL, const char *szInit = NULL);
int CmdLineChannel();
int CmdLineFolder(int iInitID = -1, char **szReturn = NULL, bool bValid = true);
int CmdLineFolder(const char *szTitle, int iInitID = -1, char **szReturn = NULL, bool bValid = true);
int CmdLineNum(const char *szTitle, int iOptions = 0, int iDefault = 0, int iInit = 0);
int CmdLineNum(const char *szTitle, const char *szExtra, int iOptions = 0, int iDefault = 0, int iInit = 0);
int CmdLineNum(int iMenuStatus);

char *CmdText(int iOptions = 0, const char *szInit = NULL, CMDFIELDSFUNC pFieldsFunc = NULL, EDF *pData = NULL);

bool CmdUserReset();

void CmdRun(const char *szProgram, bool bWait, const char *szArgs);

int CmdPID();

bool CmdOpen(const char *szFilename);

char CmdDirSep();

int CmdVersion(char *szVersion);

char *CmdEditor();

#endif
