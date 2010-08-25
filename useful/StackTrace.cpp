#include "stdafx.h"

#ifdef STACKTRACEON

#include <stdio.h>
#include <string.h>

#include "useful.h"

#define MAX_STACK 40
int StackTrace::m_iStackNum = 0;
struct StackTrace::StackTraceNode StackTrace::m_pNodes[MAX_STACK];
char StackTrace::m_szCurrent[100];

StackTrace::StackTrace(const char *szFile, const char *szFunction, int iLine, bool bLogging)
{
	if(m_iStackNum >= MAX_STACK)
	{
		printf("StackTrace::StackTrace (%s %s %d) cannot increase stack from %d\n", szFile, szFunction, iLine, m_iStackNum);
	}
	else
	{
		// printf("StackTrace::StackTrace entry %s %s %d\n", szFile, szFunction, iLine);
      if(bLogging == true)
      {
         debug("%-*s%s entry (mem: %ld bytes)\n", m_iStackNum, "", szFunction, memusage());
      }
		
      strcpy(m_pNodes[m_iStackNum].m_szFile, szFile);
		if(szFunction != NULL)
		{
			strcpy(m_pNodes[m_iStackNum].m_szFunction, szFunction);
			strcpy(m_szCurrent, szFunction);
		}
		else
		{
			strcpy(m_pNodes[m_iStackNum].m_szFunction, "");
			strcpy(m_szCurrent, "");
		}
      m_pNodes[m_iStackNum].m_iLine = iLine;
      m_pNodes[m_iStackNum].m_bLogging = bLogging;
      m_pNodes[m_iStackNum].m_dTick = gettick();
   }

	m_iStackNum++;
}

StackTrace::~StackTrace()
{
   m_iStackNum--;
	strcpy(m_szCurrent, m_pNodes[m_iStackNum].m_szFunction);

   if(m_pNodes[m_iStackNum].m_bLogging == true)
   {
      debug("%-*s%s exit (mem: %ld bytes, %ld ms)\n", m_iStackNum, "", m_pNodes[m_iStackNum].m_szFunction, memusage(), tickdiff(m_pNodes[m_iStackNum].m_dTick));
   }
}

void StackTrace::update(int iLine)
{
	m_pNodes[m_iStackNum - 1].m_iLine = iLine;
}

int StackTrace::depth()
{
   return m_iStackNum;
}

char *StackTrace::file(int iStackNum)
{
   return m_pNodes[iStackNum].m_szFile;
}

char *StackTrace::function(int iStackNum)
{
   return m_pNodes[iStackNum].m_szFunction;
}

int StackTrace::line(int iStackNum)
{
   return m_pNodes[iStackNum].m_iLine;
}

char *StackTrace::current()
{
   return m_szCurrent;
}

void StackPrint()
{
	int iStackNum, iNumStacks;

   iNumStacks = StackTrace::depth();
   debug("StackPrint entry, %d", iNumStacks);
#ifdef UNIX
   debug("\r");
#else
   debug("\n");
#endif

   debug("last current %p", StackTrace::current());
	fflush(stdout);
	debug(", %s", StackTrace::current());
#ifdef UNIX
   debug("\r");
#endif
   debug("\n");

	for(iStackNum = iNumStacks - 1; iStackNum >= 0; iStackNum--)
	{
      debug("%d", StackTrace::line(iStackNum));
		fflush(stdout);
      debug(", %s", StackTrace::file(iStackNum));
      fflush(stdout);
      if(strcmp(StackTrace::function(iStackNum), "") != 0)
      {
         debug(" (%s)", StackTrace::function(iStackNum));
      }
#ifdef UNIX
      debug("\r");
#endif
      debug("\n");
	}
	
	debug("StackPrint exit");
#ifdef UNIX
   debug("\r");
#endif
   debug("\n");
}

#endif
