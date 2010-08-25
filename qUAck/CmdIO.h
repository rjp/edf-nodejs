#ifndef _CMDIO_H_
#define _CMDIO_H_

// Output options
#define CMD_OUT_NOHIGHLIGHT 1
#define CMD_OUT_RAW 2
#define CMD_OUT_UTF 4

// Extra input flags
#define CMD_LINE_EDITOR 64

// Platform functions
void CmdStartup(int iSignal);
void CmdReset(int iSignal);
void CmdShutdown(int iSignal);

int CmdType();
bool CmdLocal();
char *CmdUsername();

// Input functions
byte CmdInputGet();

// Output functions
int CmdWrite(char cData);
int CmdWrite(const char *szData, int iOptions = 0);
int CmdWrite(byte cData);
int CmdWrite(const byte *pData, int iOptions = 0);
int CmdWrite(bytes *pData, int iOptions = 0);

// Other functions
int CmdWidth();
void CmdWidth(int iWidth);
int CmdHeight();
void CmdHeight(int iHeight);
void CmdHighlight(int iHighlight);
bool CmdColourSet(int iColour, char cColour);
void CmdHardWrap(bool bHardWrap);
void CmdUTF8(bool bUTF8);
bool CmdUTF8();

void CmdPageOn();
void CmdPageOff();

bool CmdLogOpen(const char *szFilename);
bool CmdLogClose();

void CmdBeep();
void CmdRedraw(bool bFull);
void CmdWait();

void CmdEDFPrint(const char *szTitle, EDF *pEDF, int iOptions = -1);
void CmdEDFPrint(const char *szTitle, EDF *pEDF, bool bRoot, bool bCurr);

#endif
