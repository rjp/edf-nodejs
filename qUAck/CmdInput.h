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
** CmdInput.h: Declaration of input class
*/

#ifndef _CMDINPUT_H_
#define _CMDINPUT_H_

// Input types
#define CMD_TYPE_MENU 1
#define CMD_TYPE_LINE 2

// Menu options
#define CMD_MENU_SIMPLE 8
#define CMD_MENU_YESNO 16
#define CMD_MENU_TIME 32
#define CMD_MENU_ANY 64
#define CMD_MENU_SILENT 128
#define CMD_MENU_NOCASE 256

// Menu level
#define CMDLEV_BEGIN 0
#define CMDLEV_INTERMED 1
#define CMDLEV_EXPERT 2

// Line options
#define CMD_LINE_TAB 8
#define CMD_LINE_MULTI 16
#define CMD_LINE_NOTITLE 32
#define CMD_LINE_NOESCAPE 64
#define CMD_LINE_SILENT 128

// Wrapping options
#define CMD_WRAP_EDIT 1
#define CMD_WRAP_VIEW 2

#define DASHES "---------------------------------------------------\n"

typedef char *(*CMDTABFUNC)(EDF *, const char *, int, bool, int *);
typedef bool (*CMDFIELDSFUNC)(EDF *);

class CmdInput
{
public:
   // Line input
   CmdInput(CMDFIELDSFUNC pFieldsFunc = NULL, EDF *pData = NULL);
   CmdInput(const char *szTitle, int iMax, int iOptions = 0, const char *szInit = NULL, CMDTABFUNC pTabFunc = NULL, EDF *pData = NULL);
   CmdInput(const char *szTitle, const char *szExtra, int iMax, int iOptions = 0, const char *szInit = NULL, CMDTABFUNC pTabFunc = NULL, EDF *pData = NULL);
   // Menu input
   CmdInput(int iOptions, const char *szTitle);
   ~CmdInput();

   int Type();

   bool Show();
   byte Process(byte cOption);

   bool MenuAdd(char cOption, const char *szOption = NULL, const char *szHelp = NULL, bool bDefault = false);
   bool MenuValue(char cValue);
   bool MenuDefault(char cOption);

   byte *LineData();
   bool LineInit(const char *szInit);
   int LineValue(bool bCheck = false);

   static void MenuTime(long lTime);
   static long MenuTime();
   static void MenuLevel(int iLevel);
   static int MenuLevel();
   static void MenuCase(bool bCase);
   static bool MenuCase();
   static void MenuStatus(int iStatus);
   static int MenuStatus();
   static void HardWrap(int iHardWrap);
   static int HardWrap();
   static void SpellCheck(bool bSpellCheck);
   static bool SpellCheck();

   /* STATICFN bool Width(int iWidth);
   STATICFN int Width(); */

   static void Shutdown();

private:
   struct InputMenu
   {
      char m_cOption;
      bool m_bVisible;
      char *m_szOption;
      char *m_szHelp;
   };

   struct InputLine
   {
      byte *m_szLine;
      int m_iEndType;
   };

   int m_iType;
   char *m_szTitle;
   char *m_szExtra;
   char m_cReturn;

   // STATICFN int m_iWidth;

   // Menu type
   int m_iNumOptions;
   InputMenu *m_pOptions;
   char m_cDefault;
   bool m_bSorted;

   static long m_lMenuTime;
   static long m_lMenuTimeOffset;
   static int m_iMenuLevel;
   static bool m_bMenuCase;
   static int m_iMenuStatus;
   static int m_iHardWrap;
   static bool m_bSpellCheck;

   // Line type
   int m_iMax;
   int m_iLength;
   int m_iColumn;
   byte *m_szInit;
   // void *m_pFunction;
   union
   {
      CMDTABFUNC m_pTabFunc;
      CMDFIELDSFUNC m_pFieldsFunc;
   };
   EDF *m_pData;

   // Single line
   byte *m_szLine;
   int m_iTabValue;

   // Multiple lines
   int m_iRowNum;
   int m_iNumRows;
   bool m_bAbort;
   InputLine *m_pRows;
   CmdInput *m_pChild;

   // Internal menu objects for use with multi-line
   CmdInput *m_pEdit;
   static CmdInput *m_pAbort;
   static bool m_bSetup;

   void Setup(int iType, const char *szTitle, const char *szExtra);
   void LineSetup(const char *szTitle, const char *szExtra, int iMax, int iOptions, const char *szInit, void *pFunction, EDF *pData);
   void EditSetup(bool bSend);

   bool MenuFind(int cOption);
   byte MenuProcess(byte cOption);
   void MenuOptions(bool bHelp);

   void ShowOptions(bool bHelp);

   byte LineProcess(byte cOption);

   friend int MenuSort(const void *pOpt1, const void *pOpt2);

#ifdef HAVE_LIBASPELL
   int LineSpell();
   int CheckString(char *szString);
   char *NextWord(char **szString);
   bool CheckWord(char *szWord);
   EDF *SuggestWords(char *szWord);
#endif
};

#endif
