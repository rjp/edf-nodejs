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
** qUAck.cpp: Main functions for qUAck client
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "Conn/EDFConn.h"

#include "ua.h"

#include "client/CliFolder.h"
#include "client/CliTalk.h"
#include "client/CliUser.h"

#include "CmdIO.h"
#include "CmdInput.h"

#include "CmdMenu.h"
#include "CmdShow.h"
#include "CmdProcess.h"

#include "qUAck.h"
#include "build.h"

#ifndef SIGHUP
#define SIGHUP 1
#endif
#ifndef SIGUNUSED
#define SIGUNUSED 31
#endif

char *m_szHome = NULL;

int iPanic = 0;

char *m_szVersion = NULL;
char m_szClientName[64];

bool m_bRequestWait = false;
int m_iRequest = 0, m_iRefreshMessagesID = 0;

#define CLI_REFRESH_FOLDERS 1
#define CLI_REFRESH_MESSAGES 2
#define CLI_REFRESH_USERS 4
#define CLI_USER_RESET 8

bool ProxyHost(EDF *pEDF);

EDFConn *m_pClient = NULL;

EDF **m_pAnnounces = NULL;
int m_iNumAnnounces = 0;
bool m_bAnnounceFlush = false;

EDF *m_pUser = NULL, *m_pPaging = NULL;
EDF *m_pFolderList = NULL, *m_pFolderNav = NULL, *m_pMessageList = NULL, *m_pMessageView = NULL;
EDF *m_pChannelList = NULL, *m_pUserList = NULL;
EDF *m_pServiceList = NULL;
EDF *m_pSystemList = NULL;

bool m_bThinPipe = false;
bool m_bScreenSize = false;
int m_iLogLevel = 0;

char *m_szEditor = NULL;

char *CLIENT_NAME()
{
   // if(m_szClientName == NULL)
   {
      // m_szClientName = new char[20];
      sprintf(m_szClientName, "%s%s %s%s", CLIENT_BASE, CLIENT_SUFFIX(), CLIENT_VERSION, CLIENT_PLATFORM());
   }

   return m_szClientName;
}

int BuildNum()
{
   return BUILDNUM;
}

char *BuildTime()
{
   return BUILDTIME;
}

char *BuildDate()
{
   return BUILDDATE;
}

/* int ProtocolVersion(const char *szVersion)
{
   return ProcotolCompare(PROTOCOL, szVersion);
} */

// Perform emergency shutdown on program error
void Panic(int iSignal)
{
   CmdShutdown(iSignal);

#ifdef UNIX
   printf("Panic entry %d, %d %d\r\n", iSignal, getpid(), errno);
   debug("Panic entry %d, %d %d\r\n", iSignal, getpid(), errno);
#else
   printf("Panic entry %d, %d %d\r\n", iSignal, errno, GetLastError());
   debug("Panic entry %d, %d %d\r\n", iSignal, errno, GetLastError());
#endif

   if(iPanic != 0)
   {
      printf("Panic multi panic\r\n");
      debug("Panic multi panic\r\n");

      signal(iSignal, SIG_DFL);
      exit(3);
   }

   iPanic++;

   STACKPRINT

   debug("Panic exit\r\n");

   debugclose();

   printf("Panic exit\r\n");

   fflush(stdout);

   exit(2);
}

void CmdStartup()
{
   STACKTRACE

   debug(DEBUGLEVEL_INFO, "CmdStartup entry\n");

   CmdStartup(0);

   m_pUser = new EDF();
   m_pPaging = new EDF();
   m_pFolderList = new EDF();
   m_pFolderNav = new EDF();
   m_pMessageList = new EDF();
   m_pMessageView = new EDF();
   m_pChannelList = new EDF();
   m_pUserList = new EDF();
   m_pServiceList = new EDF();

   debug(DEBUGLEVEL_INFO, "CmdStartup exit\n");
}

void CmdShutdown(const char *szError, bool bDeleteLog, bool bSupressError)
{
   STACKTRACE
   int iAnnounceNum = 0;
   bool bPageFlush = false;
   char *szAnnounce = NULL, *szFrom = NULL;

   debug(DEBUGLEVEL_INFO, "CmdShutdown\n");

	if(CmdType() != 1)
	{
		if(szError != NULL && bSupressError == false)
		{
			CmdWrite("\n\n");
			CmdWrite(szError);
			CmdWrite("\n\n");
		}
	}

   if(m_pClient->State() == Conn::OPEN)
   {
      debug(DEBUGLEVEL_INFO, "CmdShutdown disconnect\n");
      m_pClient->Disconnect();
   }

   debug(DEBUGLEVEL_DEBUG, "CmdShutdown delete conn / client\n");
   delete m_pClient;

   debug(DEBUGLEVEL_DEBUG, "CmdShutdown clear read buffer\n");
   for(iAnnounceNum = 0; iAnnounceNum < m_iNumAnnounces; iAnnounceNum++)
   {
      szAnnounce = NULL;
      m_pAnnounces[iAnnounceNum]->Get(NULL, &szAnnounce);
      if(szAnnounce != NULL && stricmp(szAnnounce, MSG_USER_PAGE) == 0)
      {
         if(bPageFlush == false)
         {
            CmdWrite("\nFlushing page buffer...\n");
         }
         m_pAnnounces[iAnnounceNum]->GetChild("fromname", &szFrom);
         // m_pAnnounces[iAnnounceNum]->GetChild("text", &szText);
         CmdUserPageView(m_pAnnounces[iAnnounceNum], "From", szFrom, NULL, "text", !bPageFlush);
         delete[] szFrom;
         // delete[] szText;

         bPageFlush = true;
      }
      delete[] szAnnounce;

      delete m_pAnnounces[iAnnounceNum];
   }
   delete[] m_pAnnounces;
   m_iNumAnnounces = 0;

   debug(DEBUGLEVEL_DEBUG, "CmdShutdown remove cached EDFs 1\n");
   // delete m_pUser;
   debug(DEBUGLEVEL_DEBUG, "CmdShutdown remove cached EDFs 2\n");
   delete m_pPaging;
   debug(DEBUGLEVEL_DEBUG, "CmdShutdown remove cached EDFs 3\n");
   delete m_pFolderList;
   debug(DEBUGLEVEL_DEBUG, "CmdShutdown remove cached EDFs 4\n");
   delete m_pMessageList;
   debug(DEBUGLEVEL_DEBUG, "CmdShutdown remove cached EDFs 5\n");
   delete m_pMessageView;
   debug(DEBUGLEVEL_DEBUG, "CmdShutdown remove cached EDFs 6\n");
   delete m_pChannelList;
   debug(DEBUGLEVEL_DEBUG, "CmdShutdown remove cached EDFs 7\n");
   delete m_pUserList;
   debug(DEBUGLEVEL_DEBUG, "CmdShutdown remove cached EDFs 8\n");
   delete m_pServiceList;
   debug(DEBUGLEVEL_DEBUG, "CmdShutdown remove cached EDFs 9\n");

   debug(DEBUGLEVEL_DEBUG, "CmdShutdown doing input routine shutdown\n");
   CmdInput::Shutdown();

   CmdShutdown(0);

   if(bDeleteLog == true)
   {
   }

   debug(DEBUGLEVEL_INFO, "CmdShutdown exit\n");
   debugclose();

	if(CmdType() == 1)
	{
		if(szError != NULL && bSupressError == false)
		{
			printf("\n\n");
			printf(szError);
			printf("\n\n");
		}
	}

   exit(0);
}

bool CmdRefreshUsers(bool bIdleReset)
{
   STACKTRACE
   bool bLoop = false;
   char *szStatusMsg = NULL;
   EDF *pRequest = NULL, *pReply = NULL;

   debug(DEBUGLEVEL_INFO, "CmdRefreshUsers entry\n");

   if(m_bRequestWait == true)
   {
      debug(DEBUGLEVEL_DEBUG, "CmdRefreshUsers request waiting");
      if(mask(m_iRequest, CLI_REFRESH_USERS) == false)
      {
         debug(DEBUGLEVEL_DEBUG, ", queueing");
         m_iRequest |= CLI_REFRESH_USERS;
      }
      debug(DEBUGLEVEL_DEBUG, "\n");
      return false;
   }

   pRequest = new EDF();
   if(m_bThinPipe == true)
   {
      pRequest->AddChild("lowest", LEVEL_MESSAGES);
   }
   if(bIdleReset == false)
   {
      pRequest->AddChild("idlereset", false);
   }
   CmdRequest(MSG_USER_LIST, pRequest, &pReply);
   delete m_pUserList;
   m_pUserList = pReply;
   m_pUserList->Sort("user", "name", false);

   m_pUserList->Root();
   bLoop = m_pUserList->Child("user");
   while(bLoop == true)
   {
      /* m_pUserList->Get(NULL, &iUserID);
      m_pUserList->GetChild("name", &szUserName);
      debug("CmdRefreshUsers %d %s\n", iUserID, szUserName);
      delete[] szUserName; */
      if(CmdVersion("2.5") < 0 && m_pUserList->GetChild("busymsg", &szStatusMsg) == true)
      {
         m_pUserList->SetChild("statusmsg", szStatusMsg);
         m_pUserList->DeleteChild("busymsg");
         delete[] szStatusMsg;
      }

      bLoop = m_pUserList->Next("user");
   }
   m_pUserList->Root();

   // debugEDFPrint("CmdRefreshUsers user list", m_pUserList);

   debug(DEBUGLEVEL_INFO, "CmddRefreshUsers exit true\n");
   return true;
}

bool CmdRefreshServices()
{
   double dTick = gettick();

   delete m_pServiceList;
   CmdRequest(MSG_SERVICE_LIST, &m_pServiceList);
   m_pServiceList->Sort("service", "name");
   debug(DEBUGLEVEL_DEBUG, "CmdRefreshServices %ld ms\n", tickdiff(dTick));

   return true;
}

bool CmdRefreshChannels()
{
   double dTick = gettick();
   EDF *pRequest = NULL;

   pRequest = new EDF();
   pRequest->AddChild("searchtype", 3);

   delete m_pChannelList;
   CmdRequest(MSG_CHANNEL_LIST, pRequest, &m_pChannelList);
   m_pChannelList->Sort("channel", "name", true);
   debug("CmdRefreshChannels %ld ms\n", tickdiff(dTick));
   debugEDFPrint(m_pChannelList);

   return true;
}

void CmdFolderFlatten(EDF *pDest, EDF *pSource)
{
   int iFolderID = 0;
   char *szType = NULL, *szName = NULL;

   pSource->Get(&szType, &iFolderID);
   pSource->GetChild("name", &szName);

   // printf("CmdFolderFlatten %s %d %s\n", szType, iFolderID, szName);

   while(pSource->Child("folder") == true)
   {
      CmdFolderFlatten(pDest, pSource);
   }

   if(stricmp(szType, "folder") == 0)
   {
      pDest->Copy(pSource);
   }

   delete[] szName;

   pSource->Delete();
}

bool CmdRefreshFolders(bool bIdleReset)
{
   STACKTRACE
   int iFolderNav = NAV_ALPHATREE, iSubType = 0, iFolderMode = FOLDERMODE_NORMAL, iFolderLevel = -1, iPos = 0, iFolderID = -1;
   bool bLoop = false;
   char *szFolderName = NULL;
   EDF *pRequest = NULL, *pReply = NULL, *pTemp = NULL;
   EDF *pEditor = NULL, *pMember = NULL, *pPrivate = NULL, *pRestricted = NULL, *pCopy = NULL;

   debug(DEBUGLEVEL_INFO, "CmdFreshFolders entry\n");

   if(m_bRequestWait == true)
   {
      debug(DEBUGLEVEL_DEBUG, "CmdRefreshFolders request waiting");
      if(mask(m_iRequest, CLI_REFRESH_FOLDERS) == false)
      {
         debug(DEBUGLEVEL_DEBUG, ", queueing");
         m_iRequest |= CLI_REFRESH_FOLDERS;
      }
      debug(DEBUGLEVEL_DEBUG, "\n");
      return false;
   }

   pRequest = new EDF();
   pRequest->AddChild("searchtype", 2);
   if(bIdleReset == false)
   {
      pRequest->AddChild("idlereset", false);
   }
   if(CmdRequest(MSG_FOLDER_LIST, pRequest, &pReply) == true)
   {
      delete m_pFolderList;
      m_pFolderList = pReply;
      m_pFolderList->Sort("folder", "name", true);

      // debugEDFPrint("CmdFreshFolders", m_pFolderList);

      /* m_pFolderList->Root();
      bLoop = m_pFolderList->Child("folder");
      while(bLoop == true)
      {
         m_pFolderList->Get(NULL, &iFolderID);
         m_pFolderList->GetChild("name", &szFolderName);
         debug("CmdRefreshFolders %d %s\n", iFolderID, szFolderName);
         delete[] szFolderName;

         bLoop = m_pFolderList->Iterate("folder");
      }

      m_pFolderList->Root(); */

      delete m_pFolderNav;
      m_pFolderNav = new EDF();
      m_pFolderNav->Copy(pReply, true, true);

      bLoop = m_pFolderNav->Child("folder");
      while(bLoop == true)
      {
         while(m_pFolderNav->DeleteChild("unread") == true);
         while(m_pFolderNav->DeleteChild("editor") == true);
         while(m_pFolderNav->DeleteChild("replyid") == true);

         bLoop = m_pFolderNav->Iterate("folder");
      }

      m_pUser->Root();
      if(m_pUser->Child("client", CLIENT_NAME()) == true)
      {
         m_pUser->GetChild("foldernav", &iFolderNav);
         m_pUser->Parent();
      }

      if(iFolderNav != NAV_ALPHATREE)
      {
         pTemp = new EDF();

         pTemp->AddChild(m_pFolderNav, "numfolders");

         CmdFolderFlatten(pTemp, m_pFolderNav);

         delete m_pFolderNav;
         m_pFolderNav = pTemp;

         // debugEDFPrint("CmdRefreshFolders flat list", m_pFolderNav);
      }

      m_pFolderNav->Sort("folder", "name", true);

      // debugEDFPrint("CmdRefreshFolders post sort", m_pFolderNav);

      if(iFolderNav == NAV_LEVEL)
      {
         pEditor = new EDF();
         pMember = new EDF();
         pPrivate = new EDF();
         pRestricted = new EDF();
         pTemp = new EDF();

         bLoop = m_pFolderNav->Child("folder");
         while(bLoop == true)
         {
            pCopy = NULL;
            iSubType = 0;
            iFolderMode = FOLDERMODE_NORMAL;
            iFolderLevel = -1;
            iFolderID = -1;
            szFolderName = NULL;

            m_pFolderNav->Get(NULL, &iFolderID);
            m_pFolderNav->GetChild("name", &szFolderName);
            m_pFolderNav->GetChild("subtype", &iSubType);
            m_pFolderNav->GetChild("accessmode", &iFolderMode);
            m_pFolderNav->GetChild("accesslevel", &iFolderLevel);

            if(iSubType == SUBTYPE_EDITOR)
            {
               debug(DEBUGLEVEL_DEBUG, "CmdRefreshFolders %d -> editor\n", iFolderID);
               pCopy = pEditor;
            }
            else if(iSubType == SUBTYPE_MEMBER || iFolderLevel != -1)
            {
               debug(DEBUGLEVEL_DEBUG, "CmdRefreshFolders %d -> member\n", iFolderID);
               pCopy = pMember;
            }
            else if(mask(iFolderMode, ACCMODE_PRIVATE) == true)
            {
               debug(DEBUGLEVEL_DEBUG, "CmdRefreshFolders %d -> private\n", iFolderID);
               pCopy = pPrivate;
            }
            else if(mask(iFolderMode, ACCMODE_SUB_READ) == false)
            {
               debug(DEBUGLEVEL_DEBUG, "CmdRefreshFolders %d -> restricted\n", iFolderID);
               pCopy = pRestricted;
            }

            if(pCopy != NULL)
            {
               pCopy->Copy(m_pFolderNav);
               iPos = m_pFolderNav->Position(true);
               m_pFolderNav->Delete();
               bLoop = m_pFolderNav->Child("folder", iPos);
            }
            else
            {
               pTemp->Copy(m_pFolderNav);
               iPos = m_pFolderNav->Position(true);
               m_pFolderNav->Delete();
               bLoop = m_pFolderNav->Child("folder", iPos);
            }
         }

         while(m_pFolderNav->DeleteChild("folder") == true);

         m_pFolderNav->Copy(pPrivate, false);
         m_pFolderNav->Copy(pEditor, false);
         m_pFolderNav->Copy(pMember, false);
         m_pFolderNav->Copy(pRestricted, false);
         m_pFolderNav->Copy(pTemp, false);

         delete pEditor;
         delete pMember;
         delete pPrivate;
         delete pRestricted;
         delete pTemp;
      }

      bLoop = m_pFolderNav->Child("folder");
      while(bLoop == true)
      {
         while(m_pFolderNav->DeleteChild("editor") == true);
         while(m_pFolderNav->DeleteChild("accessmode") == true);
         while(m_pFolderNav->DeleteChild("accesslevel") == true);
         while(m_pFolderNav->DeleteChild("replyid") == true);

         bLoop = m_pFolderNav->Iterate("folder");
      }

      if(iFolderNav != NAV_ALPHATREE)
      {
         /* CmdPageOn();
         CmdEDFPrint("CmdRefreshFolders nav", m_pFolderNav);
         CmdPageOff(); */

         debugEDFPrint("CmdRefreshFolders nav", m_pFolderNav);
      }
   }

   debug(DEBUGLEVEL_INFO, "CmdRefreshFolders exit true\n");
   return true;
}

bool CmdRefreshMessages(int iFolderID, bool bIdleReset)
{
   STACKTRACE
   int iAccessMode = FOLDERMODE_NORMAL, iUserID = 0, iFldUnread = 0, iMsgUnread = 0;
   char *szReply = NULL;
   EDF *pRequest = NULL, *pReply = NULL;

   if(m_bRequestWait == true)
   {
      debug(DEBUGLEVEL_DEBUG, "CLiRefreshMessages request waiting");
      if(mask(m_iRequest, CLI_REFRESH_MESSAGES) == false)
      {
         debug(DEBUGLEVEL_DEBUG, ", queueing");
         m_iRequest |= CLI_REFRESH_MESSAGES;
      }
      debug(DEBUGLEVEL_DEBUG, "\n");
      m_iRefreshMessagesID = iFolderID;
      return false;
   }

   m_pUser->TempMark();
   m_pUser->Root();
   m_pUser->Get(NULL, &iUserID);
   m_pUser->TempUnmark();

   // m_pFolderList->TempMark();
   if(FolderGet(m_pFolderList, iFolderID, NULL, false) == true)
   {
      m_pFolderList->GetChild("accessmode", &iAccessMode);
   }
   // m_pFolderList->TempUnmark();

   pRequest = new EDF();
   pRequest->AddChild("folderid", iFolderID);
   pRequest->AddChild("searchtype", CmdVersion("2.3") >= 0 ? 3 : 1);
   if(mask(iAccessMode, ACCMODE_PRIVATE) == true)
   {
      pRequest->AddChild("toid", iUserID);
      pRequest->AddChild("fromid", iUserID);
   }

   // EDFPrint("CmdRefreshMessages request", pRequest);

   if(bIdleReset == false)
   {
      pRequest->AddChild("idlereset", false);
   }
   if(CmdRequest(MSG_MESSAGE_LIST, pRequest, &pReply) == false)
   {
      debug(DEBUGLEVEL_ERR, "CmdRefreshMessages message list failed\n");
      debugEDFPrint(pReply);

      delete pReply;
      delete[] szReply;

      return false;
   }

   delete m_pMessageList;
   m_pMessageList = pReply;

   m_pFolderList->GetChild("unread", &iFldUnread);
   // m_pMessageList->GetChild("unread", &iMsgUnread);
   iMsgUnread = m_pMessageList->Children("message", true) - m_pMessageList->Children("read", true);
   if(iFldUnread != iMsgUnread)
   {
      debug(DEBUGLEVEL_WARN, "CmdRefreshMessages fixing unsync'ed unread count (f=%d m=%d)\n", iFldUnread, iMsgUnread);
      m_pFolderList->SetChild("unread", iMsgUnread);
   }

   // EDFPrint("CmdRefreshMessages reply", m_pMessageList);

   return true;
}

void CmdRefresh(bool bUserReset)
{
   STACKTRACE
   double dTick = 0;
   long lRecieved = 0;

   lRecieved = (long)m_pClient->Recieved();
   dTick = gettick();

   if(bUserReset == true)
   {
      CmdUserReset();
   }
   CmdRefreshUsers();
   CmdRefreshFolders();
   CmdRefreshChannels();
   CmdRefreshServices();

   lRecieved = (long)m_pClient->Recieved() - lRecieved;
   debug(DEBUGLEVEL_INFO, "CmdRefresh completed in %ld ms with extra %ld bytes recieved\n", tickdiff(dTick), lRecieved);
}

void CmdColours(bool bChild)
{
   int iColourNum = 0, iColour = 0, iHighlight = 0;
   char szColourName[32];

   // printf("CmdColours %s\n", BoolStr(bChild));

   if(bChild == false || m_pUser->Child("client") == true)
   {
      m_pUser->GetChild("highlight", &iHighlight);
      if(iHighlight == 3)
      {
         for(iColourNum = 0; iColourNum <= 8; iColourNum++)
         {
            sprintf(szColourName, "colour%d", iColourNum);
            iColour = 0;
            m_pUser->GetChild(szColourName, &iColour);
            if(iColour > 0)
            {
               CmdColourSet(iColourNum, iColour);
            }
            else
            {
               switch(iColourNum)
               {
                  case 0:
                     CmdColourSet(0, 0);
                     break;

                  case 1:
                     CmdColourSet(1, 'R');
                     break;

                  case 2:
                     CmdColourSet(2, 'R');
                     break;

                  case 3:
                     CmdColourSet(3, 'G');
                     break;

                  case 4:
                     CmdColourSet(4, 'Y');
                     break;

                  case 5:
                     CmdColourSet(5, 'B');
                     break;

                  case 6:
                     CmdColourSet(6, 'M');
                     break;

                  case 7:
                     CmdColourSet(7, 'C');
                     break;

                  case 8:
                     CmdColourSet(8, 'W');
                     break;
               }
            }
         }

         iColour = 0;
         m_pUser->GetChild("colourbg", &iColour);
         CmdColourSet(-1, iColour);
      }
      else if(iHighlight == 2)
      {
         CmdColourSet(0, 0);
         CmdColourSet(1, 'R');
         CmdColourSet(2, 'R');
         CmdColourSet(3, 'G');
         CmdColourSet(4, 'Y');
         CmdColourSet(5, 'B');
         CmdColourSet(6, 'M');
         CmdColourSet(7, 'C');
         CmdColourSet(8, 'W');

         CmdColourSet(-1, 0);
      }
      if(bChild == true)
      {
         m_pUser->Parent();
      }
   }
}

void CmdUserSet(EDF *pUser, bool bTimeOn)
{
   STACKTRACE
   bool bMenuCase = false, bUTF8 = false;
   int iUserID = 0, iTimeOn = 0, iTimeOff = 0, iMenuLevel = CMDLEV_BEGIN, iStatus = 0, iHighlight = 0, iValue = 0, iHardWrap = 0, iConfirm = 0;

   debugEDFPrint(DEBUGLEVEL_INFO, "CmdUserSet input user", pUser);

   delete m_pUser;
   m_pUser = new EDF();
   m_pUser->Copy(pUser, true, true);

   if(m_pUser->GetChild("userid", &iUserID) == true)
   {
      m_pUser->DeleteChild("userid");
      m_pUser->Set("user", iUserID);
   }

   // debugEDFPrint("CmdUserSet current user", m_pUser);

   if(m_pUser->Child("login") == true)
   {
      m_pUser->GetChild("status", &iStatus);
      m_pUser->GetChild("timeon", &iTimeOn);
      m_pUser->GetChild("timeoff", &iTimeOff);
      m_pUser->Parent();

      debug(DEBUGLEVEL_INFO, "CmdUserSet status %d, timeon %d, timeoff %d\n", iStatus, iTimeOn, iTimeOff);

      if(bTimeOn == true)
      {
         CmdInput::MenuTime(iTimeOn);
      }
   }

   // debugEDFPrint("CmdUserSet client check section", m_pUser, false);
   if(m_pUser->Child("client") == true)
   {
      // debugEDFPrint("CmdUserSet client section", m_pUser, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      m_pUser->GetChild("menulevel", &iMenuLevel);
      bMenuCase = m_pUser->GetChildBool("menucase");
      // printf("CmdUserSet menu case %s\n", BoolStr(bMenuCase));
      m_pUser->GetChild("highlight", &iHighlight);
      // bHardWrap = m_pUser->GetChildBool("hardwrap");
      m_pUser->GetChild("hardwrap", &iHardWrap);
      m_pUser->GetChild("confirm", &iConfirm);

      // CmdEDFPrint("CmdUserSet client section", m_pUser, false);
      if(CmdLocal() == false)
      {
         if(m_bScreenSize == true)
         {
            if(m_pUser->GetChild("width", &iValue) == true && iValue > 0)
            {
               CmdWidth(iValue);
            }
            if(m_pUser->GetChild("height", &iValue) == true && iValue > 0)
            {
               CmdHeight(iValue);
            }
         }

         bUTF8 = m_pUser->GetChildBool("utf8");
         CmdUTF8(bUTF8);
      }

      CmdColours(false);

      m_pUser->Parent();
   }

   // debug("CmdUserSet menulevel %d, highlight %d\n", iMenuLevel, iHighlight);

   CmdInput::MenuLevel(iMenuLevel);
   CmdInput::MenuCase(bMenuCase);
   CmdInput::MenuStatus(iStatus);
   CmdInput::HardWrap(iHardWrap);
   CmdInput::SpellCheck(mask(iConfirm, CONFIRM_SPELL_CHECK));

   CmdHighlight(iHighlight);
}

bool CmdUserReset()
{
   STACKTRACE
   EDF *pRequest = NULL, *pReply = NULL;

   if(m_bRequestWait == true)
   {
      debug(DEBUGLEVEL_DEBUG, "CmdUserReset request waiting");
      if(mask(m_iRequest, CLI_USER_RESET) == false)
      {
         debug(DEBUGLEVEL_DEBUG, ", queueing");
         m_iRequest |= CLI_USER_RESET;
      }
      debug(DEBUGLEVEL_DEBUG, "\n");
      return false;
   }

   pRequest = new EDF();
   pRequest->AddChild("searchtype", 4);
   if(CmdRequest(MSG_USER_LIST, pRequest, &pReply) == false)
   {
      debug(DEBUGLEVEL_ERR, "CmdUserReset request failed\n");
      return false;
   }

   if(pReply->Child("user") == true)
   {
      CmdUserSet(pReply, false);
   }
   else
   {
      delete pReply;

      debug(DEBUGLEVEL_ERR, "CmdUserReset no user section\n");
      return false;
   }
   delete pReply;

   return true;
}

int CmdAnnounce(EDF *pAnnounce, bool bAddToBuffer, bool bReturn)
{
   STACKTRACE
   int iTemp = 0, iReturn = 0;
   EDF **pAnnounces = NULL;

   CmdAnnounceProcess(pAnnounce);
   if(bAddToBuffer == true)
   {
      iTemp = m_iNumAnnounces;
      // printf("CmdAnnounce buffer %p\n", pAnnounce);
      ARRAY_INSERT(EDF *, m_pAnnounces, m_iNumAnnounces, pAnnounce, iTemp, pAnnounces)
   }
   else
   {
      iReturn = CmdAnnounceShow(pAnnounce, bReturn == true ? "\n" : "");
   }

   return iReturn;
}

CmdInput *CmdInputLoop(int iMenuStatus, CmdInput *pInput, byte *pcInput, byte **pszInput)
{
   STACKTRACE
   int iAnnounceNum = 0, iAnnounce = 0;
   long lTimeout = 0;
   bool bInternal = false, bLoop = true, bShow = true;
   int cRead = '\0';
   char cProcess = '\0', *szType = NULL, *szReturn = NULL;
   EDF *pRead = NULL; // new EDF();

   // debug("CmdInputLoop entry %d %p %p\n", iMenuStatus, pcInput, pszInput);

   if(iMenuStatus != -1)
   {
      pInput = CmdInputSetup(iMenuStatus);
      bInternal = true;
   }

   STACKTRACEUPDATE

   if(m_pClient != NULL)
   {
      lTimeout = m_pClient->Timeout();
      m_pClient->Timeout(0);
   }

   /* if(iMenuStatus != -1 && m_iNumAnnounces > 0)
   {
      printf("CmdInputLoop %d announcements\n", m_iNumAnnounces);
      bShow = false;
   } */

   while(bLoop == true)
   {
      if(iMenuStatus != -1 && m_iNumAnnounces > 0)
      {
         if(bShow == false)
         {
            szReturn = "\n";
         }
         else
         {
            szReturn = "";
         }

         /* if(bShow == true)
         {
            pInput->Show();
            bShow = false;
         } */

         for(iAnnounceNum = 0; iAnnounceNum < m_iNumAnnounces; iAnnounceNum++)
         {
            iAnnounce |= CmdAnnounceShow(m_pAnnounces[iAnnounceNum], szReturn);
            szReturn = "";
            delete m_pAnnounces[iAnnounceNum];
         }

         delete[] m_pAnnounces;
         m_pAnnounces = NULL;
         m_iNumAnnounces = 0;

         if(mask(iAnnounce, CMD_RESET) == true)
         {
            delete pInput;
            pInput = CmdInputSetup(iMenuStatus);
            bShow = true;
         }
         else if(mask(iAnnounce, CMD_REDRAW) == true)
         {
            bShow = true;
         }
      }

      if(bShow == true)
      {
         pInput->Show();
         bShow = false;
      }

      if(m_pClient != NULL)
      {
         pRead = m_pClient->Read();
         if(pRead != NULL)
         {
            STACKTRACEUPDATE
            pRead->Get(&szType);
            if(stricmp(szType, "announce") == 0)
            {
               debugEDFPrint(m_iLogLevel >= 2 ? 0 : DEBUGLEVEL_DEBUG, "CmdInputLoop announce", pRead);
               iAnnounce = CmdAnnounce(pRead, iMenuStatus == -1, !mask(pInput->Type(), CMD_MENU_SILENT));
               if(iMenuStatus != -1)
               {
                  delete pRead;
                  // pRead = NULL;
               }
               // pRead = new EDF();
               if(iMenuStatus != -1)
               {
                  if(mask(iAnnounce, CMD_RESET) == true)
                  {
                     delete pInput;
                     pInput = CmdInputSetup(iMenuStatus);
                     // pInput->Show();
                     bShow = true;
                  }
                  else if(mask(iAnnounce, CMD_REDRAW) == true)
                  {
                     // pInput->Show();
                     bShow = true;
                  }
               }
            }
            else
            {
               debugEDFPrint("CmdInputLoop unknown message", pRead);
               delete pRead;
               // pRead = NULL;
            }
            delete[] szType;
            // delete pRead;
         }
         else if(m_pClient->State() != Conn::OPEN)
         {
            CmdShutdown("Connection to host lost");
         }
      }

      STACKTRACEUPDATE

      cRead = CmdInputGet();
      if(cRead != '\0')
      {
         if(bShow == true)
         {
            pInput->Show();
            bShow = false;
         }

         // debug("CmdInputLoop read '%c'\n", cRead);
         cProcess = pInput->Process(cRead);
         if(cProcess != '\0')
         {
            if(pcInput != NULL)
            {
               *pcInput = cProcess;
            }
            else if(pszInput != NULL)
            {
               *pszInput = pInput->LineData();
            }
            bLoop = false;
         }
      }
   }

   STACKTRACEUPDATE

   // delete pRead;

   if(bInternal == true)
   {
      delete pInput;
      return NULL;
   }

   // debug("CmdInputLoop exit %d\n", iReturn);
   return pInput;
}

bool CmdRequest(const char *szRequest, bool bSupressError)
{
   return CmdRequest(szRequest, NULL, true, NULL, bSupressError);
}

bool CmdRequest(const char *szRequest, EDF **pReply, bool bSupressError)
{
   return CmdRequest(szRequest, NULL, true, pReply, bSupressError);
}

bool CmdRequest(const char *szRequest, EDF *pRequest, EDF **pReply, bool bSupressError)
{
   return CmdRequest(szRequest, pRequest, true, pReply, bSupressError);
}

bool CmdRequest(const char *szRequest, EDF *pRequest, bool bDelete, EDF **pReply, bool bSupressError)
{
   STACKTRACE
   bool bLoop = true, bReturn = false;
   int iRequests = 0, iAnnounces = 0, iReads = 0;
   long lTimeout = 0;
   char szShutdown[100], szDebug[200];
   char *szType = NULL, *szMessage = NULL;
   EDF *pRead = NULL, *pTemp = NULL;
   EDFElement *pElement = NULL;
   double dEntry = 0, dWrite = 0, dRead = 0, dInput = 0, dRefresh = 0, dRecieved = 0, dLoop = 0, dProcess = 0;
   long lWrite = 0, lRead = 0, lInput = 0, lRefresh = 0;

   debug(DEBUGLEVEL_INFO, "CmdRequest entry %s %p %s %p %s\n", szRequest, pRequest, BoolStr(bDelete), pReply, BoolStr(bSupressError));

   CmdRedraw(false);

   dEntry = gettick();

   if(pRequest == NULL)
   {
      pTemp = new EDF();
      pTemp->Set("request", szRequest);
   }
   else
   {
      pTemp = pRequest;
      pTemp->Root();
   }

   pTemp->Set("request", szRequest);
   debugEDFPrint(m_iLogLevel >= 1 ? 0 : DEBUGLEVEL_DEBUG, "CmdRequest sending", pTemp);
   dWrite = gettick();
   if(m_pClient->Write(pTemp) == false)
   {
      debug(DEBUGLEVEL_ERR, "CmdRequest write failed\n");
      CmdShutdown("Unable to write request");
   }
   
   lWrite = tickdiff(dWrite);

   m_bRequestWait = true;

   if(pRequest == NULL || bDelete == true)
   {
      // debug("CmdRequest delete temp\n");
      delete pTemp;
   }

   STACKTRACEUPDATE

   // debug("CmdRequest timeout %ld ms\n", m_pClient->Timeout());

   lTimeout = m_pClient->Timeout();
   m_pClient->Timeout(100);

   // dRead = gettick();

   dRecieved = m_pClient->Recieved();

   dLoop = gettick();

   // pRead = new EDF();
   while(bLoop == true)
   {
      // printf("CmdRequest read loop\n");

      // pRead = NULL;
      /* iReads++;
      if(iReads % 100 == 0)
      {
         debug("CmdRequest %d reads after %ld ms\n", iReads, tickdiff(dLoop));
      } */

      dRead = gettick();
      pRead = m_pClient->Read();
      lRead += tickdiff(dRead);
      // if(m_pClient->Read(pRead, &iReadLen) == true)
      if(pRead != NULL)
      {
         dProcess = gettick();
         // EDFPrint("CmdRequest read", pRead);
			// debug("CmdRequest read succeeded after %ld ms\n", tickdiff(dRead));

         // debug("CmdRequest parsed %d byte EDF\n", iReadLen);
         // debugEDFPrint(m_iLogLevel >= 1 ? 0 : DEBUGLEVEL_DEBUG, "CmdRequest read", pRead);
         STACKTRACEUPDATE

         // lReadDiff = tickdiff(dRead);

         pRead->Get(&szType, &szMessage);
         if(stricmp(szType, "reply") == 0)
         {
            STACKTRACEUPDATE
            debugEDFPrint(m_iLogLevel >= 1 ? 0 : DEBUGLEVEL_DEBUG, "CmdRequest reply", pRead);

            // debugEDFPrint("CmdRequest reply", pRead);

            bLoop = false;
            m_bRequestWait = false;

			   pElement = m_pUser->GetCurr();
			   m_pUser->Root();
			   if(m_pUser->Child("login") == true)
			   {
				   m_pUser->GetChild("requests", &iRequests);
				   iRequests++;
				   m_pUser->SetChild("requests", iRequests);
			   }
			   m_pUser->SetCurr(pElement);

            if(stricmp(szMessage, szRequest) == 0)
            {
               bReturn = true;
            }
            else
            {
               debugEDFPrint("CmdRequest non-request reply back", pRead);
            }

            /* if(bReplyShow == true)
            {
               STACKTRACEUPDATE

               CmdReplyShow(pRead);
            } */
            if(pReply != NULL)
            {
               *pReply = pRead;
            }
            else
            {
               // debug("CmdRequest delete read\n");
               delete pRead;
               pRead = NULL;
            }
         }
         else if(stricmp(szType, "announce") == 0)
         {
            STACKTRACEUPDATE
            debugEDFPrint(m_iLogLevel >= 2 ? 0 : DEBUGLEVEL_DEBUG, "CmdRequest announce", pRead);

            dInput = gettick();
            CmdAnnounce(pRead, true, false);
            // delete pRead;
            // pRead = NULL;
            // pRead = new EDF();
            lInput += tickdiff(dInput);

            iAnnounces++;
         }

         delete[] szType;
         delete[] szMessage;

         /* if(tickdiff(dProcess) > 250)
         {
            sprintf(szDebug, "CmdRequest process %ld ms\n", tickdiff(dProcess));
            CmdWrite(szDebug);
         } */
      }
      /* else if(tickdiff(dRead) > 250)
      {
			sprintf(szDebug, "CmdRequest read failed after %ld ms, (state=%d), %g bytes\n", tickdiff(dRead), m_pClient->State(), m_pClient->Recieved() - dRecieved);
         CmdWrite(szDebug);
         debug(szDebug);
         // lReadTotal += tickdiff(dRead);
      } */

      if(m_pClient->State() != Conn::OPEN) // && pRead != NULL)
      {
         // debug("CmdRequest delete read\n");
         delete pRead;
         pRead = NULL;

         strcpy(szShutdown, "Disconnected");
         if(m_pClient->Error() != NULL)
         {
            debug(DEBUGLEVEL_ERR, "CmdRequest adding error %s\n", m_pClient->Error());
            strcat(szShutdown, ". ");
            strcat(szShutdown, m_pClient->Error());
         }
         CmdShutdown(szShutdown, true, bSupressError);
      }

      // debug("CmdRequest single loop %ld ms\n", tickdiff(dLoop));
   }

   /* if(tickdiff(dLoop) > 250)
   {
      debug("CmdRequest loop %ld ms\n", tickdiff(dLoop));
   } */

   // lRead = tickdiff(dRead);

   /* if(pReply == NULL && pRead != NULL)
   {
      // debug("CmdRequest delete read\n");
      delete pRead;
      pRead = NULL;
   } */

   STACKTRACEUPDATE

   /* if(bReplyShow == true)
   {
      CmdAnnounceFlush("CmdRequest", true);
   } */

   STACKTRACEUPDATE

   dRefresh = gettick();
   if(mask(m_iRequest, CLI_REFRESH_FOLDERS) == true)
   {
      m_iRequest -= CLI_REFRESH_FOLDERS;
      CmdRefreshFolders();
   }
   if(mask(m_iRequest, CLI_REFRESH_MESSAGES) == true)
   {
      m_iRequest -= CLI_REFRESH_MESSAGES;
      CmdRefreshMessages(m_iRefreshMessagesID);
   }
   if(mask(m_iRequest, CLI_REFRESH_USERS) == true)
   {
      m_iRequest -= CLI_REFRESH_USERS;
      CmdRefreshUsers();
   }
   if(mask(m_iRequest, CLI_USER_RESET) == true)
   {
      m_iRequest -= CLI_USER_RESET;
      CmdUserReset();
   }
   lRefresh = tickdiff(dRefresh);

   // sprintf(szDebug, "%ld (%ld / %ld / %ld) ms", tickdiff(dEntry), lWrite, lRead, lInput);
   /* CmdWrite("CmdRequest ");
   CmdWrite(szDebug);
   CmdWrite("\n"); */

   sprintf(szDebug, "%s, nr=%d na=%d, rd=%ld / rf=%ld / fn=%ld ms", szRequest, iReads, iAnnounces, lRead, lRefresh, tickdiff(dEntry));

   m_pClient->Timeout(lTimeout);

   debug(DEBUGLEVEL_INFO, "CmdRequest exit %s, %s\n", szRequest, szDebug);
   return bReturn;
}

byte CmdMenu(int iMenuStatus)
{
   byte cInput = '\0';

   CmdInputLoop(iMenuStatus, NULL, &cInput, NULL);

   return cInput;
}

byte CmdMenu(CmdInput *pInput)
{
   STACKTRACE
   byte cInput = '\0';

   pInput = CmdInputLoop(-1, pInput, &cInput, NULL);
   delete pInput;

   return cInput;
}

bool CmdYesNo(const char *szTitle, bool bDefault)
{
   STACKTRACE
   byte cInput = '\0';
   CmdInput *pInput = NULL;

   pInput = new CmdInput(CMD_MENU_YESNO | CMD_MENU_NOCASE, szTitle);
   if(bDefault == true)
   {
      pInput->MenuDefault('y');
   }
   pInput = CmdInputLoop(-1, pInput, &cInput, NULL);
   delete pInput;

   return (cInput == 'y' ? true : false);
}

char *CmdLineStr(const char *szTitle, int iMax, int iOptions, const char *szInit, CMDTABFUNC pTabFunc, EDF *pTabData)
{
   return CmdLineStr(szTitle, NULL, iMax, iOptions, szInit, pTabFunc, pTabData);
}

char *CmdLineStr(const char *szTitle, const char *szExtra, int iMax, int iOptions, const char *szInit, CMDTABFUNC pTabFunc, EDF *pTabData)
{
   STACKTRACE
   char *szInput = NULL;
   CmdInput *pInput = NULL;

   pInput = new CmdInput(szTitle, szExtra, iMax, iOptions, szInit, pTabFunc, pTabData);
   pInput = CmdInputLoop(-1, pInput, NULL, (byte **)&szInput);
   delete pInput;

   return szInput;
}

int CmdLineName(const char *szType, const char *szTitle, CMDTABFUNC pTabFunc, EDF *pData, int iInitID, char **szReturn, bool bValid, const char *szDefaultOp, const char *szInit)
{
   STACKTRACE
   int iReturn = -1, iInput = -1;
   bool bRetro = false;
   char *szInput = NULL, *szMenu = NULL, *szTemp = NULL;
   char szWrite[100], szExtra[100];
   CmdInput *pInput = NULL;
   CMDTABFUNC pTabDefault = NULL;

   m_pUser->TempMark();
   m_pUser->Root();
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      bRetro = CmdRetroNames(m_pUser);
   }
   m_pUser->TempUnmark();

   if(szType != NULL)
   {
      if(stricmp(szType, "user") == 0)
      {
         if(pTabFunc == NULL)
         {
            pTabFunc = CmdUserTab;
         }
         pTabDefault = CmdUserTab;
         if(pData == NULL)
         {
            pData = m_pUserList;
         }
         if(iInitID != -1)
         {
            UserGet(pData, iInitID, &szMenu, true, -1);
         }
      }
      else if(stricmp(szType, "folder") == 0)
      {
         if(pTabFunc == NULL)
         {
            pTabFunc = CmdFolderTab;
         }
         if(pData == NULL)
         {
            pData = m_pFolderList;
         }
         if(iInitID != -1)
         {
            FolderGet(pData, iInitID, &szMenu, true);
         }
      }
      else if(stricmp(szType, "channel") == 0)
      {
         if(pTabFunc == NULL)
         {
            pTabFunc = CmdChannelTab;
         }
         if(pData == NULL)
         {
            pData = m_pChannelList;
         }
         if(iInitID != -1)
         {
            ChannelGet(pData, iInitID, &szMenu, true);
         }
      }
   }

   if(pTabDefault == NULL)
   {
      pTabDefault = pTabFunc;
   }

   if(szTitle != NULL)
   {
      strcpy(szWrite, szTitle);
   }
   else
   {
      sprintf(szWrite, "Name of %s", szType);
   }

   if(szMenu != NULL && szInit == NULL)
   {
      sprintf(szWrite, "%s (\0374%s\0370)", szWrite, RETRO_NAME(szMenu));
   }
   
   if(szDefaultOp != NULL)
   {
      sprintf(szExtra, "RETURN %s", szDefaultOp);
   }
   else if(iInitID == -1)
   {
      strcpy(szExtra, "RETURN to abort");
   }
   else
   {
      strcpy(szExtra, "");
   }

   pInput = new CmdInput(szWrite, szExtra, bValid == true ? UA_NAME_LEN : LINE_LEN, pTabFunc != NULL ? CMD_LINE_TAB : 0, szInit, pTabFunc, pData);

   pInput = CmdInputLoop(-1, pInput, NULL, (byte **)&szInput);
   iReturn = pInput->LineValue();
   debug("CmdLineName %d (%s)\n", iReturn, szInput);

   // debug("CmdLineName %s\n", szInput);
   if(szInput != NULL)
   {
      if(strcmp(szInput, "") == 0)
      {
         if(iInitID != -1)
         {
            iReturn = iInitID;
         }
         else if(szDefaultOp == NULL)
         {
            delete[] szInput;
            szInput = NULL;
         }
      }
      else
      {
         // iReturn = pInput->LineValue();

         iInput = atoi(szInput);
         if(iInput > 0)
         {
            // Lookup using numeric ID
            if(stricmp(szType, "user") == 0)
            {
               if(UserGet(pData, iInput, NULL, true, -1) == true)
               {
                  iReturn = iInput;
               }
            }
            else if(stricmp(szType, "folder") == 0)
            {
               if(FolderGet(pData, iInput, NULL, true) == true)
               {
                  iReturn = iInput;
               }
            }
            else if(stricmp(szType, "channel") == 0)
            {
               if(ChannelGet(pData, iInput, NULL, true) == true)
               {
                  iReturn = iInput;
               }
            }
         }
         else
         {
            szTemp = strtrim(szInput);
            delete[] szInput;
            szInput = szTemp;

            if(pTabFunc != NULL)
            {
               if(iReturn == -1 && pTabFunc != NULL)
               {
                  pTabDefault(pData, szInput, strlen(szInput), true, &iReturn);
               }
            }
         }

         if(iReturn == -1 && bValid == true)
         {
            sprintf(szWrite, "No such %s\n", szType);
            CmdWrite(szWrite);
         }
      }
   }

   if(szReturn != NULL)
   {
      *szReturn = szInput;
   }
   else
   {
      delete[] szInput;
   }

   delete pInput;

   // printf("CmdLineName exit %d\n", iReturn);
   return iReturn;
}

int CmdLineTab(const char *szTitle, CMDTABFUNC pTabFunc, EDF *pData)
{
   return CmdLineName(NULL, szTitle, pTabFunc, pData, -1, NULL, true, NULL, NULL);
}

int CmdLineUser(CMDTABFUNC pTabFunc, int iInitID, char **szReturn, bool bValid, const char *szDefaultOp, const char *szInit)
{
   return CmdLineName("user", NULL, pTabFunc, NULL, iInitID, szReturn, bValid, szDefaultOp, szInit);
}

int CmdLineFolder(int iInitID, char **szReturn, bool bValid)
{
   return CmdLineName("folder", NULL, NULL, NULL, iInitID, szReturn, bValid, NULL, NULL);
}

int CmdLineFolder(const char *szTitle, int iInitID, char **szReturn, bool bValid)
{
   return CmdLineName("folder", szTitle, NULL, NULL, iInitID, szReturn, bValid, NULL, NULL);
}

int CmdLineChannel()
{
   return CmdLineName("channel", NULL, NULL, NULL, -1, NULL, true, NULL, NULL);
}

int CmdLineNum(const char *szTitle, int iOptions, int iDefault, int iInit)
{
   return CmdLineNum(szTitle, NULL, iOptions, iDefault, iInit);
}

int CmdLineNum(const char *szTitle, const char *szExtra, int iOptions, int iDefault, int iInit)
{
   STACKTRACE
   int iInput = 0;
   byte *szInput = NULL;
   char szInit[10];
   CmdInput *pInput = NULL;

   if(iInit > 0)
   {
      sprintf(szInit, "%d", iInit);
   }
   else
   {
      strcpy(szInit, "");
   }

   pInput = new CmdInput(szTitle, szExtra, NUMBER_LEN, iOptions | CMD_LINE_NOESCAPE, szInit);
   CmdInputLoop(-1, pInput, NULL, &szInput);
   if(strcmp((char *)szInput, "") == 0)
   {
      iInput = iDefault;
   }
   else
   {
      iInput = atoi((char *)szInput);
   }
   delete[] szInput;

   return iInput;
}

int CmdLineNum(int iMenuStatus)
{
   STACKTRACE
   int iInput = 0;
   byte *szInput = NULL;

   CmdInputLoop(iMenuStatus, NULL, NULL, (byte **)&szInput);
   iInput = atoi((char *)szInput);
   delete[] szInput;

   return iInput;
}

int CmdVersion(char *szVersion)
{
   STACKTRACE
   int iReturn = 0;
   char *szCompare = NULL;

   // debug("CmdVersion %s\n", szVersion);

   if(m_pSystemList == NULL)
   {
      CmdRequest(MSG_SYSTEM_LIST, &m_pSystemList);
   }

   m_pSystemList->GetChild("protocol", &szCompare);

   iReturn = stricmp(szCompare, szVersion);
   // debug("CmdVersion %s -vs- %s = %d\n", szCompare, szVersion, iReturn);

   delete[] szCompare;

   return iReturn;
}

char *CmdEditor()
{
	return m_szEditor;
}

int main(int argc, char **argv)
{
   int iArgNum = 0, iPort = 0, iTimeOff = -1, iBuildNum = 0, iAccessLevel = LEVEL_NONE, iFolderID = 0, iValue = 0, iSystemTime = 0, iAttachmentSize = 0;
   int iUserType = 0, iNumEdits = 0, iGiveUp = 10, iLoop = 0, iMsgMark = 0, iConfigHighlight = -1, iSubType = 0, iStatus = 0, iDebugLevel = DEBUGLEVEL_INFO;
   bool bSecure = false, bBusy = false, bSilent = false, bShadow = false, bLoop = false, bFolderCheck = true, bRetro = false, bUsePID = false, bLoggedIn = false, bMarkingField = false;
   bool bNew = false, bBrowse = false, bBrowserWait = false;
   double dTick = 0;
   char szWrite[200], szError[200];
   char *szConfig = NULL, *szServer = NULL, *szUsername = NULL, *szPassword = NULL, *szDebugDir = NULL, *szReply = NULL, *szName = NULL;
   char *szServerName = NULL, *szBuildDate = NULL, *szFolder = NULL, *szCertFile = NULL, *szBrowser = NULL, *szAttachmentDir = NULL;
   EDF *pFile = NULL, *pRequest = NULL, *pRead = NULL, *pReply = NULL, *pProxy = NULL, *pSystem = NULL;

#ifdef LEAKTRACEON
   leakSetStack1();
#endif

#ifdef UNIX
#ifndef BUILDDAEMON
   if(getenv("HOME") == NULL)
   {
      umask(077);
      m_szHome = "/tmp";
   }
   else
   {
      umask(077);
      m_szHome = getenv("HOME");
   }
#endif
#endif

   if(argc == 1)
   {
      szConfig = new char[256];
      if(CmdType() == 1)
      {
         sprintf(szConfig, "%s/.qUAckrc", m_szHome);
      }
      else
      {
         strcpy(szConfig, "qUAck.edf");
      }
   }
   else if(argc == 2 && (strcmp(argv[1], "-version") == 0 || strcmp(argv[1], "--version") == 0 ||
      strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "--help") == 0))
   {
      printf("%s build %d, %s %s\n", CLIENT_NAME(), BUILDNUM, BUILDTIME, BUILDDATE);

      if(strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "--help") == 0)
      {
         printf("Usage: qUAck [<config file> [options]]\n");
         printf("             -noconfig (must be first arg)\n");
         printf("             -server <string>\n");
         printf("             -port <number>\n");
         printf("             -username <string>\n");
         printf("             -password <string>\n");
         printf("             -debugdir <string>\n");
#ifdef UNIX
         printf("             -usepid\n");
#endif
#ifdef HAVE_LIBSSL
         printf("             -secure\n");
         printf("             -cetificate <string>\n");
#endif
         printf("             -thinpipe\n");
         printf("             -screensize\n");
         printf("             -loglevel [level]\n");
      }

      return 0;
   }
   else if(argc >= 2 && argv[1][0] != '-')
   {
      szConfig = strmk(argv[1]);
   }

   if(szConfig != NULL)
   {
      printf("Using %s as config file...\n", szConfig);
      pFile = FileToEDF(szConfig);
      if(pFile != NULL)
      {
         // EDFPrint("Config file", pFile);

         pFile->GetChild("server", &szServer);
         pFile->GetChild("port", &iPort);
#ifdef HAVE_LIBSSL
         bSecure = pFile->GetChildBool("secure");
         pFile->GetChild("certificate", &szCertFile);
#endif

         pFile->GetChild("username", &szUsername);
         pFile->GetChild("password", &szPassword);

         if(CmdLocal() == true)
         {
            pFile->GetChild("editor", &m_szEditor);
            if(pFile->Child("browser") == true)
            {
               pFile->Get(NULL, &szBrowser);
               bBrowserWait = pFile->GetChildBool("wait");
               pFile->Parent();
            }
            bBrowserWait = pFile->GetChildBool("browserwait", bBrowserWait);
#ifdef WIN32
            if(szBrowser != NULL || pFile->GetChildBool("browse") == true)
            {
               CmdBrowser(szBrowser, bBrowserWait);
            }
#else
            if(szBrowser != NULL)
            {
               CmdBrowser(szBrowser, bBrowserWait);
            }
#endif
            delete[] szBrowser;

            pFile->GetChild("attachmentsize", &iAttachmentSize);
            pFile->GetChild("attachmentdir", &szAttachmentDir);
            if(szAttachmentDir != NULL && strlen(szAttachmentDir) > 0)
            {
               CmdAttachmentDir(szAttachmentDir);
            }
            delete[] szAttachmentDir;
         }

         bBusy = pFile->GetChildBool("busy");
         bSilent = pFile->GetChildBool("silent");
         bShadow = pFile->GetChildBool("shadow");
         m_bThinPipe = pFile->GetChildBool("thinpipe");
         m_bScreenSize = pFile->GetChildBool("screensize");
         pFile->GetChild("loglevel", &m_iLogLevel);

         bUsePID = pFile->GetChildBool("usepid");

         pFile->GetChild("highlight", &iConfigHighlight);

         pFile->GetChild("debuglevel", &iDebugLevel);


         // delete pFile;
      }
      else
      {
         printf("Unable to read %s, %s\n", szConfig, strerror(errno));
      }
   }

   iArgNum = 1;
   if(szConfig != NULL)
   {
      iArgNum++;
   }
   while(iArgNum < argc)
   {
      // debug("Arg %d, %s", iArgNum, argv[iArgNum]);
      if(iArgNum < argc - 1 && stricmp(argv[iArgNum], "-server") == 0)
      {
         szServer = strmk(argv[++iArgNum]);
         // debug(". server %s", szServer);
      }
      else if(iArgNum < argc - 1 && stricmp(argv[iArgNum], "-port") == 0)
      {
         iPort = atoi(argv[++iArgNum]);
         // debug(". port %d", iPort);
      }
      else if(iArgNum < argc - 1 && stricmp(argv[iArgNum], "-username") == 0)
      {
         szUsername = strmk(argv[++iArgNum]);
         // debug(". username %s", szUsername);
         delete[] szPassword;
         szPassword = NULL;
      }
      else if(iArgNum < argc - 1 && stricmp(argv[iArgNum], "-password") == 0)
      {
         szPassword = strmk(argv[++iArgNum]);
         // debug(". password %s", szPassword);
      }
      else if(iArgNum < argc - 1 && stricmp(argv[iArgNum], "-debugdir") == 0)
      {
         szDebugDir = strmk(argv[++iArgNum]);
         // debug(". debug dir %s", szDebugDir);
      }
      else if(iArgNum < argc - 1 && stricmp(argv[iArgNum], "-debuglevel") == 0)
      {
         iDebugLevel = atoi(strmk(argv[++iArgNum]));
         // debug(". debug dir %s", szDebugDir);
      }
      else if(iArgNum < argc - 1 && stricmp(argv[iArgNum], "-highlight") == 0)
      {
         iConfigHighlight = atoi(argv[++iArgNum]);
      }
      else if(iArgNum < argc - 1 && stricmp(argv[iArgNum], "-loglevel") == 0)
      {
         m_iLogLevel = atoi(argv[++iArgNum]);
      }
      else if(stricmp(argv[iArgNum], "-usepid") == 0)
      {
         bUsePID = true;
      }
      else if(stricmp(argv[iArgNum], "-busy") == 0)
      {
         bBusy = true;
      }
      else if(stricmp(argv[iArgNum], "-silent") == 0)
      {
         bSilent = true;
      }
      else if(stricmp(argv[iArgNum], "-shadow") == 0)
      {
         bShadow = true;
      }
      else if(stricmp(argv[iArgNum], "-thinpipe") == 0)
      {
         m_bThinPipe = true;
      }
      else if(stricmp(argv[iArgNum], "-screensize") == 0)
      {
         m_bScreenSize = true;
      }
#ifdef HAVE_LIBSSL
      else if(stricmp(argv[iArgNum], "-secure") == 0)
      {
         // debug(". secure");
         bSecure = true;
      }
#endif
      else
      {
         printf("Unrecognised option %s\n", argv[iArgNum]);
      }
      // debug("\n");
      iArgNum++;
   }

#ifdef WIN32
   debugopen("qUAck.txt");
#else
   char szDebug[100];
   sprintf(szDebug, "%s/qUAck", szDebugDir != NULL ? szDebugDir : m_szHome);
   if(bUsePID == true)
   {
      sprintf(szDebug, "%s-%d", szDebug, getpid());
   }
   strcat(szDebug, ".txt");
   debugopen(szDebug);
#endif

   debuglevel(iDebugLevel);

   debug("%s build %d, %s %s\n", CLIENT_NAME(), BUILDNUM, BUILDTIME, BUILDDATE);

   debug("Non-config settings: server = %s, port = %d, secure = %s, username = %s, password = %s\n", szServer, iPort, BoolStr(bSecure), szUsername, szPassword != NULL ? "***" : NULL);

   CmdStartup();

   // Divert signals to the Panic function
   for(int i = SIGHUP; i < SIGUNUSED; i++)
   {
#ifdef UNIX
      if(i != SIGINT && i != SIGCHLD && i != SIGTSTP && i != SIGWINCH)
#else
      if(i == SIGABRT || i == SIGFPE || i == SIGILL || i == SIGINT || i == SIGSEGV || i == SIGTERM)
#endif
      {
         signal(i, Panic);
      }
   }

#ifndef CONTROLC
   // signal(SIGINT, SIG_IGN);
#else
   // signal(SIGINT, Panic);
#endif

#ifdef UNIX
   signal(SIGTSTP, SIG_IGN);
   signal(SIGCHLD, SIG_IGN);

   // signal(SIGWINCH, CmdReset);
#endif

   if(szServer == NULL)
   {
      szServer = CmdLineStr("Server name", LINE_LEN, CMD_LINE_NOESCAPE);
      iPort = CmdLineNum("Port number");
#ifdef HAVE_LIBSSL
      bSecure = CmdYesNo("Use secure connection", false);
#endif
   }

   m_pClient = new EDFConn();
   m_pClient->Timeout(50);

   debug("qUAck mem0: %ld bytes\n", memusage());

   dTick = gettick();
   debug(DEBUGLEVEL_INFO, "connecting to %s / %d / %s\n", szServer, iPort, BoolStr(bSecure));
   if(m_pClient->Connect(szServer, iPort, bSecure, szCertFile) == false)
   {
      sprintf(szError, "Unable to connect to %s (port %d). %s", szServer, iPort, m_pClient->Error());
      CmdShutdown(szError);
   }

   debug("qUAck mem1: %ld bytes\n", memusage());

   iLoop = time(NULL);
   while(m_pClient->State() == Conn::OPEN && m_pClient->Connected() == false)
   {
      pRead = m_pClient->Read();

      /* if(m_pClient->State() != Conn::OPEN)
      {
         CmdShutdown("connection lost");
      } */

      if(time(NULL) > iLoop + iGiveUp)
      {
         sprintf(szWrite, "Could not enable EDF in %d seconds, giving up", iGiveUp);
         CmdShutdown(szWrite);
      }
   }

   debug("qUAck mem2: %ld bytes\n", memusage());

   if(m_pClient->State() != Conn::OPEN)
   {
      CmdShutdown("Disconnected");
   }
   debug(DEBUGLEVEL_DEBUG, "Diff after connection %ld ms\n", tickdiff(dTick));

   CmdRequest(MSG_SYSTEM_LIST, &pSystem);

   pSystem->GetChild("systemtime", &iSystemTime);
   pSystem->GetChild("version", &m_szVersion);
	CmdInput::MenuTime(iSystemTime);

   pProxy = new EDF();
   debug(DEBUGLEVEL_DEBUG, "Calling proxy hosting (version %s)\n", m_szVersion);
   ProxyHost(pProxy);

   if(szUsername == NULL)
   {
      szUsername = strmk(CmdUsername());
   }

#ifdef UNIX
#ifdef BUILDDAEMON
   // if(szUsername == NULL)
   {
      bool bCheck = false;
      char *szCheck = NULL, *szRun = NULL;
      char szExample[1000];

      if(pProxy->GetChild("hostname", &szCheck) == true && szCheck != NULL)
      {
         debug("Checking host %s\n", szCheck);

         if(strlen(szCheck) >= 17 && strcmp(szCheck + strlen(szCheck) - 17, "compsoc.man.ac.uk") == 0 && strcmp(szCheck, "mrtall.compsoc.man.ac.uk") != 0)
         {
            bCheck = true;
            szRun = "/usr/local/bin/qUAck";
         }
         else if(strlen(szCheck) > 12 && strcmp(szCheck + strlen(szCheck) - 12, "cs.man.ac.uk") == 0)
         {
            bCheck = true;
            szRun = "~willija8/bin/qUAck";
         }
         /* else if(stricmp(szCheck, "riffraff.plig.net") == 0)
         {
            bCheck = true;
            szRun = "~dsf/bin/qUAck";
         }
         else if(stricmp(szCheck, "button") == 0)
         {
            bCheck = true;
            szRun = "d:\\qUAck-0.8\\bin\\qUAck.exe";
         } */

         delete[] szCheck;

         if(bCheck == true)
         {
            CmdWrite("\nThe host you are connecting from has qUAck\nYou should use qUAck in preference to qUAckd\n");

            if(CmdYesNo("Carry on anyway", true) == false)
            {
               strcpy(szExample, "Using qUAck\n-----------\n\nPut this in a file called .qUAckrc in your home directory:\n\n");
               strcat(szExample, "<>\n");
               sprintf(szExample, "%s  <server=\"%s\"/>\n  <port=%d/>\n", szExample, szServer, iPort);
               if(CmdVersion("2.5") >= 0)
               {
                  strcat(szExample, "  <secure=1/>          <-- Use this for an encrypted session\n");
               }
               strcat(szExample, "  <username=\"...\"/>\n  <password=\"...\"/>\n");
               sprintf(szExample, "%sThen run %s\n", szExample, szRun);

               CmdShutdown(szExample);
            }
         }
      }
   }
#endif
#endif

   CmdBanner(pSystem);

   do
   {
      if(szUsername == NULL)
      {
         do
         {
            szUsername = CmdLineStr("Enter your username, NEW, GUEST or OFF", UA_NAME_LEN, CMD_LINE_NOESCAPE);
            if(stricmp(szUsername, "off") == 0)
            {
               CmdShutdown("Goodbye then");
            }
            else if(stricmp(szUsername, "new") == 0)
            {
               delete[] szUsername;
               CreateUserMenu(false, &szUsername, &szPassword);//, pSystemList);

               bNew = true;
            }
         }
         while(strcmp(szUsername, "") == 0);

         if(stricmp(szUsername, "guest") == 0)
         {
            delete[] szUsername;
            szUsername = NULL;

            iUserType = USERTYPE_TEMP;
         }
      }
      if(iUserType == 0 && szPassword == NULL)
      {
         // debug("getting password\n");
         szPassword = CmdLineStr("Enter your password", UA_NAME_LEN, CMD_LINE_SILENT | CMD_LINE_NOESCAPE);
      }

      pRequest = new EDF();
      if(szUsername != NULL)
      {
         pRequest->AddChild("name", szUsername);
         debug(DEBUGLEVEL_INFO, "login username %s\n", szUsername);
      }
      if(szPassword != NULL)
      {
         pRequest->AddChild("password", szPassword);
      }
      if(iUserType > 0)
      {
         pRequest->AddChild("usertype", iUserType);
      }
      pRequest->AddChild("client", CLIENT_NAME());
      pRequest->AddChild("clientbase", CLIENT_BASE);
      pRequest->AddChild("protocol", PROTOCOL);
      iStatus = 0;
      if(bBusy == true)
      {
         iStatus += LOGIN_BUSY;
      }
      if(bSilent == true)
      {
         iStatus += LOGIN_SILENT;
      }
      if(bShadow == true)
      {
         iStatus += LOGIN_SHADOW;
      }
      if(iStatus > 0)
      {
         pRequest->AddChild("status", iStatus);
      }

      pRequest->AddChild(pProxy, "hostname");
      pRequest->AddChild(pProxy, "address");
      if(iAttachmentSize > 0 || iAttachmentSize == -1)
      {
         pRequest->AddChild("attachmentsize", iAttachmentSize);
      }

      debug(DEBUGLEVEL_INFO, "login request, tick reset\n");
      dTick = gettick();
      if(CmdRequest(MSG_USER_LOGIN, pRequest, false, &pReply) == true)
      {
         bLoggedIn = true;
      }
      else
      {
         delete[] szUsername;
         szUsername = NULL;
         delete[] szPassword;
         szPassword = NULL;

         pReply->Get(NULL, &szReply);
         delete pReply;

         debug(DEBUGLEVEL_ERR, "login reply failed, %s\n", szReply);

         if(stricmp(szReply, MSG_USER_LOGIN_ALREADY_ON) != 0)
         {
            CmdWrite("Login failed\n");
         }

         if(stricmp(szReply, MSG_USER_LOGIN_ALREADY_ON) == 0)
         {
            if(CmdYesNo("You are already logged on. Do you wish to stop the other session", false) == false)
            {
               CmdShutdown("Goodbye then");
            }
            else
            {
               CmdWrite("\n");

               debug(DEBUGLEVEL_INFO, "login request force\n");

               pRequest->AddChild("force", true);
               if(CmdRequest(MSG_USER_LOGIN, pRequest, false, &pReply) == false)
               {
                  // CmdShutdown("Login failed");
                  CmdWrite("Login failed\n");
               }
               else
               {
                  bLoggedIn = true;
               }
            }
         }
         /* else
         {
         CmdShutdown("Login failed");
         } */

         debug(DEBUGLEVEL_DEBUG, "login delete reply string\n");

         delete[] szReply;
      }
      debug(DEBUGLEVEL_INFO, "After login request %ld ms\n", tickdiff(dTick));
   }
   while(bLoggedIn == false);

   debug(DEBUGLEVEL_DEBUG, "login post reply delete\n");

   delete[] szServer;
   delete[] szUsername;
   delete[] szPassword;
   delete pRequest;

   debug(DEBUGLEVEL_DEBUG, "user set\n");

   if(iConfigHighlight != -1)
   {
      if(pReply->Child("client") == false)
      {
         pReply->Add("client", CLIENT_NAME());
      }
      pReply->SetChild("highlight", iConfigHighlight);
      pReply->Parent();
   }
   CmdUserSet(pReply, true);
   delete pReply;

   // pSystemList->GetChild("systemtime", &iSystemTime);
   // printf("Setting server time %d (local time %d)\n", iSystemTime, time(NULL));
   // CmdServerTime(iSystemTime);

   // CmdColours(true);

   pSystem->GetChild("name", &szServerName);
   // pSystemList->GetChild("version", &szServerVersion);
   // ProtocolVersion(szServerVersion);
   pSystem->GetChild("buildnum", &iBuildNum);
   pSystem->GetChild("builddate", &szBuildDate);
   sprintf(szWrite, "\0370%s Version \0373%s\0370 Build \0373%d\0370, \0373%s\0370\n\n", szServerName, m_szVersion, iBuildNum, szBuildDate);
   CmdWrite(szWrite);
   delete[] szServerName;
   // delete[] szServerVersion;
   delete[] szBuildDate;
   delete pSystem;

   if(m_pUser->Child("folders") == true)
   {
      bMarkingField = m_pUser->IsChild("marking");
      m_pUser->Parent();
   }
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      CmdInput::MenuCase(m_pUser->GetChildBool("menucase"));

      bRetro = CmdRetroNames(m_pUser);
      if(bMarkingField == false && m_pUser->GetChild("msgmark", &iMsgMark) == true)
      {
         debug(DEBUGLEVEL_WARN, "Upgrading client msgmark setting of %d\n", iMsgMark);
         if(mask(iMsgMark, 4) == true)
         {
            iMsgMark += MARKING_EDIT_PUBLIC;
         }
         pRequest = new EDF();
         pRequest->Add("folders");
         pRequest->AddChild("marking", iMsgMark);
         pRequest->Parent();
         pRequest->Add("client", "delete");
         pRequest->AddChild("msgmark");
         CmdRequest(MSG_USER_EDIT, pRequest);

         m_pUser->Root();
         if(m_pUser->Child("folders") == false)
         {
            m_pUser->Add("folders");
         }
         m_pUser->SetChild("marking", iMsgMark);
      }

      if(m_pUser->IsChild("confirm") == false)
      {
         EDF *pTemp = NULL;

         debug(DEBUGLEVEL_DEBUG, "Setting initial confirm settings\n");

         pTemp = new EDF();
         pTemp->Add("client", "edit");
         m_pUser->SetChild("confirm", CONFIRM_ACTIVE_REPLY | CONFIRM_BUSY_PAGER);
         pTemp->SetChild("confirm", CONFIRM_ACTIVE_REPLY | CONFIRM_BUSY_PAGER);
         pTemp->Parent();

         CmdRequest(MSG_USER_EDIT, pTemp);
      }

      m_pUser->Parent();
   }

   m_pUser->GetChild("name", &szName);
   m_pUser->GetChild("accesslevel", &iAccessLevel);
   sprintf(szWrite, "Name: \0373%s\0370, Access level: \037%c%s\0370\n\n", RETRO_NAME(szName), AccessColour(iAccessLevel, 0), AccessName(iAccessLevel));
   CmdWrite(szWrite);

   if(CmdVersion("2.5") >= 0)
   {
      BulletinMenu(false);
   }

   if(m_pUser->Child("folders") == true)
   {
      iNumEdits = CmdSubList(m_pUser, SUBTYPE_EDITOR, 0, NULL, bRetro);
      if(iNumEdits > 0)
      {
         CmdWrite("\n");
      }

      m_pUser->Parent();
   }

   CmdRefresh(false);

   m_pUser->Root();
   if(m_pUser->Child("login") == true)
   {
      m_pUser->GetChild("timeoff", &iTimeOff);
      m_pUser->Parent();
   }

   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      m_pUser->GetChild("anntype", &iValue);
      bFolderCheck = mask(iValue, ANN_FOLDERCHECK);
      m_pUser->Parent();
   }

   delete[] szName;

   if(bNew == true)
   {
      if(CmdYesNo("Subscribe to all folders (recommended)", true) == true)
      {
         pRequest = new EDF();

         m_pFolderList->Root();
         bLoop = m_pFolderList->Child("folder");
         while(bLoop == true)
         {
            // EDFPrint("New folder subscribe", m_pFolderList, false);

            iSubType = 0;
            m_pFolderList->GetChild("subtype", &iSubType);
            if(iSubType == 0)
            {
               m_pFolderList->Get(NULL, &iFolderID);

               pRequest->SetChild("folderid", iFolderID);
               CmdRequest(MSG_FOLDER_SUBSCRIBE, pRequest, false);
            }

            bLoop = m_pFolderList->Iterate("folder");
         }

         delete pRequest;
      }
   }

   debug(DEBUGLEVEL_INFO, "qUAck time off %d\n", iTimeOff);
   if(iTimeOff != -1 && CmdVersion("2.3") >= 0)
   {
      pRequest = new EDF();
      pRequest->AddChild("searchtype", 2);
      pRequest->AddChild("startdate", iTimeOff);
      CmdRequest(MSG_FOLDER_LIST, pRequest, &pReply);
      debugEDFPrint("qUAck new folders", pReply);
      bLoop = pReply->Child("folder");
      while(bLoop == true)
      {
         pReply->GetChild("name", &szFolder);
         sprintf(szWrite, "\0374%s\0370: Created since last session. Subscribe", szFolder);
         // CmdWrite(szWrite);
         /* if(CmdYesNo(szWrite, true) == true)
         {
            pReply->Get(NULL, &iFolderID);

            pRequest = new EDF();
            pRequest->AddChild("folderid", iFolderID);
            CmdRequest(MSG_FOLDER_SUBSCRIBE, pRequest);
         } */
         pReply->Get(NULL, &iFolderID);
         CmdFolderJoin(iFolderID, szWrite, false);

         bLoop = pReply->Next("folder");
      }
      delete pReply;
   }

   // FolderCatchup(true);
   MessageMarkMenu(true, -1, -1, -1, NULL, true);

   if(bFolderCheck == true)
   {
      CmdMessageTreeList(m_pFolderList, "folder", 2, 5);
   }

   m_bAnnounceFlush = true;

   NewFeatures();

   if(CmdVersion("2.6") >= 0 && pFile != NULL)
   {
      ServiceActivate(pFile);
   }

   delete pFile;

   MainMenu();

   CmdShutdown();

   return 0;
}
