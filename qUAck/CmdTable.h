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
** CmdTable.h: Definition of the CmdTable class
*/

#ifndef _CMDTABLE_H_
#define _CMDTABLE_H_

// Alignment types
#define CMDTABLE_LEFT 0
#define CMDTABLE_RIGHT 1

// Column widths
#define CMDTABLE_REMAINDER -1

class CmdTable
{
public:
   CmdTable(EDF *pUser, int iNumFlags, int iNumCols, bool bShowTime, bool bRepeated = false);
   ~CmdTable();

   // Add header
   bool AddHeader(const char *szTitle, int iWidth = CMDTABLE_REMAINDER, char cColour = '\0', int iAlign = CMDTABLE_LEFT, int iSpacing = 0);

   bool SetFlag(char cValue);
   bool SetValue(const char *szValue, char cColour = '\0', int iExtra = 0);
   bool SetValue(int iValue, char cColour = '\0', int iExtra = 0);

   bool AddFooter(int iValue, const char *szSingle, const char *szPlural = NULL);

   bool AddKey(char cKey, const char *szKey);

   // Implicit flush of headers / row data / footers
   bool Flush();

private:
   struct CmdHeader
   {
      char m_szTitle[100];
      int m_iWidth;
      char m_cColour;
      int m_iAlign;
      int m_iSpacing;
   };

   struct CmdColumn
   {
      char m_szValue[200];
      char m_cColour;
      int m_iExtra;
   };

   struct CmdSet
   {
      char m_szFlags[10];

      CmdColumn *m_pCols;
   };

   struct CmdFooter
   {
      int m_iValue;
      char m_szSingle[32];
      char m_szPlural[32];
   };

   struct CmdKey
   {
      char m_cKey;
      char m_szKey[32];
   };

   // Table element type (header / row / footer)
   int m_iType;

   int m_iNumFlags;
   int m_iFlagNum;

   CmdHeader *m_pHeaders;

   // Set(s) of row data
   CmdSet *m_pSets;
   int m_iNumSets;
   int m_iSetNum;
   int m_iNumCols;
   int m_iColNum;

   CmdFooter *m_pFooters;
   int m_iNumFooters;

   CmdKey *m_pKeys;
   int m_iNumKeys;

   bool m_bShowTime;

   static bool m_bDoubleReturn;

   void MakeSets();
   void CheckHeaders();
   void Update();
};

#endif
