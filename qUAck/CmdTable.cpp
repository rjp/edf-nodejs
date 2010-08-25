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
** CmdTable.cpp: Implementation of the CmdTable class which handles most of the list output
*/

// Standard headers
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <ctype.h>

// UA headers
#include "EDF/EDF.h"

#include "CmdIO.h"
#include "CmdInput.h"
#include "CmdMenu.h"
#include "CmdShow.h"
#include "CmdTable.h"

bool CmdTable::m_bDoubleReturn = false;

CmdTable::CmdTable(EDF *pUser, int iNumFlags, int iNumCols, bool bShowTime, bool bRepeated)
{
   STACKTRACE
   m_iType = 0;

   /* pUser->Root();
   if(pUser->Child("client", CLIENT_NAME) == true)
   {
      m_bDoubleReturn = pUser->GetChildBool("doublereturn");
   } */

   m_iNumFlags = iNumFlags;
   m_iFlagNum = 0;

   m_pHeaders = new CmdHeader[iNumCols];

   if(bRepeated == false)
   {
      // Only a single set of rows
      m_pSets = new CmdSet[1];
      m_pSets[0].m_pCols = new CmdColumn[iNumCols];

      m_iNumSets = 1;
   }
   else
   {
      // Figure the rows out later
      m_pSets = NULL;

      m_iNumSets = 0;
   }
   m_iSetNum = 0;

   m_iNumCols = iNumCols;
   m_iColNum = 0;

   m_pFooters = new CmdFooter[10];
   m_iNumFooters = 0;

   m_pKeys = new CmdKey[10];
   m_iNumKeys = 0;

   m_bShowTime = bShowTime;

   // Mark this point in case of a long list
   CmdPageOn();
}

CmdTable::~CmdTable()
{
   STACKTRACE
   int iCount = 0;

   Update();

   delete[] m_pHeaders;
   for(iCount = 0; iCount < m_iNumSets; iCount++)
   {
      delete[] m_pSets[iCount].m_pCols;
   }
   delete[] m_pSets;
   delete[] m_pFooters;
   delete[] m_pKeys;

   CmdPageOff();
}

bool CmdTable::AddHeader(const char *szTitle, int iWidth, char cColour, int iAlign, int iSpacing)
{
   STACKTRACE
   if(m_iColNum >= m_iNumCols)
   {
      return false;
   }

   strcpy(m_pHeaders[m_iColNum].m_szTitle, szTitle);
   if(iWidth == -1)
   {
      iWidth = strlen(szTitle);
   }
   m_pHeaders[m_iColNum].m_iWidth = iWidth;
   m_pHeaders[m_iColNum].m_cColour = cColour;
   m_pHeaders[m_iColNum].m_iAlign = iAlign;
   m_pHeaders[m_iColNum].m_iSpacing = iSpacing;

   m_iColNum++;

   return true;
}

bool CmdTable::SetFlag(char cValue)
{
   STACKTRACE
   // debug("CmdTable::SetFlag '%c', %d (%d %d)\n", cValue, m_iSetNum, m_iColNum, m_iFlagNum);

   CheckHeaders();

   if(m_iColNum > 0)
   {
      // Values are in use, this is a new row
      m_iSetNum++;
      if(m_iSetNum >= m_iNumSets)
      {
         Update();
      }
		
		// debug("CmdTable::SetFlag reset %d\n", m_iFlagNum);
   }

   if(m_iFlagNum >= m_iNumFlags)
   {
      return false;
   }

   m_pSets[m_iSetNum].m_szFlags[m_iFlagNum] = cValue;
   m_iFlagNum++;

   return true;
}

bool CmdTable::SetValue(const char *szValue, char cColour, int iExtra)
{
   STACKTRACE
   CheckHeaders();

   // debug("CmdTable::SetValue %s '%c', s %d c %d", szValue, cColour, m_iSetNum, m_iColNum);
   if(m_iColNum >= m_iNumCols)
   {
      // debug("CmdTable::SetValue column limit (%d of %d)\n", m_iColNum, m_iNumCols);
      return false;
   }
   // debug("\n");

   // debug("CmdTable::SetValue set column %d at set %d\n", m_iColNum, m_iSetNum);
   if(strlen(szValue) > sizeof(m_pSets[m_iSetNum].m_pCols[m_iColNum].m_szValue))
   {
      strncpy(m_pSets[m_iSetNum].m_pCols[m_iColNum].m_szValue, szValue, sizeof(m_pSets[m_iSetNum].m_pCols[m_iColNum].m_szValue) - 1);
      m_pSets[m_iSetNum].m_pCols[m_iColNum].m_szValue[sizeof(m_pSets[m_iSetNum].m_pCols[m_iColNum].m_szValue) - 1] = '\0';
   }
   else
   {
      strcpy(m_pSets[m_iSetNum].m_pCols[m_iColNum].m_szValue, szValue);
   }
   m_pSets[m_iSetNum].m_pCols[m_iColNum].m_cColour = cColour;
   m_pSets[m_iSetNum].m_pCols[m_iColNum].m_iExtra = iExtra;

   m_iColNum++;
   if(m_iColNum == m_iNumCols)
   {
		m_iFlagNum = 0;
      m_iColNum = 0;

      // printf("CmdTable::SetValue reached column limit on set %d of %d\n", m_iSetNum, m_iNumSets);
      m_iSetNum++;
      if(m_iSetNum == m_iNumSets)
      {
         // printf("CmdTable::SetValue reached set limit\n");
         Update();
      }
   }

   return true;
}

bool CmdTable::SetValue(int iValue, char cColour, int iExtra)
{
   STACKTRACE
   char szValue[10];

   sprintf(szValue, "%d", iValue);
   return SetValue(szValue, cColour, iExtra);
}

bool CmdTable::AddFooter(int iValue, const char *szSingle, const char *szPlural)
{
   STACKTRACE
   // debug("CmdTable::AddFooter %d %s %s, %d %d\n", iValue, szSingle, szPlural, m_iType, m_iSetNum);

   if(m_iType == 0)
   {
      Update();

      m_iType = 2;
   }
   if(m_iType == 1)
   {
      // First footer, might be row data
      // debug("CmdTable::AddFooter col %d\n", m_iColNum);
      if(m_iColNum > 0)
      {
         m_iSetNum++;
      }
      Update();

      m_iType = 2;
   }

   if(m_iNumFooters > 0 && iValue == 0)
   {
      return false;
   }

   m_pFooters[m_iNumFooters].m_iValue = iValue;
   strcpy(m_pFooters[m_iNumFooters].m_szSingle, szSingle);
   if(szPlural != NULL)
   {
      strcpy(m_pFooters[m_iNumFooters].m_szPlural, szPlural);
   }
   else
   {
      m_pFooters[m_iNumFooters].m_szPlural[0] = '\0';
   }

   m_iNumFooters++;

   return true;
}

bool CmdTable::AddKey(char cKey, const char *szKey)
{
   STACKTRACE

   // debug("CmdTable::AddKey %c %s, %d %d\n", cKey, szKey, m_iType, m_iSetNum);

   if(m_iType == 0)
   {
      Update();

      m_iType = 2;
   }
   if(m_iType == 1)
   {
      // First key, might be row data
      // debug("CmdTable::AddKey col %d\n", m_iColNum);
      if(m_iColNum > 0)
      {
         m_iSetNum++;
      }
      Update();

      m_iType = 2;
   }

   m_pKeys[m_iNumKeys].m_cKey = cKey;
   strcpy(m_pKeys[m_iNumKeys].m_szKey, szKey);

   m_iNumKeys++;

   return true;
}

bool CmdTable::Flush()
{
   STACKTRACE
   // debug("CmdTable::Flush %d %d %d\n", m_iType, m_iSetNum, m_iColNum);

   if(m_iType == 0)
   {
      Update();
   }

   if(m_iColNum > 0)
   {
      m_iSetNum++;
      if(m_iSetNum >= m_iNumSets)
      {
         Update();
      }
   }

   return true;
}

// CmdTable::MakeSets: Calculate how many repeating rows will fit on a line
void CmdTable::MakeSets()
{
   STACKTRACE
   int iCount = 0, iTotal = 0;

   for(iCount = 0; iCount < m_iNumCols; iCount++)
   {
      // debug("CmdTable::MakeSets %d flags\n", m_iNumFlags);
      iTotal += m_iNumFlags;
      // debug("CmdTable::MakeSets column width %d\n", 1 + m_pHeaders[iCount].m_iWidth);
      iTotal += (2 + m_pHeaders[iCount].m_iWidth);
   }

   m_iNumSets = CmdWidth() / iTotal;
   // debug("CmdTable::MakeSets Sets %d / %d = %d\n", CmdWidth(m_pConn), iTotal, m_iNumSets);
   m_pSets = new CmdSet[m_iNumSets];
   for(iCount = 0; iCount < m_iNumSets; iCount++)
   {
      // debug("CmdTable::MakeSets set %d\n", iCount); 
      m_pSets[iCount].m_pCols = new CmdColumn[m_iNumCols];
   }
}

void CmdTable::CheckHeaders()
{
   STACKTRACE
   if(m_iType == 0)
   {
      if(m_iNumSets == 0)
      {
         MakeSets();
      }

      Update();
   }
}

void CmdTable::Update()
{
   STACKTRACE
   int iWritePos = 0, iCount = 0, iExtras = 0, iSetNum = 0, iKeyPos = 0;
   char cColour = '\0';
   char szWrite[1000], szTime[32], szValue[32];
   time_t tTime;
   // struct tm *tmTime = NULL;

   // debug("CmdTable::Update, type %d set %d col %d\n", m_iType, m_iSetNum, m_iColNum);

   memset(szWrite, '\0', sizeof(szWrite));

   if(m_iType == 0)
   {
      // Headers
      // debug("CmdTable::Update column headers\n");
      for(iSetNum = 0; iSetNum < m_iNumSets; iSetNum++)
      {
         // debug("CmdTable::Update header %d\n", iCount);

         if(m_iNumFlags > 0)
         {
            memset(szWrite + iWritePos, '~', m_iNumFlags);
            iWritePos += m_iNumFlags;
         }

         for(iCount = 0; iCount < m_iNumCols; iCount++)
         {
            if(m_pHeaders[iCount].m_iAlign == CMDTABLE_RIGHT && strlen(m_pHeaders[iCount].m_szTitle) < m_pHeaders[iCount].m_iWidth)
            {
               // Pad right aligned header
               memset(szWrite + iWritePos, '~', m_pHeaders[iCount].m_iWidth - strlen(m_pHeaders[iCount].m_szTitle));
               iWritePos += (m_pHeaders[iCount].m_iWidth - strlen(m_pHeaders[iCount].m_szTitle));
            }
            szWrite[iWritePos++] = ' ';
            szWrite[iWritePos++] = '\037';
            szWrite[iWritePos++] = '7';
            strcpy(szWrite + iWritePos, m_pHeaders[iCount].m_szTitle);
            iWritePos += strlen(m_pHeaders[iCount].m_szTitle);
            szWrite[iWritePos++] = '\037';
            szWrite[iWritePos++] = '0';
            szWrite[iWritePos++] = ' ';
            iExtras += 4;

            // debug("CmdTable::Update title %d length %d\n", strlen(m_pHeaders[iColNum].m_szTitle), m_pHeaders[iColNum].m_iWidth);
            if(m_pHeaders[iCount].m_iAlign == CMDTABLE_LEFT && strlen(m_pHeaders[iCount].m_szTitle) < m_pHeaders[iCount].m_iWidth)
            {
               // Pad left aligned header
               memset(szWrite + iWritePos, '~', m_pHeaders[iCount].m_iWidth - strlen(m_pHeaders[iCount].m_szTitle));
               iWritePos += (m_pHeaders[iCount].m_iWidth - strlen(m_pHeaders[iCount].m_szTitle));
            }
            if(m_pHeaders[iCount].m_iSpacing > 0)
            {
               memset(szWrite + iWritePos, '~', m_pHeaders[iCount].m_iSpacing);
               iWritePos += m_pHeaders[iCount].m_iSpacing;
            }
         }
      }

      // printf("CmdTable::Update header pad %d -vs- %d\n", iWritePos - iExtras, CmdWidth());
      if(iWritePos - iExtras <= CmdWidth())
      {
         // Pad the line out
         memset(szWrite + iWritePos, '~', CmdWidth() - iWritePos + iExtras);
         iWritePos = CmdWidth() + iExtras;
      }
      if(mask(CmdInput::HardWrap(), CMD_WRAP_VIEW) == true)
      {
         // szWrite[iWritePos++] = '\r';
         szWrite[iWritePos++] = '\n';
      }
      if(m_bDoubleReturn == true)
      {
         // szWrite[iWritePos++] = '\r';
         szWrite[iWritePos++] = '\n';
      }

      m_iType = 1;
   }
   else if(m_iType == 1)
   {
      // Row data - do number of sets used this time NOT total number of potential sets
      for(iSetNum = 0; iSetNum < m_iSetNum; iSetNum++)
      {
         memcpy(szWrite + iWritePos, m_pSets[iSetNum].m_szFlags, m_iNumFlags);
         iWritePos += m_iNumFlags;
         // printf("CmdTable::Update post-flags write pos %d, extra %d\n", iWritePos, iExtras);

         // Columns are blanked by default so no problem here
         for(iCount = 0; iCount < m_iNumCols; iCount++)
         {
            cColour = '\0';

            // debug("CmdTable::Update set %d col %d value '%s'\n", iSetNum, iCount, m_pSets[iSetNum].m_pCols[iCount].m_szValue);

            szWrite[iWritePos++] = ' ';
            // printf("CmdTable::Update check 2 write pos %d, extra %d\n", iWritePos, iExtras);

            // debug("CmdTable::Update column %d (width %d aligned %s), value len %d\n", iCount, m_pHeaders[iCount].m_iWidth, m_pHeaders[iCount].m_iAlign == CMDTABLE_RIGHT ? "right" : "left", strlen(m_pSets[iSetNum].m_pCols[iCount].m_szValue));
            if(m_pHeaders[iCount].m_iAlign == CMDTABLE_RIGHT && strlen(m_pSets[iSetNum].m_pCols[iCount].m_szValue) < m_pHeaders[iCount].m_iWidth)
            {
               // Pad out right aligned column
               memset(szWrite + iWritePos, ' ', m_pHeaders[iCount].m_iWidth - strlen(m_pSets[iSetNum].m_pCols[iCount].m_szValue));
               iWritePos += (m_pHeaders[iCount].m_iWidth - strlen(m_pSets[iSetNum].m_pCols[iCount].m_szValue));
               // printf("CmdTable::Update post-pad write pos %d, extra %d\n", iWritePos, iExtras);
            }

            if(m_pSets[iSetNum].m_pCols[iCount].m_cColour != '\0')
            {
               cColour = m_pSets[iSetNum].m_pCols[iCount].m_cColour;
            }
            else if(m_pHeaders[iCount].m_cColour != '\0')
            {
               cColour = m_pHeaders[iCount].m_cColour;
            }

            if(cColour != '\0')
            {
               szWrite[iWritePos++] = '\037';
               szWrite[iWritePos++] = cColour;
               // printf("CmdTable::Update pre-value colour write pos %d, extra %d\n", iWritePos, iExtras);
            }
            strcpy(szWrite + iWritePos, m_pSets[iSetNum].m_pCols[iCount].m_szValue);
            iWritePos += strlen(m_pSets[iSetNum].m_pCols[iCount].m_szValue);
            iExtras += m_pSets[iSetNum].m_pCols[iCount].m_iExtra;
            // printf("CmdTable::Update post-value write pos %d, extra %d\n", iWritePos, iExtras);

            if(cColour != '\0')
            {
               szWrite[iWritePos++] = '\037';
               szWrite[iWritePos++] = '0';
               iExtras += 4;
               // printf("CmdTable::Update post-value colour write pos %d, extra %d\n", iWritePos, iExtras);
            }

            // iExtras += m_pSets[iSetNum].m_pCols[iCount].m_iExtra;
            // printf("CmdTable::Update post-extra write pos %d, extra %d\n", iWritePos, iExtras);

            // Don't end the line until the last set
            if(iSetNum == m_iSetNum - 1 && iCount == m_iNumCols - 1)
            {
               // printf("CmdTable::Update last value %d -vs- %d\n", iWritePos - iExtras, CmdWidth());
               if(mask(CmdInput::HardWrap(), CMD_WRAP_VIEW) == true || iWritePos - iExtras != CmdWidth())
               {
                  // szWrite[iWritePos++] = '\r';
                  szWrite[iWritePos++] = '\n';
               }
            }
            else
            {
               if(m_pHeaders[iCount].m_iAlign == CMDTABLE_LEFT && strlen(m_pSets[iSetNum].m_pCols[iCount].m_szValue) < m_pHeaders[iCount].m_iWidth)
               {
                  // Pad out left aligned column
                  memset(szWrite + iWritePos, ' ', m_pHeaders[iCount].m_iWidth - strlen(m_pSets[iSetNum].m_pCols[iCount].m_szValue));
                  iWritePos += (m_pHeaders[iCount].m_iWidth - strlen(m_pSets[iSetNum].m_pCols[iCount].m_szValue));
               }
               if(m_pHeaders[iCount].m_iSpacing > 0)
               {
                  memset(szWrite + iWritePos, ' ', m_pHeaders[iCount].m_iSpacing);
                  iWritePos += m_pHeaders[iCount].m_iSpacing;
               }
               szWrite[iWritePos++] = ' ';
            }
         }
      }

      // debug("CmdTable::Update set reset\n");
      m_iSetNum = 0;

      // memprint("CmdTable::Update row set", (const byte *)szWrite, 200);
   }
   else if(m_iType == 2)
   {
      if(m_iNumFooters > 0)
      {
         strcpy(szWrite, "~~ Total:");
         iWritePos = 9;

         for(iCount = 0; iCount < m_iNumFooters; iCount++)
         {
            // debug("CmdTable::Update footer %d, %d %s\n", iCount, m_pFooters[iCount].m_iValue, m_pFooters[iCount].m_szSingle);
            if(iCount > 0)
            {
               szWrite[iWritePos++] = ',';
            }
            sprintf(szValue, " \0377%d\0370 ", m_pFooters[iCount].m_iValue);
            strcpy(szWrite + iWritePos, szValue);
            iWritePos += strlen(szValue);
            iExtras += 4;

            if(iCount == 0)
            {
               if(m_pFooters[iCount].m_iValue != 1 && strcmp(m_pFooters[iCount].m_szPlural, "") != 0)
               {
                  strcpy(szWrite + iWritePos, m_pFooters[iCount].m_szPlural);
                  iWritePos += strlen(m_pFooters[iCount].m_szPlural);
               }
               else
               {
                  strcpy(szWrite + iWritePos, m_pFooters[iCount].m_szSingle);
                  iWritePos += strlen(m_pFooters[iCount].m_szSingle);
                  if(m_pFooters[iCount].m_iValue != 1)
                  {
                     szWrite[iWritePos++] = 's';
                  }
               }
            }
            else
            {
               strcpy(szWrite + iWritePos, m_pFooters[iCount].m_szSingle);
               iWritePos += strlen(m_pFooters[iCount].m_szSingle);
            }
         }

         szWrite[iWritePos++] = ' ';
      }

      /* if(m_bShowTime == true)
      {
         iWidth -= 23;
      } */
      // tTime = time(NULL);
      tTime = CmdInput::MenuTime();
      // tmTime = localtime(&tTime);
      // strftime(szTime, sizeof(szTime), " \0377%d/%m/%Y %H:%M:%S\0370 ~~", tmTime);
      StrTime(szTime, STRTIME_SHORT, tTime, '7', " ", " ~~");
      // printf("CmdTable::Update footer pad %s, %d -vs- %d / %d -vs- %d\n", BoolStr(m_bShowTime), iWritePos - iExtras, iWidth - strlen(szTime) + 4, iWritePos - iExtras, iWidth);
      if(m_bShowTime == true && iWritePos - iExtras <= CmdWidth() - strlen(szTime) + 4)
      {
         int iPad = (CmdWidth() - strlen(szTime) + 4) - (iWritePos - iExtras);
         memset(szWrite + iWritePos, '~', iPad);
         iWritePos += iPad;
         memcpy(szWrite + iWritePos, szTime, strlen(szTime));
         iWritePos += strlen(szTime);
      }
      else if(iWritePos - iExtras <= CmdWidth())
      {
         // Pad the line out
         memset(szWrite + iWritePos, '~', CmdWidth() - iWritePos + iExtras);
         iWritePos = CmdWidth() + iExtras;
      }

      if(mask(CmdInput::HardWrap(), CMD_WRAP_VIEW) == true)
      {
         // szWrite[iWritePos++] = '\r';
         szWrite[iWritePos++] = '\n';
      }

      if(CmdInput::MenuLevel() < CMDLEV_EXPERT && m_iNumKeys > 0)
      {
         CmdWrite(szWrite);

         memset(szWrite, '\0', sizeof(szWrite));

         strcpy(szWrite, "~~~~ Key: ");
         iWritePos = 9;
         iExtras = 0;

         for(iCount = 0; iCount < m_iNumKeys; iCount++)
         {
            // debug("CmdTable::Update Key %d, %d %s\n", iCount, m_pKeys[iCount].m_iValue, m_pKeys[iCount].m_szSingle);
            if(iCount > 0)
            {
               szWrite[iWritePos++] = ',';
            }
            // sprintf(szValue, " \0377%c\0370 - %s", m_pKeys[iCount].m_cKey, m_pKeys[iCount].m_szKey);
            strcpy(szValue, " ");
            for(iKeyPos = 0; iKeyPos < strlen(m_pKeys[iCount].m_szKey); iKeyPos++)
            {
               if(isupper(m_pKeys[iCount].m_szKey[iKeyPos]))
               {
                  strcat(szValue, "\0377");
               }
               sprintf(szValue, "%s%c", szValue, m_pKeys[iCount].m_szKey[iKeyPos]);
               if(isupper(m_pKeys[iCount].m_szKey[iKeyPos]))
               {
                  strcat(szValue, "\0370");
               }
            }
            strcpy(szWrite + iWritePos, szValue);
            iWritePos += strlen(szValue);
            iExtras += 4;
         }
         
         szWrite[iWritePos++] = ' ';
      
         if(iWritePos - iExtras <= CmdWidth())
         {
            // Pad the line out
            memset(szWrite + iWritePos, '~', CmdWidth() - iWritePos + iExtras);
            iWritePos = CmdWidth() + iExtras;
         }

         if(mask(CmdInput::HardWrap(), CMD_WRAP_VIEW) == true)
         {
            // szWrite[iWritePos++] = '\r';
            szWrite[iWritePos++] = '\n';
         }

         // memprint("CmdTable::Update key", (const byte *)szWrite, 150, false);
      }

      if(m_bDoubleReturn == true)
      {
         // szWrite[iWritePos++] = '\r';
         szWrite[iWritePos++] = '\n';
      }
   }

   CmdWrite(szWrite);

   if(m_iType == 1)
   {
      // debug("CmdTable::Update reset flags / cols (%d sets of %d cols)\n", m_iNumSets, m_iNumCols);
      for(iSetNum = 0; iSetNum < m_iNumSets; iSetNum++)
      {
         memset(m_pSets[iSetNum].m_szFlags, ' ', m_iNumFlags);
         for(iCount = 0; iCount < m_iNumCols; iCount++)
         {
            m_pSets[iSetNum].m_pCols[iCount].m_szValue[0] = '\0';
            m_pSets[iSetNum].m_pCols[iCount].m_cColour = '\0';
            m_pSets[iSetNum].m_pCols[iCount].m_iExtra = 0;
         }
      }
   }

   m_iFlagNum = 0;
   m_iColNum = 0;

   // debug("CmdTable::Update exit ,%d %d %d\n", m_iSetNum, m_iFlagNum, m_iColNum);
}
