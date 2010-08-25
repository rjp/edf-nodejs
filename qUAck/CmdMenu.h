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
** CmdMenu.h: Declarations and defines for menu setup functions
*/

#ifndef _CMDMENU_H_
#define _CMDMENU_H_

#include "CmdInput.h"

// Input widths
#define LINE_LEN -1
#define NUMBER_LEN 10

// Announcement types
#define ANN_FOLDERCHECK 1
#define ANN_PAGEBELL 2
#define ANN_USERCHECK 4
#define ANN_ADMINCHECK 8
#define ANN_BUSYCHECK 16
#define ANN_DEBUGCHECK 32
#define ANN_EXTRACHECK 64

// Retro modes
#define RETRO_NAMES 1
#define RETRO_MENUS 2
#define RETRO_PAGENO 4

// Confirmations
#define CONFIRM_BUSY_PAGER 1
#define CONFIRM_ACTIVE_REPLY 2
#define CONFIRM_SPELL_CHECK 4

// Folder navigation
#define NAV_ALPHATREE 0
#define NAV_ALPHALIST 1
#define NAV_LEVEL 2
#define NAV_PRIORITIES 3

// Menu status
#define USER_NAME 100
#define FOLDER_NAME 200

// Menu values
#define MAIN 1000
#define ADMIN 2000
#define FOLDER 3000
#define FOLDER_EDIT FOLDER + 100
#define USER 4000
#define USER_WHO USER + 100
#define USER_EDIT USER + 200
#define TALK 5000
#define MESSAGE 6000
#define GAME 7000

char *CmdFolderTab(EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue);
char *CmdChannelTab(EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue);
char *CmdUserTab(EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue);
char *CmdUserLoginTab(EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue);
char *CmdAgentLoginTab(EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue);
char *CmdServiceTab(EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue);

CmdInput *CmdInputSetup(EDF *pData, int iStatus, unsigned const char *szInit);

CmdInput *CmdMain(int iAccessLevel, int iNumFolders, bool bDevOption);
CmdInput *CmdFolder(int iCurrID, int iAccessLevel, int iNewMsgs, const char *szFolderName, int iSubType, int iCurrMessage, int iFromID, int iToID, int iAccessMode, int iUserID, int iMsgType, int iVoteType, int iVoteID, int iNumNotes, int iNavMode, bool bURLs, bool bRetro, bool bOldMenus, bool bDevOption);
CmdInput *CmdAdmin(int iAccessLevel);
CmdInput *CmdAdminFolder();
CmdInput *CmdUserEdit(EDF *pUser, int iUserID, int iAccessLevel, int iEditID);

// char CmdMessageInput(CmdInput *pInput, bool bMessage, bool bThread, bool bWhole, bool bReplies, bool bCrossFolder, bool bFrom, bool bTo, bool bSubject, bool bKeyword, char cDefault);

int CmdContentList(EDF *pReply, bool bDisplay, int iItemNum = -1, char **szURL = NULL, int *iAttachmentID = NULL, char **szAttachmentName = NULL, bytes **pData = NULL);

bool CmdMessageRequest(int iFolderID, int iMessageID, bool bShowOnly = false, bool bPage = true, EDF **pReply = NULL, bool bRaw = false, bool bArchive = false);

void CmdReplyShow(EDF *pReply);
int CmdAnnounceShow(EDF *pAnnounce, const char *szReturn);
CmdInput *CmdInputSetup(int iStatus);
void FolderMenu(int iFolderID, int iMessageID, int iMsgPos);
void ContentMenu(EDF *pEDF, const char *szType, int iID);
bool MessageMarkMenu(bool bAdd, int iFolderID = -1, int iMessageID = -1, int iFromID = -1, const char *szSubject = NULL, bool bMinCheck = false);
bool UserEditMenu(int iEditID);
bool PageMenu(EDF *pPage, bool bBell);
void BulletinMenu(bool bShowAll);
bool TalkCommandMenu(bool bJoin, bool bPage, bool bSend, bool bActive, bool bWholist, bool bReturn);
void MainMenu();

bool CmdBrowser(char *szBrowser, bool bBrowserWait);
char *CmdBrowser();
bool CmdBrowse(const char *szURI);
// bool CmdBrowserWait();

bool CmdAttachmentDir(const char *szAttachmentDir);
char *CmdAttachmentDir();

int MessagePos(EDF *pMessage);
int MessageCount();

// void CmdServerTime(int iTime);
// int CmdServerTime();

void NewFeatures();
bool ServiceActivate(EDF *pData);

bool CreateUserMenu(bool bLoggedIn, char **szUsername, char **szPassword);//, EDF *pSystemList);

bool TalkMenu(int iChannelID);

char *URLToken(char **szString);

#endif
