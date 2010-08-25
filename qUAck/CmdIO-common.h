#ifndef _CMDIOCOMMON_H_
#define _CMDIOCOMMON_H_

// Input functions
int CmdInputGet(byte **pCurrBuffer, int iBufferLen);
bool CmdInputCheck(byte cOption);
int CmdInputLen();
byte CmdInputChar(int iCharNum);
int CmdInputRelease(int iNumChars = -1);

// Output functions
int CmdOutput(const byte *pData, int iDataLen, bool bSingleChar, bool bUTF8);

void CmdBack(int iNumChars = 1);
void CmdReturn();

void CmdAttr(char cColour);

void CmdBackground(char cColour);
void CmdForeground(char cColour);

#endif
