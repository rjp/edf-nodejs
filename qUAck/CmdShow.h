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
** CmdShow.h: Declaration of reply display functions
*/

#ifndef _CMDSHOW_H_
#define _CMDSHOW_H_

#define STRTIME_TIME 1
#define STRTIME_TIMEHM 6
#define STRTIME_DATE 2
#define STRTIME_SHORT 3
#define STRTIME_MEDIUM 4
#define STRTIME_LONG 5

#define STRVALUE_TIME 1
#define STRVALUE_HM 2
#define STRVALUE_HMN 5
#define STRVALUE_DAY 3
#define STRVALUE_TIMESU 4
#define STRVALUE_BYTE 10

#define SUBINFO_USER 1
#define SUBINFO_TREE 2
#define SUBINFO_TOTAL 4
#define SUBINFO_ACTIVE 8

#define RETRO_NAME(x) (bRetro == true ? CmdRetroName(x) : x)

void CmdTitle(const char *szString);
void CmdField(const char *szName, char cColour, const char *szValue, int iOptions = CMD_OUT_NOHIGHLIGHT);
void CmdField(const char *szName, char cColour, int iValue);
void CmdField(const char *szName, const char *szValue, int iOptions = CMD_OUT_NOHIGHLIGHT);

char *NumPos(int iValue);
char AccessColour(int iLevel, int iType);
char *GenderObject(int iGender);
char *GenderType(int iGender);
void StrTime(char *szTime, int iType, time_t iTime, char cCol = '\0', const char *szBefore = NULL, const char *szAfter = NULL);
void StrValue(char *szValue, int iType, int iValue, char cCol = '\0');

char *UserEmote(const char *szPrefix1, const char *szPrefix2, const char *szText, bool bDot, char cCol = '\0');

bool CmdRetroNames(EDF *pUser);
char *CmdRetroName(char *szName);
bool CmdRetroMenus(EDF *pUser);

int CmdSubList(EDF *pReply, int iType, int iDisplay, const char *szSpace, bool bRetro);

void CmdSystemView(EDF *pReply);
void CmdLocationList(EDF *pReply, int iListType);
void CmdHelpList(EDF *pReply);
void CmdHelpView(EDF *pReply);
void CmdMessageTreeList(EDF *pReply, const char *szType, int iListType, int iListDetails);
void CmdMessageTreeView(EDF *pReply, const char *szType);
void CmdMessageList(EDF *pReply, int iListType);
void CmdMessageView(EDF *pReply, int iFolderID, char *szFolderName, int iMsgNum, int iNumMsgs, bool bPage = true);
void CmdChannelList(EDF *pReply);
void CmdChannelView(EDF *pReply);
void CmdUserList(EDF *pReply, int iListType, int iOwnerID);
bool CmdUserListMatch(EDF *pReply, int iListType, int iOwnerID);
void CmdUserWho(EDF *pReply, int iListType);
void CmdUserLast(EDF *pReply);
void CmdUserLogoutList(EDF *pReply);
void CmdUserView(EDF *pReply);
void CmdUserFolder(EDF *pReply);
void CmdUserStats(EDF *pReply);
void CmdUserPageView(EDF *pPage, const char *szType, const char *szUser, const char *szDateField = NULL, const char *szTextField = NULL, bool bTopDashes = true);
bool CmdCentre(const char *szText, char cColour = '\0');
void CmdBanner(EDF *pReply);
void CmdTaskList(EDF *pReply);

#endif
