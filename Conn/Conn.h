/*
** Conn: Generic client / server socket connect class with SSL support
** (c) 2001 Michael Wood (mike@compsoc.man.ac.uk)
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided this copyright message remains
**
** To enable SSL support define HAVE_LIBSSL at compile time
** To use this class without subclassing define CSDATAMETHODS at compile time
*/

#ifndef _CONN_H_
#define _CONN_H_

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#endif

#ifdef HAVE_LIBSSL
#include <openssl/ssl.h>
#endif

#include "useful/useful.h"

#ifdef UNIX
typedef int SOCKET;
#define SOCKADDR_IN sockaddr_in
#endif

class Conn
{
public:
   enum ConnState { CLOSED = 0, OPEN = 1, CLOSING = 2, LOST = 3 } ;

   Conn();
   virtual ~Conn();

   // Methods which affect the connection type
   virtual bool Connect(const char *szServer, int iPort, bool bSecure = false, const char *szCertFile = NULL);
   bool Bind(int iPort, const char *szCertFile = NULL);
   Conn *Accept(bool bTimeout = true);

   bool Disconnect();

   bool AcceptReset();
   bool AcceptCheck(Conn *pSocket);

   long ID();
   virtual bool Connected();
   int State();
   long Timeout();
   void Timeout(long lTimeout);

   bool SetSecure();
   bool GetSecure();

   char *Error();

   unsigned long Address();
   char *AddressStr();
   char *Hostname();
   int Port();
   unsigned long ConnectTime();
   unsigned long UpdateTime();
   bool UpdateTime(unsigned long lTime);
   double Sent();
   double Recieved();

   void *Data();
   bool Data(void *pData);

   static bool StringToAddress(const char *szAddress, unsigned long *lAddress, bool bLookup = true);
   static bool CIDRToRange(const char *szAddress, unsigned long *pMin, unsigned long *pMax);
   static char *AddressToString(unsigned long lAddress);
   static bool ValidHostname(const char *szHostname);

#ifdef CONNBASE
public:
#else
protected:
#endif
   enum ConnType { BIND = 1, CLIENT = 2, SERVER = 3 } ;
   int Type();

   // Read / write methods
   bool Read();
   long ReadBuffer(byte **pData = NULL);
   long Release(long lLength = -1);

   bool Write(const char *szData);
   bool Write(const byte *pData, long lDataLen);
   long WriteBuffer();

   bool FlagSecure();

   virtual Conn *Create(bool bSecure = false);

protected:
   virtual bool SetSocket(SOCKET iSocket);
   void Close(int iState = CLOSING);

   SOCKET Socket();
   // bool WriteBuffer();

   bool UseTimeout(bool bTimeout);
   bool UseTimeout();
   
   // Error methods
   int GetError();

private:
   int m_iType;
   long m_lID;
   unsigned long m_lAddress;
   char *m_szAddress;
   char *m_szHostname;
   int m_iPort;
   SOCKET m_iSocket;

   long m_lTimeout;
   bool m_bTimeout;

   // Read buffer
   byte *m_pRead;
   long m_lReadLen;

   // Write buffer
   byte *m_pWrite;
   long m_lWriteLen;
   long m_lWritePos;

   void *m_pData;

   fd_set m_fReads;
   fd_set m_fWrites;

#ifdef HAVE_LIBSSL
   SSL *m_pSSL;
   int m_iSecure;
   long m_lWriteFlag;
   bool m_bRead;
   bool m_bWrite;
#endif

   char *m_szError;

   unsigned long m_lConnectTime;
   unsigned long m_lUpdateTime;
   double m_dSent;
   double m_dRecieved;

   static int m_iWidth;
#ifdef WIN32
   static bool m_bWSAStartup;
#endif
#ifdef HAVE_LIBSSL
   static SSL_CTX *m_pContext;
#endif
   static long m_lMaxID;

protected:
   int m_iState;

private:
   SOCKADDR_IN *Init(int iPort, const char *szCertFile);
   bool SetSock();
   int Select(bool bTimeout, bool bRead, bool bWrite);

   void SetError(const char *szError = NULL);
#ifdef HAVE_LIBSSL
   bool SecureInit();
   void SetErrorSSL(int iError, const char *szError = NULL);
#endif
};

#endif
