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
** CmdIO-unix.cpp: Implementation of Unix specific I/O functions
*/

#include "ua.h"

#ifdef HAVE_LIBNCURSESW
#define _XOPEN_SOURCE_EXTENDED
#endif

#if HAVE_NCURSES_CURSES_H
#include <ncurses/curses.h>
#include <ncurses/term.h>
#include <locale.h>
#include <math.h>
#endif

#if 1
#include <ncursesw/ncurses.h>
#include <ncursesw/term.h>
#include <locale.h>
#include <math.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <utmp.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif

#include "Conn/EDFConn.h"

#include "CmdIO.h"
#include "CmdIO-common.h"
#include "CmdInput.h"
#include "CmdMenu.h"

#include "qUAck.h"

#define UNIX_INPUT_BUFFER 500

typedef void (*SIGHANDLER)(int);
SIGHANDLER m_pSIGWINCH = NULL;

// char *m_szClientName = NULL;
// char m_szClientName[64];

int m_iBackground = -1, m_iForeground = -1;

char *m_szBrowser = NULL;
bool m_bBrowserWait = false;

// char *m_szEditor = NULL;

// Bit hacky but easier than including qUAck.h
// int CmdServerVersion(const char *szVersion);
CmdInput *CmdInputLoop(int iMenuStatus, CmdInput *pInput, byte *pcInput, byte **pszInput);

/* char *CLIENT_NAME()
{
   // if(m_szClientName == NULL)
   {
      // m_szClientName = new char[20];
      sprintf(m_szClientName, "%s %s (Unix)", CLIENT_BASE, CLIENT_VERSION);
   }

   return m_szClientName;
} */

char *CLIENT_SUFFIX()
{
   return "";
}

char *CLIENT_PLATFORM()
{
   return " (Unix)";
}

void initialise_curses(void)
{
#ifdef BUILDWIDE
      setlocale(LC_CTYPE, "");
#endif
      /* maybe this will fix the resize problem */
      unsetenv("LINES");
      unsetenv("COLUMNS");

      initscr();

#ifdef LEAVE_SCROLLBACK
#define isprivate(s) ((s) != 0 && strstr(s, "\033[?") != 0)
    if (isprivate(enter_ca_mode) && isprivate(exit_ca_mode)) {
	    (void) putp(exit_ca_mode);
	    (void) putp(clear_screen);
	    enter_ca_mode = 0;
	    exit_ca_mode = 0;
    }
#endif

      if(has_colors() == TRUE)
      {
         if(start_color() != ERR)
         {
            use_default_colors();
         }
         else
         {
            debug(DEBUGLEVEL_WARN, "CmdStartup start_color failed\n");
         }
      }
      else
      {
         debug(DEBUGLEVEL_WARN, "CmdStartup has_colors false\n");
      }

      noraw();
      cbreak();
      noecho();
      scrollok(stdscr, TRUE);
      idlok(stdscr, TRUE);
      keypad(stdscr, TRUE);

      intrflush(stdscr, FALSE);
      // nodelay(stdscr, TRUE);
      // halfdelay(10);
      timeout(50);

      def_prog_mode();

      // debug("CmdStartup color (%ld %ld)\n", stdscr->_attrs, stdscr->_bkgd);

      /* for(iColourNum = 0; iColourNum <= 9; iColourNum++)
      {
         iF = -1;
         iB = -1;
         pair_content(iColourNum, &iF, &iB);
         debug("CmdStartup colour %d: %d %d\n", iColourNum, iF, iB);
      } */

      // pair_content(1, &iForeground, &iBackground);
      CmdBackground(0);
}

void resize_curses(void)
{
    int eLines = -1, eCols = -2;
    char *x;
    int i_mx, i_my;

    getmaxyx(stdscr, i_my, i_mx);
    
    x = getenv("LINES"); if (x != NULL) { eLines = atoi(x); }
    x = getenv("COLUMNS"); if (x != NULL) { eCols = atoi(x); }
    debug(DEBUGLEVEL_INFO, "resize(%d, %d) -> %d, %d -> %d, %d\n", COLS, LINES, eCols, eLines, i_mx, i_my);
    resizeterm(LINES, COLS);     

}

void CmdStartup(int iSignal)
{
   // short iBackground = COLOR_BLACK, iForeground = COLOR_WHITE;
   // short iF = -1, iB = -1;
   // int iColourNum = 0;

   debug(DEBUGLEVEL_INFO, "CmdStartup entry\n");

   /* if(iSignal == SIGWINCH)
   {
      endwin();

      (*m_pSIGWINCH)(SIGWINCH);
   } */

   if(iSignal != SIGWINCH)
   {
      debug(DEBUGLEVEL_INFO, "CmdStartup init\n");
      initialise_curses();
   }
   else
   {
   /*   (*m_pSIGWINCH)(SIGWINCH); */
      debug(DEBUGLEVEL_INFO, "CmdStartup SIGWINCH\n");
      resize_curses();
   }

   CmdReset(iSignal);

   /* reset the signal */
//   m_pSIGWINCH = signal(SIGWINCH, CmdStartup);
   debug(DEBUGLEVEL_INFO, "CmdStartup exit\n");
}

void CmdReset(int iSignal)
{
   debug(DEBUGLEVEL_INFO, "CmdReset %d x %d\n", COLS, LINES);

   CmdWidth(COLS);
   CmdHeight(LINES);
}

void CmdShutdown(int iSignal)
{
   endwin();
}

int CmdType()
{
   return 1;
}

bool CmdLocal()
{
   return true;
}

char *CmdUsername()
{
   return NULL;
}

// Input functions
int CmdInputGet(byte **pCurrBuffer, int iBufferLen)
{
   int iRead = 0;
   byte *pBuffer = NULL;

   pBuffer = new byte[UNIX_INPUT_BUFFER];
   if(iBufferLen > 0)
   {
      memcpy(pBuffer, (*pCurrBuffer), iBufferLen);
   }
   delete[] (*pCurrBuffer);
   *pCurrBuffer = pBuffer;

   while(iRead != ERR && iBufferLen < UNIX_INPUT_BUFFER)
   {
      // Get a character (fails straight away if no characters are waiting)
      iRead = getch();
      if(iRead != ERR)
      {
         debug("CmdInputGet %c(%d,%o)\n", isalnum(iRead) ? iRead : '-', iRead, iRead);

         if(iRead == KEY_BREAK)
         {
            pBuffer[iBufferLen++] = '\033';
         }
         else if(iRead == KEY_DOWN)
         {
            pBuffer[iBufferLen++] = '\016';
         }
         else if(iRead == KEY_UP)
         {
            pBuffer[iBufferLen++] = '\020';
         }
         else if(iRead == KEY_LEFT)
         {
            pBuffer[iBufferLen++] = '\002';
         }
         else if(iRead == KEY_RIGHT)
         {
            pBuffer[iBufferLen++] = '\006';
         }
         else if(iRead == KEY_HOME)
         {
            pBuffer[iBufferLen++] = '\001';
         }
         else if(iRead == KEY_END)
         {
            pBuffer[iBufferLen++] = '\005';
         }
         else if(iRead == KEY_BACKSPACE)
         {
            debug("CmdInputGet backspace\n");
            pBuffer[iBufferLen++] = '\b';
         }
         else if(iRead == KEY_DC)
         {
            debug("CmdInputGet delete key\n");
            pBuffer[iBufferLen++] = '\004';
         }
         else
         {
            pBuffer[iBufferLen++] = iRead;
         }
      }
   }

   return iBufferLen;
}

bool CmdInputCheck(byte cOption)
{
   return true;
}

#ifdef BUILDWIDE
int powconv(unsigned char cSource, int iWidth, int iMask, int iDestPow)
{
   int iPow = 0, iRes1 = 0, iRes2 = 0, iReturn = 0;

   // printf("powconv entry %d %d %d %d\r\n", cChar, iWidth, iMask, iDestPow);

   if(iMask == 0 || cSource & iMask)
   {
      while(iWidth >= 0)
      {
         iRes1 = pow(2, iPow);
         iRes2 = pow(2, iPow + iDestPow);

         // printf("powconv %d?\r\n", iRes1);
         if(cSource & iRes1)
         {
            // printf("powconv %d -> %d\r\n", iPow, iRes2);
            iReturn += iRes2;
         }

         iPow++;
         iWidth--;
      }
   }
   else
   {
      printw("powconv invalid mask\r\n");
   }

   // printf("powconv exit %d\r\n", iReturn);
   return iReturn;
}

unsigned char maskcheck(int iSource, int iSourcePow, int iWidth)
{
   int iPow = 0, iRes1 = 0, iRes2 = 0;
   unsigned char cReturn = '\0';

   while(iWidth >= 0)
   {
      iRes1 = pow(2, iPow + iSourcePow);
      iRes2 = pow(2, iPow);

      if(iSource & iRes1)
      {
         cReturn += iRes2;
      }

      iPow++;
      iWidth--;
   }

   return cReturn;
}

unsigned char *CmdEncode(unsigned int iChar)
{
   unsigned char *szReturn = NULL;

   szReturn = new unsigned char[7];
   memset(szReturn, 0, 7);

   if(iChar <= 127)
   {
      szReturn[0] = iChar;
   }
   else if(iChar >= 0x80 && iChar <= 0x7FF)
   {
      // 1 extra char, 110xxxxx 10xxxxxx (UCS-4 0000 0080-0000 007FF)
      szReturn[1] = 128;
      szReturn[1] += maskcheck(iChar, 0, 6);
      szReturn[0] = 192;
      szReturn[0] += maskcheck(iChar, 6, 5);
   }
   else if(iChar >= 0x800 && iChar <= 0xFFFF)
   {
      // 2 extra chars, 1110xxxx 10xxxxxx 10xxxxxx (UCS-4 0000 0800-0000 FFFF)
      szReturn[2] = 128;
      szReturn[2] += maskcheck(iChar, 0, 6);
      szReturn[1] = 128;
      szReturn[1] += maskcheck(iChar, 6, 6);
      szReturn[0] = 224;
      szReturn[0] += maskcheck(iChar, 12, 4);
   }
   else if(iChar >= 0x10000 && iChar <= 0x1FFFFF)
   {
      // 3 extra chars, 11110xxx 10xxxxx 10xxxxxx 10xxxxxx (UCS-4 0001 0000-001F FFFF)
      szReturn[3] = 128;
      szReturn[3] += maskcheck(iChar, 0, 6);
      szReturn[2] = 128;
      szReturn[2] += maskcheck(iChar, 6, 6);
      szReturn[1] = 128;
      szReturn[1] += maskcheck(iChar, 12, 6);
      szReturn[0] = 240;
      szReturn[0] += maskcheck(iChar, 18, 3);
   }
   else if(iChar >= 0x200000 && iChar <= 0x3FFFFFF)
   {
      // 4 extra chars, 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx (UCS-4 0020 0000-03FF FFFF)
      szReturn[4] = 128;
      szReturn[4] += maskcheck(iChar, 0, 6);
      szReturn[3] = 128;
      szReturn[3] += maskcheck(iChar, 6, 6);
      szReturn[2] = 128;
      szReturn[2] += maskcheck(iChar, 12, 6);
      szReturn[1] = 128;
      szReturn[1] += maskcheck(iChar, 18, 6);
      szReturn[0] = 240;
      szReturn[0] += maskcheck(iChar, 24, 2);
   }
   else if(iChar >= 0x4000000 && iChar <= 0x7FFFFFFF)
   {
      // 5 extra chars, 1111110x 10xxxxxx ... 10xxxxxx (UCS-4 0400 0000-7FFF FFFF)
      szReturn[5] = 128;
      szReturn[5] += maskcheck(iChar, 0, 6);
      szReturn[4] = 128;
      szReturn[4] += maskcheck(iChar, 6, 6);
      szReturn[3] = 128;
      szReturn[3] += maskcheck(iChar, 12, 6);
      szReturn[2] = 128;
      szReturn[2] += maskcheck(iChar, 18, 6);
      szReturn[1] = 128;
      szReturn[1] += maskcheck(iChar, 24, 6);
      szReturn[0] = 240;
      szReturn[0] += maskcheck(iChar, 30, 1);
   }
   else
   {
      delete[] szReturn;
      szReturn = NULL;
   }

   return szReturn;
}

unsigned int CmdDecode(byte *pChars, int *iMove, bool *bValid)
{
   unsigned int iReturn = 0;

   *bValid = true;

   if(pChars[0] <= 127)
   {
      // ASCII char, 0xxxxxx (0000 0000-0000 007F)
      iReturn = pChars[0];

      *iMove = 0;
   }
   else if(pChars[0] >= 192 && pChars[0] <= 223)
   {
      // 1 extra char, 110xxxxx 10xxxxxx (UCS-4 0000 0080-0000 007FF)
      iReturn += powconv(pChars[1], 6, 128, 0);
      iReturn += powconv(pChars[0], 5, 192, 6);

      if(iReturn < 0x80 || iReturn > 0x7FF)
      {
         *bValid = false;
      }

      *iMove = 1;
   }
   else if(pChars[0] >= 224 && pChars[0] <= 239)
   {
      // 2 extra chars, 1110xxxx 10xxxxxx 10xxxxxx (UCS-4 0000 0800-0000 FFFF)
      iReturn += powconv(pChars[2], 6, 128, 0);
      iReturn += powconv(pChars[1], 6, 128, 6);
      iReturn += powconv(pChars[0], 4, 224, 12);

      if(iReturn < 0x800 || iReturn > 0xFFFF)
      {
         *bValid = false;
      }

      *iMove = 2;
   }
   else if(pChars[0] >= 240 && pChars[0] <= 247)
   {
      // 3 extra chars, 11110xxx 10xxxxx 10xxxxxx 10xxxxxx (UCS-4 0001 0000-001F FFFF)
      iReturn += powconv(pChars[3], 6, 128, 0);
      iReturn += powconv(pChars[2], 6, 128, 6);
      iReturn += powconv(pChars[1], 6, 128, 12);
      iReturn += powconv(pChars[0], 3, 240, 18);

      if(iReturn < 0x10000 || iReturn > 0x1FFFFF)
      {
         *bValid = false;
      }

      *iMove = 3;
   }
   else if(pChars[0] >= 248 && pChars[0] <= 251)
   {
      // 4 extra chars, 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx (UCS-4 0020 0000-03FF FFFF)
      iReturn += powconv(pChars[4], 6, 128, 0);
      iReturn += powconv(pChars[3], 6, 128, 6);
      iReturn += powconv(pChars[2], 6, 128, 12);
      iReturn += powconv(pChars[1], 6, 128, 18);
      iReturn += powconv(pChars[0], 2, 248, 24);

      if(iReturn < 0x200000 || iReturn > 0x3FFFFFF)
      {
         *bValid = false;
      }

      *iMove = 4;
   }
   else if(pChars[0] >= 252 && pChars[0] <= 253)
   {
      // 5 extra chars, 1111110x 10xxxxxx ... 10xxxxxx (UCS-4 0400 0000-7FFF FFFF)
      iReturn += powconv(pChars[5], 6, 128, 0);
      iReturn += powconv(pChars[4], 6, 128, 6);
      iReturn += powconv(pChars[3], 6, 128, 12);
      iReturn += powconv(pChars[2], 6, 128, 18);
      iReturn += powconv(pChars[1], 6, 128, 24);
      iReturn += powconv(pChars[0], 2, 252, 30);

      if(iReturn < 0x4000000 || iReturn > 0x7FFFFFFF)
      {
         *bValid = false;
      }

      *iMove = 5;
   }
   else
   {
      *bValid = false;
   }

   return iReturn;
}
#endif

int CmdOutput(const byte *pData, int iDataLen, bool bSingleChar, bool bUTF8)
{
#ifdef BUILDWIDE
   int iDataPos = 0, iDataMove = 0;
   unsigned int iUCS = 0;
   bool bValid = false;
   unsigned char *szChars = NULL;
   cchar_t *pChars = NULL;
#endif

   if(bSingleChar == true)
   {
#ifdef BUILDWIDE
      if(bUTF8 == true)
      {
         pChars = new cchar_t;
         pChars->attr = A_NORMAL;
         pChars->chars[4] = 0;

         for(iDataPos = 0; iDataPos < iDataLen; iDataPos++)
         {
            iUCS = CmdDecode((byte *)pData + iDataPos, &iDataMove, &bValid);
            iDataPos += iDataMove;

            if(bValid == true)
            {
               szChars = CmdEncode(iUCS);

               pChars->chars[0] = szChars[0];
               pChars->chars[1] = szChars[1];
               pChars->chars[2] = szChars[2];
               pChars->chars[3] = szChars[3];

               add_wch(pChars);

               delete[] szChars;
            }
         }

         delete pChars;
      }
      else
#endif
      {
         addch(168);
      }
   }
   else
   {
      addnstr((char *)pData, iDataLen);
   }

   return iDataLen;
}

void CmdRedraw(bool bFull)
{
   if(bFull == true)
   {
      clear();
   }

   refresh();
}

void CmdBack(int iNumChars)
{
   while(iNumChars > 0 && stdscr->_curx > 0)
   {
      move(stdscr->_cury, stdscr->_curx - 1);
      iNumChars--;
   }
}

void CmdReturn()
{
   if(stdscr->_cury == LINES - 1)
   {
      scroll(stdscr);
      move(stdscr->_cury, 0);
   }
   else
   {
      move(stdscr->_cury + 1, 0);
   }
}

void CmdAttr(char cColour)
{
   int iAttr = 0;

   if(cColour == '0')
   {
      iAttr = COLOR_PAIR(1);
   }
   else if(cColour == '1')
   {
      iAttr = COLOR_PAIR(1) | A_BOLD;
   }
   else
   {
      switch(cColour)
      {
         case 'a':
            iAttr = COLOR_PAIR(2);
            break;

         case 'r':
         case 'R':
            iAttr = COLOR_PAIR(3);
            break;

         case 'g':
         case 'G':
            iAttr = COLOR_PAIR(4);
            break;

         case 'y':
         case 'Y':
            iAttr = COLOR_PAIR(5);
            break;

         case 'b':
         case 'B':
            iAttr = COLOR_PAIR(6);
            break;

         case 'm':
         case 'M':
            iAttr = COLOR_PAIR(7);
            break;

         case 'c':
         case 'C':
            iAttr = COLOR_PAIR(8);
            break;

         case 'w':
         case 'W':
            iAttr = COLOR_PAIR(9);
            break;
      }

      if(isupper(cColour))
      {
         iAttr |= A_BOLD;
      }
   }

   attrset(iAttr);
}

int ColourMatch(char cColour)
{
   switch(cColour)
   {
      case 'a':
         return COLOR_BLACK;
         break;

      case 'r':
      case 'R':
         return COLOR_RED;
         break;

      case 'g':
      case 'G':
         return COLOR_GREEN;
         break;

      case 'y':
      case 'Y':
         return COLOR_YELLOW;
         break;

      case 'b':
      case 'B':
         return COLOR_BLUE;
         break;

      case 'm':
      case 'M':
         return COLOR_MAGENTA;
         break;

      case 'c':
      case 'C':
         return COLOR_CYAN;
         break;

      case 'w':
      case 'W':
         return COLOR_WHITE;
         break;
   }

   return -1;
}

void ColourReset()
{
   init_pair(1, m_iForeground, m_iBackground);
   init_pair(2, COLOR_BLACK, m_iBackground);
   init_pair(3, COLOR_RED, m_iBackground);
   init_pair(4, COLOR_GREEN, m_iBackground);
   init_pair(5, COLOR_YELLOW, m_iBackground);
   init_pair(6, COLOR_BLUE, m_iBackground);
   init_pair(7, COLOR_MAGENTA, m_iBackground);
   init_pair(8, COLOR_CYAN, m_iBackground);
   init_pair(9, COLOR_WHITE, m_iBackground);
}

void CmdForeground(char cColour)
{
   m_iForeground = ColourMatch(cColour);
   ColourReset();
}

void CmdBackground(char cColour)
{
   m_iBackground = ColourMatch(cColour);
   ColourReset();
}

void CmdBeep()
{
   beep();
}

void CmdWait()
{
#if HAVE_NANOSLEEP
   struct timespec tSleep, tRemain;

   tSleep.tv_sec = 1;
   tSleep.tv_nsec = 0;

   nanosleep(&tSleep, &tRemain);
#else
   usleep(1000);
#endif
}

#ifndef FreeBSD
bool procstat(int iPID, int *iPPID, int *iSession)
{
   int fStat = -1;
   int iLineLen = 0, iTemp = 0;
   char szStat[100], szLine[1000], szTemp[100];
   char cTemp = 0;

   sprintf(szStat, "/proc/%d/stat", iPID);
   fStat = open(szStat, O_RDONLY);
   if(fStat == -1)
   {
      debug(DEBUGLEVEL_ERR, "procstat open %s failed, %s\n", szStat, strerror(errno));
      return false;
   }

   iLineLen = read(fStat, szLine, sizeof(szLine) - 1);
   close(fStat);

   szLine[iLineLen] = '\0';

   sscanf(szLine, "%d %s %c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u",
      // pid -> tgpid
      &iTemp, szTemp, &cTemp, iPPID, &iTemp, iSession, &iTemp, &iTemp,
      // flags -> vsize
      &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp,
      &iTemp, &iTemp, &iTemp);

   // printf("program %s, PPID %d\n", szProgram, iPPID);

   return true;
}

utmp *utmpscan(int iSession)
{
   int iTTYPos = 0;
   bool bFound = false;
   char *szTTY = NULL;
   struct utmp *pEntry = NULL;

   debug(DEBUGLEVEL_INFO, "utmpscan entry %d\n", iSession);

   // printf("utmpscan:\n");
   if(iSession == -1)
   {
      szTTY = ttyname(0);
      debug(DEBUGLEVEL_INFO, "utmpscan tty '%s'\n", szTTY);
      iTTYPos = strlen(szTTY);
      if(iTTYPos > 0)
      {
         iTTYPos--;
         while(iTTYPos > 0 && szTTY[iTTYPos] != '/')
         {
            iTTYPos--;
         }
         szTTY = szTTY + iTTYPos;
         debug(DEBUGLEVEL_INFO, "utmpscan dev '%s'\n", szTTY);
      }
   }

#ifndef CYGWIN
   while(bFound == false && (pEntry = getutent()) != NULL)
   {
      if((iSession != -1 && pEntry->ut_pid == iSession) || (iSession == -1 && stricmp(szTTY, pEntry->ut_id) == 0))
      {
         debug(DEBUGLEVEL_INFO, "utmpscan %d, '%s'\n", iSession, pEntry->ut_host);

         bFound = true;
      }
   }

   endutent();
#endif

   debug(DEBUGLEVEL_INFO, "utmpscan exit %p\n", pEntry);
   return pEntry;
}

bool ProxyHostEntry(EDF *pEDF, struct utmp *pEntry)
{
   char *szAddress = NULL;

   if(pEntry != NULL)
   {
      if(strlen(pEntry->ut_host) > 0)
      {
#ifndef CYGWIN
         szAddress = Conn::AddressToString(ntohl(pEntry->ut_addr_v6[0]));
#else
         szAddress = Conn::AddressToString(ntohl(pEntry->ut_addr));
#endif
         debug(DEBUGLEVEL_DEBUG, "ProxyHostEntry parent connected from '%s' / '%s'\n", pEntry->ut_host, szAddress);

         if(strcmp(pEntry->ut_host, szAddress) != 0)
         {
            pEDF->AddChild("hostname", pEntry->ut_host);
            if(ProtocolVersion("2.5") >= 0)
            {
               pEDF->AddChild("address", szAddress);
            }
         }
         else if(ProtocolVersion("2.5") < 0)
         {
            pEDF->AddChild("hostname", szAddress);
         }

         delete[] szAddress;
      }
      else
      {
         debug(DEBUGLEVEL_DEBUG, "ProxyHostEntry parent connected locally\n");

         return false;
      }
   }

   return true;
}

#endif

bool ProxyHost(EDF *pEDF)
{
   debug(DEBUGLEVEL_INFO, "ProxyHost entry\n");

#ifndef FreeBSD
   int iPID = 0, iPPID = 0, iSession = 0, iPrevSession = 0;
   struct utmp *pEntry = NULL;

   // printf("tty %s\n", ttyname(0));

   // szHost[0] = '\0';
   // if(utmpscan(-1, szHost) == false)
   pEntry = utmpscan(-1);
   if(pEntry == NULL)
   {
      // Couldn't find this process

      debug(DEBUGLEVEL_ERR, "ProxyHost exit false, utmpscan failed\n");
      return false;
   }

   debug(DEBUGLEVEL_INFO, "ProxyHost host '%s'\n", pEntry->ut_host);
   if(strchr(pEntry->ut_host, ':') != NULL)
   {
      debug(DEBUGLEVEL_INFO, "ProxyHost parent check\n");

      iPID = getppid();
      while(iPID != 0 && procstat(iPID, &iPPID, &iSession) == true)
      {
         debug(DEBUGLEVEL_INFO, "ProxyHost %d (parent %d, session %d)\n", iPID, iPPID, iSession);
         if(iPrevSession == iSession)
         {
            // szHost[0] = '\0';
            pEntry = utmpscan(iSession);
            ProxyHostEntry(pEDF, pEntry);

            iPID = 0;
         }
         else
         {
            iPrevSession = iSession;
            iPID = iPPID;
         }
      }
   }
   else
   {
      ProxyHostEntry(pEDF, pEntry);
   }
#endif

   debug(DEBUGLEVEL_INFO, "ProxyHost exit true\n");
   return true;
}

void CmdRun(const char *szProgram, bool bWait, const char *szArgs)
{
   int iReturn = 0;
   int iFork = fork();

   if(iFork > 0)
   {
      if(bWait == true)
      {
         iReturn = wait(NULL);
         debug(DEBUGLEVEL_DEBUG, "CmdRun wait return %d\n", iReturn);

         CmdRedraw(true);
      }
   }
   else if(iFork == 0)
   {
      iReturn = execl(szProgram, szProgram, szArgs, NULL);
      if(iReturn == -1)
      {
         debug(DEBUGLEVEL_ERR, "CmdRun exec failed, %s\n", strerror(errno));
      }
   }
   else
   {
      debug(DEBUGLEVEL_ERR, "CmdRun fork error, %s\n", strerror(errno));
   }
}

int CmdPID()
{
   return getpid();
}

bool CmdBrowser(char *szBrowser, bool bWait)
{
   m_szBrowser = strmk(szBrowser);
   m_bBrowserWait = bWait;

   return true;
}

char *CmdBrowser()
{
   return m_szBrowser;
}

bool CmdBrowse(const char *szURI)
{
   CmdRun(m_szBrowser, m_bBrowserWait, szURI);

   return true;
}

char CmdDirSep()
{
   return '/';
}

bool CmdOpen(const char *szFilename)
{
   return false;
}

char *CmdText(int iOptions, const char *szInit, CMDFIELDSFUNC pFieldsFunc, EDF *pData)
{
   STACKTRACE
   char *szInput = NULL, *szTempFile = NULL;
   int iFile = 0;
   CmdInput *pInput = NULL;
   int iFork = 0, iReturn = -1;
   struct stat sFile;

   if(CmdEditor() != NULL && mask(iOptions, CMD_LINE_EDITOR) == true)
   {
      /* szTempFile = tempnam("/tmp", "qUAck");
      // debug("CmdText temp file %s\n", szTempFile);
      fFile = fopen(szTempFile, "w");
      if(fFile != NULL) */

      szTempFile = strmk("qUAckXXXXXXXX");
      iFile = mkstemp(szTempFile);
      if(iFile > 0)
      {
         // fclose(fFile);

         debug(DEBUGLEVEL_INFO, "CmdText temp file %s\n", szTempFile);

         iFork = fork();
         if(iFork > 0)
         {
            STACKTRACEUPDATE
            iReturn = wait(NULL);
            debug(DEBUGLEVEL_DEBUG, "CmdText wait return %d\n", iReturn);

            CmdRedraw(true);
         }
         else if(iFork == 0)
         {
            iReturn = execl(CmdEditor(), CmdEditor(), szTempFile, NULL);
            if(iReturn == -1)
            {
               debug(DEBUGLEVEL_ERR, "CmdText exec failed, %s\n", strerror(errno));
            }
         }
         else
         {
            debug(DEBUGLEVEL_ERR, "CmdText fork error, %s\n", strerror(errno));
         }
      }
      else
      {
         debug(DEBUGLEVEL_ERR, "CmdText mkstemp failed, %s\n", strerror(errno));
      }
      STACKTRACEUPDATE
      if(iReturn != -1)
      {
         if(CmdYesNo("Send message", true) == true)
         {
            fstat(iFile, &sFile);

            szInput = new char[sFile.st_size + 1];
            lseek(iFile, 0, SEEK_SET);
            read(iFile, szInput, sFile.st_size);
            szInput[sFile.st_size] = '\0';
            debug(DEBUGLEVEL_DEBUG, "CmdText temp file '%s'\n", szInput);
         }
      }

      if(iFile > 0)
      {
         close(iFile);
      }
   }

   if(iReturn == -1)
   {
      STACKTRACEUPDATE
      if(pFieldsFunc != NULL)
      {
         debug(DEBUGLEVEL_DEBUG, "CmdText fields %p %p\n", pFieldsFunc, pData);
         pInput = new CmdInput(pFieldsFunc, pData);
      }
      else
      {
         pInput = new CmdInput(NULL, CmdWidth(), CMD_LINE_MULTI | iOptions, szInit);
      }
      CmdInputLoop(-1, pInput, NULL, (byte **)&szInput);
      delete pInput;
   }

   return szInput;
}
