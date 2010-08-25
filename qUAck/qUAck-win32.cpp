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
** CmdIO-win32.cpp: Implementation of Windows specific I/O functions
*/

#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <process.h>

#include "EDF/EDF.h"

#include "Conn/EDFConn.h"

#include "CmdIO.h"
#include "CmdIO-common.h"
#include "CmdInput.h"
#include "CmdMenu.h"

#include "qUAck.h"

#define MAX_EVENTS 200

#define ISRETURN(x) (x == '\r' || x == '\n' ? '-' : x)

// char *m_szClientName = NULL;
// char m_szClientName[64];

UINT m_iConsoleOutput = 0;
int m_iDefBackground = 0, m_iBackground = m_iDefBackground, m_iDefForeground = FOREGROUND_INTENSITY + FOREGROUND_RED + FOREGROUND_GREEN + FOREGROUND_BLUE, m_iForeground = m_iDefForeground;

char *m_szBrowser = NULL;
bool m_bBrowserWait = false;

// char *m_szEditor = NULL;

CmdInput *CmdInputLoop(int iMenuStatus, CmdInput *pInput, byte *pcInput, byte **pszInput);

/* char *CLIENT_NAME()
{
   // if(m_szClientName == NULL)
   {
      // m_szClientName = new char[20];
      sprintf(m_szClientName, "%s v%s (Win32)", CLIENT_BASE, CLIENT_VERSION);
   }

   return m_szClientName;
} */

char *CLIENT_SUFFIX()
{
   return "";
}

char *CLIENT_PLATFORM()
{
   return " (Win32)";
}

void CmdStartup(int iSignal)
{
   BOOL bResult = 0;

   m_iConsoleOutput = GetConsoleOutputCP();
   bResult = SetConsoleOutputCP(CP_UTF8);
   if(bResult == 0)
   {
      debug("CmdStartup code page set failed %d\n", GetLastError());
   }

   CmdReset(0);
}

void CmdReset(int iSignal)
{
   // COORD sCoords;
   // sCoords = GetLargestConsoleWindowSize(GetStdHandle(STD_OUTPUT_HANDLE));
   // debug("CliWindow window size %d %d\n", sCoords.X, sCoords.Y);
   CONSOLE_SCREEN_BUFFER_INFO sWindow;
   if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sWindow) != 0)
   {
      // debug("CliWindow buffer size %d %d\n", sWindow.dwSize.X, sWindow.dwSize.Y);
      CmdWidth(sWindow.dwSize.X);
      CmdHeight(sWindow.dwSize.Y);

      m_iDefBackground = (sWindow.wAttributes & BACKGROUND_BLUE)
         + (sWindow.wAttributes & BACKGROUND_GREEN)
         + (sWindow.wAttributes & BACKGROUND_RED)
         + (sWindow.wAttributes & BACKGROUND_INTENSITY);
      m_iBackground = m_iDefBackground;
      m_iForeground = (sWindow.wAttributes & FOREGROUND_BLUE)
         + (sWindow.wAttributes & FOREGROUND_GREEN)
         + (sWindow.wAttributes & FOREGROUND_RED)
         + (sWindow.wAttributes & FOREGROUND_INTENSITY);
      m_iDefForeground = m_iForeground;

      // printf("CmdIOStartup default colours fg=%d,bg=%d\n", m_iForeground, m_iBackground);
   }
}

void CmdShutdown(int iSignal)
{
   BOOL bResult = 0;

   bResult = SetConsoleOutputCP(m_iConsoleOutput);
   if(bResult == 0)
   {
      debug("CmdShutdown code page set failed %d\n", GetLastError());
   }
}

int CmdType()
{
   return 0;
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
   STACKTRACE
   int iEventNum = 0, iCharNum = 0;
   double dTick = 0;
   DWORD dwNumEvents = 0, dwReadEvents = 0;
   byte cReturn = '\0', *pBuffer = NULL;
   INPUT_RECORD *pEvents = NULL;

   if(WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), 50) != WAIT_OBJECT_0)
   {
      return iBufferLen;
   }

   // Count waiting events
   while(GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &dwNumEvents) != 0 && dwNumEvents > 0)
   {
      dTick = gettick();

      // Check event type
      if(dwNumEvents >= MAX_EVENTS)
      {
         debug(DEBUGLEVEL_WARN, "CmdInput %d console input events, resizing\n", dwNumEvents);
         dwNumEvents = MAX_EVENTS;
      }
      pEvents = new INPUT_RECORD[dwNumEvents];
      if(ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), pEvents, dwNumEvents, &dwReadEvents) != 0)
      {
         if(iBufferLen > 0)
         {
            // memprint(debugfile(), "CmdInput sub-total buffer", m_pBuffer, m_iBufferLen, -1, false, -1, 50, false);

            pBuffer = new byte[iBufferLen + dwReadEvents];
            memcpy(pBuffer, (*pCurrBuffer), iBufferLen);
            delete[] (*pCurrBuffer);

            // debug("CmdInputGet check %d events at offset %d\n", dwReadEvents, m_iBufferLen);
         }
         else
         {
            pBuffer = new byte[dwReadEvents];
         }
         *pCurrBuffer = pBuffer;

         /* if(m_iBufferLen > 0)
         {
            debug("CmdInputGet put %d events at offset %d\n", dwReadEvents, m_iBufferLen);
         } */
         for(iEventNum = 0; iEventNum < dwReadEvents; iEventNum++)
         {
            if(pEvents[iEventNum].EventType == KEY_EVENT)
            {
               if(pEvents[iEventNum].Event.KeyEvent.bKeyDown == TRUE)
               {
                  if(mask(pEvents[iEventNum].Event.KeyEvent.dwControlKeyState, ENHANCED_KEY) == true)
                  {
                     if(pEvents[iEventNum].Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
                     {
                        pBuffer[iBufferLen++] = '\r';
                        iCharNum++;
                     }
                     if(pEvents[iEventNum].Event.KeyEvent.wVirtualKeyCode == VK_NEXT)
                     {
                        pBuffer[iBufferLen++] = '\004';
                        iCharNum++;
                     }
                     if(pEvents[iEventNum].Event.KeyEvent.wVirtualKeyCode == VK_END)
                     {
                        pBuffer[iBufferLen++] = '\005';
                        iCharNum++;
                     }
                     if(pEvents[iEventNum].Event.KeyEvent.wVirtualKeyCode == VK_HOME)
                     {
                        pBuffer[iBufferLen++] = '\001';
                        iCharNum++;
                     }
                     if(pEvents[iEventNum].Event.KeyEvent.wVirtualKeyCode == VK_LEFT)
                     {
                        pBuffer[iBufferLen++] = '\002';
                        iCharNum++;
                     }
                     if(pEvents[iEventNum].Event.KeyEvent.wVirtualKeyCode == VK_UP)
                     {
                        pBuffer[iBufferLen++] = '\020';
                        iCharNum++;
                     }
                     if(pEvents[iEventNum].Event.KeyEvent.wVirtualKeyCode == VK_RIGHT)
                     {
                        pBuffer[iBufferLen++] = '\006';
                        iCharNum++;
                     }
                     if(pEvents[iEventNum].Event.KeyEvent.wVirtualKeyCode == VK_DOWN)
                     {
                        pBuffer[iBufferLen++] = '\016';
                        iCharNum++;
                     }
                     else if(pEvents[iEventNum].Event.KeyEvent.wVirtualKeyCode == VK_DELETE)
                     {
                        pBuffer[iBufferLen++] = '\004';
                        iCharNum++;
                     }
                     else
                     {
                        debug(DEBUGLEVEL_INFO, "CmdInputGet enhanced key %d\n", pEvents[iEventNum].Event.KeyEvent.wVirtualKeyCode);
                     }
                  }
                  else if(mask(pEvents[iEventNum].Event.KeyEvent.dwControlKeyState, LEFT_ALT_PRESSED) == false &&
                     mask(pEvents[iEventNum].Event.KeyEvent.dwControlKeyState, RIGHT_ALT_PRESSED) == false)
                  {
                     debug(DEBUGLEVEL_DEBUG, "CmdInputGet character %c [%d]\n", pEvents[iEventNum].Event.KeyEvent.uChar.UnicodeChar, pEvents[iEventNum].Event.KeyEvent.uChar.UnicodeChar);

                     if(pEvents[iEventNum].Event.KeyEvent.uChar.AsciiChar > 0)
                     {
                        debug(DEBUGLEVEL_DEBUG, "CmdInputGet ASCII character '%c'\n", ISRETURN(pEvents[iEventNum].Event.KeyEvent.uChar.AsciiChar));

                        pBuffer[iBufferLen++] = pEvents[iEventNum].Event.KeyEvent.uChar.AsciiChar;
                        iCharNum++;
                     }
                     else
                     {
                        debug(DEBUGLEVEL_DEBUG, "CmdInputGet unicode character '%c' [%d]\n", ISRETURN(pEvents[iEventNum].Event.KeyEvent.uChar.UnicodeChar), pEvents[iEventNum].Event.KeyEvent.uChar.UnicodeChar);
                     }
                  }
                  else
                  {
                     debug(DEBUGLEVEL_DEBUG, "CmdInputGet ALT character '%c' [%d]\n", ISRETURN(pEvents[iEventNum].Event.KeyEvent.uChar.UnicodeChar), pEvents[iEventNum].Event.KeyEvent.uChar.UnicodeChar);
                  }
               }
               else
               {
                  // debug(DEBUGLEVEL_DEBUG, "CmdInputGet key up character '%c' [%d]\n", ISRETURN(pEvents[iEventNum].Event.KeyEvent.uChar.UnicodeChar), pEvents[iEventNum].Event.KeyEvent.uChar.UnicodeChar);
               }
            }
         }

         /* if(iCharNum > 0)
         {
            debug(DEBUGLEVEL_DEBUG, "CmdInputGet added %d characters from %d events in %ld ms\n", iCharNum, dwReadEvents, tickdiff(dTick));
         } */
      }
      else
      {
         // debug(DEBUGLEVEL_ERROR, "CmdInputGet read console failed, %d\n", GetLastError());
         MsgError("CmdInputGet read console failed");
      }
      delete[] pEvents;
   }

   return iBufferLen;
}

bool CmdInputCheck(byte cOption)
{
   return true;
}

// Output functions
int CmdOutput(const byte *pData, int iDataLen, bool bSingleChar, bool bUTF8)
{
   int iDataPos = 0;
   DWORD dWritten = 0;

   if(iDataLen == 0)
   {
      return 0;
   }

   // memprint(debugfile(), "CmdOutput", pData, iDataLen);

   WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), pData, iDataLen, &dWritten, NULL);

   for(iDataPos = 0; iDataPos < iDataLen; iDataPos++)
   {
      if(pData[iDataPos] != '\007' && pData[iDataPos] != '\013')
      {
         // WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), pData + iDataPos, 1, &dWritten, NULL);
      }
   }

   return iDataLen;
}

// Other functions
void CmdRedraw(bool bFull)
{
}

void CmdBack(int iNumChars)
{
   CONSOLE_SCREEN_BUFFER_INFO cBufferInfo;

   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cBufferInfo);
   // debug("CmdBack was %d, %d", cBufferInfo.dwCursorPosition.X, cBufferInfo.dwCursorPosition.Y);

   if(iNumChars > cBufferInfo.dwCursorPosition.X)
   {
      cBufferInfo.dwCursorPosition.X = 0;
   }
   else
   {
      cBufferInfo.dwCursorPosition.X -= iNumChars;
   }

   SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cBufferInfo.dwCursorPosition);

   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cBufferInfo);
   // debug(". now %d, %d\n", cBufferInfo.dwCursorPosition.X, cBufferInfo.dwCursorPosition.Y);
}

void CmdReturn()
{
   CONSOLE_SCREEN_BUFFER_INFO cBufferInfo;

   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cBufferInfo);
   // debug("CmdReturn was %d, %d / %d x %d", cBufferInfo.dwCursorPosition.X, cBufferInfo.dwCursorPosition.Y, cBufferInfo.dwSize.X, cBufferInfo.dwSize.Y);

   // printf("**");

   cBufferInfo.dwCursorPosition.X = 0;

   if(cBufferInfo.dwCursorPosition.Y == cBufferInfo.dwSize.Y - 1)
   {
      // printf("||");

      // debug(", scroll");

      SMALL_RECT srctScrollRect, srctClipRect; 
      CHAR_INFO chiFill; 
      COORD coordDest; 
       
      // The scrolling rectangle is the bottom 15 rows of the screen buffer       
      srctScrollRect.Top = cBufferInfo.dwSize.Y - cBufferInfo.dwSize.Y + 1; 
      srctScrollRect.Bottom = cBufferInfo.dwSize.Y - 1; 
      srctScrollRect.Left = 0; 
      srctScrollRect.Right = cBufferInfo.dwSize.X - 1; 
       
      // The destination for the scroll rectangle is one row up.        
      coordDest.X = 0; 
      coordDest.Y = cBufferInfo.dwSize.Y - cBufferInfo.dwSize.Y; 
       
      // The clipping rectangle is the same as the scrolling rectangle. 
      // The destination row is left unchanged. 
      srctClipRect = srctScrollRect; 
       
      // Fill the bottom row with green blanks.        
      chiFill.Attributes = m_iBackground;
      chiFill.Char.AsciiChar = ' '; 
       
      // Scroll up one line. 
       
      ScrollConsoleScreenBuffer( 
         GetStdHandle(STD_OUTPUT_HANDLE),         // screen buffer handle 
         &srctScrollRect, // scrolling rectangle 
         &srctClipRect,   // clipping rectangle 
         coordDest,       // top left destination cell 
            &chiFill);       // fill character and color 

      SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cBufferInfo.dwCursorPosition);
   }
   else
   {
      cBufferInfo.dwCursorPosition.Y++;

      SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cBufferInfo.dwCursorPosition);
   }

   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cBufferInfo);
   // debug(". now %d, %d\n", cBufferInfo.dwCursorPosition.X, cBufferInfo.dwCursorPosition.Y);

   // printf("**");
}

void CmdAttr(char cColour)
{
	int iAttr = 0;

   if(cColour == '0')
   {
      iAttr = m_iForeground;
   }
   else if(cColour == '1')
   {
      iAttr = m_iForeground + FOREGROUND_INTENSITY;
   }
   else
   {
	   switch(cColour)
	   {
         case 'a':
         case 'A':
            iAttr = 0;
            break;

		   case 'r':
		   case 'R':
			   iAttr = FOREGROUND_RED;
			   break;
   			
		   case 'g':
		   case 'G':
			   iAttr = FOREGROUND_GREEN;
			   break;
   			
		   case 'y':
		   case 'Y':
			   iAttr = FOREGROUND_RED | FOREGROUND_GREEN;
			   break;
   			
		   case 'b':
		   case 'B':
			   iAttr = FOREGROUND_BLUE;
			   break;
   			
		   case 'm':
		   case 'M':
			   iAttr = FOREGROUND_RED | FOREGROUND_BLUE;
			   break;
   			
		   case 'c':
		   case 'C':
			   iAttr = FOREGROUND_GREEN | FOREGROUND_BLUE;
			   break;

         case 'w':
		   case 'W':
            iAttr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	   }

      if(isupper(cColour))
      {
         iAttr += FOREGROUND_INTENSITY;
      }
   }

   iAttr += m_iBackground;
	
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), iAttr);
}

void CmdForeground(char cColour)
{
   // printf("CmdForeground %c\n", cColour);

	switch(cColour)
	{
      case '\0':
         m_iForeground = m_iDefForeground;
         break;

      case 'a':
         m_iForeground = 0;
         break;

		case 'r':
		case 'R':
			m_iForeground = FOREGROUND_RED;
			break;
			
		case 'g':
		case 'G':
			m_iForeground = FOREGROUND_GREEN;
			break;
			
		case 'y':
		case 'Y':
			m_iForeground = FOREGROUND_RED | FOREGROUND_GREEN;
			break;
			
		case 'b':
		case 'B':
			m_iForeground = FOREGROUND_BLUE;
			break;
			
		case 'm':
		case 'M':
			m_iForeground = FOREGROUND_RED | FOREGROUND_BLUE;
			break;
			
		case 'c':
		case 'C':
			m_iForeground = FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;

      case 'w':
		case 'W':
         m_iForeground = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	}
   if(isupper(cColour))
   {
      m_iForeground += FOREGROUND_INTENSITY;
   }
}

void CmdBackground(char cColour)
{
   // printf("CmdBackground %c\n", cColour);

	switch(cColour)
	{
      case '\0':
         m_iBackground = m_iDefBackground;
         break;

      case 'a':
         m_iBackground = 0;
         break;

		case 'r':
		case 'R':
			m_iBackground = BACKGROUND_RED;
			break;
			
		case 'g':
		case 'G':
			m_iBackground = BACKGROUND_GREEN;
			break;
			
		case 'y':
		case 'Y':
			m_iBackground = BACKGROUND_RED | BACKGROUND_GREEN;
			break;
			
		case 'b':
		case 'B':
			m_iBackground = BACKGROUND_BLUE;
			break;
			
		case 'm':
		case 'M':
			m_iBackground = BACKGROUND_RED | BACKGROUND_BLUE;
			break;
			
		case 'c':
		case 'C':
			m_iBackground = BACKGROUND_GREEN | BACKGROUND_BLUE;
			break;

      case 'w':
		case 'W':
         m_iBackground = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
	}
   if(isupper(cColour))
   {
      m_iBackground += BACKGROUND_INTENSITY;
   }
}

void CmdBeep()
{
   MessageBeep(MB_ICONQUESTION);
}

void CmdWait()
{
   Sleep(200);
}

bool ProxyHost(EDF *pEDF)
{
   return false;
}

void CmdRun(const char *szProgram, bool bWait, const char *szArgs)
{
	char szError[500];
	long lRet;

	lRet = (long)ShellExecute(GetFocus(), NULL, szProgram, szArgs, NULL, SW_SHOWNORMAL);

	if(lRet <= 32)
	{
		wsprintf(szError, "CmdRun error %d executing %s (URI %s)", lRet, szProgram, szArgs);
		MessageBox(GetFocus(), szError, "qUAck", MB_ICONSTOP | MB_OK);
	}
}

int CmdPID()
{
   return getpid();
}

bool CmdBrowser(char *szBrowser, bool bWait)
{
	m_bBrowserWait = bWait;

	char szFileType[10];
	char szCommandLine[100];
	long lSize = 0;
	HKEY hRegKey = NULL;

	if(szBrowser != NULL)
	{
		m_szBrowser = strmk(szBrowser);
	}

	if(RegOpenKey(HKEY_CLASSES_ROOT, ".htm", &hRegKey) != ERROR_SUCCESS)
	{
      if(szBrowser != NULL)
      {
		   szBrowser[0] = '\0';
      }
		return false;
	}

	lSize = sizeof(szFileType);

	RegQueryValue(hRegKey, NULL, szFileType, &lSize);
	RegCloseKey(hRegKey);

	if(RegOpenKey(HKEY_CLASSES_ROOT, szFileType, &hRegKey) != ERROR_SUCCESS)
	{
      if(szBrowser != NULL)
      {
		   szBrowser[0] = '\0';
      }
		return false;
	}

	lSize = sizeof(szCommandLine);

	RegQueryValue(hRegKey, "shell\\open\\command", szCommandLine, &lSize);
	RegCloseKey(hRegKey);

	m_szBrowser = new char[strlen(szCommandLine)];

	int iCommandLine = 0, iBrowser = 0;

	if(szCommandLine[0] == '"')
	{
		iCommandLine++;
		while(szCommandLine[iCommandLine] != '\0' && szCommandLine[iCommandLine] != '"')
		{
			m_szBrowser[iBrowser] = szCommandLine[iCommandLine];
			iCommandLine++;
			iBrowser++;
		}
	}
	else
	{
		while(szCommandLine[iCommandLine] != 0 && szCommandLine[iCommandLine] != ' ')
		{
			if(szCommandLine[iCommandLine] != '\"' && szCommandLine[iCommandLine] != '\'')
			{
				m_szBrowser[iBrowser] = szCommandLine[iCommandLine];
				iBrowser++;
			}
			iCommandLine++;
		}
	}
	m_szBrowser[iBrowser] = '\0';

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

bool CmdOpen(const char *szFilename)
{
	char szError[500];
	long lRet;

   if(CmdYesNo("Open now", true) == true)
   {
	   lRet = (long)ShellExecute(GetFocus(), NULL, szFilename, NULL, NULL, SW_SHOWNORMAL);

	   if(lRet <= 32)
	   {
		   wsprintf(szError, "Error %d opening %s", lRet, szFilename);
		   MessageBox(GetFocus(), szError, "qUAck", MB_ICONSTOP | MB_OK);

         return false;
	   }
   }

   return true;
}

char CmdDirSep()
{
   return '\\';
}

char *CmdText(int iOptions, const char *szInit, CMDFIELDSFUNC pFieldsFunc, EDF *pData)
{
   STACKTRACE
   char *szInput = NULL, *szTempFile = NULL;
   FILE *fFile = NULL;
   int iFile = 0;
   CmdInput *pInput = NULL;
   int iFork = 0, iReturn = -1;

   if(CmdEditor() != NULL && mask(iOptions, CMD_LINE_EDITOR) == true)
   {
      szTempFile = tempnam("c:\\", "qUAck");
      // debug("CmdText temp file %s\n", szTempFile);
      fFile = fopen(szTempFile, "w");
      if(fFile != NULL)
      {
         fclose(fFile);
         /* szFile = (char *)FileRead(szTempFile);
         debug("CmdText temp file '%s'\n", szFile);
         delete[] szFile; */

         // debug("CmdText spawning %s [%s]\n", m_szEditor, szTempFile);
         iReturn = spawnl(_P_WAIT, CmdEditor(), CmdEditor(), szTempFile, NULL);
         if(iReturn != -1)
         {
            debug(DEBUGLEVEL_DEBUG, "CmdText spawn return %d\n", iReturn);
         }
         else
         {
            debug(DEBUGLEVEL_ERR, "CmdText spawn error, %s\n", strerror(errno));
         }
      }

      STACKTRACEUPDATE
      if(iReturn != -1)
      {
         if(CmdYesNo("Send message", true) == true)
         {
            szInput = (char *)FileRead(szTempFile);
            debug(DEBUGLEVEL_DEBUG, "CmdText temp file '%s'\n", szInput);
         }
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
