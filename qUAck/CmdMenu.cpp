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
** CmdMenu.cpp: Implementation of menu setup functions
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>

#include "Conn/EDFConn.h"

#include "ua.h"

#include "CmdIO.h"
#include "CmdInput.h"
#include "CmdMenu.h"
#include "CmdMenuHelp.h"
#include "CmdProcess.h"
#include "CmdShow.h"

#include "client/CliFolder.h"
#include "client/CliTalk.h"
#include "client/CliUser.h"

#include "qUAck.h"
#include "GameFactory.h"

// char *m_szBrowser = NULL;
// bool m_bBrowserWait = false;

char *m_szAttachmentDir = NULL;

int /*m_iPrevFolder = -1, */ m_iMsgPos = MSG_EXIT, m_iFromID = -1;

Game *g_pGame = NULL;

bool FolderEditMenu(int iEditID);
void DefaultWholist();
char *EDFToMatchOp(EDF *pEDF, char *szOp, bool bNot, int iIndent);

bool CmdTabReset(EDF *pData, int iType)
{
   pData->Root();

   if(iType == 1)
   {
      return pData->Child("folder");
   }
   else if(iType == 2)
   {
      return pData->Child("channel");
   }
   else if(iType == 3 || iType == 4 || iType == 5)
   {
      return pData->Child("user");
   }
   else if(iType == 6)
   {
      return pData->Child("service");
   }

   return false;
}

bool CmdTabNext(EDF *pData, int iType)
{
   bool bReturn = false;

   // debugEDFPrint("CmdTabNext entry", pData, false);

   if(iType == 1)
   {
      bReturn = pData->Iterate("folder");//, "folders");
   }
   else if(iType == 2)
   {
      bReturn = pData->Next("channel");
   }
   else if(iType == 3 || iType == 4 || iType == 5)
   {
      bReturn = pData->Next("user");
   }
   else if(iType == 6)
   {
      bReturn = pData->Next("service");
   }

   // debug("CmdTabNext exit %s\n", BoolStr(bReturn));
   return bReturn;
}

bool CmdTabMatch(int iType, EDF *pData, const char *szName, const char *szMatch, int iMatchLen, bool bFull)
{
   int iUserType = 0, iStatus = LOGIN_OFF, iServiceType = 0, iNumShadows = 0;

   if(szName == NULL || szMatch == NULL)
   {
      return false;
   }

   // debug("CmdTabMatch %s", szName);
   if(iType == 4 || iType == 5)
   {
      pData->GetChild("usertype", &iUserType);
      pData->GetChild("status", &iStatus);
      pData->GetChild("numshadows", &iNumShadows);
      // debug(", %d %d", iUserType, iStatus);
   }
   else if(iType == 6)
   {
      pData->GetChild("servicetype", &iServiceType);
   }

   if(((bFull == true && stricmp(szName, szMatch) == 0) || (bFull == false && strnicmp(szName, szMatch, iMatchLen) == 0)) &&
      (iType == 1 || iType == 2 || iType == 3 ||
      (iType == 4 && (mask(iUserType, USERTYPE_DELETED) == false && (mask(iStatus, LOGIN_ON) == true) || iNumShadows > 0)) ||
      (iType == 5 && (mask(iUserType, USERTYPE_DELETED) == false && mask(iUserType, USERTYPE_AGENT) == true && mask(iStatus, LOGIN_ON) == true)) ||
      (iType == 6 && mask(iServiceType, SERVICE_CONTACT) == true)
      ))
   {
      // debug(" ** MATCH **\n");
      // debug("CmdTabMatch %s\n", szName);
      return true;
   }

   // debug("\n");
   return false;
}

char *CmdTab(int iType, EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue)
{
   STACKTRACE
   bool bRetro = false, bLoop = true, bGetCopy = pData->GetCopy(false);
   int iValue = -1, iFirst = -1;
   char *szName = NULL, *szReturn = NULL, *szFirst = NULL;

   m_pUser->TempMark();
   m_pUser->Root();
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      bRetro = CmdRetroNames(m_pUser);
   }
   m_pUser->TempUnmark();

   // debug("CmdTab entry %d, %s(%d) %s %d\n", iType, szData, iDataPos, BoolStr(bFull), *iTabValue);

   CmdTabReset(pData, iType);

   if((*iTabValue) != -1)
   {
      // debug("CmdTab find current\n");

      do
      {
         pData->Get(NULL, &iValue);
         pData->GetChild("name", &szName);

         if(CmdTabMatch(iType, pData, szName, szData, iDataPos, bFull) == true && iFirst == -1)
         {
            // debug("CmdTab first found\n");

            szFirst = strmk(szName);
            iFirst = iValue;
         }

         if(iValue == *iTabValue)
         {
            // debug("CmdTab current found\n");
            bLoop = false;
         }
         else
         {
            bLoop = CmdTabNext(pData, iType);
            if(bLoop == false)
            {
               CmdTabReset(pData, iType);
            }
         }
      }
      while(bLoop == true);

      bLoop = true;
   }

   // bLoop = CmdTabNext(pData, iType);

   while(bLoop == true && szReturn == NULL)
   {
      pData->Get(NULL, &iValue);
      pData->GetChild("name", &szName);

      if(CmdTabMatch(iType, pData, szName, szData, iDataPos, bFull) == true && iValue != *iTabValue)
      {
         szReturn = strmk(szName);
         *iTabValue = iValue;
      }
      else
      {
         bLoop = CmdTabNext(pData, iType);
      }
   }

   pData->GetCopy(bGetCopy);

   if(szReturn == NULL)
   {
      // debug("CmdTab use first\n");

      *iTabValue = iFirst;
      szReturn = szFirst;
   }
   szReturn = RETRO_NAME(szReturn);

   // debug("CmdTab exit %s, %d\n", szReturn, *iTabValue);
   return szReturn;
}

char *CmdFolderTab(EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue)
{
   return CmdTab(1, pData, szData, iDataPos, bFull, iTabValue);
}

char *CmdChannelTab(EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue)
{
   return CmdTab(2, pData, szData, iDataPos, bFull, iTabValue);
}

char *CmdUserTab(EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue)
{
   return CmdTab(3, pData, szData, iDataPos, bFull, iTabValue);
}

char *CmdUserLoginTab(EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue)
{
   return CmdTab(4, pData, szData, iDataPos, bFull, iTabValue);
}

char *CmdAgentLoginTab(EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue)
{
   return CmdTab(5, pData, szData, iDataPos, bFull, iTabValue);
}

char *CmdServiceTab(EDF *pData, const char *szData, int iDataPos, bool bFull, int *iTabValue)
{
   return CmdTab(6, pData, szData, iDataPos, bFull, iTabValue);
}

void MatchAdd(EDF *pEDF, bool *bNot, char *szCommand, char *szValue)
{
   STACKTRACE
   int iValue = 0;
   char szTemp[100];

   debug("MatchAdd entry %s %s/%s\n", BoolStr(*bNot), szCommand, szValue);

   if(stricmp(szCommand, "and") == 0 || stricmp(szCommand, "or") == 0)
   {
      debug("MatchAdd boolean\n");
      pEDF->Set(szCommand);
   }
   else if(stricmp(szCommand, "not") == 0)
   {
      debug("MatchAdd not\n");
      (*bNot) = !*(bNot);
   }
   else if(szValue != NULL && strlen(szValue) > 0)
   {
      if((*bNot) == true)
      {
         pEDF->Add("not");
      }

      if(stricmp(szCommand, "folder") == 0)
      {
         iValue = FolderGet(m_pFolderList, szValue, true);
         if(iValue > 0)
         {
            pEDF->AddChild("folderid", iValue);
         }
         else
         {
            debug("MatchAdd invalid folder %s\n", szValue);
         }
      }
      else if(stricmp(szCommand, "from") == 0 || stricmp(szCommand, "to") == 0 || stricmp(szCommand, "user") == 0)
      {
         iValue = UserGet(m_pUserList, szValue, true);
         if(iValue > 0)
         {
            if(stricmp(szCommand, "user") == 0)
            {
               pEDF->Add("or");
               pEDF->AddChild("fromid", iValue);
               pEDF->AddChild("toid", iValue);
               pEDF->Parent();
            }
            else
            {
               sprintf(szTemp, "%sid", szCommand);
               pEDF->AddChild(szTemp, iValue);
            }
         }
         else
         {
            debug("MatchAdd invalid user %s\n", szValue);
         }
      }
      else if(stricmp(szCommand, "text") == 0 || stricmp(szCommand, "subject") == 0 || stricmp(szCommand, "keyword") == 0)
      {
         pEDF->AddChild(szCommand, szValue);
      }
      else if(stricmp(szCommand, "rtext") == 0 || stricmp(szCommand, "rsubject") == 0 || stricmp(szCommand, "rkeyword") == 0)
      {
         pEDF->Add(szCommand + 1, szValue);
         pEDF->AddChild("regex", true);
         pEDF->Parent();
      }
      else
      {
         debug("MatchAdd keyword\n");
         pEDF->AddChild("keyword", szValue);
      }

      if((*bNot) == true)
      {
         pEDF->Parent();

         (*bNot) = false;
      }

      // EDFPrint("MatchAdd current", pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);
   }
   else
   {
      debug("MatchAdd doing nothing\n");
   }

   debug("MatchAdd exit\n");
}

void MatchClose(EDF *pEDF)
{
   STACKTRACE
   EDF *pTemp = NULL;
   char *szName = NULL;

   debugEDFPrint("MatchClose entry", pEDF, EDFElement::EL_CURR + EDFElement::PR_SPACE);

   if(pEDF->Children() == 0)
   {
      debug("MatchClose delete element\n");
      pEDF->Delete();
   }
   else if(pEDF->Children() == 1)
   {
      debug("MatchClose consolidate element\n");

      pEDF->Child();

      pTemp = new EDF();

      pTemp->Copy(pEDF, true, true);
      debugEDFPrint("MatchClass temp", pTemp);

      pEDF->Delete();
      pEDF->Delete();

      pEDF->Copy(pTemp);
   }
   else
   {
      debug("MatchClose move to parent\n");
      pEDF->Parent();
   }

   pEDF->Get(&szName);
   if(stricmp(szName, "not") == 0)
   {
      debug("MatchClose move not to parent\n");
      pEDF->Parent();
   }

   delete[] szName;

   debugEDFPrint("MatchClose exit", pEDF, EDFElement::EL_CURR + EDFElement::PR_SPACE);
}

EDF *MatchToEDF(char *szMatch)
{
   STACKTRACE
   int iMatchPos = 0, iStartPos = 0;
   bool bNot = false;
   char cQuote = '\0';
   char *szValue = NULL, *szCommand = NULL;
   EDF *pReturn = NULL;

   debug("MatchToEDF entry %s\n", szMatch);

   pReturn = new EDF();
   pReturn->Add("and");

   while(szMatch[iMatchPos] != '\0')
   {
      while(szMatch[iMatchPos] != '\0' && isspace(szMatch[iMatchPos]))
      {
         iMatchPos++;
      }

      if(szMatch[iMatchPos] != '\0')
      {
         debug("MatchToEDF match point %s\n", szMatch + iMatchPos);

         if(szMatch[iMatchPos] == '(')
         {
            debug("MatchToEDF open bracket\n");

            if(bNot == true)
            {
               pReturn->Add("not");
               bNot = false;
            }

            pReturn->Add("and");

            iMatchPos++;
         }
         else if(szMatch[iMatchPos] == ')')
         {
            debug("MatchToEDF close bracket\n");

            MatchClose(pReturn);

            iMatchPos++;
         }
         else
         {
            debug("MatchToEDF token\n");
            iStartPos = iMatchPos;

            if(szMatch[iMatchPos] == '\'' || szMatch[iMatchPos] == '"')
            {
               cQuote = szMatch[iMatchPos];

               iStartPos++;

               do
               {
                  iMatchPos++;
               }
               while(szMatch[iMatchPos] != '\0' && szMatch[iMatchPos] != cQuote);

               szValue = strmk(szMatch, iStartPos, iMatchPos);
               debug("MatchToEDF value %s\n", szValue);

               MatchAdd(pReturn, &bNot, "", szValue);

               delete[] szValue;
            }
            else
            {
               do
               {
                  iMatchPos++;
               }
               while(szMatch[iMatchPos] != '\0' && !isspace(szMatch[iMatchPos]) && strchr("():", szMatch[iMatchPos]) == NULL);

               if(szMatch[iMatchPos] == ':')
               {
                  debug("MatchToEDF command\n");

                  szCommand = strmk(szMatch, iStartPos, iMatchPos);

                  iStartPos = iMatchPos + 1;
                  if(szMatch[iStartPos] == '\'' || szMatch[iStartPos] == '"')
                  {
                     debug("MatchToEDF quote %c\n", szMatch[iStartPos]);
                     cQuote = szMatch[iStartPos];

                     iStartPos++;
                     iMatchPos = iStartPos;
                     do
                     {
                        iMatchPos++;
                     }
                     while(szMatch[iMatchPos] != '\0' && szMatch[iMatchPos] != cQuote);

                     szValue = strmk(szMatch, iStartPos, iMatchPos);

                     if(szMatch[iMatchPos] != '\0')
                     {
                        iMatchPos++;
                     }
                  }
                  else
                  {
                     do
                     {
                        iMatchPos++;
                     }
                     while(szMatch[iMatchPos] != '\0' && !isspace(szMatch[iMatchPos]) && strchr("():", szMatch[iMatchPos]) == NULL);

                     szValue = strmk(szMatch, iStartPos, iMatchPos);
                  }

                  debug("MatchToEDF command %s/%s\n", szCommand, szValue);

                  MatchAdd(pReturn, &bNot, szCommand, szValue);

                  delete[] szValue;
                  delete[] szCommand;
               }
               else
               {
                  szValue = strmk(szMatch, iStartPos, iMatchPos);
                  debug("MatchToEDF value %s\n", szValue);

                  MatchAdd(pReturn, &bNot, szValue, szValue);

                  delete[] szValue;
               }
            }
         }
      }
   }

   MatchClose(pReturn);

   pReturn->Root();

   debugEDFPrint("MatchToEDF exit", pReturn);

   return pReturn;
}

char *EDFToMatch(EDF *pEDF, int iIndent)
{
   STACKTRACE
   long lValue = 0;
   char *szName = NULL, *szValue = NULL, *szFolder = NULL, *szUser = NULL, *szReturn = NULL;
   char szField[100];

   debugEDFPrint("EDFToMatch entry", pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   szField[0] = '\0';

   pEDF->TypeGet(&szName, &szValue, &lValue, NULL);

   if(stricmp(szName, "folderid") == 0)
   {
      if(FolderGet(m_pFolderList, lValue, &szFolder, true) == true)
      {
         sprintf(szField, "folder:%s%s%s", strchr(szFolder, ' ') != NULL ? "\"" : "", szFolder, strchr(szFolder, ' ') != NULL ? "\"" : "");
         delete[] szFolder;
      }
   }
   else if(stricmp(szName, "fromid") == 0)
   {
      if(UserGet(m_pUserList, lValue, &szUser, true) == true)
      {
         sprintf(szField, "from:%s%s%s", strchr(szUser, ' ') != NULL ? "\"" : "", szUser, strchr(szUser, ' ') != NULL ? "\"" : "");
         delete[] szUser;
      }
   }
   else if(stricmp(szName, "toid") == 0)
   {
      if(UserGet(m_pUserList, lValue, &szUser, true) == true)
      {
         sprintf(szField, "to:%s%s%s", strchr(szUser, ' ') != NULL ? "\"" : "", szUser, strchr(szUser, ' ') != NULL ? "\"" : "");
         delete[] szUser;
      }
   }
   else if(stricmp(szName, "subject") == 0 || stricmp(szName, "text") == 0)
   {
      sprintf(szField, "%s:%s%s%s", szName, strchr(szValue, ' ') != NULL ? "\"" : "", szValue, strchr(szValue, ' ') != NULL ? "\"" : "");
   }
   else if(stricmp(szName, "keyword") == 0)
   {
      sprintf(szField, "%s%s%s", strchr(szValue, ' ') != NULL ? "\"" : "", szValue, strchr(szValue, ' ') != NULL ? "\"" : "");
   }
   else if(stricmp(szName, "and") == 0 || stricmp(szName, "or") == 0)
   {
      szReturn = EDFToMatchOp(pEDF, szName, false, iIndent + 1);
   }
   else if(stricmp(szName, "not") == 0)
   {
      szReturn = EDFToMatchOp(pEDF, "and", true, iIndent + 1);
   }

   if(szReturn == NULL && strlen(szField) > 0)
   {
      szReturn = strmk(szField);
   }

   debug("EDFToMatch exit %s\n", szReturn);
   return szReturn;
}

char *EDFToMatchOp(EDF *pEDF, char *szOp, bool bNot, int iIndent)
{
   STACKTRACE
   bool bLoop = false;
   int iNumFields = 0;
   char *szMatch = NULL, *szReturn = NULL;
   bytes *pMatch = NULL;

   debug("EDFToMatchOp entry %s %s %d:\n", szOp, BoolStr(bNot), iIndent);
   debugEDFPrint(pEDF, false, false);

   pMatch = new bytes("");

   bLoop = pEDF->Child();
   while(bLoop == true)
   {
      szMatch = EDFToMatch(pEDF, iIndent);
      if(szMatch != NULL)
      {
         debug("EDFToMatchOp adding match %s\n", szMatch);

         if(iNumFields >= 1)
         {
            pMatch->Append(" ");
            pMatch->Append(szOp);
            pMatch->Append(" ");
         }
         pMatch->Append(szMatch);

         iNumFields++;
      }

      bLoop = pEDF->Next();
      if(bLoop == false)
      {
         pEDF->Parent();
      }
   }

   if(iNumFields >= 1)
   {
      debug("EDFToMatchOp %d fields, %d indent\n", iNumFields, iIndent);

      szReturn = new char[pMatch->Length() + 10];

      szReturn[0] = '\0';
      if(bNot == true)
      {
         debug("EDFToMatchOp adding not\n");
         strcat(szReturn, "not");
      }
      if(iNumFields >= 2 && iIndent >= 2)
      {
         strcat(szReturn, "(");
      }
      else if(bNot == true && iNumFields == 1)
      {
         strcat(szReturn, " ");
      }
      strcat(szReturn, (char *)pMatch->Data(false));
      if(iNumFields >= 2 && iIndent >= 2)
      {
         strcat(szReturn, ")");
      }
   }

   delete pMatch;

   debug("EDFToMatchOp exit %s\n", szReturn);
   return szReturn;
}

char *EDFToMatch(EDF *pEDF)
{
   return EDFToMatchOp(pEDF, "and", false, 0);
}

CmdInput *CmdMain(int iAccessLevel, int iNumFolders, bool bDevOption, bool bPaging)
{
   STACKTRACE
   CmdInput *pInput = NULL;

   // debug("CmdMenuMain %d %d\n", iAccessLevel, iNumFolders);

   pInput = new CmdInput(CMD_MENU_TIME, "Main");
   if(iAccessLevel >= LEVEL_WITNESS || bDevOption == true)
   {
      pInput->MenuAdd('a', "Administration", HELP_MAIN_A);
   }
   if(mask(CmdInput::MenuStatus(), LOGIN_SHADOW) == false)
   {
      pInput->MenuAdd('b', "toggle Busy", HELP_MAIN_B);
      if(CmdVersion("2.5") >= 0 && CmdInput::MenuCase() == true)
      {
         pInput->MenuAdd('B', "Busy menu", HELP_MAIN_B);
      }
   }
   if(iAccessLevel >= LEVEL_MESSAGES)
   {
      pInput->MenuAdd('c', "who is logged in (Custom)", HELP_MAIN_C);
      pInput->MenuAdd('d', "personal Details", HELP_MAIN_D);
   }
   pInput->MenuAdd('f', "reFresh lists");
   pInput->MenuAdd('g', "loGout", HELP_MAIN_G);
   pInput->MenuAdd('h', "Help system");
   /* if(bPaging == true)
   {
      pInput->MenuAdd('i', "show Ignored page");
   } */
   pInput->MenuAdd('k', "larK about");
   if(mask(CmdInput::MenuStatus(), LOGIN_SHADOW) == false)
   {
      pInput->MenuAdd('o', "page histOry");
      pInput->MenuAdd('p', "Page", HELP_MAIN_P);
      if(CmdInput::MenuCase() == true)
      {
         if(CmdVersion("2.6") >= 0)
         {
            pInput->MenuAdd('P', "Page service", HELP_MAIN_P);
         }
         else
         {
            pInput->MenuAdd('P', "Page agent (custom to field)", HELP_MAIN_P);
         }
      }
   }
   if(iAccessLevel >= LEVEL_MESSAGES)
   {
      pInput->MenuAdd('q', "Quit", HELP_MAIN_Q);
   }
   if(iAccessLevel >= LEVEL_MESSAGES)
   {
      pInput->MenuAdd('s', "Show last few logins", HELP_MAIN_S);
   }
   if(CmdVersion("2.5") < 0 || CmdVersion("2.6") >= 0)
   {
      pInput->MenuAdd('t', "Join Talk", HELP_MAIN_T);
      if(CmdInput::MenuCase() == true)
      {
         pInput->MenuAdd('T', "Talk channels", HELP_MAIN_T);
      }
   }
   pInput->MenuAdd('u', "User directory", HELP_MAIN_U);
   pInput->MenuAdd('w', "Who is logged in", HELP_MAIN_W);
   pInput->MenuAdd('y', "view sYstem bulletins", HELP_MAIN_Y);
   /* else if(bDevOption == true)
   {
      pInput->MenuAdd('a', "informAtion", HELP_MAIN_A);
   } */
   // if(iNumFolders > 0)
   {
      if(iAccessLevel >= LEVEL_MESSAGES)
      {
         pInput->MenuAdd('e', "Enter a message", HELP_MAIN_E);
      }
      if(CmdVersion("2.5") >= 0)
      {
         pInput->MenuAdd('j', "Jump to folder / message", HELP_MAIN_J);
         if(CmdInput::MenuCase() == true)
         {
            pInput->MenuAdd('J', "Jump to message (show only)", HELP_MAIN_J);
         }
      }
      else
      {
         pInput->MenuAdd('j', "Join folder", HELP_MAIN_J);
      }
      pInput->MenuAdd('l', "List of folders", HELP_MAIN_L);
      // if(CmdVersion("2.5") >= 0)
      {
         pInput->MenuAdd('m', "Messages");
      }
      /* else
      {
         pInput->MenuAdd('m', "Mass catch-up", HELP_MAIN_M);
      } */
      pInput->MenuAdd('n', "read New messages", HELP_MAIN_N);
      pInput->MenuDefault('n');
   }

   if(CmdCurrChannel() != -1)
   {
      pInput->MenuValue('/');
   }

   return pInput;
}

CmdInput *CmdFolder(int iCurrID, int iAccessLevel, int iNewMsgs, char *szFolderName, int iSubType, int iCurrMessage, int iFromID, int iToID, int iAccessMode, int iUserID, int iMsgType, int iVoteType, int iVoteID, bool bAnnotated, int iNavMode, bool bContent, bool bRetro, bool bOldMenus, bool bDevOption)
{
   STACKTRACE
   char szWrite[200];
   CmdInput *pInput = NULL;

   // debug("CmdFolder %s %d\n", szFolderName, iNewMsgs);

   if(iNewMsgs > 0)
   {
      sprintf(szWrite, "[\037%c%s\0370] \0374%d\0370 new message%s", iSubType == SUBTYPE_EDITOR ? '1' : '4', RETRO_NAME(szFolderName), iNewMsgs, iNewMsgs != 1 ? "s" : "");
   }
   else
   {
      sprintf(szWrite, "[\037%c%s\0370]", iSubType == SUBTYPE_EDITOR ? '2' : '4', RETRO_NAME(szFolderName));
   }
   pInput = new CmdInput(CMD_MENU_TIME, szWrite);
   pInput->MenuAdd('c', "Catch-up", HELP_FOLDER_C);
   if(iAccessLevel >= LEVEL_MESSAGES)
   {
      pInput->MenuAdd('e', "Enter a new message", HELP_FOLDER_E);
   }
   if(bOldMenus == true)
   {
      pInput->MenuAdd('g', "Goto a numbered message", HELP_FOLDER_G);
   }
   pInput->MenuAdd('h', "Hold", HELP_FOLDER_H);
   pInput->MenuAdd('i', "folder Information", HELP_FOLDER_I);
   if(bOldMenus == false)
   {
      pInput->MenuAdd('j', "Jump", HELP_FOLDER_J);
      if(CmdVersion("2.5") >= 0 && CmdInput::MenuCase() == true)
      {
         pInput->MenuAdd('J', "Jump (show only)", HELP_FOLDER_J);
      }
   }
   if(bOldMenus == true)
   {
      pInput->MenuAdd('o', "whO is logged in", HELP_FOLDER_O);
   }
   pInput->MenuAdd('s', "Search messages", HELP_FOLDER_S);
   pInput->MenuAdd('u', "Unsubscribe", HELP_FOLDER_U);
   /* if(iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR)
   {
      pInput->MenuAdd('w', "Write info file", HELP_FOLDER_W);
   } */
   if(bOldMenus == false)
   {
      pInput->MenuAdd('w', "Who is logged in");
   }
   pInput->MenuAdd('x', "eXit", HELP_FOLDER_X, true);

   if(iCurrMessage != -1)
   {
      if(bOldMenus == false)
      {
         pInput->MenuAdd('a', "pArent message");
         if(CmdInput::MenuCase() == true)
         {
            pInput->MenuAdd('A', "pArent message (show only)");
         }
      }
      if(CmdInput::MenuCase() == true)
      {
         // pInput->MenuAdd('C', "stay Caught up");
      }
      if(iFromID != 0)
      {
         if(mask(CmdInput::MenuStatus(), LOGIN_SHADOW) == false && bOldMenus == false)
         {
            pInput->MenuAdd('o', "cOntact (page) user", HELP_FOLDER_O);
         }
         // pInput->MenuAdd('y', "view senders user directorY entry", HELP_FOLDER_Y);
         if(iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR || (bAnnotated == false && (iFromID == iUserID || iToID == iUserID)))
         {
            pInput->MenuAdd('d', "aDditional note", HELP_FOLDER_D);
         }
      }
      pInput->MenuAdd('b', "Backwards one message", HELP_FOLDER_B);
      if(bOldMenus == false && CmdInput::MenuCase() == true)
      {
         pInput->MenuAdd('B', "Backwards one message (show only)", HELP_FOLDER_B);
      }
      pInput->MenuAdd('f', "Forwards one message", HELP_FOLDER_F);
      if(bOldMenus == false && CmdInput::MenuCase() == true)
      {
         pInput->MenuAdd('F', "Forwards one message (show only)", HELP_FOLDER_F);
      }
      // pInput->MenuAdd('h', "Hold message", HELP_FOLDER_H);
      if(iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR)
      {
         pInput->MenuAdd('k', "Kill message / thread", HELP_FOLDER_K);
         // pInput->MenuAdd('m', "Move thread", HELP_FOLDER_M);
      }
      else if(mask(iAccessMode,  FOLDERMODE_SUB_ADEL) == true || mask(iAccessMode, FOLDERMODE_MEM_ADEL) == true)
      {
         pInput->MenuAdd('k', "Kill message / thread", HELP_FOLDER_K);
      }
      else if(iFromID == iUserID && (mask(iAccessMode, FOLDERMODE_SUB_SDEL) == true || mask(iAccessMode, FOLDERMODE_MEM_SDEL) == true))
      {
         pInput->MenuAdd('k', "Kill message", HELP_FOLDER_K);
      }
      if(iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR || mask(iAccessMode, FOLDERMODE_SUB_MOVE) == true || mask(iAccessMode, FOLDERMODE_MEM_MOVE) == true)
      {
         pInput->MenuAdd('t', "Transfer message / thread", HELP_FOLDER_T);
      }
      if(bOldMenus == true)
      {
         pInput->MenuAdd('j', "Jump to previous", HELP_FOLDER_J);
      }
      pInput->MenuAdd('l', "List current message", HELP_FOLDER_L);
      if(CmdInput::MenuCase() == true && (iAccessLevel >= LEVEL_WITNESS || bDevOption == true))
      {
         pInput->MenuAdd('L', "List current message (raw EDF)", HELP_FOLDER_L);
      }
      if(bOldMenus == true)
      {
         if(iNavMode == MSG_NEW)
         {
            if(iNewMsgs > 0)
            {
               pInput->MenuAdd('n', "Next new message", HELP_FOLDER_N, true);
            }
            else
            {
               pInput->MenuAdd('n', "Next message (thread mode)", HELP_FOLDER_N);
            }
         }
         else
         {
            pInput->MenuAdd('n', "Next message (thread mode)", HELP_FOLDER_N);
         }
      }
      else
      {
         if(iNewMsgs > 0)
         {
            pInput->MenuAdd('g', "Goto next new message", HELP_FOLDER_G, true);
         }
         pInput->MenuAdd('n', "Next message", HELP_FOLDER_N);
         if(bOldMenus == false && CmdInput::MenuCase() == true)
         {
            pInput->MenuAdd('N', "Next message (show only)", HELP_FOLDER_N);
         }
      }
      pInput->MenuAdd('p', "Previous message", HELP_FOLDER_P);
      if(bOldMenus == false && CmdInput::MenuCase() == true)
      {
         pInput->MenuAdd('P', "Previous message (show only)", HELP_FOLDER_P);
      }
      if(iAccessLevel >= LEVEL_MESSAGES)
      {
         pInput->MenuAdd('r', "Reply to message", HELP_FOLDER_R);
      }
      if((CmdVersion("2.5") >= 0 && mask(iMsgType, MSGTYPE_VOTE) == true) ||
         (CmdVersion("2.5") < 0 && iMsgType > 0))
      {
         if(mask(iVoteType, VOTE_CLOSED) == false && (iVoteID == -1 || (iVoteID > 0 && (mask(iVoteType, VOTE_CHANGE) == true || mask(iVoteType, VOTE_MULTI) == true))))
         {
            pInput->MenuAdd('v', "Vote", HELP_FOLDER_V);
         }
         // if(mask(iVoteType, VOTE_CLOSED) == false && (iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR) && CmdInput::MenuCase() == true)
         debug("CmdFolder %s %d %d %d\n", BoolStr(mask(iVoteType, VOTE_CLOSED)), iAccessLevel, iCurrID, iFromID);
         if(mask(iVoteType, VOTE_CLOSED) == false && (iAccessLevel >= LEVEL_WITNESS || iCurrID == iFromID) && CmdInput::MenuCase() == true)
         {
            pInput->MenuAdd('V', "close Vote");
         }
      }
      if(CmdLocal() == true)
      {
         pInput->MenuAdd('y', "copY message to file", HELP_FOLDER_Y);
         if(bContent == true)
         {
            pInput->MenuAdd('z', "browZe content");
         }
      }
   }
   else
   {
      /* if(iNewMsgs > 0)
      {
         pInput->MenuAdd('h', "Hold folder", HELP_FOLDER_H);
      } */
      if(bOldMenus == true)
      {
         if(iNewMsgs > 0)
         {
            if(iNavMode == MSG_NEW)
            {
               pInput->MenuAdd('n', "Next folder", HELP_FOLDER_N);
            }
            pInput->MenuAdd('r', "Read new messages", HELP_FOLDER_R, true);
         }
      }
      else
      {
         if(iNewMsgs > 0)
         {
            pInput->MenuAdd('g', "Goto next new message", HELP_FOLDER_G, true);
         }
      }
   }

   // pInput->MenuValue('#');

   switch(iNavMode)
   {
      case MSG_BACK:
         pInput->MenuDefault('b');
         break;

      case MSG_FORWARD:
         pInput->MenuDefault('f');
         break;

      case MSG_PREV:
         pInput->MenuDefault('p');
         break;

      case MSG_NEXT:
         pInput->MenuDefault('n');
         break;

      case MSG_PARENT:
         if(bOldMenus == true)
         {
            pInput->MenuDefault('j');
         }
         else
         {
            pInput->MenuDefault('a');
         }
         break;

      case MSG_JUMP:
         if(bOldMenus == true)
         {
            pInput->MenuDefault('n');
         }
         else
         {
            pInput->MenuDefault('j');
         }
         break;

      case MSG_EXIT:
         pInput->MenuDefault('x');
         break;
   }

   if(CmdCurrChannel() != -1)
   {
      pInput->MenuValue('/');
   }

   return pInput;
}

CmdInput *CmdAdmin(int iAccessLevel)
{
   STACKTRACE
   CmdInput *pInput = NULL;

   pInput = new CmdInput(CMD_MENU_TIME | CMD_MENU_NOCASE, "Admin");
   if(CmdVersion("2.5") >= 0 && iAccessLevel >= LEVEL_WITNESS)
   {
      pInput->MenuAdd('b', "enter Bulletin");
   }
   pInput->MenuAdd('a', "Add user");
   pInput->MenuAdd('e', "Edit user");
   pInput->MenuAdd('l', "Logout user");
   pInput->MenuAdd('q', "direct reQuest");
   if(iAccessLevel >= LEVEL_WITNESS)
   {
      pInput->MenuAdd('c', "Create folder");
      pInput->MenuAdd('h', "Host / locations");
      pInput->MenuAdd('i', "Set idle time");
      if(CmdVersion("2.5") >= 0)
      {
         pInput->MenuAdd('k', "tasK scheduler");
      }
      pInput->MenuAdd('m', "Modify folder");
      pInput->MenuAdd('n', "replace baNner");
      pInput->MenuAdd('o', "drOp connection");
      pInput->MenuAdd('r', "connection secuRity");
      pInput->MenuAdd('s', "System message");
      if(iAccessLevel >= LEVEL_SYSOP)
      {
         pInput->MenuAdd('t', "Transfer sysop");
      }
      pInput->MenuAdd('u', "shUtdown server");
      pInput->MenuAdd('v', "serVer library reload");
      pInput->MenuAdd('w', "Write data");
      pInput->MenuAdd('y', "sYstem maintenance");
   }

   pInput->MenuAdd('x', "eXit");

   if(CmdCurrChannel() != -1)
   {
      pInput->MenuValue('/');
   }

   return pInput;
}

CmdInput *CmdFolderEdit(int iFolderID, int iFolderMode, int iAccessLevel, int iSubType)
{
   STACKTRACE
   // int iAccessLevel = LEVEL_NONE;
   CmdInput *pInput = NULL;

   /* m_pUser->Root();
   m_pUser->GetChild("accesslevel", &iAccessLevel);

   iSubType = CmdFolderSubType(iFolderID); */

   pInput = new CmdInput(CMD_MENU_NOCASE, "Edit");
   if(iAccessLevel >= LEVEL_WITNESS)
   {
      pInput->MenuAdd('a', "Access level");
      pInput->MenuAdd('c' ,"aCcess mode");
      pInput->MenuAdd('d', "Default replies");
      pInput->MenuAdd('e', "add Editor");
      pInput->MenuAdd('i', "replIes per message");
      pInput->MenuAdd('k', "Kill");
   }
   pInput->MenuAdd('l', "List");
   if(mask(iFolderMode, ACCMODE_MEM_READ) == true && (iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR))
   {
      pInput->MenuAdd('m', "add Member");
      // pInput->MenuAdd('b', "remove memBer");
   }
   if(iAccessLevel >= LEVEL_WITNESS)
   {
      pInput->MenuAdd('n', "Name");
      pInput->MenuAdd('p', "Parent");
   }
   else if(mask(iFolderMode, ACCMODE_SUB_READ) == true && iAccessLevel >= LEVEL_WITNESS)
   {
      pInput->MenuAdd('s', "add Subscriber");
      // pInput->MenuAdd('b', "remove subscriBer");
      pInput->MenuAdd('r', "Remove user");
   }
   pInput->MenuAdd('w', "Write info file");
   pInput->MenuAdd('x', "eXit");
   if(iAccessLevel >= LEVEL_WITNESS)
   {
      pInput->MenuAdd('y', "expirY time");
   }

   return pInput;
}

CmdInput *CmdUserEdit(EDF *pUser, int iUserID, int iAccessLevel, int iEditID)
{
   STACKTRACE
   int iUserType = USERTYPE_NONE, iStatus = LOGIN_OFF, iAnnType = 0, iHardWrap = 0;
   bool bSigFilter = false, bDoubleReturn = false, bDevOption = false, bRetro = false, bBrowse = true;
   char szWrite[200];
   CmdInput *pInput = NULL;

   pUser->GetChild("usertype", &iUserType);
   if(pUser->Child("login") == true)
   {
      pUser->GetChild("status", &iStatus);
      pUser->Parent();
   }
   if(iEditID == -1)
   {
      if(pUser->Child("client", CLIENT_NAME()) == true)
      {
         pUser->GetChild("anntype", &iAnnType);

         bSigFilter = pUser->GetChildBool("sigfilter");
         // bHardWrap = pUser->GetChildBool("hardwrap");
         pUser->GetChild("hardwrap", &iHardWrap);
         bDoubleReturn = pUser->GetChildBool("doublereturn");

         bDevOption = pUser->GetChildBool("devoption");
         bRetro = CmdRetroNames(pUser);
         if(CmdBrowser() != NULL)
         {
            bBrowse = pUser->GetChildBool("browse", true);
         }

         // bFakePage = pUser->GetChildBool("fakepage");

         pUser->Parent();
      }
   }

   pInput = new CmdInput(0, (char *)(iEditID == -1 ? "Details" : "Edit"));

   if(iEditID == -1)
   {
      // sprintf(szWrite, "toggle hArd line wrap (currently \0374%s\0370)", bHardWrap == true ? "on" : "off");
      // pInput->MenuAdd('a', szWrite);
      pInput->MenuAdd('a', "hArd line wrap settings");
      /* sprintf(szWrite, "toggle page Bell (currently \0374%s\0370)", mask(iAnnType, ANN_PAGEBELL) == true ? "on" : "off");
      pInput->MenuAdd('b', szWrite);
      sprintf(szWrite, "toggle busy Checking (currently \0374%s\0370)", mask(iAnnType, ANN_BUSYCHECK) == true ? "on" : "off");
      pInput->MenuAdd('c', szWrite); */
   }
   else if(iAccessLevel >= LEVEL_WITNESS && mask(iUserType, USERTYPE_AGENT) == false && iUserID != iEditID)
   {
      pInput->MenuAdd('a', "Access level");
   }
   if(iEditID == -1)
   {
      pInput->MenuAdd('c', "Confirmations");
   }
   else if(iAccessLevel >= LEVEL_WITNESS)
   {
      pInput->MenuAdd('c', "aCcess name");
   }
   pInput->MenuAdd('d', "personal Details");
   if(iEditID == -1)
   {
      pInput->MenuAdd('e', "highlight Effect");
      pInput->MenuAdd('f', "deFault wholist", "Change the type of wholist you see when chosing 'W' from the main menu");
   }
   else if(iAccessLevel >= LEVEL_WITNESS)
   {
      pInput->MenuAdd('f', "Folders");
   }
   /* if(iEditID == -1)
   {
      sprintf(szWrite, "toggle Folder checking (currently \0374%s\0370)", mask(iAnnType, ANN_FOLDERCHECK) == true ? "on" : "off");
      pInput->MenuAdd('f', szWrite);
   } */
   pInput->MenuAdd('g', "Gender");
   if(iEditID == -1) // && CmdIOType() == 2)
   {
      pInput->MenuAdd('h', "screen Height");
      pInput->MenuAdd('w', "screen Width");
   }
   if(iEditID == -1)
   {
      pInput->MenuAdd('i', "dIsplay options");
   }
   if(iEditID == -1)
   {
      pInput->MenuAdd('k', "message marKing");
   }
   else if(iAccessLevel >= LEVEL_WITNESS && iUserID != iEditID)
   {
      pInput->MenuAdd('k', "Kill user");
   }
   pInput->MenuAdd('l', "List details");
   if(iEditID == -1)
   {
      pInput->MenuAdd('m', "Menu level");
   }
   if(iAccessLevel >= LEVEL_WITNESS)
   {
      pInput->MenuAdd('n', "Name");
   }
   if(iEditID == -1 && iAccessLevel >= LEVEL_WITNESS)
   {
      pInput->MenuAdd('v', "Validate details");
   }
   if(iEditID == -1)
   {
      // sprintf(szWrite, "toggle retrO mode (currently \0374%s\0370)", bRetro == true ? "on" : "off");
      // pInput->MenuAdd('o', szWrite);
      pInput->MenuAdd('o', "retrO options");
   }
   else if(iAccessLevel >= LEVEL_WITNESS && iUserID != iEditID)
   {
      pInput->MenuAdd('o', "Owner");
   }
   pInput->MenuAdd('p', "Password");
   if(iEditID == -1)
   {
      pInput->MenuAdd('r', "new message foldeR navigation");
      // sprintf(szWrite, "toggle instant pRivate messages (currently \0374%s\0370)", bFakePage == true ? "on" : "off");
      // pInput->MenuAdd('r', szWrite);
      // sprintf(szWrite, "toggle header / footer extra Return (currently \0374%s\0370)", bDoubleReturn == true ? "on" : "off");
      // pInput->MenuAdd('r', szWrite);
      sprintf(szWrite, "toggle sig filTer (currently \0374%s\0370)", bSigFilter == true ? "on" : "off");
      pInput->MenuAdd('t', szWrite);
      /* sprintf(szWrite, "toggle User checking (currently \0374%s\0370)", mask(iAnnType, ANN_USERCHECK) == true ? "on" : "off");
      pInput->MenuAdd('u', szWrite); */
   }
   else if(iAccessLevel >= LEVEL_WITNESS && iUserID != iEditID)
   {
      pInput->MenuAdd('t', "user Type");
   }
   if(iEditID == -1)
   {
      pInput->MenuAdd('s', "alertS");
      pInput->MenuAdd('u', "mass catchUp");
   }
   if(iEditID == -1 && iAccessLevel < LEVEL_WITNESS)
   {
      sprintf(szWrite, "toggle deVeloper option (currently \0374%s\0370)", bDevOption == true ? "on" : "off");
      pInput->MenuAdd('v', szWrite);
   }
   pInput->MenuAdd('x', "eXit");
   pInput->MenuAdd('y', "alter user directorY entry");
   if(iEditID == -1 && CmdBrowser() != NULL)
   {
      sprintf(szWrite, "toggle browZe URLs (currently \0374%s\0370)", bBrowse == true ? "on" : "off");
      pInput->MenuAdd('z', szWrite);
   }

   return pInput;
}

void CmdReplyShow(EDF *pReply)
{
   CmdEDFPrint("CmdReplyShow", pReply);
}

int CmdAnnounceShow(EDF *pAnnounce, const char *szReturn)
{
   STACKTRACE
   int iMessageID = 0, iFolderID = 0, iLStatus = LOGIN_OFF, iToID = 0, iFromID = 0, iCurrID = -1;
   int iInterval = 0, iStatus = LOGIN_OFF, iTime = 0, iValue = 0, iNumMsgs = 0, iTotalMsgs = 0, iCurrLevel = LEVEL_NONE;
   int iSubType = 0, iNumKeeps = 0, iMsgType = 0, iColourLen = 0, iWriteTime = 0, iReturn = CMD_REDRAW, iMarkType = 0, iMarked = 0;
   int iReloadTime = 0, iNumVotes = 0, iTotalVotes = 0, iAccessMode = FOLDERMODE_NORMAL, iConnID = 0, iMoveType = 0, iUnread = 0, iUserID = 0;
   bool bUserCheck = false, bFolderCheck = false, bAdminCheck = false, bForce = false, bDbgCheck = false, bBell = false;
   bool bRetro = false, bLoop = false, bExtraCheck = false, bFirst = true, bFakePage = false, bStatus = false, bLost = false, bAction = true, bDevOption = false;
   char szTime[100], szWrite[200], cColour = '6';
   char *szMessage = NULL, *szHostname = NULL, *szLocation = NULL, *szBy = NULL, *szText = NULL;
   char *szUser = NULL, *szClient = NULL, *szFolder = NULL, *szMove = NULL, *szSubject = NULL, *szChannel = NULL;
   char szEmote1[200], szEmote2[200], *szTemp = NULL;
   EDF *pRequest = NULL, *pReply = NULL;

   // debug("CmdAnnounceShow entry\n");
   // printf("CmdAnnounceShow %p %p\n", pAnnounce, szReturn);
   // debugEDFPrint("CmdAnnounceShow", pAnnounce);
   // CmdEDFPrint("CmdAnnounceshow", pAnnounce);

   m_pUser->Root();
   m_pUser->Get(NULL, &iCurrID);
   m_pUser->GetChild("accesslevel", &iCurrLevel);
   // m_pUser->GetChild("currfolder", &iCurrFolder);
   // m_pUser->GetChild("currmessage", &iCurrMessage);
   if(m_pUser->Child("login") == true)
   {
      m_pUser->GetChild("status", &iLStatus);
      m_pUser->Parent();
   }
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      m_pUser->GetChild("anntype", &iValue);
      bFolderCheck = mask(iValue, ANN_FOLDERCHECK);
      bUserCheck = mask(iValue, ANN_USERCHECK);
      bAdminCheck = mask(iValue, ANN_ADMINCHECK);
      bDbgCheck = mask(iValue, ANN_DEBUGCHECK);
      bExtraCheck = mask(iValue, ANN_EXTRACHECK);
      bBell = mask(iValue, ANN_PAGEBELL);
      bRetro = CmdRetroNames(m_pUser);
      if(mask(iLStatus, LOGIN_BUSY) == true && mask(iValue, ANN_BUSYCHECK) == false)
      {
         bFolderCheck = false;
         bUserCheck = false;
         bAdminCheck = false;
         bDbgCheck = false;
         bBell = false;
      }

      bFakePage = m_pUser->GetChildBool("fakepage");
      bDevOption = m_pUser->GetChildBool("devoption");

      m_pUser->Parent();
   }

   STACKTRACEUPDATE

   // CmdEDFPrint("CmdAnnounceshow", pAnnounce);
   pAnnounce->Get(NULL, &szMessage);
   // debug("CmdAnnounceShow %s\n", szMessage);
   // printf("CmdAnnounceShow %s\n", szMessage);
   /* sprintf(szWrite, "CmdAnnounceShow entry %s", szMessage);
   CmdWrite(szWrite);
   CmdEDFPrint("", pAnnounce, false); */

   if(szMessage != NULL)
   {
      strcpy(szWrite, "");
      if(bDbgCheck == true)
      {
         // CmdEDFPrint(NULL, pAnnounce, true, true);

         sprintf(szWrite, "%sAnnouncement \0376%s\0370\n", szReturn, szMessage);
         CmdWrite(szWrite);
         szReturn = "";

         CmdEDFPrint(NULL, pAnnounce, true, false);
         /* byte *pEDF = NULL;
         pAnnounce->Write(&pEDF, false, false, true, true);
         CmdWrite(pEDF);
         if(strcmp((char *)pEDF, "") != 0)
         {
            CmdWrite("\n");
         }
         delete[] pEDF; */
      }
      /* if(stricmp(szMessage, "") == 0)
      {
      }
      else */
      if(bAdminCheck == true && stricmp(szMessage, MSG_SYSTEM_WRITE) == 0)
      {
         pAnnounce->GetChild("writetime", &iWriteTime);

         sprintf(szWrite, "%sSystem write complete (\0376%d\0370 ms)\n", szReturn, iWriteTime);
         CmdWrite(szWrite);
      }
      else if(stricmp(szMessage, MSG_SYSTEM_MESSAGE) == 0)
      {
         pAnnounce->GetChild("fromname", &szUser);
         pAnnounce->GetChild("text", &szText);

         CmdPageOn();
         sprintf(szWrite, "%s%s** \0371SYSTEM MESSAGE\0370 **\nFrom: \0371%s\0370\n\n", szReturn, DASHES, RETRO_NAME(szUser));
         CmdWrite(szWrite);
         CmdWrite(szText, CMD_OUT_NOHIGHLIGHT);
         sprintf(szWrite, "\n%s\n", DASHES);
         CmdWrite(szWrite);
         CmdPageOff();

         delete[] szUser;
         delete[] szText;
      }
      else if(stricmp(szMessage, MSG_SYSTEM_SHUTDOWN) == 0)
      {
         pAnnounce->GetChild("interval", &iInterval);
         pAnnounce->GetChild("fromname", &szUser);
         pAnnounce->GetChild("text", &szText);

         if(iTime == 0)
         {
            sprintf(szWrite, "%s%s** \0371SERVER SHUTDOWN NOW\0370 **\n", szReturn, DASHES);
         }
         else
         {
            sprintf(szWrite, "%s%s** \0371SERVER SHUTDOWN\0370 in \0371%d\0370 minutes **\n", szReturn, DASHES, iInterval / 60);
         }
         CmdWrite(szWrite);

         if(szUser != NULL && szText != NULL)
         {
            sprintf(szWrite, "From: \0371%s\n", RETRO_NAME(szUser));
            CmdWrite(szWrite);

            CmdWrite(szText);
         }
         sprintf(szWrite, "\n%s\n", DASHES);
         CmdWrite(szWrite);

         delete[] szUser;
         delete[] szText;
      }
      else if(bAdminCheck == true && stricmp(szMessage, MSG_SYSTEM_MAINTENANCE) == 0)
      {
         // EDFPrint("CmdAnnounceShow system maintenance", pData, false);

         if(pAnnounce->GetChild("folderid", &iFolderID) == true)
         {
            pAnnounce->GetChild("foldername", &szFolder);
            pAnnounce->GetChild("delnummsgs", &iNumMsgs);
            pAnnounce->GetChild("deltotalmsgs", &iTotalMsgs);
            sprintf(szWrite, "%sMaintenance for \0376%s\0370", szReturn, RETRO_NAME(szFolder));
            if(iNumMsgs > 0)
            {
               sprintf(szWrite, "%s (deleted \0376%d\0370 message%s)", szWrite, iNumMsgs, iNumMsgs != 1 ? "s" : "");
            }
            strcat(szWrite, "\n");
            CmdWrite(szWrite);
            delete[] szFolder;
         }
         else if(pAnnounce->GetChild("userid", &iUserID) == true)
         {
            int iNumPairs = 0;

            pAnnounce->GetChild("username", &szUser);
            pAnnounce->GetChild("delnummsgs", &iNumMsgs);
            pAnnounce->GetChild("delnumpairs", &iNumPairs);
            pAnnounce->GetChild("keepnumpairs", &iNumKeeps);
            sprintf(szWrite, "%sMaintenance for \0376%s\0370", szReturn, RETRO_NAME(szUser));
            if(iNumMsgs > 0)
            {
               sprintf(szWrite, "%s (deleted \0376%d\0370 message%s in \0376%d\0370 pair%s leaving \0376%d\0370)", szWrite, iNumMsgs, iNumMsgs != 1 ? "s" : "", iNumPairs, iNumPairs != 1 ? "s" : "", iNumKeeps);
            }
            strcat(szWrite, "\n");
            CmdWrite(szWrite);
            delete[] szUser;
         }
         else if(pAnnounce->GetChild("mainttype", &iValue) == true)
         {
            pAnnounce->GetChild("byname", &szBy);

            sprintf(szWrite, "%sSystem maintenance %s", szReturn, iValue == 0 ? "started" : "ended");
            if(szBy != NULL)
            {
               sprintf(szWrite, "%s by \0376%s\0370", szWrite, szBy);
            }
            if(iValue == 1 && pAnnounce->GetChild("mainttime", &iTime) == true)
            {
               strcpy(szTime, "");
               if(iTime > 1000)
               {
                  StrValue(szTime, STRVALUE_TIME, iTime / 1000, '6');
               }
               else
               {
                  sprintf(szTime, "\0376%d\0370 ms", iTime);
               }
               sprintf(szWrite, "%s in %s", szWrite, szTime);
            }
            strcat(szWrite, "\n");
            CmdWrite(szWrite);

            delete[] szBy;
         }
      }
      else if(bAdminCheck == true && stricmp(szMessage, MSG_SYSTEM_RELOAD) == 0)
      {
         pAnnounce->GetChild("reloadtime", &iReloadTime);
         sprintf(szWrite, "%sSystem library reload complete (\0376%d\0370 ms)\n", szReturn, iReloadTime);
         CmdWrite(szWrite);
      }
      else if(bAdminCheck == true && (stricmp(szMessage, MSG_CONNECTION_OPEN) == 0 || stricmp(szMessage, MSG_CONNECTION_CLOSE) == 0 || stricmp(szMessage, MSG_CONNECTION_DENIED) == 0))
      {
         pAnnounce->GetChild("connectionid", &iConnID);
         if(pAnnounce->GetChild("hostname", &szHostname) == false)
         {
            pAnnounce->GetChild("address", &szHostname);
         }
         pAnnounce->GetChild("location", &szLocation);

         sprintf(szWrite, "%sConnection ", szReturn);
         if(CmdVersion("2.5") >= 0 && iConnID > 0)
         {
            sprintf(szWrite, "%s\0376%d\0370 ", szWrite, iConnID);
         }
         if(stricmp(szMessage, MSG_CONNECTION_OPEN) == 0)
         {
            strcat(szWrite, "opened");
         }
         else if(stricmp(szMessage, MSG_CONNECTION_CLOSE) == 0)
         {
            if(pAnnounce->GetChildBool("lost") == true)
            {
               strcat(szWrite, "lost");
            }
            else
            {
               strcat(szWrite, "closed");
            }
         }
         else if(stricmp(szMessage, MSG_CONNECTION_DENIED) == 0)
         {
            strcat(szWrite, "denied");
         }
         sprintf(szWrite, "%s from \0376%s\0370", szWrite, szHostname);
         if(szLocation != NULL)
         {
            sprintf(szWrite, "%s (\0376%s\0370)", szWrite, szLocation);
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);

         delete[] szHostname;
         delete[] szLocation;
      }
      else if(stricmp(szMessage, MSG_FOLDER_ADD) == 0 || (bFolderCheck == true && stricmp(szMessage, MSG_FOLDER_EDIT) == 0))
      {
         pAnnounce->GetChild("folderid", &iValue);
         pAnnounce->GetChild("foldername", &szFolder);
         pAnnounce->GetChild(CmdVersion("2.5") >= 0 ? "byname" : "creatorname", &szBy);

         CmdWrite(szReturn);
         sprintf(szWrite, "Folder \0376%s\0370 has been %s", RETRO_NAME(szFolder), stricmp(szMessage, MSG_FOLDER_ADD) == 0 ? "created" : "edited");
         if(szBy != NULL)
         {
            sprintf(szWrite, "%s by \0376%s\0370", szWrite, szBy);
         }

         if(stricmp(szMessage, MSG_FOLDER_ADD) == 0)
         {
            strcat(szWrite, ". Join it");

            if(CmdYesNo(szWrite, true) == true)
            {
               pRequest = new EDF();
               pRequest->AddChild("folderid", iValue);
               debugEDFPrint("CmdAnnounceShow subscription request", pRequest);
               if(CmdRequest(MSG_FOLDER_SUBSCRIBE, pRequest, &pReply) == true)
               {
                  if(FolderGet(m_pFolderList, iValue) == true)
                  {
                     m_pFolderList->SetChild("subtype", SUBTYPE_SUB);
                  }
                  else
                  {
                     debug("CmdAnnounceShow FolderGet failed on %s announcement\n", szMessage);
                  }
               }
               else
               {
                  debugEDFPrint("CmdAnnounceShow folder_subscribe request failed", pReply);
               }
               delete pReply;
            }
         }
         else
         {
            CmdWrite(szWrite);
            CmdWrite("\n");
         }

         delete[] szFolder;
         delete[] szBy;
      }
      else if(stricmp(szMessage, MSG_FOLDER_DELETE) == 0)
      {
         pAnnounce->GetChild("folderid", &iFolderID);
         pAnnounce->GetChild("foldername", &szFolder);
         pAnnounce->GetChild("byname", &szBy);

         if(bFolderCheck == true)
         {
            sprintf(szWrite, "%s\0376%s\0370 has been deleted", szReturn, RETRO_NAME(szFolder));
            if(szBy != NULL)
            {
               sprintf(szWrite, "%s by \0376%s\0370", szWrite, szBy);
            }
            strcat(szWrite, "\n");
            CmdWrite(szWrite);

            // CmdRefreshFolders();
         }
         // debug("CmdAnnounceShow folder delete %d %d\n", iFolderID, iCurrFolder);
         /* if(iFolderID == iCurrFolder)
         {
            pData->DeleteChild("currfolder");
            pData->DeleteChild("currmessage");
            pData->DeleteChild("currfrom");
            while(pData->DeleteChild("nextunreadmsg") == true);

            SvrInputSetup(pConn, pData, MAIN);
         } */

         delete[] szFolder;
         delete[] szBy;
      }
      else if(bAdminCheck == true && (stricmp(szMessage, MSG_FOLDER_SUBSCRIBE) == 0 || stricmp(szMessage, MSG_FOLDER_UNSUBSCRIBE) == 0))
      {
         pAnnounce->GetChild("userid", &iUserID);

         if(iUserID != iCurrID)
         {
            pAnnounce->GetChild("username", &szUser);
            pAnnounce->GetChild("byname", &szBy);
            pAnnounce->GetChild("foldername", &szFolder);
            pAnnounce->GetChild("subtype", &iSubType);

            sprintf(szWrite, "%s\0376%s\0370 %s ", szReturn, RETRO_NAME(szUser), szBy == NULL ? "has" : "has been");
            if(stricmp(szMessage, MSG_FOLDER_UNSUBSCRIBE) == 0)
            {
               strcat(szWrite, "unsubscribed from");
            }
            else if(iSubType == SUBTYPE_SUB)
            {
               strcat(szWrite, "subscribed to");
            }
            else if(iSubType == SUBTYPE_MEMBER)
            {
               sprintf(szWrite, "%s%s a member of", szWrite, szBy == NULL ? "become" : "made");
            }
            else if(iSubType == SUBTYPE_EDITOR)
            {
               sprintf(szWrite, "%s%s editor of", szWrite, szBy == NULL ? "become" : "made");
            }
            sprintf(szWrite, "%s \0376%s\0370", szWrite, szFolder);
            if(szBy != NULL)
            {
               sprintf(szWrite, "%s by \0376%s\0370", szWrite, RETRO_NAME(szBy));
            }
            strcat(szWrite, "\n");
            CmdWrite(szWrite);

            delete[] szUser;
            delete[] szBy;
            delete[] szFolder;
         }
         else if(bDbgCheck == false)
         {
            iReturn = 0;
         }
      }
      /* else if(bFolderCheck == true && stricmp(szMessage, MSG_FOLDER_UNSUBSCRIBE) == 0)
      {
         pAnnounce->GetChild("username", &szName);
         pAnnounce->GetChild("byname", &szByName);
         pAnnounce->GetChild("foldername", &szFolder);
         pAnnounce->GetChild("subtype", &iSubType);

         // debug("CmdAnnounceShow unsubscribe subtype %d\n", iSubType);

         sprintf(szWrite, "%s\0376%s\0370 %s %s", szReturn, RETRO_NAME(szName), szByName == NULL ? "has" : "has been", iSubType == SUBTYPE_EDITOR ? "demoted" : "unsubscribed");
         sprintf(szWrite, "%s from \0376%s\0370", szWrite, RETRO_NAME(szFolder));
         if(szByName != NULL)
         {
            sprintf(szWrite, "%s by \0376%s\0370", szWrite, RETRO_NAME(szByName));
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);

         delete[] szName;
         delete[] szByName;
         delete[] szFolder;
      } */
      else if(stricmp(szMessage, MSG_MESSAGE_ADD) == 0)
      {
         // EDFPrint("SvrAnnounce message_add", pData, false);

         pAnnounce->GetChild("folderid", &iFolderID);
         pAnnounce->GetChild("messageid", &iMessageID);
         pAnnounce->GetChild("fromid", &iFromID);
         pAnnounce->GetChild("toid", &iToID);
         pAnnounce->GetChild("marktype", &iMarkType);
         pAnnounce->GetChild("marked", &iMarked);

         m_pFolderList->TempMark();
         if(FolderGet(m_pFolderList, iFolderID, NULL, false) == true)
         {
            m_pFolderList->GetChild("accessmode", &iAccessMode);
         }
         m_pFolderList->TempUnmark();

         if(mask(iLStatus, LOGIN_BUSY) == false && bFakePage == true && mask(iAccessMode, ACCMODE_PRIVATE) == true && iToID == iCurrID)
         {
            pRequest = new EDF();
            if(CmdVersion("2.5") < 0)
            {
               pRequest->AddChild("folderid", iFolderID);
            }
            pRequest->AddChild("messageid", iMessageID);
            // EDFPrint("CmdAnnounceShow fake page request", pRequest);
            if(CmdRequest(MSG_MESSAGE_LIST, pRequest, &pReply) == true)
            {
               m_pFolderList->TempMark();
               if(FolderGet(m_pFolderList, iFolderID, NULL, false) == true)
               {
                  m_pFolderList->GetChild("unread", &iUnread);
                  if(iUnread > 0)
                  {
                     debug(DEBUGLEVEL_DEBUG, "CmdAnnounceShow reset private folder unread count (currently %d)\n", iUnread);
                     iUnread--;
                     m_pFolderList->SetChild("unread", iUnread);
                  }
               }
               m_pFolderList->TempUnmark();

               // EDFPrint("CmdAnnounceShow fake page reply", pReply);
               CmdWrite((char *)szReturn);
               if(PageMenu(pReply, bBell) == true)
               {
                  CmdMessageMark(iFolderID, NULL, -1);
               }
            }
            else
            {
               EDFPrint("CmdAnnounceShow fake page error", pReply);
            }
            delete pReply;

            iReturn = CMD_RESET;
         }
         else if(bFolderCheck == true)
         {
            pAnnounce->GetChild("foldername", &szFolder);
            pAnnounce->GetChild("fromname", &szBy);
            pAnnounce->GetChild("subject", &szSubject);

            if(CmdVersion("2.5") >= 0)
            {
               pAnnounce->GetChild("msgtype", &iMsgType);
            }
            else
            {
               pAnnounce->GetChild("votetype", &iMsgType);
            }
            // debug("CmdAnnounceShow message_add vote type %d\n", iVoteType);

            if(iFromID == iCurrID || iToID == iCurrID)
            {
               cColour = '1';
            }

            if(bExtraCheck == true)
            {
               sprintf(szWrite, "%s%s \037%c%d\0370", szReturn, (CmdVersion("2.5") >= 0 && mask(iMsgType, MSGTYPE_VOTE) == true) || (CmdVersion("2.5") < 0 && iMsgType > 0) ? "Poll" : "Message", cColour, iMessageID);
               iColourLen += 4;
            }
            else
            {
               sprintf(szWrite, "%sA new %s", szReturn, (CmdVersion("2.5") >= 0 && mask(iMsgType, MSGTYPE_VOTE) == true) || (CmdVersion("2.5") < 0 && iMsgType > 0) ? "poll" : "message");
            }
            strcat(szWrite, " has been posted ");
            sprintf(szWrite, "%sin \037%c%s\0370 by \037%c%s\0370", szWrite, cColour, RETRO_NAME(szFolder), cColour, RETRO_NAME(szBy));
            iColourLen += 8;
            if(iMarkType > 0)
            {
               iColourLen += 11;
            }
            if(bExtraCheck == true && szSubject != NULL && strlen(szWrite) - iColourLen < CmdWidth() - 6)
            {
               if(strlen(szWrite) + strlen(szSubject) - iColourLen > CmdWidth() - 3)
               {
                  strcpy(szSubject + CmdWidth() + iColourLen - (strlen(szWrite) + 6), "...");
               }
               sprintf(szWrite, "%s (\037%c%s\0370)", szWrite, cColour, szSubject);
            }
            if(iMarkType > 0)
            {
               strcat(szWrite, " [Caught-up]");
            }
            else if(iMarked == MARKED_READ)
            {
               strcat(szWrite, " [Read]");
            }
            strcat(szWrite, "\n");

            CmdWrite(szWrite);

            iReturn = CMD_RESET;
         }
         else if(bDbgCheck == false)
         {
            debug("CmdAnnounceShow not announcing message_add %d\n", iMessageID);
            iReturn = 0;
         }

         delete[] szFolder;
         delete[] szBy;
         delete[] szSubject;
      }
      /* else if(bFolderCheck == true && stricmp(szMessage, MSG_MESSAGE_DELETE) == 0)
      {
         pAnnounce->GetChild("foldername", &szFolder);
         pAnnounce->GetChild("byname", &szBy);
         pAnnounce->GetChild("nummsgs", &iNumMsgs);

         if(iNumMsgs == 1)
         {
            pAnnounce->GetChild("messageid", &iMessageID);
            sprintf(szWrite, "%sMessage \0376%d\0370", szReturn, iMessageID);
         }
         else
         {
            sprintf(szWrite, "%s\0376%d\0370 messages", szReturn, iNumMsgs);
         }
         sprintf(szWrite, "%s in \0376%s\0370 %s been deleted", szWrite, szFolder, iNumMsgs > 1 ? "have" : "has");
         if(iAccessLevel >= LEVEL_WITNESS)
         {
            sprintf(szWrite, "%s by \0376%s\0370", szWrite, RETRO_NAME(szBy));
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);

         delete[] szFolder;
         delete[] szMove;
         delete[] szBy;
      } */
      else if(bFolderCheck == true && (stricmp(szMessage, MSG_MESSAGE_DELETE) == 0 || stricmp(szMessage, MSG_MESSAGE_MOVE) == 0))
      {
         // CmdWrite("Message move\n");

         pAnnounce->GetChild("foldername", &szFolder);
         pAnnounce->GetChild("movename", &szMove);
         pAnnounce->GetChild("byname", &szBy);
         if(CmdVersion("2.5") >= 0)
         {
            pAnnounce->GetChild(stricmp(szMessage, MSG_MESSAGE_DELETE) == 0 ? "numdeleted" : "nummoved", &iNumMsgs);
         }
         else
         {
            pAnnounce->GetChild("nummsgs", &iNumMsgs);
         }
         if(pAnnounce->Child("messageid") == true)
         {
            pAnnounce->Get(NULL, &iMessageID);
            bAction = pAnnounce->GetChildBool("action", true);
            pAnnounce->Parent();
         }
         if(CmdVersion("2.5") >= 0)
         {
            pAnnounce->GetChild(stricmp(szMessage, MSG_MESSAGE_DELETE) == 0 ? "deletetype" : "movetype", &iMoveType);
         }

         if(iNumMsgs > 0)
         {
            if(iMoveType == THREAD_CHILD || bAction == false)
            {
               sprintf(szWrite, "%s\0376%d\0370 repl%s to message \0376%d\0370 in \0376%s\0370", szReturn, iNumMsgs, iNumMsgs != 1 ? "ies" : "y", iMessageID, RETRO_NAME(szFolder));
            }
            else
            {
               iNumMsgs--;
               sprintf(szWrite, "%sMessage \0376%d\0370 in \0376%s\0370", szReturn, iMessageID, RETRO_NAME(szFolder));
               if(iNumMsgs >= 1)
               {
                  sprintf(szWrite, "%s and \0376%d\0370 repl%s", szWrite, iNumMsgs, iNumMsgs != 1 ? "ies" : "y");
               }
            }
            sprintf(szWrite, "%s %s", szWrite, stricmp(szMessage, MSG_MESSAGE_DELETE) == 0 ? "deleted" : "moved");
            if(stricmp(szMessage, MSG_MESSAGE_MOVE) == 0)
            {
               sprintf(szWrite, "%s to \0376%s\0370", szWrite, RETRO_NAME(szMove));
            }
            if(iCurrLevel >= LEVEL_WITNESS)
            {
               sprintf(szWrite, "%s by \0376%s\0370", szWrite, RETRO_NAME(szBy));
            }
            strcat(szWrite, "\n");
            CmdWrite(szWrite);
         }

         delete[] szFolder;
         delete[] szMove;
         delete[] szBy;
      }
      else if(bFolderCheck == true && stricmp(szMessage, MSG_MESSAGE_EDIT) == 0)
      {
         pAnnounce->GetChild("folderid", &iFolderID);
         pAnnounce->GetChild("foldername", &szFolder);
         pAnnounce->GetChild("messageid", &iMessageID);
         pAnnounce->GetChild("byname", &szBy);
         pAnnounce->GetChild("marktype", &iMarkType);
         pAnnounce->GetChild("marked", &iMarked);

         sprintf(szWrite, "%sMessage \0376%d\0370 in \0376%s\0370 has been edited", szReturn, iMessageID, szFolder);
         if(szBy != NULL)
         {
            sprintf(szWrite, "%s by \0376%s\0370", szWrite, RETRO_NAME(szBy));
         }

         if(iMarkType > 0)
         {
            strcat(szWrite, " [Caught-up]");
         }
         else if(iMarked == MARKED_UNREAD)
         {
            strcat(szWrite, " [Unread]");
         }
         else if(iMarked == MARKED_READ)
         {
            strcat(szWrite, " [Read]");
         }

         strcat(szWrite, "\n");

         delete[] szBy;
         delete[] szFolder;

         CmdWrite(szWrite);
      }
      else if(bAdminCheck == true && bExtraCheck == true && stricmp(szMessage, MSG_MESSAGE_VOTE) == 0)
      {
         pAnnounce->GetChild("foldername", &szFolder);
         pAnnounce->GetChild("messageid", &iMessageID);
         pAnnounce->GetChild("subject", &szSubject);
         pAnnounce->GetChild("totalvotes", &iTotalVotes);

         sprintf(szWrite, "%sPoll \0376%d\0370 in \0376%s\0370 about \0376%s\0370 voted on", szReturn, iMessageID, RETRO_NAME(szFolder), szSubject);
         if(iTotalVotes > 0)
         {
            bLoop = pAnnounce->Child("vote");
            while(bLoop == true)
            {
               pAnnounce->GetChild("text", &szText);
               pAnnounce->GetChild("numvotes", &iNumVotes);
               sprintf(szWrite, "%s%c \0376%s\0370(\0376%d\0370%%)", szWrite, bFirst == true ? '.' : ',', szText, (iNumVotes * 100) / iTotalVotes);
               bFirst = false;
               delete[] szText;

               bLoop = pAnnounce->Next("vote");
               if(bLoop == false)
               {
                  pAnnounce->Parent();
               }
            }
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);

         delete[] szFolder;
         delete[] szSubject;
      }
      /* else if(stricmp(szMessage, MSG_CHANNEL_ADD) == 0)
      {
      } */
      /* else if(stricmp(szMessage, MSG_CHANNEL_DELETE) == 0)
      {
         pAnnounce->GetChild("channelid", &iChannelID);
         pAnnounce->GetChild("channelname", &szName);

         sprintf(szWrite, "%s\0376%s\0370 has been deleted\n", szReturn, szName);
         CmdWrite(szWrite);

         delete[] szName;

         SvrInputSetup(pConn, pData, MAIN);
      } */
      else if((CmdVersion("2.5") < 0 || CmdVersion("2.5") >= 0) && stricmp(szMessage, MSG_CHANNEL_SUBSCRIBE) == 0)
      {
         // EDFPrint("CmdAnnounceShow channel_subscribe", pData, false, true);
         pAnnounce->GetChild("channelname", &szChannel);
         pAnnounce->GetChild("username", &szUser);
         pAnnounce->GetChild("announcetime", &iValue);
         // tmTime = localtime((time_t *)&iValue);
         // strftime(szTime, sizeof(szTime), "%H:%M:%S", tmTime);
         StrTime(szTime, STRTIME_TIME, iValue);

         sprintf(szWrite, "%s\0376%s\0370 has joined ", szReturn, szUser);
         if(szChannel != NULL)
         {
            sprintf(szWrite, "%s\0376%s\0370", szWrite, RETRO_NAME(szChannel));
         }
         else
         {
            strcat(szWrite, "talk");
         }
         sprintf(szWrite, "%s (\0376%s\0370)\n", szWrite, szTime);
         CmdWrite(szWrite);

         delete[] szUser;
         delete[] szChannel;
      }
      else if((CmdVersion("2.5") < 0 || CmdVersion("2.5") >= 0) && stricmp(szMessage, MSG_CHANNEL_UNSUBSCRIBE) == 0)
      {
         // EDFPrint("CmdAnnounceShow channel_unsubscribe", pData, false, true);
         pAnnounce->GetChild("channelname", &szChannel);
         pAnnounce->GetChild("username", &szUser);
         pAnnounce->GetChild("announcetime", &iValue);
         // tmTime = localtime((time_t *)&iValue);
         // strftime(szTime, sizeof(szTime), "%H:%M:%S", tmTime);
         StrTime(szTime, STRTIME_TIME, iValue);

         sprintf(szWrite, "%s\0376%s\0370 has left ", szReturn, szUser);
         if(szChannel != NULL)
         {
            sprintf(szWrite, "%s\0376%s\0370", szWrite, RETRO_NAME(szChannel));
         }
         else
         {
            sprintf(szWrite, "%stalk", szWrite);
         }
         sprintf(szWrite, "%s (\0376%s\0370)\n", szWrite, szTime);
         CmdWrite(szWrite);

         delete[] szUser;
         delete[] szChannel;
      }
      else if((CmdVersion("2.5") < 0 || CmdVersion("2.5") >= 0) && stricmp(szMessage, MSG_CHANNEL_SEND) == 0)
      {
         // debug("CmdAnnounceShow talk message %d %d\n", CmdMenuType(), MENU_BLANK);

         pAnnounce->GetChild("fromid", &iFromID);
         if(pAnnounce->GetChild("fromname", &szUser) == false)
         {
            pAnnounce->GetChild("channelname", &szUser);

            cColour = '1';
         }
         else
         {
            if(iFromID == iCurrID)
            {
               cColour = '1';
            }
            else
            {
               cColour = '3';
            }
         }
         pAnnounce->GetChild("text", &szText);

         if(szText != NULL)
         {
            CmdPageOn();

            sprintf(szEmote1, "%s(\037%c%s\0370)", szReturn, cColour, RETRO_NAME(szUser));
            sprintf(szEmote2, "%s\037%c%s\0370", szReturn, cColour, RETRO_NAME(szUser));

            szTemp = UserEmote(szEmote1, szEmote2, szText, false, cColour);
            CmdWrite(szTemp);
            delete[] szTemp;

            CmdWrite("\n");
            CmdPageOff();
            delete[] szText;
         }
         delete[] szUser;
      }
      else if(stricmp(szMessage, MSG_USER_ADD) == 0)
      {
         pAnnounce->GetChild("username", &szUser);
         pAnnounce->GetChild("byname", &szBy);

         sprintf(szWrite, "%sUser \0376%s\0370 %s", szReturn, RETRO_NAME(szUser), stricmp(szMessage, MSG_USER_ADD) == 0 ? "created" : "deleted");
         if(szBy != NULL)
         {
            sprintf(szWrite, "%s by \0376%s\0370", szWrite, RETRO_NAME(szBy));
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);

         delete[] szBy;
         delete[] szUser;
      }
      else if(stricmp(szMessage, MSG_USER_DELETE) == 0)
      {
         pAnnounce->GetChild("username", &szUser);
         pAnnounce->GetChild("byname", &szBy);

         sprintf(szWrite, "%sUser \0376%s\0370 has been deleted", szReturn, RETRO_NAME(szUser));
         if(szBy != NULL)
         {
            sprintf(szWrite, "%s by \0376%s\0370", szWrite, szBy);
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);

         delete[] szUser;
         delete[] szBy;
      }
      else if(bUserCheck == true && (stricmp(szMessage, MSG_USER_LOGIN) == 0 || stricmp(szMessage, MSG_USER_LOGIN_DENIED) == 0))
      {
         pAnnounce->GetChild("userid", &iUserID);
         pAnnounce->GetChild("connectionid", &iConnID);
         pAnnounce->GetChild("status", &iStatus);

         if(iCurrLevel >= LEVEL_WITNESS || iUserID == iCurrID || bDevOption == true || !(mask(iStatus, LOGIN_SHADOW) == true || iConnID > 0))
         {
            pAnnounce->GetChild("username", &szUser);
            if(pAnnounce->GetChild("hostname", &szHostname) == false)
            {
               pAnnounce->GetChild("address", &szHostname);
            }
            pAnnounce->GetChild("location", &szLocation);
            pAnnounce->GetChild("client", &szClient);

            sprintf(szWrite, "%s\0376%s\0370", szReturn, RETRO_NAME(szUser));
            if(mask(iStatus, LOGIN_SHADOW) == true || iConnID > 0)
            {
               // sprintf(szWrite, "%s [\0376%d\0370]", szWrite, iConnID);
               strcat(szWrite, "(\0376H\0370)");
            }
            sprintf(szWrite, "%s has %s", szWrite, stricmp(szMessage, MSG_USER_LOGIN) == 0 ? "logged in" : "been denied login");

            if(szHostname != NULL)
            {
               int iEnd = CmdWidth() - strlen(szUser) - 24;
               if(szLocation != NULL)
               {
                  iEnd -= strlen(szLocation);
               }
               // debug("CmdAnnounceShow end %d, host %d\n", iEnd, strlen(szHostname));
               if(szClient != NULL)
               {
                  iEnd -= strlen(szClient);
                  iEnd -= 7;
               }
               if(strlen(szHostname) > iEnd)
               {
                  if(iEnd < 0)
                  {
                     szHostname[0] = '\0';
                  }
                  else
                  {
                     szHostname[iEnd] = '\0';
                  }
               }
               sprintf(szWrite, "%s from \0376%s\0370", szWrite, szHostname);
               if(szLocation != NULL)
               {
                  sprintf(szWrite, "%s (\0376%s\0370)", szWrite, szLocation);
               }
               if(szClient != NULL)
               {
                  sprintf(szWrite, "%s using \0376%s\0370", szWrite, szClient);
               }
               /* if(pAnnounce->GetChildBool("secure") == true)
               {
               } */
            }
            else if(szLocation != NULL)
            {
               sprintf(szWrite, "%s from \0376%s\0370", szWrite, szLocation);
            }
            strcat(szWrite, "\n");
            CmdWrite(szWrite);

            delete[] szUser;
            delete[] szHostname;
            delete[] szLocation;
            delete[] szClient;
         }
         else
         {
            iReturn = 0;
         }
      }
      else if(bUserCheck == true && stricmp(szMessage, MSG_USER_LOGOUT) == 0)
      {
         pAnnounce->GetChild("userid", &iUserID);
         pAnnounce->GetChild("connectionid", &iConnID);
         pAnnounce->GetChild("status", &iStatus);

         if(iCurrLevel >= LEVEL_WITNESS || iUserID == iCurrID || bDevOption == true || !(mask(iStatus, LOGIN_SHADOW) == true || iConnID > 0))
         {
            pAnnounce->GetChild("username", &szUser);
            bForce = pAnnounce->GetChildBool("force");
            pAnnounce->GetChild("byname", &szBy);
            bForce = pAnnounce->GetChildBool("force");
            bLost = pAnnounce->GetChildBool("lost");
            pAnnounce->GetChild("text", &szText);

            sprintf(szEmote1, "%s\0376%s\0370", szReturn, RETRO_NAME(szUser));
            if(mask(iStatus, LOGIN_SHADOW) == true || iConnID > 0)
            {
               strcat(szEmote1, "(\0376H\0370)");
            }
            strcat(szEmote1, " has ");
            if(bLost == true)
            {
               strcat(szEmote1, "lost connection");
            }
            else
            {
               sprintf(szEmote1, "%s%slogged out", szEmote1, bForce == true ? "been " : "");
            }

            sprintf(szEmote2, "%sLogout: \0376%s\0370", szReturn, RETRO_NAME(szUser));
            if(mask(iStatus, LOGIN_SHADOW) == true || iConnID > 0)
            {
               strcat(szEmote2, "(\0376H\0370)");
            }

            if(szUser != NULL && szBy != NULL && strcmp(szUser, szBy) != 0)
            {
               sprintf(szEmote1, "%s by \0376%s\0370", szEmote1, RETRO_NAME(szBy));
               sprintf(szEmote2, "%s by \0376%s\0370", szEmote2, RETRO_NAME(szBy));
            }

            szTemp = UserEmote(szEmote1, szEmote2, szText, true, '6');
            strcpy(szWrite, szTemp);
            delete[] szTemp;

            strcat(szWrite, "\n");
            CmdWrite(szWrite);

            delete[] szUser;
            delete[] szBy;
            delete[] szText;
         }
         else
         {
            iReturn = 0;
         }
      }
      else if(stricmp(szMessage, MSG_USER_PAGE) == 0)
      {
         CmdWrite((char *)szReturn);

         // debugEDFPrint("CmdAnnounceShow paging", m_pPaging);

         PageMenu(pAnnounce, bBell);
      }
      else if(bUserCheck == true && stricmp(szMessage, MSG_USER_STATUS) == 0)
      {
         // EDFPrint("CmdAnnounceShow user status", pData, false, false);

         bStatus = pAnnounce->GetChild("status", &iLStatus);
         pAnnounce->GetChild("username", &szUser);
         pAnnounce->GetChild("gender", &iValue);
         if(bExtraCheck == true)
         {
            pAnnounce->GetChild(CmdVersion("2.5") >= 0 ? "statusmsg" : "busymsg", &szText);
         }

         if(bStatus == true || szText != NULL)
         {
            if(bStatus == true)
            {
               sprintf(szEmote1, "%s\0376%s\0370 has turned %s pager \0376%s\0370", szReturn, RETRO_NAME(szUser), GenderObject(iValue), mask(iLStatus, LOGIN_BUSY) == true ? "off" : "on");
               sprintf(szEmote2, "%sPager %s: \0376%s\0370", szReturn, mask(iLStatus, LOGIN_BUSY) == true ? "off" : "on", RETRO_NAME(szUser));
            }
            else
            {
               sprintf(szEmote1, "%s\0376%s\0370", szReturn, RETRO_NAME(szUser));
               strcpy(szEmote2, szEmote1);
            }

            szTemp = UserEmote(szEmote1, szEmote2, szText, true, '6');
            strcpy(szWrite, szTemp);
            delete[] szTemp;

            strcat(szWrite, "\n");

            CmdWrite(szWrite);
         }

         delete[] szUser;
         delete[] szText;
      }
      else if(bUserCheck == true && stricmp(szMessage, MSG_USER_LOGIN_INVALID) == 0)
      {
         pAnnounce->GetChild("username", &szUser);
         if(pAnnounce->GetChild("hostname", &szHostname) == false)
         {
            pAnnounce->GetChild("address", &szHostname);
         }
         pAnnounce->GetChild("location", &szLocation);

         sprintf(szWrite, "%s\0376%s\0370 has failed to login from \0376%s\0370", szReturn, RETRO_NAME(szUser), szHostname);
         if(szLocation != NULL)
         {
            sprintf(szWrite, "%s (\0376%s\0370)", szWrite, szLocation);
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);

         delete[] szUser;
         delete[] szHostname;
      }
      /* else if(bDbgCheck == true)
      {
         sprintf(szWrite, "%sDebug: announce message \0376%s\0370\n", szReturn, szMessage);
         CmdWrite(szWrite);
         byte *pEDF = NULL;
         pAnnounce->Write(&pEDF, false, false, true, true);
         CmdWrite(pEDF);
         if(strcmp((char *)pEDF, "") != 0)
         {
            CmdWrite("\n");
         }
         delete[] pEDF;
      } */
      else if(bDbgCheck == false)
      {
         iReturn = 0;

         // EDFPrint("CmdAnnounceShow not displaying", pAnnounce);
      }

      delete[] szMessage;
   }
   else
   {
      iReturn = 0;
   }

   STACKTRACEUPDATE

   // debug("CmdAnnounceShow exit %d\n", iReturn);
   return iReturn;
}

CmdInput *CmdInputSetup(int iStatus)
{
   STACKTRACE
   int iUserID = 0, iAccessLevel = LEVEL_NONE, iSubType = 0, iFolderMode = 0, iNumUnread = 0, iMsgType = 0, iVoteType = 0, iVoteID = -1;
   int iNumFolders = 0, iRetro = 0, iToID = 0, iFromID = -1;
   bool bLoop = false, bDevOption = false, bRetro = false, bBrowse = false, bOldMenus = false, bAnnotated = false, bContent = false;//, bVoted = false;
   char *szFolderName = NULL;//, *szText = NULL;
   EDF *pOptions = NULL;
   CmdInput *pInput = NULL;

   m_pUser->Root();
   m_pUser->Get(NULL, &iUserID);
   m_pUser->GetChild("accesslevel", &iAccessLevel);
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      bDevOption = m_pUser->GetChildBool("devoption");
      m_pUser->GetChild("retro", &iRetro);
      bRetro = mask(iRetro, RETRO_NAMES);
      bOldMenus = mask(iRetro, RETRO_MENUS);
      if(CmdBrowser() != NULL)
      {
         bBrowse = m_pUser->GetChildBool("browse", true);
      }
      m_pUser->Parent();
   }

   switch(iStatus)
   {
      case MAIN:
         m_pUser->Root();
         m_pUser->GetChild("accesslevel", &iAccessLevel);
         m_pFolderList->Root();
         m_pFolderList->GetChild("numfolders", &iNumFolders);

         m_pPaging->Root();
         // EDFPrint("Paging", m_pPaging);

         pInput = CmdMain(iAccessLevel, iNumFolders, bDevOption, m_pPaging->IsChild("ignore"));
         break;

      case ADMIN:
         m_pUser->Root();
         m_pUser->GetChild("accesslevel", &iAccessLevel);

         pInput = CmdAdmin(iAccessLevel);
         break;

      case FOLDER:
         // debug("CmdInputSetup current message %d / folder %d\n", CmdCurrMessage(), CmdCurrFolder());

         // iSubType = CmdFolderSubType(CmdCurrFolder());

         FolderGet(m_pFolderList, CmdCurrFolder(), &szFolderName, false);
         m_pFolderList->GetChild("subtype", &iSubType);
         m_pFolderList->GetChild("accessmode", &iFolderMode);
         m_pFolderList->GetChild("unread", &iNumUnread);
         /* debug("CmdInputSetup %d unread in folder list, ", iNumUnread);
         m_pMessageList->Root();
         iNumUnread = m_pMessageList->Children("message", true) - m_pMessageList->Children("read", true);
         debug("%d unread in message list\n", iNumUnread); */
         if(szFolderName == NULL)
         {
            debug(DEBUGLEVEL_DEBUG, "CmdInputSetup NULL folder name for %d\n", CmdCurrFolder());
         }

         m_iFromID = -1;
         if(m_pMessageView != NULL && CmdCurrMessage() != -1)
         {
            m_pMessageView->GetChild("fromid", &m_iFromID);
            m_pMessageView->GetChild("toid", &iToID);
            m_pMessageView->GetChild("msgtype", &iMsgType);
            if(CmdVersion("2.5") >= 0)
            {
               if(m_pMessageView->Child("votes") == true)
               {
                  m_pMessageView->GetChild("votetype", &iVoteType);
                  // bVoted = m_pMessageView->GetChildBool("voted");

                  if(mask(iVoteType, VOTE_NAMED) == true)
                  {
                  }
                  else
                  {
                     if(m_pMessageView->GetChildBool("voted") == true)
                     {
                        iVoteID = 0;
                     }
                  }

                  m_pMessageView->Parent();
               }
            }
            else
            {
               m_pMessageView->GetChild("votetype", &iMsgType);

               if(iAccessLevel < LEVEL_WITNESS && iSubType < SUBTYPE_EDITOR)
               {
                  bLoop = m_pMessageView->Child("note");
                  while(bLoop == true && bAnnotated == false)
                  {
                     m_pMessageView->GetChild("fromid", &iFromID);
                     if(iFromID == iUserID)
                     {
                        bAnnotated = true;
                        m_pMessageView->Parent();
                     }
                     else
                     {
                        bLoop = m_pMessageView->Next("note");
                        if(bLoop == false)
                        {
                           m_pMessageView->Parent();
                        }
                     }
                  }
               }
            }
         }

         bLoop = true;
         // m_iPrevFolder = CmdCurrFolder();
         // debug("FolderMenu %d %d %d\n", iFolderID, iMessageID, iMsgPos);
         // EDFPrint("FolderMenu message list", m_pMessageList);
         if(m_pMessageView != NULL)
         {
            if(CmdContentList(m_pMessageView, false) > 0)
            {
               bContent = true;
            }
         }
         pInput = CmdFolder(iUserID, iAccessLevel, iNumUnread, szFolderName, iSubType, CmdCurrMessage(), m_iFromID, iToID, iFolderMode, iUserID, iMsgType, iVoteType, iVoteID, bAnnotated, m_iMsgPos, bContent, bRetro, bOldMenus, bDevOption);

         delete[] szFolderName;
         break;

      case TALK:
         pInput = new CmdInput(CMD_MENU_ANY | CMD_MENU_SILENT, "");
         break;

      case MESSAGE:
         pInput = new CmdInput(CMD_MENU_TIME | CMD_MENU_NOCASE, "Messages");
         if(CmdVersion("2.5") >= 0 && iAccessLevel >= LEVEL_WITNESS)
         {
            pInput->MenuAdd('a', "Archive message");
         }
         pInput->MenuAdd('c', "Close vote");
         pInput->MenuAdd('m', "Mass catch-up");
         if(CmdVersion("2.5") >= 0)
         {
            pInput->MenuAdd('p', "folder Priorities");
         }
         if(CmdVersion("2.7") >= 0)
         {
            debug("CmdInputSetup adding marking rules\n");
            pInput->MenuAdd('r', "marking Rules");
         }
         if(iAccessLevel >= LEVEL_WITNESS || (CmdVersion("2.5") >= 0 && iAccessLevel >= LEVEL_EDITOR))
         {
            pInput->MenuAdd('v', "enter Vote");
         }
         // if(CmdVersion("2.5") >= 0)
         {
            pInput->MenuAdd('s', "Search messages");
         }
         if(CmdVersion("2.7") >= 0)
         {
            debug("CmdInputSetup adding free-form search\n");
            pInput->MenuAdd('f', "Free-form search");
         }
         pInput->MenuAdd('x', "eXit", NULL, true);

         if(CmdCurrChannel() != -1)
         {
            pInput->MenuValue('/');
         }
         break;

      case GAME:
         pInput = new CmdInput(CMD_MENU_TIME, "Game");

         if(g_pGame != NULL)
         {
            if(g_pGame->IsRunning() == true)
            {
               g_pGame->Loop();

               pOptions = g_pGame->KeyOptions();

               bLoop = pOptions->Child("key");
               while(bLoop == true)
               {
                  bLoop = pOptions->Next("key");
                  if(bLoop == false)
                  {
                     pOptions->Parent();
                  }
               }

               delete pOptions;
            }
            else if(g_pGame->IsCreator() == true)
            {
               pInput->MenuAdd('s', "Start game");
               pInput->MenuAdd('e', "End game");
            }
            pInput->MenuAdd('x', "eXit");
         }
         else
         {
            pInput->MenuAdd('c', "Create game");
            pInput->MenuAdd('j', "Join game");
            pInput->MenuAdd('x', "eXit");
         }
         break;
   }

   if(pInput == NULL)
   {
      CmdWrite("** CmdInputSetup ERROR no menu for status %d **\n", iStatus);
      // CmdShutdown();
   }

   return pInput;
}

char CmdMessageInput(CmdInput *pInput, bool bMessage, bool bThread, bool bWhole, bool bReplies, bool bCrossFolder, bool bFrom, bool bTo, bool bSubject, bool bKeyword, char cDefault)
{
   char cOption = '\0';

   if(bMessage == true)
   {
      pInput->MenuAdd('m', "Message");
   }
   if(bThread == true)
   {
      pInput->MenuAdd('t', "This and replies");
      if(bCrossFolder == true && CmdVersion("2.6") >= 0 && CmdInput::MenuCase() == true)
      {
         pInput->MenuAdd('T', "This and replies (cross folder)");
      }
   }
   if(bWhole == true)
   {
      pInput->MenuAdd('w', "Whole thread");
      if(bCrossFolder == true && CmdVersion("2.6") >= 0 && CmdInput::MenuCase() == true)
      {
         pInput->MenuAdd('W', "Whole thread (cross folder)");
      }
   }
   if(bReplies == true && CmdVersion("2.5") >= 0)
   {
      pInput->MenuAdd('r', "Replies only");
      if(bCrossFolder == true && CmdVersion("2.6") >= 0 && CmdInput::MenuCase() == true)
      {
         pInput->MenuAdd('R', "Replies only (cross folder)");
      }
   }
   if(bFrom == true)
   {
      pInput->MenuAdd('f', "From");
   }
   if(bTo == true)
   {
      pInput->MenuAdd(bThread == false ? 't' : 'o', bThread == false ? "To" : "tO");
   }
   if(bSubject == true)
   {
      pInput->MenuAdd('s', "Subject");
   }
   if(bKeyword == true)
   {
      pInput->MenuAdd('k', "Keyword");
   }
   pInput->MenuAdd('x', "eXit");

   if(cDefault != '\0')
   {
      pInput->MenuDefault(cDefault);
   }

   cOption = CmdMenu(pInput);
   return cOption;
}

int CmdWholistInput()
{
   char cOption = '\0';
   int iAccessLevel = LEVEL_NONE, iWhoType = -1;
   CmdInput *pInput = NULL;

   // cOption = CmdMenu(USER_WHO);
   m_pUser->Root();
   m_pUser->GetChild("accesslevel", &iAccessLevel);

   pInput = new CmdInput(CMD_MENU_NOCASE, "Wholist");
   pInput->MenuAdd('a', "sort by Access level", HELP_WHOLIST_A);
   pInput->MenuAdd('b', "show only Busy users", HELP_WHOLIST_B);
   pInput->MenuAdd('c', "show only aCtive users", HELP_WHOLIST_C);
   pInput->MenuAdd('d', "Default", HELP_WHOLIST_D);
   if(iAccessLevel >= LEVEL_WITNESS)
   {
      pInput->MenuAdd('e', "show cliEnt / protocol", HELP_WHOLIST_E);
      pInput->MenuAdd('h', "show Hostname / client", HELP_WHOLIST_H);
      pInput->MenuAdd('p', "show Proxy logins", HELP_WHOLIST_P);
   }
   pInput->MenuAdd('i', "show only Idle users", HELP_WHOLIST_I);
   pInput->MenuAdd('l', "show as name List", HELP_WHOLIST_L, true);
   pInput->MenuAdd('n', "sort by Name", HELP_WHOLIST_N);
   // pInput->MenuAdd('o', "Old style", HELP_WHOLIST_O);
   if(CmdVersion("2.5") >= 0)
   {
      if(iAccessLevel >= LEVEL_WITNESS)
      {
         pInput->MenuAdd('r', "show hostname / addRess", HELP_WHOLIST_R);
      }
      pInput->MenuAdd('s', "show only Status message users", HELP_WHOLIST_S);
   }
   pInput->MenuAdd('t', "sort by Time on", HELP_WHOLIST_T);
   pInput->MenuAdd('x', "eXit", HELP_WHOLIST_X);

   cOption = CmdMenu(pInput);

   switch(cOption)
   {
      case 'a':
         iWhoType = 1;
         break;

      case 'b':
         iWhoType = 8;
         break;

      case 'c':
         iWhoType = 5;
         break;

      case 'd':
         iWhoType = 7;
         break;

      case 'e':
         iWhoType = 6;
         break;

      case 'h':
         iWhoType = 12;
         break;

      case 'i':
         iWhoType = 4;
         break;

      case 'l':
         iWhoType = 3;
         break;

      case 'n':
         iWhoType = 2;
         break;

      /* case 'o':
         iWhoType = 9;
         break; */

      case 'p':
         iWhoType = 10;
         break;

      case 'r':
         iWhoType = 13;
         break;

      case 's':
         iWhoType = 11;
         break;

      case 't':
         iWhoType = 0;
         break;
   }

   return iWhoType;
}

void CmdContentView(int iContentNum, int iType, char *szString, int iLength)
{
   char szLength[20];
   char *szWrite = NULL;

   if(iContentNum == 1)
   {
      CmdWrite("--\n");
   }

   szWrite = new char[strlen(szString) + 50];

   sprintf(szWrite, "\0373%d\0370: \0373%s\0370", iContentNum, szString);
   if(iLength != -1)
   {
      szLength[0] = '\0';

      strcat(szWrite, " (");
      StrValue(szLength, STRVALUE_BYTE, iLength, '3');
      strcat(szWrite, szLength);
      strcat(szWrite, ")");
   }
   strcat(szWrite, "\n");
   CmdWrite(szWrite);

   delete[] szWrite;
}

char *URLToken(char **szString)
{
	int iURLBack = 0, iAddressLen = 0;
	bool bSlash = false;
	char *szReturn = NULL, *szURL = NULL, *szHTTP = NULL, *szWWW = NULL;

	debug("URLToken entry '%s'\n", *szString);

   szHTTP = strstr(*szString, "http://");
   szWWW = strstr(*szString, "www.");
   if(szWWW > *szString && strchr("., \n", szWWW[-1]) == NULL)
   {
      szWWW = NULL;
   }
   if(szHTTP != NULL && (szWWW == NULL || szHTTP < szWWW))
   {
      szURL = szHTTP + 7;
      iURLBack = 7;
   }
   else if(szWWW != NULL)
   {
      szURL = szWWW;
   }
   else
   {
      szURL = NULL;
   }

   if(szURL != NULL)
   {
      // while(szURL[iAddressLen] != '\0' && szURL[iAddressLen] != ' ' && szURL[iAddressLen] != '\r' && szURL[iAddressLen] != '\n')
      while(szURL[iAddressLen] != '\0' &&
         (isupper(szURL[iAddressLen]) || islower(szURL[iAddressLen]) || isdigit(szURL[iAddressLen]) || strchr("-./", szURL[iAddressLen]) != NULL ||
         (bSlash == false && szURL[iAddressLen] == ':') || (bSlash == true && strchr("~_?%=&,:+#@';()", szURL[iAddressLen]) != NULL)))
      {
         if(szURL[iAddressLen] == '/')
         {
            bSlash = true;
         }
         /* if(szURL[iAddressLen] == '~')
         {
            bTilde = true;
         } */
         iAddressLen++;
      }
      szReturn = strmk(szURL - iURLBack, 0, iAddressLen + iURLBack);
	}

	*szString = szURL + iAddressLen;
	debug("URLToken exit %s, '%s'\n", szReturn, *szString);

	return szReturn;
}

int CmdURLList(const char *szString, int iContentNum, bool bDisplay, int iItemNum, char **szReturn)
{
   bool bSlash = false;
   int iURLBack = 0, iAddressLen = 0;
   char *szURL = NULL, *szHTTP = NULL, *szWWW = NULL, *szAddress = NULL;

   /* if(szString == NULL)
   {
      return iContentNum;
   } */

   szURL = (char *)szString;

   while(szURL != NULL)
   {
		szAddress = URLToken(&szURL);

		if(szAddress != NULL)
		{
         iContentNum++;
         if(bDisplay == true)
         {
            CmdContentView(iContentNum, 0, szAddress, -1);
         }
         if(iItemNum != -1 && iContentNum == iItemNum && szURL != NULL)
         {
            (*szReturn) = strmk(szAddress);
            szURL = NULL;
         }
         else
         {
            if(szURL[iAddressLen] == '\0')
            {
               szURL = NULL;
            }
            else
            {
               szURL += iAddressLen;
            }
         }

         delete[] szAddress;
			szAddress = NULL;
      }
   }

   return iContentNum;
}

int CmdContentList(EDF *pReply, bool bDisplay, int iItemNum, char **szURL, int *iAttachmentID, char **szAttachmentName, bytes **pData)
{
   int iReturn = 0, iID = -1, iLength = -1;
   bool bLoop = false;
   char *szSubject = NULL, *szText = NULL, *szContentType = NULL, *szFilename = NULL;

   if(CmdBrowser() != NULL)
   {
      if(pReply->GetChild("subject", &szSubject) == true && szSubject != NULL)
      {
         iReturn = CmdURLList(szSubject, iReturn, bDisplay, iItemNum, szURL);
         delete[] szSubject;

         if(iItemNum != -1 && iReturn >= iItemNum)
         {
            return iReturn;
         }
      }

      if(pReply->GetChild("text", &szText) == true && szText != NULL)
      {
         iReturn = CmdURLList(szText, iReturn, bDisplay, iItemNum, szURL);
         delete[] szText;

         if(iItemNum != -1 && iReturn >= iItemNum)
         {
            return iReturn;
         }
      }
   }

   if(CmdVersion("2.5") >= 0)
   {
      bLoop = pReply->Child("attachment");
      while(bLoop == true)
      {
         pReply->Get(NULL, &iID);

         if(CmdBrowser() != NULL && pReply->GetChild("url", &szText) == true && szText != NULL)
         {
            iReturn++;
            if(bDisplay == true)
            {
               CmdContentView(iReturn, 0, szText, -1);
            }
            if(iItemNum != -1 && iReturn == iItemNum && szURL != NULL)
            {
               (*szURL) = strmk(szText);
               pReply->Parent();

               bLoop = false;
            }

            delete[] szText;
         }
         else
         {
            if(pReply->GetChild("content-type", &szContentType) == true && szContentType != NULL)
            {
               if(stricmp(szContentType, MSGATT_ANNOTATION) == 0 || stricmp(szContentType, MSGATT_LINK) == 0)
               {
                  if(CmdBrowser() != NULL && pReply->GetChild("text", &szText) == true)
                  {
                     iReturn = CmdURLList(szText, iReturn, bDisplay, iItemNum, szURL);
                     delete[] szText;

                     if(iItemNum != -1 && iReturn >= iItemNum)
                     {
                        pReply->Parent();

                        bLoop = false;
                     }
                  }
               }
               else
               {
                  szFilename = NULL;
                  iLength = -1;

                  pReply->GetChild("filename", &szFilename);
                  if(szFilename == NULL)
                  {
                     szFilename = new char[20];
                     sprintf(szFilename, "att%d", *iAttachmentID);
                  }
                  pReply->GetChild("content-length", &iLength);

                  iReturn++;
                  if(bDisplay == true)
                  {
                     CmdContentView(iReturn, 1, szFilename, iLength);
                  }

                  if(iItemNum != -1 && iReturn >= iItemNum && iAttachmentID != NULL)
                  {
                     pReply->Get(NULL, iAttachmentID);
                     if(szAttachmentName != NULL)
                     {
                        (*szAttachmentName) = strmk(szFilename);
                     }
                     pReply->GetChild("data", pData);
                     pReply->Parent();

                     bLoop = false;
                  }

                  delete[] szFilename;
               }

               delete[] szContentType;
            }
         }

         if(bLoop == true)
         {
            bLoop = pReply->Next("attachment");
            if(bLoop == false)
            {
               pReply->Parent();
            }
         }
      }
   }
   else if(CmdBrowser() != NULL)
   {
      bLoop = pReply->Child("note");
      while(bLoop == true)
      {
         if(pReply->GetChild("text", &szText) == true && szText != NULL)
         {
            iReturn = CmdURLList(szText, iReturn, bDisplay, iItemNum, szURL);
            delete[] szText;

            if(iItemNum != -1 && iReturn >= iItemNum)
            {
               pReply->Parent();
               bLoop = false;
            }
         }

         if(bLoop == true)
         {
            bLoop = pReply->Next("note");
            if(bLoop == false)
            {
               pReply->Parent();
            }
         }
      }
   }

   return iReturn;
}

bool CmdMessageRequest(int iFolderID, int iMessageID, bool bShowOnly, bool bPage, EDF **pReply, bool bRaw, bool bArchive)
{
   int iAccessMode = FOLDERMODE_NORMAL, iUserID = -1, iFromID = -1, iToID = -1, iNumMsgs = 0, iMsgPos = 0, iMsgType = 0;
   bool bReturn = true;
   char *szFolderName = NULL;
   EDF *pRequest = NULL, *pTemp = NULL, *pMessage = NULL;

   pRequest = new EDF();
   if(CmdVersion("2.5") < 0)
   {
      pRequest->AddChild("folderid", iFolderID);
   }
   pRequest->AddChild("messageid", iMessageID);
   if(CmdVersion("2.5") >= 0 && bArchive == true)
   {
      pRequest->AddChild("archive", true);
   }

   if(CmdRequest(MSG_MESSAGE_LIST, pRequest, &pTemp) == false)
   {
      debugEDFPrint("CmdMessageRequest request failed", pTemp);

      CmdWrite("No such message\n");

      CmdCurrMessage(-1);
      m_iMsgPos = MSG_EXIT;

      return false;
   }

   pTemp->GetChild("folderid", &iFolderID);
   pTemp->GetChild("foldername" ,&szFolderName);
   pTemp->GetChild("nummsgs", &iNumMsgs);

   if(pTemp->Child("message") == true)
   {
      // delete pMessage;
      pMessage = new EDF();
      pMessage->Copy(pTemp, true, true);

      m_pUser->Root();

      m_pFolderList->TempMark();
      if(FolderGet(m_pFolderList, iFolderID, NULL, false) == true)
      {
         m_pFolderList->GetChild("accessmode", &iAccessMode);
         // debug("CmdMessageRequest folder mode %d\n", iAccessMode);
      }
      m_pFolderList->TempUnmark();

      if(pMessage->GetChild("msgtype", &iMsgType) == true && mask(iMsgType, MSGTYPE_DELETED) == true)
      {
         if(CmdYesNo("Message has been deleted. View anyway", false) == false)
         {
            bReturn = false;
         }
      }

      if(bReturn == true)
      {
         if(mask(iAccessMode, ACCMODE_PRIVATE) == true)
         {
            m_pUser->Get(NULL, &iUserID);

            pMessage->GetChild("fromid", &iFromID);
            pMessage->GetChild("toid", &iToID);

            // debug("CmdMessageRequest private message user IDs %d, %d %d\n", iUserID, iFromID, iToID);

            if(iFromID != iUserID && iToID != iUserID)
            {
               if(CmdYesNo("Private message is not to or from you. View anyway", false) == false)
               {
                  bReturn = false;
               }
            }
         }

         if(bReturn == true)
         {
            if(bShowOnly == false)
            {
               if(iFolderID != CmdCurrFolder())
               {
                  CmdCurrFolder(iFolderID);
                  CmdRefreshMessages(iFolderID);
                  MessageInFolder(m_pMessageList, iMessageID);
               }

               CmdCurrMessage(iMessageID);
               m_pMessageView = pMessage;

               debug("CmdMessageGoto reset ID point 3 (message %d, folder %d)\n", CmdCurrMessage(), CmdCurrFolder());
            }

            CmdMessageMark(-1, szFolderName, -1);

            // CmdEDFPrint("FolderMenu current message", m_pMessageView, false);

            if(bRaw == false)
            {
               pMessage->GetChild("msgpos", &iMsgPos);

               CmdMessageView(pMessage, iFolderID, szFolderName, CmdVersion("2.6") >= 0 ? iMsgPos : MessagePos(pMessage), CmdVersion("2.6") >= 0 ? iNumMsgs : MessageCount(), bPage);
            }
            else
            {
               if(bPage == true)
               {
                  CmdPageOn();
               }
               CmdEDFPrint(NULL, pTemp, true, true);
               if(bPage == true)
               {
                  CmdPageOff();
               }
            }
         }
      }
   }
   else
   {
      debug(DEBUGLEVEL_ERR, "CmdMessageGoto no message section\n");
   }

   delete[] szFolderName;

   delete pTemp;

   if(pReply != NULL)
   {
      *pReply = pMessage;
   }
   else if(m_pMessageView != pMessage)
   {
      delete pMessage;
   }

   return bReturn;
}

int MessageSearchMenu(int iFolderID, int iUserID, bool bFreeForm)
{
   STACKTRACE
   int iListType = 0, iFolderEDF = -1, iMessageID = -1, iNumMsgs = 0, iColourLen = 0, iSearchType = 1;
   bool bRequest = true, bLoop = true, bDetails = false, bReply = false, bCrossFolder = false;
   char cOption = '\0';
   char *szKeyword = NULL, *szFolderName = NULL, *szFromName = NULL, *szToName = NULL, *szSubject = NULL, *szMessageFolder = NULL;
   char *szMatch = NULL;
   char szWrite[200];
   EDF *pRequest = NULL, *pReply = NULL, *pMessage = NULL, *pMatch = NULL;
   CmdInput *pInput = NULL;

   debug(DEBUGLEVEL_INFO, "MessageSearchMenu %d %d\n", iFolderID, iUserID);

   pRequest = new EDF();

   if(bFreeForm == true)
   {
      szMatch = CmdText();

      if(szMatch != NULL)
      {
         pMatch = MatchToEDF(szMatch);

         delete[] szMatch;

         pRequest->Copy(pMatch, false);

         delete pMatch;
      }
      else
      {
         bRequest = false;
      }
   }
   else
   {
      /* if(iFolderID != -1)
      {
         pRequest->AddChild("folderid", iFolderID);
      } */
      // pRequest->SetChild("searchtype", 1);
      if(CmdVersion("2.6") >= 0)
      {
         pRequest->Add("and");
      }

      do
      {
         bRequest = true;

         pInput = new CmdInput(CMD_MENU_NOCASE, "Search");

         if(iFolderID != -1)
         {
            pInput->MenuAdd('h', "Headers");
         }
         if(iFolderID == -1)
         {
            if(CmdVersion("2.6") >= 0)
            {
               pInput->MenuAdd('a', "match Any field");
               pInput->MenuAdd('l', "match aLl fields");
            }
            pInput->MenuAdd('c', "exeCute");
         }
         if(iFolderID != -1)
         {
            pInput->MenuAdd('p', "toP level");
            pInput->MenuAdd('u', "Unread");
            pInput->MenuAdd('v', "Votes");
         }
         cOption = CmdMessageInput(pInput, false, false, false, false, false, true, true, iFolderID != -1, true, iFolderID != -1 ? 's' : 'x');
         if(cOption == 'x')
         {
            delete pRequest;

            return -1;
         }

         if(isupper(cOption))
         {
            cOption = tolower(cOption);
            bCrossFolder = true;
         }

         switch(cOption)
         {
            case 'a':
               pRequest->Set("or");
               break;

            case 'c':
               if(pRequest->Children() == 0)
               {
                  CmdWrite("No criteria to match\n");
                  cOption = '\0';
               }
               break;

            case 'f':
            case 't':
               iListType = 1;
               /* if(iUserID != -1)
               {
                  UserGet(m_pUserList, iUserID, &szUser);
               } */
               iUserID = CmdLineUser(CmdUserTab, iUserID);
               if(iUserID != -1)
               {
                  pRequest->SetChild(cOption == 'f' ? "fromid" : "toid", iUserID);
                  // pRequest->AddChild("toid", iUserID);
               }
               else
               {
                  if(iFolderID != -1)
                  {
                     bRequest = false;
                  }
                  else
                  {
                     pRequest->DeleteChild(cOption == 'f' ? "fromid" : "toid");
                  }
               }
               break;

            case 'h':
               // pRequest->SetChild("searchtype", 1);
               iSearchType = 1;
               if(CmdVersion("2.6") >= 0)
               {
                  pRequest->Delete();
               }
               break;

            case 'k':
               iListType = 1;
               szKeyword = CmdLineStr("Keyword to search for", LINE_LEN);
               if(szKeyword != NULL && strcmp(szKeyword, "") != 0)
               {
                  pRequest->SetChild("keyword", szKeyword);
               }
               else
               {
                  if(iFolderID != -1)
                  {
                     bRequest = false;
                  }
                  else
                  {
                     pRequest->DeleteChild("keyword");
                  }
               }
               delete[] szKeyword;
               break;

            case 'l':
               pRequest->Set("and");
               break;

            case 'p':
               iListType = 2;
               if(CmdVersion("2.6") >= 0)
               {
                  pRequest->Delete();
               }
               break;

            case 's':
               if(CmdVersion("2.6") >= 0)
               {
                  pRequest->Delete();
               }
               break;

            case 'u':
               iListType = 3;
               if(CmdVersion("2.6") >= 0)
               {
                  pRequest->Delete();
               }
               break;

            case 'v':
               iListType = 4;
               if(CmdVersion("2.6") >= 0)
               {
                  pRequest->Delete();
               }
               break;

            default:
               debug(DEBUGLEVEL_WARN, "MessageSearchMenu no action for %c\n", cOption);
               break;
         }
      }
      while(iFolderID == -1 && cOption != 'c');
   }

   if(bRequest == true)
   {
      pRequest->Root();

      if(iFolderID != -1)
      {
         pRequest->AddChild("folderid", iFolderID);
      }
      pRequest->AddChild("searchtype", iSearchType);

      CmdEDFPrint("MessageSearchMenu request", pRequest);

      if(iFolderID == -1)
      {
         bDetails = CmdYesNo("Show full details", false);

         m_pFolderList->Root();
         bLoop = m_pFolderList->Child("folder");
      }
      else
      {
         iFolderEDF = iFolderID;
      }

      if(bDetails == false)
      {
         CmdPageOn();
      }

      while(bLoop == true)
      {
         if(iFolderID == -1 && CmdVersion("2.6") < 0)
         {
            m_pFolderList->Get(NULL, &iFolderEDF);
            pRequest->SetChild("folderid", iFolderEDF);

            // CmdEDFPrint("MessageSearchMenu request", pRequest);
         }

         if(CmdRequest(MSG_MESSAGE_LIST, pRequest, false, &pReply) == true)
         {
            iNumMsgs += pReply->Children("messages");

            if(iFolderID == -1)
            {
               pReply->GetChild("foldername", &szFolderName);

               if(pReply->Children("message") > 0)
               {
                  // EDFPrint("MessageSearchMenu reply", pReply);

                  /* if(bDetails == true)
                  {
                     CmdPageOn();
                  } */

                  bReply = pReply->Child("message");
                  while(bReply == true)
                  {
                     pReply->Get(NULL, &iMessageID);

                     if(bDetails == true)
                     {
                        if(CmdMessageRequest(iFolderEDF, iMessageID, true, true, &pMessage) == true)
                        {
                           pInput = new CmdInput(CMD_MENU_NOCASE, "Results");
                           pInput->MenuAdd('c', "Continue");
                           pInput->MenuAdd('j', "Jump to message");
                           pInput->MenuAdd('x', "eXit");

                           cOption = CmdMenu(pInput);
                           if(cOption == 'j')
                           {
                              CmdRefreshMessages(iFolderEDF);

                              m_pMessageView = pMessage;

                              MessageInFolder(m_pMessageList, iMessageID);

                              FolderMenu(iFolderEDF, iMessageID, MSG_EXIT);

                              bReply = false;
                              bLoop = false;
                           }
                           else if(cOption == 'x')
                           {
                              bReply = false;
                              bLoop = false;
                           }
                        }
                     }
                     else
                     {
                        szFromName = NULL;
                        szToName = NULL;
                        szSubject = NULL;
                        szMessageFolder = NULL;

                        pReply->GetChild("foldername", &szMessageFolder);
                        pReply->GetChild("fromname", &szFromName);
                        pReply->GetChild("toname", &szToName);
                        pReply->GetChild("subject", &szSubject);
                        sprintf(szWrite, "Matched \0374%d\0370 in \0374%s\0370 from \0374%s\0370", iMessageID, szMessageFolder != NULL ? szMessageFolder : szFolderName, szFromName);
                        iColourLen = 12;
                        if(szToName != NULL)
                        {
                           sprintf(szWrite, "%s to \0374%s\0370", szWrite, szToName);
                           iColourLen += 4;
                        }
                        if(szSubject != NULL && strlen(szWrite) - iColourLen < CmdWidth() - 7)
                        {
                           if(strlen(szWrite) + strlen(szSubject) - iColourLen > CmdWidth() - 4)
                           {
                              strcpy(szSubject + CmdWidth() + iColourLen - (strlen(szWrite) + 7), "...");
                           }
                           sprintf(szWrite, "%s (\0374%s\0370)", szWrite, szSubject);
                        }
                        strcat(szWrite, "\n");
                        CmdWrite(szWrite);

                        delete[] szFromName;
                        delete[] szToName;
                        delete[] szSubject;
                        delete[] szMessageFolder;
                     }

                     if(bReply == true)
                     {
                        bReply = pReply->Iterate("message");
                     }
                  }

                  /* if(bDetails == true)
                  {
                     CmdPageOff();
                  }

                  bLoop = true; */
               }
               else if(bDetails == true)
               {
                  sprintf(szWrite, "No matches found");
                  if(szFolderName != NULL)
                  {
                     sprintf(szWrite, "%s in \0374%s\0370", szWrite, szFolderName);
                  }
                  strcat(szWrite, "\n");
                  CmdWrite(szWrite);
               }

               delete[] szFolderName;
            }
            else
            {
               CmdMessageList(pReply, iListType);
            }
         }

         if(bLoop == true && iFolderID == -1 && CmdVersion("2.6") < 0)
         {
            if(bDetails == true && pReply->Children("message") > 0)
            {
               pInput = new CmdInput(CMD_MENU_NOCASE, "Results");
               pInput->MenuAdd('c', "Continue");
               pInput->MenuAdd('x', "eXit");

               cOption = CmdMenu(pInput);
               if(cOption == 'x')
               {
                  bLoop = false;
               }
               /* else if(bDetails == false)
               {
                  CmdPageOff();
                  CmdPageOn();
               } */
            }

            if(bLoop == true)
            {
               bLoop = m_pFolderList->Iterate("folder");
            }
         }
         else
         {
            bLoop = false;
         }

         delete pReply;
      }

      if(bDetails == false)
      {
         CmdPageOff();
      }
   }

   sprintf(szWrite, "\0374%d\0370 messages matched in total\n", iNumMsgs);

   delete pRequest;

   return iMessageID;
}

bool MessageMoveMenu(int iFolderID, int iMessageID, int iMessageTop)
{
   int iID = 0;
   bool bReturn = false, bCrossFolder = false;
   char cOption = '\0';
   EDF *pRequest = NULL, *pReply = NULL;
   CmdInput *pInput = NULL;

   debug("MessageMoveMenu %d %d %d\n", iFolderID, iMessageID, iMessageTop);

   pInput = new CmdInput(0, "Move");
   cOption = CmdMessageInput(pInput, true, true, true, true, true, false, false, false, false, 'x');
   if(cOption == 'x')
   {
      return false;
   }

   debug("MessageMoveMenu options '%c'\n", cOption);
   if(isupper(cOption))
   {
      cOption = tolower(cOption);
      bCrossFolder = true;
   }

   iID = CmdLineFolder();
   if(iID == -1)
   {
      return false;
   }
   else if(iID == iFolderID)
   {
      CmdWrite("Source and destination folder are the same\n");
      return false;
   }

   pRequest = new EDF();
   if(CmdVersion("2.5") < 0)
   {
      pRequest->AddChild("folderid", iFolderID);
   }
   pRequest->AddChild("messageid", iMessageID);
   pRequest->AddChild("moveid", iID);
   if(cOption == 'r')
   {
      pRequest->AddChild("movetype", THREAD_CHILD);
   }
   else if(cOption == 't')
   {
      pRequest->AddChild("movetype", THREAD_MSGCHILD);
   }
   else if(cOption == 'w')
   {
      pRequest->SetChild("messageid", iMessageTop);
      pRequest->AddChild("movetype", THREAD_MSGCHILD);
   }
   else
   {
      debug(DEBUGLEVEL_WARN, "MessageMoveMenu no action for %c\n", cOption);
   }
   if(bCrossFolder == true)
   {
      pRequest->AddChild("crossfolder", true);
   }

   if(CmdVersion("2.5") >= 0)
   {
      CmdEDFPrint("MessageMoveMenu request", pRequest);
   }

   bReturn = CmdRequest(MSG_MESSAGE_MOVE, pRequest, &pReply);

   if(CmdVersion("2.5") >= 0)
   {
      CmdEDFPrint("MessageMoveMenu reply", pReply);
   }

   delete pReply;

   return bReturn;
}

bool MessageDeleteMenu(int iFolderID, int iMessageID, int iMessageTop, bool bAttachments)
{
   int iType = 0, iAccessLevel = LEVEL_NONE, iSubType = 0, iAttachmentID = -1;
   bool bReturn = false, bCrossFolder = false;
   char cOption = '\0';
   char *szRequest = MSG_MESSAGE_DELETE;
   EDF *pRequest = NULL, *pReply = NULL;
   CmdInput *pInput = NULL;

   m_pUser->Root();
   m_pUser->GetChild("accesslevel", &iAccessLevel);
   // iSubType = CmdFolderSubType(CmdCurrFolder());
   FolderGet(m_pFolderList, iFolderID, NULL, false);
   m_pFolderList->GetChild("subtype", &iSubType);

   pInput = new CmdInput(0, "Kill");
   if(CmdVersion("2.5") >= 0)
   {
      pInput->MenuAdd('a', "Attachments");
   }
   if(iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR)
   {
      cOption = CmdMessageInput(pInput, true, true, true, true, true, false, false, false, false, 'x');
   }
   else
   {
      cOption = CmdMessageInput(pInput, true, false, false, false, false, false, false, false, false, 'x');
   }
   if(cOption == 'x')
   {
      return false;
   }

   if(isupper(cOption))
   {
      cOption = tolower(cOption);
      bCrossFolder = true;
   }

   if(cOption == 'a')
   {
      iAttachmentID = CmdLineNum("Attachment ID", "RETURN to abort", 0, -1);
      if(iAttachmentID == -1)
      {
         return false;
      }
   }
   else if(cOption == 'm')
   {
      iType = 0;
   }
   else if(cOption == 't' || cOption == 'w')
   {
      iType = 1;
   }
   else if(cOption == 'r')
   {
      iType = 2;
   }
   else
   {
      debug(DEBUGLEVEL_WARN, "MessageDeleteMenu no action for %c\n", cOption);
   }

   pRequest = new EDF();
   if(CmdVersion("2.5") < 0)
   {
      pRequest->AddChild("folderid", iFolderID);
   }
   pRequest->AddChild("messageid", cOption == 'w' ? iMessageTop : iMessageID);
   if(iType > 0)
   {
      pRequest->AddChild("deletetype", iType);
   }
   if(cOption == 'a')
   {
      szRequest = MSG_MESSAGE_EDIT;
      pRequest->Add("attachment", "delete");
      pRequest->AddChild("attachmentid", iAttachmentID);
      pRequest->Parent();
   }
   // CmdEDFPrint("MessageDeleteMenu request", pRequest);
   bReturn = CmdRequest(szRequest, pRequest, &pReply);
   if(bReturn == false)
   {
      CmdEDFPrint("MessageDeleteMenu request failed", pReply);
   }

   return bReturn;
}


bool MessageMarkMenu(bool bAdd, int iFolderID, int iMessageID, int iFromID, const char *szSubject, bool bMinCheck)
{
   STACKTRACE
   int iMarkType = 0, iNumMarked = 0, iNumUnread = 0, iNumMsgs = 0;
   int iMinCatchup = 2000, iSubType = 0, iAccessMode, iTotalUnread = 0, iValue = 0, iEndDate = -1, iTotalMarked = 0;
   int iFolderMessage = -1, iFolderUnread = 0;
   bool bLoop = false, bRefresh = false, bMarkKeep = false;
   char cOption = '\0';
   char szWrite[100];
   char *szValue = NULL, *szOption = NULL, *szFolderName = NULL;
   EDF *pRequest = NULL, *pReply = NULL;
   CmdInput *pInput = NULL;
   struct tm *tmTime = NULL;

   debug(DEBUGLEVEL_INFO, "MessageMarkMenu %s %d %d %d %s %s\n", BoolStr(bAdd), iFolderID, iMessageID, iFromID, szSubject, BoolStr(bMinCheck));

   // CmdRequestLog(true);

   if(iFolderID == -1)
   {
      if(bMinCheck == true && m_pUser->Child("client", CLIENT_NAME()) == true)
      {
         m_pUser->GetChild("mincatchup", &iMinCatchup);
         m_pUser->Parent();
      }

      m_pFolderList->Root();
      bLoop = m_pFolderList->Child("folder");
      while(bLoop == true)
      {
         iSubType = 0;
         iAccessMode = FOLDERMODE_NORMAL;

         m_pFolderList->GetChild("subtype", &iSubType);
         m_pFolderList->GetChild("accessmode", &iAccessMode);
         if(mask(iAccessMode, ACCMODE_PRIVATE) == false && iSubType > 0 && m_pFolderList->GetChild("unread", &iNumUnread) == true)
         {
            iTotalUnread += iNumUnread;
         }

         bLoop = m_pFolderList->Iterate("folder");
      }

      if(bMinCheck == true && iTotalUnread < iMinCatchup)
      {
         return false;
      }

      sprintf(szWrite, "You have \0374%d\0370 messages to catch up\n", iTotalUnread);
      CmdWrite(szWrite);
   }

   pInput = new CmdInput(CMD_MENU_NOCASE, bAdd == true ? "Catch-up" : "Hold");
   if(iFolderID != -1 && iMessageID == -1)
   {
      pInput->MenuAdd('y', "Yes");
   }
   else
   {
      pInput->MenuAdd('a', "All");
   }
   if(bAdd == true)
   {
      pInput->MenuAdd('d', "until <x> Days ago");
      pInput->MenuAdd('h', "until <x> Hours ago");
   }
   /* if(iMessageID != -1 && CmdVersion("2.6") >= 0)
   {
      if(CmdInput::MenuCase() == true)
      {
         pInput->MenuAdd('k', "Keep caught up (from here)");
         pInput->MenuAdd('K', "Keep caught up (whole thread)");
      }
      else
      {
         pInput->MenuAdd('k', "Keep caught up");
      }
   } */
   // pInput->MenuAdd('n', "until Now");
   if(bAdd == true)
   {
      pInput->MenuAdd('u', "Until today");
   }
   if(iMessageID != -1)
   {
      cOption = CmdMessageInput(pInput, bAdd == false, true, true, true, false, iFolderID != -1, false, true, false, bAdd == false ? 'm' : 'x');
   }
   else
   {
      cOption = CmdMessageInput(pInput, false, false, false, false, false, iFolderID != -1, false, false, false, bAdd == false ? 'm' : 'x');
   }
   if(cOption == 'x')
   {
      return false;
   }

   if(CmdVersion("2.5") < 0)
   {
      if(cOption == 's')
      {
         cOption = 't';
      }
   }

   switch(cOption)
   {
      case 'a':
      case 'y':
         if(iFolderID != -1)
         {
            if(CmdYesNo(bAdd == true ? "Catch-up all messages" : "Hold all messages", false) == true)
            {
               iMessageID = -1;
               // iMarkType = 2;
            }
            else
            {
               cOption = 'x';
            }
         }
         break;

      case 'd':
         iValue = CmdLineNum("Number of days");
         iEndDate = CmdInput::MenuTime();
         iEndDate -= 86400 * iValue;
         break;

      case 'f':
         iMessageID = -1;
         iFromID = CmdLineUser(CmdUserTab, iFromID);
         break;

      case 'h':
         iValue = CmdLineNum("Number of hours");
         iEndDate = CmdInput::MenuTime();
         iEndDate -= 3600 * iValue;
         break;

      case 'm':
         break;

      /* case 'n':
         iEndDate = CmdInput::MenuTime();
         break; */

      case 'r':
         iMarkType = THREAD_CHILD;
         break;

      case 's':
         iMessageID = -1;
         break;

      case 't':
      case 'k':
         iMarkType = THREAD_MSGCHILD;
         break;

      case 'w':
      case 'K':
         // iMessageID = -1;
         if(CmdVersion("2.5") >= 0)
         {
            m_pMessageView->Root();
            if(m_pMessageView->Child("replyto", EDFElement::LAST) == true)
            {
               m_pMessageView->Get(NULL, &iMessageID);
            }
            iMarkType = THREAD_MSGCHILD;
         }
         else
         {
            m_pMessageList->Root();
            if(MessageInFolder(m_pMessageList, iMessageID) == true)
            {
               bLoop = true;
               while(bLoop == true)
               {
                  m_pMessageList->Get(&szValue);
                  // printf("SvrInputRequest mark read thread field %s", szValue);
                  if(stricmp(szValue, "folder") == 0)
                  {
                     bLoop = false;
                     // printf("\n");
                  }
                  else
                  {
                     m_pMessageList->Get(NULL, &iMessageID);
                     // printf(", msg %d\n", iMessageID);
                     bLoop = m_pMessageList->Parent();
                  }
                  delete[] szValue;
               }

               iMarkType = THREAD_MSGCHILD;
            }
         }
         break;

      case 'u':
         iEndDate = CmdInput::MenuTime();
         tmTime = localtime((const time_t*)&iEndDate);
         iEndDate -= 3600 * tmTime->tm_hour;
         iEndDate -= 60 * tmTime->tm_min;
         iEndDate -= tmTime->tm_sec;
         break;

      default:
         debug(DEBUGLEVEL_WARN, "MessageMarkMenu no action for %c\n", cOption);
         break;
   }

   if(bAdd == true && iMarkType > 0 && CmdVersion("2.6") >= 0)
   {
      bMarkKeep = CmdYesNo("Stay caught up", false);
   }
   /* if(tolower(cOption) == 'k')
   {
      bMarkKeep = true;
   } */

   // if(iMarkType > 0 || iFromID != -1)
   if(cOption != 'x')
   {
      pRequest = new EDF();
      if(iMessageID != -1)
      {
         pRequest->AddChild("messageid", iMessageID);
         if(CmdVersion("2.6") >= 0)
         {
            pRequest->AddChild("crossfolder", true);
         }
      }
      if(iMarkType > 0)
      {
         pRequest->AddChild("marktype", iMarkType);
         if(bMarkKeep == true)
         {
            pRequest->AddChild("markkeep", true);
         }
      }
      if(iEndDate != -1)
      {
         pRequest->SetChild("enddate", iEndDate);
      }
      if(cOption == 'f' && iFromID != -1)
      {
         pRequest->AddChild("fromid", iFromID);
      }
      if(cOption == 's')
      {
         pRequest->AddChild("subject", szSubject);
      }

      if(iFolderID != -1)
      {
         if(CmdVersion("2.5") < 0 || iMessageID == -1)
         {
            pRequest->AddChild("folderid", iFolderID);
         }
         if(CmdVersion("2.5") >= 0)
         {
            debugEDFPrint("MessageMarkMenu request", pRequest);
         }
         CmdRequest(bAdd == true ? MSG_MESSAGE_MARK_READ : MSG_MESSAGE_MARK_UNREAD, pRequest, &pReply);
         if(CmdVersion("2.5") >= 0)
         {
            debugEDFPrint("MessageMarkMenu reply", pReply);
         }

         /* if(bAdd == false)
         {
            CmdEDFPrint("MessageMarkMenu hold reply", pReply);
         } */

         FolderGet(m_pFolderList, iFolderID, NULL, false);
         // if(iMarkType == 2)
         if(iMessageID == -1 && cOption != 's')
         {
            pReply->GetChild("nummarked", &iNumMarked);
            if(iNumMarked > 0)
            {
               sprintf(szWrite, "\0374%d\0370", iNumMarked);
               if(CmdVersion("2.5") >= 0)
               {
                  pReply->GetChild("nummsgs", &iNumMsgs);
                  sprintf(szWrite, "%s of \0374%d\0370", szWrite, iNumMsgs);
               }
               sprintf(szWrite, "%s messages marked as %s\n", szWrite, bAdd == true ? "read" : "unread");
               CmdWrite(szWrite);

               m_pMessageList->Root();
               bLoop = m_pMessageList->Child("message");
               while(bLoop == true)
               {
                  if(bAdd == true && m_pMessageList->GetChildBool("read") == false)
                  {
                     m_pMessageList->SetChild("read", true);
                  }
                  else if(bAdd == false && m_pMessageList->GetChildBool("read") == true)
                  {
                     m_pMessageList->DeleteChild("read");
                  }
                  bLoop = m_pMessageList->Iterate("message");
               }
            }

            if(bAdd == true)
            {
               iNumUnread = 0;
            }
            else
            {
               iNumUnread = m_pMessageList->Children("message", true);
            }
         }
         else
         {
            m_pFolderList->GetChild("unread", &iNumUnread);

            // printf("MessageMarkMenu pre-mark unread %d\n", iNumUnread);

            bLoop = pReply->Child("messageid");
            while(bLoop == true)
            {
               szFolderName = NULL;

               if(pReply->GetChildBool("action", true) == true)
               {
                  pReply->Get(NULL, &iMessageID);
                  if(pReply->GetChild("folderid", &iFolderMessage) == true && iFolderMessage != iFolderID)
                  {
                     pReply->GetChild("foldername", &szFolderName);

                     m_pFolderList->TempMark();
                     if(FolderGet(m_pFolderList, iFolderMessage) == true)
                     {
                        m_pFolderList->GetChild("unread", &iFolderUnread);
                        if(bAdd == true)
                        {
                           iFolderUnread--;
                        }
                        else
                        {
                           iFolderUnread++;
                        }
                        if(iFolderUnread >= 0)
                        {
                           m_pFolderList->SetChild("unread", iFolderUnread);
                        }
                     }
                     m_pFolderList->TempUnmark();
                  }
                  else
                  {
                     m_pMessageList->Root();
                     if(MessageInFolder(m_pMessageList, iMessageID) == true)
                     {
                        if(bAdd == true && m_pMessageList->GetChildBool("read") == false)
                        {
                           m_pMessageList->SetChild("read", true);
                           iNumUnread--;
                        }
                        else if(bAdd == false && m_pMessageList->GetChildBool("read") == true)
                        {
                           m_pMessageList->DeleteChild("read");
                           iNumUnread++;
                        }
                     }
                  }

                  sprintf(szWrite, "Message \0374%d\0370", iMessageID);
                  if(szFolderName != NULL)
                  {
                     sprintf(szWrite, "%s in \0374%s\0370", szWrite, szFolderName);
                  }
                  sprintf(szWrite, "%s marked as %s\n", szWrite, bAdd == true ? "read" : "unread");
                  CmdWrite(szWrite);

                  delete[] szFolderName;
               }

               bLoop = pReply->Next("messageid");
            }
         }

         debug(DEBUGLEVEL_DEBUG, "MessageMarkMenu resetting unread folder list count to %d\n", iNumUnread);
         m_pFolderList->SetChild("unread", iNumUnread);
         // printf("MessageMarkMenu post-mark unread %d\n", iNumUnread);
      }
      else
      {
         szOption = CmdLineStr("Really perform catch-up", "YES to proceed");
         if(szOption != NULL && stricmp(szOption, "YES") == 0)
         {
            m_pFolderList->Root();
            bLoop = m_pFolderList->Child("folder");
            while(bLoop == true)
            {
               m_pFolderList->Get(NULL, &iFolderID);

               iSubType = 0;
               m_pFolderList->GetChild("subtype", &iSubType);
               m_pFolderList->GetChild("accessmode", &iAccessMode);
               if(mask(iAccessMode, ACCMODE_PRIVATE) == false && iSubType > 0 && m_pFolderList->GetChild("unread", &iNumUnread) == true && iNumUnread > 0)
               {
                  pReply = NULL;

                  pRequest->SetChild("folderid", iFolderID);
                  // CmdEDFPrint("MessageMarkMenu request", pRequest);
                  if(CmdRequest(MSG_MESSAGE_MARK_READ, pRequest, false, &pReply) == true)
                  {
                     bRefresh = true;

                     // CmdEDFPrint("FolderCatchup reply", pReply);
                     pReply->GetChild("nummarked", &iNumMarked);
                     // sprintf(szWrite, "MessageMarkRead %d marked as read\n", iNumMarked);
                     // CmdWrite(szWrite);
                     iTotalMarked += iNumMarked;
                  }
                  /* else
                  {
                  } */

                  delete pReply;
               }

               bLoop = m_pFolderList->Iterate("folder");
            }

            delete pRequest;

            if(bRefresh == true)
            {
               sprintf(szWrite, "\0374%d\0370 message%s marked as read\n", iTotalMarked, iTotalMarked != 1 ? "s" : "");
               CmdWrite(szWrite);

               CmdWrite("Refreshing folder list\n");
               CmdRefreshFolders();
            }
         }
      }
   }

   // CmdRequestLog(false);

   return true;
}

void ContentMenu(EDF *pEDF, const char *szType, int iID)
{
   STACKTRACE
   int iOption = 0, iItemNum = 0, iAttachmentID = -1, iWritten = 0;
   char szWrite[100];
   char *szURL = NULL, *szURI = NULL, *szAttachmentName = NULL, *szFilename = NULL, *szFullPath = NULL;
   bytes *pData = NULL;
   EDF *pRequest = NULL, *pReply = NULL;

   if(CmdContentList(pEDF, false) > 1)
   {
      iOption = CmdLineNum("Content", "RETURN to abort");
   }
   else
   {
      iOption = 1;
   }

   iItemNum = CmdContentList(pEDF, false, iOption, &szURL, &iAttachmentID, &szAttachmentName, &pData);

   debug(DEBUGLEVEL_INFO, "ContentMenu %d -> %d (%s %d)\n", iOption, iItemNum, szURL, iAttachmentID);

   if(szURL != NULL)
   {
      if(CmdBrowser() != NULL)
      {
         if(strnicmp(szURL, "http://", 7) == 0)
         {
            szURI = szURL;
         }
         else
         {
            szURI = new char[strlen(szURL) + 10];
            sprintf(szURI, "http://%s", szURL);
         }

         CmdBrowse(szURI);
      }
      else
      {
         CmdWrite("Browser not set\n");
      }

      if(szURI != szURL)
      {
         delete[] szURI;
      }
      delete[] szURL;
   }
   else if(iAttachmentID != -1)
   {
      if(szAttachmentName == NULL)
      {
         szAttachmentName = new char[20];
         sprintf(szAttachmentName, "%s%d-att%d", szType, iID, iAttachmentID);
      }

      if(m_szAttachmentDir != NULL)
      {
         szFullPath = new char[strlen(m_szAttachmentDir) + strlen(szAttachmentName) + 10];
         strcpy(szFullPath, m_szAttachmentDir);
         if(m_szAttachmentDir[strlen(m_szAttachmentDir) - 1] != CmdDirSep())
         {
            sprintf(szFullPath, "%s%c", szFullPath, CmdDirSep());
         }
         strcat(szFullPath, szAttachmentName);
      }
      else
      {
         szFullPath = strmk(szAttachmentName);
      }

      szFilename = CmdLineStr("Filename", LINE_LEN, 0, szFullPath);
      if(szFilename != NULL)
      {
         if(pData == NULL)
         {
            pRequest = new EDF();
            pRequest->AddChild("messageid", iID);
            pRequest->AddChild("attachmentid", iAttachmentID);
            // CmdEDFPrint("ContentMenu attachment request", pRequest);
            if(CmdRequest(MSG_MESSAGE_LIST, pRequest, &pReply) == true)
            {
               if(pReply->Child("message") == true)
               {
                  if(pReply->Child("attachment") == true)
                  {
                     if(pReply->GetChild("data", &pData) == false)
                     {
                        CmdWrite("No data to write\n");
                     }
                  }
                  else
                  {
                     CmdWrite("No attachment\n");
                  }
               }
               else
               {
                  debug(DEBUGLEVEL_ERR, "ContentMenu no message section\n");
               }
            }
            else
            {
               debug(DEBUGLEVEL_ERR, "ContentMenu no message section\n");
            }

            delete pReply;
         }

         if(pData != NULL)
         {
            iWritten = FileWrite(szFilename, pData->Data(false), pData->Length());
            if(iWritten > 0)
            {
               sprintf(szWrite, "Wrote \0374%d\0370 bytes to \0374%s\0370\n", iWritten, szFilename);
               CmdWrite(szWrite);

               CmdOpen(szFilename);
            }
            else
            {
               sprintf(szWrite, "Write failed, %s\n", strerror(errno));
               CmdWrite(szWrite);
            }

            delete pData;
         }
      }

      delete[] szFilename;
      delete[] szAttachmentName;
      delete[] szFullPath;
   }
}

void MessageJumpMenu()
{
   bool bShowOnly = false;
   char cOption = '\0';
   CmdInput *pInput = NULL;

   // cOption = CmdMenu(FOLDER_JUMP);
   pInput = new CmdInput(0, "Jump (0-9 for ID)");
   pInput->MenuAdd('t', "Top");
   if(CmdInput::MenuCase() == true)
   {
      pInput->MenuAdd('T', "Top (show only)");
   }
   pInput->MenuAdd('b', "Bottom");
   if(CmdInput::MenuCase() == true)
   {
      pInput->MenuAdd('B', "Bottom (show only)");
   }
   if(CmdCurrMessage() != -1)
   {
      pInput->MenuAdd('o', "tOp of thread");
      if(CmdInput::MenuCase() == true)
      {
         pInput->MenuAdd('O', "tOp of thread (show only)");
      }
   }
   pInput->MenuAdd('l', "Latest");
   if(CmdInput::MenuCase() == true)
   {
      pInput->MenuAdd('L', "Latest (show only)");
   }
   // pInput->MenuAdd('r', "Reply number");
   // pInput->MenuAdd('f', "Folder");
   pInput->MenuAdd('x', "eXit", NULL, true);
   pInput->MenuValue('0');
   pInput->MenuValue('1');
   pInput->MenuValue('2');
   pInput->MenuValue('3');
   pInput->MenuValue('4');
   pInput->MenuValue('5');
   pInput->MenuValue('6');
   pInput->MenuValue('7');
   pInput->MenuValue('8');
   pInput->MenuValue('9');
   cOption = CmdMenu(pInput);

   switch(cOption)
   {
      case 'b':
      case 'B':
         m_iMsgPos = MSG_BOTTOM;
         break;

      /* case 'f':
         break; */

      case 'l':
      case 'L':
         m_iMsgPos = MSG_LAST;
         break;

      case 'o':
      case 'O':
         m_iMsgPos = MSG_TOPTHREAD;
         break;

      /* case 'r':
         break; */

      case 't':
      case 'T':
         m_iMsgPos = MSG_TOP;
         break;
   }

   if(cOption >= '0' && cOption <= '9')
   {
      m_iMsgPos = CmdLineNum("Jump to message", 0, 0, cOption - '0');
      CmdMessageGoto(&m_iMsgPos);
   }
   else if(cOption != 'x')
   {
      if(isupper(cOption))
      {
         bShowOnly = true;
      }
      CmdMessageGoto(&m_iMsgPos, bShowOnly);
   }
}

void FolderMenu(int iFolderID, int iMessageID, int iMsgPos)
{
   STACKTRACE
   int iUserID = 0, iAccessLevel = LEVEL_NONE, iInitPos = 0, iVoteType = 0, iFromID = -1, iVoteID = 0;
   int iAccessMode = FOLDERMODE_NORMAL, iStatus = LOGIN_OFF, iRetro = 0, iConfirm = 0, iMessageTop = 0, iValue = 0;
   bool bLoop = true, bAction = false, bRetro = false, bOldMenus = false, bValid = false;
   char cOption = '\0', szWrite[100], szFilename[100];
   char *szFolderName = NULL, *szOption = NULL, *szValue = NULL, *szUsername = NULL, *szFromName = NULL, *szSubject = NULL;
   EDF *pRequest = NULL, *pReply = NULL;

   m_pUser->Get(NULL, &iUserID);
   m_pUser->GetChild("accesslevel", &iAccessLevel);
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      m_pUser->GetChild("retro", &iRetro);
      bRetro = mask(iRetro, RETRO_NAMES);
      bOldMenus = mask(iRetro, RETRO_MENUS);
      // bRetro = CmdRetroNames(m_pUser);
      m_pUser->Parent();
   }

   CmdCurrFolder(iFolderID);
   // m_iPrevFolder = -1;
   CmdCurrMessage(iMessageID);
   m_iMsgPos = iMsgPos;
   iInitPos = iMsgPos;

   debug(DEBUGLEVEL_INFO, "FolderMenu entry %d %d %d\n", CmdCurrFolder(), CmdCurrMessage(), m_iMsgPos);

   if(m_iMsgPos == MSG_NEW)
   {
      bLoop = CmdMessageGoto(&m_iMsgPos);
   }

   while(bLoop == true)
   {
      cOption = CmdMenu(FOLDER);
      switch(cOption)
      {
         case 'a':
            m_iMsgPos = MSG_PARENT;
            CmdMessageGoto(&m_iMsgPos);
            break;

         case 'A':
            iMsgPos = MSG_PARENT;
            CmdMessageGoto(&iMsgPos, true);
            break;

         case 'b':
            m_iMsgPos = MSG_BACK;
            CmdMessageGoto(&m_iMsgPos);
            break;

         case 'B':
            iMsgPos = MSG_BACK;
            CmdMessageGoto(&iMsgPos, true);
            break;

         case 'c':
         case 'C':
            iFromID = -1;
            szSubject = NULL;
            if(m_pMessageView != NULL)
            {
               m_pMessageView->GetChild("fromid", &iFromID);
               m_pMessageView->GetChild("subject", &szSubject);
            }
            MessageMarkMenu(true, CmdCurrFolder(), CmdCurrMessage(), iFromID, szSubject);
            delete[] szSubject;
            break;

         case 'd':
            szOption = CmdLineStr("Note", "RETURN to abort");
            if(szOption != NULL && strcmp(szOption, "") != 0)
            {
               pRequest = new EDF();
               if(CmdVersion("2.5") < 0)
               {
                  pRequest->AddChild("folderid", CmdCurrFolder());
               }
               pRequest->AddChild("messageid", CmdCurrMessage());
               if(CmdVersion("2.5") >= 0)
               {
                  pRequest->Add("attachment", "add");
                  pRequest->AddChild("content-type", MSGATT_ANNOTATION);
                  pRequest->AddChild("text", szOption);
                  pRequest->Parent();
               }
               else
               {
                  pRequest->AddChild("note", szOption);
               }
               if(CmdRequest(MSG_MESSAGE_EDIT, pRequest, &pReply) == false)
               {
                  CmdEDFPrint("FolderMenu annotation failed", pReply);
               }
               delete pReply;
            }
            delete[] szOption;
            break;

         case 'e':
            // FolderGet(m_pFolderList, CmdCurrFolder());
            CmdMessageAdd(-1, CmdCurrFolder());
            delete[] szFolderName;
            break;

         case 'f':
            m_iMsgPos = MSG_FORWARD;
            CmdMessageGoto(&m_iMsgPos);
            break;

         case 'F':
            iMsgPos = MSG_FORWARD;
            CmdMessageGoto(&iMsgPos, true);
            break;

         case 'g':
            if(bOldMenus == true)
            {
               szOption = CmdLineStr("Which message number", "./#/' for first/last/end, RETURN to abort", NUMBER_LEN);
               if(szOption != NULL)
               {
                  if(stricmp(szOption, ".") == 0)
                  {
                     m_iMsgPos = MSG_FIRST;
                     CmdMessageGoto(&m_iMsgPos);
                  }
                  else if(stricmp(szOption, "#") == 0)
                  {
                     m_iMsgPos = MSG_LAST;
                     CmdMessageGoto(&m_iMsgPos);
                  }
                  else if(stricmp(szOption, "'") == 0)
                  {
                    m_iMsgPos = MSG_BOTTOM;
                     CmdMessageGoto(&m_iMsgPos);
                  }
                  else if(stricmp(szOption, "") != 0)
                  {
                     m_iMsgPos = atoi(szOption);
                     CmdMessageGoto(&m_iMsgPos);
                  }
               }
               delete[] szOption;
            }
            else
            {
               m_iMsgPos = MSG_NEW;
               bLoop = CmdMessageGoto(&m_iMsgPos);
            }
            break;

         case 'h':
            if(CmdCurrMessage() != -1 || m_iMsgPos != MSG_NEW && m_iMsgPos != MSG_NEWFOLDER)
            {
               MessageMarkMenu(false, CmdCurrFolder(), CmdCurrMessage());
            }
            else
            {
               CmdCurrMessage(-1);
               if(iInitPos == MSG_EXIT || (iInitPos = MSG_NEW && m_iMsgPos != MSG_NEW))
               {
                  bLoop = false;
               }
               else if(m_iMsgPos == MSG_NEW)
               {
                  m_iMsgPos = MSG_NEWFOLDER;
                  bLoop = CmdMessageGoto(&m_iMsgPos);
               }
               else
               {
                  m_iMsgPos = MSG_EXIT;
               }
               if(bLoop == false)
               {
                  delete m_pMessageView;
                  m_pMessageView = NULL;
               }
            }
            break;

         case 'i':
            FolderEditMenu(CmdCurrFolder());
            break;

         case 'j':
            if(bOldMenus == true)
            {
               m_iMsgPos = MSG_PARENT;
               CmdMessageGoto(&m_iMsgPos);
            }
            else
            {
               MessageJumpMenu();
            }
            break;

         case 'J':
            iMessageID = CmdLineNum("Message ID", 0, NUMBER_LEN, -1);
            CmdMessageRequest(-1, iMessageID);
            break;

         case 'k':
            if(m_pMessageView->Child("replyto", EDFElement::LAST) == true)
            {
               m_pMessageView->Get(NULL, &iMessageTop);
               m_pMessageView->Parent();
            }
            else
            {
               iMessageTop = CmdCurrMessage();
            }
            MessageDeleteMenu(CmdCurrFolder(), CmdCurrMessage(), iMessageTop, m_pMessageView->Children("attachment") > 0);
            break;

         case 'l':
         case 'L':
            CmdMessageRequest(CmdCurrFolder(), CmdCurrMessage(), false, true, NULL, cOption == 'L');
            break;

         /* case 'm':
            MessageMoveMenu(CmdCurrFolder(), CmdCurrMessage());
            break; */

         case 'n':
            if(bOldMenus == true)
            {
               if(m_iMsgPos == MSG_NEW)
               {
                  bLoop = CmdMessageGoto(&m_iMsgPos);
               }
               else
               {
                  m_iMsgPos = MSG_NEXT;
                  CmdMessageGoto(&m_iMsgPos);
               }
            }
            else
            {
               m_iMsgPos = MSG_NEXT;
               CmdMessageGoto(&m_iMsgPos);
            }
            break;

         case 'N':
            iMsgPos = MSG_NEXT;
            CmdMessageGoto(&iMsgPos, true);
            break;

         case 'o':
            if(bOldMenus == true)
            {
               DefaultWholist();
            }
            else
            {
               m_pMessageView->Root();
               m_pMessageView->GetChild("fromid", &iFromID);
               m_pMessageView->GetChild("fromname", &szUsername);
               sprintf(szWrite, "Paging \0374%s\0370...\n", szUsername);
               CmdUserPage(NULL, iFromID);
               delete[] szUsername;
            }
            break;

         case 'p':
            m_iMsgPos = MSG_PREV;
            CmdMessageGoto(&m_iMsgPos);
            break;

         case 'P':
            iMsgPos = MSG_PREV;
            CmdMessageGoto(&iMsgPos, true);
            break;

         case 'r':
            if(CmdCurrMessage() == -1)
            {
               m_iMsgPos = MSG_NEW;
               bLoop = CmdMessageGoto(&m_iMsgPos);
            }
            else
            {
               // szFolderName = NULL;
               szUsername = NULL;
               szFromName = NULL;
               bAction = false;
               iFolderID = CmdCurrFolder();

               FolderGet(m_pFolderList, CmdCurrFolder());
               m_pFolderList->GetChild("replyid", &iFolderID);
               /* if(iReplyID != -1)
               {
                  FolderGet(m_pFolderList, iReplyID);
               }
               else
               {
                  iReplyID = CmdCurrFolder();
               } */

               // szFromName = NULL;
               if(CmdVersion("2.5") >= 0)
               {
                  m_pMessageView->GetChild("replyid", &iFolderID);
               }
               m_pMessageView->GetChild("fromid", &iFromID);
               m_pMessageView->GetChild("fromname", &szFromName);
               m_pFolderList->GetChild("accessmode", &iAccessMode);

               if(mask(iAccessMode, ACCMODE_PRIVATE) == true)
               {
                  UserGet(m_pUserList, iFromID, &szUsername, false);
                  m_pUserList->GetChild("status", &iStatus);

                  m_pUser->Root();
                  if(m_pUser->Child("client", CLIENT_NAME()) == true)
                  {
                     m_pUser->GetChild("confirm", &iConfirm);
                     m_pUser->Parent();
                  }
                  if(mask(iConfirm, CONFIRM_ACTIVE_REPLY) == true && mask(iStatus, LOGIN_ON) == true && mask(iStatus, LOGIN_BUSY) == false && mask(iStatus, LOGIN_NOCONTACT) == false && mask(iStatus, LOGIN_SHADOW) == false)
                  {
                     sprintf(szWrite, "\0374%s\0370 is active. Use paging instead", RETRO_NAME(szUsername));
                     if(CmdYesNo(szWrite, true) == true)
                     {
                        CmdUserPage(NULL, iFromID, false, szFromName);
                        bAction = true;
                     }
                  }
               }

               if(bAction == false)
               {
                  CmdMessageAdd(CmdCurrMessage(), iFolderID, CmdCurrFolder(), iFromID, szFromName);
               }

               // delete[] szFolderName;
               delete[] szUsername;
               delete[] szFromName;
            }
            break;

         case 's':
            MessageSearchMenu(CmdCurrFolder(), m_iFromID, false);
            break;

         case 't':
            /* bLoop = m_pMessageView->Child("replyto");
            while(bLoop == true)
            {
               m_pMessageView->Get(NULL, &iMessageTop);
               bLoop = m_pMessageView->Next("replyto");
               if(bLoop == false)
               {
                  m_pMessageView->Parent();
               }
            } */
            if(m_pMessageView->Child("replyto", EDFElement::LAST) == true)
            {
               m_pMessageView->Get(NULL, &iMessageTop);
               m_pMessageView->Parent();
            }
            else
            {
               iMessageTop = CmdCurrMessage();
            }
            MessageMoveMenu(CmdCurrFolder(), CmdCurrMessage(), iMessageTop);
            break;

         case 'u':
            if(CmdFolderLeave(CmdCurrFolder()) == true)
            {
               bLoop = false;
            }
            break;

         case 'v':
            if(CmdVersion("2.5") >= 0)
            {
               if(m_pMessageView->Child("votes") == true)
               {
                  m_pMessageView->GetChild("votetype", &iVoteType);
                  m_pMessageView->Parent();
               }

               /* if(mask(iVoteType, VOTE_CHOICE) == true)
               {
                  if(CmdYesNo("Vote anonymously", true) == false)
                  {
                     pRequest->AddChild("votetype", VOTE_NAMED);
                  }
               } */
            }

            szOption = NULL;
            bValid = false;
            if(IS_VALUE_VOTE(iVoteType) == true)
            {
               szOption = CmdLineStr("Vote value");
               if(szOption != NULL && strcmp(szOption, "") != 0)
               {
                  bValid = true;
               }
            }
            else
            {
               iVoteID = CmdLineNum("Vote ID", 0, -1);
               if(iVoteID > 0)
               {
                  bValid = true;
               }
               else
               {
                  CmdWrite("Invalid number\n");
               }
            }

            if(bValid == true)
            {
               pRequest = new EDF();
               if(CmdVersion("2.5") < 0)
               {
                  pRequest->AddChild("folderid", CmdCurrFolder());
               }
               pRequest->AddChild("messageid", CmdCurrMessage());
               if(IS_INT_VOTE(iVoteType) == true)
               {
                  pRequest->AddChild("votevalue", atoi(szOption));
               }
               else if(IS_FLOAT_VOTE(iVoteType) == true)
               {
                  pRequest->AddChild("votevalue", atof(szOption));
               }
               else if(mask(iVoteType, VOTE_STRVALUES) == true)
               {
                  pRequest->AddChild("votevalue", szOption);
               }
               else
               {
                  pRequest->AddChild("voteid", iVoteID);
               }
               debugEDFPrint("FolderMenu vote", pRequest);
               if(CmdRequest(MSG_MESSAGE_VOTE, pRequest, &pReply) == false)
               {
                  CmdEDFPrint("Voting failed", pReply);
                  delete pReply;
               }
            }
            delete[] szOption;
            break;

         case 'V':
            if(CmdYesNo("Really close vote", false) == true)
            {
               m_pMessageView->Child("votes");
               m_pMessageView->GetChild("votetype", &iVoteType);
               iVoteType |= VOTE_CLOSED;
               pRequest = new EDF();
               pRequest->AddChild("messageid", CmdCurrMessage());
               pRequest->AddChild("votetype", iVoteType);
               if(CmdRequest(MSG_MESSAGE_EDIT, pRequest, &pReply) == false)
               {
                  CmdEDFPrint("FolderMenu vote close failed", pReply);
                  delete pReply;
               }
            }
            break;

         case 'w':
            DefaultWholist();
            break;

         case 'x':
            if(CmdCurrMessage() == -1)
            {
               bLoop = false;
            }
            else
            {
               CmdCurrMessage(-1);
               if(iInitPos == MSG_EXIT || (iInitPos = MSG_NEW && m_iMsgPos != MSG_NEW))
               {
                  bLoop = false;
               }
               else if(m_iMsgPos == MSG_NEW)
               {
                  m_iMsgPos = MSG_NEWFOLDER;
                  bLoop = CmdMessageGoto(&m_iMsgPos);
               }
               else
               {
                  m_iMsgPos = MSG_EXIT;
               }
            }
            if(bLoop == false)
            {
               delete m_pMessageView;
               m_pMessageView = NULL;
            }
            break;

         case 'y':
            sprintf(szFilename, "msg-%d", CmdCurrMessage());
            szOption = CmdLineStr("Filename", LINE_LEN, 0, szFilename);
            if(szOption != NULL)
            {
               if(CmdLogOpen(szOption) == true)
               {
                  m_pMessageView->Root();

                  // EDFPrint("FolderMenu current message list", m_pMessageList);

                  m_pMessageList->Root();
                  m_pMessageList->GetChild("foldername", &szValue);

                  // if(m_pMessageView->Child("message") == true)
                  {
                     m_pUser->Root();
                     CmdMessageView(m_pMessageView, CmdCurrFolder(), szValue, MessagePos(m_pMessageView), MessageCount(), false);
                  }

                  CmdLogClose();

                  delete[] szValue;
               }
               else
               {
                  CmdWrite("Unable to open file\n");
               }
            }
            delete[] szOption;
            break;

         case 'z':
            ContentMenu(m_pMessageView, "msg", CmdCurrMessage());
            break;

         case '/':
            TalkCommandMenu(false, false, true, false, false, false);
            break;

         default:
            debug(DEBUGLEVEL_ERR, "FolderMenu no processing for option '%c'\n", cOption);
            break;
      }
   }
}

bool FolderJoinMenu(int iFolderID)
{
   STACKTRACE
   int iJoin = 0, iNumUnread = 0;

   if(iFolderID == -1)
   {
      iFolderID = CmdLineFolder();
   }

   if(iFolderID == -1)
   {
      return false;
   }

   iJoin = CmdFolderJoin(iFolderID);

   if(iJoin == 0)
   {
      return false;
   }

   CmdRefreshMessages(iFolderID);

   // m_pMessageList->GetChild("unread", &iNumUnread);
   // EDFPrint("FolderJoinMenu", m_pMessageList);
   iNumUnread = m_pMessageList->Children("message", true) - m_pMessageList->Children("read", true);
   // printf("FolderJoinMenu %d unread\n", iNumUnread);
   FolderMenu(iFolderID, -1, iJoin == 1 && iNumUnread > 0 ? MSG_NEW : MSG_JUMP);

   if(iJoin == 2)
   {
      CmdFolderLeave(iFolderID);
   }

   return true;
}

bool UserEditDetail(EDF *pRequest, char *szTitle, char *szName, EDF *pUser)
{
   STACKTRACE
   bool bReturn = false;
   char *szValue = NULL, *szOption = NULL, *szType = NULL;

   if(pUser->Child("details") == true)
   {
      pUser->GetChild(szName, &szValue);
      pUser->Parent();
   }

   szOption = CmdLineStr(szTitle, LINE_LEN, 0, szValue);

   if(szOption != NULL && ((szValue == NULL && strcmp(szOption, "") != 0) || (szValue != NULL && strcmp(szOption, szValue) != 0)))
   {
      if(stricmp(szOption, "") == 0)
      {
         szType = "delete";

         delete[] szOption;
         szOption = NULL;
      }
      else
      {
         szType = "edit";
      }

      if(pRequest->Child("details", szType) == false)
      {
         pRequest->Add("details", szType);
      }
      if(szOption != NULL)
      {
         pRequest->SetChild(szName, szOption);
         pUser->SetChild(szName, szOption);

         delete[] szOption;
      }
      else
      {
         pRequest->SetChild(szName);
         pUser->DeleteChild(szName);
      }

      pRequest->Parent();

      bReturn = true;
   }

   delete[] szValue;

   return bReturn;
}

bool UserFolderMenu(EDF *pUser, int iEditID)
{
   STACKTRACE
   int iFolderID = 0, iRetro = 0, iNumSubs = 0;
   bool bRetro = false;
   char cOption = '\0';
   char *szRequest = NULL;
   CmdInput *pInput = NULL;
   EDF *pRequest = NULL, *pReply = NULL;

   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      m_pUser->GetChild("retro", &iRetro);
      bRetro = mask(iRetro, RETRO_NAMES);
      m_pUser->Parent();
   }

   debugEDFPrint("UserFolderMenu", pUser);
   if(pUser->Child("folders") == true)
   {
      iNumSubs = CmdSubList(pUser, SUBTYPE_EDITOR, SUBINFO_USER, NULL, bRetro);
      iNumSubs += CmdSubList(pUser, SUBTYPE_MEMBER, SUBINFO_USER, NULL, bRetro);
      iNumSubs += CmdSubList(pUser, SUBTYPE_SUB, SUBINFO_USER, NULL, bRetro);
      if(iNumSubs > 0)
      {
         CmdWrite("\n");
      }
      pUser->Parent();
   }

   while(cOption != 'x')
   {
      pInput = new CmdInput(CMD_MENU_NOCASE, "Folders");
      pInput->MenuAdd('s', "add Subscription");
      pInput->MenuAdd('r', "Remove");
      pInput->MenuAdd('m', "add Membership");
      pInput->MenuAdd('e', "add Editorship");
      pInput->MenuAdd('l', "List folders");
      pInput->MenuAdd('x', "eXit", NULL, true);

      cOption = CmdMenu(pInput);
      if(cOption == 'l')
      {
         if(pUser->Child("folders") == true)
         {
            iNumSubs =CmdSubList(pUser, SUBTYPE_EDITOR, SUBINFO_USER, NULL, bRetro);
            iNumSubs += CmdSubList(pUser, SUBTYPE_MEMBER, SUBINFO_USER, NULL, bRetro);
            iNumSubs += CmdSubList(pUser, SUBTYPE_SUB, SUBINFO_USER, NULL, bRetro);
            if(iNumSubs > 0)
            {
               CmdWrite("\n");
            }
            pUser->Parent();
         }
      }
      else if(cOption != 'x')
      {
         iFolderID = CmdLineFolder();
         if(iFolderID != -1)
         {
            pRequest = new EDF();
            pRequest->AddChild("folderid", iFolderID);
            pRequest->AddChild("userid", iEditID);
            szRequest = MSG_FOLDER_SUBSCRIBE;
            switch(cOption)
            {
               case 'e':
                  pRequest->AddChild("subtype", SUBTYPE_EDITOR);
                  break;

               case 'm':
                  pRequest->AddChild("subtype", SUBTYPE_MEMBER);
                  break;

               case 'r':
                  szRequest = MSG_FOLDER_UNSUBSCRIBE;
                  break;
            }
            CmdEDFPrint("UserFolderEdit request", pRequest);
            CmdRequest(szRequest, pRequest, &pReply);
            CmdEDFPrint("UserFolderEdit reply", pReply);
            delete pReply;
         }
      }
   }

   return true;
}

void CmdColourSet(EDF *pRequest, int iColour, char *szUse, EDF *pUser)
{
   char szColour[100];
   int cColour = '\0';
   CmdInput *pInput = NULL;

   if(iColour >= 0)
   {
      sprintf(szColour, "Colour %d (used for %s)", iColour, szUse);
   }
   else
   {
      strcpy(szColour, "Background colour");
   }
   pInput = new CmdInput(CMD_MENU_NOCASE, szColour);
   pInput->MenuAdd('a', "blAck");
   pInput->MenuAdd('b', "Blue");
   pInput->MenuAdd('g', "Green");
   pInput->MenuAdd('c', "Cyan");
   pInput->MenuAdd('r', "Red");
   pInput->MenuAdd('m', "Magenta");
   pInput->MenuAdd('y', "Yellow");
   pInput->MenuAdd('w', "White");
   pInput->MenuAdd('f', "deFault", NULL, true);

   pRequest->Root();
   if(iColour >= 0)
   {
      sprintf(szColour, "colour%d", iColour);
   }
   else
   {
      strcpy(szColour, "colourbg");
   }
   pUser->Root();
   if(pUser->Child("client", CLIENT_NAME()) == false)
   {
      pUser->Add("client", CLIENT_NAME());
   }

   pUser->GetChild(szColour, &cColour);
   if(cColour != '\0')
   {
      pInput->MenuDefault(cColour);
   }

   cColour = CmdMenu(pInput);
   debug("CmdColourSet col %d\n", cColour);

   if(cColour == 'f')
   {
      if(pRequest->Child("client", "delete") == false)
      {
         pRequest->Add("client", "delete");
      }
      pRequest->SetChild(szColour);
      pUser->DeleteChild(szColour);
   }
   else
   {
      if(cColour != 'a')
      {
         pInput = new CmdInput(CMD_MENU_SIMPLE | CMD_MENU_NOCASE, "Shade");
         pInput->MenuAdd('d', "Dark");
         pInput->MenuAdd('b', "Bright");
         if(CmdMenu(pInput) == 'b')
         {
            cColour = toupper(cColour);
            debug("CmdColourSet toupper %d\n", cColour);
         }
      }

      if(pRequest->Child("client", CmdVersion("2.3") >= 0 ? "edit" : "set") == false)
      {
         pRequest->Add("client", CmdVersion("2.3") >= 0 ? "edit" : "set");
      }
      pRequest->SetChild(szColour, (int)cColour);

      pUser->SetChild(szColour, (int)cColour);
   }

   pRequest->Parent();
   pUser->Parent();
}

bool WholistDefaultMenu(EDF *pUser, EDF *pRequest)
{
   int iWhoType = 0;

   iWhoType = CmdWholistInput();

   if(iWhoType != -1)
   {
      pRequest->Root();
      if(pRequest->Child("client", CmdVersion("2.3") >= 0 ? "edit" : "set") == false)
      {
         pRequest->Add("client", CmdVersion("2.3") >= 0 ? "edit" : "set");
      }
      pRequest->SetChild("wholist", iWhoType);
      pRequest->Parent();

      pUser->Root();
      if(pUser->Child("client", CLIENT_NAME()) == false)
      {
         pUser->Add("client", CLIENT_NAME());
      }
      pUser->SetChild("wholist", iWhoType);
      pUser->Parent();
   }

   return true;
}

void CmdOptionMenu(CmdInput *pInput, char cOption, bool bOption, const char *szOption)
{
   char szWrite[200];

   sprintf(szWrite, "toggle %s (currently \0374%s\0370)", szOption, bOption == true ? "on" : "off");
   pInput->MenuAdd(cOption, szWrite);
}

bool CmdOptionToggle(bool bValue, const char *szValue)
{
   char szWrite[200];

   bValue = !bValue;

   sprintf(szWrite, "%s is now \0374%s\0370\n", szValue, bValue == true ? "on" : "off");
   CmdWrite(szWrite);

   return bValue;
}

int CmdAnnounceToggle(int iValue, int iMask, const char *szValue)
{
   char szWrite[200];

   if(mask(iValue, iMask) == true)
   {
      iValue -= iMask;
   }
   else
   {
      iValue += iMask;
   }

   sprintf(szWrite, "%s is now \0374%s\0370\n", szValue, mask(iValue, iMask) == true ? "on" : "off");
   CmdWrite(szWrite);

   return iValue;
}

void AlertsMenu(EDF *pUser, EDF *pRequest, int iAccessLevel)
{
   int iAnnType = 0, iOldType = 0;
   bool bFakePage = false, bOldPage = false, bDevOption = false;
   char cOption = '\0';
   // char szWrite[200];
   CmdInput *pInput = NULL;

   // sprintf(szWrite, "toggle hArd line wrap (currently \0374%s\0370)", bHardWrap == true ? "on" : "off");

   if(pUser->Child("client", CLIENT_NAME()) == true)
   {
      pUser->GetChild("anntype", &iAnnType);
      bDevOption = pUser->GetChildBool("devoption");
      bFakePage = pUser->GetChildBool("fakepage");
      pUser->Parent();
   }

   iOldType = iAnnType;
   bOldPage = bFakePage;

   while(cOption != 'x')
   {
      pInput = new CmdInput(CMD_MENU_NOCASE, "Alerts");

      CmdOptionMenu(pInput, 'p', mask(iAnnType, ANN_PAGEBELL), "Page bell");
      CmdOptionMenu(pInput, 'f', mask(iAnnType, ANN_FOLDERCHECK), "Folder checking");
      CmdOptionMenu(pInput, 'u', mask(iAnnType, ANN_USERCHECK), "User checking");
      if(iAccessLevel >= LEVEL_WITNESS)
      {
         CmdOptionMenu(pInput, 'a', mask(iAnnType, ANN_ADMINCHECK), "Admin checking");
      }
      if(iAccessLevel >= LEVEL_WITNESS || bDevOption == true)
      {
         CmdOptionMenu(pInput, 'd', mask(iAnnType, ANN_DEBUGCHECK), "Debug checking");
      }

      CmdOptionMenu(pInput, 'b', mask(iAnnType, ANN_BUSYCHECK), "announcements when Busy");

      CmdOptionMenu(pInput, 'e', mask(iAnnType, ANN_EXTRACHECK), "Extra fields");

      CmdOptionMenu(pInput, 'i', bFakePage, "Instant private messages");

      pInput->MenuAdd('x', "eXit");

      cOption = CmdMenu(pInput);

      switch(cOption)
      {
         case 'p':
            iAnnType = CmdAnnounceToggle(iAnnType, ANN_PAGEBELL, "Page bell");
            break;

         case 'f':
            iAnnType = CmdAnnounceToggle(iAnnType, ANN_FOLDERCHECK, "Folder checking");
            break;

         case 'u':
            iAnnType = CmdAnnounceToggle(iAnnType, ANN_USERCHECK, "User checking");
            break;

         case 'a':
            iAnnType = CmdAnnounceToggle(iAnnType, ANN_ADMINCHECK, "Admin checking");
            break;

         case 'd':
            iAnnType = CmdAnnounceToggle(iAnnType, ANN_DEBUGCHECK, "Debug checking");
            break;

         case 'b':
            iAnnType = CmdAnnounceToggle(iAnnType, ANN_BUSYCHECK, "Announcements when busy");
            break;

         case 'e':
            iAnnType = CmdAnnounceToggle(iAnnType, ANN_EXTRACHECK, "Extra fields");
            break;

         case 'i':
            bFakePage = CmdOptionToggle(bFakePage, "Instant private messages");
            break;
      }
   }

   if(iAnnType != iOldType || bFakePage != bOldPage)
   {
      if(pUser->Child("client", CLIENT_NAME()) == false)
      {
         pUser->Add("client", CLIENT_NAME());
      }
      if(iAnnType != iOldType)
      {
         pUser->SetChild("anntype", iAnnType);
      }
      if(bFakePage != bOldPage)
      {
         pUser->SetChild("fakepage", bFakePage);
      }
      pUser->Parent();

      if(pRequest->Child("client", CmdVersion("2.3") >= 0 ? "edit" : "set") == false)
      {
         pRequest->Add("client", CmdVersion("2.3") >= 0 ? "edit" : "set");
      }
      if(iAnnType != iOldType)
      {
         pRequest->SetChild("anntype", iAnnType);
      }
      if(bFakePage != bOldPage)
      {
         pRequest->SetChild("fakepage", bFakePage);
      }
      pRequest->Parent();
   }
}

bool UserEditMenu(int iEditID)
{
   STACKTRACE
   bool bLoop = true;
   int iAccessLevel = LEVEL_NONE, iUserID = 0, iMenuLevel = -1, iHighlight = -1, iID = 0, iMarking = 0;
   int iValue = 0, iRetro = 0, iType = 0, iConfirm = 0, iHardWrap = 0, iUserLevel = LEVEL_NONE, iOwnerID = -1;
   char cOption = '\0';
   char *szDesc = NULL, *szPassword1 = NULL, *szPassword2 = NULL, *szOption = NULL;
   CmdInput *pInput = NULL;
   EDF *pRequest = NULL, *pReply = NULL, *pUser = NULL, *pKill = NULL;

   m_pUser->Root();
   m_pUser->Get(NULL, &iUserID);
   m_pUser->GetChild("accesslevel", &iAccessLevel);
   if(m_pUser->Child("folders") == true)
   {
      m_pUser->GetChild("marking", &iMarking);
      m_pUser->Parent();
   }

   // pRequest = new EDF();
   if(iEditID != -1)
   {
      pRequest = new EDF();
      pRequest->AddChild("userid", iEditID);
      if(CmdRequest(MSG_USER_LIST, pRequest, &pReply) == false)
      {
         CmdWrite("No such user\n");
         return false;
      }

      if(pReply->Child("user") == true)
      {
         pReply->GetChild("accesslevel", &iUserLevel);
         pReply->GetChild("ownerid", &iOwnerID);

         debug(DEBUGLEVEL_DEBUG, "UserEditMenu access check %d >= %d, %d != %d\n", iUserLevel, iAccessLevel, iOwnerID, iUserID);
         if(iUserLevel >= iAccessLevel && iOwnerID != iUserID && !(iAccessLevel >= LEVEL_WITNESS && iUserID == iEditID))
         {
            CmdWrite("Access denied\n");
            delete pReply;
            return false;
         }

         pUser = new EDF();
         pUser->Copy(pReply, false, true);
         pUser->Set("user", iEditID);
      }

      delete pReply;
   }
   else
   {
      pUser = new EDF();
      pUser->Copy(m_pUser, false, true);
      pUser->Set("user", iUserID);
   }

   /* if(pUser->Child("folders") == true)
   {
      pUser->Sort(NULL, "name");
      pUser->Parent();
   } */

   // EDFPrint("UserEditMenu user", pUser);
   CmdPageOn();
   CmdUserView(pUser);
   CmdPageOff();

   pRequest = new EDF();

   while(bLoop == true)
   {
      pUser->Root();
      pInput = CmdUserEdit(pUser, iUserID, iAccessLevel, iEditID);

      pRequest->Root();
      cOption = CmdMenu(pInput);
      // cOption = CmdMenu(USER_EDIT);
      switch(cOption)
      {
         case 'a':
            if(iEditID == -1)
            {
               // CmdOptionSet(pUser, pRequest, "Hard line wrapping", "hardwrap");
               iHardWrap = 0;
               iValue = 0;
               pUser->Root();
               if(pUser->Child("client", CLIENT_NAME()) == true)
               {
                  pUser->GetChild("hardwrap", &iHardWrap);
               }

               if(CmdYesNo("Hard wrap in editor", mask(iHardWrap, CMD_WRAP_EDIT)) == true)
               {
                  iValue += CMD_WRAP_EDIT;
               }
               if(CmdYesNo("Hard wrap viewing text", mask(iHardWrap, CMD_WRAP_VIEW)) == true)
               {
                  iValue += CMD_WRAP_VIEW;
               }

               if(iHardWrap != iValue)
               {
                  pUser->SetChild("hardwrap", iValue);
                  pUser->Parent();

                  pRequest->Root();
                  if(pRequest->Child("client", CmdVersion("2.3") >= 0 ? "edit" : "set") == false)
                  {
                     pRequest->Add("client", CmdVersion("2.3") >= 0 ? "edit" : "set");
                  }
                  pRequest->SetChild("hardwrap", iValue);
               }
            }
            else
            {
               pInput = new CmdInput(CMD_MENU_NOCASE, "Access");
               pInput->MenuAdd('n', "None");
               pInput->MenuAdd('g', "Guest");
               pInput->MenuAdd('m', "Messages");
               pInput->MenuAdd('e', "Editor");
               if(iAccessLevel >= LEVEL_SYSOP)
               {
                  pInput->MenuAdd('w', "Witness");
               }
               // pInput->MenuAdd('a', "Agent");
               pInput->MenuAdd('x', "eXit", NULL, true);
               cOption = CmdMenu(pInput);
               switch(cOption)
               {
                  case 'n':
                     pUser->SetChild("accesslevel", LEVEL_NONE);
                     pRequest->SetChild("accesslevel", LEVEL_NONE);
                     break;

                  case 'g':
                     pUser->SetChild("accesslevel", LEVEL_GUEST);
                     pRequest->SetChild("accesslevel", LEVEL_GUEST);
                     break;

                  case 'm':
                     pUser->SetChild("accesslevel", LEVEL_MESSAGES);
                     pRequest->SetChild("accesslevel", LEVEL_MESSAGES);
                     break;

                  case 'e':
                     pUser->SetChild("accesslevel", LEVEL_EDITOR);
                     pRequest->SetChild("accesslevel", LEVEL_EDITOR);
                     break;

                  case 'w':
                     pUser->SetChild("accesslevel", LEVEL_WITNESS);
                     pRequest->SetChild("accesslevel", LEVEL_WITNESS);
                     break;

                  case 'a':
                     pUser->DeleteChild("accesslevel");
                     pUser->SetChild("usertype", 1);
                     pRequest->DeleteChild("accesslevel");
                     pRequest->SetChild("usertype", 1);
                     break;
               }
            }
            break;

         /* case 'b':
            CmdAnnounceToggle(pUser, pRequest, ANN_PAGEBELL);
            break;

         case 'c':
            CmdAnnounceToggle(pUser, pRequest, ANN_BUSYCHECK);
            break; */

         /* case 'd':
            SvrInputSetup(pConn, m_pUser, DETAILS_REALNAME);
            break; */

         case 'c':
            if(iEditID == -1)
            {
               iConfirm = 0;
               iValue = 0;
               pUser->Root();
               if(pUser->Child("client", CLIENT_NAME()) == true)
               {
                  pUser->GetChild("confirm", &iConfirm);
               }

               if(CmdYesNo("Pager on when paging if busy", mask(iConfirm, CONFIRM_BUSY_PAGER)) == true)
               {
                  iValue += CONFIRM_BUSY_PAGER;
               }
               if(CmdYesNo("Use paging when replying to active user", mask(iConfirm, CONFIRM_ACTIVE_REPLY)) == true)
               {
                  iValue += CONFIRM_ACTIVE_REPLY;
               }
#ifdef HAVE_LIBASPELL
               if(CmdYesNo("Use spell check in editor", mask(iConfirm, CONFIRM_SPELL_CHECK)) == true)
               {
                  iValue += CONFIRM_SPELL_CHECK;
               }
#else
               if(mask(iConfirm, CONFIRM_SPELL_CHECK) == true)
               {
                  iValue += CONFIRM_SPELL_CHECK;
               }
#endif

               if(iConfirm != iValue)
               {
                  pUser->SetChild("confirm", iValue);
                  pUser->Parent();

                  pRequest->Root();
                  if(pRequest->Child("client", CmdVersion("2.3") >= 0 ? "edit" : "set") == false)
                  {
                     pRequest->Add("client", CmdVersion("2.3") >= 0 ? "edit" : "set");
                  }
                  pRequest->SetChild("confirm", iValue);
               }
            }
            else
            {
               szOption = CmdLineStr("Access name", ". for default", UA_NAME_LEN);

               if(szOption != NULL)
               {
                  if(stricmp(szOption, ".") == 0)
                  {
                     delete[] szOption;
                     szOption = NULL;
                  }

                  pUser->SetChild("accessname", szOption);
                  pRequest->SetChild("accessname", szOption);

                  delete[] szOption;
               }
            }
            break;

         case 'd':
            UserEditDetail(pRequest, "Real name", "realname", pUser);
            UserEditDetail(pRequest, "Email", "email", pUser);
            UserEditDetail(pRequest, "SMS", "sms", pUser);
            UserEditDetail(pRequest, "Homepage", "homepage", pUser);
            UserEditDetail(pRequest, "Picture", "picture", pUser);
            break;

         case 'e':
            iHighlight = 0;
            if(pUser->Child("client", CLIENT_NAME()) == true)
            {
               pUser->GetChild("highlight", &iHighlight);
               pUser->Parent();
            }

            pInput = new CmdInput(CMD_MENU_SIMPLE | CMD_MENU_NOCASE, "Highlight level");
            pInput->MenuAdd('n', "None", NULL, iHighlight == 0);
            pInput->MenuAdd('b', "Bold", NULL, iHighlight == 1);
            pInput->MenuAdd('c', "Colour", NULL, iHighlight == 2);
            pInput->MenuAdd('u', "cUstom colour", NULL, iHighlight == 3);

            if(pRequest->Child("client", CmdVersion("2.3") >= 0 ? "edit" : "set") == false)
            {
               pRequest->Add("client", CmdVersion("2.3") >= 0 ? "edit" : "set");
            }

            iValue = iHighlight;
            cOption = CmdMenu(pInput);
            switch(cOption)
            {
               case 'n':
                  iHighlight = 0;
                  break;

               case 'b':
                  iHighlight = 1;
                  break;

               case 'c':
                  iHighlight = 2;
                  break;

               case 'u':
                  iHighlight = 3;
                  break;
            }

            if(iValue != iHighlight || cOption == 'u')
            {
               pRequest->SetChild("highlight", iHighlight);
               if(cOption == 'u')
               {
                  CmdColourSet(pRequest, 0, "normal text", pUser);
                  CmdColourSet(pRequest, 1, "Self info", pUser);
                  CmdColourSet(pRequest, 2, "SysOp level", pUser);
                  CmdColourSet(pRequest, 3, "item info, guests / message level", pUser);
                  CmdColourSet(pRequest, 4, "menu / witness level", pUser);
                  CmdColourSet(pRequest, 5, "agents", pUser);
                  CmdColourSet(pRequest, 6, "announcements / editor level", pUser);
                  CmdColourSet(pRequest, 7, "list headers / footers, item IDs", pUser);
                  // CmdColourSet(pRequest, 8, "unused", pUser);

                  CmdColourSet(pRequest, -1, NULL, pUser);
               }
            }
            break;

         /* case 'f':
            CmdAnnounceToggle(pUser, pRequest, ANN_FOLDERCHECK);
            break; */

         case 'f':
            if(iEditID != -1)
            {
               UserFolderMenu(pUser, iEditID);
            }
            else
            {
               WholistDefaultMenu(pUser, pRequest);
            }
            break;

         case 'g':
            pInput = new CmdInput(CMD_MENU_SIMPLE | CMD_MENU_NOCASE, "Gender");
            pInput->MenuAdd('m', "Male");
            pInput->MenuAdd('f', "Female");
            pInput->MenuAdd('p', "Person");
            pInput->MenuAdd('n', "None");
            cOption = CmdMenu(pInput);
            switch(cOption)
            {
               case 'm':
                  pRequest->SetChild("gender", GENDER_MALE);
                  pUser->SetChild("gender", GENDER_MALE);
                  break;

               case 'f':
                  pRequest->SetChild("gender", GENDER_FEMALE);
                  pUser->SetChild("gender", GENDER_FEMALE);
                  break;

               case 'p':
                  pRequest->SetChild("gender", GENDER_PERSON);
                  pUser->SetChild("gender", GENDER_PERSON);
                  break;

               case 'n':
                  pRequest->SetChild("gender", GENDER_NONE);
                  pUser->SetChild("gender", GENDER_NONE);
                  break;
            }
            break;

         case 'h':
            CmdValueSet(pUser, pRequest, "Height (20-60)", "height", 3, 20, 60);
            /* iValue = CmdLineNum("Height (20-60)");
            if(iValue >= 20 && iValue <= 60)
            {
               if(pUser->Child("client", CLIENT_NAME()) == false)
               {
                  pUser->Add("client", CLIENT_NAME());
               }
               if(pRequest->Child("client", "edit") == false)
               {
                  pRequest->Add("client", "edit");
               }

               pUser->SetChild("height", iValue);
               pRequest->SetChild("height", iValue);

               pUser->Parent();
               pRequest->Parent();
            } */
            break;

         /* case 'i':
            CmdAnnounceToggle(pUser, pRequest, ANN_DEBUGCHECK);
            break; */

         case 'i':
            CmdOptionSet(pUser, pRequest, "Extra return on header / footer", "doublereturn", true);
            CmdOptionSet(pUser, pRequest, "Show custom access levels", "accessnames", true, true);
            CmdOptionSet(pUser, pRequest, "Use slim headers in message view", "slimheaders", true, false);
            if(CmdLocal() == false)
            {
               CmdOptionSet(pUser, pRequest, "Use UTF-8", "utf8", true, false);
            }
            break;

         case 'l':
            CmdUserView(pUser);
            break;

         case 'k':
            if(iEditID == -1)
            {
               iMarking = 0;
               pUser->Root();
               if(pUser->Child("folders") == true)
               {
                  pUser->GetChild("marking", &iMarking);
               }
               else
               {
                  pUser->Add("folders");
               }
               debug(DEBUGLEVEL_DEBUG, "UserEditMenu message marking %d\n", iMarking);

               iValue = 0;

               if(CmdYesNo("Read for adds (by you) in private folder", mask(iMarking, MARKING_ADD_PRIVATE)) == true)
               {
                  iValue |= MARKING_ADD_PRIVATE;
               }
               if(CmdYesNo("Read for adds (by you) in public folder", mask(iMarking, MARKING_ADD_PUBLIC)) == true)
               {
                  iValue |= MARKING_ADD_PUBLIC;
               }
               if(CmdYesNo("Unread for edits (by anyone) in private folder", mask(iMarking, MARKING_EDIT_PRIVATE)) == true)
               {
                  iValue |= MARKING_EDIT_PRIVATE;
               }
               if(CmdYesNo("Unread for edits (by anyone) in public folder", mask(iMarking, MARKING_EDIT_PUBLIC)) == true)
               {
                  iValue |= MARKING_EDIT_PUBLIC;
               }

               debug(DEBUGLEVEL_DEBUG, "UserEditMenu new marking %d\n", iValue);

               if(iValue != iMarking)
               {
                  pUser->SetChild("marking", iValue);

                  if(pRequest->Child("folders") == false)
                  {
                     pRequest->Add("folders");
                  }
                  pRequest->SetChild("marking", iValue);
                  pRequest->Parent();
               }
               pUser->Parent();
            }
            else
            {
               // if(CmdYesNo("Really kill user", false) == true)
               szOption = CmdLineStr("Really kill user", "YES to proceed");
               if(szOption != NULL && strcmp(szOption, "YES") == 0)
               {
                  pRequest->Root();
                  while(pRequest->DeleteChild() == true);

                  pKill = new EDF();
                  pKill->AddChild("userid", iEditID);
                  CmdRequest(MSG_USER_DELETE, pKill);

                  bLoop = false;
               }

               delete[] szOption;
            }
            break;

         case 'm':
            pInput = new CmdInput(CMD_MENU_SIMPLE | CMD_MENU_NOCASE, "Menu level");
            pInput->MenuAdd('n', "Novice");
            pInput->MenuAdd('i', "Intermediate");
            pInput->MenuAdd('e', "Expert");

            if(pRequest->Child("client", CmdVersion("2.3") >= 0 ? "edit" : "set") == false)
            {
               pRequest->Add("client", CmdVersion("2.3") >= 0 ? "edit" : "set");
            }

            cOption = CmdMenu(pInput);
            switch(cOption)
            {
               case 'n':
                  iMenuLevel = CMDLEV_BEGIN;
                  break;

               case 'i':
                  iMenuLevel = CMDLEV_INTERMED;
                  break;

               case 'e':
                  iMenuLevel = CMDLEV_EXPERT;
                  break;
            }
            pRequest->SetChild("menulevel", iMenuLevel);
            pRequest->Parent();

            CmdOptionSet(pUser, pRequest, "Case sensitive menus", "menucase", true);
            break;

         /* case 'n':
            SvrInputSetup(pConn, m_pUser, DETAILS_NAME);
            break; */

         case 'n':
            szOption = CmdLineStr("Name", UA_NAME_LEN);
            if(szOption != NULL && strcmp(szOption, "") != 0)
            {
               iValue = UserGet(m_pUserList, szOption);
               if(iValue == -1 || iValue == iEditID)
               {
                  pRequest->SetChild("name", szOption);
                  pUser->SetChild("name", szOption);
               }
               else
               {
                  CmdWrite("That name is already in use\n");
               }
            }
            delete[] szOption;
            break;

         case 'o':
            if(iEditID != -1)
            {
               iID = CmdLineUser(CmdUserTab);
               if(iID != -1)
               {
                  pUser->SetChild("ownerid", iID);
                  pRequest->SetChild("ownerid", iID);
               }
               /* else
               {
                  pUser->DeleteChild("ownerid");
                  pRequest->SetChild("ownerid", -1);
               } */
            }
            else
            {
               // SvrInputSetup(pConn, m_pUser, DETAILS_IDLEHIDE);
               // CmdOptionToggle(pUser, pRequest, "Retro mode", "retro");
               iRetro = 0;
               iValue = 0;
               pUser->Root();
               if(pUser->Child("client", CLIENT_NAME()) == true)
               {
                  pUser->GetChild("retro", &iRetro);
               }

               if(CmdYesNo("Upper case names", mask(iRetro, RETRO_NAMES)) == true)
               {
                  iValue += RETRO_NAMES;
               }
               if(CmdYesNo("Old menu keys", mask(iRetro, RETRO_MENUS)) == true)
               {
                  iValue += RETRO_MENUS;
               }
               if(CmdYesNo("No on page reply", mask(iRetro, RETRO_PAGENO)) == true)
               {
                  iValue += RETRO_PAGENO;
               }

               if(iRetro != iValue)
               {
                  pUser->SetChild("retro", iValue);
                  pUser->Parent();

                  pRequest->Root();
                  if(pRequest->Child("client", CmdVersion("2.3") >= 0 ? "edit" : "set") == false)
                  {
                     pRequest->Add("client", CmdVersion("2.3") >= 0 ? "edit" : "set");
                  }
                  pRequest->SetChild("retro", iValue);
               }
            }
            break;

         /* case 'p':
            SvrInputSetup(pConn, m_pUser, DETAILS_PASSWORD1);
            break; */

         case 'p':
            szPassword1 = CmdLineStr("Password", "RETURN to abort", UA_NAME_LEN, CMD_LINE_SILENT);
            if(szPassword1 != NULL && stricmp(szPassword1, "") != 0)
            {
               szPassword2 = CmdLineStr("Confirm password", "RETURN to abort", UA_NAME_LEN, CMD_LINE_SILENT);
               if(szPassword2 != NULL && stricmp(szPassword2, "") != 0)
               {
                  if(strcmp(szPassword1, szPassword2) == 0)
                  {
                     pRequest->AddChild("password", szPassword2);
                  }
                  else
                  {
                     CmdWrite("Passwords do not match\n");
                  }
               }
               delete[] szPassword2;
            }
            delete[] szPassword1;
            break;

         /* case 'r':
            CmdOptionSet(pUser, pRequest, "Instant private messages", "fakepage", false, false);
            break; */

         case 'r':
            pInput = new CmdInput(CMD_MENU_NOCASE, "Navigation");
            pInput->MenuAdd('a', "Alphabetical with hierarchy");
            pInput->MenuAdd('h', "alpHabetical without hierarchy");
            if(iAccessLevel >= LEVEL_WITNESS)
            {
               pInput->MenuAdd('l', "sub Level (private, editor, member, restricted, subscriber)");
            }
            else
            {
               pInput->MenuAdd('l', "sub Level (private, editor, member, subscriber)");
            }
            /* if(CmdVersion("2.5") >= 0)
            {
               pInput->MenuAdd('p', "Priorities (specifics then as sub level)");
            } */
            pInput->MenuDefault('a');

            cOption = CmdMenu(pInput);
            switch(cOption)
            {
               case 'a':
                  iValue = NAV_ALPHATREE;
                  break;

               case 'h':
                  iValue = NAV_ALPHALIST;
                  break;

               case 'l':
                  iValue = NAV_LEVEL;
                  break;

               case 'p':
                  iValue = NAV_PRIORITIES;
                  break;
            }

            if(pUser->Child("client", CLIENT_NAME()) == false)
            {
               pUser->Add("client", CLIENT_NAME());
            }
            pUser->SetChild("foldernav", iValue);
            pUser->Parent();

            if(pRequest->Child("client", "edit") == false)
            {
               pRequest->Add("client", "edit");
            }
            pRequest->SetChild("foldernav", iValue);
            pRequest->Parent();

            break;

         case 's':
            AlertsMenu(pUser, pRequest, iAccessLevel);
            break;

         case 't':
            if(iEditID == -1)
            {
               CmdOptionSet(pUser, pRequest, "Sig filter", "sigfilter");
            }
            else
            {
               pInput = new CmdInput(CMD_MENU_NOCASE, "User type");
               pInput->MenuAdd('a', "Agent");
               pInput->MenuAdd('n', "None");
               pInput->MenuAdd('x', "eXit", NULL, true);
               cOption = CmdMenu(pInput);
               if(cOption == 'a')
               {
                  szOption = CmdLineStr("Agent name", "may include wildcards", UA_NAME_LEN);
                  if(szOption != NULL)
                  {
                     iValue = USERTYPE_NONE;
                     pUser->GetChild("usertype", &iValue);
                     iValue |= USERTYPE_AGENT;
                     pUser->SetChild("usertype", iValue);
                     pUser->SetChild("agent", szOption);

                     pRequest->SetChild("usertype", iValue);
                     if(pRequest->Child("agent") == false)
                     {
                        pRequest->Add("agent");
                     }
                     pRequest->SetChild("name", szOption);
                     pRequest->Parent();

                     delete[] szOption;
                  }
               }
            }
            break;

         /* case 'u':
            CmdAnnounceToggle(pUser, pRequest, ANN_USERCHECK);
            break; */

         case 'u':
            CmdValueSet(pUser, pRequest, "Minimum catchup", "mincatchup", 5, -1, 0, 2000);
            break;

         case 'v':
            if(iEditID == -1 && iAccessLevel < LEVEL_WITNESS)
            {
               CmdOptionSet(pUser, pRequest, "Developer option", "devoption");
            }
            else // if(iEditID != -1)
            {
               if(pUser->GetChild("accesslevel", &iValue) == true && iValue < LEVEL_MESSAGES)
               {
                  pUser->SetChild("accesslevel", LEVEL_MESSAGES);
                  pRequest->SetChild("accesslevel", LEVEL_MESSAGES);
               }
               if(pRequest->Child("details", "edit") == false)
               {
                  pRequest->Add("details", "edit");

                  if(pRequest->Child("realname") == false)
                  {
                     pRequest->Add("realname");
                  }
                  iType = 0;
                  pRequest->GetChild("type", &iType);
                  iType |= DETAIL_VALID;
                  pRequest->SetChild("type", iType);
                  pRequest->Parent();

                  if(pRequest->Child("email") == false)
                  {
                     pRequest->Add("email");
                  }
                  iType = 0;
                  pRequest->GetChild("type", &iType);
                  iType |= DETAIL_VALID;
                  pRequest->SetChild("type", iType);
                  pRequest->Parent();

                  if(pRequest->Child("sms") == false)
                  {
                     pRequest->Add("sms");
                  }
                  iType = 0;
                  pRequest->GetChild("type", &iType);
                  iType |= DETAIL_VALID;
                  pRequest->SetChild("type", iType);
                  pRequest->Parent();

                  if(pRequest->Child("homepage") == false)
                  {
                     pRequest->Add("homepage");
                  }
                  iType = 0;
                  pRequest->GetChild("type", &iType);
                  iType |= DETAIL_VALID;
                  pRequest->SetChild("type", iType);
                  pRequest->Parent();
               }
            }
            break;

         case 'w':
            CmdValueSet(pUser, pRequest, "Width (60-140)", "width", 3, 60, 140);
            /* iValue = CmdLineNum("Width (60-140)");
            if(iValue >= 60 && iValue <= 140)
            {
               if(pUser->Child("client", CLIENT_NAME()) == false)
               {
                  pUser->Add("client", CLIENT_NAME());
               }
               if(pRequest->Child("client", "edit") == false)
               {
                  pRequest->Add("client", "edit");
               }

               pUser->SetChild("width", iValue);
               pRequest->SetChild("width", iValue);

               pUser->Parent();
               pRequest->Parent();
            } */
            break;

         case 'y':
            pUser->GetChild("description", &szDesc);
            if(szDesc != NULL)
            {
               CmdWrite("\n\0374");
               CmdWrite(szDesc, CMD_OUT_NOHIGHLIGHT);
               CmdWrite("\0370\n");
               delete[] szDesc;
            }
            if(CmdYesNo("Replace description", false) == true)
            {
               szDesc = CmdText();

               if(szDesc != NULL)
               {
                  pUser->Root();
                  pUser->SetChild("description", szDesc);

                  pRequest->Root();
                  pRequest->SetChild("description", szDesc);
               }

               delete[] szDesc;
            }
            break;

         case 'z':
            if(iEditID == -1)
            {
               CmdOptionSet(pUser, pRequest, "Browse URLs", "browse");
            }
            break;

         case 'x':
            bLoop = false;
            break;

         default:
            debug(DEBUGLEVEL_WARN, "UserEditMenu no action for %c\n", cOption);
            break;
      }
   }

   pRequest->Root();
   if(pRequest->Children() > 0)
   {
      if(CmdYesNo("Save changes", false) == true)
      {
         if(iEditID != -1)
         {
            pRequest->AddChild("userid", iEditID, EDF_ABSFIRST);
         }

         debugEDFPrint("UserEditMenu request", pRequest);

         CmdRequest(MSG_USER_EDIT, pRequest, false, &pReply);
         delete pReply;
         if(iEditID == -1)
         {
            CmdUserReset();
         }
      }
   }
   delete pRequest;

   return true;
}

bool FolderEditMenu(int iEditID)
{
   STACKTRACE
   int iUserID = 0, iAccessLevel = LEVEL_NONE, iSubType = 0, iFolderMode = FOLDERMODE_NORMAL, iOption = 0;
   bool bLoop = true;
   char cOption = '\0';
   char *szUserName = NULL, *szName = NULL, *szOption = NULL, *szText = NULL;
   EDF *pReply = NULL, *pFolder = NULL, *pRequest = NULL, *pKill = NULL, *pSub = NULL;
   CmdInput *pInput = NULL;

   m_pUser->Root();
   m_pUser->Get(NULL, &iUserID);
   m_pUser->GetChild("name", &szUserName);
   m_pUser->GetChild("accesslevel", &iAccessLevel);

   if(iEditID == -1)
   {
      iEditID = CmdLineFolder();
   }
   if(iEditID != -1)
   {
      pRequest = new EDF();
      pRequest->AddChild("folderid", iEditID);
      if(CmdRequest(MSG_FOLDER_LIST, pRequest, &pReply) == false)
      {
         CmdWrite("No such folder\n");

         delete[] szUserName;

         return false;
      }

      // EDFPrint("FolderEditMenu reply", pReply);

      if(pReply->Child("folder") == true)
      {
         pFolder = new EDF();
         pFolder->Copy(pReply, false, true);
         pFolder->Set("folder", iEditID);
      }
      else
      {
         debug(DEBUGLEVEL_ERR, "FolderEditMenu no folder section\n");
         return false;
      }

      delete pReply;
   }
   else
   {
      delete[] szUserName;

      return false;
   }

   // m_pUser->Root();
   // iSubType = CmdFolderSubType(iEditID);
   pFolder->GetChild("subtype", &iSubType);

   pFolder->GetChild("accessmode", &iFolderMode);
   CmdMessageTreeView(pFolder, "folder");
   if(iAccessLevel < LEVEL_WITNESS && iSubType < SUBTYPE_EDITOR)
   {
      delete[] szUserName;

      return false;
   }

   pRequest = new EDF();

   while(bLoop == true)
   {
      pFolder->Root();
      pInput = CmdFolderEdit(iEditID, iFolderMode, iAccessLevel, iSubType);

      pRequest->Root();
      cOption = CmdMenu(pInput);
      switch(cOption)
      {
         case 'a':
            // access level
            pInput = new CmdInput(CMD_MENU_NOCASE, "Access");
            pInput->MenuAdd('n', "None");
            pInput->MenuAdd('e', "Editor");
            pInput->MenuAdd('w', "Witness");
            pInput->MenuAdd('x', "eXit", NULL, true);
            cOption = CmdMenu(pInput);
            switch(cOption)
            {
               case 'n':
                  pFolder->DeleteChild("accesslevel");
                  pRequest->SetChild("accesslevel", -1);
                  break;

               case 'e':
                  pFolder->SetChild("accesslevel", LEVEL_EDITOR);
                  pRequest->SetChild("accesslevel", LEVEL_EDITOR);
                  break;

               case 'w':
                  pFolder->SetChild("accesslevel", LEVEL_WITNESS);
                  pRequest->SetChild("accesslevel", LEVEL_WITNESS);
                  break;
            }
            break;

         case 'c':
            // access mode
            pInput = new CmdInput(CMD_MENU_NOCASE, "Access");
            pInput->MenuAdd('n', "Normal");
            pInput->MenuAdd('r', "Read-only");
            pInput->MenuAdd('e', "rEstriced");
            pInput->MenuAdd('p', "Private");
            // pInput->MenuAdd('a', "Advanced");
            cOption = CmdMenu(pInput);
            switch(cOption)
            {
               case 'n':
                  pFolder->SetChild("accessmode", FOLDERMODE_NORMAL);
                  pRequest->SetChild("accessmode", FOLDERMODE_NORMAL);
                  break;

               case 'r':
                  pFolder->SetChild("accessmode", ACCMODE_READONLY);
                  pRequest->SetChild("accessmode", ACCMODE_READONLY);
                  break;

               case 'e':
                  pFolder->SetChild("accessmode", FOLDERMODE_RESTRICTED);
                  pRequest->SetChild("accessmode", FOLDERMODE_RESTRICTED);
                  break;

               case 'p':
                  pFolder->SetChild("accessmode", FOLDERMODE_NORMAL + ACCMODE_PRIVATE);
                  pRequest->SetChild("accessmode", FOLDERMODE_NORMAL + ACCMODE_PRIVATE);
                  break;

               case 'a':
                  break;
            }
            break;

         case 'd':
            // default replies
            iOption = CmdLineFolder(-1, &szName, false);
            if(iOption != -1)
            {
               pFolder->SetChild("replyid", iOption);
               pFolder->SetChild("replyname", szName);
               pRequest->SetChild("replyid", iOption);
            }
            else if(szName != NULL && strcmp(szName, ".") == 0)
            {
               pFolder->DeleteChild("replyid");
               pRequest->SetChild("replyid", -1);
            }
            delete[] szName;
            break;

         case 'e':
            // add editor
            iOption = CmdLineUser(CmdUserTab);//, -1, NULL, true, NULL, NULL);
            if(iOption != -1)
            {
               pSub = new EDF();
               pSub->AddChild("folderid", iEditID);
               pSub->AddChild("userid", iOption);
               pSub->AddChild("subtype", SUBTYPE_EDITOR);
               CmdRequest(MSG_FOLDER_SUBSCRIBE, pSub);
            }
            break;

         case 'i':
            // replies
            iOption = CmdLineNum("Number of replies", ". for infinite");
            if(iOption > 0)
            {
               pRequest->SetChild("replies", iOption);
               pFolder->SetChild("replies", iOption);
            }
            else
            {
               pRequest->DeleteChild("replies");
               pFolder->DeleteChild("replies");
            }
            break;

         case 'k':
            // if(CmdYesNo("Really kill folder", false) == true)
            szOption = CmdLineStr("Really kill folder", "YES to proceed");
            if(strcmp(szOption, "YES") == 0)
            {
               pRequest->Root();
               while(pRequest->DeleteChild() == true);

               pKill = new EDF();
               pKill->AddChild("folderid", iEditID);
               CmdRequest(MSG_FOLDER_DELETE, pKill);

               bLoop = false;
            }
            delete[] szOption;
            break;

         case 'l':
            CmdMessageTreeView(pFolder, "folder");
            break;

         case 'm':
            // add member
            iOption = CmdLineUser(CmdUserTab);//, -1, NULL, true, NULL, NULL);
            if(iOption != -1)
            {
               pSub = new EDF();
               pSub->AddChild("folderid", iEditID);
               pSub->AddChild("userid", iOption);
               pSub->AddChild("subtype", SUBTYPE_MEMBER);
               CmdRequest(MSG_FOLDER_SUBSCRIBE, pSub);
            }
            break;

         case 'n':
            // name
            szOption = CmdLineStr("Name", UA_NAME_LEN);
            if(szOption != NULL && strcmp(szOption, "") != 0)
            {
               pFolder->SetChild("name", szOption);
               pRequest->SetChild("name", szOption);
            }
            delete[] szOption;
            break;

         case 'p':
            // parent
            iOption = CmdLineFolder(-1, &szName, true);
            if(iOption != -1)
            {
               pFolder->SetChild("parentid", iOption);
               pFolder->SetChild("parentname", szName);
               pRequest->SetChild("parentid", iOption);
            }
            else if(szName != NULL && strcmp(szName, ".") == 0)
            {
               pFolder->DeleteChild("parentid");
               pRequest->SetChild("parentid", -1);
            }
            delete[] szName;
            break;

         case 'r':
            // remove user
            iOption = CmdLineUser(CmdUserTab);//, -1, NULL, true, NULL, NULL);
            if(iOption!= -1)
            {
               pSub = new EDF();
               pSub->AddChild("folderid", iEditID);
               pSub->AddChild("userid", iOption);
               CmdRequest(MSG_FOLDER_UNSUBSCRIBE, pSub);
            }
            break;

         case 's':
            // add subscriber
            iOption = CmdLineUser(CmdUserTab);//, -1, NULL, true, NULL, NULL);
            if(iOption != -1)
            {
               pSub = new EDF();
               pSub->AddChild("folderid", iEditID);
               pSub->AddChild("userid", iOption);
               pSub->AddChild("subtype", SUBTYPE_SUB);
               CmdRequest(MSG_FOLDER_SUBSCRIBE, pSub);
            }
            break;

         case 'w':
            // write info file
            szText = CmdText();
            if(szText != NULL)
            {
               if(pFolder->Child("info") == false)
               {
                  pFolder->Add("info");
               }
               pFolder->SetChild("fromid", iOption);
               pFolder->SetChild("fromname", szUserName);
               pFolder->SetChild("text", szText);
               pFolder->Parent();

               if(pRequest->Child("info") == false)
               {
                  pRequest->Add("info");
               }
               pRequest->SetChild("text", szText);
               pRequest->Parent();

               delete[] szText;
            }
            break;

         case 'x':
            bLoop = false;
            break;

         case 'y':
            // expiry
            iOption = CmdLineNum("Expiry time in days (-1 for no expiry, 0 for system default)");
            if(iOption == -1)
            {
               pFolder->SetChild("expire", -1);
               pRequest->SetChild("expire", -1);
            }
            else
            {
               if(CmdVersion("2.5") >= 0)
               {
                  iOption *= 86400;
               }
               pFolder->SetChild("expire", iOption);
               pRequest->SetChild("expire", iOption);
            }
            break;

         default:
            debug(DEBUGLEVEL_WARN, "FolderEditMenu no action for %c\n", cOption);
            break;
      }
   }

   pRequest->Root();
   if(pRequest->Children() > 0)
   {
      if(CmdYesNo("Save changes", false) == true)
      {
         if(iEditID != -1)
         {
            pRequest->AddChild("folderid", iEditID, EDF_ABSFIRST);
         }

         CmdEDFPrint("FolderEditMenu request", pRequest, false, false);

         CmdRequest(MSG_FOLDER_EDIT, pRequest, &pReply);
         /* if(iEditID == -1)
         {
            CmdUserReset();
         } */
         delete pReply;
      }
      else
      {
         delete pRequest;
      }
   }
   else
   {
      delete pRequest;
   }

   delete[] szUserName;

   return true;
}

bool UserListMenu()
{
   STACKTRACE
   int iAccessLevel = LEVEL_NONE, iUserID = 0, iLowest = LEVEL_NONE, iHighest = LEVEL_SYSOP, iListType = 0, iUserType = -1, iOwnerID = 0, iSearchType = 0;
   bool bLoop = false, bFull = false;
   char cOption = '\0';
   char szWrite[100];
   char *szUser = NULL;
   EDF *pRequest = NULL, *pReply = NULL, *pSystem = NULL;
   CmdInput *pInput = NULL;

   m_pUser->Root();
   m_pUser->GetChild("accesslevel", &iAccessLevel);

   CmdRequest(MSG_SYSTEM_LIST, &pSystem);
   CmdUserStats(pSystem);
   delete pSystem;

   // CmdLineUser(NULL, -1, &szUser, false);
   szUser = CmdLineStr("Name of user");
   if(szUser == NULL || strlen(szUser) == 0)
   {
      delete[] szUser;

      return false;
   }
   else if(isdigit(szUser[0]))
   {
      iUserID = atoi(szUser);
   }

   pRequest = new EDF();

   if(iUserID != 0)
   {
      pRequest->AddChild("userid", iUserID);
      bFull = true;
   }
   else if(strchr(szUser, '*') == NULL)
   {
      iUserID = UserGet(m_pUserList, szUser);
      if(iUserID == -1)
      {
         sprintf(szWrite, "\0374%s\0370 does not exist\n", szUser);
         CmdWrite(szWrite);

         delete pRequest;

         return false;
      }
      pRequest->AddChild("userid", iUserID);
      bFull = true;
   }
   else
   {
      if(strcmp(szUser, "*") == 0)
      {
         delete[] szUser;
         szUser = NULL;
      }

      if(CmdYesNo("Show full details", false) == true)
      {
         iSearchType = 3;
         bFull = true;
      }

      pInput = new CmdInput(CMD_MENU_NOCASE, "Lowest level");
      pInput->MenuAdd('f', "Full");
      pInput->MenuAdd('n', "None");
      pInput->MenuAdd('g', "Guest");
      pInput->MenuAdd('m', "Messages");
      pInput->MenuAdd('e', "Editor");
      pInput->MenuAdd('a', "Admin");
      pInput->MenuAdd('o', "Other");
      /* pInput->MenuAdd('t', "agenTs");
      if(iAccessLevel >= LEVEL_WITNESS)
      {
         pInput->MenuAdd('o', "Owned");
      } */
      pInput->MenuDefault('f');
      cOption = CmdMenu(pInput);
      /* if(cOption == 't')
      {
         // iListType = 1;
         iUserType = USERTYPE_AGENT;
      } */
      if(cOption == 'o')
      {
         if(iSearchType == 0)
         {
            iSearchType = 3;
         }

         pInput = new CmdInput(CMD_MENU_NOCASE, "Other");
         pInput->MenuAdd('a', "Agents");
         if(iAccessLevel >= LEVEL_WITNESS)
         {
            pInput->MenuAdd('d', "Deleted");
            // if(CmdVersion("2.5") >= 0)
            {
               pInput->MenuAdd('o', "Owned users");
               pInput->MenuAdd('w', "oWner users");
            }
         }
         cOption = CmdMenu(pInput);

         if(cOption == 'o' || cOption == 'w')
         {
            iOwnerID = CmdLineUser(CmdUserTab, -1, NULL, true, "for everyone");
            debug(DEBUGLEVEL_DEBUG, "UserListMenu owner ID %d\n", iOwnerID);
         }

         if(cOption == 'a')
         {
            iUserType = USERTYPE_AGENT;
            iListType = 1;
         }
         else if(cOption == 'd')
         {
            iUserType = USERTYPE_DELETED;
            iListType = 2;
         }
         else if(cOption == 'o')
         {
            iListType = 3;
         }
         else if(cOption == 'w')
         {
            iListType = 4;
         }
      }
      else if(cOption != 'f')
      {
         switch(cOption)
         {
            case 'n':
               iLowest = LEVEL_NONE;
               break;

            case 'g':
               iLowest = LEVEL_GUEST;
               break;

            case 'm':
               iLowest = LEVEL_MESSAGES;
               break;

            case 'e':
               iLowest = LEVEL_EDITOR;
               break;

            case 'a':
               iLowest = LEVEL_WITNESS;
               break;
         }

         if(iLowest < LEVEL_WITNESS)
         {
            pInput = new CmdInput(CMD_MENU_NOCASE, "Highest level");
            if(iLowest <= LEVEL_WITNESS)
            {
               pInput->MenuAdd('a', "Admin");
            }
            if(iLowest <= LEVEL_EDITOR)
            {
               pInput->MenuAdd('e', "Editor");
            }
            if(iLowest <= LEVEL_MESSAGES)
            {
               pInput->MenuAdd('m', "Messages");
            }
            if(iLowest <= LEVEL_GUEST)
            {
               pInput->MenuAdd('g', "Guest");
            }
            if(iLowest <= LEVEL_NONE)
            {
               pInput->MenuAdd('n', "None");
            }
            cOption = CmdMenu(pInput);
            switch(cOption)
            {
               case 'n':
                  iHighest = LEVEL_NONE;
                  break;

               case 'g':
                  iHighest = LEVEL_GUEST;
                  break;

               case 'm':
                  iHighest = LEVEL_MESSAGES;
                  break;

               case 'e':
                  iHighest = LEVEL_EDITOR;
                  break;
            }
         }
      }

      if(szUser != NULL)
      {
         pRequest->AddChild("name", szUser);
      }
      if(iSearchType > 0)
      {
         pRequest->SetChild("searchtype", iSearchType);
      }
      if(iLowest > LEVEL_NONE)
      {
         pRequest->AddChild("lowest", iLowest);
      }
      if(iHighest < LEVEL_SYSOP)
      {
         pRequest->AddChild("highest", iHighest);
      }
      if(iUserType != -1)
      {
         pRequest->AddChild("usertype", iUserType);
      }
   }
   delete[] szUser;

   CmdRequest(MSG_USER_LIST, pRequest, &pReply);
   if(bFull == true)
   {
      CmdPageOn();

      bLoop = pReply->Child("user");
      while(bLoop == true)
      {
         if(CmdUserListMatch(pReply, iListType, iOwnerID) == true)
         {
            CmdUserView(pReply);
         }

         bLoop = pReply->Next("user");
      }

      CmdPageOff();
   }
   else
   {
      CmdUserList(pReply, iListType, iOwnerID);
   }

   delete pReply;

   return true;
}

void FolderListMenu()
{
   STACKTRACE
   char cOption = '\0';
   int iListType = 0, iListDetails = 0, iSearchType = 0, iAccessLevel = LEVEL_NONE;
   EDF *pRequest = NULL, *pReply = NULL;
   CmdInput *pInput = NULL;

   pInput = new CmdInput(CMD_MENU_NOCASE, "Folders");
   pInput->MenuAdd('u', "Unread");
   pInput->MenuAdd('s', "Subscribed", NULL, true);
   pInput->MenuAdd('a', "All");
   pInput->MenuAdd('n', "uNsubscribed");
   pInput->MenuAdd('x', "eXit");

   m_pUser->Root();
   m_pUser->GetChild("accesslevel", &iAccessLevel);

   cOption = CmdMenu(pInput);
   if(cOption != 'x')
   {
      pRequest = new EDF();

      switch(cOption)
      {
         case 'u':
            iListType = 3;
            iSearchType = 0;
            break;

         case 's':
            iListType = 2;
            iSearchType = 0;
            break;

         case 'a':
            iListType = 0;
            iSearchType = 2;
            break;

         case 'n':
            iListType = 1;
            iSearchType = 2;
            break;
      }

      pInput = new CmdInput(CMD_MENU_NOCASE, "Details");
      pInput->MenuAdd('n', "None", NULL, true);
      pInput->MenuAdd('y', "summarY");
      pInput->MenuAdd('a', "Access");
      // pInput->MenuAdd('s', "sort by Size");
      pInput->MenuAdd('e', "Editor");
      if(iAccessLevel >= LEVEL_WITNESS)
      {
         pInput->MenuAdd('c', "Creation");
         pInput->MenuAdd('p', "exPiry");
      }
      cOption = CmdMenu(pInput);
      if(cOption != 'n')
      {
         iSearchType++;
      }
      switch(cOption)
      {
         case 'n':
            iListDetails = 0;
            break;

         case 'y':
            iListDetails = 1;
            break;

         case 'a':
            iListDetails = 6;
            break;

         case 'e':
            iListDetails = 2;
            break;

         case 'c':
            iListDetails = 3;
            break;

         case 'p':
            iListDetails = 4;
            break;

         case 's':
            iListDetails = 5;
            break;
      }
      pRequest->AddChild("searchtype", iSearchType);

      CmdRequest(MSG_FOLDER_LIST, pRequest, &pReply);

      m_pUser->Root();
      CmdMessageTreeList(pReply, "folder", iListType, iListDetails);
      delete pReply;
   }
}

void LocationEditMenu(EDF *pLocation)
{
   int iLocationID = -1;
   char cOption = '\0';
   char *szMatch = NULL;
   EDF *pRequest = NULL;
   CmdInput *pInput = NULL;

   pLocation->Get(NULL, &iLocationID);

   CmdEDFPrint("LocationEditMenu", pLocation);

   pRequest = new EDF();

   while(cOption != 'k' && cOption != 'x')
   {
      pInput = new CmdInput(CMD_MENU_NOCASE, "Location");
      if(CmdVersion("2.5") >= 0)
      {
         pInput->MenuAdd('a', "Add match");
         pInput->MenuAdd('d', "Delete match");
      }
      pInput->MenuAdd('k', "Kill location");
      pInput->MenuAdd('l', "List location");
      pInput->MenuAdd('x', "eXit");

      cOption = CmdMenu(pInput);
      switch(cOption)
      {
         case 'a':
         case 'd':
            szMatch = CmdLineStr("Match");
            if(szMatch != NULL && strlen(szMatch) > 0)
            {
               if(cOption == 'a' || pLocation->Child(isdigit(szMatch[0]) ? "address" : "hostname", szMatch) == true)
               {
                  if(cOption == 'a')
                  {
                     if(pRequest->Child("add") == false)
                     {
                        pRequest->Add("add");
                     }
                     pLocation->AddChild(isdigit(szMatch[0]) ? "address" : "hostname", szMatch);
                  }
                  else
                  {
                     if(pRequest->Child("delete") == false)
                     {
                        pRequest->Add("delete");
                     }
                     pLocation->Delete();
                  }
                  pRequest->AddChild(isdigit(szMatch[0]) ? "address" : "hostname", szMatch);
                  pRequest->Parent();
               }
               else
               {
                  CmdWrite("Cannot find match\n");
               }
            }
            delete[] szMatch;
            break;

         case 'k':
            if(CmdYesNo("Really kill location", false) == true)
            {
               while(pRequest->DeleteChild() == true);
               pRequest->AddChild("locationid", iLocationID);
               CmdEDFPrint("LocationsMenu request", pRequest);
               CmdRequest(MSG_LOCATION_DELETE, pRequest);
            }
            break;

         case 'l':
            CmdEDFPrint("LocationEditMenu", pLocation);
            break;

         case 'x':
            if(pRequest->Children() > 0)
            {
               if(CmdYesNo("Save changes", false) == true)
               {
                  pRequest->AddChild("locationid", iLocationID);
                  CmdEDFPrint("LocationsEditMenu request", pRequest);
                  CmdRequest(MSG_LOCATION_EDIT, pRequest);
               }
            }
            break;
      }
   }
}

void LocationsMenu()
{
   STACKTRACE
   int iLocationID = 0, iParentID = -1;
   bool bLoop = false, bRequest = true;
   char cOption = '\0';
   char szWrite[200];
   char *szName = NULL, *szParent = NULL, *szOption = NULL, *szLocation = NULL;
   EDF *pRequest = NULL, *pReply = NULL, *pLocation = NULL;
   CmdInput *pInput = NULL;

   CmdRequest(MSG_LOCATION_LIST, &pReply);
   pReply->Sort("location", "name");
   CmdLocationList(pReply, 0);
   delete pReply;

   while(cOption != 'x')
   {
      pInput = new CmdInput(CMD_MENU_NOCASE, "Locations");
      pInput->MenuAdd('a', "Add location");
      if(CmdVersion("2.5") >= 0)
      {
         pInput->MenuAdd('e', "Edit location");
         pInput->MenuAdd('s', "Sort locations");
      }
      else
      {
         pInput->MenuAdd('d', "Delete location");
      }
      pInput->MenuAdd('l', "List locations (sorted)");
      if(CmdVersion("2.5") >= 0)
      {
         pInput->MenuAdd('o', "lOokup location");
      }
      pInput->MenuAdd('u', "list locations (Unsorted)");
      pInput->MenuAdd('x', "eXit");

      cOption = CmdMenu(pInput);
      switch(cOption)
      {
         case 'a':
            szName = CmdLineStr("Name", LINE_LEN);
            if(szName != NULL && strlen(szName) > 0)
            {
               pRequest = new EDF();
               pRequest->AddChild("name", szName);

               szParent = CmdLineStr("Parent ID", "RETURN for top level");
               if(szParent != NULL && strlen(szParent) > 0)
               {
                  iParentID = atoi(szParent);
                  if(iParentID != -1)
                  {
                     pRequest->AddChild("parentid", iParentID);
                  }
               }

               // pRequest->Add("add");
               bLoop = true;
               while(bLoop == true)
               {
                  szOption = CmdLineStr("Match with", "RETURN to end", LINE_LEN);
                  if(szOption != NULL && strlen(szOption) > 0)
                  {
                     if(isdigit(szOption[0]))
                     {
                        pRequest->AddChild("address", szOption);
                     }
                     else
                     {
                        pRequest->AddChild("hostname", szOption);
                     }
                  }
                  else
                  {
                     bLoop = false;
                     if(szOption == NULL)
                     {
                        bRequest = false;
                     }
                  }
                  delete[] szOption;
               }
               // pRequest->Parent();

               if(bRequest == true)
               {
                  CmdEDFPrint("LocationsMenu request", pRequest);
                  CmdRequest(MSG_LOCATION_ADD, pRequest);
               }

               delete[] szParent;
            }
            delete[] szName;
            break;

         case 'd':
            iLocationID = CmdLineNum("Location");
            pRequest = new EDF();
            pRequest->AddChild("locationid", iLocationID);
            CmdEDFPrint("LocationsMenu request", pRequest);
            CmdRequest(MSG_LOCATION_DELETE, pRequest);
            break;

         case 'e':
            iLocationID = CmdLineNum("Location");
            pRequest = new EDF();
            pRequest->AddChild("locationid", iLocationID);
            if(CmdRequest(MSG_LOCATION_LIST, pRequest, &pReply) == true)
            {
               if(pReply->Child("location") == true)
               {
                  pLocation = new EDF();
                  pLocation->Copy(pReply, true, true);
                  LocationEditMenu(pLocation);
                  delete pLocation;
               }
               else
               {
                  debug(DEBUGLEVEL_ERR, "LocationsMenu no location section\n");
               }
            }
            else
            {
               CmdWrite("No such location\n");
            }
            delete pReply;
            break;

         case 'l':
         case 'u':
            CmdRequest(MSG_LOCATION_LIST, &pReply);
            if(cOption == 'l')
            {
               pReply->Sort("location", "name");
            }
            CmdLocationList(pReply, 0);
            delete pReply;
            break;

         case 'o':
            pRequest = new EDF();
            szOption = CmdLineStr("Lookup");
            if(szOption != NULL && strlen(szOption) > 0)
            {
               if(isdigit(szOption[0]))
               {
                  pRequest->AddChild("address", szOption);
               }
               else
               {
                  pRequest->AddChild("hostname", szOption);
               }

               if(CmdRequest(MSG_LOCATION_LOOKUP, pRequest, &pReply) == true)
               {
                  // CmdEDFPrint("LocationsMenu lookup", pReply);
                  if(pReply->GetChild("location", &szLocation) == true)
                  {
                     sprintf(szWrite, "\0374%s\0370 matches \0374%s\0370\n", szOption, szLocation);
                     delete[] szLocation;
                  }
                  else
                  {
                     sprintf(szWrite, "\0374%s\0370 does not match any location\n", szOption);
                  }
                  CmdWrite(szWrite);
               }
               delete pReply;
            }
            delete[] szOption;
            break;

         case 's':
            CmdWrite("The sorting process will combine like named locations and move more exact matches inside less exact ones, possibly resulting in undesired effects\n");
            if(CmdYesNo("Are you sure", false) == true)
            {
               CmdRequest(MSG_LOCATION_SORT);
            }
            break;
      }
   }
}

void ConnectionMenu()
{
   char cOption = '\0';
   char *szOption = NULL;
   CmdInput *pInput = NULL;
   EDF *pRequest = NULL;

   while(cOption != 'x')
   {
      pInput = new CmdInput(CMD_MENU_NOCASE, "Connection");
      pInput->MenuAdd('a', "Add allow");
      pInput->MenuAdd('r', "Remove allow");
      pInput->MenuAdd('d', "aDd deny");
      pInput->MenuAdd('e', "rEmove deny");
      pInput->MenuAdd('m', "reMove all denies");
      pInput->MenuAdd('n', "deNy from all");
      pInput->MenuAdd('t', "add Trust");
      pInput->MenuAdd('o', "remOve trust");
      pInput->MenuAdd('x', "eXit");

      cOption = CmdMenu(pInput);

      if(cOption != 'x')
      {
         pRequest = new EDF();
         pRequest->Add(CmdVersion("2.5") >= 0 ? "connection" : "login");

         if(cOption == 'm')
         {
            pRequest->AddChild("deny", "delete");
         }
         else if(cOption == 'n')
         {
            pRequest->AddChild("deny", "add");
         }
         else
         {
            szOption = CmdLineStr("Match");
            if(szOption != NULL && strlen(szOption) > 0)
            {
               if(cOption == 'a' || cOption == 'r')
               {
                  pRequest->Add("allow");
               }
               else if(cOption == 'd' || cOption == 'e')
               {
                  pRequest->Add("deny");
               }
               else
               {
                  pRequest->Add("trust");
               }
               if(cOption == 'a' || cOption == 'd' || cOption == 't')
               {
                  pRequest->Set(NULL, "add");
               }
               else
               {
                  pRequest->Set(NULL, "delete");
               }
               pRequest->AddChild(isdigit(szOption[0]) ? "address" : "hostname", szOption);
               pRequest->Parent();

               delete[] szOption;
            }
            else
            {
               delete pRequest;
               pRequest = NULL;
            }
         }

         if(pRequest != NULL)
         {
            pRequest->Parent();

            CmdEDFPrint("ConnectionMenu request", pRequest);
            CmdRequest(MSG_SYSTEM_EDIT, pRequest);
         }
      }
   }
}

EDF *DirectRequestMenu(char **szRequest)
{
   STACKTRACE
   int iParse = 0;
   char *szText = NULL, *szParse = NULL;
   EDF *pReturn = NULL;

   CmdWrite("EDF Direct Request: Create a request for 'request name'\nAdditional fields should be entered in EDF format:\n\n<userid=1/>\n<accesslevel=1/>\n\nfor example\n\n*** PLEASE do not use unless you know EXACTLY what you are doing ***\n");
   (*szRequest) = CmdLineStr("Name of request", "RETURN to abort", LINE_LEN);
   if((*szRequest) != NULL && stricmp((*szRequest), "") != 0)
   {
      szText = CmdText(CMD_LINE_EDITOR);
      if(szText != NULL)
      {
         pReturn = new EDF();
         if(strlen(szText) > 5)
         {
            debug(DEBUGLEVEL_DEBUG, "DirectRequestMenu close tag check %s\n", (char *)(szText + strlen(szText) - 3));
            if(strncmp(szText, "<>", 2) != 0) // && strncmp(szText + strlen(szText) - 3, "</>", 3) != 0)
            {
               szParse = new char[strlen(szText) + 10];
               strcpy(szParse, "<>");
               strcpy(szParse + 2, szText);
               strcpy(szParse + 2 + strlen(szText), "</>");
               debug(DEBUGLEVEL_DEBUG, "DirectRequestMenu parsing with additions:\n%s\n", szParse);
               iParse = pReturn->Read(szParse);
               delete[] szParse;
            }
            else
            {
               iParse = pReturn->Read(szText);
            }
            if(iParse <= 0)
            {
               CmdWrite("Unable to parse additional fields\n");
               delete pReturn;
               pReturn = NULL;
            }
         }
         delete[] szText;
      }
   }

   return pReturn;
}

int TaskDayMenu()
{
   STACKTRACE
   int iReturn = 0;
   char cOption = '\0';
   CmdInput *pInput = NULL;

   pInput = new CmdInput(CMD_MENU_SIMPLE, "Day");
   pInput->MenuAdd('m', "Monday");
   pInput->MenuAdd('t', "Tuesday");
   pInput->MenuAdd('w', "Wednesday");
   pInput->MenuAdd('h', "tHursday");
   pInput->MenuAdd('f', "Friday");
   pInput->MenuAdd('s', "Saturday");
   pInput->MenuAdd('u', "sUnday");

   cOption = CmdMenu(pInput);

   switch(cOption)
   {
      case 'm':
         iReturn = 0;
         break;

      case 't':
         iReturn = 1;
         break;

      case 'w':
         iReturn = 2;
         break;

      case 'h':
         iReturn = 3;
         break;

      case 'f':
         iReturn = 4;
         break;

      case 's':
         iReturn = 5;
         break;

      case 'u':
         iReturn = 6;
         break;
   }

   return iReturn;
}

void TaskMenu()
{
   STACKTRACE
   int iTaskID = 0;
   int iDay = 0, iRepeat = 0, iTime = 0, iHour = 0, iMinute = 0;
   char cOption = '\0';
   char *szRequest = NULL, *szBanner = NULL;
   EDF *pRequest = NULL, *pReply = NULL, *pEDF = NULL;
   CmdInput *pInput = NULL;

   CmdRequest(MSG_TASK_LIST, &pReply);
   CmdTaskList(pReply);
   delete pReply;

   while(cOption != 'x')
   {
      pInput = new CmdInput(CMD_MENU_NOCASE, "Task");
      pInput->MenuAdd('a', "Add task");
      pInput->MenuAdd('b', "Banner change");
      pInput->MenuAdd('d', "Delete task");
      pInput->MenuAdd('x', "eXit");

      cOption = CmdMenu(pInput);

      switch(cOption)
      {
         case 'a':
            pInput = new CmdInput(CMD_MENU_SIMPLE, "Repeat");
            pInput->MenuAdd('d', "Daily");
            pInput->MenuAdd('w', "Weekday");
            pInput->MenuAdd('e', "weEkend");
            pInput->MenuAdd('k', "weeKly");
            pInput->MenuAdd('o', "Once off", NULL, true);

            cOption = CmdMenu(pInput);
            switch(cOption)
            {
               case 'd':
                  iRepeat = TASK_DAILY;
                  break;

               case 'w':
                  iRepeat = TASK_WEEKDAY;
                  break;

               case 'e':
                  iRepeat = TASK_WEEKEND;
                  break;

               case 'k':
                  iRepeat = TASK_WEEKLY;
                  break;

               case 'o':
                  iRepeat = 0;
                  break;
            }

            pRequest = new EDF();

            if(iRepeat > 0)
            {
               pRequest->AddChild("repeat", iRepeat);

               iTime = 0;

               if(iRepeat == TASK_WEEKLY)
               {
                  iDay = TaskDayMenu();
                  pRequest->AddChild("day", iDay);
               }

               iHour = CmdLineNum("Hour");
               pRequest->AddChild("hour", iHour);
               iMinute = CmdLineNum("Minute");
               pRequest->AddChild("minute", iMinute);
            }
            else
            {
               while(iTime <= time(NULL))
               {
                  iTime = CmdLineNum("Run time (seconds)");
                  if(iTime <= time(NULL))
                  {
                     CmdWrite("Cannot be in the past\n");
                  }
               }
               pRequest->AddChild("time", iTime);
            }

            pEDF = DirectRequestMenu(&szRequest);
            if(pEDF != NULL)
            {
               pRequest->Add("request", szRequest);
               pRequest->Copy(pEDF, false);

               CmdEDFPrint("TaskMenu adding task", pRequest);

               if(CmdRequest(MSG_TASK_ADD, pRequest, &pReply) == false)
               {
                  CmdEDFPrint("Task add failed:", pReply);
               }

               delete pEDF;
            }
            delete[] szRequest;
            break;

         case 'b':
            iTime = CmdLineNum("Run time (seconds)");
            szBanner = CmdText();
            if(szBanner != NULL)
            {
               pRequest = new EDF();
               pRequest->AddChild("time", iTime);
               pRequest->Add("request", MSG_SYSTEM_EDIT);
               pRequest->AddChild("banner", szBanner);
               pRequest->Parent();

               CmdEDFPrint("TaskMenu banner change", pRequest);
               if(CmdRequest(MSG_TASK_ADD, pRequest, &pReply) == false)
               {
                  CmdEDFPrint("Task banner change:", pReply);
               }
            }
            break;

         case 'd':
            iTaskID = CmdLineNum("ID of task");
            if(iTaskID > 0)
            {
               pRequest = new EDF();
               pRequest->AddChild("taskid", iTaskID);

               if(CmdRequest(MSG_TASK_DELETE, pRequest, &pReply) == true)
               {
                  CmdWrite("Task deleted\n");
               }
               else
               {
                  CmdWrite("Task not found\n");
               }

               delete pReply;
            }
            break;
      }
   }
}

void AdminMenu()
{
   STACKTRACE
   int iAccessLevel = LEVEL_NONE, iID = 0, iValue = 0, iCurrID = 0, iExpire = -1, iParentID = -1, iAccessMode = 0;
   // long lWriteLen = 0;
   bool bLoop = true;
   char cOption = '\0';
   char szWrite[100];
   char *szOption = NULL, *szText = NULL, *szReply = NULL, *szBanner = NULL, *szInfo = NULL;
   // byte *pWrite = NULL;
   EDF *pRequest = NULL, *pReply = NULL, *pEDF = NULL, *pSystem = NULL;
   CmdInput *pInput = NULL;

   CmdRequest(MSG_SYSTEM_LIST, &pSystem);
   CmdSystemView(pSystem);

   // sprintf(szWrite, "Client:       \0373%s\0370 build \0373%d\0370, \0373%s %s\0370\n\n", CLIENT_NAME(), BUILDNUM, BUILDTIME, BUILDDATE);
   sprintf(szWrite, "Client:       \0373%s\0370 build \0373%d\0370, \0373%s %s\0370 (PID \0373%lu\0370)\n", CLIENT_NAME(), BuildNum(), BuildTime(), BuildDate(), CmdPID());
   CmdWrite(szWrite);
   sprintf(szWrite, "Server:       \0373%s\0370 (port \0373%d\0370", m_pClient->Hostname(), m_pClient->Port());
   if(m_pClient->GetSecure() == true)
   {
      strcat(szWrite, ", secure");
   }
   strcat(szWrite, ")\n\n");
   CmdWrite(szWrite);

   m_pUser->Root();
   m_pUser->Get(NULL, &iCurrID);
   m_pUser->GetChild("accesslevel", &iAccessLevel);

   while(bLoop == true)
   {
      cOption = CmdMenu(ADMIN);
      switch(cOption)
      {
         case 'a':
            // Add user
            CreateUserMenu(true, NULL, NULL);
            break;

         case 'b':
            CmdMessageAdd(-1, 0, 0);
            break;

         case 'c':
            // Create folder
            CmdLineFolder(-1, &szOption, false);
            if(szOption != NULL && strcmp(szOption, "") != 0)
            {
               // Find parent
               iParentID = CmdLineFolder("Parent folder");

               iExpire = CmdLineNum("Expiry time in days", CmdVersion("2.5") >= 0 ? "-1 for no expiry, 0 for system default" : NULL);
               if(iExpire > 0)
               {
                  if(CmdVersion("2.5") >= 0)
                  {
                     iExpire *= 86400;
                  }
               }

               if(CmdVersion("2.5") >= 0)
               {
                  pInput = new CmdInput(CMD_MENU_NOCASE, "Access");
                  pInput->MenuAdd('n', "Normal");
                  pInput->MenuAdd('r', "Read-only");
                  pInput->MenuAdd('e', "rEstriced");
                  pInput->MenuAdd('p', "Private");
                  cOption = CmdMenu(pInput);
                  switch(cOption)
                  {
                     case 'n':
                        iAccessMode = FOLDERMODE_NORMAL;
                        break;

                     case 'r':
                        iAccessMode = ACCMODE_READONLY;
                        break;

                     case 'e':
                        iAccessMode = FOLDERMODE_RESTRICTED;
                        break;

                     case 'p':
                        iAccessMode = FOLDERMODE_NORMAL + ACCMODE_PRIVATE;
                        break;

                     case 'a':
                        break;
                  }

                  szInfo = CmdText();
               }

               if(CmdVersion("2.5") < 0 || szInfo != NULL)
               {
                  pRequest = new EDF();
                  pRequest->AddChild("name", szOption);
                  if(iParentID != -1)
                  {
                     pRequest->AddChild("parentid", iParentID);
                  }
                  pRequest->AddChild("expire", iExpire);
                  pRequest->AddChild("accessmode", iAccessMode);
                  if(szInfo != NULL)
                  {
                     pRequest->Add("info");
                     pRequest->AddChild("text", szInfo);
                     pRequest->Parent();
                  }
                  CmdRequest(MSG_FOLDER_ADD, pRequest);

                  delete[] szInfo;
               }
            }
            delete[] szOption;
            break;

         /* case 'o':
            iID = CmdLineFolder(CmdFolderTab);
            if(iID != -1)
            {
               pRequest = new EDF();
               pRequest->AddChild("folderid", iID);
               CmdRequest(MSG_FOLDER_DELETE, pRequest);
            }
            break; */

         case 'e':
            iID = CmdLineUser(CmdUserTab);
            if(iID != -1)
            {
               UserEditMenu(iID);
            }
            break;

         case 'h':
            LocationsMenu();
            break;

         case 'i':
            iValue = CmdLineNum("Number of minutes before idling (RETURN for none)", 0, -1);
            if(iValue != -1)
            {
               pRequest = new EDF();
               pRequest->AddChild("idletime", iValue * 60);
               CmdRequest(MSG_SYSTEM_EDIT, pRequest);
            }
            break;

         case 'k':
            TaskMenu();
            break;

         case 'l':
            iID = CmdLineUser(CmdUserLoginTab);
            if(iID != -1)
            {
               szOption = CmdLineStr("Message", "RETURN for none");
               if(szOption != NULL)
               {
                  pRequest = new EDF();
                  pRequest->AddChild("userid", iID);
                  if(strlen(szOption) > 0)
                  {
                     pRequest->AddChild("text", szOption);
                  }
                  if(CmdRequest(MSG_USER_LOGOUT, pRequest, &pReply) == false)
                  {
                     CmdEDFPrint("User logout failed", pReply);
                  }
                  delete pReply;

                  delete[] szOption;
               }
            }
            break;

         case 'm':
            FolderEditMenu(-1);
            break;

         case 'n':
            if(pSystem->GetChild("banner", &szBanner) == true)
            {
               CmdWrite(szBanner);
               CmdWrite("\n");

               delete[] szBanner;
            }
            if(CmdYesNo("Replace banner", false) == true)
            {
               szText = CmdText(CMD_LINE_EDITOR);
               if(szText != NULL)
               {
                  pRequest = new EDF();
                  pRequest->AddChild("banner", szText);
                  CmdRequest(MSG_SYSTEM_EDIT, pRequest);
                  delete[] szText;
               }
            }
            break;

         case 'o':
            iID = CmdLineNum("ID of connection", "RETURN to abort", 0, -1);
            if(iID != -1)
            {
               pRequest = new EDF();
               pRequest->AddChild("connectionid", iID);
               if(CmdRequest(CmdVersion("2.5") >= 0 ? MSG_CONNECTION_CLOSE : "connection_drop", pRequest, &pReply) == false)
               {
                  iID = -1;
                  pReply->GetChild("connectionid", &iID);
                  strcpy(szWrite, "Cannot drop connection");
                  if(iID != -1)
                  {
                     sprintf(szWrite, "%s \0374%d\0373", szWrite, iID);
                  }
                  strcat(szWrite, "\n");
                  CmdWrite(szWrite);
               }
               else
               {
                  CmdWrite("Connection closed\n");
               }
            }
            break;

         case 'q':
            pRequest = DirectRequestMenu(&szOption);
            if(pRequest != NULL)
            {
               CmdRequest(szOption, pRequest, &pReply);
               pReply->Get(NULL, &szReply);
               // pWrite = pReply->Write(true, false, true, true);

               CmdPageOn();
               sprintf(szWrite, "Reply \0374%s\0370", szReply);
               /* if(lWriteLen > 1)
               {
                  sprintf(szWrite, "%s (\0374%ld\0370 bytes):", szWrite, lWriteLen);
               } */
               // strcat(szWrite, "\n");
               // CmdWrite(szWrite);
               // if(lWriteLen > 1)
               if(pReply->Children() > 0)
               {
                  CmdEDFPrint(szWrite, pReply, false, false);
                  // CmdWrite(pWrite, CMD_OUT_NOHIGHLIGHT);
                  // CmdWrite("\n");
               }
               else
               {
                  CmdWrite(szWrite);
                  CmdWrite("\n");
               }
               CmdPageOff();
               delete[] szReply;
               // delete[] pWrite;
            }
            delete[] szOption;
            break;

         /* case 'r':
            iID = CmdLineUser(CmdUserTab);
            if(iID != -1)
            {
               pRequest = new EDF();
               pRequest->AddChild("userid", iID);
               CmdRequest(MSG_USER_DELETE, pRequest);
            }
            break; */

         case 'r':
            ConnectionMenu();
            break;

         case 's':
            szText = CmdText();
            if(szText != NULL)
            {
               pRequest = new EDF();
               pRequest->AddChild("text", szText);
               CmdRequest(MSG_SYSTEM_MESSAGE, pRequest);
               delete[] szText;
            }
            break;

         case 't':
            iID = CmdLineUser(CmdUserTab, -1, NULL, true);
            if(iID != -1)
            {
               if(iID == iCurrID)
               {
                  CmdWrite("You are already SysOp\n");
               }
               else
               {
                  szOption = CmdLineStr("Really transfer SysOp level", "YES to proceed", LINE_LEN, 0, NULL, NULL, NULL);
                  if(szOption != NULL && strcmp(szOption, "YES") == 0)
                  {
                     pRequest = new EDF();
                     pRequest->AddChild("userid", iID);
                     CmdRequest(MSG_USER_SYSOP, pRequest);

                     delete[] szOption;

                     CmdUserReset();
                  }
               }
            }
            break;

         case 'u':
            szOption = CmdLineStr("Really shut down server", "YES to proceed");
            if(szOption != NULL && strcmp(szOption, "YES") == 0)
            {
               CmdRequest(MSG_SYSTEM_SHUTDOWN);
            }
            delete[] szOption;
            break;

         case 'v':
            szOption = CmdLineStr("Really reload server library", "YES to proceed");
            if(szOption != NULL && strcmp(szOption, "YES") == 0)
            {
               pEDF = new EDF();
               pEDF->Set("edf", "reload");
               m_pClient->Write(pEDF);
               delete pEDF;
            }
            delete[] szOption;
            break;

         case 'w':
            CmdRequest(MSG_SYSTEM_WRITE);
            break;

         case 'x':
            bLoop = false;
            break;

         case 'y':
            szOption = CmdLineStr("Really run maintenance", "YES to proceed");
            if(szOption != NULL && strcmp(szOption, "YES") == 0)
            {
               CmdRequest(MSG_SYSTEM_MAINTENANCE);
            }
            delete[] szOption;
            break;

         case '/':
            TalkCommandMenu(false, false, true, false, false, false);
            break;

         default:
            debug(DEBUGLEVEL_WARN, "AdminMenu no action for %c\n", cOption);
            break;
      }
   }

   delete pSystem;
}

void HelpMenu()
{
}

bool PageHistoryMenu()
{
   STACKTRACE
   int iUserID = 0, iUserEDF = 0;
   int iDate = -1, iSentDate = -1;
   bool bLoop = false;
   char *szUser = NULL;

   iUserID = CmdLineUser(CmdUserTab, -1);//, &szUser, true, NULL);
   if(iUserID == -1)
   {
      return false;
   }

   UserGet(m_pUserList, iUserID, &szUser, true, -1);

   m_pPaging->Root();
   bLoop = m_pPaging->Child("page");
   while(bLoop == true)
   {
      m_pPaging->Get(NULL, &iUserEDF);
      if(iUserEDF == iUserID)
      {
         // EDFPrint("PageHistoryMenu", m_pPaging, EDFElement::EL_CURR | EDFElement::PR_SPACE);
         // m_pPaging->GetChild("text", &szPrevFrom);
         // m_pPaging->GetChild("sent", &szPrevTo);

         m_pPaging->GetChild("date", &iDate);
         m_pPaging->GetChild("sentdate", &iSentDate);

         CmdWrite(DASHES);
         if(iDate != -1 && (iSentDate == -1 || iDate < iSentDate))
         {
            if(m_pPaging->IsChild("text") == true)
            {
               CmdUserPageView(m_pPaging, "From", szUser, "date", "text", false);
            }
            if(m_pPaging->IsChild("sent") == true)
            {
               CmdUserPageView(m_pPaging, "To", szUser, "sentdate", "sent", false);
            }
         }
         else
         {
            if(m_pPaging->IsChild("sent") == true)
            {
               CmdUserPageView(m_pPaging, "To", szUser, "sentdate", "sent", false);
            }
            if(m_pPaging->IsChild("text") == true)
            {
               CmdUserPageView(m_pPaging, "From", szUser, "date", "text", false);
            }
         }

         bLoop = false;
      }
      else
      {
         bLoop = m_pPaging->Next("page");
         if(bLoop == false)
         {
            CmdWrite("No page history for user\n");

            m_pPaging->Parent();
         }
      }
   }

   delete[] szUser;

   return true;
}

void UserWhoMenu()
{
   STACKTRACE
   EDF *pRequest = NULL, *pReply = NULL;
   int iWhoType = -1;

   iWhoType = CmdWholistInput();

   if(iWhoType != -1)
   {
      pRequest = new EDF();
      pRequest->AddChild("searchtype", 1);

      CmdRequest(MSG_USER_LIST, pRequest, &pReply);

      m_pUser->Root();
      CmdUserWho(pReply, iWhoType);
      delete pReply;
   }
}

bool TalkJoinMenu()
{
   STACKTRACE
   int iChannelID = -1;

   iChannelID = CmdLineChannel();

   if(iChannelID == -1)
   {
      return false;
   }

   if(CmdChannelJoin(iChannelID) == false)
   {
      return false;
   }

   return true;
}

bool TalkCommandMenu(bool bJoin, bool bPage, bool bSend, bool bActive, bool bWholist, bool bReturn)
{
   int iAccessLevel = LEVEL_NONE;
   bool bQuit = false;
   char cOption = '\0';
   char *szEmote = NULL, *szText = NULL;
   CmdInput *pInput = NULL;
   EDF *pRequest = NULL, *pReply = NULL;

   m_pUser->GetChild("accesslevel", &iAccessLevel);

   pInput = new CmdInput(CMD_MENU_NOCASE, "Talk");
   // pInput->MenuAdd('e', "Emote");
   pInput->MenuAdd('i', "channel Info");
   if(bJoin == true)
   {
      pInput->MenuAdd('j', "Join a channel");
   }
   pInput->MenuAdd('l', "List channels");
   if(mask(CmdInput::MenuStatus(), LOGIN_SHADOW) == false && bPage == true)
   {
      pInput->MenuAdd('p', "Page");
   }
   pInput->MenuAdd('q', "Quit talk");
   if(bSend == true)
   {
      pInput->MenuAdd('s', "Send");
      if(iAccessLevel >= LEVEL_WITNESS)
      {
         pInput->MenuAdd('c', "Channel message");
      }
   }
   if(bActive == true)
   {
      pInput->MenuAdd('t', "quiT talk (leave channel active)");
   }
   if(bWholist == true)
   {
      pInput->MenuAdd('w', "Who is logged in");
   }
   pInput->MenuAdd('x', bReturn == true ? "eXit (return to talk)" : "eXit");

   cOption = CmdMenu(pInput);
   switch(cOption)
   {
      case 'c':
      case 'e':
      case 's':
         szText = CmdText(CMD_LINE_NOTITLE);
         if(szText != NULL && strcmp(szText, "") != 0)
         {
            if(cOption == 'e')
            {
               szEmote = new char[strlen(szText) + 2];
               szEmote[0] = ':';
               strcpy(szEmote + 1, szText);

               delete[] szText;
               szEmote = szText;
            }

            pRequest = new EDF();
            pRequest->AddChild("channelid", CmdCurrChannel());
            pRequest->AddChild("text", szText);
            if(cOption == 'c')
            {
               pRequest->AddChild("nameless", true);
            }
            if(CmdRequest(MSG_CHANNEL_SEND, pRequest, &pReply) == false)
            {
               CmdEDFPrint("TalkMenu request failed", pReply);
            }
            delete pReply;
         }
         delete[] szText;
         break;

      case 'i':
         pRequest = new EDF();
         pRequest->AddChild("channelid", CmdCurrChannel());
         if(CmdRequest(MSG_CHANNEL_LIST, pRequest, &pReply) == true)
         {
            if(pReply->Child("channel") == true)
            {
               CmdEDFPrint("TalkMenu reply", pReply);

               CmdMessageTreeView(pReply, "channel");
            }
            else
            {
               CmdWrite("TalkMenu no channel section\n");
            }
         }
         else
         {
            CmdEDFPrint("TalkMenu request failed", pReply);
         }
         delete pReply;
         break;

      case 'l':
         pRequest = new EDF();
         pRequest->AddChild("searchtype", 3);

         CmdRequest(MSG_CHANNEL_LIST, pRequest, &pReply);
         // pReply->Sort("channel", "name", true);
         CmdMessageTreeList(pReply, "channel", 0, 1);
         break;

      case 'j':
         TalkJoinMenu();
         break;

      case 'p':
         CmdUserPage(NULL);
         break;

      case 'q':
         CmdChannelLeave();
         bQuit = true;
         break;

      case 't':
         if(CmdInput::MenuLevel() < CMDLEV_EXPERT)
         {
            CmdWrite("This channel will remain active. Type '/' to see channel options, '/q' to quit\n");
         }

         bQuit = true;
         break;

      case 'w':
         DefaultWholist();
         break;
   }

   return bQuit;
}

bool TalkMenu()
{
   STACKTRACE
   // int iMenuLevel = CMDLEV_BEGIN;
   bool bQuit = false;
   char cOption = '\0', szInit[2];
   char *szText = NULL;
   EDF *pRequest = NULL, *pReply = NULL;

   szInit[1] = '\0';

   if(TalkJoinMenu() == false)
   {
      return false;
   }

   // iMenuLevel = CmdInput::MenuLevel();
   if(CmdInput::MenuLevel() < CMDLEV_EXPERT)
   {
      CmdWrite("Welcome to multichannel talk\n\nType '/?' for help, '/q' to quit\n");
   }

   while(bQuit == false)
   {
      cOption = CmdMenu(TALK);

      switch(cOption)
      {
         case '/':
            bQuit = TalkCommandMenu(true, true, true, true, true, true);
            break;

         default:
            szInit[0] = cOption;
            szText = CmdText(CMD_LINE_NOTITLE, szInit);
            if(strcmp(szText, "") != 0)
            {
               pRequest = new EDF();
               pRequest->AddChild("channelid", CmdCurrChannel());
               pRequest->AddChild("text", szText);
               if(CmdRequest(MSG_CHANNEL_SEND, pRequest, &pReply) == false)
               {
                  CmdEDFPrint("TalkMenu request failed", pReply);
               }
               delete pReply;
            }
            delete[] szText;
            break;
      }
   }

   return true;
}

bool TalkMenuOld()
{
   STACKTRACE
   int iFolderID = 0, iMenuLevel = CMDLEV_BEGIN;
   bool bQuit = false;
   char cOption = '\0', szInit[2];
   char *szText = NULL, *szEmote = NULL;
   EDF *pRequest = NULL, *pReply = NULL;
   CmdInput *pInput = NULL;

   debug(DEBUGLEVEL_INFO, "TalkMenuOld entry\n");

   pRequest = new EDF();
   if(CmdVersion("2.5") < 0)
   {
      pRequest->AddChild("channelid", 0);
   }
   // EDFPrint("TalkMenuOld subscription request", pRequest);
   if(CmdRequest(CmdVersion("2.5") >= 0 ? MSG_FOLDER_ACTIVATE : MSG_CHANNEL_SUBSCRIBE, pRequest, &pReply) == false)
   {
      // EDFPrint("TalkMenuOld subscribe failed", pReply);

      delete pReply;

      debug(DEBUGLEVEL_INFO, "TalkMenuOld exit false\n");
      return false;
   }
   else if(CmdVersion("2.5") >= 0)
   {
      pReply->GetChild("folderid", &iFolderID);
   }
   // EDFPrint("TalkMenuOld subscribe success", pReply);

   szInit[1] = '\0';

   delete pReply;

   /* m_pUser->Root();
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      m_pUser->GetChild("menulevel", &iMenuLevel);
      m_pUser->Parent();
   } */
   iMenuLevel = CmdInput::MenuLevel();

   if(iMenuLevel < CMDLEV_EXPERT)
   {
      CmdWrite("Welcome to multichannel talk\n\nType '/?' for help, '/q' to quit\n");
   }

   while(bQuit == false)
   {
      cOption = CmdMenu(TALK);

      switch(cOption)
      {
         case '/':
            pInput = new CmdInput(CMD_MENU_NOCASE, "Talk");
            // pInput->MenuAdd('i', "channel Info");
            // pInput->MenuAdd('j', "Join a channel");
            // pInput->MenuAdd('l', "List new messages");
            pInput->MenuAdd('m', "eMote");
            if(mask(CmdInput::MenuStatus(), LOGIN_SHADOW) == false)
            {
               pInput->MenuAdd('p', "Page");
            }
            pInput->MenuAdd('q', "Quit talk");
            pInput->MenuAdd('w', "Who is logged in");
            pInput->MenuAdd('x', "eXit (return to talk)");
            cOption = CmdMenu(pInput);
            switch(cOption)
            {
               case 'm':
                  szText = CmdText(CMD_LINE_NOTITLE);
                  if(strcmp(szText, "") != 0)
                  {
                     szEmote = new char[strlen(szText) + 2];
                     szEmote[0] = ':';
                     strcpy(szEmote + 1, szText);

                     pRequest = new EDF();
                     if(CmdVersion("2.5") >= 0)
                     {
                        pRequest->AddChild("folderid", iFolderID);
                     }
                     else
                     {
                        pRequest->AddChild("channelid", 0);
                     }
                     pRequest->AddChild("text", szEmote);
                     // EDFPrint("TalkMenuOld send", pRequest);
                     CmdRequest(CmdVersion("2.5") >= 0 ? MSG_MESSAGE_ADD : MSG_CHANNEL_SEND, pRequest);

                     delete[] szEmote;
                  }
                  delete[] szText;
                  break;

               case 'p':
                  CmdUserPage(NULL);
                  break;

               case 'q':
                  pRequest = new EDF();
                  if(CmdVersion("2.5") >= 0)
                  {
                     pRequest->AddChild("folderid", iFolderID);
                  }
                  else
                  {
                     pRequest->AddChild("channelid", 0);
                  }
                  CmdRequest(CmdVersion("2.5") >= 0 ? MSG_FOLDER_DEACTIVATE : MSG_CHANNEL_UNSUBSCRIBE, pRequest);
                  bQuit = true;
                  break;

               case 'w':
                  DefaultWholist();
                  break;
            }
            break;

         default:
            szInit[0] = cOption;
            szText = CmdText(CMD_LINE_NOTITLE, szInit);
            if(strcmp(szText, "") != 0)
            {
               pRequest = new EDF();
               if(CmdVersion("2.5") >= 0)
               {
                  pRequest->AddChild("folderid", iFolderID);
               }
               else
               {
                  pRequest->AddChild("channelid", 0);
               }
               pRequest->AddChild("text", szText);
               // EDFPrint("TalkMenuOld send", pRequest);
               CmdRequest(CmdVersion("2.5") >= 0 ? MSG_MESSAGE_ADD : MSG_CHANNEL_SEND, pRequest);
            }
            delete[] szText;
            break;
      }
   }

   debug(DEBUGLEVEL_INFO, "TalkMenuOld exit true\n");
   return true;
}

bool PageMenu(EDF *pPage, bool bBell)
{
   STACKTRACE
   int iDate = 0, iFromID = -1, iPageID = 0, iServiceID = -1;
   bool bMessage = false, bFound = false, bLoop = false, bReturn = true;
   char cOption = '\0';
   char *szFolder = NULL, *szUser = NULL, *szService = NULL, *szServiceAction = NULL;
   char szTime[100], szWrite[200];
   CmdInput *pInput = NULL;

   pPage->GetChild("foldername", &szFolder);
   if(pPage->Child("message") == true)
   {
      bMessage = true;
   }
   pPage->GetChild("date", &iDate);
   pPage->GetChild("fromid", &iFromID);
   pPage->GetChild("fromname", &szUser);
   pPage->GetChild("serviceid", &iServiceID);
   pPage->GetChild("servicename", &szService);
   // tmTime = localtime((time_t *)&iValue);
   // strftime(szTime, 64, "%H:%M:%S", tmTime);
   StrTime(szTime, STRTIME_TIME, iDate);

   if(pPage->Child("serviceaction") == true)
   {
      pPage->Get(NULL, &szServiceAction);

      m_pServiceList->Root();

      if(stricmp(szServiceAction, ACTION_LOGIN) == 0 || stricmp(szServiceAction, ACTION_LOGOUT) == 0)
      {
         sprintf(szWrite, "Service \0374%s\0370 %s\n", szService, stricmp(szServiceAction, ACTION_LOGIN) == 0 ? "active" : "inactive");
         CmdWrite(szWrite);

         if(EDFFind(m_pServiceList, "service", iServiceID, false) == false)
         {
            m_pServiceList->Add("service", iServiceID);
         }
         m_pServiceList->SetChild("active", stricmp(szServiceAction, ACTION_LOGIN) == 0 ? true : false);

         m_pServiceList->Parent();

         if(g_pGame != NULL && g_pGame->ServiceID() == iServiceID && stricmp(szServiceAction, ACTION_LOGOUT) == 0)
         {
            delete g_pGame;
            g_pGame = NULL;
         }
      }
      else
      {
         // sprintf(szWrite, "Service \0374%s\0370 action\0374%s\0370\n", szService, szServiceAction != NULL ? " " : "");
         debug("PageMenu service %s action %s\n", szService, szServiceAction);

         bReturn = false;
      }
      // CmdWrite(szWrite);

      if(g_pGame != NULL && g_pGame->ServiceID() == iServiceID)
      {
         debug("PageMenu matched service ID %d with game\n", iServiceID);
         bReturn = g_pGame->Action(szServiceAction, pPage);

         m_pServiceList->Parent();
      }

      delete[] szServiceAction;

      pPage->Parent();
   }
   else
   {
      if(bBell == true)
      {
         CmdBeep();
      }
      if(bMessage == true)
      {
         sprintf(szWrite, "Message in \0374%s\0370 from \0374%s\0370", szFolder, szUser);
      }
      else
      {
         if(szUser != NULL)
         {
            sprintf(szWrite, "\0374%s\0370", szUser);
            if(szService != NULL)
            {
               sprintf(szWrite, "%s (\0374%s\0370)", szWrite, szService);
            }
            sprintf(szWrite, "%s is %spaging you", szWrite, bMessage == true ? "fake " : "");
         }
         else if(szService != NULL)
         {
            sprintf(szWrite, "\0374%s\0370 service", szService);
            if(pPage->IsChild("active") == true)
            {
               sprintf(szWrite, "%s \0374%s\0370", szWrite, pPage->GetChildBool("active") == true ? "on" : "off");
            }
         }
         sprintf(szWrite, "%s (%s)", szWrite, szTime);
      }
      pInput = new CmdInput(CMD_MENU_SIMPLE | CMD_MENU_NOCASE, szWrite);
      pInput->MenuAdd('a', "Accept", NULL, true);
      pInput->MenuAdd('i', "Ignore");

      cOption = CmdMenu(pInput);
      if(cOption == 'a')
      {
         CmdUserPage(pPage);
      }
      else if(iFromID != -1)
      {
         m_pPaging->Root();
         if(m_pPaging->Child("ignore") == false)
         {
            m_pPaging->Add("ignore");
         }
         else
         {
            while(m_pPaging->DeleteChild() == true);
         }

         m_pPaging->AddChild("date", iDate);
         m_pPaging->AddChild("fromid", iFromID);
         m_pPaging->AddChild("fromname", szUser);
         m_pPaging->AddChild(pPage, "text");

         m_pPaging->Parent();

         // iReturn = CLI_RESET;
         bReturn = false;
      }

      if(iFromID != -1)
      {
         m_pPaging->Root();
         bLoop = m_pPaging->Child("page");
         while(bLoop == true)
         {
            m_pPaging->Get(NULL, &iPageID);
            if(iPageID == iFromID)
            {
               m_pPaging->SetChild(pPage, "text");
               m_pPaging->SetChild("date", iDate);
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
            m_pPaging->Add("page", iFromID);
            m_pPaging->SetChild(pPage, "text");
            m_pPaging->SetChild("date", iDate);
         }
      }
   }

   delete[] szUser;

   if(bMessage == true)
   {
      pPage->Parent();
   }

   return bReturn;
}

void FolderPrioritiesMenu()
{
   int iFolderID = 0, iPriority = 0;
   char cOption = '\0';
   CmdInput *pInput = NULL;
   EDF *pRequest = NULL;

   m_pFolderNav->Root();
   CmdMessageTreeList(m_pFolderNav, "folder", 2, 7);

   while(cOption != 'x')
   {
      pInput = new CmdInput(CMD_MENU_NOCASE, "Priorities");
      pInput->MenuAdd('l', "List priorities");
      pInput->MenuAdd('s', "Set priority");
      pInput->MenuAdd('u', "Unset priority");
      pInput->MenuAdd('x', "eXit");

      cOption = CmdMenu(pInput);

      if(cOption == 'l')
      {
         m_pFolderNav->Root();
         CmdMessageTreeList(m_pFolderNav, "folder", 2, 7);
      }
      else if(cOption == 's' || cOption == 'u')
      {
         iFolderID = CmdLineFolder();
         if(iFolderID > 0)
         {
            if(cOption == 's')
            {
               iPriority = CmdLineNum("Priority", "RETURN to abort", 0, -1);
            }
            else
            {
               iPriority = 0;
            }
            if(iPriority != -1)
            {
               pRequest = new EDF();
               pRequest->AddChild("folderid", iFolderID);
               pRequest->AddChild("priority", iPriority);
               if(CmdRequest(MSG_FOLDER_SUBSCRIBE, pRequest) == true)
               {
                  if(FolderGet(m_pFolderNav, iFolderID, NULL, false) == true)
                  {
                     m_pFolderNav->SetChild("priority", iPriority);
                  }
               }
            }
         }
      }
   }
}

void MessageRulesMenu()
{
   STACKTRACE
   int iRuleID = 0;
   bool bLoop = false;
   char cOption = '\0';
   char szWrite[100];
   char *szMatch = NULL;
   CmdInput *pInput = NULL;
   EDF *pMatch = NULL, *pRequest = NULL, *pReply = NULL;

   while(cOption != 'x')
   {
      pReply = NULL;

      m_pUser->Root();

      if(m_pUser->Child("folders") == true)
      {
         debugEDFPrint("MessageRulesMenu folders", m_pUser, EDFElement::EL_CURR | EDFElement::PR_SPACE);

         bLoop = m_pUser->Child("rule");
         while(bLoop == true)
         {
            m_pUser->Get(NULL, &iRuleID);

            sprintf(szWrite, "Rule \0373%d\0370: ", iRuleID);
            szMatch = EDFToMatch(m_pUser);
            CmdWrite(szWrite);
            CmdWrite(szMatch);
            CmdWrite("\n");

            delete[] szMatch;

            bLoop = m_pUser->Next("rule");
            if(bLoop == false)
            {
               m_pUser->Parent();
            }
         }

         m_pUser->Parent();
      }

      pInput = new CmdInput(0, "Rules");
      pInput->MenuAdd('a', "Add rule");
      pInput->MenuAdd('d', "Delete rule");
      pInput->MenuAdd('x' , "eXit");

      cOption = CmdMenu(pInput);

      if(cOption == 'a')
      {
         szMatch = CmdText();

         if(szMatch != NULL)
         {
            pMatch = MatchToEDF(szMatch);
            CmdEDFPrint("MessageRulesMenu match", pMatch);

            pRequest = new EDF();
            pRequest->Add("folders");
            pRequest->Add("rules", "add");
            pRequest->Add("rule");
            pRequest->Copy(pMatch, false);
            pRequest->Parent();
            pRequest->Parent();
            pRequest->Parent();
            CmdEDFPrint("MessageRulesMenu request", pRequest);

            CmdRequest(MSG_USER_EDIT, pRequest, &pReply);

            CmdEDFPrint("MessageRulesMenu reply", pReply);

            delete pReply;

            delete pMatch;

            delete[] szMatch;

            CmdUserReset();
         }
      }
      else if(cOption == 'd')
      {
         iRuleID = CmdLineNum("Rule ID");

         pRequest = new EDF();
         pRequest->Add("folders");
         pRequest->Add("rules", "delete");
         pRequest->Add("ruleid", iRuleID);
         pRequest->Parent();
         pRequest->Parent();

         CmdEDFPrint("MessageRulesMenu request", pRequest);

         CmdRequest(MSG_USER_EDIT, pRequest, &pReply);

         CmdEDFPrint("MessageRulesMenu reply", pReply);

         delete pReply;

         CmdUserReset();
      }
   }
}

void MessageMenu()
{
   int iMessageID = -1, iVoteType = 0;
   char cOption = '\0';
   EDF *pRequest = NULL, *pReply = NULL, *pTemp = NULL, *pMessageIn, *pMessageOut = NULL;

   while(cOption != 'x')
   {
      cOption = CmdMenu(MESSAGE);
      switch(cOption)
      {
         case 'a':
            iMessageID = CmdLineNum("Message ID", NUMBER_LEN);
            CmdMessageRequest(-1, iMessageID, true, true, NULL, false, true);
            break;

         case 'c':
            iMessageID = CmdLineNum("Message ID", NUMBER_LEN);
            pRequest = new EDF();
            pRequest->AddChild("messageid", iMessageID);
            if(CmdRequest(MSG_MESSAGE_LIST, pRequest, false, &pTemp, false) == true)
            {
               // CmdEDFPrint("MessageMenu message", pTemp);

               if(pTemp->Child("message") == true)
               {
                  if(pTemp->Child("votes") == true)
                  {
                     pTemp->GetChild("votetype", &iVoteType);
                     pTemp->Parent();

                     if(mask(iVoteType, VOTE_CLOSED) == false)
                     {
                        pRequest->AddChild("votetype", iVoteType | VOTE_CLOSED);
                        if(CmdRequest(MSG_MESSAGE_EDIT, pRequest, &pReply) == false)
                        {
                           CmdEDFPrint("MessageMenu request failed", pReply);
                        }

                        delete pReply;
                     }
                  }
                  else
                  {
                     CmdWrite("Message is not a poll\n");
                  }
               }
            }
            else
            {
               CmdWrite("No such message\n");
            }
            delete pTemp;
            break;

         case 'f':
         case 's':
            MessageSearchMenu(-1, -1, true);
            break;

         case 'm':
            // FolderCatchup(false);
            MessageMarkMenu(true);
            break;

         case 'p':
            FolderPrioritiesMenu();
            break;

         case 'r':
            MessageRulesMenu();
            break;

         /* case 's':
            MessageSearchMenu(-1, -1, false);
            break; */

         case 'v':
            CmdMessageAdd(-1, -1, -1, -1, NULL, MSGTYPE_VOTE);
            break;

         case '/':
            TalkCommandMenu(false, false, true, false, false, false);
            break;
      }
   }
}

void BulletinMenu(bool bShowAll)
{
   STACKTRACE
   int iAccessLevel = LEVEL_NONE, iBulletinID = 0, iMsgNum = 0, iNumMsgs = 0;
   bool bLoop = false;
   char cOption = '\0';
   EDF *pList = NULL, *pRequest = NULL, *pBulletin = NULL;
   CmdInput *pInput = NULL;

   m_pUser->Root();
   m_pUser->GetChild("accesslevel", &iAccessLevel);

   CmdRequest(MSG_BULLETIN_LIST, &pList);

   pList->GetChild("nummsgs", &iNumMsgs);

   bLoop = pList->Child("bulletin");
   if(bLoop == true)
   {
      while(cOption != 'x' && bLoop == true)
      {
         iMsgNum++;

         if(bShowAll == true || pList->GetChildBool("read") == false)
         {
            pList->Get(NULL, &iBulletinID);

            pRequest = new EDF();
            pRequest->AddChild("bulletinid", iBulletinID);

            if(CmdRequest(MSG_BULLETIN_LIST, pRequest, &pBulletin) == true)
            {
               if(pBulletin->Child("bulletin") == true)
               {
                  CmdMessageView(pBulletin, -1, NULL, iMsgNum, iNumMsgs, CmdBrowser() != NULL);

                  pInput = new CmdInput(CMD_MENU_SIMPLE | CMD_MENU_NOCASE, "Kill");
                  pInput->MenuAdd('c', "Continue", NULL, true);
                  if(iAccessLevel >= LEVEL_WITNESS)
                  {
                     pInput->MenuAdd('k', "Kill");
                  }
                  pInput->MenuAdd('x', "eXit");

                  cOption = CmdMenu(pInput);

                  pBulletin->Parent();
               }
            }

            delete pBulletin;
         }

         bLoop = pList->Next("bulletin");
      }
   }
}

void BusyMenu()
{
   char cOption = '\0';
   int iStatus = LOGIN_OFF;
   char szWrite[100];
   char *szText = NULL;
   CmdInput *pInput = NULL;
   EDF *pRequest = NULL;

   pInput = new CmdInput(0, "Busy");
   iStatus = CmdInput::MenuStatus();
   sprintf(szWrite, "toggle Pager (currently \0374%s\0370)", mask(iStatus, LOGIN_BUSY) == true ? "off" : "on");
   pInput->MenuAdd('p', szWrite);
   pInput->MenuAdd('s', "change Status message");
   pInput->MenuAdd('x', "eXit");

   cOption = CmdMenu(pInput);

   if(cOption == 'p')
   {
      if(mask(iStatus, LOGIN_BUSY) == true)
      {
         iStatus -= LOGIN_BUSY;
      }
      else
      {
         iStatus += LOGIN_BUSY;
      }
   }

   if(cOption != 'x')
   {
      szText = CmdLineStr("Message", "RETURN for none", LINE_LEN);
      if(szText != NULL)
      {
         pRequest = new EDF();
         pRequest->Add("login");
         if(cOption == 'p')
         {
            pRequest->AddChild("status", iStatus);
         }
         pRequest->AddChild("statusmsg", szText);
         pRequest->Parent();

         // CmdEDFPrint("MainMenu busy request", pRequest);
         CmdRequest(MSG_USER_EDIT, pRequest);
         CmdUserReset();

         delete[] szText;
      }
   }
}

void GameMenu()
{
   STACKTRACE
   int iServiceID = 0, iOption = 0;
   bool bMenu = true, bLoop = false;
   char cOption = '\0';
   char szWrite[200];
   char *szName = NULL, *szContentType = NULL;
   CmdInput *pInput = NULL;
   EDF *pOptions = NULL;
   Game *pTemp = NULL;

   while(bMenu == true)
   {
      cOption = CmdMenu(GAME);

      if(g_pGame != NULL && g_pGame->IsRunning() == true)
      {
         g_pGame->Key(cOption);

         if(g_pGame->IsEnded() == true)
         {
            delete g_pGame;
            g_pGame = NULL;
         }
      }
      else
      {
         switch(cOption)
         {
            case 'c':
            case 'j':
               iServiceID = -1;
               bLoop = m_pServiceList->Child("service");
               while(bLoop == true)
               {
                  if(m_pServiceList->GetChild("content-type", &szContentType) == true && szContentType != NULL)
                  {
                     pTemp = FindGame(szContentType);
                     if(pTemp != NULL)
                     {
                        m_pServiceList->Get(NULL, &iServiceID);
                        m_pServiceList->GetChild("name", &szName);

                        sprintf(szWrite, "\0373%d\0370: \0373%s\0370\n", iServiceID, szName);
                        CmdWrite(szWrite);

                        delete[] szName;

                        delete pTemp;
                     }
                     delete[] szContentType;
                  }

                  bLoop = m_pServiceList->Next("service");
                  if(bLoop == false)
                  {
                     m_pServiceList->Parent();
                  }
               }

               if(iServiceID != -1)
               {
                  iOption = CmdLineNum("Game");
                  if(EDFFind(m_pServiceList, "service", iOption, false) == true)
                  {
                     m_pServiceList->GetChild("content-type", &szContentType);

                     g_pGame = FindGame(szContentType);

                     if(cOption == 'c')
                     {
                        pOptions = g_pGame->CreateOptions();

                        if(g_pGame->Create(iOption, pOptions) == false)
                        {
                           CmdWrite("Cannot create game\n");
                           delete g_pGame;
                           g_pGame = NULL;
                        }

                        delete pOptions;
                     }
                     else if(cOption == 'j')
                     {
                        pOptions = g_pGame->JoinOptions();

                        if(g_pGame->Join(iOption, pOptions) == false)
                        {
                           CmdWrite("Cannot join game\n");
                           delete g_pGame;
                           g_pGame = NULL;
                        }

                        delete pOptions;
                     }

                     delete[] szContentType;

                     m_pServiceList->Parent();
                  }
                  else
                  {
                     CmdWrite("Invalid game\n");
                  }
               }
               else
               {
                  CmdWrite("No games found\n");
               }
               break;

            case 'e':
               g_pGame->End();
               break;

            case 's':
               pOptions = g_pGame->StartOptions();

               if(g_pGame->Start(pOptions) == false)
               {
                  CmdWrite("Cannot start game\n");
                  delete g_pGame;
                  g_pGame = NULL;
               }

               delete pOptions;
               break;

            case 'x':
               if(g_pGame != NULL)
               {
                  g_pGame->End();
               }

               bMenu = false;
               break;
         }
      }
   }

   delete g_pGame;
   g_pGame = NULL;
}

void MainMenu()
{
   STACKTRACE
   bool bQuit = false, bRequest = false;
   int iStatus = LOGIN_OFF, iAccessLevel = LEVEL_NONE, iMessageID = 0, iFolderID = 0;
   char cOption = '\0', szWrite[200];
   char *szText = NULL, *szName = NULL, *szMessage = NULL;
   EDF *pRequest = NULL, *pReply = NULL;

   debug(DEBUGLEVEL_INFO, "MainMenu entry\n");

   m_pUser->Root();
   m_pUser->GetChild("accesslevel", &iAccessLevel);

   while(bQuit == false)
   {
      cOption = CmdMenu(MAIN);
      switch(cOption)
      {
         case 'a':
            AdminMenu();
            break;

         case 'b':
            bRequest = false;

            iStatus = CmdInput::MenuStatus();
            if(mask(iStatus, LOGIN_BUSY) == false)
            {
               iStatus += LOGIN_BUSY;
               szText = CmdLineStr("Busy message", "RETURN for none", LINE_LEN);
               if(szText != NULL)
               {
                  bRequest = true;
               }
            }
            else
            {
               iStatus -= LOGIN_BUSY;

               if(cOption == 'B')
               {
                  szText = CmdLineStr("Busy message", "RETURN for none", LINE_LEN);
                  if(szText != NULL)
                  {
                     bRequest = true;
                  }
               }
               else
               {
                  bRequest = true;
                  szText = NULL;
               }
            }

            if(bRequest == true)
            {
               pRequest = new EDF();
               pRequest->Add("login");
               pRequest->AddChild("status", iStatus);
               if(mask(iStatus, LOGIN_BUSY) == false || (szText != NULL && stricmp(szText, "") != 0))
               {
                  pRequest->AddChild(CmdVersion("2.5") >= 0 ? "statusmsg" : "busymsg", szText);
               }
               delete[] szText;
               pRequest->Parent();

               // CmdEDFPrint("MainMenu busy request", pRequest);
               CmdRequest(MSG_USER_EDIT, pRequest);
               CmdUserReset();
            }
            break;

         case 'B':
            BusyMenu();
            break;

         case 'c':
            UserWhoMenu();
            break;

         case 'd':
            UserEditMenu(-1);
            break;

         case 'e':
            CmdMessageAdd(-1, -1);
            break;

         case 'f':
            CmdWrite("Refreshing lists...\n");
            CmdRefresh(true);
            break;

         case 'g':
         case 'q':
            if(CmdYesNo("Are you sure you want to logout", false) == true)
            {
               bRequest = false;
               pRequest = NULL;

               if(cOption == 'q')
               {
                  szText = CmdLineStr("Message", LINE_LEN);
                  if(szText != NULL)
                  {
                     bRequest = true;

                     if(strcmp(szText, "") != 0)
                     {
                        pRequest = new EDF();
                        pRequest->AddChild("text", szText);
                     }
                  }
               }
               else
               {
                  bRequest = true;
               }

               if(bRequest == true)
               {
                  m_pUser->Root();
                  m_pUser->GetChild("name", &szName);
                  sprintf(szWrite, "\nGoodbye \0374%s\0370, call again soon!\n", szName);
                  CmdWrite(szWrite);
                  delete[] szName;

                  debug(DEBUGLEVEL_DEBUG, "MainMenu logout request sending supress true...\n");
                  CmdRequest(MSG_USER_LOGOUT, pRequest, true, NULL, true);
                  debug(DEBUGLEVEL_DEBUG, "MainMenu logout request sent\n");
                  bQuit = true;
               }
            }
            break;

         case 'h':
            HelpMenu();
            break;

         case 'i':
            m_pPaging->Root();
            if(m_pPaging->Child("ignore") == true)
            {
               CmdUserPage(m_pPaging);
               m_pPaging->Parent();
            }
            break;

         case 'j':
         case 'J':
            // Join folder
            if(CmdVersion("2.5") >= 0)
            {
               // Jump to message
               szMessage = NULL;
               iMessageID = -1;

               if(cOption == 'j')
               {
                  iFolderID = CmdLineFolder("Folder name / message ID", -1, &szMessage, false);
                  if(iFolderID > 0)
                  {
                     FolderJoinMenu(iFolderID);
                  }
                  else
                  {
                     if(szMessage != NULL)
                     {
                        if(isdigit(szMessage[0]))
                        {
                           iMessageID = atoi(szMessage);
                        }
                        else
                        {
                           CmdWrite("No such folder\n");
                        }
                     }
                  }
               }
               else
               {
                  iMessageID = CmdLineNum("Message number");
               }

               if(iMessageID > 0)
               {
                  if(CmdMessageRequest(iFolderID, iMessageID) == true)
                  {
                     if(cOption == 'j')
                     {
                        FolderMenu(CmdCurrFolder(), CmdCurrMessage(), MSG_EXIT);
                     }
                  }
                  /* else
                  {
                     CmdWrite("No such message\n");
                  } */
               }
            }
            else
            {
               FolderJoinMenu(-1);
            }
            break;

         case 'k':
            GameMenu();
            break;

         case 'l':
            FolderListMenu();
            break;

         case 'm':
            MessageMenu();
            break;

         case 'n':
            FolderMenu(-1, -1, MSG_NEW);
            break;

         case 'o':
            PageHistoryMenu();
            break;

         case 'p':
         case 'P':
            CmdUserPage(NULL, -1, cOption == 'P');
            break;

         case 's':
            pRequest = new EDF();
            pRequest->AddChild("searchtype", 2);
            CmdRequest(MSG_USER_LIST, pRequest, &pReply);
            CmdUserLast(pReply);
            delete pReply;
            break;

         case 't':
            if(CmdVersion("2.6") >= 0)
            {
               TalkMenu();
            }
            else
            {
               TalkMenuOld();
            }
            break;

         case 'T':
            pRequest = new EDF();
            pRequest->AddChild("searchtype", 3);

            CmdRequest(MSG_CHANNEL_LIST, pRequest, &pReply);
            CmdMessageTreeList(pReply, "channel", 0, 1);
            break;

         case 'u':
            UserListMenu();
            break;

         case 'w':
            DefaultWholist();
            break;

         case 'y':
            BulletinMenu(true);
            break;

         case '/':
            TalkCommandMenu(false, false, true, false, false, false);
            break;

         default:
            debug(DEBUGLEVEL_WARN, "MainMenu no action for %c\n", cOption);
            break;
      }
   }

   debug(DEBUGLEVEL_INFO, "MainMenu exit\n");
}

bool CmdAttachmentDir(const char *szAttachmentDir)
{
   m_szAttachmentDir = strmk(szAttachmentDir);

   return true;
}

char *CmdAttachmentDir()
{
   return m_szAttachmentDir;
}

int MessagePos(EDF *pMessage)
{
   int iMessageID = 0, iMessageEDF = 0, iMsgPos = 0;
   bool bLoop = false;

   // debug("MessagePos %p, %p -> ", pMessage, m_pMessageList);
   if(pMessage == NULL || m_pMessageList == NULL)
   {
      // debug("0\n");
      return 0;
   }

   pMessage->Get(NULL, &iMessageID);

   m_pMessageList->Root();
   bLoop = m_pMessageList->Child("message");
   while(bLoop == true)
   {
      iMsgPos++;

      m_pMessageList->Get(NULL, &iMessageEDF);
      if(iMessageID == iMessageEDF)
      {
         bLoop = false;
      }
      else
      {
         bLoop = m_pMessageList->Iterate("message");
         if(bLoop == false)
         {
            iMsgPos = 0;
         }
      }
   }

   // debug("%d\n", iMsgPos);
   return iMsgPos;
}

int MessageCount()
{
   int iNumMsgs = 0;

   // debug("MessageCount %p -> ", m_pMessageList);
   if(m_pMessageList == NULL)
   {
      // debug("0\n");
      return 0;
   }

   m_pMessageList->Root();
   iNumMsgs = m_pMessageList->Children("message", true);
   // debug("%d\n", iNumMsgs);
   return iNumMsgs;
}

/* void CmdInput::MenuTime(long lTime)
{
   m_iServerDiff = time(NULL) - lTime;
   // printf("CmdInput::MenuTime diff %d\n", m_iServerDiff);
}

int CmdInput::MenuTime()
{
   // printf("CmdInput::MenuTime %d -> %d\n", time(NULL), time(NULL) - m_iServerDiff);
   return time(NULL) - m_iServerDiff;
} */

void NewFeatures()
{
   /* char szWrite[100];

   CmdWrite("\n");

   sprintf(szWrite, "New stuff in \0374%s\0370 build \0374%d\0370, \0374%s\0370:\n\n", CLIENT_NAME(), BuildNum(), BuildDate());
   CmdWrite(szWrite);

   CmdWrite("  - Case sensitive menu options (see 'Menu level' in 'Details')\n");
   CmdWrite("  - 'Show only' message navigation keys (case sensitive mode)\n");
   CmdWrite("  - 'RETURN to abort' removed from prompts for expert menu level\n");
   CmdWrite("  - Banner and YP entries centered for display\n");
   CmdWrite("\n"); */
}

bool ServiceActivate(EDF *pData)
{
   STACKTRACE
   int iServiceID = -1;
   bool bLoop = false;
   char szWrite[100];
   char *szService = NULL;
   EDF *pRequest = NULL, *pReply = NULL;

   pData->Root();
   bLoop = pData->Child("service");
   while(bLoop == true)
   {
      pData->Get(NULL, &iServiceID);

      pRequest = new EDF();
      pRequest->AddChild("serviceid", iServiceID);
      pRequest->AddChild("active", true);
      pRequest->AddChild(pData, "name");
      pRequest->AddChild(pData, "password");
      // pRequest->AddChild("active", true);

      if(CmdRequest(MSG_SERVICE_SUBSCRIBE, pRequest, &pReply) == false)
      {
         szService = NULL;
         pReply->GetChild("servicename", &szService);
         if(szService != NULL)
         {
            sprintf(szWrite, "Cannot activate \0374%s\0370 service\n", szService);
            delete[] szService;
         }
         else
         {
            sprintf(szWrite, "Cannot find service \0374%d\0370\n", iServiceID);
         }
         CmdWrite(szWrite);
      }

      delete pReply;

      bLoop = pData->Next("service");
      if(bLoop == false)
      {
         pData->Parent();
      }
   }

   return true;
}

void DefaultWholist()
{
   int iWhoType = 7;
   EDF *pRequest = NULL, *pReply = NULL;

   pRequest = new EDF();
   pRequest->AddChild("searchtype", 1);
   CmdRequest(MSG_USER_LIST, pRequest, &pReply);
   m_pUser->Root();
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      m_pUser->GetChild("wholist", &iWhoType);
      m_pUser->Parent();
   }

   // debug("DefaultWholist calling\n");

   CmdUserWho(pReply, iWhoType);

   delete pReply;
}

bool CreateUserMenu(bool bLoggedIn, char **szUsername, char **szPassword)//, EDF *pSystemList)
{
   STACKTRACE
   int iGender = 0;
   bool bLoop = true;
   char *szNewUser = NULL, *szName = NULL, *szRealName = NULL, *szEmail = NULL, *szRefer = NULL, *szPW1 = NULL, *szPW2 = NULL;
   char szWrite[200];
   char cOption = '\0';
   EDF *pUserList = NULL, *pRequest = NULL, *pReply = NULL;
   CmdInput *pInput = NULL;

   if(bLoggedIn == false)
   {
      m_pServiceList->GetChild("newuser", &szNewUser);
      CmdWrite("\n");
      if(szNewUser != NULL)
      {
         CmdWrite(szNewUser, CMD_OUT_NOHIGHLIGHT);
         CmdWrite("\n\n");
         delete[] szNewUser;
      }

      if(CmdYesNo("Do you still want to become a user", false) == false)
      {
         CmdShutdown("Goodbye then");
      }
   }

   CmdRequest(MSG_USER_LIST, &pUserList);

   while(bLoop == true)
   {
      bLoop = true;
      while(bLoop == true)
      {
         delete[] szName;
         szName = CmdLineStr(bLoggedIn == true ? "Name of user" : "What name would you like to use (spaces allowed)", UA_NAME_LEN, bLoggedIn == false ? CMD_LINE_NOESCAPE : 0);
         if(szName == NULL)
         {
            delete pUserList;

            return false;
         }
         else if(NameValid(szName) == false)
         {
            CmdWrite("Sorry, that name is invalid. Please choose another\n");
         }
         else if(UserGet(pUserList, szName) != -1)
         {
            CmdWrite("Sorry, that name is already in user. Please choose another\n\n");
         }
         else
         {
            bLoop = false;
         }
      }

      if(bLoggedIn == false)
      {
         *szUsername = szName;
      }

      szRealName = CmdLineStr("Real name", LINE_LEN, CMD_LINE_NOESCAPE);
      szEmail = CmdLineStr("Email address", LINE_LEN, CMD_LINE_NOESCAPE);
      if(bLoggedIn == false && CmdVersion("2.5") >= 0)
      {
         szRefer = CmdLineStr("How did you hear about us", LINE_LEN, CMD_LINE_NOESCAPE);
      }

      bLoop = true;
      while(bLoop == true)
      {
         szPW1 = CmdLineStr("Password (3 characters minimum)", UA_NAME_LEN, CMD_LINE_SILENT | CMD_LINE_NOESCAPE);
         if(strlen(szPW1) < 3)
         {
            CmdWrite("Sorry, that password is too short\n");
         }
         else
         {
            szPW2 = CmdLineStr("Confirm password", UA_NAME_LEN, CMD_LINE_SILENT | CMD_LINE_NOESCAPE);
            if(strcmp(szPW1, szPW2) != 0)
            {
               CmdWrite("Sorry, passwords do not match\n");
            }
            else
            {
               bLoop = false;
            }
         }
      }

      if(bLoggedIn == false)
      {
         *szPassword = szPW1;
      }
      delete[] szPW2;

      pInput = new CmdInput(CMD_MENU_SIMPLE | CMD_MENU_NOCASE, "Gender");
      pInput->MenuAdd('m', "Male");
      pInput->MenuAdd('f', "Female");
      cOption = CmdMenu(pInput);
      if(cOption == 'm')
      {
         iGender = GENDER_MALE;
      }
      else if(cOption == 'f')
      {
         iGender = GENDER_FEMALE;
      }

      sprintf(szWrite, "Name:      %s\n", szName);
      CmdWrite(szWrite);
      sprintf(szWrite, "Real name: %s\n", szRealName);
      CmdWrite(szWrite);
      sprintf(szWrite, "Email:     %s\n", szEmail);
      CmdWrite(szWrite);
      sprintf(szWrite, "Gender:    %s\n", GenderType(iGender));
      CmdWrite(szWrite);

      bLoop = true;
      if(CmdYesNo("Is this correct", false) == true)
      {
         pRequest = new EDF();
         pRequest->AddChild("name", szName);
         pRequest->AddChild("password", szPW1);
         pRequest->AddChild("gender", iGender);
         pRequest->Add("details", "add");
         if(szRealName != NULL)
         {
            pRequest->AddChild("realname", szRealName);
         }
         if(szEmail != NULL)
         {
            pRequest->AddChild("email", szEmail);
         }
         if(szRefer != NULL)
         {
            pRequest->AddChild("refer", szRefer);
         }
         pRequest->Parent();
         if(CmdRequest(MSG_USER_ADD, pRequest, &pReply) == false)
         {
            CmdEDFPrint("Unable to create user", pReply);
         }
         else
         {
            bLoop = false;
         }
         delete pReply;
      }
      else if(bLoggedIn == true)
      {
         bLoop = false;
      }
   }

   delete[] szRealName;
   delete[] szEmail;

   delete pUserList;

   return true;
}
