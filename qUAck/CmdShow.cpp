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
** CmdShow.cpp: Implementation of reply display functions
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#include "Conn/EDFConn.h"

#include "ua.h"

#include "CmdIO.h"
#include "CmdInput.h"
#include "CmdMenu.h"

#include "client/CliUser.h"

#include "CmdShow.h"
#include "CmdTable.h"

#include "qUAck.h"

char *DAYS[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

void CmdTitle(const char *szString)
{
   CmdWrite("\n");
   CmdWrite(szString);
   CmdWrite("\n");
}

void CmdField(const char *szName, char cColour, const char *szValue, int iOptions)
{
   char szColour[10];

   CmdWrite(szName);
   CmdWrite(": ");

   if(cColour != '\0')
   {
      sprintf(szColour, "\037%c", cColour);
      CmdWrite(szColour);
   }
   CmdWrite(szValue, iOptions);
   if(cColour != '\0')
   {
      CmdWrite("\0370");
   }

   CmdWrite("\n");
}

void CmdField(const char *szName, char cColour, int iValue)
{
   char szValue[32];

   sprintf(szValue, "%d", iValue);
   CmdField(szName, cColour, szValue, 0);
}

void CmdField(const char *szName, const char *szValue, int iOptions)
{
   CmdField(szName, '\0', szValue, iOptions);
}

void CmdTime(const char *szName, char cColour, int iType, int iTime)
{
   char szValue[100];

   StrTime(szValue, iType, iTime, cColour);
   CmdField(szName, '\0', szValue, 0);
}

void CmdValue(const char *szName, char cColour, int iType, int iValue)
{
   char szValue[100];

   StrValue(szValue, iType, iValue, cColour);
   CmdField(szName, '\0', szValue, 0);
}

void CmdMessageTreeAccess(char *szString, int iAccessMode)
{
   sprintf(szString, "%c%c%c%c%c%c%c%c%c%c%c",
      mask(iAccessMode, ACCMODE_MEM_READ) == true ? 'r' : '-',
      mask(iAccessMode, ACCMODE_MEM_WRITE) == true ? 'w' : '-',
      mask(iAccessMode, FOLDERMODE_MEM_SDEL) == true ? 's' : '-',
      mask(iAccessMode, FOLDERMODE_MEM_ADEL) == true ? 'a' : '-',
      mask(iAccessMode, FOLDERMODE_MEM_MOVE) == true ? 'm' : '-',
      mask(iAccessMode, ACCMODE_SUB_READ) == true ? 'r' : '-',
      mask(iAccessMode, ACCMODE_SUB_WRITE) == true ? 'w' : '-',
      mask(iAccessMode, FOLDERMODE_SUB_SDEL) == true ? 's' : '-',
      mask(iAccessMode, FOLDERMODE_SUB_ADEL) == true ? 'a' : '-',
      mask(iAccessMode, FOLDERMODE_SUB_MOVE) == true ? 'm' : '-',
      mask(iAccessMode, ACCMODE_PRIVATE) == true ? 'p' : '-');
}

// CmdSubList: Show subscription information
int CmdSubList(EDF *pReply, int iType, int iDisplay, const char *szSpace, bool bRetro)
{
   STACKTRACE
   int iNumTypes = 0, iLoggedIn = 0;
   bool bLoop = true, bFirst = true;
   char szWrite[100];
   char *szName = NULL, *szType = NULL, *szTitle = NULL;

   // EDFPrint("CmdSubList entry", pReply);

   if(iType == SUBTYPE_EDITOR)
   {
      szType = SUBNAME_EDITOR;
      if(mask(iDisplay, SUBINFO_USER) == true)
      {
         szTitle = "Editorship";
      }
      else
      {
         szTitle = "Editor";
      }
   }
   else if(iType == SUBTYPE_MEMBER)
   {
      szType = SUBNAME_MEMBER;
      if(mask(iDisplay, SUBINFO_USER) == true)
      {
         szTitle = "Membership";
      }
      else
      {
         szTitle = "Member";
      }
   }
   else
   {
      szType = SUBNAME_SUB;
      if(iType == SUBTYPE_SUB && mask(iDisplay, SUBINFO_USER) == true)
      {
         szTitle = "Subscription";
      }
      else
      {
         szTitle = "Subscriber";
      }
   }

   iNumTypes = pReply->Children(szType);
   // printf("CmdSubList %d of type %s\n", iNumTypes, szType);

   if(mask(iDisplay, SUBINFO_TOTAL) == false || iNumTypes > 0)
   {
      if(mask(iDisplay, SUBINFO_TOTAL) == true)
      {
         // Simple quantity output
         sprintf(szWrite, "%s%s: %s\0373%d\0370", szTitle, iNumTypes != 1 ? "s" : "", szSpace != NULL ? szSpace : "", iNumTypes);
         if(CmdVersion("2.5") >= 0)
         {
            bLoop = pReply->Child(szType);
            while(bLoop == true)
            {
               if(pReply->GetChildBool("login") == true)
               {
                  iLoggedIn++;
               }
               bLoop = pReply->Next(szType);
               if(bLoop == false)
               {
                  pReply->Parent();
               }
            }
            if(iLoggedIn > 0)
            {
               sprintf(szWrite, "%s (\0373%d\0370 logged in)", szWrite, iLoggedIn);
            }
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);
      }
      else if(iNumTypes > 0)
      {
         pReply->Sort(szType, "name", false);

         bLoop = pReply->Child(szType);
         while(bLoop == true)
         {
            pReply->GetChild("name", &szName);
            if(iType >= SUBTYPE_EDITOR && mask(iDisplay, SUBINFO_USER) == true)
            {
               sprintf(szWrite, "%s: %s\0373%s\0370\n", szTitle, szSpace != NULL ? szSpace : "", szName);
            }
            else
            {
               if(bFirst == true)
               {
                  sprintf(szWrite, "%s%s: %s\0373%s\0370", szTitle, iNumTypes != 1 ? "s" : "", szSpace != NULL ? szSpace : "", szName);
                  bFirst = false;
               }
               else
               {
                  sprintf(szWrite, ", \0373%s\0370", szName);
               }
            }
            if(mask(iDisplay, SUBINFO_ACTIVE) == true && pReply->GetChildBool("active") == true)
            {
               strcat(szWrite, " (\0373active\0370)");
            }
            CmdWrite(szWrite);
            delete[] szName;

            bLoop = pReply->Next(szType);
            if(bLoop == false)
            {
               pReply->Parent();
               if(iType < SUBTYPE_EDITOR || mask(iDisplay, SUBINFO_USER) == false)
               {
                  CmdWrite("\n");
               }
            }
         }
      }
   }

   return iNumTypes;
}

// NumPos: Calculate positional ending for number based on a value
char *NumPos(int iValue)
{
   if(iValue % 10 == 1 && iValue % 100 != 11)
   {
      return "st";
   }
   else if(iValue % 10 == 2 && iValue % 100 != 12)
   {
      return "nd";
   }
   else if(iValue % 10 == 3 && iValue % 100 != 13)
   {
      return "rd";
   }
   else
   {
      return "th";
   }
}

// AccessColour: Return a colour code based on the access level
char AccessColour(int iLevel, int iType)
{
   if(mask(iType, USERTYPE_AGENT) == true)
   {
      return '5';
   }

   switch(iLevel)
   {
      case LEVEL_GUEST:
         return '3';

      case LEVEL_MESSAGES:
         return '3';

      case LEVEL_EDITOR:
         return '6';

      case LEVEL_WITNESS:
         return '4';

      case LEVEL_SYSOP:
         return '2';
   }

   return '0';
}

// GenderObject: Return a gender type based on the gender value
char *GenderObject(int iGender)
{
   if(iGender == GENDER_MALE)
   {
      return "his";
   }
   else if(iGender == GENDER_FEMALE)
   {
      return "her";
   }
   else if(iGender == GENDER_NONE)
   {
      return "its";
   }

   return "their";
}

// GenderType: Return a gender type string based on the gender value
char *GenderType(int iGender)
{
   if(iGender == GENDER_MALE)
   {
      return "Male";
   }
   else if(iGender == GENDER_FEMALE)
   {
      return "Female";
   }
   else if(iGender == GENDER_NONE)
   {
      return "Object";
   }
   return "Person";
}

void StrTime(char *szTime, int iType, time_t iTime, char cCol, const char *szBefore, const char *szAfter)
{
   STACKTRACE
   int iYear = 0;
   time_t iNow = time(NULL);
   char *szFormat = NULL;
   struct tm *tmTime = NULL;
   // debug("StrTime entry %d %d %c %s %s", iType, iTime, cCol, szBefore, szAfter);

   if(iTime < 0)
   {
      debug(DEBUGLEVEL_WARN, "StrTime negative %d\n", iTime);
   }

   if(szBefore != NULL)
   {
      strcpy(szTime, szBefore);
   }
   else
   {
      strcpy(szTime, "");
   }

   if(cCol != '\0')
   {
      sprintf(szTime, "%s\037%c", szTime, cCol);
   }

   STACKTRACEUPDATE

   tmTime = localtime((time_t*)&iNow);
   iYear = tmTime->tm_year;
   tmTime = localtime((time_t*)&iTime);

   STACKTRACEUPDATE

   if(iType == STRTIME_TIME)
   {
      szFormat = "%H:%M:%S";
   }
   else if(iType == STRTIME_TIMEHM)
   {
      szFormat = "%H:%M";
   }
   else if(iType == STRTIME_DATE)
   {
      szFormat = "%d/%m/%y";
   }
   else if(iType == STRTIME_SHORT)
   {
      szFormat = "%H:%M:%S %d/%m/%y";
   }
   else if(iType == STRTIME_MEDIUM)
   {
      if(iYear != tmTime->tm_year)
      {
         szFormat = "%H:%M, %a %d/%m/%y";
      }
      else
      {
         szFormat = "%H:%M, %a %d/%m";
      }
   }
   else
   {
      szFormat = "%A, %d %B %Y - %H:%M:%S";
   }

   strftime(szTime + strlen(szTime), 100, szFormat, tmTime);

   if(cCol != '\0')
   {
      strcat(szTime, "\0370");
   }

   if(szAfter != NULL)
   {
      strcat(szTime, szAfter);
   }

   // debug(" -> '%s'\n", szTime);
}

bool StrValueAdd(char *szValue, int iValue, const char *szUnit, char cCol, bool bFirst)
{
   if(bFirst == false)
   {
      strcat(szValue, ", ");
   }
   if(cCol != '\0')
   {
      sprintf(szValue, "%s\037%c", szValue, cCol);
   }
   sprintf(szValue, "%s%d", szValue, iValue);
   if(cCol != '\0')
   {
      strcat(szValue, "\0370");
   }
   sprintf(szValue, "%s %s%s", szValue, szUnit, iValue != 1 ? "s" : "");

   return false;
}

void StrValue(char *szValue, int iType, int iValue, char cCol)
{
   bool bFirst = true;
   int iUnit = 0;
   double dUnit = 0.0;
   char *szUnit = "", *szBase = strmk(szValue);

   szValue[0] = '\0';

   /* if(iType == STRVALUE_HM || iType == STRVALUE_HMS)
   {
      if(cCol != '\0')
      {
         sprintf(szValue, "%s\037%c", szValue, cCol);
      }

      sprintf(szValue, "%s%02d:%02d", szValue, iValue / 3600, (iValue / 60) % 60);
      if(iType == STRVALUE_HMS)
      {
         sprintf(szValue, "%s:%02d", szValue, iValue % 60);
      }

      if(cCol != '\0')
      {
         strcat(szValue, "\0370");
      }
   } */
   if(iType == STRVALUE_TIME || iType == STRVALUE_HM || iType == STRVALUE_DAY || iType == STRVALUE_TIMESU)
   {
      if(iValue >= 31536000 && iType != STRVALUE_DAY && iType != STRVALUE_HM)
      {
         iUnit = iValue / 31536000;
         iValue = 0; // iValue % 31536000;
         bFirst = StrValueAdd(szValue, iUnit, "year", cCol, bFirst);
      }
      if(iValue >= 604800 && iType != STRVALUE_DAY && iType != STRVALUE_HM)
      {
         iUnit = iValue / 604800;
         iValue = iValue % 604800;
         bFirst = StrValueAdd(szValue, iUnit, "week", cCol, bFirst);
      }
      if(iValue >= 86400 && iType != STRVALUE_HM && (iType != STRVALUE_TIMESU || bFirst == true))
      {
         iUnit = iValue / 86400;
         iValue = iValue % 86400;
         bFirst = StrValueAdd(szValue, iUnit, "day", cCol, bFirst);
      }
      if(iValue >= 3600 && (iType != STRVALUE_TIMESU || bFirst == true))
      {
         iUnit = iValue / 3600;
         iValue = iValue % 3600;
         bFirst = StrValueAdd(szValue, iUnit, "hour", cCol, bFirst);
      }
      if(iValue >= 60 && (iType != STRVALUE_TIMESU || bFirst == true))
      {
         iUnit = iValue / 60;
         iValue = iValue % 60;
         bFirst = StrValueAdd(szValue, iUnit, "minute", cCol, bFirst);
      }
      if(iValue > 0 && (iType == STRVALUE_TIME || iType == STRVALUE_TIMESU) && (iType != STRVALUE_TIMESU || bFirst == true))
      {
         bFirst = StrValueAdd(szValue, iValue, "second", cCol, bFirst);
      }
   }
   else if(iType == STRVALUE_HMN)
   {
      iUnit = iValue / 3600;
      iValue = iValue % 3600;
      debug("StrValue HMN %d hours\n", iUnit);
      sprintf(szValue, "%02d:", iUnit);

      iUnit = iValue / 60;
      iValue = iValue % 60;
      debug("StrValue HMN %d minutes\n", iUnit);
      sprintf(szValue, "%s%02d", szValue, iUnit);
   }
   else if(iType == STRVALUE_BYTE)
   {
      if(cCol != '\0')
      {
         sprintf(szValue, "%s\037%c", szValue, cCol);
      }

      if(iValue >= 1048576)
      {
         dUnit = iValue / 1048576.0;
         szUnit = "Mb";
      }
      else if(iValue >= 1024)
      {
         dUnit = iValue / 1024.0;
         szUnit = "Kb";
      }
      else
      {
         if(iValue == 1)
         {
            szUnit = "byte";
         }
         else
         {
            szUnit = "bytes";
         }
      }

      if(dUnit > 0)
      {
         sprintf(szValue, "%s%0.1f", szValue, dUnit);
      }
      else
      {
         sprintf(szValue, "%s%d", szValue, iValue);
      }

      if(cCol != '\0')
      {
         strcat(szValue, "\0370");
      }

      sprintf(szValue, "%s %s", szValue, szUnit);
   }

   delete[] szBase;
}

bool IsSmiley(const char *szText)
{
   if(strcmp(szText, ":)") == 0 || strcmp(szText, ":(") == 0)
   {
      return true;
   }

   if(strcmp(szText, ":-)") == 0 || strcmp(szText, ":-(") == 0 || strcmp(szText, ":-/") == 0)
   {
      return true;
   }

   return false;
}

char *UserEmote(const char *szPrefix1, const char *szPrefix2, const char *szText, bool bDot, char cCol)
{
   bool bSet = false;
   char *szReturn = NULL;

   if(szText == NULL || strlen(szText) == 0)
   {
      return strmk(szPrefix1);
   }

   if(strlen(szText) > 2)
   {
      if(szText[0] == ':')
      {
         if(strncmp(szText + 1, "'s ", 3) == 0)
         {
            szReturn = new char[strlen(szPrefix2) + 4 + 2 + 4 + strlen(szText) + 1];
            strcpy(szReturn, szPrefix2);
            if(cCol != '\0')
            {
               sprintf(szReturn, "%s\037%c's\0370", szReturn, cCol);
            }
            else
            {
               strcat(szReturn, "'s");
            }
            strcat(szReturn, szText + 3);

            bSet = true;
         }
         else if(IsSmiley(szText) == false)
         {
            szReturn = new char[strlen(szPrefix2) + 1 + strlen(szText)];
            sprintf(szReturn, "%s %s", szPrefix2, szText + 1);

            bSet = true;
         }
      }
   }

   if(bSet == false)
   {
      szReturn = new char[strlen(szPrefix1) + 1 + strlen(szText) + 2];
      sprintf(szReturn, "%s%s %s", szPrefix1, bDot == true ? "." : "", szText);
   }

   return szReturn;
}

bool CmdRetroNames(EDF *pUser)
{
   int iValue = 0;

   pUser->GetChild("retro", &iValue);
   return mask(iValue, RETRO_NAMES);
}

char *CmdRetroName(char *szName)
{
   int iPos = 0;

   if(szName == NULL)
   {
      return NULL;
   }

   while(szName[iPos] != '\0')
   {
      szName[iPos] = toupper(szName[iPos]);
      iPos++;
   }

   return szName;
}

bool CmdRetroMenus(EDF *pUser)
{
   int iValue = 0;

   pUser->GetChild("retro", &iValue);
   return mask(iValue, RETRO_MENUS);
}

// TrimFields: Places two fields into the space required, truncating if required
void TrimFields(CmdTable *pTable, int iMax, char *szField1, char *szField2, char cColour = '\0')
{
   int iSpace = iMax - (strlen(szField1) + 1 + strlen(szField2));
   char *szField = new char[iMax + 6];

   // debug("TrimFields %d (using %d + %d) -> %d", iMax, strlen(szField1), strlen(szField2), iSpace);

   if(iSpace > 0)
   {
      sprintf(szField, "%s%-*s%s", szField1, iSpace, "", szField2);
      pTable->SetValue(szField, cColour);
      // debug(", f1 %d", strlen(szField));
   }
   else if(strlen(szField2) > iMax || iMax - strlen(szField2) - 2 < 0)
   {
      szField2[iMax - 1] = '\0';
      pTable->SetValue(szField2, cColour);
      // debug(", f2 %d", strlen(szField2));
   }
   else
   {
      szField1[iMax - strlen(szField2) - 2] = '\0';
      sprintf(szField, "%s %s", szField1, szField2);
      pTable->SetValue(szField, cColour);
      // debug(", f3 %d", strlen(szField));
   }
   delete[] szField;

   // debug("\n");
}

bool CmdSystemLogin(EDF *pReply, char *szType)
{
   STACKTRACE
   bool bLoop = false, bFirst = true;
   char *szTitle = NULL, *szSpace = NULL, *szValue = NULL;
   char szWrite[100];

   if(stricmp(szType, "allow") == 0)
   {
      szTitle = "Allowed";
      szSpace = "";
   }
   else if(stricmp(szType, "deny") == 0)
   {
      szTitle = "Denied";
      szSpace = " ";
   }
   else if(stricmp(szType, "trust") == 0)
   {
      szTitle = "Trusted";
      szSpace = "";
   }
   else
   {
      return false;
   }

   if(pReply->Child(szType) == false)
   {
      return false;
   }

   // EDFPrint("CmdSystemLogin login deny section", pReply, false);
   if(pReply->Children("hostname") + pReply->Children("address") == 0)
   {
      sprintf(szWrite, "%s from all\n", szTitle);
      CmdWrite(szWrite);
   }
   else
   {
      sprintf(szWrite, "%s from:%s ", szTitle, szSpace);
      CmdWrite(szWrite);

      bFirst = true;
      bLoop = pReply->Child();
      while(bLoop == true)
      {
         pReply->Get(&szType, &szValue);
         if(stricmp(szType, "hostname") == 0 || stricmp(szType, "address") == 0)
         {
            if(bFirst == false)
            {
               CmdWrite(", ");
            }
            sprintf(szWrite, "\0373%s\0370", szValue);
            CmdWrite(szWrite);
            bFirst = false;
         }

         bLoop = pReply->Next();
         if(bLoop == false)
         {
            pReply->Parent();
         }
      }
      CmdWrite("\n");
   }

   pReply->Parent();

   return true;
}

// Output functions
void CmdSystemView(EDF *pReply)
{
   STACKTRACE
   long lUpLen = 0;
   bool bLoop = false, bFirst = false;
   int iSystemTime = 0, iUptime = 0, iReloadTime = 0, iIdle = 0, iBuildNum = 0, iExpiry = -1;
   int iWritetime = 0, iMainttime = 0, iAccessLevel = LEVEL_NONE, iAttachmentSize = 0;
   int iMaxrss = 0, iIxrss = 0, iIdrss = 0, iIsrss = 0, iRequests = 0, iAnnounces = 0;
   int iMinFlt = 0, iMajFlt = 0, iNSwap = 0;
   int iConnections = 0, iSent = 0, iRecieved = 0, iUserTime = 0;
   int iMax = 0, iCount = 0, iMemUsage = 0;
   char szWrite[200], szValue[100];
   char *szVersion = NULL, *szProtocol = NULL, *szBuildDate = NULL, *szBuildTime = NULL, *szUserTime = NULL, *szSystemTime = NULL, *szName = NULL;

   debug(DEBUGLEVEL_INFO, "CmdSytemView entry\n");

   m_pUser->GetChild("accesslevel", &iAccessLevel);

   pReply->GetChild("name", &szName);
   pReply->GetChild("version", &szVersion);
   pReply->GetChild("protocol", &szProtocol);
   pReply->GetChild("buildnum", &iBuildNum);
   pReply->GetChild("builddate", &szBuildDate);
   pReply->GetChild("buildtime", &szBuildTime);
   pReply->GetChild("systemtime", &iSystemTime);
   pReply->GetChild("uptime", &iUptime);
   pReply->GetChild("idletime", &iIdle);

   sprintf(szWrite, "%s \0373%s\0370 (build \0373%d\0370, \0373%s\0370, \0373%s\0370)\n", szName, szVersion, iBuildNum, szBuildDate, szBuildTime);
   CmdTitle(szWrite);
   if(szProtocol != NULL)
   {
      CmdField("Protocol", '3', szProtocol);
   }
   delete[] szName;
   delete[] szProtocol;
   delete[] szVersion;
   delete[] szBuildDate;
   delete[] szBuildTime;

   lUpLen = iSystemTime - iUptime;
   CmdTime("Server time", '3', STRTIME_TIME, iSystemTime);
   CmdTime("Start time", '3', STRTIME_LONG, iUptime);
   CmdValue("Uptime", '3', STRVALUE_TIME, lUpLen);

   if(iAccessLevel >= LEVEL_WITNESS)
   {
      if(pReply->GetChild("writetime", &iWritetime) == true)
      {
         CmdTime("Last write", '3', STRTIME_LONG, iWritetime);
      }

      if(pReply->GetChild("mainttime", &iMainttime) == true)
      {
         CmdTime("Last maint", '3', STRTIME_LONG, iMainttime);
      }

      if(pReply->GetChild("reloadtime", &iReloadTime, EDFElement::LAST) == true)
      {
         CmdTime("Last reload", '3', STRTIME_LONG, iReloadTime);
      }

      // strcpy(szWrite, "Transfers:    ");
      CmdWrite("Transfers: ");

      pReply->GetChild("sent", &iSent);
      // StrValue(szWrite, STRVALUE_BYTE, iSent, '3');
      // strcat(szWrite, " sent, ");
      StrValue(szValue, STRVALUE_BYTE, iSent, '3');
      sprintf(szWrite, "%s sent", szValue);
      CmdWrite(szWrite);

      pReply->GetChild("received", &iRecieved);
      // StrValue(szWrite, STRVALUE_BYTE, iRecieved, '3');
      // strcat(szWrite, " recieved");
      StrValue(szValue, STRVALUE_BYTE, iRecieved, '3');
      sprintf(szWrite, ", %s recieved\n", szValue);
      CmdWrite(szWrite);

      pReply->GetChild("requests", &iRequests);
      pReply->GetChild("announces", &iAnnounces);
      sprintf(szWrite, "           \0373%d\0370 request%s, \0373%d\0370 announcement%s\n\n", iRequests, iRequests != 1 ? "s" : "", iAnnounces, iAnnounces == 1 ? "" : "s");
      CmdWrite(szWrite);

      pReply->GetChild("usercputime", &szUserTime);
      pReply->GetChild("syscputime", &szSystemTime);
      if(szUserTime != NULL && szSystemTime != NULL)
      {
         iUserTime = atoi(szUserTime);
         sprintf(szWrite, "CPU: \0373%d:%02d:%02d\0370 user/\0373%s\0370s system CPU time\n", iUserTime / 3600, (iUserTime / 60) % 60, iUserTime % 60, szSystemTime);
         CmdWrite(szWrite);

         delete[] szUserTime;
         delete[] szSystemTime;
      }

      pReply->GetChild("maxrss", &iMaxrss);
      pReply->GetChild("ixrss", &iIxrss);
      pReply->GetChild("idrss", &iIdrss);
      pReply->GetChild("isrss", &iIsrss);
      if(iMaxrss > 0 || iIxrss > 0 || iIdrss > 0 || iIsrss > 0)
      {
         sprintf(szWrite, "Resources: \0373%d\0370 Max/\0373%d\0370 Ix/\0373%d\0370 Id/\0373%d\0370 Is\n", iMaxrss, iIxrss, iIdrss, iIsrss);
         CmdWrite(szWrite);
      }

      pReply->GetChild("iminflt", &iMinFlt);
      pReply->GetChild("imajflt", &iMajFlt);
      pReply->GetChild("inswap", &iNSwap);
      if(iMinFlt > 0 || iMajFlt > 0 || iNSwap > 0)
      {
         sprintf(szWrite, "Paging: \0373%d\0370 minor faults/\0373%d\0370 major faults/\0373%d\0370 in swap\n", iMinFlt, iMajFlt, iNSwap);
         CmdWrite(szWrite);
      }

      CmdWrite("\n");

      pReply->GetChild("memusage", &iMemUsage);
      if(iMemUsage > 0)
      {
         CmdValue("Memory usage", '3', STRVALUE_BYTE, iMemUsage);
      }

      pReply->GetChild("connections", &iConnections);
      sprintf(szWrite, "Connections:  \0373%d\0370\n", iConnections);
      CmdWrite(szWrite);
      if(pReply->Child(CmdVersion("2.5") >= 0 ? "connection" : "login") == true)
      {
         // CmdEDFPrint("CmdSystemView login section", pReply, false);

         CmdSystemLogin(pReply, "deny");
         CmdSystemLogin(pReply, "allow");
         CmdSystemLogin(pReply, "trust");
         CmdWrite("\n");

         pReply->Parent();
      }

      if(iIdle > 0)
      {
         // sprintf(szWrite, "Idle time:    \0373%02d:%02d:%02d\0370\n\n", iIdle / 3600, (iIdle / 60) % 60, iIdle % 60);
         CmdValue("Idle time", '3', STRVALUE_TIME, iIdle);
      }
      else
      {
         // sprintf(szWrite, "No idle time\n\n");
         CmdWrite("No idle time\n");
      }
      // CmdWrite(szWrite);
      CmdWrite("\n");

      iCount = 0;
      pReply->GetChild("numfolders", &iCount);
      pReply->GetChild("maxfoldermsgid", &iMax);
      sprintf(szWrite, "Folders:      \0373%d\0370 (max message ID \0373%d\0370)\n", iCount, iMax);
      CmdWrite(szWrite);
      if(pReply->Child("defaultsubs") == true)
      {
         pReply->Sort("folder", "name");

         bFirst = true;
         bLoop = pReply->Child("folder");
         while(bLoop == true)
         {
            if(bFirst == true)
            {
               CmdWrite("Default subs: ");
               bFirst = false;
            }
            else
            {
               CmdWrite(", ");
            }

            pReply->GetChild("name", &szName);
            sprintf(szWrite, "\0373%s\0370", szName);
            CmdWrite(szWrite);
            delete[] szName;

            bLoop = pReply->Next("folder");
            if(bLoop == false)
            {
               pReply->Parent();
               CmdWrite("\n");
            }
         }
         pReply->Parent();
      }
      if(CmdVersion("2.5") >= 0)
      {
         pReply->GetChild("attachmentsize", &iAttachmentSize);
         if(iAttachmentSize == 0)
         {
            CmdWrite("              No attachments allowed\n");
         }
         else if(iAttachmentSize == -1)
         {
            CmdWrite("              Any size attachment allowed\n");
         }
         else
         {
            CmdWrite("              Maximum attachment allowed ");
            StrValue(szWrite, STRVALUE_BYTE, iAttachmentSize, '3');
            strcat(szWrite, "\n");
            CmdWrite(szWrite);
         }
         // CmdWrite("\n");
         pReply->GetChild("expire", &iExpiry);
         if(iExpiry == -1)
         {
            CmdWrite("              No expiry time\n");
         }
         else if(iExpiry > 0)
         {
            /* strcpy(szWrite, "              Expiry time ");
            StrValue(szWrite, STRVALUE_TIMESU, iExpiry, '3');
            strcat(szWrite, "\n");
            CmdWrite(szWrite); */
            CmdValue("Expiry time", '3', STRVALUE_TIMESU, iExpiry);
         }
      }
      CmdWrite("\n");

      iCount = 0;
      pReply->GetChild("numusers", &iCount);
      pReply->GetChild("maxuserid", &iMax);
      sprintf(szWrite, "Users: \0373%d\0370 (max ID \0373%d\0370)\n", iCount, iMax);
      CmdWrite(szWrite);

      if(CmdVersion("2.5") < 0)
      {
         CmdWrite("\n");

         iCount = 0;
         pReply->GetChild("numchannels", &iCount);
         pReply->GetChild("maxchannelmsgid", &iMax);
         sprintf(szWrite, "Channels: \0373%d\0370 (max message ID \0373%d\0370)\n", iCount, iMax);
         CmdWrite(szWrite);
      }

      if(CmdVersion("2.5") >= 0)
      {
         iCount = 0;
         pReply->GetChild("numtasks", &iCount);
         if(iCount > 0)
         {
            // sprintf(szWrite, "Tasks: \0373%d\0370\n", iCount);
            // CmdWrite(szWrite);
            CmdField("Tasks", '3', iCount);
         }
      }
   }

   CmdWrite("\n");

   debug(DEBUGLEVEL_INFO, "CmdSytemView exit\n");
}

#define LOCLIST_COL 48

void CmdLocationList(EDF *pReply, int iListType)
{
   STACKTRACE
   bool bLoop = true, bFirst = true; // , bPostHeader = false;
   int iNumLocations = 0, iLocationID = 0, iUses = 0, iLastUse = 0;
   char *szName = NULL, *szType = NULL, *szValue = NULL;
   char szDepth[100], szTime[32];
   // struct tm *tmTime = NULL;
   CmdTable *pTable = NULL;

   if(iListType == 1)
   {
      pReply->Sort("location", "name", true);
   }

   // printf("CmdLocationList entry %d\n", iListType);

   pTable = new CmdTable(m_pUser, 1, 4, false);
   pTable->AddHeader("ID", 5, '\0', CMDTABLE_RIGHT, 1);
   pTable->AddHeader("Location / Match", LOCLIST_COL);
   pTable->AddHeader("Uses", 6, '\0', CMDTABLE_RIGHT, 1);
   pTable->AddHeader("Last Use", 10);

   // printf("CmdLocationList post header\n");

   pReply->GetChild("numlocations", &iNumLocations);
   bLoop = pReply->Child("location");
   while(bLoop == true)
   {
      szName = NULL;
      iUses = 0;
      iLastUse = 0;

      pReply->Get(NULL, &iLocationID);
      pReply->GetChild("name", &szName);
      pReply->GetChild("totaluses", &iUses);
      pReply->GetChild("lastuse", &iLastUse);
      if(szName == NULL)
      {
         szName = strmk("");
      }

      bFirst = true;
      bLoop = pReply->Child();
      while(bLoop == true)
      {
         szValue = NULL;

         pReply->Get(&szType, &szValue);
         if((stricmp(szType, "hostname") == 0 || stricmp(szType, "address") == 0) && szValue != NULL)
         {
            // printf("flag time\n");
            /* if(bPostHeader == false)
            {
               printf("post header\n");
               bPostHeader = true;
            } */
            pTable->SetFlag(stricmp(szType, "hostname") == 0 ? 'H' : 'A');
            if(bFirst == true)
            {
               pTable->SetValue(iLocationID);
               sprintf(szDepth, "%-*s%s", pReply->Depth() - 2, "", szName);
               TrimFields(pTable, LOCLIST_COL, szDepth, szValue, '3');

               if(iUses > 0)
               {
                  pTable->SetValue(iUses);
                  if(iLastUse != 0)
                  {
                     // tmTime = localtime((time_t *)&iLastUse);
                     // strftime(szTime, 32, "%d/%m/%Y", tmTime);
                     StrTime(szTime, STRTIME_DATE, iLastUse);
                     pTable->SetValue(szTime);
                  }
               }

               bFirst = false;
            }
            else
            {
               pTable->SetValue("");

               sprintf(szDepth, "%-*s", LOCLIST_COL - strlen(szValue), "");
               TrimFields(pTable, LOCLIST_COL, szDepth, szValue, '3');
               // pTable->SetValue(szValue, '3');
            }
         }
         delete[] szType;
         delete[] szValue;

         bLoop = pReply->Next();
         if(bLoop == false)
         {
            pReply->Parent();
         }
      }
      if(bFirst == true)
      {
         pTable->SetFlag(' ');
         pTable->SetValue(iLocationID);
         sprintf(szDepth, "%-*s%s", pReply->Depth() - 1, "", szName);
         pTable->SetValue(szDepth, '3');

         if(iUses > 0)
         {
            pTable->SetValue(iUses);
            if(iLastUse != 0)
            {
               // tmTime = localtime((time_t *)&iLastUse);
               // strftime(szTime, 32, "%d/%m/%Y", tmTime);
               StrTime(szTime, STRTIME_DATE, iLastUse);
               pTable->SetValue(szTime);
            }
         }

         bFirst = false;
      }

      delete[] szName;

      bLoop = pReply->Iterate("location");
   }

   pTable->AddFooter(iNumLocations, "location");
   pTable->AddKey('H', "Hostname");
   pTable->AddKey('A', "Address");

   delete pTable;

   // printf("CmdLocationList exit\n");
}

void CmdHelpList(EDF *pReply)
{
   STACKTRACE
   int iHelpID = 0, iNumTopics = 0;
   bool bLoop = false;
   char *szTopic = NULL;
   CmdTable *pTable = NULL;

   debug(DEBUGLEVEL_INFO, "CmdHelpList entry\n");

   pTable = new CmdTable(m_pUser, 0, 2, false);
   pTable->AddHeader("ID", 4, '\0', CMDTABLE_RIGHT, 1);
   pTable->AddHeader("Topic", CMDTABLE_REMAINDER, '3');

   bLoop = pReply->Child("help");
   while(bLoop == true)
   {
      szTopic = NULL;

      pReply->Get(NULL, &iHelpID);
      pReply->GetChild("subject", &szTopic);
      debug(DEBUGLEVEL_DEBUG, "CmdHelpList %d %s\n", iHelpID, szTopic);

      pTable->SetValue(iHelpID);
      pTable->SetValue(szTopic);

      delete[] szTopic;

      iNumTopics++;

      bLoop = pReply->Next("help");
   }

   pTable->AddFooter(iNumTopics, "topic");

   delete pTable;
}

void CmdHelpView(EDF *pReply)
{
   STACKTRACE
   int iHelpID = 0, iDate = 0, iFromID = 0, iCurrID = 0;
   char szWrite[200];
   char *szFrom = NULL, *szTopic = NULL, *szText = NULL;
   // struct tm *tmDate = NULL;

   debug(DEBUGLEVEL_INFO, "CmdHelpView entry\n");
   // EDFPrint(pReply);

   m_pUser->Get(NULL, &iCurrID);

   CmdPageOn();

   pReply->Get(NULL, &iHelpID);
   sprintf(szWrite, "Help \0377%d\0370", iHelpID);
   CmdTitle(szWrite);

   if(pReply->GetChild("date", &iDate) == true)
   {
      // tmDate = localtime((time_t*)&iDate);
      // strftime(szWrite, sizeof(szWrite), "Date: \0372%A, %d %B %Y - %H:%M\0370\n", tmDate);
      StrTime(szWrite, STRTIME_LONG, iDate, '3', "Date: ", "\n");
      CmdWrite(szWrite);
   }

   pReply->GetChild("fromid", &iFromID);
   if(pReply->GetChild("fromname", &szFrom) == true)
   {
      // sprintf(szWrite, "From: \037%c%s\0370\n", iFromID == iCurrID ? '1' : '3', szFrom);
      CmdField("From", iFromID == iCurrID ? '1' : '3', szFrom);
      delete[] szFrom;
   }

   pReply->GetChild("subject", &szTopic);
   // sprintf(szWrite, "Topic: \0373%s\0370\n\n", szTopic);
   // CmdWrite(szWrite);
   CmdField("Topic", '3', szTopic);

   pReply->GetChild("text", &szText);
   CmdWrite(szText, CMD_OUT_NOHIGHLIGHT | CMD_OUT_UTF);

   CmdWrite("\n\n");

   delete[] szTopic;
   delete[] szText;

   CmdPageOff();

   debug(DEBUGLEVEL_INFO, "CmdHelpView exit\n");
}

/* CmdFolderList: List of folders
**
** listtype: Display option
** 0 - All folders
** 1 - Unsubscribed
** 2 - Subscribed
** 3 - Unread
**
** listdetails: Details option
** 0 - None
** 1 - Summary
** 2 - Editor
** 3 - Creation time
** 4 - Expiry
** 5 - Sort by size
** 6 - Access
** 7 - priority
*/
void CmdMessageTreeList(EDF *pReply, const char *szType, int iListType, int iListDetails)
{
   STACKTRACE
   int iAccessLevel = LEVEL_NONE, iNumTrees = 0, iSubscribed = 0, iReadOnly = 0, iRestricted = 0, iSubType = 0;
   int iTotalMsgs = 0, iTotalSize = 0, iTotalUnread = 0, iSearchType = 0, iFound = 0, iExpire = -1, iTreeID = 0;
   int iCurrUser = 0, iEditorID = 0, iNumCols = 0, iPriority = 0;
   bool bRetro = false, bDevOption = false, bLoop = true, bDeleted = false;
   char cSubType = '\0';
   char szWrite[100], szDepth[100];
   char szNumTypes[32];
   char *szName = NULL, *szEditor = NULL;
   int iNumUnread = 0, iDepth = 0, iNumMsgs = 0, iCreated = 0, iSize = 0, iTreeMode = 0, iTreeLevel = LEVEL_NONE, iNumFlags = 4;
   // struct tm *tmDate = NULL;

   sprintf(szNumTypes, "num%ss", szType);

   // CmdEDFPrint("CmdMessageTreeList user", pUser);
   m_pUser->Get(NULL, &iCurrUser);
   m_pUser->GetChild("accesslevel", &iAccessLevel);

   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      bDevOption = m_pUser->GetChildBool("devoption");
      bRetro = CmdRetroNames(m_pUser);
   }

   pReply->GetChild(szNumTypes, &iNumTrees);
   pReply->GetChild("searchtype", &iSearchType);

   debug(DEBUGLEVEL_INFO, "CmdMessageTreeList entry %d, %d, %d, %d\n", iListType, iNumTrees, iListDetails, iCurrUser);
   // debugEDFPrint(pReply);

   if(iNumTrees == 0)
   {
      sprintf(szWrite, "No %ss to list\n", szType);
      CmdWrite(szWrite);
      return;
   }

   if(iListDetails != 7)
   {
      pReply->Sort(szType, "name", true);
   }

   if(iListDetails == 5 || iListDetails == 7)
   {
      CmdPageOn();
      sprintf(szWrite, "Checking %ss...\n", szType);
      CmdWrite(szWrite);

      bLoop = pReply->Child(szType);
      while(bLoop == true)
      {
         if(pReply->GetChildBool("deleted") == false)
         {
            iSubType = 0;
            iNumUnread = 0;
            iPriority = 0;

            pReply->GetChild("name", &szName);
            pReply->GetChild("unread", &iNumUnread);
            pReply->GetChild("subtype", &iSubType);
            pReply->GetChild("priority", &iPriority);
            if(iListDetails == 5 && iSubType > 0 && iNumUnread > 0)
            {
               sprintf(szWrite, "\0373%s\0370: \0373%d\0370 new message%s\n", RETRO_NAME(szName), iNumUnread, iNumUnread != 1 ? "s" : "");
               CmdWrite(szWrite);
            }
            else if(iListDetails == 7 && iSubType > 0)
            {
               sprintf(szWrite, "\0373%s\0370: \0373%s\0370", RETRO_NAME(szName), SubTypeStr(iSubType));
               if(iPriority > 0)
               {
                  sprintf(szWrite, "%s priority \0373%d\0370", szWrite, iPriority);
               }
               strcat(szWrite, "\n");
               CmdWrite(szWrite);
            }

            delete[] szName;
         }

         bLoop = pReply->Iterate(szType);
      }

      CmdPageOff();
   }
   else
   {
      CmdTable *pTable = NULL;

      // debug("CmdMessageTreeList header\n");
      if(iListType == 0)
      {
         iNumFlags--;
      }
      if(iListDetails > 0)
      {
         if(iAccessLevel >= LEVEL_WITNESS)
         {
            iNumCols = 4;
            if(iListDetails == 1 || iListDetails == 6)
            {
               iNumCols++;
            }
            pTable = new CmdTable(m_pUser, iListDetails == 6 ? 1 : 4, iNumCols, false);
         }
         else
         {
            iNumCols = 3;
            if(bDevOption == true)
            {
               iNumCols++;
            }
            if(iListDetails == 6)
            {
               iNumCols++;
            }
            pTable = new CmdTable(m_pUser, iListDetails == 6 ? 1 : 4, iNumCols, false);
         }
      }
      else
      {
         pTable = new CmdTable(m_pUser, 4, 2, false);
      }
      if(iListDetails > 0 && (iAccessLevel >= LEVEL_WITNESS || bDevOption == true))
      {
         pTable->AddHeader("ID", 8);
      }
      pTable->AddHeader("Name", 30, '\0');
      if(iListDetails > 0)
      {
         if(iListDetails == 1)
         {
            /* if(iAccessLevel >= LEVEL_WITNESS)
            {
               pTable->AddHeader("Access", 11, '3');
            } */
            pTable->AddHeader("Msgs", 8, '\0', CMDTABLE_RIGHT);
            if(iAccessLevel >= LEVEL_WITNESS)
            {
               pTable->AddHeader("Size", 8, '\0', CMDTABLE_RIGHT);
            }
         }
         else if(iListDetails == 2)
         {
            pTable->AddHeader("Editor", UA_NAME_LEN, '3');
         }
         else if(iListDetails == 3)
         {
            pTable->AddHeader("Creation", 18, '\0');
         }
         else if(iListDetails == 4)
         {
            pTable->AddHeader("Expiry", 8, '\0', CMDTABLE_RIGHT);
         }
         else if(iListDetails == 6)
         {
            pTable->AddHeader("Mode", 11, '\0');
            pTable->AddHeader("Level", 10, '\0');
         }
      }
      pTable->AddHeader("Unread", 8, '\0', CMDTABLE_RIGHT);
      // pTable->Flush();

      // debug("CmdMessageTreeList content %d\n", iNumTrees);
      pReply->Root();
      bLoop = pReply->Child(szType);
      while(bLoop == true)
      {
         // debugEDFPrint("CmdMessageTreeList", pReply, false);

         if(pReply->GetChildBool("deleted") == false)
         {
            iNumUnread = 0;
            iSubType = 0;
            pReply->GetChild("unread", &iNumUnread);
            pReply->GetChild("subtype", &iSubType);
            if(iListType == 0 ||
               (iListType == 1 && iSubType == 0) ||
               (iListType == 2 && iSubType > 0) ||
               (iListType == 3 && iNumUnread > 0))
            {
               iFound++;
               iExpire = -1;
               iTreeLevel = LEVEL_NONE;

               pReply->Get(NULL, &iTreeID);
               pReply->GetChild("name", &szName);
               pReply->GetChild("accessmode", &iTreeMode);
               pReply->GetChild("accesslevel", &iTreeLevel);
               pReply->GetChild("nummsgs", &iNumMsgs);
               pReply->GetChild("totalmsgs", &iSize);
               pReply->GetChild("expire", &iExpire);
               if(CmdVersion("2.5") >= 0 && iExpire > 0)
               {
                  iExpire /= 86400;
               }
               pReply->GetChild("created", &iCreated);
               if(iListType == 0)
               {
                  iDepth = pReply->Depth() - 1;
               }
               // debug("CmdMessageTreeList %d %d %d\n", iAccessMode, iNumMsgs, iSize);

               if(iSubType > 0)
               {
                  iSubscribed++;
               }
               if(mask(iTreeMode, ACCMODE_SUB_WRITE) == false && mask(iTreeMode, ACCMODE_MEM_READ) == false)
               {
                  iReadOnly++;
               }
               if(mask(iTreeMode, ACCMODE_MEM_READ) == true)
               {
                  iRestricted++;
               }

               if(iListType == 0)
               {
                  if(iSubType == SUBTYPE_EDITOR)
                  {
                     cSubType = 'E';
                  }
                  else if(iSubType == SUBTYPE_MEMBER)
                  {
                     cSubType = 'M';
                  }
                  else if(iSubType == SUBTYPE_SUB)
                  {
                     cSubType = 'S';
                  }
                  else
                  {
                     cSubType = ' ';
                  }
                  pTable->SetFlag(cSubType);
               }
               if(iListDetails != 6)
               {
                  pTable->SetFlag((mask(iTreeMode, ACCMODE_SUB_WRITE) == false && mask(iTreeMode, ACCMODE_MEM_READ) == false) ? 'R' : ' ');
                  pTable->SetFlag(mask(iTreeMode, ACCMODE_MEM_READ) == true ? 'T' : ' ');
                  pTable->SetFlag(mask(iTreeMode, ACCMODE_PRIVATE) == true ? 'P' : ' ');
               }

               if(iListDetails > 0 && (iAccessLevel >= LEVEL_WITNESS || bDevOption == true))
               {
                  pTable->SetValue(iTreeID);
               }
               if(iListType == 0)
               {
                  memset(szDepth, ' ', pReply->Depth() - 1);
                  strcpy(szDepth + pReply->Depth() - 1, RETRO_NAME(szName));
               }
               else
               {
                  strcpy(szDepth, RETRO_NAME(szName));
               }
               pTable->SetValue(szDepth, iSubType == SUBTYPE_EDITOR ? '1' : '3');
               if(iListDetails == 1)
               {
                  /* if(iAccessLevel >= LEVEL_WITNESS)
                  {
                     CmdMessageTreeAccess(szWrite, iAccessMode);
                     pTable->SetValue(szWrite);
                  } */
                  pTable->SetValue(iNumMsgs);
                  iTotalMsgs += iNumMsgs;
                  if(iAccessLevel >= LEVEL_WITNESS)
                  {
                     pTable->SetValue(iSize);
                     iTotalSize += iSize;
                  }
               }
               else if(iListDetails == 2)
               {
                  if(pReply->Child("editor") == true)
                  {
                     iEditorID = 0;

                     pReply->Get(NULL, &iEditorID);
                     pReply->GetChild("name", &szEditor);
                     debug(DEBUGLEVEL_DEBUG, "CmdMessageTreeList editor %d %s\n", iEditorID, szEditor);
                     pTable->SetValue(RETRO_NAME(szEditor), iEditorID == iCurrUser ? '1' : '3');
                     // debug("CmdMessageTreeList %s: %s\n", szName, szEditor);
                     delete[] szEditor;
                     pReply->Parent();
                  }
                  else
                  {
                     pTable->SetValue("");
                  }
               }
               else if(iListDetails == 3)
               {
                  // tmDate = localtime((time_t *)&iCreated);
                  // strftime(szWrite, sizeof(szWrite), "%d/%m/%y %H:%M", tmDate);
                  StrTime(szWrite, STRTIME_SHORT, iCreated);
                  pTable->SetValue(szWrite);
               }
               else if(iListDetails == 4)
               {
                  if(iExpire == 0)
                  {
                     pTable->SetValue("-");
                  }
                  else if(iExpire == -1)
                  {
                     pTable->SetValue("");
                  }
                  else
                  {
                     pTable->SetValue(iExpire);
                  }
               }
               else if(iListDetails == 6)
               {
                  CmdMessageTreeAccess(szWrite, iTreeMode);
                  pTable->SetValue(szWrite);
                  if(iTreeLevel != LEVEL_NONE)
                  {
                     pTable->SetValue(AccessName(iTreeLevel));
                  }
                  else
                  {
                     pTable->SetValue("");
                  }
               }

               if(iNumUnread > 0)
               {
                  pTable->SetValue(iNumUnread);
                  iTotalUnread += iNumUnread;
               }
               pTable->Flush();

               if(iListDetails == 2 && pReply->Child("editor", 1) == true)
               {
                  do
                  {
                     pReply->Get(NULL, &iEditorID);
                     pReply->GetChild("name", &szEditor);
                     if(iAccessLevel >= LEVEL_WITNESS || bDevOption == true)
                     {
                        pTable->SetValue("");
                     }
                     pTable->SetValue("");
                     // pTable->SetValue(szEditor);
                     pTable->SetValue(RETRO_NAME(szEditor), iEditorID == iCurrUser ? '1' : '3');
                     pTable->Flush();
                     // debug("CmdMessageTreeList %s: %s\n", szName, szEditor);
                     delete[] szEditor;
                  }
                  while(pReply->Next("editor") == true);
                  pReply->Parent();
               }

               delete[] szName;
            }
         }

         bLoop = pReply->Iterate(szType);
      }

      pTable->Flush();

      // debug("CmdMessageTreeList summary\n");

      pTable->SetValue("Total", '0');
      if(iListDetails > 0 && (iAccessLevel >= LEVEL_WITNESS || bDevOption == true))
      {
         pTable->SetValue("");
      }
      if(iListDetails == 1)
      {
         /* if(iAccessLevel >= LEVEL_WITNESS)
         {
            pTable->SetValue("");
         } */
         pTable->SetValue(iTotalMsgs);
         if(iAccessLevel >= LEVEL_WITNESS)
         {
            pTable->SetValue(iTotalSize);
         }
      }
      else if(iListDetails != 0)
      {
         pTable->SetValue("");
      }
      if(iTotalUnread > 0)
      {
         pTable->SetValue(iTotalUnread);
      }
      // pTable->Flush();

      // debug("CmdMessageTreeList footer\n");
      pTable->AddFooter(iFound, szType);
      if(iListType != 2)
      {
         pTable->AddFooter(iSubscribed, "subscribed");
      }
      pTable->AddKey('S', "Subscriber");
      pTable->AddKey('M', "Member");
      pTable->AddKey('E', "Editor");
      if(iListDetails != 6)
      {
         pTable->AddKey('R', "Read-only");
         pTable->AddKey('T', "resTricted");
         pTable->AddKey('P', "Private");
      }

      delete pTable;
   }

   debug(DEBUGLEVEL_INFO, "CmdMessageTreeList exit\n");
}

void CmdMessageTreeView(EDF *pReply, const char *szType)
{
   STACKTRACE
   int iTreeID = 0, iNumMsgs = 0, iSize = 0, iValue = 0, iAccessLevel = LEVEL_NONE, iSubs = 0;
   int iTreeMode = FOLDERMODE_NORMAL, iTreeLevel = -1, iLastEdit = -1, iCreatorID = 0, iCurrID = 0, iActive = 0;
   bool bRetro = false, bDevOption = false, bPath = false;
   char szWrite[100], szTime[100];
   char szTypeName[32], szTypeID[32];
   char *szTreeName = NULL, *szTreePath = NULL, *szValue = NULL, *szReplyName = NULL, *szCreator = NULL, *szUserName = NULL;
   // struct tm *tmDate = NULL;

   sprintf(szTypeName, "%sname", szType);
   sprintf(szTypeID, "%sid", szType);

   if(strcmp(szType, "channel") == 0)
   {
      iActive = SUBINFO_ACTIVE;
   }

   CmdPageOn();

   m_pUser->Root();
   m_pUser->Get(NULL, &iCurrID);
   m_pUser->GetChild("accesslevel", &iAccessLevel);
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      bDevOption = m_pUser->GetChildBool("devoption");
      bRetro = CmdRetroNames(m_pUser);
      m_pUser->Parent();
   }

   debug(DEBUGLEVEL_INFO, "CmdMessageTreeView entry %d\n", iAccessLevel);
   // EDFPrint(pReply);

   pReply->Get(NULL, &iTreeID);
   debug(DEBUGLEVEL_DEBUG, "CmdMessageTreeView ID %d\n", iTreeID);

   debug(DEBUGLEVEL_DEBUG, "CmdMessageTreeView content\n");
   pReply->GetChild("name", &szTreeName);
   sprintf(szWrite, "Name:    \0373%s\0370\n", RETRO_NAME(szTreeName));
   if(iAccessLevel >= LEVEL_WITNESS || bDevOption == true)
   {
      sprintf(szWrite, "%sID:      \0373%d\0370\n", szWrite, iTreeID);
   }
   CmdWrite(szWrite);
   strcpy(szWrite, "Path:    \0373");
   bPath = pReply->Child("pathid");
   while(bPath == true)
   {
      szTreePath = NULL;
      pReply->GetChild(szTypeName, &szTreePath);
      strcat(szWrite, "\0370/\0373");
      strcat(szWrite, RETRO_NAME(szTreePath));

      delete[] szTreePath;

      bPath = pReply->Next("pathid");
      if(bPath == false)
      {
         strcat(szWrite, "\0370\n");
         CmdWrite(szWrite);

         pReply->Parent();
      }
   }

   pReply->GetChild("accessmode", &iTreeMode);
   pReply->GetChild("accesslevel", &iTreeLevel);

   // if(iTreeMode != Tree_NORMAL || iTreeLevel != -1)
   {
      // memset(szWrite, sizeof(szWrite), '\0');
      strcpy(szWrite, "Access:  \0373");
      CmdMessageTreeAccess(szWrite + 11, iTreeMode);
      strcat(szWrite, "\0370");

      if(iTreeLevel != -1)
      {
         sprintf(szWrite, "%s (level \0373%s\0370)", szWrite, AccessName(iTreeLevel));
      }
      strcat(szWrite, "\n");
      CmdWrite(szWrite);
   }

   STACKTRACEUPDATE

   iSubs = CmdSubList(pReply, SUBTYPE_EDITOR, SUBINFO_TREE, " ", bRetro);

   if(pReply->Child("replyid") == true)
   {
      pReply->GetChild(szTypeName, &szReplyName);
      // sprintf(szWrite, "Replies: \0373%s\0370\n", RETRO_NAME(szReplyName));
      // CmdWrite(szWrite);
      CmdField("Replies", '3', RETRO_NAME(szReplyName));

      delete[] szReplyName;

      pReply->Parent();
   }

   if(pReply->Child("info") == true)
   {
      debug(DEBUGLEVEL_INFO, "CmdMessageTreeView info entry\n");

      pReply->GetChild("date", &iValue);
      // tmDate = localtime((time_t *)&iValue);
      // strftime(szWrite, sizeof(szWrite), "\nDate: \0373%A, %d %B %Y - %H:%M\0370\n", tmDate);
      StrTime(szWrite, STRTIME_LONG, iValue, '3', "\nDate: ", "\n");
      CmdWrite(szWrite);

      szValue = NULL;
      if(pReply->GetChild("fromname", &szValue) == true)
      {
         // sprintf(szWrite, "From: \0373%s\0370\n", RETRO_NAME(szValue));
         // CmdWrite(szWrite);
         CmdField("From", '3', RETRO_NAME(szValue));
         delete[] szValue;
      }

      szValue = NULL;
      if(pReply->GetChild("text", &szValue) == true)
      {
         CmdWrite("\n");
         CmdWrite(szValue, CMD_OUT_NOHIGHLIGHT | CMD_OUT_UTF);
         CmdWrite("\n\n");
         delete[] szValue;
      }
      pReply->Parent();

      debug(DEBUGLEVEL_INFO, "CmdMessageTreeView info exit\n");
   }
   else
   {
      CmdWrite("\n");
   }

   STACKTRACEUPDATE

   pReply->GetChild("nummsgs", &iNumMsgs);
   pReply->GetChild("totalmsgs", &iSize);
   if(iAccessLevel >= LEVEL_WITNESS || iSize > 0)
   {
      if(pReply->GetChild("lastmsg", &iValue) == true && iValue != -1)
      {
         StrTime(szTime, STRTIME_SHORT, iValue, '3', ", last at ");
      }
      else
      {
         strcpy(szTime, "");
      }
      sprintf(szWrite, "Messages:    \0373%d\0370 (\0373%d\0370 bytes%s)\n", iNumMsgs, iSize, szTime);
      CmdWrite(szWrite);

      pReply->GetChild("created", &iValue);
      pReply->GetChild("creatorid", &iCreatorID);
      pReply->GetChild("creatorname", &szCreator);
      // tmDate = localtime((time_t *)&iValue);
      // strftime(szWrite, 64, "Created:     \0373%A, %d %B, %Y - %H:%M\0370", tmDate);
      if(iValue != -1)
      {
         StrTime(szWrite, STRTIME_LONG, iValue, '3', "Created:     ");
         if(szCreator != NULL)
         {
            sprintf(szWrite, "%s by \037%c%s\0370", szWrite, iCreatorID == iCurrID ? '1' : '3', RETRO_NAME(szCreator));
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);
      }

      iValue = -1;
      pReply->GetChild("expire", &iValue);
      strcpy(szWrite, "Expiry time: ");
      if(iValue > 0)
      {
         if(CmdVersion("2.5") < 0)
         {
            iValue *= 86400;
         }
         StrValue(szTime, iValue > 172200 ? STRVALUE_DAY : STRVALUE_HM, iValue, '3');
         strcat(szWrite, szTime);
      }
      else if(iValue == 0)
      {
         strcat(szWrite, "System default");
      }
      else
      {
         strcat(szWrite, "None");
      }
      strcat(szWrite, "\n");
      CmdWrite(szWrite);

      if(pReply->GetChild("maxreplies", &iValue) == true)
      {
         // sprintf(szWrite, "Max replies: \0373%d\0370\n", iValue);
         // CmdWrite(szWrite);
         CmdField("Max replies", '3', iValue);
      }
      if(pReply->Child("lastedit") == true)
      {
         szUserName = NULL;

         pReply->Get(NULL, &iLastEdit);
         pReply->GetChild("username", &szUserName);

         if(iLastEdit != -1 && szUserName != NULL)
         {
            StrTime(szTime, STRTIME_LONG, iLastEdit, '3');
            sprintf(szWrite, "Last edit:   %s by \0373%s\0370\n", szTime, RETRO_NAME(szUserName));
            CmdWrite(szWrite);

            delete[] szUserName;
         }

         pReply->Parent();
      }
   }
   else
   {
      // sprintf(szWrite, "Messages: \0373%d\0370\n", iNumMsgs);
      // CmdWrite(szWrite);
      CmdField("Messages", '3', iNumMsgs);
   }
   CmdWrite("\n");

   STACKTRACEUPDATE

   iSubs = CmdSubList(pReply, SUBTYPE_MEMBER, SUBINFO_TREE | iActive, NULL, bRetro);
   if(mask(iTreeMode, ACCMODE_SUB_READ) == false || stricmp(szType, "channel") == 0)
   {
      iSubs += CmdSubList(pReply, SUBTYPE_SUB, SUBINFO_TREE | iActive, NULL, bRetro);
   }
   else
   {
      iSubs += CmdSubList(pReply, SUBTYPE_SUB, SUBINFO_TREE | SUBINFO_TOTAL | iActive, NULL, bRetro);
   }
   if(iSubs > 0)
   {
      CmdWrite("\n");
   }
   CmdPageOff();

   delete[] szTreeName;
   delete[] szCreator;

   debug(DEBUGLEVEL_INFO, "CmdMessageTreeView exit\n");
}

/* CmdMessageList: List messages in a folder
**
** iListType: Type of list to produce
** 0 - Tree hierarchy
** 1 - Flat list
** 2 - Top messages only
** 3 - Unread messages
** 4 - Vote messages
*/
void CmdMessageList(EDF *pReply, int iListType)
{
   STACKTRACE
   int iMessageID = 0, iNumMsgs = 0, iType = 0, iFound = 0, iUnread = 0, iReplies = 0, iMsgType = 0, iCurrID = -1, iFromID = -1, iToID = -1;
   char *szSubject = NULL, *szDepth = NULL;
   bool bLoop = true, bRead = false;
   CmdTable *pTable = NULL;

   m_pUser->Root();
   m_pUser->Get(NULL, &iCurrID);

   // EDFPrint("CmdMessageList entry", pReply);

   // m_pUser->GetChild("listtype", &iListType);
   debug(DEBUGLEVEL_INFO, "CmdMessageList entry %d\n", iListType);

   pReply->GetChild("searchtype", &iType);
   pReply->GetChild("nummsgs", &iNumMsgs);
   // pReply->GetChild("found", &iFound);

   // debug("CmdMessageList header\n");
   pTable = new CmdTable(m_pUser, 2, iListType == 2 ? 3 : 2, false);
   pTable->AddHeader("ID", 7, '\0', CMDTABLE_RIGHT, 1);
   if(iListType == 2)
   {
      pTable->AddHeader("Replies", 7, '3', CMDTABLE_RIGHT, 1);
   }
   pTable->AddHeader("Subject", CMDTABLE_REMAINDER, '3');

   // debug("CmdMessageList content %d\n", pReply->Children("message", true));
   /* if(iListType == 3)
   {
      iFound = 0;
   } */
   bLoop = pReply->Child("message");
   while(bLoop == true)
   {
      iMsgType = 0;
      iFromID = -1;
      iToID = -1;

      szSubject = NULL;
      pReply->Get(NULL, &iMessageID);
      if(CmdVersion("2.5") >= 0)
      {
         pReply->GetChild("msgtype", &iMsgType);
      }
      pReply->GetChild("fromid", &iFromID);
      pReply->GetChild("toid", &iToID);
      pReply->GetChild("subject", &szSubject);
      bRead = pReply->GetChildBool("read");
      if(CmdVersion("2.5") < 0)
      {
         pReply->GetChild("votetype", &iMsgType);
      }
      if(bRead == false)
      {
         iUnread++;
      }
      if(szSubject == NULL)
      {
         szSubject = strmk("");
      }

      if(iListType == 0 || iListType == 1 ||
         (iListType == 2 && pReply->Depth() == 1) ||
         (iListType == 3 && bRead == false) ||
         (iListType == 4 && ((CmdVersion("2.5") >= 0 && mask(iMsgType, MSGTYPE_VOTE) == true) || (CmdVersion("2.5") < 0 && iMsgType > 0))))
      {
         iFound++;

         pTable->SetFlag(bRead == true ? 'R' : 'U');
         pTable->SetFlag((CmdVersion("2.5") >= 0 && mask(iMsgType, MSGTYPE_VOTE) == true) || (CmdVersion("2.5") < 0 && iMsgType > 0) ? 'V' : ' ');
         pTable->SetValue(iMessageID);
         if(iListType == 2)
         {
            iReplies = pReply->Children("message", true);
            if(iReplies > 0)
            {
               pTable->SetValue(iReplies);
            }
            else
            {
               pTable->SetValue("");
            }
         }
         if(iListType == 0)
         {
            szDepth = new char[pReply->Depth() + 1 + strlen(szSubject) + 2];
            sprintf(szDepth, "%-*s%s", pReply->Depth() - 1, "", szSubject);
            pTable->SetValue(szDepth, iCurrID == iFromID || iCurrID == iToID ? '1' : '\0');
            delete[] szDepth;
         }
         else
         {
            pTable->SetValue(szSubject);
         }
      }

      delete[] szSubject;

      bLoop = pReply->Iterate("message");
   }

   // debug("CmdMessageList footer\n");
   pTable->AddFooter(iNumMsgs, "message");
   if(iListType != 0 && iListType != 3)
   {
      pTable->AddFooter(iFound, "match");
   }
   pTable->AddFooter(iUnread, "unread");
   pTable->AddKey('R', "Read");
   pTable->AddKey('U', "Unread");
   pTable->AddKey('V', "Vote");

   delete pTable;

   debug(DEBUGLEVEL_INFO, "CmdMessageList exit\n");
}

void CmdMessageView(EDF *pReply, int iFolderID, char *szFolderName, int iMsgNum, int iNumMsgs, bool bPage)
{
   STACKTRACE
   int iMsgID = 0, iValue = 0, iReplyID = 0, iMyID = 0, iMsgType = 0, iVoteID = 0, iNumVotes = 0, iTotalVotes = 0, iUserVote = 0, iVoteType = 0, iFolderEDF = -1;
   int iNumReplies = 0, iDate = 0, iAnnDate = 0, iNoteID = 0, iPrevNote = -1, iUserID = 0, iReplyNum = 0, iAttDate = 0, iVoted = 0, iReplyFolder = -1, iMarkType = 0;
   int iMinValue = -1, iMaxValue = -1;
   double dMinValue = 0, dMaxValue = 0;
   // int iFromID = 0, iVoted = 0;
   bool bVoted = false, bMinValue = false, bMaxValue = false;
   char cColour = '7';
   char szWrite[1000], *szFrom = NULL, *szTo = NULL, *szSubject = NULL, *szText = NULL, *szUserName = NULL;//, *szFromName = NULL;
   char szDate[100], *szReplyFolderName = NULL, *szSig = NULL, *szProxyFrom = NULL;
   char *szType = NULL, *szNoteName = NULL, *szNote = NULL, *szUnits = NULL, *szVote = NULL, *szContent = NULL, *szMsgType = NULL;
   // struct tm *tmDate = NULL;
   bool bRetro = false, bFirst = true, bRead = false, bSigFilter = false, bLoop = false, bPrevNote = false;
   bool bSlimHeaders = false;

   // debug("CmdMessageView entry\n");
   // EDFPrint(pReply, false, true);

   if(bPage == true)
   {
      CmdPageOn();
   }

   pReply->Get(NULL, &iMsgID);
   // pReply->GetChild("msgpos", &iMsgNum);
   bRead = pReply->GetChildBool("read");
   pReply->GetChild("marktype", &iMarkType);
   pReply->GetChild("msgtype", &iMsgType);

   m_pUser->Get(NULL, &iMyID);

   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      bSigFilter = m_pUser->GetChildBool("sigfilter");
      bRetro = CmdRetroNames(m_pUser);
      bSlimHeaders = m_pUser->GetChildBool("slimheaders");
      m_pUser->Parent();
   }

   if(mask(iMsgType, MSGTYPE_DELETED) == true)
   {
      if(bSlimHeaders == true)
      {
         szMsgType = "\0371** DELETED **\0370";
      }
      else
      {
         szMsgType = " [\0371** DELETED **\0370]";
      }
   }
   else if(bRead == true)
   {
      if(bSlimHeaders == true)
      {
         szMsgType = "+";
      }
      else
      {
         if(iMarkType == THREAD_CHILD || iMarkType == THREAD_MSGCHILD)
         {
            szMsgType = " [Caught-up]";
         }
         else
         {
            szMsgType = " [Re-read]";
         }
      }
   }
   else
   {
      szMsgType = "";
   }

   pReply->GetChild("date", &iDate);
   StrTime(szDate, bSlimHeaders == true ? STRTIME_MEDIUM : STRTIME_LONG, iDate);//, szFolderName != NULL ? '3' : '1');

   strcpy(szWrite, "\n");
   if(szFolderName != NULL)
   {
      strcat(szWrite, "Message");
      cColour = '7';
   }
   else
   {
      strcat(szWrite, "Bulletin");
      cColour = '1';
   }
   strcat(szWrite, ": ");
   if(bSlimHeaders == true)
   {
      if(szFolderName != NULL)
      {
         sprintf(szWrite, "%s\0377%s\0370/", szWrite, RETRO_NAME(szFolderName));
      }
      sprintf(szWrite, "%s\037%c%d\0370%s", szWrite, cColour, iMsgID, szMsgType);
   }
   else
   {
      sprintf(szWrite, "%s\037%c%d\0370", szWrite, cColour, iMsgID);
   }
   if(iMsgNum > 0 && iNumMsgs > 0)
   {
      sprintf(szWrite, "%s (\037%c%d%s\0370%s\037%c%d\0370)", szWrite, cColour, iMsgNum, NumPos(iMsgNum), bSlimHeaders == true ? "/" : " of ", cColour, iNumMsgs);
   }
   if(bSlimHeaders == false)
   {
      if(szFolderName != NULL)
      {
         sprintf(szWrite, "%s in \0377%s\0370", szWrite, RETRO_NAME(szFolderName));
      }
      strcat(szWrite, szMsgType);
   }
   if(bSlimHeaders == true)
   {
      sprintf(szWrite, "%s at \037%c%s\0370", szWrite, szFolderName != NULL ? '3' : '1', szDate);
   }
   strcat(szWrite, "\n");
   CmdWrite(szWrite);

   if(bSlimHeaders == false)
   {
      // sprintf(szWrite, "Date: %s\n", szDate);
      // CmdWrite(szWrite);
      CmdField("Date", szFolderName != NULL ? '3' : '1', szDate);
   }

   iValue = -1;
   if(pReply->GetChild("fromid", &iValue) == true)
   {
      // m_pUser->SetChild("currfrom", iValue);
      /* if(pCurrFrom != NULL)
      {
         *pCurrFrom = iValue;
      } */
   }

   if(CmdVersion("2.5") >= 0)
   {
      if(mask(iMsgType, MSGTYPE_VOTE) == true && pReply->Child("votes") == true)
      {
         pReply->GetChild("votetype", &iVoteType);
         if(mask(iVoteType, VOTE_INTVALUES) == true)
         {
            bMinValue = pReply->GetChild("minvalue", &iMinValue);
            bMaxValue = pReply->GetChild("maxvalue", &iMaxValue);
         }
         else if(mask(iVoteType, VOTE_FLOATVALUES) == true)
         {
            bMinValue = pReply->GetChild("minvalue", &dMinValue);
            bMaxValue = pReply->GetChild("maxvalue", &dMaxValue);
         }
         /* if(pVoteType != NULL)
         {
            *pVoteType = iVoteType;
         }
         if(pVoteID != NULL)
         {
            *pVoteID = pReply->GetChildBool("voteid");
         } */

         pReply->Parent();
      }
      /* else if(pVoteType != NULL)
      {
         *pVoteType = 0;
      } */

      // pReply->Parent();
   }
   else
   {
      if(pReply->GetChild("votetype", &iVoteType) == true && iVoteType > 0)
      {
         // m_pUser->SetChild("votetype", iVoteType);
         /* if(pVoteType != NULL)
         {
            *pVoteType = iVoteType;
         } */
         // m_pUser->SetChild("voted", false);
         /* if(pVoteID != NULL)
         {
            *pVoteID = -1;
         } */
      }
      /* else if(pVoteType != NULL)
      {
         // m_pUser->DeleteChild("votetype");
         *pVoteType = 0;
      } */
   }

   strcpy(szWrite, "");
   if(pReply->GetChild("fromname", &szFrom) == true)
   {
      sprintf(szWrite, "%sFrom: \037%c%s\0370", szWrite, szFolderName == NULL || iValue == iMyID ? '1' : '3', RETRO_NAME(szFrom));
      if(pReply->GetChild("proxyfromname", &szProxyFrom) == true)
      {
         sprintf(szWrite, "%s (posted by \0373%s\0370)", szWrite, RETRO_NAME(szProxyFrom));
         // CmdWrite(szWrite);
         delete[] szProxyFrom;
      }
      if(bSlimHeaders == false)
      {
         strcat(szWrite, "\n");
      }
      // CmdWrite(szWrite);
   }
   iValue = -1;
   pReply->GetChild("toid", &iValue);
   if(pReply->GetChild("toname", &szTo) == true)
   {
      // strcpy(szWrite, "");
      sprintf(szWrite, "%s%s\037%c%s\0370\n", szWrite, bSlimHeaders == true ? " to " : "To: ", iValue == iMyID ? '1' : '3', RETRO_NAME(szTo));
      // CmdWrite(szWrite);
      delete[] szTo;
   }
   else if(bSlimHeaders == true)
   {
      strcat(szWrite, "\n");
   }
   CmdWrite(szWrite);

   if(pReply->GetChild("subject", &szSubject) == true && szSubject != NULL)
   {
      // sprintf(szWrite, "Subject: \037%c%s\0370\n", szFolderName == NULL ? '1' : '3', szSubject);
      // CmdWrite(szWrite);
      CmdField("Subject", szFolderName == NULL ? '1' : '3', szSubject, CMD_OUT_NOHIGHLIGHT | CMD_OUT_UTF);
      delete[] szSubject;
   }

   // Form the In-Reply-To line
   bFirst = true;
   iReplyFolder = iFolderID;
   // debug("CmdMessageView in reply to START\n");
   bLoop = pReply->Child("replyto");
   while(bLoop == true)
   {
      iReplyNum++;

      pReply->Get(NULL, &iReplyID);
      if(bFirst == true)
      {
         sprintf(szWrite, "In-Reply-To: \0377%d\0370", iReplyID);
         bFirst = false;
      }
      else
      {
         sprintf(szWrite, ", \0377%d\0370", iReplyID);
      }
      szFolderName = NULL;
      if(pReply->GetChild("foldername", &szFolderName) == true)
      {
         pReply->GetChild("folderid", &iFolderEDF);
         if(iFolderEDF != iReplyFolder)
         {
            sprintf(szWrite, "%s (in \0377%s\0370)", szWrite, RETRO_NAME(szFolderName));
            iReplyFolder = iFolderEDF;
         }
         delete[] szFolderName;
      }
      if(pReply->GetChildBool("exist", true) == false)
      {
         strcat(szWrite, "*");
      }
      CmdWrite(szWrite);

      bLoop = pReply->Next("replyto");
      if(bLoop == false)
      {
         pReply->Parent();
      }
      else if(bSlimHeaders == true && iReplyNum >= 3)
      {
         bLoop = false;
         pReply->Parent();

         sprintf(szWrite, "... (\0377%d\0370 more)", pReply->Children("replyto") - 3);
         CmdWrite(szWrite);
      }
   }
   if(bFirst == false)
   {
      // Only need carriage return if there were replyto fields
      CmdWrite("\n");
   }
   // debug("CmdMessageView in reply to STOP\n");

   pReply->GetChild("numreplies", &iNumReplies);
   if(iNumReplies > 0)
   {
      // sprintf(szWrite, "All-Replies: \0373%d\0370\n", iNumReplies);
      // CmdWrite(szWrite);
      CmdField("All-Replies", '3', iNumReplies);
   }

   if(bSlimHeaders == false)
   {
      bFirst = true;
      bLoop = pReply->Child();
      while(bLoop == true)
      {
         pReply->Get(&szType);

         if(stricmp(szType, "delete") == 0 || stricmp(szType, "edit") == 0 || stricmp(szType, CmdVersion("2.5") >= 0 ? "move" : "movefrom") == 0)
         {
            bFirst = false;
            szUserName = NULL;

            pReply->Get(NULL, &iValue);
            pReply->GetChild("username", &szUserName);

            StrTime(szDate, STRTIME_SHORT, iValue);

            if(stricmp(szType, "delete") == 0)
            {
               strcpy(szWrite, "Deleted");
            }
            else if(stricmp(szType, "edit") == 0)
            {
               strcpy(szWrite, "Edited");
            }
            else if(stricmp(szType, CmdVersion("2.5") >= 0 ? "move" : "movefrom") == 0)
            {
               strcpy(szWrite, "Moved-From");
            }
            strcat(szWrite, ": ");
            if(stricmp(szType, CmdVersion("2.5") >= 0 ? "move" : "movefrom") == 0)
            {
               pReply->GetChild("foldername", &szFolderName);
               sprintf(szWrite, "%s\0373%s\0370, ", szWrite, szFolderName);
               delete[] szFolderName;
            }
            sprintf(szWrite, "%s\0373%s\0370", szWrite, szDate);
            if(szUserName != NULL)
            {
               sprintf(szWrite, "%s (by \0373%s\0370)", szWrite, RETRO_NAME(szUserName));
            }
            strcat(szWrite, "\n");
            CmdWrite(szWrite);

            delete[] szUserName;
         }

         bLoop = pReply->Next();
         if(bLoop == false)
         {
            pReply->Parent();
         }
      }

      if(bFirst == false)
      {
         // Only need carriage return if there were fields printed
         CmdWrite("\n");
      }
   }

   if(pReply->GetChild("text", &szText) == true && szText != NULL)
   {
      CmdWrite("\n");

      if(bSigFilter == true)
      {
         szSig = strrchr(szText, '\n');
         if(szSig != NULL)
         {
            while(szSig[0] != '\0' && (szSig[0] == '\r' || szSig[0] == '\n'))
            {
               szSig++;
            }
            if(szSig[0] != '\0')
            {
               debug(DEBUGLEVEL_DEBUG, "CmdMessageView last line '%s'", szSig);
               if(stricmp(szSig, "ref") == 0 || stricmp(szSig, "weazel") == 0 || stricmp(szSig, "adam") == 0)
               {
                  debug(DEBUGLEVEL_DEBUG, ". remove");
                  do
                  {
                     szSig[0] = '\0';
                     szSig--;
                  } while(szSig > szText && (szSig[0] == '\r' || szSig[0] == '\n'));
               }
               debug(DEBUGLEVEL_DEBUG, "\n");
            }
         }
      }
      CmdWrite(szText, CMD_OUT_NOHIGHLIGHT | CMD_OUT_UTF);
      CmdWrite("\n");
   }

   if((CmdVersion("2.5") >= 0 && mask(iMsgType, MSGTYPE_VOTE) == true) || (CmdVersion("2.5") < 0 && iVoteType > 0))
   {
      if(CmdVersion("2.5") >= 0)
      {
         pReply->Child("votes");
      }

      pReply->GetChild("numvotes", &iTotalVotes);
      debug(DEBUGLEVEL_DEBUG, "CmdMessageView vote type %d, total %d\n", iVoteType, iTotalVotes);

      CmdWrite("\n--\n");

      sprintf(szWrite, "Vote:");
      if(mask(iVoteType, VOTE_NAMED) == true)
      {
         sprintf(szWrite, "%s%s \0373Name logged\0370", szWrite, strlen(szWrite) > 10 ? "," : "");
         if(mask(iVoteType, VOTE_CHANGE) == true)
         {
            strcat(szWrite, "(\0373changable\0370)");
         }
      }
      /* else if(mask(iVoteType, VOTE_CHOICE) == true)
      {
         sprintf(szWrite, "%s%s voter's choice", szWrite, strlen(szWrite) > 10 ? "," : "");
      } */
      else
      {
         sprintf(szWrite, "%s%s \0373Anonymous\0370", szWrite, strlen(szWrite) > 10 ? "," : "");
      }
      /* if(mask(iVoteType, VOTE_VALUES) == true)
      {
         sprintf(szWrite, "%s%s values%s", szWrite, strlen(szWrite) > 10 ? "," : "", iMinValue == 0 && iMaxValue == 100 ? "(percentage)" : "");
      } */
      if(mask(iVoteType, VOTE_PUBLIC) == true)
      {
         sprintf(szWrite, "%s%s \0373public\0370", szWrite, strlen(szWrite) > 10 ? "," : "");
      }
      else if(mask(iVoteType, VOTE_PUBLIC_CLOSE) == true)
      {
         sprintf(szWrite, "%s%s \0373public after closing\0370", szWrite, strlen(szWrite) > 10 ? "," : "");
      }
      if(mask(iVoteType, VOTE_CLOSED) == true)
      {
         sprintf(szWrite, "%s%s \0373closed\0370", szWrite, strlen(szWrite) > 10 ? "," : "");
      }
      if(mask(iVoteType, VOTE_INTVALUES) == true)
      {
         sprintf(szWrite, "%s%s \0373integer\0370", szWrite, strlen(szWrite) > 10 ? "," : "");

         if(bMinValue == true && bMaxValue == true)
         {
            sprintf(szWrite, "%s (\0373%d\0370 to \0373%d\0370)", szWrite, iMinValue, iMaxValue);
         }
         else if(bMinValue == true)
         {
            sprintf(szWrite, "%s (\0373%d\0370 or more)", szWrite, iMinValue);
         }
         else if(bMaxValue == true)
         {
            sprintf(szWrite, "%s (\0373%d\0370 or less)", szWrite, iMaxValue);
         }
      }
      else if(mask(iVoteType, VOTE_PERCENT) == true)
      {
         sprintf(szWrite, "%s%s \0373percent\0370", szWrite, strlen(szWrite) > 10 ? "," : "");
      }
      else if(mask(iVoteType, VOTE_FLOATVALUES) == true)
      {
         sprintf(szWrite, "%s%s \0373float\0370", szWrite, strlen(szWrite) > 10 ? "," : "");

         if(bMinValue == true && bMaxValue == true)
         {
            sprintf(szWrite, "%s (\0373%g\0370 to \0373%g\0370)", szWrite, dMinValue, dMaxValue);
         }
         else if(bMinValue == true)
         {
            sprintf(szWrite, "%s (\0373%g\0370 or more)", szWrite, dMinValue);
         }
         else if(bMaxValue == true)
         {
            sprintf(szWrite, "%s (\0373%g\0370 or less)", szWrite, dMaxValue);
         }
      }
      else if(mask(iVoteType, VOTE_FLOATPERCENT) == true)
      {
         sprintf(szWrite, "%s%s \0373percent(float)\0370", szWrite, strlen(szWrite) > 10 ? "," : "");
      }
      else if(mask(iVoteType, VOTE_STRVALUES) == true)
      {
         sprintf(szWrite, "%s%s \0373string\0370", szWrite, strlen(szWrite) > 10 ? "," : "");
      }

      strcat(szWrite, "\n");
      CmdWrite(szWrite);

      bFirst = true;
      bLoop = pReply->Child("vote");
      while(bLoop == true)
      {
         iVoteID = 0;
         iNumVotes = 0;
         szVote = NULL;

         pReply->Get(NULL, &iVoteID);
         pReply->GetChild("numvotes", &iNumVotes);
         bVoted = pReply->GetChildBool("voted");

         if(CmdVersion("2.5") < 0 && iVoted == 0)
         {
            bLoop = pReply->Child("userid");
            while(bLoop == true)
            {
               pReply->Get(NULL, &iUserVote);
               if(iUserVote == iMyID)
               {
                  bLoop = false;
                  iVoted = 1;
                  // m_pUser->SetChild("voted", true);
                  /* if(pVoteID != NULL)
                  {
                     *pVoteID = iVoteID;
                  } */

                  pReply->Parent();
               }
               else
               {
                  bLoop = pReply->Next("userid");
                  if(bLoop == false)
                  {
                     pReply->Parent();
                  }
               }
            }
         }

         pReply->GetChild("text", &szVote);
         if(IS_INT_VOTE(iVoteType) == true || IS_FLOAT_VOTE(iVoteType) == true)
         {
            sprintf(szWrite, "%s\0373%s\0370%s", bFirst == false ? ", " : "", szVote, mask(iVoteType, VOTE_FLOATVALUES) == true || mask(iVoteType, VOTE_FLOATPERCENT) == true ? "%" : "");
            if(iNumVotes > 1)
            {
               sprintf(szWrite, "%s(\0373%d\0370)", szWrite, iNumVotes);
            }
            CmdWrite(szWrite);
            bFirst = false;
         }
         else
         {
				strcpy(szWrite, "");
				if(mask(iVoteType, VOTE_STRVALUES) == true)
				{
					CmdWrite(szVote);
					strcat(szWrite, " ");
				}
				else
				{
					sprintf(szWrite, "%sVote \0373%d\0370 ", szWrite, iVoteID);
				}
            if(iVoted == 1 || bVoted == true)
            {
               strcat(szWrite, " *");
               iVoted = 2;
            }
            if(iTotalVotes > 0)
            {
               sprintf(szWrite, "%s(\0373%d\0370 voter%s, \0373%d\0370%%)", szWrite, iNumVotes, iNumVotes != 1 ? "s" : "", (100 * iNumVotes) / iTotalVotes);
            }
				if(mask(iVoteType, VOTE_STRVALUES) == false)
				{
					strcat(szWrite, ": ");
				}
				CmdWrite(szWrite);
				if(mask(iVoteType, VOTE_STRVALUES) == false && szVote != NULL)
				{
					CmdWrite(szVote);
				}
            CmdWrite("\n");
         }
			MAKE_NULL(szVote);

         bLoop = pReply->Next("vote");
         if(bLoop == false)
         {
            pReply->Parent();
         }
      }

      if(iVoteType == VOTE_SIMPLE)
      {
         CmdWrite("\n");
      }
      else if(IS_INT_VOTE(iVoteType) == true || IS_FLOAT_VOTE(iVoteType) == true)
      {
         CmdWrite("\n");
      }

      if(iTotalVotes > 0)
      {
         bVoted = pReply->GetChildBool("voted");

         sprintf(szWrite, "Total: \0373%d\0370", iTotalVotes);
         /* if(mask(iVoteType, VOTE_CLOSED) == true)
         {
            strcat(szWrite, " (closed)");
         } */
         if(bVoted == true)
         {
            strcat(szWrite, " (you have voted)");
            iVoted = 2;
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);
      }
      else if(pReply->GetChildBool("voted") == true)
      {
         CmdWrite("You have voted\n");
         iVoted = 2;
      }

      if(CmdVersion("2.5") >= 0)
      {
         pReply->Parent();
      }
   }

   // Formulate the Replied-To-By lines
   iAnnDate = iDate;
   bFirst = true;
   bLoop = pReply->Child();
   while(bLoop == true)
   {
      pReply->Get(&szType, &iReplyID);
      if(stricmp(szType, "replyby") == 0)
      {
         szReplyFolderName = NULL;
         szUserName = NULL;

         if(bFirst == true || bPrevNote == true)
         {
            CmdWrite("--\n");
            bFirst = false;
         }
         pReply->GetChild("foldername", &szReplyFolderName);
         pReply->GetChild("fromid", &iUserID);
         pReply->GetChild("fromname", &szUserName);

         sprintf(szWrite, "Replied-to-by: \037%c%s\0370 (\037%c%d\0370", iUserID == iMyID ? '1' : '3', RETRO_NAME(szUserName), iUserID == iMyID ? '1' : '7', iReplyID);
         if(szReplyFolderName != NULL)
         {
            sprintf(szWrite, "%s in \037%c%s\0370", szWrite, iUserID == iMyID ? '1' : '7', RETRO_NAME(szReplyFolderName));
         }
         strcat(szWrite, ")\n");
         CmdWrite(szWrite);
         delete[] szUserName;
         delete[] szReplyFolderName;

         bPrevNote = false;
         iPrevNote = -1;
      }
      else if(CmdVersion("2.5") < 0 && stricmp(szType, "note") == 0)
      {
         bFirst = false;
         szNote = NULL;

         pReply->Get(NULL, &iDate);
         pReply->GetChild("fromid", &iNoteID);
         pReply->GetChild("fromname", &szNoteName);
         pReply->GetChild("text", &szNote);

         if(iNoteID != iPrevNote)
         {
            CmdWrite("--\n");

            // pReply->GetChild("date", &iValue);
            iValue = iDate - iAnnDate;
            iAnnDate = iDate;
            if(iValue > 3600)
            {
               iValue /= 3600;
               szUnits = "hour";
            }
            else if(iValue > 60)
            {
               iValue /= 60;
               szUnits = "minute";
            }
            else
            {
               szUnits = "second";
            }
            sprintf(szWrite, "Annotated-By: \037%c%s\0370 (\037%c%d\0370 %s%s after message posted)\n", iNoteID == iMyID ? '1' : '3', RETRO_NAME(szNoteName), iNoteID == iMyID ? '1' : '3', iValue, szUnits, iValue != 1 ? "s" : "");
            CmdWrite(szWrite);

            iPrevNote = iNoteID;
         }
         CmdWrite(szNote, CMD_OUT_NOHIGHLIGHT | CMD_OUT_UTF);
         CmdWrite("\n");

         delete[] szNoteName;
         delete[] szNote;

         bPrevNote = true;
      }
      else if(CmdVersion("2.5") >= 0 && stricmp(szType, "attachment") == 0)
      {
         debugEDFPrint("CmdMessageView attachment", pReply, EDFElement::EL_CURR | EDFElement::PR_SPACE);

         if(pReply->GetChild("content-type", &szContent) == true && szContent != NULL)
         {
            if(stricmp(szContent, MSGATT_ANNOTATION) == 0)
            {
               szNoteName = NULL;
               szNote = NULL;
               bPrevNote = true;

               pReply->GetChild("date", &iAttDate);
               pReply->GetChild("fromid", &iNoteID);
               pReply->GetChild("fromname", &szNoteName);
               pReply->GetChild("text", &szNote);

               strcpy(szDate, "");
               StrValue(szDate, STRVALUE_TIMESU, iAttDate - iDate, iNoteID == iMyID ? '1' : '3');
               CmdWrite("--\n");
               sprintf(szWrite, "Annotated-By: \037%c%s\0370 (%s after message posted)", iNoteID == iMyID ? '1' : '3', RETRO_NAME(szNoteName), szDate);
               CmdWrite(szWrite);

               if(pReply->IsChild("delete") == true)
               {
                  CmdWrite(" \0371[** DELETED **]\0370\n");
               }
               else
               {
                  CmdWrite("\n");
                  CmdWrite(szNote, CMD_OUT_NOHIGHLIGHT | CMD_OUT_UTF);
                  CmdWrite("\n");
               }

               delete[] szNoteName;
               delete[] szNote;
            }

            delete[] szContent;
         }
      }

      delete[] szType;

      bLoop = pReply->Next();
      if(bLoop == false)
      {
         pReply->Parent();
      }
   }

   CmdContentList(pReply, true);

   CmdWrite("\n");

   delete[] szFrom;
   delete[] szText;

   if(bPage == true)
   {
      CmdPageOff();
   }

   // EDFPrint("CmdMessageView current message", m_pMessageView, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   // debug("CmdMessageView exit\n");
}

void CmdChannelList(EDF *pReply)
{
   STACKTRACE
   int iAccessLevel = LEVEL_NONE, iNumChannels = 0, iFound = 0, iNumMsgs = 0, iTotalMsgs = 0, iSubscribed = 0, iSubType = 0;
   bool bLoop = false;
   char *szName = NULL;
   // char szWrite[200];
   // struct channelslot *clist = NULL;
   CmdTable *pTable = NULL;

   // pReply->GetChild("searchtype", &iType);

   debug(DEBUGLEVEL_INFO, "CmdChannelList entry\n");
   // EDFPrint(NULL, pReply);

   m_pUser->GetChild("accesslevel", &iAccessLevel);

   pReply->GetChild("numchannels", &iNumChannels);
   pReply->GetChild("found", &iFound);

   pReply->Sort("channel", "name", true);

   pTable = new CmdTable(m_pUser, 1, iAccessLevel >= LEVEL_WITNESS ? 3 : 2, true);

   pTable->AddHeader("Name", UA_NAME_LEN, 'g');
   pTable->AddHeader("Msgs", 8, '\0', CMDTABLE_RIGHT);
   pTable->AddHeader("Size", 8, '\0', CMDTABLE_RIGHT);

   debug(DEBUGLEVEL_DEBUG, "CmdChannelList content %d\n", iFound);
   bLoop = pReply->Child("channel");
   // for(iChannelNum = 0; iChannelNum < iFound; iChannelNum++)
   while(bLoop == true)
   {
      szName = NULL;
      iSubType = 0;
      iNumMsgs = 0;
      iTotalMsgs = 0;

      pReply->GetChild("name", &szName);
      pReply->GetChild("subtype", &iSubType);
      pReply->GetChild("nummsgs", &iNumMsgs);
      pReply->GetChild("totalmsgs", &iTotalMsgs);

      pTable->SetFlag(iSubType > 0 ? 'S' : ' ');
      pTable->SetValue(szName);
      pTable->SetValue(iNumMsgs);
      if(iAccessLevel >= LEVEL_WITNESS)
      {
         pTable->SetValue(iTotalMsgs);
      }

      delete[] szName;

      if(iSubType > 0)
      {
         iSubscribed++;
      }

      bLoop = pReply->Child("channel");
   }

   pTable->AddFooter(iNumChannels, "channel");
   pTable->AddFooter(iFound, "found");
   pTable->AddFooter(iSubscribed, "sucsribed");

   delete pTable;

   debug(DEBUGLEVEL_INFO, "CmdChannelList exit\n");
}

void CmdChannelView(EDF *pReply)
{
   STACKTRACE
   int iChannelID = 0, iNumMsgs = 0, iSize = 0, iValue = 0, iAccessLevel = LEVEL_NONE, iSubs = 0, iMaxMsgs = 0;
   bool bRetro = false;
   char *szChannelName = NULL, szWrite[100], *szValue = NULL;
   // struct tm *tmDate = NULL;

   m_pUser->GetChild("accesslevel", &iAccessLevel);
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      bRetro = CmdRetroNames(m_pUser);
      m_pUser->Parent();
   }

   debug(DEBUGLEVEL_INFO, "CmdChannelView entry %d\n", iAccessLevel);
   // EDFPrint(NULL, pReply);

   pReply->Get(NULL, &iChannelID);
   debug(DEBUGLEVEL_DEBUG, "ChannelShow ID %d\n", iChannelID);

   debug(DEBUGLEVEL_DEBUG, "ChannelShow content\n");
   pReply->GetChild("name", &szChannelName);
   strcpy(szWrite, "");
   if(szChannelName != NULL)
   {
      // sprintf(szWrite, "Name: \0373%s\0370\n", RETRO_NAME(szChannelName));
      CmdField("Name", '3', RETRO_NAME(szChannelName));
   }
   if(iAccessLevel >= LEVEL_WITNESS)
   {
      // sprintf(szWrite, "%sID:   \0373%d\0370\n", szWrite, iChannelID);
      CmdField("ID", '3', iChannelID);
   }
   // CmdWrite(szWrite);

   if(pReply->Child("info") == true)
   {
      debug(DEBUGLEVEL_DEBUG, "CmdChannelInfo info entry\n");

      pReply->GetChild("date", &iValue);
      // tmDate = localtime((time_t *)&iValue);
      // strftime(szWrite, sizeof(szWrite), "\nDate: \0373%A, %d %B %Y - %H:%M\0370\n", tmDate);
      StrTime(szWrite, STRTIME_LONG, iValue, '3', "\nDate: ", "\n");
      CmdWrite(szWrite);

      szValue = NULL;
      if(pReply->GetChild("fromname", &szValue) == true)
      {
         // sprintf(szWrite, "From: \0377%s\0370\n", szValue);
         // CmdWrite(szWrite);
         CmdField("From", '7', szValue);
         delete[] szValue;
      }

      szValue = NULL;
      if(pReply->GetChild("text", &szValue) == true)
      {
         CmdWrite("\n");
         CmdWrite(szValue, CMD_OUT_NOHIGHLIGHT | CMD_OUT_UTF);
         CmdWrite("\n\n");
         delete[] szValue;
      }
      pReply->Parent();

      debug(DEBUGLEVEL_DEBUG, "CmdChannelInfo info exit\n");
   }
   else
   {
      CmdWrite("\n");
   }

   pReply->GetChild("nummsgs", &iNumMsgs);
   pReply->GetChild("totalmsgs", &iSize);
   pReply->GetChild("maxmsgs", &iMaxMsgs);
   if(iAccessLevel >= LEVEL_WITNESS)
   {
      sprintf(szWrite, "Messages:    \0373%d\0370 (", iNumMsgs);
      if(iMaxMsgs > 0)
      {
         sprintf(szWrite, "%smaximum \0373%d\0370, ", szWrite, iMaxMsgs);
      }
      sprintf(szWrite, "%stotal size \0373%d\0370 bytes)\n", szWrite, iSize);
      CmdWrite(szWrite);

      pReply->GetChild("created", &iValue);
      // tmDate = localtime((time_t *)&iValue);
      // strftime(szWrite, 64, "Created:     \0373%A, %d %B, %Y - %H:%M\0370\n", tmDate);
      StrTime(szWrite, STRTIME_LONG, iValue, '3', "Created:    ", "\n");
      CmdWrite(szWrite);
   }
   else
   {
      // sprintf(szWrite, "Messages: \0373%d\0370\n", iNumMsgs);
      // CmdWrite(szWrite);
      CmdField("Messages", '3', iNumMsgs);
   }
   CmdWrite("\n");

   iSubs = CmdSubList(pReply, SUBTYPE_SUB, SUBINFO_TREE, NULL, bRetro);
   if(iSubs > 0)
   {
      CmdWrite("\n");
   }

   delete[] szChannelName;

   debug(DEBUGLEVEL_INFO, "CmdChannelView exit\n");
}

void CmdUserList(EDF *pReply, int iListType, int iOwnerID)
{
   STACKTRACE
   int iNumUsers = 0, iUserNum = 0, iFound = 0, iAccessLevel = LEVEL_NONE, iID = 0, iUserLevel = LEVEL_NONE, iUserType = 0, iOwnerEDF = 0;
   bool bLoop = false;
   char *szName = NULL;
   // char szWrite[200];
   CmdTable *pTable = NULL;

   debug(DEBUGLEVEL_INFO, "CmdUserList entry\n");
   // EDFPrint(NULL, pReply);

   m_pUser->GetChild("accesslevel", &iAccessLevel);

   pReply->GetChild("numusers", &iNumUsers);

   debug(DEBUGLEVEL_DEBUG, "CmdUserList header\n");
   pTable = new CmdTable(m_pUser, 0, 2, false, true);
   pTable->AddHeader("ID", 4, '\0', CMDTABLE_RIGHT);
   pTable->AddHeader("Name", UA_NAME_LEN);

   pReply->GetChild("found", &iFound);
   // debug("CmdUserList found %d\n", iFound);

   if(iFound > 0)
   {
      pReply->Sort("user", "name", false);

      debug(DEBUGLEVEL_INFO, "CmdUserList content %d\n", iFound);
      bLoop = pReply->Child("user");
      while(bLoop == true)
      {
         szName = NULL;
         iUserType = USERTYPE_NONE;
         iOwnerEDF = -1;

         pReply->Get(NULL, &iID);
         pReply->GetChild("name", &szName);
         pReply->GetChild("usertype", &iUserType);
         pReply->GetChild("accesslevel", &iUserLevel);
         // pReply->GetChild("ownerid", &iOwnerEDF);

         // debug("CmdUserList %d %p %s\n", iID, szName, szName);
         // if(iListType == 0 || (iListType == 1 && iUserType == USERTYPE_DELETED) || (iListType == 2 && iOwnerEDF != -1 && (iOwnerID == -1 || iOwnerID == iOwnerEDF)))
         if(CmdUserListMatch(pReply, iListType, iOwnerID) == true)
         {
            pTable->SetValue(iID);
            pTable->SetValue(szName, AccessColour(iUserLevel, iUserType));
         }

         delete[] szName;

         bLoop = pReply->Next("user");
         iUserNum++;
      }
   }
   debug(DEBUGLEVEL_DEBUG, "CmdUserList footer\n");
   // Footer(pUser, false, "user", iNumUsers, "found", iFound, NULL);
   pTable->AddFooter(iNumUsers, "user");
   pTable->AddFooter(iFound, "found");

   delete pTable;

   debug(DEBUGLEVEL_INFO, "CmdUserList exit\n");
}

bool CmdUserListMatch(EDF *pReply, int iListType, int iOwnerID)
{
   int iUserID = -1, iUserType = USERTYPE_NONE, iOwnerEDF = -1;

   if(iListType == 0)
   {
      return true;
   }

   pReply->GetChild("usertype", &iUserType);

   if((iListType == 1 && mask(iUserType, USERTYPE_AGENT) == true) ||
      (iListType == 2 && mask(iUserType, USERTYPE_DELETED) == true))
   {
      return true;
   }

   pReply->Get(NULL, &iUserID);
   pReply->GetChild("ownerid", &iOwnerEDF);

   if(iOwnerEDF != -1 && (iOwnerID == -1 ||
      (iListType == 3 && iOwnerID == iOwnerEDF) ||
      (iListType == 4 && iUserID == iOwnerID)))
   {
      return true;
   }

   return false;
}

/* CmdUserWho: List logged in users
**
** listtype: Display option
** 0 - Sort by time
** 1 - Sort by access level
** 2 - Sort by name
** 3 - Sort by name (name only)
** 4 - Sort by idle time (idle users only)
** 5 - Sort by time (active users only)
** 6 - Sort by time (client / protocol info)
** 7 - Sort by time (hide long idles users)
** 8 - Sort by busytime (busy users only, show busy message)
** 10 - Show proxy logins
** 11 - Sort by time (status messages only)
** 12 - Sort by name (hostname / client info)
** 13 - Sort by name (hostname / address info)
*/
void CmdUserWho(EDF *pReply, int iListType)
{
   STACKTRACE
   int iAccessLevel = LEVEL_NONE, iWidth = 80, iSystemTime = 0, iFound = 0, iHidden = 0, iHiddenTime = 10800;
   int iUserStatus = LOGIN_OFF, iUserLevel = LEVEL_NONE, iUserType = USERTYPE_NONE, iMaxFlags = 0;
   int iAccessLen = 8, iNameLen = 4, iTimeVal = 0, iNumFlags = 0, iTimeIdle = 0, iExtra = 0;
   int iNumLogins = 0, iNumRows = 0, iIdle = 0, iBusy = 0, iTalking = 0, iAgents = 0, iConnID = 0;//, iPos = 0;
   bool bRetro = false, bLoop = false, bHidden = false, bAccessNames = true, bSecure = false;
   char *szType = NULL, *szUserName = NULL, *szUserAccess = NULL, *szField1 = NULL, *szField2 = NULL;
   char szTime[10], *szColTime = "Time", szConn[30];
   char cColour = '\0';
   CmdTable *pTable = NULL;

   debug(DEBUGLEVEL_INFO, "CmdUserWho entry\n");

   m_pUser->GetChild("accesslevel", &iAccessLevel);
   iWidth = CmdWidth();
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      bAccessNames = m_pUser->GetChildBool("accessnames", true);
      m_pUser->GetChild("hiddentime", &iHiddenTime);
      bRetro = CmdRetroNames(m_pUser);
      m_pUser->Parent();
   }

   // printf("CmdUserWho accessname %s\n", BoolStr(bAccessNames));

   // int iTemp = 0;
   // pReply->GetChild("systemtime", &iTemp);
   iSystemTime = CmdInput::MenuTime();
   // printf("CmdUserWho times %d -vs- %d actual, diff %d\n", iSystemTime, iTemp, abs(iSystemTime - iTemp));
   pReply->GetChild("found", &iFound);

   // iHiddenTime = iSystemTime - iHiddenTime;

   // debug("CmdUserWho entry %d (level %d), hidden %d\n", iListType, iAccessLevel, iHiddenTime);
   // CmdEDFPrint(NULL, pReply, true, false);

   if(iListType == 1)
   {
      // pReply->Sort(NULL, "accesslevel", false, false);
      pReply->SortReset(NULL);
      pReply->SortAddKey("accesslevel", false);
      pReply->SortParent();
      pReply->SortAddKey("name");
      pReply->Sort();
   }
   else if(iListType == 2 || iListType == 3 || iListType == 10 || iListType == 12)
   {
      pReply->Sort(NULL, "name", false);
   }
   else
   {
      // pReply->Sort(NULL, "timeidle", false, true, "login");
      pReply->SortReset(NULL);
      pReply->SortAddSection("login");
      if(iListType == 4)
      {
         pReply->SortAddKey("timeidle");
      }
      else if(iListType == 6)
      {
         pReply->SortAddKey("client");
      }
      else if(iListType == 8)
      {
         pReply->SortAddKey("timebusy");
      }
      else
      {
         pReply->SortAddKey("timeon");
      }
      pReply->SortParent();
      pReply->SortParent();
      pReply->SortAddKey("name");
      pReply->Sort();
   }
   /* else if(iListType == 6)
   {
      pReply->Sort(NULL, "client", false, true, "login");
   }
   else if(iListType == 8)
   {
      pReply->Sort(NULL, "timebusy", false, true, "login");
   }
   else
   {
      pReply->Sort(NULL, "timeon", false, true, "login");
   } */

   iMaxFlags++; // LOGIN_TALK
   if(iListType != 4 && iListType != 5) // && mask(iUserStatus, LOGIN_IDLE) == true)
   {
      iMaxFlags++;
   }
   if(iListType != 8) // && mask(iUserStatus, LOGIN_BUSY) == true)
   {
      iMaxFlags++;
   }

   // debug("CmdUserWho column widths\n");
   bLoop = pReply->Child(NULL);
   while(bLoop == true)
   {
      pReply->Get(&szType);

      if(stricmp(szType, "user") == 0)
      {
         szUserName = NULL;
         szUserAccess = NULL;

         pReply->GetChild("name", &szUserName);
         if(szUserName != NULL && strlen(szUserName) > iNameLen)
         {
            iNameLen = strlen(szUserName);
         }

         if(bAccessNames == true)
         {
            pReply->GetChild("accessname", &szUserAccess);
            if(szUserAccess != NULL && strlen(szUserAccess) > iAccessLen)
            {
               iAccessLen = strlen(szUserAccess);
            }
            delete[] szUserAccess;
         }

         if(pReply->Child("login") == true)
         {
            iUserStatus = 0;
            iNumFlags = 0;

            pReply->Get(NULL, &iConnID);
            pReply->GetChild("status", &iUserStatus);
            // debug("CmdUserWho check %s status %d\n", szUserName, iUserStatus);

            if(iListType != 4 && iListType != 5) // && mask(iUserStatus, LOGIN_IDLE) == true)
            {
               iNumFlags++;
            }
            if(iListType != 8) // && mask(iUserStatus, LOGIN_BUSY) == true)
            {
               iNumFlags++;
            }
            if(mask(iUserStatus, LOGIN_TALKING) == true)
            {
               iNumFlags++;
            }
            if(mask(iUserStatus, LOGIN_SILENT) == true)
            {
               iNumFlags++;
            }
            if(mask(iUserStatus, LOGIN_SHADOW) == true)
            {
               iNumFlags++;
            }
            if(mask(iUserStatus, LOGIN_NOCONTACT) == true)
            {
               iNumFlags++;
            }
            if(pReply->GetChildBool("secure") == true)
            {
               iNumFlags++;
            }
            if(iNumFlags > iMaxFlags)
            {
               iMaxFlags = iNumFlags;
            }
            pReply->Parent();

            if(mask(iUserStatus, LOGIN_SHADOW) == true)
            {
               sprintf(szConn, "%s [%d]", szUserName, iConnID);
               if(strlen(szConn) > iNameLen)
               {
                  iNameLen = strlen(szConn);
               }
            }
         }

         delete[] szUserName;
      }
      else if(stricmp(szType, "connection") == 0)
      {
         if(iNameLen < 13)
         {
            iNameLen = 13;
         }
      }

      delete[] szType;

      bLoop = pReply->Next();
   }

   pReply->Root();

   /* if(iListType != 4 && iListType != 5)
   {
      // Flag for idle
      iNumFlags++;
   }
   if(iListType != 8)
   {
      // Flag for busy
      iNumFlags++;
   } */
   // Flags for talking and silent (admin and agent owners, best make it everyone)
   // iNumFlags += 2;
   // printf("CmdUserWho %d flags in table\n", iNumFlags);

   // debug("CmdUserWho header\n");
   // debug("CmdUserWho %d flags\n", iMaxFlags);
   if(iListType == 3)
   {
      pTable = new CmdTable(m_pUser, iMaxFlags, 1, true, true);

      pTable->AddHeader("Name", iNameLen);
   }
   else
   {
      pTable = new CmdTable(m_pUser, iMaxFlags, iListType == 8 || iListType == 11 ? 3: 4, true, false);

      if(iListType == 4)
      {
         szColTime = "Idle";
      }
      else if(iListType == 8)
      {
         szColTime = "Busy";
      }
      pTable->AddHeader(szColTime, 7, '\0', CMDTABLE_RIGHT, 1);
      pTable->AddHeader("Access", iAccessLen);
      pTable->AddHeader("Name", iNameLen);
      if(iListType != 8 && iListType != 11)
      {
         if(iAccessLevel >= LEVEL_WITNESS)
         {
            if(iListType == 6)
            {
               pTable->AddHeader("Client / Protocol");
            }
            else if(iListType == 10)
            {
               pTable->AddHeader("Host / Proxy");
            }
            else if(iListType == 12)
            {
               pTable->AddHeader("Host / Client");
            }
            else if(iListType == 13)
            {
               pTable->AddHeader("Host / Address");
            }
            else
            {
               pTable->AddHeader("Host / Location");
            }
         }
         else
         {
            pTable->AddHeader("Location");
         }
      }
   }

   // debug("CmdUserWho content %d\n", iNumRows);
   bLoop = pReply->Child(NULL);
   while(bLoop == true)
   {
      pReply->Get(&szType);
      if(stricmp(szType, "user") == 0 || stricmp(szType, "connection") == 0)
      {
         szUserName = NULL;
         iUserLevel = LEVEL_NONE;
         iUserType = USERTYPE_NONE;
         szUserAccess = NULL;
         iUserStatus = LOGIN_OFF;
         szField1 = NULL;
         szField2 = NULL;
         iTimeVal = 0;
         iTimeIdle = 0;
         bHidden = false;
         bSecure = false;
         iExtra = 0;

         pReply->GetChild("name", &szUserName);
         if(szUserName != NULL)
         {
            pReply->GetChild("usertype", &iUserType);
            pReply->GetChild("accesslevel", &iUserLevel);
            if(mask(iUserType, USERTYPE_AGENT) == true)
            {
               szUserAccess = strmk("Agent");
            }
            else
            {
               if(bAccessNames == true)
               {
                  pReply->GetChild("accessname", &szUserAccess);
               }
               if(szUserAccess == NULL)
               {
                  szUserAccess = strmk(AccessName(iUserLevel));
               }
            }
         }
         else
         {
            pReply->Get(NULL, &iConnID);
            szUserName = new char[UA_NAME_LEN];
            sprintf(szUserName, "[connid %d]", iConnID);
            szUserAccess = strmk("");
         }

         cColour = AccessColour(iUserLevel, iUserType);

         if(iListType == 11)
         {
            debug(DEBUGLEVEL_DEBUG, "CmdUserWho user %s\n", szUserName);
         }

         if(pReply->Child("login") == true)
         {
            pReply->Get(NULL, &iConnID);
            pReply->GetChild("status", &iUserStatus);
            if(mask(iUserStatus, LOGIN_SHADOW) == true)
            {
               sprintf(szConn, "\037%c%s\0370 [\037%c%d\0370]", cColour, szUserName, cColour, iConnID);
               delete[] szUserName;
               szUserName = strmk(szConn);

               cColour = '\0';
               iExtra = 8;
            }
            bSecure = pReply->GetChildBool("secure");
            // debug("CmdUserWho content %s status %d\n", szUserName, iUserStatus);
            if(iListType == 4)
            {
               pReply->GetChild("timeidle", &iTimeVal);
            }
            else if(iListType == 8)
            {
               pReply->GetChild("timebusy", &iTimeVal);
            }
            else
            {
               pReply->GetChild("timeon", &iTimeVal);
               // iTimeOn = iSystemTime - iTimeOn;
               pReply->GetChild("timeidle", &iTimeIdle);
               // printf("CmdUserWho time idle %d\n", iTimeIdle);
               if(iTimeIdle > 0)
               {
                  iTimeIdle = iSystemTime - iTimeIdle;
               }
            }
            // printf("CmdUserWho time value %d\n", iTimeVal);
            if(iTimeVal > 0)
            {
               iTimeVal = iSystemTime - iTimeVal;
            }

            if(iAccessLevel >= LEVEL_WITNESS)
            {
               if(iListType == 6)
               {
                  pReply->GetChild("client", &szField1);
                  pReply->GetChild("protocol", &szField2);
               }
               else if(iListType == 8)
               {
                  if(mask(iUserStatus, LOGIN_BUSY) == true)
                  {
                     pReply->GetChild(CmdVersion("2.5") >= 0 ? "statusmsg" : "busymsg", &szField1);
                     // debug("CmdUserWho busy %s\n", szField1);
                  }
               }
               else if(iListType == 10)
               {
                  if(CmdVersion("2.5") >= 0)
                  {
                     if(pReply->GetChild("hostname", &szField1) == false)
                     {
                        pReply->GetChild("address", &szField1);
                     }
                     if(pReply->GetChild("proxyhostname", &szField2) == false)
                     {
                        pReply->GetChild("proxyaddress", &szField2);
                     }
                  }
                  else
                  {
                     if(pReply->Child("hostname") == true)
                     {
                        pReply->Get(NULL, &szField1);
                        pReply->GetChild("hostname", &szField2);
                        pReply->Parent();
                     }
                  }
               }
               else if(iListType == 11)
               {
                  pReply->GetChild(CmdVersion("2.5") >= 0 ? "statusmsg" : "busymsg", &szField1);
                  debug(DEBUGLEVEL_DEBUG, "CmdUserWho status %s\n", szField1);
               }
               else if(iListType == 12)
               {
                  if(pReply->GetChild("hostname", &szField1) == false)
                  {
                     pReply->GetChild("address", &szField1);
                  }
                  pReply->GetChild("client", &szField2);
               }
               else if(iListType == 13)
               {
                  pReply->GetChild("hostname", &szField1);
                  pReply->GetChild("address", &szField2);
               }
               else
               {
                  if(pReply->GetChild("hostname", &szField1) == false)
                  {
                     pReply->GetChild("address", &szField1);
                  }
                  pReply->GetChild("location", &szField2);
                  if(szField2 == NULL)
                  {
                     szField2 = strmk("Unknown");
                  }
               }
            }
            else
            {
               if(iListType == 8)
               {
                  if(mask(iUserStatus, LOGIN_BUSY) == true)
                  {
                     pReply->GetChild(CmdVersion("2.5") >= 0 ? "statusmsg" : "busymsg", &szField1);
                     // debug("CmdUserWho busy %s\n", szField1);
                  }
               }
               else if(iListType == 11)
               {
                  pReply->GetChild(CmdVersion("2.5") >= 0 ? "statusmsg" : "busymsg", &szField1);
                  debug(DEBUGLEVEL_DEBUG, "CmdUserWho status %s, %s\n", szUserName, szField1);
               }
               else
               {
                  pReply->GetChild("location", &szField1);
                  if(szField1 == NULL)
                  {
                     szField1 = strmk("Unknown");
                  }
               }
            }
            pReply->Parent();
         }
         // debug("CmdUserWho row %s\n", szUserName);
         if(szField1 == NULL)
         {
            szField1 = strmk("");
         }
         if(szField2 == NULL)
         {
            szField2 = strmk("");
         }

         if(mask(iUserStatus, LOGIN_IDLE) == true)
         {
            iIdle++;
            if(iHiddenTime > 0 && iTimeIdle > iHiddenTime)
            {
               iHidden++;
               bHidden = true;
            }
         }
         if(mask(iUserStatus, LOGIN_BUSY) == true)
         {
            iBusy++;
         }
         if(mask(iUserStatus, LOGIN_TALKING) == true)
         {
            iTalking++;
         }
         if(mask(iUserStatus, LOGIN_AGENT) == true)
         {
            iAgents++;
         }

         /* if(iListType == 7)
         {
            debug("CmdUserWho %s %s %d %d\n", szUserName, BoolStr(mask(iUserStatus, LOGIN_IDLE)), iTimeIdle, iHiddenTime);
         } */
         if(iListType == 11)
         {
            debug(DEBUGLEVEL_DEBUG, "CmdUserWho status check %s\n", szField1);
         }
         if((iListType >= 0 && iListType <= 3) || iListType == 6 || iListType == 12 || iListType == 13 ||
            (iListType == 4 && mask(iUserStatus, LOGIN_IDLE) == true) ||
            (iListType == 5 && mask(iUserStatus, LOGIN_IDLE) == false && mask(iUserStatus, LOGIN_BUSY) == false) ||
            (iListType == 7 && (bHidden == false || mask(iUserType, USERTYPE_AGENT) == true)) ||
            (iListType == 8 && mask(iUserStatus, LOGIN_BUSY) == true) ||
            (iListType == 10 && strcmp(szField2, "") != 0) ||
            (iListType == 11 && strcmp(szField1, "") != 0))
         {
            // debug("CmdUserWho %s %s %d %d\n", szUserName, BoolStr(mask(iUserStatus, LOGIN_IDLE)), iTimeOn, iTimeIdle);
            if(iListType != 4 && iListType != 5) // && mask(iUserStatus, LOGIN_IDLE) == true)
            {
               pTable->SetFlag(mask(iUserStatus, LOGIN_IDLE) == true ? 'I' : ' ');
            }
            if(iListType != 8) // && mask(iUserStatus, LOGIN_BUSY) == true)
            {
               pTable->SetFlag(mask(iUserStatus, LOGIN_BUSY) == true ? 'B' : ' ');
            }
            if(mask(iUserStatus, LOGIN_TALKING) == true)
            {
               pTable->SetFlag('T');
            }
            if(mask(iUserStatus, LOGIN_SILENT) == true)
            {
               pTable->SetFlag('S');
            }
            if(mask(iUserStatus, LOGIN_SHADOW) == true)
            {
               pTable->SetFlag('H');
            }
            if(mask(iUserStatus, LOGIN_NOCONTACT) == true)
            {
               pTable->SetFlag('N');
            }
            if(bSecure == true)
            {
               pTable->SetFlag('E');
            }
            // printf("CmdUserWho silent flag %s\n", BoolStr(mask(iUserStatus, LOGIN_SILENT)));

            if(iListType == 3)
            {
               pTable->SetValue(RETRO_NAME(szUserName), AccessColour(iUserLevel, iUserType));
            }
            else
            {
               sprintf(szTime, "%d:%02d", iTimeVal / 3600, (iTimeVal / 60) % 60);
               pTable->SetValue(szTime);

               /* if(iUserType == USERTYPE_AGENT)
               {
                  pTable->SetValue("");
               }
               else */
               {
                  pTable->SetValue(mask(iUserStatus, LOGIN_ON) == true ? szUserAccess : "", AccessColour(iUserLevel, iUserType));
               }
               if(iListType == 8 || iListType == 11)
               {
                  if(szField1 == NULL || szField1[0] == '\0')
                  {
                     pTable->SetValue(RETRO_NAME(szUserName), cColour, iExtra);
                  }
                  else
                  {
                     char *szBusy = new char[strlen(szUserName) + 2 + strlen(szField1) + 100];
                     debug(DEBUGLEVEL_DEBUG, "CmdUserWho busy string %p\n", szBusy);
                     if(szField1[0] == ':')
                     {
                        sprintf(szBusy, "\037%c%s\0370%s%s", AccessColour(iUserLevel, iUserType), RETRO_NAME(szUserName), (strlen(szField1) >= 2 && strnicmp(szField1, ":'s", 3) == 0) ? "" : " ", (char *)&szField1[1]);
                     }
                     else
                     {
                        sprintf(szBusy, "\037%c%s\0370. %s", AccessColour(iUserLevel, iUserType), RETRO_NAME(szUserName), szField1);
                     }
                     pTable->SetValue(szBusy, '\0', 4);
                     delete[] szBusy;
                  }
               }
               else
               {
                  pTable->SetValue(RETRO_NAME(szUserName), iExtra > 0 ? '\0' : AccessColour(iUserLevel, iUserType), iExtra);

                  if(iAccessLevel >= LEVEL_WITNESS)
                  {
                     TrimFields(pTable, CmdWidth() - 14 - iNameLen - iAccessLen - iMaxFlags, szField1, szField2);
                  }
                  else
                  {
                     pTable->SetValue(szField1);
                  }
               }
            }
         }

         delete[] szUserName;
         delete[] szUserAccess;
         delete[] szField1;
         delete[] szField2;

         // printf("CmdUserWho flush\n");
         // pTable->Flush();
         iNumRows++;
      }

      delete[] szType;

      bLoop = pReply->Next(NULL);
   }

   debug(DEBUGLEVEL_INFO, "CmdUserWho footer\n");
   pTable->AddFooter(iNumRows, "user");
   if(iAccessLevel >= LEVEL_WITNESS && iNumLogins < iNumRows)
   {
      pTable->AddFooter(iNumLogins, "Logged in");
   }
   if(iListType == 7)
   {
      pTable->AddFooter(iHidden, "hidden");
   }
   pTable->AddKey('I', "Idle");
   pTable->AddKey('B', "Busy");
   pTable->AddKey('T', "Talking");
   pTable->AddKey('S', "Silent");
   pTable->AddKey('H', "sHadow");
   pTable->AddKey('N', "No-contact");
   pTable->AddKey('E', "sEcure");

   debug(DEBUGLEVEL_DEBUG, "CmdUserWho table delete\n");

   delete pTable;

   debug(DEBUGLEVEL_INFO, "CmdUserWho exit\n");
}

void CmdUserLast(EDF *pReply)
{
   STACKTRACE
   int iAccessLevel = LEVEL_NONE, iNumUsers = 0, iCurrent = 0, iFound = 0, iHidden = 0;
   char szTime[64];
   // struct tm tmTime;
   time_t tTemp = 0;
   bool bLoop = false, bRetro = false;
   int iTimeOn = 0, iUserStatus = LOGIN_OFF, iUserLevel = LEVEL_NONE, iUserType = 0, iHideTime = CmdInput::MenuTime() - 30 * 24 * 3600;
   char *szUserName = NULL, *szHostname = NULL, *szLocation = NULL;
   CmdTable *pTable = NULL;

   debug(DEBUGLEVEL_INFO, "CmdUserLast entry\n");

   m_pUser->GetChild("accesslevel", &iAccessLevel);
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      bRetro = CmdRetroNames(m_pUser);
      m_pUser->Parent();
   }

   // pReply->Root();
   pReply->GetChild("found", &iNumUsers);

   debug(DEBUGLEVEL_DEBUG, "CmdUserLast header\n");
   pTable = new CmdTable(m_pUser, 1, 3, false);
   pTable->AddHeader("Time", 17);
   pTable->AddHeader("Name", UA_NAME_LEN);
   pTable->AddHeader(iAccessLevel >= LEVEL_WITNESS ? "Host / Location" : "Location");

   // pReply->Sort("user", "timeon", false, false, "login");
   pReply->SortReset(NULL);
   // pReply->SortAddSection("user");
   pReply->SortAddSection("login");
   pReply->SortAddKey("timeon", false);
   pReply->Sort();

   debug(DEBUGLEVEL_DEBUG, "CmdUserLast content %d\n", iNumUsers);
   bLoop = pReply->Child("user");
   while(bLoop == true)
   {
      iUserType = 0;
      iUserStatus = LOGIN_OFF;

      // debug("CmdUserLast row %d\n", iUserNum);
      pReply->GetChild("name", &szUserName);
      pReply->GetChild("accesslevel", &iUserLevel);
      pReply->GetChild("usertype", &iUserType);

      if(pReply->Child("login") == true)
      {
         szLocation = NULL;
         szHostname = NULL;

         pReply->GetChild("timeon", &iTimeOn);
         tTemp = iTimeOn;
         pReply->GetChild("status", &iUserStatus);
         if(pReply->GetChild("hostname", &szHostname) == false)
         {
            pReply->GetChild("address", &szHostname);
         }
         if(szHostname == NULL)
         {
            szHostname = strmk("");
         }
         pReply->GetChild("location", &szLocation);
         if(szLocation == NULL)
         {
            szLocation = strmk("Unknown");
         }

         if(mask(iUserStatus, LOGIN_ON) == true)
         {
            iCurrent++;
         }

         if(iTimeOn >= iHideTime)
         {
            // memcpy(&tmTime, localtime(&tTemp), sizeof(// struct tm));
            // strftime(szTime, sizeof(szTime), "%d/%m/%y %H:%M:%S", &tmTime);
            StrTime(szTime, STRTIME_SHORT, tTemp);

            pTable->SetFlag(mask(iUserStatus, LOGIN_ON) == true ? 'C' : ' ');
            pTable->SetValue(szTime);
            pTable->SetValue(RETRO_NAME(szUserName), AccessColour(iUserLevel, iUserType));
            if(iAccessLevel >= LEVEL_WITNESS)
            {
               TrimFields(pTable, CmdWidth() - 42, szHostname, szLocation);
            }
            else
            {
               pTable->SetValue(szLocation);
            }
         }
         else
         {
            iHidden++;
         }

         delete[] szHostname;
         delete[] szLocation;

         pReply->Parent();

         iFound++;
      }

      delete[] szUserName;

      bLoop = pReply->Next("user");
   }

   debug(DEBUGLEVEL_DEBUG, "CmdUserLast footer\n");
   // Footer(pUser, false, "login", iFound, "Current", iCurrent, NULL);
   pTable->AddFooter(iFound, "login");
   pTable->AddFooter(iCurrent, "current");
   if(iHidden > 0)
   {
      pTable->AddFooter(iFound - iHidden, "shown");
      pTable->AddFooter(iHidden, "hidden");
   }

   pTable->AddKey('C', "Current");

   delete pTable;

   debug(DEBUGLEVEL_INFO, "CmdUserLast exit\n");
}

void CmdUserLogoutList(EDF *pReply)
{
   STACKTRACE
   bool bLoop = false;
   char szWrite[200], *szName = NULL, *szText = NULL;

   debug(DEBUGLEVEL_INFO, "CmdUserLogoutList entry\n");

   CmdPageOn();

   debug(DEBUGLEVEL_DEBUG, "CmdUserLogoutlist post row\n");

   bLoop = pReply->Child("user");
   while(bLoop == true)
   {
      szName = NULL;
      szText = NULL;

      pReply->GetChild("name", &szName);
      pReply->GetChild("text", &szText);
      // debug("CmdUserLogoutList %s, %s\n", szName, szText);

      if(szName != NULL && szText != NULL)
      {
         strcpy(szWrite, "\0373");
         strcat(szWrite, szName);
         strcat(szWrite, "\0370");
         if(szText[0] == ':')
         {
            strcat(szWrite, " ");
            strcat(szWrite, (char *)&szText[1]);
         }
         else
         {
            strcat(szWrite, " has logged out. ");
            strcat(szWrite, szText);
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);
      }

      delete[] szName;
      delete[] szText;

      bLoop = pReply->Next("user");
   }

   CmdPageOff();

   debug(DEBUGLEVEL_INFO, "CmdUserLogoutList exit\n");
}

void CmdUserViewDetail(EDF *pReply, char *szName, char *szTitle)
{
   int iType = 0, iComma = 0;
   char *szValue = NULL;
   char szWrite[200];

   if(pReply->Child(szName) == true)
   {
      pReply->Get(NULL, &szValue);
      pReply->GetChild("type", &iType);

      if(szValue != NULL && strlen(szValue) > 0)
      {
         sprintf(szWrite, "%s:%*s\0373%s\0370", szTitle, 10 - strlen(szTitle), "", szValue);
         if(mask(iType, DETAIL_PUBLIC) == true || mask(iType, DETAIL_VALID) == false || mask(iType, DETAIL_OWNER) == true)
         {
            strcat(szWrite, " (");
            iComma = strlen(szWrite);
            if(mask(iType, DETAIL_PUBLIC) == true)
            {
               sprintf(szWrite, "%s%s\0373public\0370", szWrite, strlen(szWrite) > iComma ? " / " : "");
            }
            if(mask(iType, DETAIL_VALID) == false)
            {
               sprintf(szWrite, "%s%s\0373not validated\0370", szWrite, strlen(szWrite) > iComma ? " / " : "");
            }
            if(mask(iType, DETAIL_OWNER) == true)
            {
               sprintf(szWrite, "%s%s\0373from owner\0370", szWrite, strlen(szWrite) > iComma ? " / " : "");
            }
            strcat(szWrite, ")");
            // sprintf(szWrite, "%s (type \0373%d\0370)", szWrite, iType);
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);
         delete[] szValue;
      }

      pReply->Parent();
   }
}

void CmdUserView(EDF *pReply)
{
   STACKTRACE
   int iUserID = -1, iAccessLevel = LEVEL_NONE, iDays = 0, iLastMsg = -1, iFolderNav = NAV_ALPHATREE;
   int iStatus = 0, iGender = 0, iIdle = 0, iCreated = 0, iUserLevel = LEVEL_NONE;
   int iSubs = 0, iAnnType = 0, iBusy = 0, iMarking = 0, iConfirm = 0, iWholist = -1;
   int iTimeOn = 0, iTimeOff = 0, iTotalLogins = 0, iNumLogins = 0, iLongestLogin = 0, iAvgLogin = 0, iNumMsgs = 0, iTotalMsgs = 0;
   int iWidth = 80, iHeight = 25, iHighlight = 0, iNumVotes = 0, iValue = 0, iMenuLevel = 0, iHiddenTime = 0, iUserType = USERTYPE_NONE, iRetro = 0;
   int iRequests = 0, iAnnounces = 0, iLoginLen = 0;
   bool bRetro = false, bDevOption = false, bAddMarking = false, bAccessNames = true, bMenuCase = false, bFakePage = false, bSlimHeaders = false;
   bool bLoop = false, bFirst = false;
   char szWrite[300], szTime[64];
   char *szDesc = NULL, *szName = NULL, *szHostname = NULL, *szProxyHostname = NULL, *szLocation = NULL, *szAgent = NULL;
   char *szClient = NULL, *szAccessName = NULL, *szOwner = NULL, *szProtocol = NULL, *szAddress = NULL, *szProxyAddress = NULL;
   char *szLoginType = NULL;

   // debugEDFPrint("CmdUserView entry", pReply, false);
   debug(DEBUGLEVEL_INFO, "CmdUserView entry\n");
   // EDFPrint("CmdUserView", pReply);

   m_pUser->GetChild("accesslevel", &iAccessLevel);
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      bAccessNames = m_pUser->GetChildBool("accessnames", true);
      bDevOption = m_pUser->GetChildBool("devoption");
      m_pUser->GetChild("retro", &iRetro);
      bRetro = mask(iRetro, RETRO_NAMES);
      bFakePage = m_pUser->GetChildBool("fakepage");
      bSlimHeaders = m_pUser->GetChildBool("slimheaders");
      m_pUser->Parent();
   }

   pReply->Get(NULL, &iUserID);
   pReply->GetChild("name", &szName);
   pReply->GetChild("usertype", &iUserType);
   pReply->GetChild("ownername", &szOwner);
   pReply->GetChild("gender", &iGender);

   if(pReply->Child("folders") == true)
   {
      pReply->GetChild("marking", &iMarking);
      pReply->Parent();
   }

   strcpy(szWrite, "");
   sprintf(szWrite, "Name:         \0373%s\0370\n", RETRO_NAME(szName));
   if(iAccessLevel >= LEVEL_WITNESS || bDevOption == true)
   {
      sprintf(szWrite, "%sID:           \0373%d\0370\n", szWrite, iUserID);
   }
   if(iUserType == USERTYPE_AGENT)
   {
      pReply->GetChild("agent", &szAgent);
   }
   else
   {
      pReply->GetChild("accesslevel", &iUserLevel);
      if(bAccessNames == true)
      {
         pReply->GetChild("accessname", &szAccessName);
      }
   }
   CmdWrite(szWrite);

   if(mask(iUserType, USERTYPE_DELETED) == true)
   {
      strcpy(szWrite, "Access:       \0371** DELETED **\0370\n");
   }
   else if(iUserType == USERTYPE_AGENT)
   {
      sprintf(szWrite, "Access:       \037%cAgent\0370", AccessColour(iUserLevel, iUserType));
      if(szAgent != NULL)
      {
         sprintf(szWrite, "%s (client \0373%s\0370)", szWrite, szAgent);
         delete[] szAgent;
      }
      strcat(szWrite, "\n");
   }
   else
   {
      sprintf(szWrite, "Access level: \037%c%s\0370", AccessColour(iUserLevel, 0), szAccessName != NULL ? szAccessName : AccessName(iUserLevel));
      if(szAccessName != NULL)
      {
         sprintf(szWrite, "%s (level \037%c%s\0370)", szWrite, AccessColour(iUserLevel, 0), AccessName(iUserLevel));
      }
      strcat(szWrite, "\n");
   }
   CmdWrite(szWrite);
   if(pReply->Child(CmdVersion("2.5") >= 0 ? "privilege" : "proxy") == true)
   {
      CmdEDFPrint("Privileges", pReply, false, false);
      CmdWrite("\n");

      pReply->Parent();
   }

   if(iGender > 0)
   {
      sprintf(szWrite, "Gender:       \0373%s\0370\n", GenderType(iGender));
      CmdWrite(szWrite);
   }
   if(szOwner != NULL)
   {
      sprintf(szWrite, "Owner:        \0373%s\0370\n", RETRO_NAME(szOwner));
      CmdWrite(szWrite);
   }

   pReply->GetChild("created", &iCreated);

   if(pReply->GetChild("description", &szDesc) == true && szDesc != NULL)
   {
      CmdWrite("\n");
      debug("CmdUserView desc '%s'\n", szDesc);
      CmdCentre(szDesc, '4');
      delete[] szDesc;
   }
   CmdWrite("\n");

   if(pReply->Child("stats") == true)
   {
      pReply->GetChild("numlogins", &iNumLogins);
      pReply->GetChild("totallogins", &iTotalLogins);
      pReply->GetChild("longestlogin", &iLongestLogin);

      pReply->GetChild("nummsgs", &iNumMsgs);
      pReply->GetChild("totalmsgs", &iTotalMsgs);
      pReply->GetChild("lastmsg", &iLastMsg);

      pReply->GetChild("numvotes", &iNumVotes);

      pReply->Parent();
   }

   bLoop = pReply->Child("login");
   while(bLoop == true)
   {
      iRequests = 0;
      iAnnounces = 0;
      iTimeOn = 0;
      iTimeOff = 0;
      iIdle = 0;
      iBusy = 0;
      iStatus = 0;
      szLocation = NULL;
      szAddress = NULL;
      szHostname = NULL;
      szProxyAddress = NULL;
      szProxyHostname = NULL;
      szClient = NULL;
      szProtocol = NULL;

      pReply->GetChild("requests", &iRequests);
      pReply->GetChild("announces", &iAnnounces);
      pReply->GetChild("timeon", &iTimeOn);
      pReply->GetChild("timeoff", &iTimeOff);
      pReply->GetChild("timeidle", &iIdle);
      pReply->GetChild("timebusy", &iBusy);
      pReply->GetChild("status", &iStatus);
      pReply->GetChild("location", &szLocation);
      if(szLocation == NULL)
      {
         szLocation = strmk("Unknown");
      }
      if(CmdVersion("2.5") >= 0)
      {
         if(pReply->GetChild("hostname", &szHostname) == true)
         {
            pReply->GetChild("address", &szAddress);
         }
         else
         {
            pReply->GetChild("address", &szHostname);
         }
         if(pReply->GetChild("proxyhostname", &szProxyHostname) == true)
         {
            pReply->GetChild("proxyaddress", &szProxyAddress);
         }
         else
         {
            pReply->GetChild("proxyaddress", &szProxyHostname);
         }
      }
      else
      {
         if(pReply->Child("hostname") == true)
         {
            pReply->Get(NULL, &szHostname);
            pReply->GetChild("hostname", &szProxyHostname);
            pReply->Parent();
         }
      }
      pReply->GetChild("client", &szClient);
      pReply->GetChild("protocol", &szProtocol);

      iValue = CmdInput::MenuTime() - iTimeOn;
      StrTime(szTime, STRTIME_SHORT, iTimeOn, '3');
      if(mask(iStatus, LOGIN_ON) == true)
      {
         if(mask(iStatus, LOGIN_SHADOW) == false)
         {
            szLoginType = "Current";
            iLoginLen = 0;
         }
         else
         {
            szLoginType = "Shadow";
            iLoginLen = 1;
         }
      }
      else
      {
         szLoginType = "Last";
         iLoginLen = 3;
      }

      sprintf(szWrite, "%s login: %*s%s", szLoginType, iLoginLen, "", szTime);
      if(mask(iStatus, LOGIN_ON) == true)
      {
         if(iIdle > 0)
         {
            iIdle = CmdInput::MenuTime() - iIdle;
            sprintf(szWrite, "%s, idle \0373%02d:%02d:%02d\0370",
               szWrite, iIdle / 3600, (iIdle / 60) % 60, iIdle % 60);
         }
         if(iBusy > 0)
         {
            iBusy = CmdInput::MenuTime() - iBusy;
            sprintf(szWrite, "%s, busy \0373%02d:%02d:%02d\0370",
               szWrite, iBusy / 3600, (iBusy / 60) % 60, iBusy % 60);
         }
      }
      else if(iTimeOff > 0 && iTimeOff > iTimeOn)
      {
         iValue = iTimeOff - iTimeOn;
         strcat(szWrite, " for ");
         StrValue(szTime, iValue > 60 ? STRVALUE_HM : STRVALUE_TIMESU, iValue, '3');
         strcat(szWrite, szTime);
      }
      strcat(szWrite, "\n");
      CmdWrite(szWrite);

      if(szHostname != NULL)
      {
         sprintf(szWrite, "%*sfrom \0373%s\0370", 15, "", szHostname);
         if(szAddress != NULL)
         {
            sprintf(szWrite, "%s / \0373%s\0370", szWrite, szAddress);
         }
         if(szProxyHostname != NULL)
         {
            sprintf(szWrite, "%s\n%*s(", szWrite, 15, "");
            if(szLocation != NULL)
            {
               sprintf(szWrite, "%s\0373%s\0370, ", szWrite, szLocation);
            }
            sprintf(szWrite, "%sproxy \0373%s\0370", szWrite, szProxyHostname);
            if(szProxyAddress != NULL)
            {
               sprintf(szWrite, "%s / \0373%s\0370", szWrite, szProxyAddress);
            }
            strcat(szWrite, ")");
         }
         else if(szLocation != NULL)
         {
            sprintf(szWrite, "%s (\0373%s\0370)", szWrite, szLocation);
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);

         if(szClient != NULL && strlen(szClient) > 0)
         {
            sprintf(szWrite, "%*s", 15, "");
            sprintf(szWrite, "%susing \0373%s\0370", szWrite, szClient);
            if(szProtocol != NULL && strlen(szProtocol) > 0)
            {
               sprintf(szWrite, "%s (protocol \0373%s\0370)", szWrite, szProtocol);
            }
            strcat(szWrite, "\n");
            CmdWrite(szWrite);
         }
      }
      else if(szLocation != NULL)
      {
         sprintf(szWrite, "%*sfrom \0373%s\0370\n", 15, "", szLocation);
         CmdWrite(szWrite);
      }

      if(mask(iStatus, LOGIN_ON) == true && (iRequests > 0 || iAnnounces > 0))
      {
         sprintf(szWrite, "%*s", 15, "");
         sprintf(szWrite, "%s\0373%d\0370 request%s / \0373%d\0370 announcement%s", szWrite, iRequests, iRequests != 1 ? "s" : "", iAnnounces, iAnnounces != 1 ? "s" : "");
         strcat(szWrite, "\n");
         CmdWrite(szWrite);
      }

      bLoop = pReply->Next("login");
      if(bLoop == false)
      {
         pReply->Parent();
      }
   }

   bFirst = true;
   bLoop = pReply->Child("oldlogin");
   while(bLoop == true)
   {
      pReply->GetChild("timeon", &iTimeOn);
      pReply->GetChild("timeoff", &iTimeOff);

      if(bFirst == true)
      {
         strcpy(szWrite, "\nOld logins:    ");
      }
      else
      {
         strcpy(szWrite, "               ");
      }
      StrTime(szTime, STRTIME_SHORT, iTimeOn, '3');
      strcat(szWrite, szTime);
      strcat(szWrite, " to ");
      StrTime(szTime, STRTIME_SHORT, iTimeOff, '3');
      strcat(szWrite, szTime);
      strcat(szWrite, "\n");
      CmdWrite(szWrite);

      bLoop = pReply->Next("oldlogin");
      if(bLoop == false)
      {
         pReply->Parent();
      }

      bFirst = false;
   }

   if(iNumLogins > 0)
   {
      sprintf(szWrite, "Logins:     %s\0373%d\0370 (total time ",
         mask(iStatus, LOGIN_ON) == true ? "   " : "", iNumLogins);
      // CmdWrite(szWrite);

      iAvgLogin = iTotalLogins / iNumLogins;
      iDays = iTotalLogins / 86400;
      if(iDays > 0)
      {
         sprintf(szWrite, "%s\0373%d\0370 day%s ", szWrite, iDays, iDays != 1 ? "s" : "");
         iTotalLogins = iTotalLogins % 86400;
      }
      sprintf(szWrite, "%s\0373%d:%02d:%02d\0370,\n", szWrite, iTotalLogins / 3600, (iTotalLogins / 60) % 60, iTotalLogins % 60);
      CmdWrite(szWrite);

      sprintf(szWrite, "            %slongest \0373%d:%02d:%02d\0370, average \0373%d:%02d:%02d\0370)\n",
         mask(iStatus, LOGIN_ON) == true ? "   " : "",
         iLongestLogin / 3600, (iLongestLogin / 60) % 60, iLongestLogin % 60,
         iAvgLogin / 3600, (iAvgLogin / 60) % 60, iAvgLogin % 60);
      CmdWrite(szWrite);
   }

   if(pReply->Child("details") == true)
   {
      CmdWrite("\n");

      CmdUserViewDetail(pReply, "realname", "Real name");
      CmdUserViewDetail(pReply, "email", "Email");
      CmdUserViewDetail(pReply, "sms", "SMS Email");
      CmdUserViewDetail(pReply, "homepage", "Homepage");
      CmdUserViewDetail(pReply, "refer", "Referal");

      CmdWrite("\n");

      pReply->Parent();
   }

   if(iCreated > 0)
   {
      // tmTime = localtime((time_t *)&iCreated);
      // strftime(szWrite, 100, "Created:   \0373%A, %d %B, %Y - %H:%M\0370", tmTime);
      StrTime(szWrite, STRTIME_LONG, iCreated, '3', "Created:   ", "\n\n");
      CmdWrite(szWrite);
   }

   if(pReply->Child("client", CLIENT_NAME()) == true)
   {
      CmdWrite("\n");
      pReply->GetChild("anntype", &iAnnType);
      if(iAnnType > 0)
      {
         strcpy(szWrite, "Alerts:     ");
         if(mask(iAnnType, ANN_PAGEBELL) == true)
         {
            sprintf(szWrite, "%s%s\0373page bell\0370", szWrite, strlen(szWrite) > 12 ? ", " : "");
         }
         if(mask(iAnnType, ANN_FOLDERCHECK) == true)
         {
            sprintf(szWrite, "%s%s\0373folder\0370", szWrite, strlen(szWrite) > 12 ? ", " : "");
         }
         if(mask(iAnnType, ANN_USERCHECK) == true)
         {
            sprintf(szWrite, "%s%s\0373user\0370", szWrite, strlen(szWrite) > 12 ? ", " : "");
         }
         if(mask(iAnnType, ANN_ADMINCHECK) == true)
         {
            sprintf(szWrite, "%s%s\0373admin\0370", szWrite, strlen(szWrite) > 12 ? ", " : "");
         }
         if(mask(iAnnType, ANN_DEBUGCHECK) == true)
         {
            sprintf(szWrite, "%s%s\0373debug\0370", szWrite, strlen(szWrite) > 12 ? ", " : "");
         }
         if(mask(iAnnType, ANN_BUSYCHECK) == true)
         {
            sprintf(szWrite, "%s%s\0373busy\0370", szWrite, strlen(szWrite) > 12 ? ", " : "");
         }
         if(bFakePage == true)
         {
            sprintf(szWrite, "%s%s\0373instant private\0370", szWrite, strlen(szWrite) > 12 ? ", " : "");
         }
         strcat(szWrite, "\n");
         CmdWrite(szWrite);
      }
      strcpy(szWrite, "Screen:     ");
      if(CmdLocal() == false)// || m_bScreenSize == true)
      {
         pReply->GetChild("width", &iWidth);
         pReply->GetChild("height", &iHeight);
         sprintf(szWrite, "%s\0373%d\0370 x \0373%d\0370, ", szWrite, iWidth, iHeight);
      }
      strcat(szWrite, "\0373");
      pReply->GetChild("highlight", &iHighlight);
      if(iHighlight == 3)
      {
         strcat(szWrite, "custom colour");
      }
      else if(iHighlight == 2)
      {
         strcat(szWrite, "full colour");
      }
      else if(iHighlight == 1)
      {
         strcat(szWrite, "bold colour");
      }
      else
      {
         strcat(szWrite, "no colour");
      }
      strcat(szWrite, "\0370\n");
      CmdWrite(szWrite);

      pReply->GetChild("menulevel", &iMenuLevel);
      bMenuCase = pReply->GetChildBool("menucase");
      strcpy(szWrite, "Menus:      \0373");
      if(iMenuLevel == 2)
      {
         strcat(szWrite, "Expert");
      }
      else if(iMenuLevel == 1)
      {
         strcat(szWrite, "Intermediate");
      }
      else
      {
         strcat(szWrite, "Expert");
      }
      strcat(szWrite, "\0370");
      if(bMenuCase == true)
      {
         strcat(szWrite, ", \0373case on\0370");
      }
      strcat(szWrite, "\n");
      CmdWrite(szWrite);

      pReply->GetChild("retro", &iRetro);
      strcpy(szWrite, "Retro mode: ");
      if(iRetro == 0)
      {
         strcat(szWrite, "\0373off\0370");
      }
      else
      {
         if(mask(iRetro, RETRO_NAMES) == true)
         {
            strcat(szWrite, "\0373names\0370");
            if(mask(iRetro, RETRO_MENUS) == true)
            {
               strcat(szWrite, ", ");
            }
         }
         if(mask(iRetro, RETRO_MENUS) == true)
         {
            strcat(szWrite, "\0373menus\0370");
         }
      }
      strcat(szWrite, "\n");
      CmdWrite(szWrite);

      pReply->GetChild("confirm", &iConfirm);
      strcpy(szWrite, "Confirms:   ");
      if(iConfirm == 0)
      {
         strcat(szWrite, "\0373off\0370");
      }
      else
      {
         if(mask(iConfirm, CONFIRM_BUSY_PAGER) == true)
         {
            strcat(szWrite, "\0373busy pager\0370");
         }
         if(mask(iConfirm, CONFIRM_ACTIVE_REPLY) == true)
         {
            if(mask(iConfirm, CONFIRM_BUSY_PAGER) == true)
            {
               strcat(szWrite, ", ");
            }
            strcat(szWrite, "\0373active reply\0370");
         }
#ifdef HAVE_LIBASPELL
         if(mask(iConfirm, CONFIRM_SPELL_CHECK) == true)
         {
            if(mask(iConfirm, CONFIRM_BUSY_PAGER) == true || mask(iConfirm, CONFIRM_ACTIVE_REPLY) == true)
            {
               strcat(szWrite, ", ");
            }
            strcat(szWrite, "\0373spell check\0370");
         }
#endif HAVE_LIBASPELL
      }
      strcat(szWrite, "\n");
      CmdWrite(szWrite);

      pReply->GetChild("wholist", &iWholist);
      if(iWholist != -1)
      {
         strcpy(szWrite, "Wholist:    \0373");
         switch(iWholist)
         {
            case 0:
               strcat(szWrite, "Sort by time on");
               break;

            case 1:
               strcat(szWrite, "Sort by access level");
               break;

            case 2:
               strcat(szWrite, "Sort by name");
               break;

            case 3:
               strcat(szWrite, "Show as name list");
               break;

            case 4:
               strcat(szWrite, "Show only idle users");
               break;

            case 5:
               strcat(szWrite, "Show only active users");
               break;

            case 6:
               strcat(szWrite, "Show client / protocol");
               break;

            case 7:
               strcat(szWrite, "Default");
               break;

            case 8:
               strcat(szWrite, "Show only busy users");
               break;

            /* case 9:
               strcat(szWrite, "sort by Time on");
               break; */

            case 10:
               strcat(szWrite, "Show only proxy logins");
               break;
         }
         strcat(szWrite, "\0370\n");
         CmdWrite(szWrite);
      }

      pReply->GetChild("foldernav", &iFolderNav);
      strcpy(szWrite, "Folder nav: \0373");
      switch(iFolderNav)
      {
         case NAV_LEVEL:
            strcat(szWrite, "sub level (editor, member, private, subscriber)");
            break;

         case NAV_PRIORITIES:
            if(CmdVersion("2.5") >= 0)
            {
               strcat(szWrite, "Priorities (specifics then as sub level)");
            }
            break;

         default:
            strcat(szWrite, "Alphabetical with hierarchy");
            break;
      }
      strcat(szWrite, "\0370\n");
      CmdWrite(szWrite);

      strcpy(szWrite, "Marking:    ");
      if(mask(iMarking, MARKING_ADD_PRIVATE) == true || mask(iMarking, MARKING_ADD_PUBLIC) == true)
      {
         sprintf(szWrite, "%s\0373read on add\0370 (\0373%s\0370%s\0373%s\0370)", szWrite,
            mask(iMarking, MARKING_ADD_PRIVATE) == true ? "private" : "",
            mask(iMarking, MARKING_ADD_PRIVATE) == true && mask(iMarking, MARKING_ADD_PUBLIC) == true ? "/" : "",
            mask(iMarking, MARKING_ADD_PUBLIC) == true ? "public" : "");

         bAddMarking = true;
      }
      if(mask(iMarking, MARKING_EDIT_PRIVATE) == true || mask(iMarking, MARKING_EDIT_PUBLIC) == true)
      {
         sprintf(szWrite, "%s%s\0373unread on edit\0370 (\0373%s\0370%s\0373%s\0370)", szWrite, bAddMarking == true ? ", " : "",
            mask(iMarking, MARKING_EDIT_PRIVATE) == true ? "private" : "",
            mask(iMarking, MARKING_EDIT_PRIVATE) == true && mask(iMarking, MARKING_EDIT_PUBLIC) == true ? "/" : "",
            mask(iMarking, MARKING_EDIT_PUBLIC) == true ? "public" : "");
      }
      strcat(szWrite, "\n");
      CmdWrite(szWrite);

      pReply->GetChild("idlehide", &iHiddenTime);
      if(iHiddenTime > 0)
      {
         sprintf(szWrite, "\nIdle hide time: \0373%d\0370 minutes\n", iHiddenTime / 60);
         CmdWrite(szWrite);
      }

      pReply->Parent();
   }

   if(pReply->Child("folders") == true)
   {
      // Folder info

      CmdWrite("\n");

      // EDFPrint("CmdUserView folder info", pReply, false);

      iSubs = CmdSubList(pReply, SUBTYPE_EDITOR, SUBINFO_USER, NULL, bRetro);
      iSubs += CmdSubList(pReply, SUBTYPE_MEMBER, SUBINFO_USER, NULL, bRetro);
      iSubs += CmdSubList(pReply, SUBTYPE_SUB, SUBINFO_USER | SUBINFO_TOTAL, NULL, bRetro);

      pReply->Parent();
   }

   if(iNumMsgs > 0)
   {
      if(iLastMsg != -1)
      {
         StrTime(szTime, STRTIME_SHORT, iLastMsg, '3', ", last at ", NULL);
      }
      else
      {
         strcpy(szTime, "");
      }
      sprintf(szWrite, "Messages: \0373%d\0370 (total \0373%d\0370 bytes%s)", iNumMsgs, iTotalMsgs, szTime);
      CmdWrite(szWrite);

      if(iNumVotes > 0)
      {
         sprintf(szWrite, ", \0373%d\0370 vote%s", iNumVotes, iNumVotes != 1 ? "s" : "");
         CmdWrite(szWrite);
      }
      CmdWrite("\n");
   }

   if(iSubs > 0 || iNumMsgs > 0)
   {
      CmdWrite("\n");
   }

   delete[] szName;
   delete[] szHostname;
   delete[] szAddress;
   delete[] szProxyHostname;
   delete[] szProxyAddress;
   delete[] szLocation;
   delete[] szClient;
   delete[] szProtocol;
   delete[] szOwner;

   debug(DEBUGLEVEL_INFO, "CmdUserView exit\n");
}

/* void CmdUserFolder(EDF *pUser, EDF *pReply, bool bRetro)
{
   STACKTRACE
   int iSubs = 0;

   if(pReply->Child("folders") == true)
   {
      iSubs = CmdSubList(pReply, FOLDER_SUBNAME_EDITOR, 0, -1, bRetro);
      iSubs += CmdSubList(pReply, FOLDER_SUBNAME_MEMBER, 0, -1, bRetro);
      iSubs += CmdSubList(pReply, FOLDER_SUBNAME_SUB, 0, -1, bRetro);
   }

   if(iSubs == 0)
   {
      CmdWrite("No folder rights");
   }
   CmdWrite("\n");
} */

void CmdUserStats(EDF *pReply)
{
   STACKTRACE
   int iNumUsers = 0, iAccessLevel = LEVEL_NONE, iValue = 0, iDays = 0, iDate = 0, iMaxLogins = -1, iMaxLoginsDate = 0;
   int iAll = 0, iAllTotal = 0, iAllAverage = 0;
   bool bRetro = false;
   // double dValue = 0;
   char szWrite[200], szDate[100], *szUserName = NULL, szValue[100];
   // struct tm *tmDate = NULL;

   debug(DEBUGLEVEL_INFO, "CmdUserStats entry\n");
   // EDFPrint(pReply);

   m_pUser->GetChild("accesslevel", &iAccessLevel);
   if(m_pUser->Child("client", CLIENT_NAME()) == true)
   {
      bRetro = CmdRetroNames(m_pUser);
      m_pUser->Parent();
   }

   pReply->GetChild("numusers", &iNumUsers);
   sprintf(szWrite, "Users:  \0373%d\0370\n\n", iNumUsers);
   CmdWrite(szWrite);
   /* if(pReply->GetChild("totalalllogins", &iAllLogins) == true)
   {
      debug("CmdUserStats all logins %d\n", iAllLogins);
   }
   else
   {
      debug("CmdUserStats no all logins\n");
   } */

   STACKTRACEUPDATE

   if(iAccessLevel >= LEVEL_WITNESS)
   {
		if(pReply->Child("maxlogins") == true)
		{
			pReply->Get(NULL, &iMaxLogins);
			pReply->GetChild("date", &iMaxLoginsDate);
			pReply->Parent();
		}
		STACKTRACEUPDATE

		if(pReply->Child("totalstats") == true)
		{
			pReply->GetChild("numlogins", &iAll);
			pReply->GetChild("totallogins", &iAllTotal);
			pReply->GetChild("averagelogin", &iAllAverage);
			pReply->GetChild("nummsgs", &iAll);
			pReply->GetChild("totalmsgs", &iAllTotal);
			pReply->GetChild("averagemsg", &iAllAverage);

			pReply->Parent();
		}

		if(pReply->Child("recordstats") == true)
		{
			if(pReply->Child("numlogins") == true)
			{
				szUserName = NULL;

				pReply->Get(NULL, &iValue);
				pReply->GetChild("username", &szUserName);
				sprintf(szWrite, "Logins: most \0373%d\0370 by \0373%s\0370,\n", iValue, RETRO_NAME(szUserName));
				CmdWrite(szWrite);
				delete[] szUserName;
				pReply->Parent();
			}
			STACKTRACEUPDATE
			if(pReply->Child("totallogins") == true)
			{
				szUserName = NULL;

				pReply->Get(NULL, &iValue);
				pReply->GetChild("username", &szUserName);
				strcpy(szWrite, "        total time ");
				iDays = iValue / 86400;
				if(iDays > 0)
				{
					sprintf(szWrite, "%s\0373%d\0370 day%s ", szWrite, iDays, iDays != 1 ? "s" : "");
					iValue = iValue % 86400;
				}
				sprintf(szWrite, "%s\0373%d:%02d:%02d\0370 by \0373%s\0370,\n", szWrite, iValue / 3600, (iValue / 60) % 60, iValue % 60, RETRO_NAME(szUserName));
				CmdWrite(szWrite);
				delete[] szUserName;
				pReply->Parent();
			}
			STACKTRACEUPDATE
			if(pReply->Child("longestlogin") == true)
			{
				szUserName = NULL;

				pReply->Get(NULL, &iValue);
				pReply->GetChild("username", &szUserName);
				sprintf(szWrite, "        longest \0373%d:%02d:%02d\0370 by \0373%s\0370,\n", iValue / 3600, (iValue / 60) % 60, iValue % 60, RETRO_NAME(szUserName));
				CmdWrite(szWrite);
				delete[] szUserName;
				pReply->Parent();
			}
			STACKTRACEUPDATE
			if(iMaxLogins != -1)
			{
				StrTime(szDate, STRTIME_SHORT, iMaxLoginsDate);
				sprintf(szWrite, "        maximum \0373%d\0370 (\0373%s\0370)\n", iMaxLogins, szDate);
				CmdWrite(szWrite);
			}
			STACKTRACEUPDATE
			if(pReply->Child("avglogin") == true)
			{
				szUserName = NULL;

				pReply->GetChild("value", &iValue);
				pReply->GetChild("username", &szUserName);
				sprintf(szWrite, "        highest average \0373%d:%02d:%02d\0370 by \0373%s\0370\n", iValue / 3600, (iValue / 60) % 60, iValue % 60, RETRO_NAME(szUserName));
				CmdWrite(szWrite);
				delete[] szUserName;
				pReply->Parent();
			}
			STACKTRACEUPDATE
			StrValue(szValue, STRVALUE_TIMESU, iAllTotal, '3');
			/* dValue = iAllTotal;
			if(dValue > 31499600)
			{
				dValue = dValue / 31499600;
				szUnits = "years";
			}
			else if(dValue > 604800)
			{
				dValue = dValue / 604800;
				szUnits = "weeks";
			}
			else if(dValue > 86400)
			{
				dValue = dValue / 86400;
				szUnits = "days";
			}
			else if(dValue > 3600)
			{
				dValue = dValue / 3600;
				szUnits = "hours";
			}
			else if(dValue > 60)
			{
				dValue = dValue / 60;
				szUnits = "minutes";
			}
			else
			{
				szUnits = "seconds";
			} */
			sprintf(szWrite, "        total time for all users %s (\0373%d\0370 logins of \0373%d:%02d:%02d\0370 average)\n", szValue, iAll, iAllAverage / 3600, (iAllAverage / 60) % 60, iAllAverage % 60);
			CmdWrite(szWrite);
			CmdWrite("\n");

			STACKTRACEUPDATE
			if(pReply->Child("nummsgs") == true)
			{
				szUserName = NULL;

				pReply->Get(NULL, &iValue);
				pReply->GetChild("username", &szUserName);
				sprintf(szWrite, "Messages: most \0373%d\0370 by \0373%s\0370,\n", iValue, RETRO_NAME(szUserName));
				CmdWrite(szWrite);
				delete[] szUserName;
				pReply->Parent();
			}
			if(pReply->Child("totalmsgs") == true)
			{
				szUserName = NULL;

				pReply->Get(NULL, &iValue);
				pReply->GetChild("username", &szUserName);
				StrValue(szValue, STRVALUE_BYTE, iValue, '3');
				sprintf(szWrite, "          total size %s by \0373%s\0370\n", szValue, RETRO_NAME(szUserName));
				CmdWrite(szWrite);
				delete[] szUserName;
				pReply->Parent();
			}
			StrValue(szValue, STRVALUE_BYTE, iAllTotal, '3');
			/* dValue = iAllTotal;
			if(dValue > 1000000)
			{
				dValue = dValue / 1000000;
				szUnits = "Mb";
			}
			else if(dValue > 1000)
			{
				dValue = dValue / 1000;
				szUnits = "Kb";
			}
			else
			{
				szUnits = "bytes";
			} */
			sprintf(szWrite, "          total size for all users %s (\0373%d\0370 messages of \0373%d\0370 bytes)\n", szValue, iAll, iAllAverage);
			CmdWrite(szWrite);
			CmdWrite("\n");
		}

		pReply->Parent();
   }

   STACKTRACEUPDATE

   CmdWrite("Wildcards to list users, name to list details, RETURN to abort\n\n");

   debug(DEBUGLEVEL_INFO, "CmdUserStats exit\n");
}

void CmdUserPageView(EDF *pPage, const char *szType, const char *szUser, const char *szDateField, const char *szTextField, bool bTopDashes)
{
   STACKTRACE
   long lDate = -1;
   char szDate[100];
   // char szWrite[1000];
   char *szSubject = NULL, *szText = NULL;

   /* if(szUserField == NULL)
   {
      szUserField = "fromname";
   }
   pPage->GetChild(szUserField, &szUser); */
   pPage->GetChild("subject", &szSubject);
   if(szTextField == NULL)
   {
      szTextField = "text";
   }
   pPage->GetChild(szTextField, &szText);

   CmdPageOn();

   if(bTopDashes == true)
   {
      CmdWrite(DASHES);
   }

   if(szDateField != NULL)
   {
      pPage->GetChild(szDateField, &lDate);
      if(lDate != -1)
      {
         strcpy(szDate, "");
         StrTime(szDate, STRTIME_TIME, lDate);
         // sprintf(szWrite, "Time: \0373%s\0370\n", szDate);
         // CmdWrite(szWrite);
         CmdField("Time", '3', szDate);
      }
   }

   if(szUser != NULL)
   {
      // sprintf(szWrite, "%s: \0373%s\0370\n", szType, szUser);
      CmdField(szType, '3', szUser);
   }
   else
   {
      // strcpy(szWrite, "");
   }
   if(szSubject != NULL)
   {
      // sprintf(szWrite, "%sSubject: \0373%s\0370\n", szWrite, szSubject);
      CmdField("Subject", '3', szSubject, CMD_OUT_NOHIGHLIGHT | CMD_OUT_UTF);
   }
   // strcat(szWrite, "\n");
   CmdWrite("\n");
   CmdWrite(szText, CMD_OUT_NOHIGHLIGHT | CMD_OUT_UTF);
   CmdWrite("\n");

   CmdContentList(pPage, true);

   CmdWrite(DASHES);

   CmdPageOff();

   // delete[] szUser;
   delete[] szText;
}

bool CmdCentre(const char *szText, char cColour)
{
   STACKTRACE
   int iLine = 0, iMaxLine = 0, iShift = 0, iPos = 0;
   char *szReturn = NULL, *szPrev = NULL, *szWrite = NULL;

   if(szText == NULL)
   {
      return false;
   }

   szReturn = (char *)szText;
   szPrev = szReturn;

   // memprint("CmdCentre text", (byte *)szText, strlen(szText), false);

   while(szReturn != NULL && *szReturn != '\0')
   {
      szReturn = streol(szPrev);

      if(szReturn == NULL)
      {
         szReturn = (char *)szText + strlen(szText);
      }
      if(szReturn != NULL)
      {
         /* if(szPrev != NULL && *szPrev == '\r')
         {
            szPrev++;
         } */

         iLine = szReturn - szPrev;
         if(iLine > iMaxLine)
         {
            iMaxLine = iLine;
         }
         // debug("CmdCentre return at %d chars past %d\n", iLine, szPrev - szText);
         szPrev = szReturn + 1;
      }
   }

   debug(DEBUGLEVEL_DEBUG, "CmdCentre max line %d\n", iMaxLine);
   if(iMaxLine < CmdWidth())
   {
      iShift = (CmdWidth() - iMaxLine) / 2;
      // debug("CmdCentre right shift %d\n", iShift);

      szWrite = new char[CmdWidth() + 10];

      szReturn = (char *)szText;
      szPrev = szReturn;

      while(szReturn != NULL && *szReturn != '\0')
      {
         szReturn = streol(szPrev);

         if(szReturn == NULL)
         {
            szReturn = (char *)szText + strlen(szText);
         }
         if(szReturn != NULL)
         {
            /* if(*szPrev == '\r')
            {
               szPrev++;
            } */

            iLine = szReturn - szPrev;
            memset(szWrite, ' ', iShift);
            iPos = iShift;
            if(cColour != '\0')
            {
               szWrite[iPos++] = '\037';
               szWrite[iPos++] = cColour;
            }
            strncpy(szWrite + iPos, szPrev, iLine);
            iPos += iLine;
            // szWrite[iPos++] = '\r';
            szWrite[iPos++] = '\n';
            if(cColour != '\0')
            {
               szWrite[iPos++] = '\037';
               szWrite[iPos++] = '0';
            }
            szWrite[iPos++] = '\0';
            // debug("CmdCentre '%s'\n", szWrite);

            CmdWrite(szWrite);

            // printf("CmdCentre return at %d chars past %d\n", iLine, szPrev - szText);
            szPrev = szReturn + 1;
         }
      }

      delete[] szWrite;
   }
   else
   {
      szWrite = new char[strlen(szText) + 10];
      szWrite[0] = '\0';

      sprintf(szWrite, "\037%c", cColour);
      strcat(szWrite, szText);
      strcat(szWrite, "\0370\n");

      CmdWrite(szWrite);

      delete[] szWrite;
   }

   return true;
}

void CmdBanner(EDF *pReply)
{
   STACKTRACE
   char *szText = NULL;

   pReply->GetChild("banner", &szText);

   if(szText != NULL)
   {
      CmdCentre(szText);
   }
   CmdWrite("\n\n");
}

void CmdTaskList(EDF *pReply)
{
   STACKTRACE
   int iTaskID = 0, iRepeat = 0, iNextTime = 0, iDay = 0, iHour = 0, iMinute = 0;
   int iNumRepeats = 0, iNumDones = 0;
   bool bLoop = false;
   char szWrite[100];
   char *szRequest = NULL;
   CmdTable *pTable = NULL;

   debugEDFPrint("CmdTaskList", pReply);

   pTable = new CmdTable(m_pUser, 1, 6, false);

   pTable->AddHeader("ID", 4);
   pTable->AddHeader("Repeat", 7);
   pTable->AddHeader("Day", 4);
   pTable->AddHeader("Time", 6);
   pTable->AddHeader("Next run", 9);
   pTable->AddHeader("Request");

   bLoop = pReply->Child("task");
   while(bLoop == true)
   {
      iHour = 0;
      iMinute = 0;
      iRepeat = 0;

      if(pReply->GetChildBool("done") == true)
      {
         pTable->SetFlag('D');
         iNumDones++;
      }

      pReply->Get(NULL, &iTaskID);
      pTable->SetValue(iTaskID);

      pReply->GetChild("nexttime", &iNextTime);

      if(pReply->GetChild("repeat", &iRepeat) == true)
      {
         switch(iRepeat)
         {
            case TASK_DAILY:
               pTable->SetValue("Daily");
               break;

            case TASK_WEEKDAY:
               pTable->SetValue("Weekday");
               break;

            case TASK_WEEKEND:
               pTable->SetValue("Weekend");
               break;

            case TASK_WEEKLY:
               pTable->SetValue("Weekly");
               break;
         }

         iNumRepeats++;
      }
      else
      {
         pTable->SetValue("");
      }

      if(pReply->GetChild("day", &iDay) == true)
      {
         pTable->SetValue(DAYS[iDay]);
      }
      else
      {
         pTable->SetValue("");
      }

      if(pReply->GetChild("hour", &iHour) == true)
      {
         pReply->GetChild("minute", &iMinute);
         StrValue(szWrite, STRVALUE_HMN, 3600 * iHour + 60 * iMinute);
         pTable->SetValue(szWrite);
      }
      else if(iRepeat == 0)
      {
         StrTime(szWrite, STRTIME_TIMEHM, iNextTime);
         pTable->SetValue(szWrite);
      }
      else
      {
         pTable->SetValue("");
      }

      StrTime(szWrite, STRTIME_DATE, iNextTime);
      pTable->SetValue(szWrite);

      if(pReply->GetChild("request", &szRequest) == true)
      {
         pTable->SetValue(szRequest);
         delete[] szRequest;
      }
      else
      {
         pTable->SetValue("");
      }

      bLoop = pReply->Next("task");
      if(bLoop == false)
      {
         pReply->Parent();
      }
   }

   pTable->AddFooter(pReply->Children("task"), "task");
   pTable->AddFooter(iNumRepeats, "repeating", "repeating");
   pTable->AddFooter(iNumDones, "done", "done");

   delete pTable;
}
