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
** CmdIO-daemon.cpp: Implementation of special inetd run daemon I/O functions
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/telnet.h>
#include <unistd.h>
#include <netdb.h>

#include "Conn/EDFConn.h"

#include "ua.h"

#include "CmdIO.h"
#include "CmdIO-common.h"
#include "CmdInput.h"
#include "CmdMenu.h"

#include "qUAck.h"

char *g_szEnvUser = NULL;

// char *m_szClientName = NULL;
// char m_szClientName[64];

bool m_bStartup = true;

// int CmdServerVersion(const char *szVersion);
CmdInput *CmdInputLoop(int iMenuStatus, CmdInput *pInput, byte *pcInput, byte **pszInput);

/* char *CLIENT_NAME()
{
   // if(m_szClientName == NULL)
   {
      // m_szClientName = new char[20];
      sprintf(m_szClientName, "%s%s %s%s", CLIENT_BASE, CLIENT_SUFFIX(), CLIENT_VERSION, CLIENT_PLATFORM());
   }

   return m_szClientName;
} */

char *CLIENT_SUFFIX()
{
   return "d";
}

char *CLIENT_PLATFORM()
{
   return "";
}

int CmdStartupEnv(unsigned char *szEnv, int iEnvPos, int iType, char *szName)
{
   szEnv[iEnvPos++] = iType;
   memcpy(szEnv + iEnvPos, szName, strlen(szName));
   iEnvPos += strlen(szName);

   return iEnvPos;
}

void CmdStartup(int iSignal)
{
   bool bLoop = true, bEnvUser = false;
   int cRead = 0, cCMD = 0, cOPT = 0, iEnvPos = 0, iEnvLen = 0;
   unsigned char szEnv[50];

   debug(DEBUGLEVEL_INFO, "CmdStartup entry %d\n", iSignal);

   printf("%c%c%c%c%c%c%c%c%c", IAC, WILL, TELOPT_ECHO, IAC, WILL, TELOPT_SGA, IAC, DO, TELOPT_NEW_ENVIRON);
   fflush(stdout);

   while(bLoop == true)
   {
      cRead = CmdInputGet();
      debug(DEBUGLEVEL_DEBUG, "CmdStartup read %d, length %d\n", cRead, CmdInputLen());

      if(cRead == IAC && CmdInputLen() >= 2)
      {
         debug(DEBUGLEVEL_DEBUG, "CmdStartup IAC (%d more chars)", CmdInputLen(), cRead);
         cCMD = CmdInputGet();
         cOPT = CmdInputGet();
         debug(DEBUGLEVEL_DEBUG, " [%d][%d]\n", cCMD, cOPT);

         if(cCMD == WILL && cOPT == TELOPT_NEW_ENVIRON)
         {
            iEnvLen = 0;

            szEnv[iEnvLen++] = IAC;
            szEnv[iEnvLen++] = SB;
            szEnv[iEnvLen++] = TELOPT_NEW_ENVIRON;
            szEnv[iEnvLen++] = TELQUAL_SEND;

            iEnvLen = CmdStartupEnv(szEnv, iEnvLen, NEW_ENV_VAR, "USER");
            iEnvLen = CmdStartupEnv(szEnv, iEnvLen, ENV_USERVAR, "TERM");
            // iEnvLen = CmdStartupEnv(szEnv, iEnvLen, ENV_USERVAR, "COLUMNS");
            // iEnvLen = CmdStartupEnv(szEnv, iEnvLen, ENV_USERVAR, "LINES");

            szEnv[iEnvLen++] = IAC;
            szEnv[iEnvLen++] = SE;

            debug(DEBUGLEVEL_DEBUG, "CmdStartup env request: ");
            for(iEnvPos = 0; iEnvPos < iEnvLen; iEnvPos++)
            {
               if(isalnum(szEnv[iEnvPos]))
               {
                  debug(DEBUGLEVEL_DEBUG, "%c", szEnv[iEnvPos]);
               }
               else
               {
                  debug(DEBUGLEVEL_DEBUG, "[%d]", szEnv[iEnvPos]);
               }
            }
            debug(DEBUGLEVEL_DEBUG, "\n");

            fwrite(szEnv, sizeof(char), iEnvLen, stdout);
            fflush(stdout);

            // sleep(2);
         }
         else if(cCMD == SB && cOPT == TELOPT_NEW_ENVIRON)
         {
            debug(DEBUGLEVEL_DEBUG, "CmdStartup env settings\n");

            cRead = CmdInputGet();
            if(cRead == TELQUAL_IS)
            {
               iEnvPos = 0;
               do
               {
                  cRead = CmdInputGet();
                  if(cRead == IAC || cRead == NEW_ENV_VAR || cRead == ENV_USERVAR)
                  {
                     if(iEnvPos > 0)
                     {
                        szEnv[iEnvPos] = '\0';
                        debug(DEBUGLEVEL_DEBUG, ", value '%s'\n", szEnv);

                        if(bEnvUser == true)
                        {
                           g_szEnvUser = strmk((char *)szEnv);
                           bEnvUser = false;
                        }
                     }

                     // debug("CmdStartup env setting reset\n");
                     iEnvPos = 0;
                  }
                  else if(cRead == NEW_ENV_VALUE)
                  {
                     if(iEnvPos > 0)
                     {
                        szEnv[iEnvPos] = '\0';
                        debug(DEBUGLEVEL_DEBUG, "CmdStartup env '%s'", szEnv);

                        if(stricmp((char *)szEnv, "USER") == 0)
                        {
                           bEnvUser = true;
                        }
                     }

                     iEnvPos = 0;
                  }
                  else if(cRead == IAC)
                  {
                     cRead = CmdInputGet();
                     if(cRead == SE)
                     {
                     }
                  }
                  else
                  {
                     szEnv[iEnvPos++] = cRead;
                  }
               } while(cRead != SE && CmdInputLen() > 0);
            }
         }
      }
      else
      {
         bLoop = false;
      }
   }

   debug(DEBUGLEVEL_DEBUG, "CmdStartup %d chars in buffer\n", CmdInputLen());

   CmdWidth(80);
   CmdHeight(25);

   m_bStartup = false;

   debug(DEBUGLEVEL_INFO, "CmdStartup exit\n");
}

void CmdReset(int iSignal)
{
}

void CmdShutdown(int iSignal)
{
}

int CmdType()
{
   return 2;
}

bool CmdLocal()
{
   return false;
}

char *CmdUsername()
{
   debug(DEBUGLEVEL_DEBUG, "CmdUsername '%s'\n", g_szEnvUser);

   return g_szEnvUser;
}

// Input functions
int CmdInputGet(byte **pCurrBuffer, int iBufferLen)
{
   int iReady = 0, iBufferPos = 0;
   long lReturn = 0, lBytes = 0, lRead = 0;
   byte *pBuffer = NULL;
   fd_set fInput;
   timeval tvTimeout;
   char szError[200];

   FD_ZERO(&fInput);
   FD_SET(fileno(stdin), &fInput);

   tvTimeout.tv_sec = 0;
   tvTimeout.tv_usec = 50000; // 200000;

   iReady = select(fileno(stdin) + 1, &fInput, NULL, NULL, &tvTimeout);
   if(iReady == -1 && errno != EINTR && errno != EWOULDBLOCK)
   {
      sprintf(szError, "CmdGetInput select error, %s\n", strerror(errno));
      debug(szError);

      CmdShutdown(szError);
      // return '\0';
   }

   if(FD_ISSET(fileno(stdin), &fInput))
   {
      lReturn = ioctl(fileno(stdin), FIONREAD, &lBytes);
      if((lReturn == -1 && errno != EINTR && errno != EWOULDBLOCK) || (lReturn == 0 && lBytes == 0))
      {
         sprintf(szError, "CmdGetInput ioctl error, %s\n", strerror(errno));
         debug(szError);

         CmdShutdown(szError);
         // return '\0';
      }

      if(lBytes > 0)
      {
         if(iBufferLen == 0)
         {
            pBuffer = new byte[lBytes + 1];
         }
         else
         {
            pBuffer = new byte[iBufferLen + lBytes + 1];
            memcpy(pBuffer, (*pCurrBuffer), iBufferLen);
            delete[] (*pCurrBuffer);
         }
         *pCurrBuffer = pBuffer;

         lRead = read(fileno(stdin), pBuffer + iBufferLen, lBytes);
         if(lRead > 0)
         {
            iBufferLen += lRead;
         }
      }

      if(m_bStartup == true)
      {
         debug(DEBUGLEVEL_DEBUG, "CmdInputGet buffer: ");
         for(iBufferPos = 0; iBufferPos < iBufferLen; iBufferPos++)
         {
            if(isalnum((*pCurrBuffer)[iBufferPos]))
            {
               debug(DEBUGLEVEL_DEBUG, "%c", (*pCurrBuffer)[iBufferPos]);
            }
            else
            {
               debug(DEBUGLEVEL_DEBUG, "[%d]", (*pCurrBuffer)[iBufferPos]);
            }
         }
         debug(DEBUGLEVEL_DEBUG, "\n");
      }
   }

   return iBufferLen;
}

bool CmdInputCheck(byte cOption)
{
   if(cOption == '\0' || cOption == '\n')
   {
      return false;
   }

   return true;
}

int CmdOutput(const byte *pData, int iDataLen, bool bSingleChar, bool bUTF8)
{
   char pSingleChar[2];

   if(bSingleChar == true)
   {
      if(bUTF8 == true)
      {
         fwrite(pData, sizeof(char), iDataLen, stdout);
      }
      else
      {
         pSingleChar[0] = 168;
         pSingleChar[1] = '\0';

         fwrite(pSingleChar, sizeof(char), 1, stdout);
      }
   }
   else
   {
      fwrite(pData, sizeof(char), iDataLen, stdout);
   }

   fflush(stdout);

   return iDataLen;
}

void CmdRedraw(bool bFull)
{
   fflush(stdout);
}

void CmdBack(int iNumChars)
{
   byte szBack[] = "\b";

   while(iNumChars > 0)
   {
      CmdOutput(szBack, 1, false, false);
      iNumChars--;
   }
}

void CmdReturn()
{
   byte szReturn[] = "\r\n";
   CmdOutput(szReturn, 2, false, false);
}

void CmdAttr(char cColour)
{
   if(cColour == '0')
   {
      // Return to normal colour
      printf("\033[0m");
   }
   else if(cColour == '1')
   {
      // Bold colour
      printf("\033[1m");
   }
   else
   {
      switch(cColour)
      {
         case 'a':
         case 'A':
            // Black
            break;

         case 'r':
            // Dark red
            printf("\033[31m");
            break;

         case 'R':
            // Bright red
            printf("\033[1m\033[31m");
            break;

         case 'g':
            // Dark green
            printf("\033[32m");
            break;

         case 'G':
            // Bright green
            printf("\033[1m\033[32m");
            break;

         case 'y':
            // Dark yellow
            printf("\033[33m");
            break;

         case 'Y':
            // Bright yellow
            printf("\033[1m\033[33m");
            break;

         case 'b':
            // Dark blue
            printf("\033[34m");
            break;

         case 'B':
            // Bright blue
            printf("\033[1m\033[34m");
            break;

         case 'm':
            // Dark magenta
            printf("\033[35m");
            break;

         case 'M':
            // Bright magenta
            printf("\033[1m\033[35m");
            break;

         case 'c':
            // Dark cyan
            printf("\033[36m");
            break;

         case 'C':
            // Bright cyan
            printf("\033[1m\033[36m");
            break;

         case 'w':
            // Dark white ie. grey
            printf("\033[1m");
            break;

         case 'W':
            // Bright white
            printf("\033[0m\033[1m");
            break;
      }
   }

   fflush(stdout);
}

void CmdForeground(char cColour)
{
}

void CmdBackground(char cColour)
{
}

void CmdBeep()
{
   byte szReturn[] = "\007";
   CmdOutput(szReturn, 1, false, false);
}

void CmdWait()
{
   usleep(1000);
}

#ifdef IN6_IS_ADDR_V4MAPPED
static void mappedtov4(struct sockaddr *ss)
{
   struct sockaddr_in sin;
   struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ss;

   debug(DEBUGLEVEL_INFO, "mappedtov4 %p\n", ss);

   if (ss->sa_family == AF_INET6 &&
         IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr) ) {
      memcpy(&sin.sin_addr, sin6->sin6_addr.s6_addr+12,
            sizeof(sin.sin_addr));
      sin.sin_port = ((struct sockaddr_in6 *)ss)->sin6_port;
      sin.sin_family = AF_INET;
#ifdef SIN6_LEN
      sin.sin_len = 16;
#endif
      memcpy(ss, &sin, sizeof(sin));
   }
}
#else
#define        mappedtov4(A)
#endif

bool ProxyHost(EDF *pEDF)
{
   STACKTRACE
   int iHostname = 0, iAddress = 0;
   socklen_t iSockLen = 0;
   char szHostname[NI_MAXHOST], szAddress[NI_MAXSERV], pBuffer[BUFSIZ]; //, *szReturn = NULL;
   struct sockaddr pSockAddr;

   debug(DEBUGLEVEL_INFO, "ProxyHost entry %p\n", pEDF);

   iSockLen = sizeof(pSockAddr);
   if(getpeername(fileno(stdin), (struct sockaddr *)&pSockAddr, &iSockLen) < 0)
   {
      debug(DEBUGLEVEL_ERR, "ProxyHost getpeername failed, %s\n", strerror(errno));

      iSockLen = sizeof(pSockAddr);
      if(recvfrom(fileno(stdin), pBuffer, sizeof(pBuffer), MSG_PEEK, (struct sockaddr *)&pSockAddr, &iSockLen) < 0)
      {
         debug(DEBUGLEVEL_INFO, "ProxyHost exit false, recvfrom failed\n");
         return false;
      }
   }

   mappedtov4(&pSockAddr);

   szHostname[0] = '\0';
   szAddress[0] = '\0';

   iHostname = getnameinfo(&pSockAddr, sizeof(struct sockaddr), szHostname, sizeof(szHostname), NULL, 0, NI_NAMEREQD);
   iAddress = getnameinfo(&pSockAddr, sizeof(struct sockaddr), szAddress, sizeof(szAddress), NULL, 0, NI_NUMERICHOST);

   debug(DEBUGLEVEL_INFO, "ProxyHost '%s'(%d) / '%s'(%d)\n", szHostname, iHostname, szAddress, iAddress);

   if(strlen(szHostname) > 0)
   {
      pEDF->AddChild("hostname", szHostname);
      if(strlen(szAddress) > 0 && ProtocolVersion("2.5") >= 0)
      {
         pEDF->AddChild("address", szAddress);
      }
   }
   else if(strlen(szAddress) > 0)
   {
      pEDF->AddChild(ProtocolVersion("2.5") >= 0 ? "address" : "hostname", szAddress);
   }

   // debugEDFPrint("ProxyHost fields", pEDF);

   debug(DEBUGLEVEL_INFO, "ProxyHost exit true\n");
   return true;
}


int CmdPID()
{
   return getpid();
}

bool CmdBrowser(char *szBrowser, bool bWait)
{
   return true;
}

char *CmdBrowser()
{
   return NULL;
}

bool CmdBrowse(const char *szURI)
{
   return false;
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
   char *szInput = NULL;
   CmdInput *pInput = NULL;

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

   return szInput;
}
