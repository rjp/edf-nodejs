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
** CmdInter.cpp: Implementation of input class and output functions
*/

#include "ua.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#ifdef HAVE_LIBASPELL
#include <aspell.h>
#endif

#include "EDF/EDF.h"

#include "CmdIO.h"
#include "CmdIO-common.h"
#include "CmdInput.h"

#define ENTER_TEXT "\nEnter your text, press Escape to finish or for help\n"

#define CMD_LINE_RETEND 1
#define CMD_LINE_WRAPSPACE 2
#define CMD_LINE_WRAPNOSPACE 3

#define LINE_DATA 1000

long CmdInput::m_lMenuTime = 0;
long CmdInput::m_lMenuTimeOffset = 0;
int CmdInput::m_iMenuLevel = CMDLEV_BEGIN;
bool CmdInput::m_bMenuCase = false;
int CmdInput::m_iMenuStatus = 0;
int CmdInput::m_iHardWrap = 0;
bool CmdInput::m_bSpellCheck = false;

bool CmdInput::m_bSetup = false;
CmdInput *CmdInput::m_pAbort = NULL;

#ifdef HAVE_LIBASPELL
char g_szConfigDir[200];
AspellConfig *g_pConfig = NULL;
AspellSpeller *g_pSpeller = NULL;
#endif

#ifdef HAVE_LIBASPELL
void SpellConfig(char *szField, char *szValue)
{
   char szReplace[200];

#ifdef WIN32
   if(szValue != NULL)
   {
      sprintf(szReplace, "%s\\%s", g_szConfigDir, szValue);
   }
   else
   {
      strcpy(szReplace, g_szConfigDir);
   }
#endif
#ifdef UNIX
   strcpy(szReplace, szValue);
#endif

   debug("SpellConfig replacing field value for %s with %s\n", szField, szReplace);

   aspell_config_replace(g_pConfig, szField, szReplace);
}
#endif

int MenuSort(const void *a, const void *b)
{
   CmdInput::InputMenu *pOpt1 = (CmdInput::InputMenu *)a, *pOpt2 = (CmdInput::InputMenu *)b;

   if(tolower(pOpt1->m_cOption) < tolower(pOpt2->m_cOption))
   {
      return -1;
   }
   else if(tolower(pOpt1->m_cOption) > tolower(pOpt2->m_cOption))
   {
      return 1;
   }
   else if(pOpt1->m_cOption < pOpt2->m_cOption)
   {
      return -1;
   }
   else if(pOpt1->m_cOption > pOpt2->m_cOption)
   {
      return 1;
   }

   return 0;
}

CmdInput::CmdInput(CMDFIELDSFUNC pFieldsFunc, EDF *pData)
{
   // STACKTRACE
   LineSetup(NULL, NULL, -1, CMD_LINE_MULTI, NULL, (void *)pFieldsFunc, pData);
}

CmdInput::CmdInput(const char *szTitle, int iMax, int iOptions, const char *szInit, CMDTABFUNC pTabFunc, EDF *pData)
{
   LineSetup(szTitle, NULL, iMax, iOptions, szInit, (void *)pTabFunc, pData);
}

CmdInput::CmdInput(const char *szTitle, const char *szExtra, int iMax, int iOptions, const char *szInit, CMDTABFUNC pTabFunc, EDF *pData)
{
   LineSetup(szTitle, szExtra, iMax, iOptions, szInit, (void *)pTabFunc, pData);
}

CmdInput::CmdInput(int iOptions, const char *szTitle)
{
   Setup(CMD_TYPE_MENU | iOptions, szTitle, NULL);

   m_iNumOptions = 0;
   m_pOptions = NULL;
   m_cDefault = '\0';
   m_bSorted = false;

   if(mask(m_iType, CMD_MENU_YESNO) == true)
   {
      MenuAdd('y');
      MenuAdd('n', NULL, NULL, true);
   }
}

CmdInput::~CmdInput()
{
   int iPos = 0;

   // debug("CmdInput::CmdInput entry %p\n", this);

   delete[] m_szTitle;
   delete[] m_szExtra;

   if(mask(m_iType, CMD_TYPE_MENU) == true)
   {
      for(iPos = 0; iPos < m_iNumOptions; iPos++)
      {
         delete[] m_pOptions[iPos].m_szOption;
         delete[] m_pOptions[iPos].m_szHelp;
      }
      delete[] m_pOptions;
   }
   else if(mask(m_iType, CMD_TYPE_LINE) == true)
   {
      if(mask(m_iType, CMD_LINE_MULTI) == false)
      {
         // debug("CmdInput::~CmdInput deleting single line %p\n", m_szLine);
         delete[] m_szLine;
      }
      else
      {
         for(iPos = 0; iPos < m_iNumRows; iPos++)
         {
            delete[] m_pRows[iPos].m_szLine;
         }
         delete[] m_pRows;
      }
      delete[] m_szInit;

      delete m_pEdit;
   }
}

int CmdInput::Type()
{
   return m_iType;
}

bool CmdInput::Show()
{
   STACKTRACE
   int iPos = 0, iCharPos = 0, iWriteLen = 0;
   long lTimeOn = 0;
   char cStatus = '\0';
   char szWrite[200];

	szWrite[0] = '\0';
   if(mask(m_iType, CMD_TYPE_LINE) == true)
   {
      if(mask(m_iType, CMD_LINE_MULTI) == true && m_pChild != NULL)
      {
         m_pChild->Show();
         return true;
      }
      else
      {
         szWrite[0] = '\0';
         if(mask(m_iType, CMD_LINE_MULTI) == false)
         {
            strcat(szWrite, m_szTitle);
            if(m_iMenuLevel != CMDLEV_EXPERT && m_szExtra != NULL && strlen(m_szExtra) > 0)
            {
               sprintf(szWrite, "%s (%s)", szWrite, m_szExtra);
            }
            strcat(szWrite, ": ");
         }
         else if(mask(m_iType, CMD_LINE_NOTITLE) == false)
         {
            strcpy(szWrite, ENTER_TEXT);
            strcat(szWrite, DASHES);
         }
         CmdWrite(szWrite);

         if(m_szInit != NULL)
         {
            // strcat(szWrite, (char *)m_szInit);
            CmdWrite(m_szInit, CMD_OUT_NOHIGHLIGHT);

            if(mask(m_iType, CMD_LINE_MULTI) == true)
            {
               strcpy((char *)m_pRows[0].m_szLine, (char *)m_szInit);
            }
            else
            {
               strcpy((char *)m_szLine, (char *)m_szInit);
            }
            m_iColumn = strlen((char *)m_szInit);
            m_iLength = m_iColumn;
            delete[] m_szInit;
            m_szInit = NULL;
         }
      }
   }
   else if(mask(m_iType, CMD_MENU_SILENT) == false)
   {
      // debug("CmdInput::Show redrawing menu %s\n", m_szTitle);

      if(mask(m_iType, CMD_MENU_YESNO) == true)
      {
         strcat(szWrite, m_szTitle);
         if(m_iMenuLevel != CMDLEV_EXPERT && m_szExtra != NULL)
         {
            sprintf(szWrite, "%s (%s)", szWrite, m_szExtra);
         }
         strcat(szWrite, "? ");
      }
      else if(mask(m_iType, CMD_MENU_SIMPLE) == true)
      {
         strcat(szWrite, m_szTitle);
         if(m_iMenuLevel != CMDLEV_EXPERT && m_szExtra != NULL)
         {
            sprintf(szWrite, "%s (%s)", szWrite, m_szExtra);
         }
         strcat(szWrite, ". ");
         for(iPos = 0; iPos < m_iNumOptions; iPos++)
         {
            if(m_pOptions[iPos].m_bVisible == true)
            {
               iCharPos = 0;
               if(iPos > 0 && m_pOptions[iPos].m_szOption[iCharPos] != '\0')
               {
                  strcat(szWrite, ", ");
               }
               while(m_pOptions[iPos].m_szOption[iCharPos] != '\0')
               {
                  if(isupper(m_pOptions[iPos].m_szOption[iCharPos]))
                  {
                     strcat(szWrite, "\0374");
                  }
                  strncat(szWrite, (char *)&m_pOptions[iPos].m_szOption[iCharPos], 1);
                  if(isupper(m_pOptions[iPos].m_szOption[iCharPos]))
                  {
                     strcat(szWrite, "\0370");
                  }
                  iCharPos++;
               }
            }
         }
         strcat(szWrite, "? ");
      }
      else
      {
         lTimeOn = time(NULL) - m_lMenuTime - m_lMenuTimeOffset;
         if(m_bSorted == false)
         {
            qsort(m_pOptions, m_iNumOptions, sizeof(InputMenu), MenuSort);
            m_bSorted = true;
         }
         if(m_iMenuLevel == CMDLEV_BEGIN)
         {
            ShowOptions(false);
         }
         szWrite[0] = '\0';
         if(mask(m_iType, CMD_MENU_TIME) == true)
         {
            if(mask(m_iMenuStatus, LOGIN_BUSY) == true)
            {
               cStatus = 'B';
            }
            else if(mask(m_iMenuStatus, LOGIN_SHADOW) == true)
            {
               cStatus = 'H';
            }
            else
            {
               cStatus = '+';
            }
            sprintf(szWrite, "\0374%c%02d:%02d\0370 ", cStatus, abs(lTimeOn) / 3600, (abs(lTimeOn) / 60) % 60);
         }
         strcat(szWrite, m_szTitle);
         if(m_iMenuLevel != CMDLEV_EXPERT && m_szExtra != NULL)
         {
            sprintf(szWrite, "%s (%s)", szWrite, m_szExtra);
         }
         if(m_iMenuLevel == CMDLEV_BEGIN || m_iMenuLevel == 1)
         {
            strcat(szWrite, " (\0374");
            iWriteLen = strlen(szWrite);
            for(iPos = 0; iPos < m_iNumOptions; iPos++)
            {
               if(m_pOptions[iPos].m_bVisible == true)
               {
                  szWrite[iWriteLen++] = m_bMenuCase == false ? toupper(m_pOptions[iPos].m_cOption) : m_pOptions[iPos].m_cOption;
               }
            }
            szWrite[iWriteLen] = '\0';
            strcat(szWrite, "\0370, ?+ for help)");
            /* if(m_bExtraHelp == true)
            {
               strcat(szWrite, "+");
            }
            strcat(szWrite, " for help)"); */
         }
         strcat(szWrite, ": ");
      }

      CmdWrite(szWrite);

      if(m_cDefault != '\0')
      {
         // sprintf(szWrite, "%s%c\b", szWrite, m_bMenuCase == false ? toupper(m_cDefault) : m_cDefault);
         CmdWrite((const char)(m_bMenuCase == false ? toupper(m_cDefault) : m_cDefault));
         CmdBack();
      }
   }

   return true;
}

byte CmdInput::Process(byte cOption)
{
   STACKTRACE
   int iRowNum = 0;

   if(mask(m_iType, CMD_TYPE_MENU) == true)
   {
      cOption = MenuProcess(cOption);
   }
   else if(mask(m_iType, CMD_TYPE_LINE) == true)
   {
      if(mask(m_iType, CMD_LINE_MULTI) == true && m_pChild != NULL)
      {
         cOption = m_pChild->Process(cOption);
         if(m_pChild == m_pEdit)
         {
            if(cOption == 'l')
            {
               CmdWrite(DASHES);
               for(iRowNum = 0; iRowNum < m_iNumRows; iRowNum++)
               {
                  CmdWrite(m_pRows[iRowNum].m_szLine, CMD_OUT_NOHIGHLIGHT);
                  CmdWrite("\n");
               }
               CmdWrite(DASHES);

               m_pChild->Show();

               cOption = '\0';
            }
            else if(cOption == 'c')
            {
               m_pChild = NULL;

               CmdWrite(ENTER_TEXT);
               CmdWrite(DASHES);

               m_iRowNum = m_iNumRows - 1;
               m_iLength = strlen((char *)m_pRows[m_iRowNum].m_szLine);
               m_iColumn = m_iLength;

               CmdWrite(m_pRows[m_iRowNum].m_szLine, CMD_OUT_NOHIGHLIGHT);

               cOption = '\0';
            }
            else if(cOption == 'f')
            {
               if(m_pFieldsFunc(m_pData) == false)
               {
                  m_bAbort = true;

                  cOption = '\033';
               }
               else
               {
                  m_pChild = NULL;

                  CmdWrite(ENTER_TEXT);
                  CmdWrite(DASHES);

                  m_iRowNum = m_iNumRows - 1;
                  m_iLength = strlen((char *)m_pRows[m_iRowNum].m_szLine);
                  m_iColumn = m_iLength;

                  CmdWrite(m_pRows[m_iRowNum].m_szLine, CMD_OUT_NOHIGHLIGHT);


                  cOption = '\0';
               }
            }
            else if(cOption == 's' || cOption == 'o')
            {
               m_pChild = NULL;

               CmdWrite("Sending message...\n");

               cOption = '\033';
            }
            else if(cOption == 'a')
            {
               m_pChild = m_pAbort;
               m_pChild->Show();

               cOption = '\0';
            }
#ifdef HAVE_LIBASPELL
            else if(cOption == 'p')
            {
               LineSpell();

               m_pChild->Show();

               cOption = '\0';
            }
#endif
            else if(cOption == 'h')
            {
               CmdWrite("\nUNaXcess editor commands:\n\n");
               CmdWrite("          ^B - Back a character           ^F - Forwards a character\n");
               CmdWrite(" ^H,DEL,BKSP - Delete a character         ^W - Delete a word\n");
               CmdWrite("          ^A - Start of line              ^E - End of line\n");
               CmdWrite("       ^P,^U - Previous line              ^N - Next line\n");
               CmdWrite("          ^K - Kill line                  ^X - Join lines\n");
               CmdWrite("\n");
               CmdWrite("    ESCAPE - finish\n\n\n");

               m_pChild->Show();

               cOption = '\0';
            }
            /* else
            {
               m_pChild->Show();
            } */
         }
         else if(m_pChild == m_pAbort)
         {
            if(cOption == 'y')
            {
               m_pChild = NULL;
               m_bAbort = true;

               cOption = '\033';
            }
            else if(cOption == 'n')
            {
               m_pChild = m_pEdit;
               m_pChild->Show();

               cOption = '\0';
            }
         }
      }
      else
      {
         cOption = LineProcess(cOption);
      }
   }

   return cOption;
}

bool CmdInput::MenuAdd(char cOption, const char *szOption, const char *szHelp, bool bDefault)
{
   STACKTRACE
   int iOptionNum = 0;
   InputMenu *pOptions = NULL;

   if(mask(m_iType, CMD_TYPE_MENU) == false || MenuFind(cOption) == true)
   {
      debug(DEBUGLEVEL_ERR, "CmdInput::MenuAdd cannot add '%c' to menu\n", cOption);
      debug(DEBUGLEVEL_ERR, "CmdInput::MenuAdd currently %d options:\n", m_iNumOptions);
      for(iOptionNum = 0; iOptionNum < m_iNumOptions; iOptionNum++)
      {
         debug(DEBUGLEVEL_ERR, "  '%c' - %s\n", m_pOptions[iOptionNum].m_cOption, m_pOptions[iOptionNum].m_szOption);
      }

      return false;
   }

   pOptions = new InputMenu[m_iNumOptions + 1];
   memcpy(pOptions, m_pOptions, m_iNumOptions * sizeof(InputMenu));
   if(m_bMenuCase == true)
   {
      pOptions[m_iNumOptions].m_cOption = cOption;
   }
   else
   {
      pOptions[m_iNumOptions].m_cOption = tolower(cOption);
   }
   pOptions[m_iNumOptions].m_bVisible = true;
   pOptions[m_iNumOptions].m_szOption = strmk(szOption);
   /* if(mask(m_iType, CMD_MENU_NOHELP) == false)
   { */
      pOptions[m_iNumOptions].m_szHelp = strmk(szHelp);
   /* }
   else
   {
      pOptions[m_iNumOptions].m_szHelp = NULL;
   } */
   /* if(pOptions[m_iNumOptions].m_szHelp != NULL)
   {
      m_bExtraHelp = true;
   } */
   if(bDefault == true)
   {
      m_cDefault = pOptions[m_iNumOptions].m_cOption;
   }
   delete[] m_pOptions;
   m_pOptions = pOptions;

   m_iNumOptions++;
   m_bSorted = false;

   return true;
}

bool CmdInput::MenuValue(char cValue)
{
   STACKTRACE
   InputMenu *pOptions = NULL;

   if(mask(m_iType, CMD_TYPE_MENU) == false || MenuFind(cValue) == true)
   {
      // debug("CmdInput::MenuAdd cannot add '%c' to menu\n", cOption);
      return false;
   }

   pOptions = new InputMenu[m_iNumOptions + 1];
   memcpy(pOptions, m_pOptions, m_iNumOptions * sizeof(InputMenu));
   if(m_bMenuCase == true)
   {
      pOptions[m_iNumOptions].m_cOption = cValue;
   }
   else
   {
      pOptions[m_iNumOptions].m_cOption = tolower(cValue);
   }
   pOptions[m_iNumOptions].m_bVisible = false;
   pOptions[m_iNumOptions].m_szOption = NULL;
   /* if(mask(m_iType, CMD_MENU_NOHELP) == false)
   { */
      pOptions[m_iNumOptions].m_szHelp = NULL;
   /* }
   else
   {
      pOptions[m_iNumOptions].m_szHelp = NULL;
   } */
   /* if(pOptions[m_iNumOptions].m_szHelp != NULL)
   {
      m_bExtraHelp = true;
   } */
   /* if(bDefault == true)
   {
      m_cDefault = pOptions[m_iNumOptions].m_cOption;
   } */
   delete[] m_pOptions;
   m_pOptions = pOptions;

   m_iNumOptions++;
   m_bSorted = false;

   return true;
}

bool CmdInput::MenuDefault(char cOption)
{
   STACKTRACE

   if(mask(m_iType, CMD_TYPE_MENU) == false || MenuFind(cOption) == false)
   {
      return false;
   }

   m_cDefault = cOption;

   return true;
}

/* bool CmdInput::MenuDisplay(long lTimeOn, int iMenuLevel, int iStatus)
{
   if(mask(m_iType, CMD_MENU_COMPLEX) == false)
   {
      return false;
   }

   // debug("CmdInput::MenuDisplay %ld %d %d\n", lTimeOn, iMenuLevel, iStatus);
   m_lTimeOn = lTimeOn;
   m_iMenuLevel = iMenuLevel;
   m_iStatus = iStatus;

   return true;
} */

byte *CmdInput::LineData()
{
   STACKTRACE
   int iRow = 0, iDataPos = 0, iDataSize = LINE_DATA, iLineLen = 0;
   byte *szData = NULL, *szTemp = NULL;

   if(mask(m_iType, CMD_TYPE_LINE) == false)
   {
      return NULL;
   }

   if(mask(m_iType, CMD_LINE_MULTI) == false)
   {
      if(m_cReturn == '\033')
      {
         return NULL;
      }

      // strraw("CmdInput::LineData", m_szLine, m_iLength + 1);
      return (byte *)strmk((char *)m_szLine);
   }

   if(m_bAbort == true)
   {
      return NULL;
   }

   szData = new byte[LINE_DATA];
   // debug("CmdInput::LineData %d rows\n", m_iNumRows);
   for(iRow = 0; iRow < m_iNumRows; iRow++)
   {
      // debug("CmdInput::LineData %s\n", m_pRows[iRow].m_szLine);
      iLineLen = strlen((char *)m_pRows[iRow].m_szLine);
      if(iLineLen > 0 || iDataPos > 0)
      {
         if(iDataPos + iLineLen + 3 > iDataSize)
         {
            szTemp = new byte[iDataSize + LINE_DATA];
            memcpy(szTemp, szData, iDataPos);
            delete[] szData;
            szData = szTemp;

            iDataSize += LINE_DATA;
         }
         memcpy((char *)&szData[iDataPos], m_pRows[iRow].m_szLine, iLineLen);
         iDataPos += iLineLen;
         // debug("CmdInput::LineData %d %d\n", iRow, m_pRows[iRow].m_iEndType);
         if(m_pRows[iRow].m_iEndType == CMD_LINE_RETEND)
         {
            szData[iDataPos++] = '\n';
         }
         else if(m_pRows[iRow].m_iEndType == CMD_LINE_WRAPSPACE)
         {
            szData[iDataPos++] = ' ';
         }
      }
   }
   // debug("CmdInput::LineData NULL char at %d\n", iDataPos);
   szData[iDataPos--] = '\0';
   while(iDataPos > 0 && szData[iDataPos] == '\n')
   {
      // debug("CmdInput::LineData move back %d\n", iDataPos);
      szData[iDataPos--] = '\0';
   }

   // memprint("CmdInput::LineData", szData, iDataPos + 1);
   // debug("CmdInput::LineData %p\n", szData);
   return szData;
}

void CmdInput::Shutdown()
{
   delete m_pAbort;

   m_bSetup = false;
}

bool CmdInput::LineInit(const char *szInit)
{
   if(mask(m_iType, CMD_TYPE_LINE) == false)
   {
      return false;
   }

   m_szInit = (byte *)strmk((char *)szInit);

   return true;
}

int CmdInput::LineValue(bool bCheck)
{
   char *szTemp = NULL;

   if(m_pTabFunc != NULL && m_iTabValue == -1 && bCheck == true)
   {
      // printf("CmdInput::LineValue '%s' %d\n", m_szLine, strlen((char *)m_szLine));
      szTemp = (*m_pTabFunc)(m_pData, (char *)m_szLine, strlen((char *)m_szLine), true, &m_iTabValue);
      delete[] szTemp;
   }

   return m_iTabValue;
}

void CmdInput::MenuTime(long lTime)
{
   m_lMenuTime = lTime;
   m_lMenuTimeOffset = time(NULL) - lTime;

   // return true;
}

long CmdInput::MenuTime()
{
   return time(NULL) - m_lMenuTimeOffset;
}

void CmdInput::MenuLevel(int iLevel)
{
   // int iReturn = m_iMenuLevel;

   m_iMenuLevel = iLevel;

   // return iReturn;
}

int CmdInput::MenuLevel()
{
   return m_iMenuLevel;
}

void CmdInput::MenuCase(bool bCase)
{
   m_bMenuCase = bCase;

   // return true;
}

bool CmdInput::MenuCase()
{
   return m_bMenuCase;
}

void CmdInput::MenuStatus(int iStatus)
{
   // int iReturn = m_iMenuStatus;

   m_iMenuStatus = iStatus;

   // return iReturn;
}

int CmdInput::MenuStatus()
{
   return m_iMenuStatus;
}

void CmdInput::HardWrap(int iWrap)
{
   // int iReturn = m_iHardWrap;

   m_iHardWrap = iWrap;

   if(mask(iWrap, CMD_WRAP_VIEW) == true)
   {
      CmdHardWrap(true);
   }
   else
   {
      CmdHardWrap(false);
   }

   // return iReturn;
}

int CmdInput::HardWrap()
{
   return m_iHardWrap;
}

void CmdInput::SpellCheck(bool bSpellCheck)
{
#ifdef HAVE_LIBASPELL
   m_bSpellCheck = bSpellCheck;
#endif
}

bool CmdInput::SpellCheck()
{
   return m_bSpellCheck;
}

void CmdInput::Setup(int iType, const char *szTitle, const char *szExtra)
{
   if(m_bSetup == false)
   {
      m_bSetup = true;

      m_pAbort = new CmdInput(CMD_MENU_YESNO | CMD_MENU_NOCASE, "Really abort edit");

#ifdef HAVE_LIBASPELL
      g_pConfig = new_aspell_config();

#ifdef WIN32
      char *szProgramFiles = getenv("PROGRAMFILES");
      if(szProgramFiles == NULL)
      {
         szProgramFiles = "c:\\Program Files";
      }
      sprintf(g_szConfigDir, "%s\\aspell", szProgramFiles);

      SpellConfig("conf-dir", NULL);
      SpellConfig("data-dir", "data");
      SpellConfig("dict-dir", "dict");
#endif
#if 0
      SpellConfig("dict-dir", "/usr/local/lib/aspell");
      SpellConfig("data-dir", "/usr/local/share/aspell");
#endif

      SpellConfig("per-conf", "/home/rjp/.aspellrc");

      char *szLang = "en_GB";
      aspell_config_replace(g_pConfig, "lang", szLang);

	   AspellCanHaveError *iReturn = new_aspell_speller(g_pConfig);

	   if(aspell_error_number(iReturn) != 0)
	   {
         debug("CmdInput::Setup spell manager creation error %s\n", aspell_error_message(iReturn));
		   delete_aspell_can_have_error(iReturn);

         delete_aspell_config(g_pConfig);
	   }

      g_pSpeller = to_aspell_speller(iReturn);
#endif
   }

   m_pEdit = NULL;

   m_iType = iType;
   m_szTitle = strmk(szTitle);
   m_szExtra = strmk(szExtra);
   m_cReturn = '\0';
}

void CmdInput::LineSetup(const char *szTitle, const char *szExtra, int iMax, int iOptions, const char *szInit, void *pFunction, EDF *pData)
{
   Setup(CMD_TYPE_LINE | iOptions, szTitle, szExtra);

   // debug("CmdInput::LineSetup %s %s\n", szTitle, szExtra);

   m_iLength = 0;
   m_iColumn = 0;
   m_szInit = (byte *)strmk((char *)szInit);
   m_pData = pData;

   if(mask(iOptions, CMD_LINE_MULTI) == true)
   {
      // debug("CmdInput::LineSetup multi line fields %p\n", pFunction);
      m_iMax = CmdWidth();
      m_pFieldsFunc = (CMDFIELDSFUNC)pFunction;
      // debug("CmdInput::CmdInput line max %d\n", m_iMax);

      m_iRowNum = 0;
      m_iNumRows = 1;
      m_bAbort = false;
      m_pRows = new InputLine[1];
      m_pRows[0].m_szLine = new byte[m_iMax + 2];
      memset(m_pRows[0].m_szLine, '\0', m_iMax + 1);
      m_pRows[0].m_iEndType = 0;
      m_pChild = NULL;
   }
   else
   {
      if(iMax != -1)
      {
         m_iMax = iMax;
      }
      else
      {
         m_iMax = CmdWidth() - strlen(szTitle) - 3;
      }
      // debug("CmdInput::CmdInput line max %d\n", m_iMax);
      m_pTabFunc = (CMDTABFUNC)pFunction;

      if(m_szInit != NULL && strlen((char *)m_szInit) > m_iMax)
      {
         m_szInit[m_iMax] = '\0';
      }

      m_szLine = new byte[m_iMax + 2];
      memset(m_szLine, '\0', m_iMax + 1);
      m_iTabValue = -1;
   }
}

void CmdInput::EditSetup(bool bSend)
{
   m_pEdit = new CmdInput(CMD_MENU_SIMPLE | CMD_MENU_NOCASE, "Edit");
   m_pEdit->MenuAdd('l', "List");
   m_pEdit->MenuAdd('c', "Continue");
   if(m_pFieldsFunc != NULL && m_pData != NULL)
   {
      m_pEdit->MenuAdd('f', "Fields");
   }
#ifdef HAVE_LIBASPELL
   m_pEdit->MenuAdd('p', "sPell");
#endif
   if(bSend == true)
   {
      m_pEdit->MenuAdd('s', "Send");
   }
   else
   {
      m_pEdit->MenuAdd('o', "Override");
   }
   m_pEdit->MenuAdd('a', "Abort");
   m_pEdit->MenuAdd('h', "Help");
}

bool CmdInput::MenuFind(int cOption)
{
   STACKTRACE
   int iPos = 0;
   bool bFound = false;

   if(m_bMenuCase == false)
   {
      cOption = tolower(cOption);
   }
   while(bFound == false && iPos < m_iNumOptions)
   {
      if(cOption == m_pOptions[iPos].m_cOption)
      {
         bFound = true;
      }
      else
      {
         iPos++;
      }
   }

   return bFound;
}

byte CmdInput::MenuProcess(byte cOption)
{
   STACKTRACE

   if(CmdInputCheck(cOption) == true)
   {
      if(mask(m_iType, CMD_MENU_ANY) == true)
      {
      }
      else
      {
         if(cOption == ' ' || cOption == '\r' || cOption == '\n')
         {
            if(m_cDefault != '\0')
            {
               cOption = m_cDefault;
            }
            else
            {
               cOption = '\n';
            }
         }
         else
         {
            if(m_bMenuCase == false || mask(m_iType, CMD_MENU_NOCASE) == true)
            {
               cOption = tolower(cOption);
            }
         }

         if(cOption == '\r' || cOption == '\n')
         {
            CmdWrite("\n");
            Show();

            cOption = '\0';
         }
         else if(cOption == '?')
         {
            CmdWrite('?');

            MenuOptions(mask(m_iType, CMD_MENU_SIMPLE));

            cOption = '\0';
         }
         else if(cOption == '+')
         {
            CmdWrite('+');

            MenuOptions(true);

            cOption = '\0';
         }
         else if(!isprint(cOption))
         {
            cOption = '\0';
         }
         else if(MenuFind(cOption) == false)
         {
            CmdWrite((char)(m_bMenuCase == false ? toupper(cOption) : cOption));
            // CmdWrite("\nUnrecognised command. Type ? for help\n");
            if(mask(m_iType, CMD_MENU_SIMPLE) == false)
            {
               CmdWrite("\nUnrecognised command. Type ? for help\n");
               Show();
            }
            else
            {
               MenuOptions(false);
            }

            cOption = '\0';
         }
         else
         {
            CmdWrite((char)(m_bMenuCase == false ? toupper(cOption) : cOption));
            // CmdBack();
            CmdWrite("\n");
         }
      }
   }
   else
   {
      cOption = '\0';
   }

   return cOption;
}

void CmdInput::MenuOptions(bool bHelp)
{
   STACKTRACE

   CmdWrite("\n");

   ShowOptions(bHelp);

   Show();
}

void CmdInput::ShowOptions(bool bHelp)
{
   STACKTRACE
   int iPos = 0;
   char szWrite[5000];

   if(bHelp == true)
   {
      CmdPageOn();
   }

   if(mask(m_iType, CMD_MENU_YESNO) == false)
   {
      strcpy(szWrite, "Commands:\n");
      /* if(m_bExtraHelp == true && bHelp == false)
      {
         strcat(szWrite, " (press '+' for extra help)");
      }
      strcat(szWrite, ":\n"); */
      for(iPos = 0; iPos < m_iNumOptions; iPos++)
      {
         if(m_pOptions[iPos].m_bVisible == true)
         {
            // Allow a space when help is off
            sprintf(szWrite, "%s%s\0374%c\0370 - ", szWrite, bHelp == false ? "  " : "", m_bMenuCase == false ? toupper(m_pOptions[iPos].m_cOption) : m_pOptions[iPos].m_cOption);
            strcat(szWrite, m_pOptions[iPos].m_szOption);

            if(bHelp == true)
            {
               if(m_pOptions[iPos].m_szHelp != NULL && strlen(m_pOptions[iPos].m_szHelp) > 0)
               {
                  strcat(szWrite, ": ");
                  strcat(szWrite, m_pOptions[iPos].m_szHelp);
                  strcat(szWrite, "\n");
               }
            }
            strcat(szWrite, "\n");
         }
      }
      CmdWrite(szWrite);
   }
   else
   {
      strcpy(szWrite, "Please press ");
      for(iPos = 0; iPos < m_iNumOptions; iPos++)
      {
         if(m_pOptions[iPos].m_bVisible == true)
         {
            if(iPos == m_iNumOptions - 1)
            {
               strcat(szWrite, " or ");
            }
            else if(iPos > 0)
            {
               strcat(szWrite, ", ");
            }
            sprintf(szWrite, "%s\0374%c\0370", szWrite, m_pOptions[iPos].m_cOption);
         }
      }
      /* if(m_bExtraHelp == true && bHelp == false)
      {
         strcat(szWrite, " (or '+' for extra help)");
      } */
      strcat(szWrite, "\n");
      CmdWrite(szWrite);
   }

   if(bHelp == true)
   {
      CmdPageOff();
   }
}

byte CmdInput::LineProcess(byte cOption)
{
   STACKTRACE
   int iCharPos = 0, iInputLen = 0, iInputNum = 0, iNumUnknowns = 0;
   bool bLoop = true;
   // char cReturn = '\0';
   byte *szLine = NULL, *szInput = NULL;
   InputLine *pRows = NULL;

   // debug("CmdInput::LineProcess entry %d\n", cOption);
   if(mask(m_iType, CMD_LINE_MULTI) == false)
   {
      szLine = m_szLine;
   }
   else
   {
      szLine = m_pRows[m_iRowNum].m_szLine;
   }

   // debug("CmdInput::LineProcess '%s' (%d of %d)\n", szLine, m_iColumn, m_iMax);

   do
   {
      // debug("CmdInput::LineProcess '%c' [%d]\n", cOption != '\n' ? cOption : '*', cOption);

      if(CmdInputCheck(cOption) == false)
      {
         debug(DEBUGLEVEL_ERR, "CmdInput::LineProcess menu check failed %d\n", cOption);
      }
      else if(cOption == '\002')
      {
         if(m_iColumn > 0)
         {
            // Back one character
            if(mask(m_iType, CMD_LINE_SILENT) == false)
            {
               // CmdWrite('\b');
               CmdBack();
            }
            m_iColumn--;
         }
      }
      else if(cOption == '\006')
      {
         if(m_iColumn < m_iLength)
         {
            // Forwards one character
            if(mask(m_iType, CMD_LINE_SILENT) == false)
            {
               // Redraw the characters after the insertion point
               CmdWrite((char *)&szLine[m_iColumn], CMD_OUT_NOHIGHLIGHT);
            }

            m_iColumn++;

            if(mask(m_iType, CMD_LINE_SILENT) == false)
            {
               // Move back to the insertion point
               for(iCharPos = m_iColumn; iCharPos < m_iLength; iCharPos++)
               {
                  // CmdWrite('\b');
                  CmdBack();
               }
            }
         }
      }
      else if(cOption == '\001')
      {
         // Start of line
         if(mask(m_iType, CMD_LINE_SILENT) == false)
         {
            // Move back to the start point
            for(iCharPos = 0; iCharPos < m_iColumn; iCharPos++)
            {
               // CmdWrite('\b');
               CmdBack();
            }
         }
         m_iColumn = 0;
      }
      else if(cOption == '\005')
      {
         // End of line
         if(mask(m_iType, CMD_LINE_SILENT) == false)
         {
            // Write text to the end point
            CmdWrite((char *)&szLine[m_iColumn], CMD_OUT_NOHIGHLIGHT);
         }
         m_iColumn = m_iLength;
      }
      else if(cOption == '\016' || cOption == '\020' || cOption == '\025')
      {
         if((cOption == '\016' && mask(m_iType, CMD_LINE_MULTI) == true && m_iRowNum < m_iNumRows - 1) ||
            ((cOption == '\020' || cOption == '\025') && mask(m_iType, CMD_LINE_MULTI) == true && m_iRowNum > 0))
         {
            // Line movement
            CmdWrite("\n");

            if(cOption == '\016')
            {
               m_iRowNum++;
            }
            else
            {
               m_iRowNum--;
            }
            CmdWrite(m_pRows[m_iRowNum].m_szLine, CMD_OUT_NOHIGHLIGHT);

            m_iLength = strlen((char *)m_pRows[m_iRowNum].m_szLine);
            m_iColumn = m_iLength;

            szLine = m_pRows[m_iRowNum].m_szLine;
         }
      }
      else if(cOption == '\013')
      {
         if(mask(m_iType, CMD_LINE_MULTI) == true)
         {
            // Kill line
            // debug("CmdInput::LineProcess kill line %d\n", m_iRowNum);
            int iPos = 0;

            CmdWrite("\n");

            if(m_iRowNum > 0 || m_iNumRows > 1)
            {
               // debug("CmdInput::LineProcess remove line\n");

               pRows = new InputLine[m_iNumRows];

               // Copy rows before the current one
               memcpy(pRows, m_pRows, (m_iRowNum + 1) * sizeof(InputLine));

               m_iNumRows--;
               if(m_iRowNum == m_iNumRows)
               {
                  // This is the last row, move the current row up too
                  // debug("CmdInput::LineProcess move current\n");
                  m_iRowNum--;
               }
               else
               {
                  // Copy rows after the current one
                  memcpy((void *)&pRows[m_iRowNum], (void *)&m_pRows[m_iRowNum + 1], (m_iNumRows - m_iRowNum) * sizeof(InputLine));
               }

               /* for(int iTemp = 0; iTemp < m_iNumRows; iTemp++)
               {
                  debug("CmdInput::LineProcess line %d, %s\n", iTemp, pRows[iTemp].m_szLine);
               } */

               szLine = pRows[m_iRowNum].m_szLine;
               // debug("CmdInput::LineProcess new line %s\n", szLine);
               m_iLength = strlen((char *)szLine);
               m_iColumn = 0;

               CmdWrite(szLine, CMD_OUT_NOHIGHLIGHT);
               for(iPos = 0; iPos < m_iLength; iPos++)
               {
                  // CmdWrite('\b');
                  CmdBack();
               }

               delete[] m_pRows;
               m_pRows = pRows;
            }
            else
            {
               // Blank the only line
               // debug("CmdInput::LineProcess blank line\n");

               memset(szLine, '\0', m_iLength);
               m_iColumn = 0;
               m_iLength = 0;
            }
         }
         else
         {
            int iPos = 0;

            if(mask(m_iType, CMD_LINE_SILENT) == false)
            {
               for(iPos = 0; iPos < m_iColumn; iPos++)
               {
                  // CmdWrite('\b');
                  CmdBack();
               }
               for(iPos = 0; iPos < m_iLength; iPos++)
               {
                  CmdWrite(' ');
               }
               for(iPos = 0; iPos < m_iLength; iPos++)
               {
                  // CmdWrite('\b');
                  CmdBack();
               }
            }

            memset(szLine, '\0', m_iLength);
            m_iColumn = 0;
            m_iLength = 0;
            m_iTabValue = -1;
         }
      }
      else if(cOption == '\030')
      {
         if(cOption == '\030' && mask(m_iType, CMD_LINE_MULTI) == true && m_iColumn == m_iLength &&
            m_iRowNum < m_iNumRows - 1 && strlen((char *)m_pRows[m_iRowNum].m_szLine) + strlen((char *)m_pRows[m_iRowNum + 1].m_szLine) < m_iMax)
         {
            // Join line
            int iLen = strlen((char *)m_pRows[m_iRowNum + 1].m_szLine), iPos = 0;

            CmdWrite(m_pRows[m_iRowNum + 1].m_szLine, CMD_OUT_NOHIGHLIGHT);
            for(iPos = 0; iPos < iLen; iPos++)
            {
               // CmdWrite('\b');
               CmdBack();
            }

            pRows = new InputLine[m_iNumRows];

            // Copy rows before the current one
            memcpy(pRows, m_pRows, (m_iRowNum + 1) * sizeof(InputLine));

            // Append to the current row
            // debug("CmdInput::LineProcess append %s to %d\n", m_pRows[m_iRowNum + 1].m_szLine, m_iLength);
            memcpy((char *)&pRows[m_iRowNum].m_szLine[m_iLength], m_pRows[m_iRowNum + 1].m_szLine, iLen);
            m_iLength += iLen;

            m_iNumRows--;

            if(m_iRowNum < m_iNumRows - 1)
            {
               // Copy rows after the current one
               memcpy((void *)&pRows[m_iRowNum + 1], (void *)&m_pRows[m_iRowNum + 2], (m_iNumRows - m_iRowNum - 1) * sizeof(InputLine));
            }

            delete[] m_pRows;
            m_pRows = pRows;
         }
      }
      else if((cOption == '\b' || cOption == '\177'))
      {
         if(m_iColumn > 0)
         {
            // Backspace
            if(mask(m_iType, CMD_LINE_SILENT) == false)
            {
               // Move back to the insertion point
               // CmdWrite('\b');
               CmdBack();
            }

            if(m_iColumn < m_iLength)
            {
               // Copy characters after insertion point
               memmove((char *)&szLine[m_iColumn - 1], (char *)&szLine[m_iColumn], m_iLength - m_iColumn);
            }
            szLine[m_iLength - 1] = '\0';

            if(mask(m_iType, CMD_LINE_SILENT) == false)
            {
               CmdWrite((char *)&szLine[m_iColumn - 1], CMD_OUT_NOHIGHLIGHT);
               // Blank the end character
               CmdWrite(' ');
               // Move back to the insertion point
               for(iCharPos = m_iColumn - 1; iCharPos < m_iLength; iCharPos++)
               {
                  // CmdWrite('\b');
                  CmdBack();
               }
            }

            m_iColumn--;
            m_iLength--;
            m_iTabValue = -1;
         }
      }
      else if(cOption == '\004')
      {
         if(m_iColumn < m_iLength)
         {
            // Delete
            memmove((char *)&szLine[m_iColumn], (char *)&szLine[m_iColumn + 1], m_iLength - m_iColumn - 1);
            szLine[m_iLength - 1] = '\0';
            if(mask(m_iType, CMD_LINE_SILENT) == false)
            {
               CmdWrite((char *)&szLine[m_iColumn], CMD_OUT_NOHIGHLIGHT);
            }

            if(mask(m_iType, CMD_LINE_SILENT) == false)
            {
               // Blank the end character and move back to the insertion point
               CmdWrite(' ');
               for(iCharPos = m_iColumn; iCharPos < m_iLength; iCharPos++)
               {
                  // CmdWrite('\b');
                  CmdBack();
               }
            }
            m_iLength--;
            m_iTabValue = -1;
         }
      }
      else if(cOption == '\027')
      {
         if(m_iColumn > 0)
         {
            // Delete word
            int iStartPos = m_iColumn;

            while(iStartPos > 0 && szLine[iStartPos - 1] == ' ')
            {
               iStartPos--;
            }
            while(iStartPos > 0 && szLine[iStartPos - 1] != ' ')
            {
               iStartPos--;
            }
            // debug("CmdInput::LineProcess word position %d (%d of %d), '%s'\n", iStartPos, m_iColumn, m_iLength, (char *)&szLine[iStartPos]);

            if(mask(m_iType, CMD_LINE_SILENT) == false)
            {
               for(iCharPos = iStartPos; iCharPos < m_iColumn; iCharPos++)
               {
                  // CmdWrite('\b');
                  CmdBack();
               }
               CmdWrite((char *)&szLine[m_iColumn], CMD_OUT_NOHIGHLIGHT);
            }
            // debug("CmdInput::LineProcess blank %d chars at %d, %s\n", m_iColumn - iStartPos, m_iLength - (m_iColumn - iStartPos), (char *)&szLine[m_iLength - (m_iColumn - iStartPos)]);
            memmove((char *)&szLine[iStartPos], (char *)&szLine[m_iColumn], m_iLength - iStartPos);
            memset((char *)&szLine[m_iLength - (m_iColumn - iStartPos)], '\0', m_iColumn - iStartPos);
            if(mask(m_iType, CMD_LINE_SILENT) == false)
            {
               for(iCharPos = iStartPos; iCharPos < m_iColumn; iCharPos++)
               {
                  CmdWrite(' ');
               }
               for(iCharPos = iStartPos; iCharPos < m_iLength; iCharPos++)
               {
                  // CmdWrite('\b');
                  CmdBack();
               }
            }

            m_iLength -= (m_iColumn - iStartPos);
            m_iColumn = iStartPos;
            m_iTabValue = -1;
         }
      }
      else if(cOption == '\r' || cOption == '\n')
      {
         if((mask(m_iType, CMD_LINE_MULTI) == false || mask(m_iType, CMD_LINE_NOTITLE) == true))
         {
            // Return
            // CmdWrite("\n");
            CmdReturn();

            m_cReturn = '\n';
            bLoop = false;

            // debug("CmdInput::LineProcess ending loop (end char [%d])\n", cOption);
         }
         else if(mask(m_iType, CMD_LINE_MULTI) == true)
         {
            // Return
            char *szCopy = NULL;
            int iCopy = 0, iPos = 0;

            if(strcmp((char *)szLine, ".") == 0 && m_iRowNum == m_iNumRows - 1)
            {
               if(m_iNumRows > 1)
               {
                  m_iNumRows--;
               }
               else
               {
                  // debug("CmdInput::LineProcess only line, blank %d chars in '%s'\n", m_iLength, m_pRows[m_iRowNum].m_szLine);
                  memset(m_pRows[m_iRowNum].m_szLine, '\0', m_iLength);
               }

               CmdWrite("\n");
               CmdWrite(DASHES);

#if HAVE_LIBASPELL
               if(SpellCheck() == true)
               {
                  iNumUnknowns = LineSpell();
               }
#endif

               EditSetup(iNumUnknowns == 0);
               m_pChild = m_pEdit;
               m_pChild->Show();

               bLoop = false;
            }
            else
            {
               if(m_iColumn < m_iLength)
               {
                  // debug("CmdInput::LineProcess move line %s\n", (char *)&m_pRows[m_iRowNum].m_szLine[m_iColumn]);
                  iCopy = m_iLength - m_iColumn;
                  szCopy = strmk((char *)&m_pRows[m_iRowNum].m_szLine[m_iColumn], 0, iCopy);
                  memset((char *)&m_pRows[m_iRowNum].m_szLine[m_iColumn], '\0', iCopy);

                  for(iPos = 0; iPos < iCopy; iPos++)
                  {
                     CmdWrite(' ');
                  }
                  for(iPos = 0; iPos < iCopy; iPos++)
                  {
                     // CmdWrite('\b');
                     CmdBack();
                  }
               }

               CmdWrite("\n");

               pRows = new InputLine[m_iNumRows + 1];

               // Copy rows before the current one
               memcpy(pRows, m_pRows, (m_iRowNum + 1) * sizeof(InputLine));

               pRows[m_iRowNum].m_iEndType = CMD_LINE_RETEND;

               m_iRowNum++;
               m_iNumRows++;

               // Set up the new row
               pRows[m_iRowNum].m_szLine = new byte[m_iMax + 2];
               pRows[m_iRowNum].m_iEndType = CMD_LINE_RETEND;
               memset(pRows[m_iRowNum].m_szLine, '\0', m_iMax + 1);
               if(szCopy != NULL)
               {
                  memcpy(pRows[m_iRowNum].m_szLine, szCopy, iCopy);
                  CmdWrite(szCopy, CMD_OUT_NOHIGHLIGHT);
                  for(iPos = 0; iPos < iCopy; iPos++)
                  {
                     // CmdWrite('\b');
                     CmdBack();
                  }
               }
               m_iLength = 0;
               m_iColumn = 0;

               if(m_iRowNum < m_iNumRows - 1)
               {
                  // Copy rows after the current one
                  memcpy((void *)&pRows[m_iRowNum + 1], (void *)&m_pRows[m_iRowNum], (m_iNumRows - m_iRowNum - 1) * sizeof(InputLine));
               }
               delete[] m_pRows;
               m_pRows = pRows;

               szLine = m_pRows[m_iRowNum].m_szLine;

               delete[] szCopy;
            }
         }
      }
      else if(cOption == '\022')
      {
         // Force hard wrap
         debug(DEBUGLEVEL_DEBUG, "CmdInput::LineProcess hard wrap %s %d\n", BoolStr(mask(m_iType, CMD_LINE_MULTI)), m_iRowNum);

         if(mask(m_iType, CMD_LINE_MULTI) == true && m_iRowNum > 0)
         {
            m_pRows[m_iRowNum - 1].m_iEndType = CMD_LINE_RETEND;
         }
      }
      else if(cOption == '\033')
      {
         if(mask(m_iType, CMD_LINE_MULTI) == true && mask(m_iType, CMD_LINE_NOTITLE) == false)
         {
            /* if(m_iRowNum < m_iNumRows - 1)
            {
               printf("CmdInput::LineProcess return ending row %d\n", m_iRowNum);
               pRows[m_iRowNum].m_iEndType = CMD_LINE_RETEND;
            } */

            // Escape
            CmdWrite("\n");
            CmdWrite(DASHES);

#if HAVE_LIBASPELL
            if(SpellCheck() == true)
            {
               iNumUnknowns = LineSpell();
            }
#endif

            EditSetup(iNumUnknowns == 0);
            m_pChild = m_pEdit;
            m_pChild->Show();

            bLoop = false;
         }
         else if(mask(m_iType, CMD_LINE_MULTI) == false && mask(m_iType, CMD_LINE_NOESCAPE) == false)
         {
            // debug("CmdInput::LineProcess single line escape\n");

            CmdWrite("\n");

            m_cReturn = '\033';
            bLoop = false;
         }
      }
      else if(cOption == '\t')
      {
         if(mask(m_iType, CMD_LINE_TAB) == true && m_pTabFunc != NULL)
         {
            // Tab complete
            int iPos = 0;
            char *szTemp = NULL;

            szTemp = (*m_pTabFunc)(m_pData, (char *)szLine, m_iColumn, false, &m_iTabValue);
            if(szTemp != NULL)
            {
               for(iPos = m_iColumn; iPos < m_iLength; iPos++)
               {
                  CmdWrite(' ');
               }
               for(iPos = 0; iPos < m_iLength; iPos++)
               {
                  // CmdWrite('\b');
                  CmdBack();
               }
               CmdWrite(szTemp, CMD_OUT_NOHIGHLIGHT);

               // debug("CmdInput::LineProcess tab set %d\n", m_iLineWidth);
               memset(szLine, '\0', m_iLength);
               m_iLength = strlen(szTemp);
               memcpy(szLine, szTemp, m_iLength);

               for(iPos = m_iColumn; iPos < m_iLength; iPos++)
               {
                  // CmdWrite('\b');
                  CmdBack();
               }

               delete[] szTemp;
            }
         }
         else
         {
            // Tab
         }
      }
      /* else if(CmdIOType() == 2 && cOption == IAC)
      {
         // debug("CmdInput::LineProcess IAC\n");
      } */
      /* else if(CmdIOType() == 2 && cOption == '\n')
      {
         // do nothing
         printf("thing!\n");
      } */
      else if(cOption >= 32 && cOption <= 126)
      {
			if(!isprint(cOption))
			{
         	debug(DEBUGLEVEL_DEBUG, "CmdInput::LineProcess warning non-printable char %d\n", cOption);
			}

         // Printable character
         if(mask(m_iType, CMD_LINE_MULTI) == true && m_iColumn == m_iMax - 1)
         {
            int iSpace = m_iColumn - 1, iPos = 0;

            pRows = new InputLine[m_iNumRows + 1];
            // Copy rows before the current one
            memcpy(pRows, m_pRows, (m_iRowNum + 1) * sizeof(InputLine));

            while(iSpace >= 0 && szLine[iSpace] != ' ')
            {
               iSpace--;
            }
            if(iSpace >= 0)
            {
               // Blank the current word
               // debug("CmdInput::LineProcess wrap character %d, '%s'\n", iSpace, (char *)&szLine[iSpace + 1]);

               for(iPos = m_iColumn - 1; iPos > iSpace; iPos--)
               {
                  // CmdWrite('\b');
                  CmdBack();
               }
               for(iPos = m_iColumn - 1; iPos > iSpace; iPos--)
               {
                  CmdWrite(' ');
               }

               // rjp 20010728, hard wraps instead
               if(mask(m_iHardWrap, CMD_WRAP_EDIT) == true)
               {
                  pRows[m_iRowNum].m_iEndType = CMD_LINE_RETEND;
               }
               else
               {
                  pRows[m_iRowNum].m_iEndType = CMD_LINE_WRAPSPACE;
               }
            }
            else
            {
               // debug("CmdInput::LineProcess no wrapping point 1\n");
               if(mask(m_iHardWrap, CMD_WRAP_EDIT) == true)
               {
                  pRows[m_iRowNum].m_iEndType = CMD_LINE_RETEND;
               }
               else
               {
                  pRows[m_iRowNum].m_iEndType = CMD_LINE_WRAPNOSPACE;
               }
            }
            CmdWrite("\n");

            m_iRowNum++;
            m_iNumRows++;

            pRows[m_iRowNum].m_szLine = new byte[m_iMax + 2];
            pRows[m_iRowNum].m_iEndType = 0;
            memset(pRows[m_iRowNum].m_szLine, '\0', m_iMax + 1);
            if(iSpace >= 0)
            {
               // Insert a new row with the incomplete word
               memcpy(pRows[m_iRowNum].m_szLine, (char *)&szLine[iSpace + 1], m_iColumn - 1 - iSpace);

               // Remove the incomplete word from the current line
               memset((char *)&szLine[iSpace], '\0', m_iColumn - iSpace);
            }

            if(m_iRowNum < m_iNumRows - 1)
            {
               // Copy rows after the current one
               memcpy((void *)&pRows[m_iRowNum + 1], (void *)&m_pRows[m_iRowNum], (m_iNumRows - m_iRowNum - 1) * sizeof(InputLine));
            }

            delete[] m_pRows;
            m_pRows = pRows;

            if(iSpace >= 0)
            {
               // Set the current line pointer
               pRows[m_iRowNum].m_szLine[m_iColumn - 1 - iSpace] = cOption;
               szLine = pRows[m_iRowNum].m_szLine;
               CmdWrite(szLine, CMD_OUT_NOHIGHLIGHT);

               m_iColumn = strlen((char *)szLine);
               m_iLength = m_iColumn;
            }
            else
            {
               // debug("CmdInput::LineProcess no wrapping point 2\n");
               pRows[m_iRowNum].m_szLine[0] = cOption;
               szLine = pRows[m_iRowNum].m_szLine;
               CmdWrite(szLine, CMD_OUT_NOHIGHLIGHT);
               m_iColumn = 1;
               m_iLength = 1;
            }
         }
         else if(m_iLength < m_iMax)
         {
            // debug("CmdInput::LineProcess add %d (max %d)\n", m_iLength, m_iMax);
            if(mask(m_iType, CMD_LINE_SILENT) == false)
            {
               // Add current character
               CmdWrite((char )cOption);
            }
            if(m_iColumn < m_iLength)
            {
               // Copy characters after the insertion point
               memmove((char *)&szLine[m_iColumn + 1], (char *)&szLine[m_iColumn], m_iLength - m_iColumn);
            }
            szLine[m_iColumn] = cOption;

            if(m_iColumn < m_iLength)
            {
               if(mask(m_iType, CMD_LINE_SILENT) == false)
               {
                  CmdWrite((char *)&szLine[m_iColumn + 1], CMD_OUT_NOHIGHLIGHT);
                  for(iCharPos = m_iLength - m_iColumn; iCharPos > 0; iCharPos--)
                  {
                     // CmdWrite('\b');
                     CmdBack();
                  }
               }
            }

            m_iColumn++;
            m_iLength++;
            m_iTabValue = -1;
         }
      }
      else
      {
         debug(DEBUGLEVEL_WARN, "CmdInput::LineProcess bad character '%c' [%d] (%d %d)\n", isprint(cOption) || cOption == ' ' ? cOption : '|', cOption, mask(m_iType, CMD_LINE_MULTI) == true ? m_iRowNum : -1, m_iColumn);
      }

      // debug("CmdInput %d '%s' %d\n", cOption, szInput, iInputNum);
      if(bLoop == true && szInput == NULL)
      {
         iInputLen = CmdInputLen();
         if(iInputLen == 0)
         {
            bLoop = false;
         }
         /* if(szInput != NULL)
         {
            debug("CmdInput::LineProcess read %d bytes into buffer\n", iInputLen);
         } */
      }
      if(bLoop == true)
      {
         if(iInputNum == iInputLen)
         {
            // debug("CmdInput::LineProcess end of buffer\n");
            CmdInputRelease(iInputLen);
            bLoop = false;
         }
         else
         {
            if(bLoop == true)
            {
               cOption = CmdInputChar(iInputNum++);
            }
         }
      }
      else if(bLoop == false)
      {
         CmdInputRelease(iInputNum);
      }
   }
   while(bLoop == true);

   if(szInput != NULL)
   {
      delete[] szInput;
   }

   // debug("CmdInput::LineProcess exit %d\n", m_cReturn);
   return m_cReturn;
}

#ifdef HAVE_LIBASPELL
int CmdInput::LineSpell()
{
   STACKTRACE
   int iRowNum = 0, iTotalErrors = 0;

   for(iRowNum = 0; iRowNum < m_iNumRows; iRowNum++)
   {
      iTotalErrors += CheckString((char *)m_pRows[iRowNum].m_szLine);
   }

   if(iTotalErrors == 0)
   {
      CmdWrite("No unrecognised words\n");
   }

   return iTotalErrors;
}

int CmdInput::CheckString(char *szString)
{
   STACKTRACE
   int iErrors = 0;
   bool bLoop = false, bFirst = true;
   char szWrite[200];
   char *szPos = szString, *szWord = NULL, *szSuggestion = NULL;
   EDF *pSuggestions = NULL;

   while(szPos != NULL)
   {
      szWord = NextWord(&szPos);

      if(szWord != NULL)
      {
         debug("CmdInput::CheckString word '%s'\n", szWord);
         if(CheckWord(szWord) == false)
         {
            iErrors++;
            bFirst = true;

            sprintf(szWrite, "\0373%s\0370 is not recognised", szWord);
            CmdWrite(szWrite);

            pSuggestions = SuggestWords(szWord);
            if(pSuggestions->Children("suggestion") > 0)
            {
               CmdWrite(" (suggestions ");
               bLoop = pSuggestions->Child("suggestion");
               while(bLoop == true)
               {
                  pSuggestions->Get(NULL, &szSuggestion);
                  sprintf(szWrite, "%s\0373%s\0370", bFirst == true ? "" : ", ", szSuggestion);
                  CmdWrite(szWrite);
                  delete[] szSuggestion;

                  bLoop = pSuggestions->Next("suggestion");
                  if(bLoop == false)
                  {
                     pSuggestions->Parent();
                  }

                  bFirst = false;
               }
               CmdWrite(")\n");
            }
            else
            {
               CmdWrite(" (no suggestions)\n");
            }
         }
      }
   }

   return iErrors;
}

#define BREAK " \r\n!\"£$%^&*()-=[];#,./_+{}:@~<>?\\|\t`¬"

char *CmdInput::NextWord(char **szString)
{
   STACKTRACE
   bool bSlash = false;
   int iWordLen = 0;
   char *szPos = *szString, *szReturn = NULL;

   debug("CmdInput::NextWord entry '%s'\n", *szString);

   // Find word start
   while(*szPos != '\0' && (strchr(BREAK, *szPos) != NULL || *szPos == '\''))
   {
      szPos++;

      if(strnicmp(szPos, "www.", 4) == 0 ||
      strnicmp(szPos, "http://", 7) == 0 ||
      strnicmp(szPos, "https://", 8) == 0 ||
      strnicmp(szPos, "ftp://", 6) == 0)
      {
         // Skip over the URL
         while(*szPos != '\0' &&
         (isalnum(*szPos) || strchr("-./", *szPos) != NULL ||
         (bSlash == false && *szPos == ':') || (bSlash == true && strchr("~_?%=&,:+#@';()", *szPos) != NULL)))
         {
            if(*szPos == '/')
            {
               bSlash = true;
            }

            szPos++;
         }
      }
   }

   if(*szPos == '\0')
   {
      *szString = NULL;

      debug("CmdInput::NextWord exit NULL\n");
      return NULL;
   }

   // Find word end
   while(szPos[iWordLen] != '\0' && !(strchr(BREAK, szPos[iWordLen]) != NULL || (szPos[iWordLen] == '\'' && strchr(BREAK, szPos[iWordLen + 1]) != NULL)))
   {
      iWordLen++;
   }
   debug("CmdInput::NextWord %d for %d chars\n", szPos - *szString, iWordLen);
   szReturn = strmk(szPos, 0, iWordLen);

   *szString = szPos + iWordLen;

   debug("CmdInput::NextWord exit '%s', '%s'\n", szReturn, *szString);
   return szReturn;
}

char *szEndings[] =
{
    "'s", "am", "esque", "ft", "gb", "ghz", "ish", "k", "kb", "khz", "m", "mb", "mhz", "mph", "nd", "p", "pm", "rd", "s", "st", "th", "x", NULL
};

bool CmdInput::CheckWord(char *szWord)
{
   STACKTRACE
   int iCheck = 0, iEndingNum = 0;
   bool bDigits = true, bEnding = false, bUppers = true, bPlural = false;
   char *szPos = NULL;

   /* if(strlen(szWord) <= 2)
   {
      return true;
   } */

   // Number or upper case?
   szPos = szWord;
   while(*szPos != '\0' && (bDigits == true || bUppers == true))
   {
      if(bDigits == true && !isdigit(*szPos))
      {
	      // Could be an ending
	      while(bEnding == false && szEndings[iEndingNum] != NULL)
	      {
            if(stricmp(szPos, szEndings[iEndingNum]) == 0)
            {
               bEnding = true;
            }
            else
            {
               iEndingNum++;
            }
         }

         bDigits = false;
      }
      if(bUppers == true && !isupper(*szPos))
      {
         // Could be a plural acronym
         if(*szPos == 's')
         {
            bPlural = true;
         }

         bUppers = false;
      }

      szPos++;
   }

   // Digits or endings are OK
   if(bDigits == true || bEnding == true)
   {
      return true;
   }

   // Acronyms are OK (more than 4 letters is probably shouting though)
   if(strlen(szWord) <= 4 && (bUppers == true || bPlural == true))
   {
      return true;
   }

   // Check the spelling
   iCheck = aspell_speller_check(g_pSpeller, szWord, -1);
   if(iCheck == 1)
   {
      return true;
   }
   else if(iCheck == -1)
   {
      debug("CmdInput::CheckWord error on %s\n", szWord);
   }

   return false;
}

EDF *CmdInput::SuggestWords(char *szWord)
{
   const char *szSuggestion = NULL;
   const AspellWordList *pSuggestions = aspell_speller_suggest(g_pSpeller, szWord, strlen(szWord));
   AspellStringEnumeration *pElements = aspell_word_list_elements(pSuggestions);
   EDF *pReturn = new EDF();

   while((szSuggestion = aspell_string_enumeration_next(pElements)) != 0)
   {
      pReturn->AddChild("suggestion", szSuggestion);
   }

   delete_aspell_string_enumeration(pElements);

   return pReturn;
}
#endif
