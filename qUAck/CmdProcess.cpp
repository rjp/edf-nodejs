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
** CmdProcess.cpp: Processing functions for menu options
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>

#include "Conn/EDFConn.h"

#include "ua.h"

#include "client/CliFolder.h"
#include "client/CliTalk.h"
#include "client/CliUser.h"

#include "CmdIO.h"
#include "CmdMenu.h"
#include "CmdShow.h"
#include "CmdProcess.h"

#include "qUAck.h"

int m_iFolderID = -1, m_iMessageID = -1, m_iAddReplyID = -1, m_iAddReplyFolder = -1;
int m_iChannelID = -1;

bool CmdAnnounceProcess(EDF *pAnnounce)
{
   STACKTRACE
   int iUserID = 0, iStatus = LOGIN_OFF, iFolderID = 0, iMessageID = 0, iFolderEDF = 0, iReplyID = -1, iNumUnread = 0;
   int iMoveID = 0, iFromID = 0, iMarked = 0, iTimeOff = -1;//, iValue = 0;
   int iFolderUnread = 0, iMoveUnread = 0, iAnnounces = 0, iNumShadows = 0;
   bool bLoop = false, bFound = false;
   char *szAnnounce = NULL, *szStatusMsg = NULL, *szTemp = NULL, *szFolderName = NULL;
   EDFElement *pElement = NULL, *pTemp = NULL;

   pElement = m_pUser->GetCurr();
   if(m_pUser->Child("login") == true)
   {
	   m_pUser->GetChild("announces", &iAnnounces);
	   iAnnounces++;
	   m_pUser->SetChild("announces", iAnnounces);
   }
   m_pUser->SetCurr(pElement);

   pAnnounce->Get(NULL, &szAnnounce);
   // debug("CmdAnnounceProcess %s\n", szAnnounce);

   if(stricmp(szAnnounce, MSG_FOLDER_ADD) == 0 || stricmp(szAnnounce, MSG_FOLDER_DELETE) == 0)
   {
      CmdRefreshFolders(false);
   }
   /* else if(stricmp(szAnnounce, MSG_FOLDER_SUBSCRIBE) == 0 || stricmp(szAnnounce, MSG_FOLDER_SUBSCRIBE) == 0)
   {
      m_pUser->Get(NULL, &iUserID);

      pAnnounce->GetChild("userid", &iValue);
      if(iValue == iUserID)
      {
         CmdRefreshFolders();
      }
   } */
   else if(stricmp(szAnnounce, MSG_FOLDER_EDIT) == 0)
   {
      debugEDFPrint("CmdAnnounceProcess folder edit", pAnnounce);

      pAnnounce->GetChild("folderid", &iFolderID);
      pAnnounce->GetChild("foldername", &szTemp);

      if(szTemp != NULL)
      {
         if(FolderGet(m_pFolderList, iFolderID, &szFolderName, false) == true)
         {
            if(strcmp(szTemp, szFolderName) != 0)
            {
               m_pFolderList->SetChild("name", szTemp);
            }
            delete[] szTemp;
         }
      }
   }
   else if(stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0 || stricmp(szAnnounce, MSG_MESSAGE_EDIT) == 0)
   {
      // debugEDFPrint("CmdAnnounceProcess message_add", pAnnounce);

      iUserID = -1;
      iReplyID = -1;

      pAnnounce->GetChild("folderid", &iFolderID);
      pAnnounce->GetChild("messageid", &iMessageID);
      pAnnounce->GetChild("replyto", &iReplyID);
      pAnnounce->GetChild("replyid", &iReplyID);
      if(stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0)
      {
         pAnnounce->GetChild("fromid", &iFromID);
      }
      else
      {
         pAnnounce->GetChild("byid", &iFromID);
      }
      pAnnounce->GetChild("marked", &iMarked);
      debug("CmdAnnounceProcess %s %d in %d by %d reply to %d (marked %d)\n", szAnnounce, iMessageID, iFolderID, iFromID, iReplyID, iMarked);

      if(FolderGet(m_pFolderList, iFolderID, NULL, false) == true)
      {
         // pAnnounce->GetChild("marked", &iMarked);
         // debug("CmdAnnounceProcess %s message marked %d\n", szAnnounce, iMarked);
         if((stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0 && iMarked != MARKED_READ) ||
            (stricmp(szAnnounce, MSG_MESSAGE_EDIT) == 0 && iMarked == MARKED_UNREAD))
         {
            // debug("CmdAnnounceProcess %s increasing unread count in folder list\n", szAnnounce);
            // debugEDFPrint(m_pFolderList, false);

            m_pFolderList->GetChild("unread", &iNumUnread);
            iNumUnread++;
            // debug("CmdAnnounceProcess %s resetting unread folder list count to %d\n", szAnnounce, iNumUnread);
            m_pFolderList->SetChild("unread", iNumUnread);
         }

         STACKTRACEUPDATE

         if(m_pMessageList != NULL)
         {
            pTemp = m_pMessageList->GetCurr();

            m_pMessageList->Root();
            m_pMessageList->GetChild("folderid", &iFolderEDF);
            if(iFolderID == iFolderEDF)
            {
               // debug("CmdAnnounceProcess %s current folder\n", szAnnounce);

               if(MessageInFolder(m_pMessageList, iMessageID) == false)
               {
                  m_pMessageList->Root();

                  if(stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0 && iReplyID == -1)
                  {
                     bFound = true;
                  }
                  else if(stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0 && iReplyID != -1)
                  {
                     bFound = MessageInFolder(m_pMessageList, iReplyID);
                  }
                  else if(stricmp(szAnnounce, MSG_MESSAGE_EDIT) == 0)
                  {
                     bFound = MessageInFolder(m_pMessageList, iMessageID);
                  }

                  if(bFound == true)
                  {
                     if(stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0)
                     {
                        if(iReplyID != -1)
                        {
                           // debugEDFPrint("CmdAnnounceProcess message add point", m_pMessageList);
                        }
                        m_pMessageList->Add("message", iMessageID);
                        // m_pMessageList->Copy(pAnnounce, false, false);
                        m_pMessageList->AddChild(pAnnounce, "announcetime", "date");
                     }

                     if(stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0 && iMarked == MARKED_READ)
                     {
                        // debug("CmdAnnounceProcess %s marked as read in message list\n", szAnnounce);

                        m_pMessageList->SetChild("read", true);
                     }
                     else if(stricmp(szAnnounce, MSG_MESSAGE_EDIT) == 0 && iMarked == MARKED_UNREAD)
                     {
                        // debug("CmdAnnounceProcess %s marked as unread in message list\n", szAnnounce);

                        m_pMessageList->DeleteChild("read");
                     }
                  }
                  /* else
                  {
                     debug("CmdAnnounceProcess cannot find message %d / %d for announce %s\n", iMessageID, iReplyID, szAnnounce);
                  } */
               }
               else
               {
                  debug("CmdAnnounceProcess message %d already in folder %d\n", iMessageID, iFolderID);
               }
            }

            m_pMessageList->SetCurr(pTemp);
         }
      }

      // debug("CmdAnnounceProcess %s complete\n", szAnnounce);
   }
   else if(stricmp(szAnnounce, MSG_MESSAGE_DELETE) == 0 || stricmp(szAnnounce, MSG_MESSAGE_MOVE) == 0)
   {
      pAnnounce->GetChild("folderid", &iFolderID);
      if(FolderGet(m_pFolderList, iFolderID, NULL, false) == true)
      {
         m_pFolderList->GetChild("unread", &iFolderUnread);
      }
      if(stricmp(szAnnounce, MSG_MESSAGE_MOVE) == 0)
      {
         pAnnounce->GetChild("moveid", &iMoveID);
         if(FolderGet(m_pFolderList, iMoveID, NULL, false) == true)
         {
            m_pFolderList->GetChild("unread", &iMoveUnread);
         }
      }
      if(iFolderUnread > 0 || iMoveUnread > 0)
      {
         // debug("CmdAnnounceProcess %s refreshing folder list\n", szAnnounce);
         CmdRefreshFolders(false);
      }

      if(iFolderID == CmdCurrFolder() || (stricmp(szAnnounce, MSG_MESSAGE_MOVE) == 0 && iMoveID == CmdCurrFolder()))
      {
         // debug("CmdAnnounceProcess %s refreshing message list\n", szAnnounce);
         CmdRefreshMessages(CmdCurrFolder(), false);
      }

      if(CmdVersion("2.5") < 0 && stricmp(szAnnounce, MSG_MESSAGE_MOVE) == 0)
      {
         bLoop = pAnnounce->Child("messageid");
         while(bLoop == true)
         {
            pAnnounce->Get(NULL, &iMessageID);
            if(iMessageID == m_iAddReplyID)
            {
               debug(DEBUGLEVEL_INFO, "CmdAnnounceProcess %s reply message %d moved to %d\n", szAnnounce, m_iAddReplyID, iMoveID);
               m_iAddReplyFolder = iMoveID;

               bLoop = false;
            }
            else
            {
               bLoop = pAnnounce->Next("messageid");
            }
         }
      }

      pAnnounce->Root();
   }
   else if(stricmp(szAnnounce, MSG_USER_ADD) == 0 || stricmp(szAnnounce, MSG_USER_DELETE) == 0)
   {
      CmdRefreshUsers(false);
   }
   else if(stricmp(szAnnounce, MSG_USER_LOGIN) == 0 || stricmp(szAnnounce, MSG_USER_LOGOUT) == 0 || stricmp(szAnnounce, MSG_USER_STATUS) == 0)
   {
      debugEDFPrint("CmdAnnounceProcess user something", pAnnounce);

      pAnnounce->GetChild("userid", &iUserID);

      if(UserGet(m_pUserList, iUserID, NULL, false) == true)
      {
         if(pAnnounce->GetChild("status", &iStatus) == true)
         {
            if(mask(iStatus, LOGIN_SHADOW) == false)
            {
               if(stricmp(szAnnounce, MSG_USER_LOGOUT) == 0)
               {
                  pAnnounce->GetChild("announcetime", &iTimeOff);

                  m_pUserList->SetChild("status", LOGIN_OFF);
                  m_pUserList->SetChild("timeoff", iTimeOff);
               }
               else
               {
                  m_pUserList->SetChild("status", iStatus);
               }

               if(CmdVersion("2.5") >= 0 || mask(iStatus, LOGIN_BUSY) == true)
               {
                  pAnnounce->GetChild(CmdVersion("2.5") >= 0 ? "statusmsg" : "busymsg", &szStatusMsg);
                  if(szStatusMsg != NULL)
                  {
                     // printf("CmdAnnounceProcess setting status message %s\n", szStatusMsg);
                     m_pUserList->SetChild("statusmsg", szStatusMsg);
                     delete[] szStatusMsg;
                  }
                  else
                  {
                     m_pUserList->DeleteChild("statusmsg");
                  }
                  if(mask(iStatus, LOGIN_BUSY) == true)
                  {
                     m_pUserList->SetChild(pAnnounce, "timebusy");
                  }
               }

               // debugEDFPrint("CmdAnnounceProcess user changes", m_pUserList, false);
            }
            else
            {
               m_pUserList->GetChild("numshadows", &iNumShadows);
               if(stricmp(szAnnounce, MSG_USER_LOGIN) == 0)
               {
                  iNumShadows++;
               }
               else if(stricmp(szAnnounce, MSG_USER_LOGOUT) == 0)
               {
                  iNumShadows--;
               }
               m_pUserList->SetChild("numshadows", iNumShadows);

               debugEDFPrint("CmdAnnounceProcess user shadow changes", m_pUserList, EDFElement::PR_SPACE);
            }
         }
         else
         {
            pAnnounce->GetChild(CmdVersion("2.5") >= 0 ? "statusmsg" : "busymsg", &szStatusMsg);
            if(szStatusMsg != NULL)
            {
               // printf("CmdAnnounceProcess setting status message %s\n", szStatusMsg);
               m_pUserList->SetChild("statusmsg", szStatusMsg);
               delete[] szStatusMsg;
            }
            else
            {
               m_pUserList->DeleteChild("statusmsg");
            }
         }
      }
      else
      {
         debug(DEBUGLEVEL_ERR, "CmdAnnounceProcess user %d not found\n", iUserID);
      }
   }

   delete[] szAnnounce;

   return false;
}

void CmdExtraVote(EDF *pRequest)
{
   STACKTRACE
   bool bLoop = true;
   char *szValue = NULL;

   while(bLoop == true)
   {
      szValue = CmdLineStr("Vote", "RETURN to end", LINE_LEN, 0, NULL, NULL, NULL);
      if(szValue != NULL && strlen(szValue) > 0)
      {
         pRequest->AddChild("vote", szValue);
      }
      else
      {
         bLoop = false;
      }
      delete[] szValue;
   }
}

void CmdExtraAttachment(EDF *pRequest)
{
   STACKTRACE
   size_t tDataLen = 0;
   char szWrite[200];
   char *szValue = NULL, *szFilename = NULL;
   byte *pData = NULL;
   bytes *pBytes = NULL;

   szValue = CmdLineStr("URL / Filename", "RETURN to abort", LINE_LEN);
   if(szValue != NULL && strlen(szValue) > 0)
   {
      if(strstr(szValue, "://") != 0)
      {
         pRequest->Add("attachment");
         pRequest->AddChild("url", szValue);
         pRequest->Parent();
      }
      else
      {
         pData = FileRead(szValue, &tDataLen);
         if(pData != NULL)
         {
            pRequest->Add("attachment");

            szFilename = szValue + strlen(szValue);
            while(szFilename > szValue && (*szFilename) != CmdDirSep())
            {
               szFilename--;
            }
            if((*szFilename) == CmdDirSep())
            {
               szFilename++;
            }
            pRequest->AddChild("filename", szFilename);

            pBytes = new bytes(pData, tDataLen);
            pRequest->AddChild("data", pBytes);
            delete pBytes;

            pRequest->Parent();

            delete[] pData;
         }
         else
         {
            sprintf(szWrite, "Cannot read \0374%s\0370, %s\n", szValue, strerror(errno));
            CmdWrite(szWrite);
         }
      }
   }

   delete[] szValue;
}

bool CmdExtraFields(EDF *pRequest, char *szField, bool bInitial)
{
   STACKTRACE
   int iFieldNum = 0;
   bool bLoop = false;
   char *szValue = NULL;
   char szTitle[100], szWrite[200];
   char cOption = '\0';
   CmdInput *pInput = NULL;

   if(bInitial == true && stricmp(szField, "vote") == 0)
   {
      CmdExtraVote(pRequest);
   }
   else
   {
      sprintf(szTitle, "%c%ss", toupper(szField[0]), szField + 1);

      while(cOption != 'x')
      {
         iFieldNum = 0;
         bLoop = pRequest->Child(szField);
         while(bLoop == true)
         {
            iFieldNum++;
            if((stricmp(szField, "vote") == 0 && pRequest->Get(NULL, &szValue) == true) ||
               pRequest->GetChild("url", &szValue) == true ||
               pRequest->GetChild("filename", &szValue) == true)
            {
               sprintf(szWrite, "\0373%d\0370: %s\n", iFieldNum, szValue);
               CmdWrite(szWrite);

               delete[] szValue;
            }

            bLoop = pRequest->Next(szField);
            if(bLoop == false)
            {
               pRequest->Parent();
            }
         }

         pInput = new CmdInput(CMD_MENU_NOCASE, szTitle);
         pInput->MenuAdd('a', "Add");
         pInput->MenuAdd('d', "Delete");
         pInput->MenuAdd('x', "eXit", NULL, true);

         cOption = CmdMenu(pInput);
         switch(cOption)
         {
            case 'a':
               if(stricmp(szField, "vote") == 0)
               {
                  CmdExtraVote(pRequest);
               }
               else if(stricmp(szField, "attachment") == 0)
               {
                  CmdExtraAttachment(pRequest);
               }
               break;

            case 'd':
               iFieldNum = CmdLineNum("Field", "RETURN to abort");
               if(iFieldNum > 0)
               {
                  pRequest->DeleteChild(szField, iFieldNum - 1);
               }
               break;

            case 'x':
               if(stricmp(szField, "vote") == 0 && pRequest->Children("vote") == 0)
               {
                  if(CmdYesNo("No voting options. Really exit", false) == false)
                  {
                     cOption = '\0';
                  }
               }
               break;
         }
      }
   }

   return true;
}

bool CmdMessageAddFields(EDF *pRequest)
{
   STACKTRACE
   bool bFolder = false, bInitial = false, bOption = false, bMinValue = false, bMaxValue = false;
   int iFolderMode = FOLDERMODE_NORMAL, iSubType = 0, iAccessLevel = LEVEL_NONE, iVoteType = 0;
   int iFolderID = -1, iMsgType = 0, iToID = -1, iVoteEDF = 0, iDefTo = -1;
   int iMinValue = -1, iMaxValue = -1;
	double dMinValue = 0, dMaxValue = 0;
   char cVoteType = 'a', cVoteValues = 'f';
   char *szOption = NULL, *szSubject = NULL, *szToName = NULL, *szUser = NULL;
   CmdInput *pInput = NULL;
   char cOption = '\0', cDefault = '\0';

   // debugEDFPrint("CmdMessageAddFields entry", pRequest);

   bFolder = pRequest->GetChild("folderid", &iFolderID);
   pRequest->GetChild("toid", &iToID);
   pRequest->DeleteChild("toid");
   pRequest->GetChild("toname", &szToName);
   pRequest->DeleteChild("toname");
   pRequest->GetChild("msgtype", &iMsgType);
   pRequest->GetChild("subject", &szSubject);
   if(pRequest->GetChildBool("initial") == true)
   {
      bInitial = true;
      pRequest->DeleteChild("initial");
   }

   iDefTo = iToID;

   if(bFolder == true && iFolderID != 0)
   {
      iFolderID = CmdLineFolder(iFolderID);
      // delete[] szFolderName;

      if(iFolderID == -1)
      {
         return false;
      }

      pRequest->SetChild("folderid", iFolderID);

      CmdFolderJoin(iFolderID, NULL, false);

      m_pUser->TempMark();
      m_pUser->Root();
      m_pUser->GetChild("accesslevel", &iAccessLevel);
      // iSubType = CmdFolderSubType(iFolderID);
      m_pUser->TempUnmark();

      m_pFolderList->TempMark();
      FolderGet(m_pFolderList, iFolderID, NULL, false);
      m_pFolderList->GetChild("subtype", &iSubType);
      m_pFolderList->GetChild("accessmode", &iFolderMode);
      m_pFolderList->TempUnmark();
      /* if(iAccessLevel < LEVEL_WITNESS &&
         !((mask(iFolderMode, ACCMODE_SUB_WRITE) == true && iSubType >= SUBTYPE_MEMBER) ||
         (mask(iFolderMode, ACCMODE_MEM_WRITE) == true && iSubType >= SUBTYPE_EDITOR)))
      {
         CmdWrite("Folder is read-only\n");
         return false;
      } */
      if(mask(iMsgType, MSGTYPE_VOTE) == true)
      {
         debug(DEBUGLEVEL_DEBUG, "CmdMessageAddFields vote check %d %d\n", iAccessLevel, iSubType);
      }
      /* if(mask(iMsgType, MSGTYPE_VOTE) == true && !(iAccessLevel >= LEVEL_WITNESS || iSubType >= SUBTYPE_EDITOR))
      {
         CmdWrite("Cannot post vote in folder\n");
         return false;
      } */

      if(mask(iFolderMode, ACCMODE_PRIVATE) == true)
      {
         iToID = CmdLineUser(CmdUserTab, iToID, &szOption, true, NULL, szToName);

         if(iToID == -1)
         {
            if(szOption != NULL && szToName != NULL && strcmp(szOption, szToName) == 0)
            {
               iToID = iDefTo;
            }
            else
            {
               return false;
            }
         }
      }
      else if(iMsgType == 0)
      {
         debug("CmdMessageAddFields params %d %s\n", iToID, szToName);
         iToID = CmdLineUser(CmdUserTab, -1, &szOption, false, "for everyone", szToName);
         debug("CmdMessageAddFields result %d %s\n", iToID, szOption);
         // debug("CmdMessageAddFields to check %d %s\n", iToID, szToName);
         if(iToID == -1)
         {
            debug("CmdMessageAddFields unmatched to ID\n");
            if(szOption != NULL && szToName != NULL && strcmp(szOption, szToName) == 0)
            {
               iToID = iDefTo;
            }
			}
         delete[] szToName;
         szToName = szOption;
      }
   }

   szOption = CmdLineStr("Subject", LINE_LEN, 0, szSubject);
   delete[] szSubject;
   if(szOption == NULL || strcmp(szOption, "") == 0)
   {
      return false;
   }
   pRequest->SetChild("subject", szOption);
   delete[] szOption;

   if(iMsgType == MSGTYPE_VOTE)
   {
      if(CmdVersion("2.7") >= 0)
      {
         iVoteType = 0;

         if(pRequest->GetChild("votetype", &iVoteEDF) == false)
         {
            debug("CmdMessageAddFields default vote type\n");
            iVoteEDF = VOTE_PUBLIC_CLOSE;
         }

         if(mask(iVoteEDF, VOTE_MULTI) == true)
         {
            cDefault = 'm';
         }
         else if(IS_VALUE_VOTE(iVoteEDF) == true)
         {
            cDefault = 'c';
         }
         else
         {
            cDefault = 's';
         }

         pInput = new CmdInput(CMD_MENU_NOCASE, "Vote type");
         pInput->MenuAdd('s', "Single option");
         pInput->MenuAdd('m', "Multiple options");
         pInput->MenuAdd('u', "User's own value");
         pInput->MenuDefault(cDefault);

         cOption = CmdMenu(pInput);

         if(cOption == 'm')
         {
            iVoteType += VOTE_NAMED + VOTE_MULTI;
         }
         else
         {
            if(CmdYesNo("Anonymous voting", mask(iVoteEDF, VOTE_NAMED) == false) == false)
            {
               iVoteType += VOTE_NAMED;
            }

            if(cOption == 'u')
            {
               pInput = new CmdInput(CMD_MENU_NOCASE, "Value type");
               pInput->MenuAdd('i', "Integer", NULL, mask(iVoteEDF, VOTE_INTVALUES) == true);
               pInput->MenuAdd('f', "Float", NULL, mask(iVoteEDF, VOTE_INTVALUES) == true);
               pInput->MenuAdd('p', "Percentage", NULL, mask(iVoteEDF, VOTE_PERCENT) == true);
               pInput->MenuAdd('l', "fLoat percentage", NULL, mask(iVoteEDF, VOTE_PERCENT) == true);
               pInput->MenuAdd('s', "String", NULL, mask(iVoteEDF, VOTE_STRVALUES) == true);

               cOption = CmdMenu(pInput);
               switch(cOption)
               {
                  case 'i':
                     iVoteType += VOTE_INTVALUES;

                     szOption = CmdLineStr("Minimum value", "RETURN for none");
                     if(szOption != NULL && strlen(szOption) > 0)
                     {
                        bMinValue = true;
                        iMinValue = atoi(szOption);
                     }
                     delete[] szOption;

                     szOption = CmdLineStr("Maximum value", "RETURN for none");
                     if(szOption != NULL && strlen(szOption) > 0)
                     {
                        bMaxValue = true;
                        iMaxValue = atoi(szOption);
                     }
                     delete[] szOption;
                     break;

                  case 'f':
                     iVoteType += VOTE_FLOATVALUES;

                     szOption = CmdLineStr("Minimum value", "RETURN for none");
                     if(szOption != NULL && strlen(szOption) > 0)
                     {
                        bMinValue = true;
                        dMinValue = atof(szOption);
                     }
                     delete[] szOption;

                     szOption = CmdLineStr("Maximum value", "RETURN for none");
                     if(szOption != NULL && strlen(szOption) > 0)
                     {
                        bMaxValue = true;
                        dMaxValue = atof(szOption);
                     }
                     delete[] szOption;
                     break;

                  case 'p':
                     iVoteType += VOTE_PERCENT;
                     break;

                  case 'l':
                     iVoteType += VOTE_FLOATPERCENT;
                     break;

                  case 's':
                     iVoteType += VOTE_STRVALUES;
                     break;
               }
            }
         }

         if(mask(iVoteType, VOTE_NAMED) == true && CmdYesNo("Allow vote changing", mask(iVoteEDF, VOTE_CHANGE) == true) == true)
         {
            iVoteType += VOTE_CHANGE;
         }

         bOption = CmdYesNo("Public results during voting", mask(iVoteEDF, VOTE_PUBLIC));
			if(bOption == true)
			{
				iVoteType += VOTE_PUBLIC;
			}
         if(bOption == false && CmdYesNo("Public results after voting", mask(iVoteEDF, VOTE_PUBLIC_CLOSE)) == true)
         {
            iVoteType += VOTE_PUBLIC_CLOSE;
         }

         pRequest->SetChild("votetype", iVoteType);

         if(bMinValue == true)
         {
				if(mask(iVoteType, VOTE_INTVALUES) == true)
				{
					pRequest->SetChild("minvalue", iMinValue);
				}
				else
				{
	            pRequest->SetChild("minvalue", dMinValue);
				}
         }
         else
         {
            pRequest->DeleteChild("minvalue");
         }
         if(bMaxValue == true)
         {
				if(mask(iVoteType, VOTE_INTVALUES) == true)
				{
	            pRequest->SetChild("maxvalue", iMaxValue);
				}
				else
				{
	            pRequest->SetChild("maxvalue", dMaxValue);
				}
         }
         else
         {
            pRequest->DeleteChild("maxvalue");
         }
      }
	   else if(CmdVersion("2.5") >= 0)
	   {
			iVoteType = 0;

         pRequest->GetChild("votetype", &iVoteEDF);

         pInput = new CmdInput(CMD_MENU_NOCASE, "Vote type");
         pInput->MenuAdd('a', "Anonymous", NULL, mask(iVoteEDF, VOTE_NAMED) == false);
         pInput->MenuAdd('n', "Named logged", NULL, mask(iVoteEDF, VOTE_NAMED) == true);
         // pInput->MenuAdd('v', "Voter's choice", NULL, cDefault == 'v');

         cOption = CmdMenu(pInput);
         switch(cOption)
         {
            case 'a':
               break;

            case 'n':
               iVoteType |= VOTE_NAMED;
				   /* if(CmdYesNo("Allow vote changes", false) == true)
				   {
					   iVoteType += VOTE_CHANGE;
				   } */
               break;

            case 'v':
               // iVoteType += VOTE_CHOICE;
               break;
			}

         if(CmdYesNo("Vote values", IS_VALUE_VOTE(iVoteEDF) == true) == true)
         {
            pInput = new CmdInput(CMD_MENU_NOCASE, "Values type");
            pInput->MenuAdd('i', "Numbers", NULL, cVoteValues == 'i');
            pInput->MenuAdd('p', "Percentage", NULL, cVoteValues == 'p');
            // pInput->MenuAdd('s', "Strings", NULL, cVoteValues == 's');
            // pInput->MenuAdd('v', "Voter's choice", NULL, cDefault == 'v');

            cOption = CmdMenu(pInput);
            switch(cOption)
            {
               case 'i':
                  iVoteType |= VOTE_INTVALUES;

                  iMinValue = CmdLineNum("Minimum value (blank for none)", 0, -1);
                  iMaxValue = CmdLineNum("Maximum value (blank for none)", 0, -1);
                  break;

               case 'p':
                  iVoteType |= VOTE_PERCENT;
                  break;

               /* case 's':
                  iVoteType |= VOTE_STRVALUES;
                  break; */
            }
         }

			if(CmdYesNo("Show results during voting", mask(iVoteEDF, VOTE_PUBLIC)) == true)
			{
				iVoteType |= VOTE_PUBLIC;
            iVoteEDF |= VOTE_PUBLIC_CLOSE;
			}
         else if(CmdYesNo("Show results after voting", mask(iVoteEDF, VOTE_PUBLIC_CLOSE)) == true)
         {
            iVoteType |= VOTE_PUBLIC_CLOSE;
         }

         pRequest->SetChild("votetype", iVoteType);
         if(mask(iVoteType, VOTE_INTVALUES) == true)
         {
            pRequest->AddChild("minvalue", iMinValue);
            pRequest->AddChild("maxvalue", iMaxValue);
         }
         else
         {
            pRequest->DeleteChild("minvalue");
            pRequest->DeleteChild("maxvalue");
         }
	   }

      if(IS_VALUE_VOTE(iVoteType) == false && CmdExtraFields(pRequest, "vote", bInitial) == false)
      {
         return false;
      }
   }

   if(iToID != -1)
   {
      pRequest->SetChild("toid", iToID);
      if(szToName != NULL && UserGet(m_pUserList, iToID, &szUser) == true)
		{
			debug("CmdMessageAddFields to check %d '%s' -vs- '%s'\n", iToID, szToName, szUser);

			if(stricmp(szToName, szUser) == 0)
      	{
         	delete[] szToName;
         	szToName = NULL;
      	}
		}
   }
   if(szToName != NULL)
   {
      pRequest->SetChild("toname", szToName);
   }
   if(iMsgType > 0)
   {
      pRequest->SetChild("msgtype", iMsgType);
   }

   if(CmdLocal() == true && CmdVersion("2.5") >= 0)
   {
      if(pRequest->GetChildBool("attachments") == true)
      {
         if(CmdExtraFields(pRequest, "attachment", false) == false)
         {
            return false;
         }
      }
      else
      {
         pRequest->SetChild("attachments", true);
      }
   }

   pRequest->SetChild("fields", true);

   // debugEDFPrint("CmdMessageAddFields exit", pRequest);
   return true;
}

bool CmdMessageAdd(int iReplyID, int iFolderID, int iReplyFolder, int iFromID, const char *szFromName, int iMsgType, bool bAttachments)
{
   STACKTRACE
   int iFolderEDF = -1;
   char *szText = NULL;
   EDF *pRequest = NULL, *pReply = NULL;

   debug(DEBUGLEVEL_INFO, "CmdMessageAdd entry %d %d %d %d %s %d\n", iReplyID, iFolderID, iReplyFolder, iFromID, szFromName, iMsgType);

   pRequest = new EDF();

   pRequest->AddChild("folderid", iFolderID);
   if(iReplyID != -1)
   {
      if(m_pMessageView != NULL)
      {
         pRequest->AddChild(m_pMessageView, "subject");
      }
   }

   m_iAddReplyID = iReplyID;
   m_iAddReplyFolder = iReplyFolder;

   /* if(iDefFolder != 0 && iDefFolder != -1)
   {
      pRequest->AddChild("deffolder", iDefFolder);
   }
   if(iReplyFolder != 0 && iReplyFolder != -1 && iReplyFolder != iDefFolder)
   {
      pRequest->AddChild("replyfolder", iReplyFolder);
   } */
   if(iFromID != -1)
   {
      pRequest->AddChild("toid", iFromID);
   }
   if(szFromName != NULL)
   {
      pRequest->AddChild("toname", szFromName);
   }
   if(iMsgType > 0)
   {
      pRequest->AddChild("msgtype", iMsgType);
   }

   // pRequest->AddChild("attachments", bAttachments);
   pRequest->AddChild("initial", true);

   if(CmdMessageAddFields(pRequest) == false)
   {
      delete pRequest;

      // debug("CmdMessageAdd exit false\n");
      return false;
   }

   szText = CmdText(CMD_LINE_EDITOR, NULL, CmdMessageAddFields, pRequest);
   if(szText == NULL)
   {
      delete pRequest;

      return false;
   }

	EDF *pSearch = NULL, *pResult = NULL;
	char *szTemp = szText, *szURL = NULL;
	while(szTemp != NULL)
	{
		szURL = URLToken(&szTemp);
		if(szURL != NULL)
		{
			pSearch = new EDF();
			pSearch->AddChild("text", szURL);
			debugEDFPrint(pSearch);
			if(CmdRequest(MSG_MESSAGE_LIST, pSearch, &pResult) == true)
			{
				debugEDFPrint("CmdMessageAdd result", pResult);
				delete pResult;
			}

			delete[] szURL;
		}
	}

   pRequest->GetChild("folderid", &iFolderEDF);
   pRequest->DeleteChild("folderid");

   pRequest->DeleteChild("attachments");

   pRequest->DeleteChild("fields");

   if(iReplyID != -1)
   {
      pRequest->SetChild("replyid", iReplyID);
   }
   // debug("CmdMessageAdd replyid(m) %d, folderid(m) %d, deffolder %d\n", m_iAddReplyID, m_iAddFolderID, iDefFolder);
   if(iReplyID == -1 || CmdVersion("2.5") < 0 || iFolderEDF != iFolderID)
   {
      debug(DEBUGLEVEL_INFO, "CmdMessageAdd folder ID %d EDF %d\n", iFolderID, iFolderEDF);
      pRequest->SetChild("folderid", iFolderEDF);
   }
   if(CmdVersion("2.5") < 0 && iReplyFolder != iFolderEDF)
   {
      pRequest->SetChild("replyfolder", iReplyFolder);
   }
   pRequest->AddChild("text", szText);

   debugEDFPrint("CmdMessageAdd request", pRequest, EDFElement::EL_ROOT | EDFElement::EL_CURR | EDFElement::PR_SPACE | EDFElement::PR_BIN);
   if(CmdRequest(iReplyFolder == 0 ? MSG_BULLETIN_ADD : MSG_MESSAGE_ADD, pRequest, &pReply) == false)
   {
      CmdEDFPrint("CmdMessageAddReply failed", pReply);
   }
   delete pReply;

   delete[] szText;

   debug(DEBUGLEVEL_INFO, "CmdMessageAdd exit true\n");
   return true;
}

bool CmdMessageGoto(int *iMsgPos, bool bShowOnly)
{
   STACKTRACE
   int iNumUnread = 0, iPos = 0, iDateCurr = -1, iDateInit = 0, iDateEDF = 0, iMessageEDF = 0;//, iFolderInit = m_iFolderID;
   int /* iCurrFrom = 0, iVoteType = 0, */ iFolderMode = 0, iMessageID = 0, iFolderID = m_iFolderID, iSubType = 0;
   int iFolderNav = 0;
   bool bLoop = false, bNewFolder = false, bReturn = false, bIDFromList = true;
   char szWrite[100];
   // char *szFolderName = NULL;
   // EDF *pRequest = NULL, *pReply = NULL, *pMessage = NULL;
   // EDFElement *pElement = NULL;

   // CmdRequestLog(true);

   // printf("CmdMessageGoto entry %d %s, %d %d\n", *iMsgPos, BoolStr(bShowOnly), m_iFolderID, m_iMessageID);
   // printf("CmdMessageGoto entry %d, %d %d\n", *iMsgPos, m_iFolderID, m_iMessageID);
   // EDFPrint(m_pMessageList);

   iMessageID = m_iMessageID;

   if(*iMsgPos == MSG_NEW || *iMsgPos == MSG_NEWFOLDER)
   {
      if(m_iFolderID == -1)
      {
         bNewFolder = true;
         m_pFolderNav->Root();
         bLoop = m_pFolderNav->Child("folder");
      }
      else
      {
         bLoop = FolderGet(m_pFolderNav, m_iFolderID, NULL, false);
         if(*iMsgPos == MSG_NEWFOLDER)
         {
            bNewFolder = true;
            *iMsgPos = MSG_NEW;
            bLoop = m_pFolderNav->Iterate("folder");
         }
      }

      // debug("CmdMessageGoto new message loop start folder %d\n", iFolderNav);

      while(bLoop == true && iNumUnread == 0)
      {
         // debugEDFPrint("CmdMessageGoto nav folder point", m_pFolderNav, EDFElement::EL_CURR | EDFElement::PR_SPACE);
         m_pFolderNav->Get(NULL, &iFolderNav);
         FolderGet(m_pFolderList, iFolderNav, NULL, false);

         iSubType = 0;
         m_pFolderList->GetChild("subtype", &iSubType);
         if(iSubType > 0)
         {
            iNumUnread = 0;
            m_pFolderList->GetChild("unread", &iNumUnread);
            m_pFolderList->GetChild("accessmode", &iFolderMode);
            // debug("CmdMessageGoto new message iteration folder %d, unread %d\n", iFolderNav, iNumUnread);

            // m_pFolderList->GetChild("name", &szFolderName);
            // printf("CmdMessageGoto %s, %d unread\n", szFolderName, iNumUnread);
            // delete[] szFolderName;
         }

         if(iNumUnread == 0)
         {
            bNewFolder = true;
            bLoop = m_pFolderNav->Iterate("folder");
         }
      }

      if(iNumUnread == 0)
      {
         m_iFolderID = -1;
         m_iMessageID = -1;
         debug(DEBUGLEVEL_DEBUG, "CmdMessageGoto reset message ID point 1\n");

         // debug("CmdMessageGoto exit false (num unread = 0), %d %d %d\n", *iMsgPos, m_iFolderID, m_iMessageID);
         // printf("CmdMessageGoto exit false (num unread = 0), %d %d %d\n", *iMsgPos, m_iFolderID, iMessageID);
         return false;
      }

      m_pFolderList->Get(NULL, &iFolderID);

      // printf("CmdMessageGoto new folder %s, %d %d\n", BoolStr(bNewFolder), *iFolderID, *iMessageID);
      if(bNewFolder == true)
      {
         CmdRefreshMessages(iFolderID);
         m_iFolderID = iFolderID;
         m_iMessageID = -1;
         debug(DEBUGLEVEL_DEBUG, "CmdMessageGoto reset message ID point 2\n");

         // debug("CmdMessageGoto exit true (new folder = true), %d %d %d\n", *iMsgPos, m_iFolderID, m_iMessageID);
         // printf("CmdMessageGoto exit true (new folder = true), %d %d %d\n", *iMsgPos, m_iFolderID, iMessageID);
         return true;
      }

      m_pMessageList->Root();
      if(iMessageID != -1)
      {
         bLoop = MessageInFolder(m_pMessageList, iMessageID);
      }
      // printf("CmdMessageGoto pre-iterate %s\n", BoolStr(bLoop));
      while(bLoop == true)
      {
         bLoop = m_pMessageList->Iterate("message");
         m_pMessageList->Get(NULL, &iMessageEDF);
         // printf("CmdMessageGoto iterate check %s '%s' %d %s\n", BoolStr(bLoop), m_pMessageList->GetCurr()->getName(false), iMessageEDF, BoolStr(m_pMessageList->GetChildBool("read")));
         if(bLoop == true && m_pMessageList->GetChildBool("read") == false)
         {
            // m_pMessageList->Get(NULL, iMessageID);

            bLoop = false;
            bReturn = true;
         }
         /* else
         {
            bLoop = m_pMessageList->Iterate("message");
         } */
      }
   }
   else if(*iMsgPos == MSG_TOP)
   {
      m_pMessageList->Root();
      if(m_pMessageList->Child("message") == true)
      {
         bReturn = true;
         // m_pMessageList->Get(NULL, iMessageID);
         *iMsgPos = MSG_FORWARD;
      }
   }
   else if(*iMsgPos == MSG_TOPTHREAD)
   {
      // if(CmdVersion("2.5") >= 0)
      {
         if(m_pMessageView != NULL)
         {
            if(m_pMessageView->Child("replyto", EDFElement::LAST) == true)
            {
               debug(DEBUGLEVEL_DEBUG, "CmdMessageGoto MSG_TOPTHREAD %d/%d", iMessageID, iFolderID);
               m_pMessageView->Get(NULL, &iMessageID);
               m_pMessageView->GetChild("folderid", &iFolderID);
               /* if(bShowOnly == false)
               {
                  m_iFolderID = iFolderID;
               } */
               m_pMessageView->Parent();
               debug(DEBUGLEVEL_DEBUG, " -> %d/%d\n", iMessageID, iFolderID);

               bReturn = true;
               bIDFromList = false;
            }
         }
      }
   }
   else if(*iMsgPos == MSG_BOTTOM)
   {
      m_pMessageList->Root();
      if(m_pMessageList->Child("message", EDFElement::LAST) == true)
      {
         while(m_pMessageList->Child("message", EDFElement::LAST) == true);
         bReturn = true;
         // m_pMessageList->Get(NULL, iMessageID);
         *iMsgPos = MSG_BACK;
      }
   }
   else if(*iMsgPos == MSG_FIRST || *iMsgPos == MSG_LAST || *iMsgPos == MSG_BACK || *iMsgPos == MSG_FORWARD)
   {
      if(*iMsgPos == MSG_FIRST || *iMsgPos == MSG_FORWARD)
      {
         iDateCurr = CmdInput::MenuTime();
      }
      else if(*iMsgPos == MSG_LAST || *iMsgPos == MSG_BACK)
      {
         iDateCurr = 0;
      }

      if(*iMsgPos == MSG_BACK || *iMsgPos == MSG_FORWARD)
      {
         m_pMessageList->Root();
         if(MessageInFolder(m_pMessageList, iMessageID) == true)
         {
            m_pMessageList->GetChild("date", &iDateInit);
         }
         else
         {
            debug(DEBUGLEVEL_ERR, "CmdMessageGoto cannot find %d in message list\n", iMessageID);
         }
      }
      // debug("CmdMessageGoto curr date %d, init date %d\n", iDateCurr, iDateInit);
      m_pMessageList->Root();

      // debug("CmdMessageGoto init %d\n", iDateInit);
      while(m_pMessageList->Iterate("message") == true)
      {
         m_pMessageList->Get(NULL, &iMessageEDF);
         m_pMessageList->GetChild("date", &iDateEDF);

         // debug("CmdMessageGoto check date %d %d", iMessageEDF, iDateEDF);
         if((*iMsgPos == MSG_FIRST && iDateEDF < iDateCurr) ||
            (*iMsgPos == MSG_LAST && iDateEDF > iDateCurr) ||
            (*iMsgPos == MSG_BACK && iDateEDF > iDateCurr && iDateEDF < iDateInit) ||
            (*iMsgPos == MSG_FORWARD && iDateEDF < iDateCurr && iDateEDF > iDateInit))
         {
            // debug(". set");

            bReturn = true;
            iDateCurr = iDateEDF;

             m_pMessageList->TempMark();
         }
         // debug("\n");
      }

      if(bReturn == true)
      {
         m_pMessageList->TempUnmark();
         // m_pMessageList->Get(NULL, iMessageID);
      }
      else
      {
         if(*iMsgPos == MSG_FIRST)
         {
            CmdWrite("No messages in folder\n");
         }
         else if(*iMsgPos == MSG_LAST)
         {
            CmdWrite("No messages in folder\n");
         }
         else if(*iMsgPos == MSG_BACK)
         {
            CmdWrite("No messages before this one\n");
         }
         else if(*iMsgPos == MSG_FORWARD)
         {
            CmdWrite("No messages after this one\n");
         }

         *iMsgPos = MSG_EXIT;
      }
   }
   else if(*iMsgPos == MSG_PREV || *iMsgPos == MSG_NEXT)
   {
      m_pMessageList->Root();
      if(MessageInFolder(m_pMessageList, iMessageID) == true)
      {
         if(*iMsgPos == MSG_NEXT)
         {
            if(bReturn == false && m_pMessageList->Iterate("message") == true)
            {
               bReturn = true;
            }
         }
         else
         {
            // EDFPrint("CmdMessageGoto current message", m_pMessageList, false);
            if(m_pMessageList->Prev("message") == true)
            {
               // EDFPrint("CmdMessageGoto prev message", m_pMessageList, false);
               bReturn = true;

               while(m_pMessageList->Child("message", EDFElement::LAST) == true);
               // EDFPrint("CmdMessageGoto new message", m_pMessageList, false);
            }
            else
            {
               iPos = m_pMessageList->Position(true);
               // debug("CmdMessageGoto message pos %d\n", iPos);
               if(m_pMessageList->Parent() == true)
               {
                  // EDFPrint("CmdMessageGoto parent message", m_pMessageList, false);
                  // bReturn = true;

                  if(iPos > 0)
                  {
                     m_pMessageList->Child("message", iPos - 1);
                     // EDFPrint("CmdMessageGoto child message", m_pMessageList, false);
                     while(m_pMessageList->Child("message", EDFElement::LAST) == true);
                     // EDFPrint("CmdMessageGoto new message", m_pMessageList, false);

                     bReturn = true;
                  }
                  else if(m_pMessageList->Depth() > 0)
                  {
                     bReturn = true;
                  }
               }
               /* else
               {
                  debug("CmdMessageGoto parent failed\n");
               } */
            }
         }

         /* if(bReturn == true)
         {
            m_pMessageList->Get(NULL, iMessageID);
         } */

         if(bReturn == false)
         {
            if(*iMsgPos == MSG_PREV)
            {
               CmdWrite("No previous message\n");
               *iMsgPos = MSG_EXIT;
            }
            else if(*iMsgPos == MSG_NEXT)
            {
               CmdWrite("No next message\n");
               *iMsgPos = MSG_EXIT;
            }
         }
      }
      /* else
      {
         debug("CmdMessageGoto cannot find current message\n");
      } */
   }
   else if(*iMsgPos == MSG_PARENT)
   {
      // EDFDebugPrint("CmdMessageGoto message list pos", m_pMessageList, false);
      // EDFDebugPrint("CmdMessageGoto current message", m_pMessageView, false);
      // EDFPrint("CmdMessageGoto current message", m_pMessageView, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      if(m_pMessageView != NULL && m_pMessageView->Child("replyto") == true)
      {
         // EDFDebugPrint("CmdMessageGoto reply to", m_pMessageView, false);

         m_pMessageView->Get(NULL, &iMessageID);
         // debug("CmdMessageGoto parent message ID %d\n", *iMessageID);
         m_pMessageView->GetChild("folderid", &iFolderID);
         m_pMessageView->Parent();

         bIDFromList = false;
         bReturn = true;
      }
      else
      {
         CmdWrite("No parent message to jump to\n");
         *iMsgPos = MSG_EXIT;
      }
   }
   else if(*iMsgPos > 0)
   {
      m_pMessageList->Root();
      bReturn = MessageInFolder(m_pMessageList, *iMsgPos);
      if(bReturn == false)
      {
         if(CmdVersion("2.5") >= 0)
         {
            iMessageID = *iMsgPos;
            bIDFromList = false;
            bReturn = true;
         }
         else
         {
            sprintf(szWrite, "No message \0374%d\0370 in folder\n", *iMsgPos);
            CmdWrite(szWrite);
         }
      }
      else
      {
         *iMsgPos = MSG_JUMP;
      }
   }

   if(bReturn == true)
   {
      if(iFolderID == m_iFolderID && bIDFromList == true)
      {
         m_pMessageList->Get(NULL, &iMessageID);
      }
      /* else if(bShowOnly == false)
      {
         CmdRefreshMessages(iFolderID);
         MessageInFolder(m_pMessageList, iMessageID);
      } */

      CmdMessageRequest(iFolderID, iMessageID, bShowOnly, true);
   }

   // debug("CmdMessageGoto exit %s, %d %d %d\n", BoolStr(bReturn), *iMsgPos, m_iFolderID, m_iMessageID);
   // printf("CmdMessageGoto exit %s, %d %d %d\n", BoolStr(bReturn), *iMsgPos, m_iFolderID, m_iMessageID);
   return bReturn;
}

char CmdUserPageTo(bool bReply, EDF *pReply, int iToID, bool bRetro, int iAccessLevel, EDF *pPage)
{
   STACKTRACE
   int iStatus = LOGIN_OFF, iToEDF = -1, iTimeOff = -1, iTimeDiff = 0, iTimeBusy = -1, iRetro = 0, iFromID = -1, iNumShadows = 0;
   bool bLoop = false, bDefault = true;
   char *szReply = NULL, *szStatusMsg = NULL, *szFromName = NULL, *szEmote = NULL;
   char szDate[100], szWrite[200], szLastOn[100], cOption = '\0', szPrefix1[100], szPrefix2[100];
   CmdInput *pInput = NULL;

   if(pReply != NULL)
   {
      // debugEDFPrint("CmdUserPageTo reply", pReply);
      pReply->Get(NULL, &szReply);

      pReply->GetChild("fromname", &szFromName);
      if(szFromName == NULL)
      {
         pReply->GetChild("username", &szFromName);
      }
      pReply->GetChild("status", &iStatus);
      if(stricmp(szReply, MSG_USER_CONTACT_NOCONTACT) == 0)
      {
         iStatus = LOGIN_SHADOW;
      }
      else if(iStatus == LOGIN_OFF && stricmp(szReply, MSG_USER_BUSY) == 0)
      {
         iStatus = LOGIN_BUSY;
      }
      pReply->GetChild(CmdVersion("2.5") >= 0 ? "statusmsg" : "busymsg", &szStatusMsg);
      pReply->GetChild("timeoff", &iTimeOff);
      pReply->GetChild("timebusy", &iTimeBusy);

      delete[] szReply;
   }
   else if(UserGet(m_pUserList, iToID, &szFromName, false) == true)
   {
      // CmdEDFPrint("CmdUserPageTo user", m_pUserList, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      m_pUserList->GetChild("status", &iStatus);
      if(mask(iStatus, LOGIN_ON) == false)
      {
         m_pUserList->GetChild("numshadows", &iNumShadows);
         if(iNumShadows > 0)
         {
            iStatus = LOGIN_SHADOW;
         }
      }
      m_pUserList->GetChild("statusmsg", &szStatusMsg);
      m_pUserList->GetChild("timeoff", &iTimeOff);
      m_pUserList->GetChild("timebusy", &iTimeBusy);
   }

   if(mask(iStatus, LOGIN_NOCONTACT) == true || mask(iStatus, LOGIN_SHADOW) == true)
   {
		/* sprintf(szWrite, "\0374%s\0370 cannot be paged\n", RETRO_NAME(szFromName));
      CmdWrite(szWrite); */

      sprintf(szPrefix1, "Unpageable: \0374%s\0370", RETRO_NAME(szFromName));
      sprintf(szPrefix2, "\0374%s\0370 cannot be paged", RETRO_NAME(szFromName));

      szEmote = UserEmote(szPrefix1, szPrefix2, szStatusMsg, true, '4');
      CmdWrite(szEmote);
      delete[] szEmote;
      CmdWrite("\n");
   }
   else if(mask(iStatus, LOGIN_BUSY) == true)
   {
      if(iTimeBusy != -1)
      {
         // StrTime(szBusy, STRTIME_TIME, iTimeBusy, '4', " (last active ", ")");
         StrValue(szDate, STRVALUE_TIMESU, CmdInput::MenuTime() - iTimeBusy, '4');
         sprintf(szWrite, " (last active %s ago)", szDate);
      }
      else
      {
         strcpy(szWrite, "");
      }

      /* if(szStatusMsg != NULL)
      {
         if(szStatusMsg[0] == ':')
         {
            sprintf(szWrite, "Busy%s: \0374%s\0370 %s\n", szBusy, RETRO_NAME(szFromName), (char *)&szStatusMsg[1]);
         }
         else
         {
            sprintf(szWrite, "\0374%s\0370 is busy%s. %s\n", RETRO_NAME(szFromName), szBusy, szStatusMsg);
         }
      }
      else
      {
         sprintf(szWrite, "\0374%s\0370 is busy%s\n", RETRO_NAME(szFromName), szBusy);
      }
      CmdWrite(szWrite); */

      sprintf(szPrefix1, "Busy%s: \0374%s\0370", szWrite, RETRO_NAME(szFromName));
      sprintf(szPrefix2, "\0374%s\0370 is busy%s", RETRO_NAME(szFromName), szWrite);

      szEmote = UserEmote(szPrefix1, szPrefix2, szStatusMsg, true, '4');
      CmdWrite(szEmote);
      delete[] szEmote;
      CmdWrite("\n");
   }
   else if(mask(iStatus, LOGIN_ON) == false)
   {
      if(szFromName != NULL)
      {
         sprintf(szWrite, "\0374%s\0370 is not logged in", RETRO_NAME(szFromName));
         if(iTimeOff != -1)
         {
            iTimeDiff = CmdInput::MenuTime() - iTimeOff;
            if(iTimeDiff > 0)
            {
               strcat(szWrite, " (last on ");

               if(iTimeDiff > 86400)
               {
                  StrTime(szLastOn, STRTIME_SHORT, iTimeOff, '4');
               }
               else
               {
                  szLastOn[0] = '\0';
                  StrValue(szLastOn, iTimeDiff > 60 ? STRVALUE_HM : STRVALUE_TIME, iTimeDiff, '4');
                  strcat(szLastOn, " ago");
               }
               strcat(szWrite, szLastOn);
               strcat(szWrite, ")");
            }
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);
      }
      /* else
      {
         cOption = 'x';
      } */
   }
   else if(bReply == false && pReply == NULL)
   {
      return 'y';
   }

   delete[] szStatusMsg;

   m_pPaging->Root();
   bLoop = m_pPaging->Child("page");
   while(bLoop == true)
   {
      m_pPaging->Get(NULL, &iToEDF);
      if(iToEDF == iToID)
      {
         // EDFPrint("CmdUserPageTo page history", m_pPaging, EDFElement::EL_CURR | EDFElement::PR_SPACE);

         // m_pPaging->GetChild("text", &szPrevFrom);
         // m_pPaging->GetChild("sent", &szPrevTo);
         bLoop = false;
      }
      else
      {
         bLoop = m_pPaging->Next("page");
         if(bLoop == false)
         {
            m_pPaging->Parent();
         }
      }
   }

   m_pUser->Root();
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      m_pUser->GetChild("retro", &iRetro);
      m_pUser->Parent();
   }

   while(cOption == '\0')
   {
      // CmdEDFPrint("CmdUserPageTo", m_pPaging, false);
      pInput = new CmdInput(CMD_MENU_SIMPLE | CMD_MENU_NOCASE, bReply == true && szFromName != NULL ? "Reply" : "Page");
      if(mask(iStatus, LOGIN_ON) == true && mask(iStatus, LOGIN_BUSY) == false && mask(iStatus, LOGIN_NOCONTACT) == false && mask(iStatus, LOGIN_SHADOW) == false)
      {
         if(bReply == true)
         {
            pInput->MenuAdd('y', "Yes", NULL, true);
            bDefault = false;
         }
         if(iAccessLevel >= LEVEL_MESSAGES)
         {
            pInput->MenuAdd('d', "Divert", "Post in a private folder instead");
         }
      }
      else if(mask(iStatus, LOGIN_BUSY) == true || mask(iStatus, LOGIN_NOCONTACT) == true || mask(iStatus, LOGIN_SHADOW) == true)
      {
         if(iAccessLevel >= LEVEL_MESSAGES)
         {
            pInput->MenuAdd('d', "Divert", "Post in a private folder instead", true);
         }
         bDefault = false;
         if(mask(iStatus, LOGIN_NOCONTACT) == false && mask(iStatus, LOGIN_SHADOW) == false && iAccessLevel >= LEVEL_WITNESS)
         {
            pInput->MenuAdd('o', "Override", "Ignore busy flag and page them anyway");
         }
      }
      else if(szFromName != NULL && mask(iStatus, LOGIN_ON) == false)
      {
         if(iAccessLevel >= LEVEL_MESSAGES)
         {
            pInput->MenuAdd('d', "Divert", "Post in a private folder instead", true);
         }
         bDefault = false;
      }
      if(szFromName != NULL && bReply == true)// && mask(iRetro, RETRO_PAGENO) == true)
      {
         pInput->MenuAdd('n', "No");
      }
      if(pPage != NULL)
      {
         pInput->MenuAdd('l', "List");
      }
      if(m_pPaging->IsChild("sent") == true)
      {
         pInput->MenuAdd('t', "last To", "Display the last page you sent this person");
      }
      if(m_pPaging->IsChild("text") == true)
      {
         pInput->MenuAdd('f', "last From", "Display the last page this person sent you");
      }
      if(pPage != NULL && CmdContentList(pPage, false) > 0)
      {
         pInput->MenuAdd('z', "browZe content");
      }
      pInput->MenuAdd('x', "eXit", NULL, bDefault);
      cOption = CmdMenu(pInput);
      if(cOption == 'n')
      {
         cOption = 'x';
      }
      else if(cOption == 'l')
      {
         szFromName = NULL;
         pPage->GetChild("fromname", &szFromName);
         CmdUserPageView(pPage, "From", szFromName);
         delete[] szFromName;
         cOption = '\0';
      }
      else if(cOption == 't')
      {
         CmdUserPageView(m_pPaging, "To", szFromName, NULL, "sent");
         cOption = '\0';
      }
      else if(cOption == 'f')
      {
         CmdUserPageView(m_pPaging, "From", szFromName);
         cOption = '\0';
      }
      else if(cOption == 'z')
      {
         pPage->GetChild("fromid", &iFromID);
         ContentMenu(pPage, "page", iFromID);
         cOption = '\0';
      }
   }

   // delete[] szFromName;

   return cOption;
}

bool CmdUserPageFields(EDF *pRequest)
{
   debugEDFPrint("CmdUserPageFields entry", pRequest);

   if(CmdLocal() == true)
   {
      CmdExtraFields(pRequest, "attachment", false);
   }

   debugEDFPrint("CmdUserPageFields exit", pRequest);

   return true;
}

bool CmdUserPage(EDF *pAnnounce, int iFromID, bool bCustomTo, const char *szToName)
{
   STACKTRACE
   int iAccessLevel = LEVEL_NONE, iConfirm = 0, iRetro = 0, iToID = -1, iToEDF = -1, iMessageID = -1, iFailed = 0, iServiceID = -1, iServiceType = 0;
   long iContactID = -1;
   bool bLoop = false, bFound = false; //, bMessage = true;
   char *szFromName = NULL, *szText = NULL, *szMessage = NULL, *szFilename = NULL, *szUser = NULL;
   char *szServiceUser = NULL, *szServicePassword = NULL;
   char cOption = '\0';
   char szWrite[100];
   EDF *pRequest = NULL, *pReply = NULL;

   m_pUser->Root();
   m_pUser->GetChild("accesslevel", &iAccessLevel);
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      m_pUser->GetChild("retro", &iRetro);
      m_pUser->GetChild("confirm", &iConfirm);
      m_pUser->Parent();
   }

   if(mask(iConfirm, CONFIRM_BUSY_PAGER) == true && mask(CmdInput::MenuStatus(), LOGIN_BUSY) == true && CmdYesNo("Turn your pager on", true) == true)
   {
      pRequest = new EDF();
      pRequest->Add("login");
      pRequest->AddChild("status", 0);
      pRequest->Parent();

      // EDFPrint("MainMenu busy request", pRequest, false);
      CmdRequest(MSG_USER_EDIT, pRequest);
      CmdUserReset();
   }

   if(pAnnounce != NULL)
   {
      /* if(pAnnounce->Child("message") == true)
      {
         bMessage = true;
      } */
      pAnnounce->Get(&szMessage);
      if(stricmp(szMessage, "message") == 0)
      {
         pAnnounce->Get(NULL, &iMessageID);
      }
      pAnnounce->GetChild("contactid", &iContactID);
      pAnnounce->GetChild("serviceid", &iServiceID);
      pAnnounce->GetChild("fromid", &iFromID);
      pAnnounce->GetChild("fromname", &szFromName);
      CmdUserPageView(pAnnounce, "From", szFromName);
      /* if(bMessage == true)
      {
         pAnnounce->Parent();
      } */

      cOption = CmdUserPageTo(true, NULL, iFromID, mask(iRetro, RETRO_NAMES), iAccessLevel, pAnnounce);
      if(cOption == 'x')
      {
         delete[] szFromName;

         return false;
      }

      iToID = iFromID;
   }
   else if(iFromID == -1)
   {
      if(bCustomTo == true && CmdVersion("2.6") >= 0)
      {
         m_pServiceList->Root();
         CmdEDFPrint("CmdUserPage services", m_pServiceList);

         iServiceID = CmdLineTab("Name of service", CmdServiceTab, m_pServiceList);
         if(iServiceID == -1)
         {
            CmdWrite("No such service\n");
            return false;
         }

         m_pServiceList->Root();
         if(EDFFind(m_pServiceList, "service", iServiceID, false) == true)
         {
            m_pServiceList->GetChild("servicetype", &iServiceType);

            debug("CmdUserPage service %d %d %s\n", iServiceID, iServiceType, BoolStr(m_pServiceList->GetChildBool("active")));
            if(mask(iServiceType, SERVICE_CONTACT) == false)
            {
               CmdWrite("Service is not contactable\n");
            }
            else if(m_pServiceList->GetChildBool("active") == false && mask(iServiceType, SERVICE_LOGIN) == true)
            {
               CmdWrite("Service requires login\n");

               szServiceUser = CmdLineStr("Username");
               if(szServiceUser != NULL)
               {
                  szServicePassword = CmdLineStr("Password", LINE_LEN, CMD_LINE_SILENT);
                  if(szServicePassword != NULL)
                  {
                     pRequest = new EDF();
                     pRequest->AddChild("serviceid", iServiceID);
                     pRequest->AddChild("active", true);
                     pRequest->AddChild("name", szServiceUser);
                     pRequest->AddChild("password", szServicePassword);
                     if(CmdRequest(MSG_SERVICE_SUBSCRIBE, pRequest, &pReply) == false)
                     {
                        CmdEDFPrint("CmdUserPage request failed", pReply);

                        delete pReply;

                        return false;
                     }
                     delete pReply;

                     delete[] szServicePassword;
                  }
                  else
                  {
                     delete[] szServiceUser;

                     return false;
                  }

                  delete[] szServiceUser;
               }
               else
               {
                  return false;
               }
            }
         }
         m_pServiceList->Parent();
      }
      else
      {
         iToID = CmdLineUser(bCustomTo == true ? CmdAgentLoginTab : CmdUserLoginTab);
         if(iToID == -1)
         {
            delete[] szFromName;

            // debug("CmdUserPage exit false\n");
            return false;
         }
      }

      if(bCustomTo == true)
      {
         szUser = CmdLineStr("To name", LINE_LEN);
         if(szUser == NULL)
         {
            return false;
         }
      }

      if(iToID != -1)
      {
         cOption = CmdUserPageTo(false, NULL, iToID, mask(iRetro, RETRO_NAMES), iAccessLevel, NULL);
         if(cOption == 'x')
         {
            delete[] szFromName;
            delete[] szUser;

            return false;
         }
      }
   }
   else
   {
      iToID = iFromID;
   }

   debug("CmdUserPage option '%c'\n", cOption);

   pRequest = new EDF();
   if(iServiceID != -1)
   {
      pRequest->AddChild("serviceid", iServiceID);
   }
   else if(iToID != -1)
   {
      pRequest->AddChild("toid", iToID);
   }
   if(szUser != NULL)
   {
      pRequest->AddChild("toname", szUser);
   }
   else if(szToName != NULL)
   {
      pRequest->AddChild("toname", szToName);
   }
   else if(szFromName != NULL)
   {
      pRequest->AddChild("toname", szFromName);
   }
   if(cOption == 'd')
   {
      if(CmdVersion("2.6") >= 0)
      {
         pRequest->AddChild("divert", true);
      }
      else
      {
         pRequest->AddChild("contacttype", CONTACT_POST);
      }
      if(iMessageID != -1)
      {
         pRequest->AddChild("replyid", iMessageID);
      }
      if(pAnnounce == NULL || pRequest->AddChild(pAnnounce, "subject") == false)
      {
         pRequest->AddChild("subject", "Diverted Page");
      }
   }
   else if(cOption == 'o')
   {
      pRequest->AddChild("override", true);
   }
   if(iContactID != -1)
   {
      pRequest->AddChild("contactid", iContactID);
   }

   szText = CmdText(0, NULL, CmdVersion("2.5") >= 0 ? CmdUserPageFields : NULL, CmdVersion("2.5") >= 0 ? pRequest : NULL);
   if(szText == NULL)
   {
      delete pRequest;
      delete[] szFromName;

      return false;
   }

   pRequest->AddChild("text", szText);

   // CmdEDFPrint("CmdUserPage sending", pRequest);
   while(CmdRequest(MSG_USER_CONTACT, pRequest, false, &pReply) == false)
   {
      cOption = CmdUserPageTo(false, pReply, iToID, mask(iRetro, RETRO_NAMES), iAccessLevel, NULL);
      if(cOption == 'x')
      {
         delete pRequest;
         delete[] szFromName;
         delete[] szText;

         return false;
      }
      if(cOption == 'd')
      {
         if(CmdVersion("2.6") >= 0)
         {
            pRequest->AddChild("divert", true);
         }
         else
         {
            pRequest->AddChild("contacttype", CONTACT_POST);
         }
         if(iMessageID != -1)
         {
            pRequest->AddChild("replyid", iMessageID);
         }
         if(pAnnounce == NULL || pRequest->AddChild(pAnnounce, "subject") == false)
         {
            pRequest->AddChild("subject", "Diverted Page");
         }
      }
      else if(cOption == 'o')
      {
         pRequest->SetChild("override", true);
      }
      // delete pReply;
   }

   bLoop = pReply->Child("attachment");
   while(bLoop == true)
   {
      szFilename = NULL;

      if(pReply->GetChild("failed", &iFailed) == true)
      {
         pReply->GetChild("filename", &szFilename);
         strcpy(szWrite, "Attachment");
         if(szFilename != NULL)
         {
            sprintf(szWrite, "%s \0374%s\0370", szWrite, szFilename);
            delete[] szFilename;
         }
         strcat(szWrite, " not sent");

         if(iFailed == MSGATT_FAILED)
         {
            strcat(szWrite, " (attachments not allowed)");
         }
         else if(iFailed == MSGATT_TOOBIG)
         {
            strcat(szWrite, " (attachment too large)");
         }
         else if(iFailed == MSGATT_MIMETYPE)
         {
            strcat(szWrite, " (MIME type not supported)");
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);
      }

      bLoop = pReply->Next("attachment");
      if(bLoop == false)
      {
         pReply->Parent();
      }
   }

   delete pRequest;

   if(iToID != -1)
   {
      m_pPaging->Root();
      bLoop = m_pPaging->Child("page");
      while(bLoop == true)
      {
         m_pPaging->Get(NULL, &iToEDF);
         if(iToID == iToEDF)
         {
            m_pPaging->SetChild("sent", szText);
            m_pPaging->SetChild("sentdate", CmdInput::MenuTime());
            bLoop = false;
            bFound = true;
         }
         bLoop = m_pPaging->Next("page");
         if(bLoop == false)
         {
            m_pPaging->Parent();
         }
      }

      if(bFound == false)
      {
         m_pPaging->Add("page", iToID);
         m_pPaging->AddChild("sent", szText);
         m_pPaging->AddChild("sentdate", CmdInput::MenuTime());
      }
   }

   delete[] szFromName;
   delete[] szText;

   return true;
}

void CmdOptionSet(EDF *pUser, EDF *pRequest, const char *szTitle, const char *szField, bool bPrompt, bool bDefault)
{
   STACKTRACE
   bool bOption = false, bValue = false;
   char szWrite[100];

   pUser->Root();
   // EDFPrint("CmdOptionSet client settings", pUser, EDFElement::EL_CURR | EDFElement::PR_SPACE);
   if(pUser->Child("client", CLIENT_NAME()) == false)
   {
      pUser->Add("client", CLIENT_NAME());
   }
   bOption = pUser->GetChildBool(szField, bDefault);
   if(bPrompt == true)
   {
      bValue = CmdYesNo(szTitle, bOption);
   }
   else
   {
      bValue = !bOption;
   }

   if(bValue != bOption)
   {
      pUser->SetChild(szField, bValue);

      if(bPrompt == false)
      {
         sprintf(szWrite, "%s is now \0374%s\0370\n", szTitle, bValue == true ? "on" : "off");
         CmdWrite(szWrite);
      }

      if(pRequest->Child("client", CmdVersion("2.3") >= 0 ? "edit" : "set") == false)
      {
         pRequest->Add("client", CmdVersion("2.3") >= 0 ? "edit" : "set");
      }
      pRequest->SetChild(szField, bValue);
      pRequest->Parent();
   }

   pUser->Parent();
}

void CmdValueSet(EDF *pUser, EDF *pRequest, const char *szTitle, const char *szField, int iOptions, int iMin, int iMax, int iDefault)
{
   STACKTRACE
   int iOption = 0, iValue = 0;
   // char szWrite[100];

   if(pUser->Child("client", CLIENT_NAME()) == false)
   {
      pUser->Add("client", CLIENT_NAME());
   }
   // EDFPrint("CmdOptionSet client settings", pUser, false);
   // bOption = pUser->GetChildBool(szField, bDefault);
   if(pUser->GetChild(szField, &iValue) == false && mask(iOptions, 4) == true)
   {
      iValue = iDefault;
   }
   iOption = CmdLineNum(szTitle, 0, iDefault);
   if((mask(iOptions, 1) == false || iOption >= iMin)
      && (mask(iOptions, 2) == false || iOption <= iMax))
   {
      if(iValue != iOption)
      {
         pUser->SetChild(szField, iOption);

         /* if(bPrompt == false)
         {
            sprintf(szWrite, "%s is now \0374%s\0370\n", szTitle, bValue == true ? "on" : "off");
            CmdWrite(szWrite);
         } */

         if(pRequest->Child("client", CmdVersion("2.3") >= 0 ? "edit" : "set") == false)
         {
            pRequest->Add("client", CmdVersion("2.3") >= 0 ? "edit" : "set");
         }
         pRequest->SetChild(szField, iOption);
         pRequest->Parent();
      }
   }

   pUser->Parent();
}

int CmdFolderJoin(int iFolderID, const char *szPrompt, bool bView)
{
   int iSubType = 0;
   char cOption = '\0';
   char *szReply = NULL;
   EDF *pRequest = NULL, *pReply = NULL;
   CmdInput *pInput = NULL;

   if(FolderGet(m_pFolderList, iFolderID) == false)
   {
      return 0;
   }

   m_pFolderList->GetChild("subtype", &iSubType);

   if(iSubType == 0)
   {
      if(szPrompt != NULL)
      {
         if(CmdYesNo(szPrompt, false) == false)
         {
            return 0;
         }
      }
      else
      {
         pInput = new CmdInput(CMD_MENU_SIMPLE | CMD_MENU_NOCASE, "You are not subscribed. Subscribe now");
         pInput->MenuAdd('y', "Yes");
         pInput->MenuAdd('n', "No");
         if(bView == true)
         {
            pInput->MenuAdd('v', "View only");
         }
         cOption = CmdMenu(pInput);
         if(cOption == 'n')
         {
            return 0;
         }
      }

      if(cOption != 'v' || CmdVersion("2.6") >= 0)
      {
         pRequest = new EDF();
         pRequest->AddChild("folderid", iFolderID);
         if(cOption == 'v' && CmdVersion("2.6") >= 0)
         {
            pRequest->AddChild("temp", true);
         }

         if(CmdRequest(MSG_FOLDER_SUBSCRIBE, pRequest, &pReply) == false)
         {
            pReply->Get(NULL, &szReply);

            if(CmdVersion("2.1") < 0 && stricmp(szReply, "folder_subscribed") != 0)
            {
               delete[] szReply;

               delete pReply;

               return 0;
            }

            delete[] szReply;
         }

         delete pReply;

         m_pFolderList->SetChild("subtype", SUBTYPE_SUB);
         if(cOption == 'v' && CmdVersion("2.6") >= 0)
         {
            m_pFolderList->AddChild("temp", true);
         }
      }
   }

   if(cOption == 'v')
   {
      return 2;
   }

   return 1;
}

bool CmdFolderLeave(int iFolderID)
{
   int iSubType = 0;
   bool bReturn = false;
   EDF *pRequest = NULL, *pReply = NULL;

	debug("CmdFolderLeave entry %d\n", iFolderID);

   // iSubType = CmdFolderSubType(iFolderID, false);
   FolderGet(m_pFolderList, iFolderID, NULL, false);
   m_pFolderList->GetChild("subtype", &iSubType);
   if(iSubType > 0)
   {
      if(m_pFolderList->GetChildBool("temp") == true || (CmdYesNo("Really unsubscribe", false) == true && (iSubType < SUBTYPE_MEMBER || CmdYesNo("You have privileges on this folder. Confirm unsubscribe", false) == true)))
      {
         pRequest = new EDF();
         pRequest->AddChild("folderid", iFolderID);
         if(CmdRequest(MSG_FOLDER_UNSUBSCRIBE, pRequest, &pReply) == true)
         {
            bReturn = true;
         }
      }

      if(bReturn == true)
      {
         m_pFolderList->SetChild("subtype", 0);
         m_pFolderList->DeleteChild("temp");
      }
   }
   else
   {
      bReturn = true;
   }

	debug("CmdFolderLeave exit %s\n", BoolStr(bReturn));
   return bReturn;
}

int CmdCurrFolder()
{
   return m_iFolderID;
}

void CmdCurrFolder(int iFolderID)
{
   m_iFolderID = iFolderID;
}

int CmdCurrMessage()
{
   return m_iMessageID;
}

void CmdCurrMessage(int iMessageID)
{
   m_iMessageID = iMessageID;
   debug(DEBUGLEVEL_DEBUG, "CmdCurrMessage reset message ID (%d)\n", m_iMessageID);
}

bool CmdMessageMark(int iFolderID, const char *szFolderName, int iMessageID)
{
   int iNumUnread = 0;

   if(iFolderID != -1)
   {
      if(FolderGet(m_pFolderList, iFolderID) == false)
      {
         return false;
      }
   }

   if(iMessageID != -1)
   {
      if(MessageInFolder(m_pMessageList, iMessageID) == false)
      {
         return false;
      }
   }

   if(m_pMessageList->GetChildBool("read") == false)
   {
      // EDFPrint("CmdReplyShow read field set point", m_pMessageList, false);
      m_pMessageList->SetChild("read", true);

      // EDFPrint("CmdReplyShow read field set point", m_pFolderList, false);
      m_pFolderList->GetChild("unread", &iNumUnread);
      iNumUnread--;
      if(iNumUnread >= 0)
      {
         debug(DEBUGLEVEL_DEBUG, "CmdMessageMark resetting unread folder list count in %s to %d\n", szFolderName, iNumUnread);
         m_pFolderList->SetChild("unread", iNumUnread);
      }
      else
      {
         debug(DEBUGLEVEL_WARN, "CmdMessageMark unable to set unread folder list count in %s to %d\n", szFolderName, iNumUnread);
      }
   }

   return true;
}

bool CmdChannelLeave()
{
   STACKTRACE
   int iSubType = 0;
   bool bReturn = false;
   char *szRequest = NULL;
   EDF *pRequest = NULL, *pReply = NULL;

   if(ChannelGet(m_pChannelList, m_iChannelID) == false)
   {
      return false;
   }

   m_pChannelList->GetChild("subtype", &iSubType);
   debug("CmdChannelLeave %d subtype %d\n", m_iChannelID, iSubType);

   pRequest = new EDF();
   pRequest->AddChild("channelid", m_iChannelID);

   if(iSubType >= SUBTYPE_MEMBER)
   {
      szRequest = MSG_CHANNEL_SUBSCRIBE;

      pRequest->AddChild("subtype", iSubType);
      pRequest->AddChild("active", false);
   }
   else
   {
      szRequest = MSG_CHANNEL_UNSUBSCRIBE;
   }

   bReturn = CmdRequest(szRequest, pRequest, &pReply);

   if(bReturn == true)
   {
      if(iSubType < SUBTYPE_MEMBER)
      {
         m_pChannelList->SetChild("subtype", 0);
      }
      m_pChannelList->DeleteChild("active");
   }
   else
   {
      debugEDFPrint("CmdChannelLeave request failed", pReply);
   }

   delete pReply;

   m_iChannelID = -1;

   return true;
}

bool CmdChannelJoin(int iChannelID, const char *szPrompt)
{
   STACKTRACE
   int iSubType = 0;
   EDF *pRequest = NULL, *pReply = NULL;

   if(ChannelGet(m_pChannelList, iChannelID) == false)
   {
      return false;
   }

   m_pChannelList->GetChild("subtype", &iSubType);

   if(iSubType == 0)
   {
      if(szPrompt == NULL)
      {
         szPrompt = "You are not subscribed. Subscribe now";
      }
      if(CmdYesNo(szPrompt, false) == false)
      {
         return false;
      }
   }

   pRequest = new EDF();
   pRequest->AddChild("channelid", iChannelID);
   pRequest->AddChild("active", true);

   if(CmdRequest(MSG_CHANNEL_SUBSCRIBE, pRequest, &pReply) == false)
   {
      CmdEDFPrint("CmdChannelJoin request failed", pReply);

      delete pReply;

      return false;
   }

   delete pReply;

   if(iSubType == 0)
   {
      m_pChannelList->SetChild("subtype", SUBTYPE_SUB);
   }
   m_pChannelList->SetChild("active", true);

   if(m_iChannelID != -1)
   {
      CmdChannelLeave();
   }

   m_iChannelID = iChannelID;

   return true;
}

int CmdCurrChannel()
{
   return m_iChannelID;
}
