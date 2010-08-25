#ifndef _CMDPROCESS_H_
#define _CMDPROCESS_H_

// Message navigation functions
#define MSG_BACK -1
#define MSG_FORWARD -2
#define MSG_PREV -3
#define MSG_NEXT -4
#define MSG_UP -5
#define MSG_DOWN -6
#define MSG_FIRST -7
#define MSG_LAST -8
#define MSG_TOP -9
#define MSG_TOPTHREAD -10
#define MSG_BOTTOM -11
#define MSG_NEW -12
#define MSG_NEWFOLDER -13
#define MSG_PARENT -14
#define MSG_JUMP -15
#define MSG_EXIT -16

bool CmdAnnounceProcess(EDF *pAnnounce);

bool CmdMessageAdd(int iReplyID, int iFolderID, int iReplyFolder = -1, int iFromID = -1, const char *szFromName = NULL, int iMsgType = 0, bool bAttachments = false);
bool CmdMessageGoto(int *iMsgPos, bool bShowOnly = false);
bool CmdUserPage(EDF *pAnnounce = NULL, int iFromID = -1, bool bCustomTo = false, const char *szToName = NULL);
void CmdOptionSet(EDF *pUser, EDF *pRequest, const char *szTitle, const char *szField, bool bPrompt = false, bool bDefault = false);
void CmdValueSet(EDF *pUser, EDF *pRequest, const char *szTitle, const char *szField, int iOptions = 0, int iMin = 0, int iMax = 0, int iDefault = 0);
int CmdFolderJoin(int iFolderID, const char *szPrompt = NULL, bool bView = true);
bool CmdFolderLeave(int iFolderID);
bool CmdMessageMark(int iFolderID, const char *szFolderName, int iMessageID);

int CmdCurrFolder();
void CmdCurrFolder(int iFolderID);
int CmdCurrMessage();
void CmdCurrMessage(int iMessageID);

bool CmdChannelJoin(int iChannelID, const char *szPrompt = NULL);
bool CmdChannelLeave();

int CmdCurrChannel();

#endif
