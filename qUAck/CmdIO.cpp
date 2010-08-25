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
** CmdIO.cpp: Implementation of I/O functions
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "Conn/EDFConn.h"

#include "CmdIO.h"
#include "CmdIO-common.h"

// Default members to a normal terminal with no colours
int m_iWidth = 80, m_iHeight = 25, m_iRowNum = -1, m_iHighlight = 0, m_iLinePos = 0;
char m_cColours[9];
bool m_bHardWrap = false, m_bUTF8 = false;

/*
** Display boolean indicates to the CmdWrite function not to display any text after
** 'Q' is pressed from the '-- More --' menu
*/
bool m_bDisplay = true;
char m_cColour = '\0';

byte *m_pBuffer = NULL;
int m_iBufferLen = 0;

FILE *m_fLogFile = NULL;

// Internal functions
bool CmdColour(char cColour)
{
   if(m_iHighlight == 0)
   {
      return false;
   }

   if(cColour == '0')
   {
      CmdAttr('0');
      return true;
   }

   if(m_iHighlight == 1)
   {
      CmdAttr('1');
   }
   else
   {
      CmdAttr(m_cColours[cColour - '0']);
   }

   return true;
}

bool CmdRowInc()
{
   STACKTRACE
   // int iTemp = 0;
   char cRead = '\0';
   // char szColour[2];

   if(m_iRowNum == -1)
   {
      return false;
   }

   m_iLinePos = 0;
   m_iRowNum++;
   if(m_iRowNum > m_iHeight - 1)
   {
      CmdColour('0');
      CmdOutput((byte *)"-- More --", 10, false, false);
      while(cRead == '\0')
      {
         cRead = CmdInputGet();
         if(cRead != '\0')
         {
            cRead = toupper(cRead);
            if(cRead != ' ' && cRead != '\r' && cRead != '\n' && cRead != 'Q')
            {
               cRead = '\0';
            }
         }
         else
         {
            CmdWait();
         }
      }
      CmdBack(10);
      CmdOutput((byte *)"          ", 10, false, false);
      CmdBack(10);
      if(m_cColour != '\0')
      {
         CmdColour(m_cColour);
      }
      if(cRead == 'Q')
      {
         m_bDisplay = false;
      }

      m_iRowNum = 1;
   }

   return true;
}

// Input functions
byte CmdInputGet()
{
   char cReturn = '\0';

   m_iBufferLen = CmdInputGet(&m_pBuffer, m_iBufferLen);

   if(m_iBufferLen == 0)
   {
      return '\0';
   }

   cReturn = m_pBuffer[0];
   // m_bTemp = false;
   // debug("CmdGetInput first char %c[%d]\n", isprint(cReturn) ? cReturn : ' ', cReturn);
   CmdInputRelease(1);
   // m_bTemp = true;
   /* if(m_iBufferLen > 1)
   {
      memprint(debugfile(), "CmdGetInput new buffer", m_pBuffer, m_iBufferLen, -1, false, -1, -1, false);
   } */

   return cReturn;
}

int CmdInputLen()
{
   return m_iBufferLen;
}

byte CmdInputChar(int iCharNum)
{
   if(iCharNum < 0 || iCharNum >= m_iBufferLen)
   {
      return '\0';
   }

   return m_pBuffer[iCharNum];
}

int CmdInputRelease(int iNumChars)
{
   STACKTRACE
   byte *pBuffer = NULL;

   if(iNumChars == 0 || m_iBufferLen == 0)
   {
      return 0;
   }

   // debug("CmdReleaseInput %d, %d '%s'", iNumChars, m_iBufferLen, m_iBufferLen > 0 ? (char *)m_pBuffer : "");
   /* if(m_bTemp == true)
   {
      debug("CmdReleaseInput %d\n", iNumChars);
      memprint(debugfile(), "CmdReleaseInput before", m_pBuffer, m_iBufferLen, -1, false, -1, -1, false);
   } */
   // debug("CmdReleaseInput %d, %d\n", iNumChars, m_iBufferLen);
   if(iNumChars == -1 || iNumChars >= m_iBufferLen)
   {
      delete[] m_pBuffer;
      m_pBuffer = NULL;

      m_iBufferLen = 0;
   }
   else
   {
      pBuffer = new byte[m_iBufferLen - iNumChars];
      memcpy(pBuffer, m_pBuffer + iNumChars, m_iBufferLen - iNumChars);
      delete[] m_pBuffer;
      m_pBuffer = pBuffer;

      m_iBufferLen -= iNumChars;
   }
   // debug("-> %d '%s'\n", m_iBufferLen, m_iBufferLen > 0 ? (char *)m_pBuffer : "");
   // if(m_bTemp == true && m_iBufferLen > 0)
   /* if(m_iBufferLen > 0)
   {
      memprint(debugfile(), "CmdReleaseInput buffer", m_pBuffer, m_iBufferLen, -1, false, -1, -1, false);
   } */

   return m_iBufferLen;
}


// Output functions

// Find the next return
#define RET_CHECK \
pReturn = (byte *)strchr((char *)pData, '\r'); \
if(pReturn != NULL) \
{ \
   if(pReturn + 1 < pStop && pReturn[1] == '\n') \
   { \
      pReturn++; \
   } \
} \
else \
{ \
	pReturn = (byte *)strchr((char *)pData, '\n'); \
}

// Output text to space
int CmdSpace(const byte *pData, int iDataLen, bool bAddSpace, int iOutput = -1)
{
   // int iDataPos = 0;

   if(iOutput == -1)
   {
      iOutput = iDataLen; 
   }

   if(m_fLogFile != NULL)
   {
      fwrite(pData, iOutput, 1, m_fLogFile);
      if(bAddSpace == true)
      {
         fwrite(" ", 1, 1, m_fLogFile);
      }
   }
   else
   {
      if(m_iRowNum != -1)
      {
         // Page mode on

         /* debug("CmdSpace %d, '", m_iLinePos);
         for(iDataPos = 0; iDataPos < iDataLen; iDataPos++)
         {
            debug("%c", isprint(pData[iDataPos]) ? pData[iDataPos] : '*');
         }
         debug("'"); */

         if(m_iLinePos + iDataLen > CmdWidth())
         {
            // Text will wrap line when printed, move to new line
            // debug(", wrap");

            CmdReturn();
            CmdRowInc();
         }
         else if(m_iLinePos + iDataLen == CmdWidth())
         {
            // Text will reach end of line when printed
            // debug(", end");
         }

         // memprint(debugfile(), "CmdSpace out1", pData, iOutput, false);
         m_iLinePos += CmdOutput(pData, iOutput, iOutput != iDataLen, m_bUTF8);
         
         // debug(" -> %d", m_iLinePos);
         if(m_iLinePos == CmdWidth())
         {
            // Text has reached end of line
            // debug(", text end");

            if(m_bHardWrap == true)
            {
               CmdReturn();
               CmdRowInc();
            }
            else
            {
               CmdRowInc();
            }
            bAddSpace = false;
         }
         if(bAddSpace == true)
         {
            // Print the space
            m_iLinePos += CmdOutput((byte *)" ", 1, false, false);

            if(m_iLinePos == CmdWidth())
            {
               // Text has reached end of line
               // debug(", space end");

               if(m_bHardWrap == true)
               {
                  CmdReturn();
                  CmdRowInc();
               }
               else
               {
                  CmdRowInc();
               }
            }
         }

         // debug(" -> %d\n", m_iLinePos);
      }
      else
      {
         // Page mode off, just output text

         // memprint(debugfile(), "CmdSpace out2", pData, iOutput, true);
         m_iLinePos += CmdOutput(pData, iOutput, iOutput != iDataLen, m_bUTF8);
         if(bAddSpace == true)
         {
            m_iLinePos += CmdOutput((byte *)" ", 1, false, false);
         }
      }
   }

   return iDataLen;
}

// Write characters to the screen
int CmdWrite(const byte *pData, int iDataLen, int iOptions)
{
   STACKTRACE
   byte *pStart = NULL, *pStop = NULL, *pSpace = NULL, *pUTF = NULL, *pMarkup = NULL, *pReturn = NULL;

   if(m_bDisplay == false || iDataLen == 0 || pData == NULL || pData[0] == '\0')
   {
      // Quit from more, null string, empty string
      return 0;
   }

   pStart = (byte *)pData;

   // debug("CmdWrite entry '%s'\n", pData);

   if(iDataLen != -1)
   {
      pStop = (byte *)pData + iDataLen;
   }
   else
   {
      pStop = (byte *)pData + strlen((char *)pData);
   }

   if(mask(iOptions, CMD_OUT_UTF) == true)
   {
      memprint(debugfile(), "CmdWrite UTF", pData, pStop - pData, false);
   }

   if(mask(iOptions, CMD_OUT_RAW) == true)
   {
      CmdOutput(pData, iDataLen, false, false);
      return iDataLen;
   }

   iDataLen = 0;

   if(m_iRowNum != -1)
	{
      // Find the next space
		pSpace = (byte *)strchr((char *)pData, ' ');
	}	
	if(mask(iOptions, CMD_OUT_NOHIGHLIGHT) == false)
	{
      // Find the next markup
		pMarkup = (byte *)strchr((char *)pData, '\037');
	}
   if(mask(iOptions, CMD_OUT_UTF) == true)
   {
      // Find next UTF char
      pUTF = (byte *)pData;
      while(pUTF < pStop && (*pUTF) <= 127)
      {
         pUTF++;
      }
      if(pUTF == pStop)
      {
         pUTF = NULL;
      }

      debug("CmdWrite UTF init %p(%d)\n", pUTF, pUTF != NULL ? pUTF - pStart : -1);
   }

   RET_CHECK

   while(pData != NULL && pData < pStop && m_bDisplay == true)
   {
      /* if(mask(iOptions, CMD_OUT_UTF) == true || m_fLogFile != NULL)
      {
         debug("CmdWrite %p < %p, %d\n", pData, pStop, iDataLen);
      } */

		if(pSpace != NULL && (pUTF == NULL || pSpace < pUTF) && (pMarkup == NULL || pSpace < pMarkup) && (pReturn == NULL || pSpace < pReturn))
		{
         // Space
         // memprint(debugfile(), "CmdWrite space", pData, pSpace - pData);

         iDataLen += CmdSpace(pData, pSpace - pData, true);

         pData = pSpace + 1;
			pSpace = (byte *)strchr((char *)pData, ' ');
      }
		else if(pUTF != NULL && (pMarkup == NULL || pUTF < pMarkup) && (pReturn == NULL || pUTF < pReturn))
		{
         // UTF-8
         memprint(debugfile(), "CmdWrite UTF", pData, pUTF - pData);

         if(pUTF > pData)
         {
            iDataLen += CmdSpace(pData, pUTF - pData, false);
         }

         pData = pUTF;
         pUTF = (byte *)pData + 1;
         while(pUTF < pStop && (*pUTF) >= 128 && (*pUTF) <= 191)
         {
            pUTF++;
         }
         // debug("CmdWrite UTF space %p(%d)\n", pUTF, pUTF != NULL ? pUTF - pStart : -1);

         iDataLen += CmdSpace(pData, 1, false, pUTF - pData);

         pData = pUTF;

         while(pUTF < pStop && (*pUTF) <= 127)
         {
            pUTF++;
         }
         if(pUTF == pStop)
         {
            pUTF = NULL;
         }
         // debug("CmdWrite UTF next %p(%d)\n", pUTF, pUTF != NULL ? pUTF - pStart : -1);
         // memprint(debugfile(), "CmdWrite continue", pData, pStop - pData, false);
      }
		else if(pMarkup != NULL && (pReturn == NULL || pMarkup < pReturn))
		{
         // Markup
         // memprint(debugfile(), "CmdWrite markup", pData, pMarkup - pData);

         iDataLen += CmdSpace(pData, pMarkup - pData, false);

         m_cColour = pMarkup[1];
         CmdColour(m_cColour);

         pData = pMarkup + 2;
			pMarkup = (byte *)strchr((char *)pData, '\037');
		}
		else if(pReturn != NULL)
		{
         // Return
         // memprint(debugfile(), "CmdWrite return", pData, pReturn - pData);

         if(m_fLogFile != NULL)
         {
            fwrite(pData, pReturn - pData, 1, m_fLogFile);
            iDataLen += (pReturn - pData);

            fwrite("\n", 1, 1, m_fLogFile);
         }
         else
         {
            // memprint(debugfile(), "CmdWrite out1", pData, pReturn - pData, true);
            iDataLen += CmdOutput(pData, pReturn - pData, false, false);

            CmdReturn();
            CmdRowInc();
         }

   		pData = pReturn + 1;

         RET_CHECK
      }
		else
		{
         // End of data
         // memprint(debugfile(), "CmdWrite EOD", pData, pStop - pData);

         if(pData < pStop)
         {
            iDataLen += CmdSpace(pData, pStop - pData, false);
         }

   	   pData = NULL;
		}
   }

   // debug("CmdWrite exit %d\n", iDataLen);

   return iDataLen;
}

int CmdWrite(char cData)
{
   byte szData[2];

   szData[0] = cData;
   szData[1] = '\0';

	return CmdWrite(szData, 1, 0);
}

int CmdWrite(const char *szData, int iOptions)
{
	return CmdWrite((byte *)szData, -1, iOptions);
}

int CmdWrite(byte cData)
{
   byte szData[2];

   szData[0] = cData;
   szData[1] = '\0';

	return CmdWrite(szData, 1, 0);
}

int CmdWrite(const byte *pData, int iOptions)
{
   return CmdWrite(pData, -1, iOptions);
}

int CmdWrite(bytes *pData, int iOptions)
{
   // debug("CmdWrite %p(%ld)\n", pData, pData->Length());
   return CmdWrite(pData->Data(false), pData->Length(), iOptions);
}

// Other functions
void CmdWidth(int iWidth)
{
   m_iWidth = iWidth;
}

int CmdWidth()
{
   return m_iWidth;
}

void CmdHeight(int iHeight)
{
   m_iHeight = iHeight;
}

int CmdHeight()
{
   return m_iHeight;
}

void CmdHighlight(int iHighlight)
{
   m_iHighlight = iHighlight;
}

bool CmdColourSet(int iColour, char cColour)
{
   // printf("CmdColourSet entry %d %c\n", iColour, cColour);

   if(iColour == -1)
   {
      CmdBackground(cColour);
      return true;
   }
   else if(iColour == 0)
   {
      CmdForeground(cColour);
   }
   else if(iColour > 0 && iColour <= 8)
   {
	   m_cColours[iColour] = cColour;
      return true;
      // debug("CmdColourSet %d %c\n", iColour, cColour);
   }

   return false;
}

void CmdHardWrap(bool bHardWrap)
{
   m_bHardWrap = bHardWrap;
}

void CmdUTF8(bool bUTF8)
{
   m_bUTF8 = bUTF8;
}

bool CmdUTF8()
{
   return m_bUTF8;
}

void CmdPageOn()
{
   if(m_fLogFile == NULL)
   {
      m_iRowNum = 1;
      m_iLinePos = 0;

      m_bDisplay = true;
   }
}

void CmdPageOff()
{
   if(m_fLogFile == NULL)
   {
      m_iRowNum = -1;
      m_iLinePos = 0;

      m_bDisplay = true;
   }
}

bool CmdLogOpen(const char *szFilename)
{
   CmdLogClose();

   m_fLogFile = fopen(szFilename, "w");

   if(m_fLogFile == NULL)
   {
      return false;
   }

   return true;
}

bool CmdLogClose()
{
   if(m_fLogFile != NULL)
   {
      fclose(m_fLogFile);

      m_fLogFile = NULL;
   }

   return true;
}

void CmdEDFPrint(const char *szTitle, EDF *pEDF, int iOptions)
{
   bytes *pWrite = NULL;

	if(iOptions == -1)
	{
		iOptions = EDFElement::EL_ROOT | EDFElement::EL_CURR;
	}

   if(szTitle != NULL)
   {
      CmdWrite(szTitle);
      CmdWrite(":\n");
   }

   pWrite = pEDF->Write(iOptions | EDFElement::PR_SPACE);
   CmdWrite(pWrite);//, CMD_OUT_RAW);
   CmdWrite("\n");
   delete pWrite;
}

void CmdEDFPrint(const char *szTitle, EDF *pEDF, bool bRoot, bool bCurr)
{
   int iOptions = 0;

   if(bRoot == true)
   {
      iOptions += EDFElement::EL_ROOT;
   }
   if(bCurr == true)
   {
      iOptions += EDFElement::EL_CURR;
   }

   CmdEDFPrint(szTitle, pEDF, iOptions);
}
