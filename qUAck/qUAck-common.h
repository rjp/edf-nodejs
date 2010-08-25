#ifndef _QUACKCOMMON_H_
#define _QUACKCOMMON_H_

extern char *m_szEditor;
extern char *m_szBrowser;
extern bool m_bBrowserWait;
extern char *m_szAttachmentDir;
extern int m_iAttachmentSize;

void CmdStartup(int iSignal);
void CmdReset(int iSignal);
void CmdShutdownEntry(const char *szError);
void CmdShutdown(int iSignal);
void CmdShutdownExit(const char *szError);

// bool CmdLocal();

// Input functions
int CmdInputGet(byte **pCurrBuffer, int iBufferLen);

// Output functions
int CmdOutput(const byte *pData, int iDataLen, bool bSingleChar = false);

void CmdAttr(char cColour);
void CmdBackground(char cColour);
void CmdForeground(char cColour);

// Config functions
void CmdProgramOptions();

char *CmdFileConfig();
char *CmdFileDebug();

bool CmdFileOptions(EDF *pFile);

bool CmdFlagOptions(char **argv, int iArgNum);

bool CmdProxy(EDF *pEDF);

#endif
