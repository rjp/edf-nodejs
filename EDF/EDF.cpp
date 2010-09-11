/*
** EDF - Encapsulated Data Format
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
**
** EDF.cpp: Implementation of EDF class
*/

#include "stdafx.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#include "EDF.h"

// bool m_bDebug = false;

// Constructor
EDF::EDF(const char *szData)
{
	STACKTRACE
   init();

   if(szData != NULL)
   {
      Read(szData);
   }

   // printf(" EDF::EDF root element %p\n", m_pRoot);
}

EDF::EDF(const byte *pData, const long lDataLen)
{
	STACKTRACE
   init();

   if(pData != NULL)
   {
      Read(pData, lDataLen);
   }

   // printf(" EDF::EDF root element %p\n", m_pRoot);
}

EDF::EDF(EDF *pEDF)
{
   STACKTRACE
   init();

   if(pEDF != NULL)
   {
      Copy(pEDF, true, true);
   }
}

// Destructor
EDF::~EDF()
{
	STACKTRACE
   double dTick = gettick();

   // printf(" EDF::~EDF deleting root %p\n", m_pRoot);

   delete m_pRoot;
   if(tickdiff(dTick) > 250)
   {
      debug("EDF::~EDF delete time %ld ms\n", tickdiff(dTick));
   }

   sortDelete(m_pSortRoot);
}


// Current name and value retrieval methods (methods protect against EDFElementExceptions by checking type first)
bool EDF::Get(char **szName)
{
	STACKTRACE
   if(szName != NULL)
   {
      *szName = m_pCurr->getName(m_bGetCopy);
   }

   return true;
}

bool EDF::Get(char **szName, char **szValue)
{
	STACKTRACE
   if(szName != NULL)
   {
      *szName = m_pCurr->getName(m_bGetCopy);
   }

   STACKTRACEUPDATE

   if(m_pCurr->getType() != EDFElement::BYTES && m_pCurr->getType() != EDFElement::NONE)
   {
      return false;
   }

   STACKTRACEUPDATE

   if(szValue != NULL)
   {
      *szValue = m_pCurr->getValueStr(m_bGetCopy);
   }

   STACKTRACEUPDATE

   return true;
}

bool EDF::Get(char **szName, bytes **pValue)
{
	STACKTRACE
   // long lReturn = 0;

   if(szName != NULL)
   {
      *szName = m_pCurr->getName(m_bGetCopy);
   }

   if(m_pCurr->getType() != EDFElement::BYTES && m_pCurr->getType() != EDFElement::NONE)
   {
      return false;
   }

   if(pValue != NULL)
   {
      // lReturn = m_pCurr->getValueByte(pValue, m_bGetCopy);
      *pValue = m_pCurr->getValueBytes(m_bGetCopy);
   }
   /* if(lValueLen != NULL)
   {
      *lValueLen = lReturn;
   } */

   return true;
}

bool EDF::Get(char **szName, int *iValue)
{
   return Get(szName, (long *)iValue);
}

bool EDF::Get(char **szName, long *lValue)
{
	STACKTRACE
   if(szName != NULL)
   {
      *szName = m_pCurr->getName(m_bGetCopy);
   }

   if(m_pCurr->getType() != EDFElement::INT && m_pCurr->getType() != EDFElement::FLOAT && m_pCurr->getType() != EDFElement::NONE)
   {
      // printf("EDF::Get invalid type for integer get\n");
      return false;
   }

   // printf("EDF::Get %s %d\n", m_pCurr->getName(false), m_pCurr->getValueInt());

   if((m_pCurr->getType() == EDFElement::INT || m_pCurr->getType() == EDFElement::FLOAT) && lValue != NULL)
   {
      (*lValue) = m_pCurr->getValueInt();
   }

   return true;
}

bool EDF::Get(char **szName, double *dValue)
{
	STACKTRACE
   if(szName != NULL)
   {
      *szName = m_pCurr->getName(m_bGetCopy);
   }

   if(m_pCurr->getType() != EDFElement::INT && m_pCurr->getType() != EDFElement::FLOAT && m_pCurr->getType() != EDFElement::NONE)
   {
      // printf("EDF::Get invalid type for integer get\n");
      return false;
   }

   // printf("EDF::Get %s %d\n", m_pCurr->getName(false), m_pCurr->getValueInt());

   if((m_pCurr->getType() == EDFElement::INT || m_pCurr->getType() == EDFElement::FLOAT) && dValue != NULL)
   {
      (*dValue) = m_pCurr->getValueFloat();
   }

   return true;
}

// Current name and value type independent retrieval methods
int EDF::TypeGet(char **szName, char **szValue, long *lValue, double *dValue)
{
	STACKTRACE
   int iReturn = 0;
   bytes *pValue = NULL;

   iReturn = TypeGet(szName, szValue != NULL ? &pValue : NULL, lValue, dValue);

   if(szValue != NULL && pValue != NULL)
   {
      if(m_bGetCopy == true)
      {
         *szValue = strmk((char *)pValue->Data(false), 0, pValue->Length());
      }
      else
      {
         *szValue = (char *)pValue->Data(false);
      }
   }

   if(m_bGetCopy == true)
   {
      delete pValue;
   }

   return iReturn;
}

int EDF::TypeGet(char **szName, bytes **pValue, long *lValue, double *dValue)
{
	STACKTRACE
   int iType = m_pCurr->getType();
   // long lTemp = 0;

   if(szName != NULL)
   {
      *szName = m_pCurr->getName(m_bGetCopy);
   }

   if(iType == EDFElement::BYTES && pValue != NULL)
   {
      /* lTemp = m_pCurr->getValueByte(pValue, m_bGetCopy);
      if(lValueLen != NULL)
      {
         *lValueLen = lTemp;
      } */
      (*pValue) = m_pCurr->getValueBytes(m_bGetCopy);
   }
   else if(iType == EDFElement::INT && lValue != NULL)
   {
      *lValue = m_pCurr->getValueInt();
   }
   else if(iType == EDFElement::FLOAT && dValue != NULL)
   {
      *dValue = m_pCurr->getValueFloat();
   }
   
   return iType;
}


bool EDF::Set(const char *szName, const char *szValue)
{
	STACKTRACE
   m_pCurr->set(szName);
   m_pCurr->setValue(szValue);

   return true;
}

bool EDF::Set(const char *szName, const bytes *pValue)
{
	STACKTRACE
   m_pCurr->set(szName);
   m_pCurr->setValue(pValue);

   return true;
}

bool EDF::Set(const char *szName, const int iValue)
{
	STACKTRACE
   m_pCurr->set(szName);
   m_pCurr->setValue((long)iValue);

   return true;
}

bool EDF::Set(const char *szName, const long lValue)
{
	STACKTRACE
   m_pCurr->set(szName);
   m_pCurr->setValue(lValue);

   return true;
}

bool EDF::Set(const char *szName, const unsigned long lValue)
{
	STACKTRACE
   m_pCurr->set(szName);
   m_pCurr->setValue((double)lValue);

   return true;
}

bool EDF::Set(const char *szName, const double dValue)
{
	STACKTRACE
   m_pCurr->set(szName);
   m_pCurr->setValue(dValue);

   return true;
}


// Add methods are convenience functions which map to private method
bool EDF::Add(const char *szName, const char *szValue, const int iPosition)
{
	STACKTRACE

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, true);
   m_pCurr = new EDFElement(m_pCurr, szName, szValue, iPosition);

   return true;
}

bool EDF::Add(const char *szName, const bytes *pValue, const int iPosition)
{
	STACKTRACE
   
   // return add(szName, EDFElement::BYTES, pValue, lValueLen, 0, iPosition, true);
   m_pCurr = new EDFElement(m_pCurr, szName, pValue, iPosition);
   
   return true;
}

bool EDF::Add(const char *szName, const int iValue, const int iPosition)
{
	STACKTRACE

   // return add(szName, EDFElement::INT, NULL, 0, iValue, iPosition, true);
   m_pCurr = new EDFElement(m_pCurr, szName, (long)iValue, iPosition);

   return true;
}

bool EDF::Add(const char *szName, const long lValue, const int iPosition)
{
	STACKTRACE

   // return add(szName, EDFElement::INT, NULL, 0, lValue, iPosition, true);
   m_pCurr = new EDFElement(m_pCurr, szName, lValue, iPosition);

   return true;
}

bool EDF::Add(const char *szName, const double dValue, const int iPosition)
{
	STACKTRACE

   // return add(szName, EDFElement::INT, NULL, 0, dValue, iPosition, true);
   m_pCurr = new EDFElement(m_pCurr, szName, dValue, iPosition);

   return true;
}

// Delete method removeds the current element
bool EDF::Delete()
{
	STACKTRACE
   EDFElement *pParent = NULL;

   if(m_pCurr == m_pRoot)
   {
      // Cannot delete the root element
      return false;
   }

   // printf("EDF::Delete finding parent\n");
   pParent = m_pCurr->parent();

   // printf("EDF::Delete deleting current\n");
   delete m_pCurr;

   // printf("EDF::Delete resetting current\n");
   m_pCurr = pParent;

   // printf("EDF::Delete exit true\n");
   return true;
}


bool EDF::GetChild(const char *szName, char **szValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   if(pChild->getType() == EDFElement::BYTES && szValue != NULL)
   {
      *szValue = pChild->getValueStr(m_bGetCopy);
   }
   else if(pChild->getType() == EDFElement::NONE && szValue != NULL)
   {
      *szValue = NULL;
   }

   return true;
}

bool EDF::GetChild(const char *szName, bytes **pValue, int iPosition)
{
	STACKTRACE
   // long lTemp = 0;
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   if(pChild->getType() == EDFElement::BYTES && pValue != NULL)
   {
      /* lTemp = pChild->getValueByte(pValue, m_bGetCopy);
      if(lValueLen != NULL)
      {
         *lValueLen = lTemp;
      } */
      (*pValue) = pChild->getValueBytes(m_bGetCopy);
   }

   return true;
}

bool EDF::GetChild(const char *szName, int *iValue, int iPosition)
{
	STACKTRACE
   return GetChild(szName, (long *)iValue, iPosition);
}

bool EDF::GetChild(const char *szName, long *lValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   if((pChild->getType() == EDFElement::INT || pChild->getType() == EDFElement::FLOAT) && lValue != NULL)
   {
      *lValue = pChild->getValueInt();
   }

   return true;
}

bool EDF::GetChild(const char *szName, unsigned long *lValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   if((pChild->getType() == EDFElement::INT || pChild->getType() == EDFElement::FLOAT) && lValue != NULL)
   {
      *lValue = (unsigned long)pChild->getValueFloat();
   }

   return true;
}

bool EDF::GetChild(const char *szName, double *dValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   if((pChild->getType() == EDFElement::INT || pChild->getType() == EDFElement::FLOAT) && dValue != NULL)
   {
      *dValue = pChild->getValueFloat();
   }

   return true;
}

bool EDF::GetChildBool(const char *szName, const bool bDefault, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return bDefault;
   }

   if(pChild->getType() == EDFElement::BYTES)
	{
		if(stricmp(pChild->getValueStr(false), "y") == 0 || stricmp(pChild->getValueStr(false), "yes") == 0 || stricmp(pChild->getValueStr(false), "true") == 0 || stricmp(pChild->getValueStr(false), "1") == 0)
	   {
   	   return true;
		}
		else
		{
			return false;
		}
   }
   else if(pChild->getType() == EDFElement::INT || pChild->getType() == EDFElement::FLOAT)
	{
		if(pChild->getValueInt() == 1)
	   {
   	   return true;
		}
		else
		{
			return false;
		}
   }

   return bDefault;
}

int EDF::TypeGetChild(const char *szName, char **szValue, long *lValue, double *dValue, int iPosition)
{
	STACKTRACE
   int iType = 0;
   // long lTemp = 0;
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return -1;
   }

   iType = pChild->getType();
   if(iType == EDFElement::BYTES && szValue != NULL)
   {
      /* lTemp = pChild->getValueByte(pValue, m_bGetCopy);
      if(lValueLen != NULL)
      {
         *lValueLen = lTemp;
      } */
      (*szValue) = pChild->getValueStr(m_bGetCopy);
   }
   else if(iType == EDFElement::INT && lValue != NULL)
   {
      *lValue = pChild->getValueInt();
   }
   else if(iType == EDFElement::FLOAT && dValue != NULL)
   {
      *dValue = pChild->getValueFloat();
   }

   return iType;
}

int EDF::TypeGetChild(const char *szName, bytes **pValue, long *lValue, double *dValue, int iPosition)
{
	STACKTRACE
   int iType = 0;
   // long lTemp = 0;
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return -1;
   }

   iType = pChild->getType();
   if(iType == EDFElement::BYTES && pValue != NULL)
   {
      /* lTemp = pChild->getValueByte(pValue, m_bGetCopy);
      if(lValueLen != NULL)
      {
         *lValueLen = lTemp;
      } */
      (*pValue) = pChild->getValueBytes(m_bGetCopy);
   }
   else if(iType == EDFElement::INT && lValue != NULL)
   {
      *lValue = pChild->getValueInt();
   }
   else if(iType == EDFElement::FLOAT && dValue != NULL)
   {
      *dValue = pChild->getValueFloat();
   }

   return iType;
}


bool EDF::SetChild(const char *szName, const char *szValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' '%s' %d\n", szName, szValue, iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, szValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue(szValue);

   return true;
}

bool EDF::SetChild(const char *szName, const bytes *pValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' %p %ld %d\n", szName, pValue, lValueLen, iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::BYTES, pValue, lValueLen, 0, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, pValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue(pValue);

   return true;
}

bool EDF::SetChild(const char *szName, const int iValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' %d %d\n", szName, iValue, iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::INT, NULL, 0, iValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, (long)iValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue((long)iValue);

   return true;
}

bool EDF::SetChild(const char *szName, const bool bValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' %s %d\n", szName, BoolStr(bValue), iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::INT, NULL, 0, iValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, (long)bValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue((long)bValue);

   return true;
}

bool EDF::SetChild(const char *szName, const long lValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' %ld %d\n", szName, lValue, iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::INT, NULL, 0, lValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, lValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue(lValue);

   return true;
}

bool EDF::SetChild(const char *szName, const unsigned long lValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' %ld %d\n", szName, lValue, iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::INT, NULL, 0, (double)lValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, (double)lValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue((double)lValue);

   return true;
}

bool EDF::SetChild(const char *szName, const double dValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' %ld %d\n", szName, lValue, iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::INT, NULL, 0, dValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, dValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue(dValue);

   return true;
}

bool EDF::SetChild(EDF *pEDF)
{
   STACKTRACE
   // long lValueLen = 0;
   // bytes *pValue = NULL;
   EDFElement *pElement = NULL, *pChild = NULL;

   // printf("EDF::SetChild entry %p", pEDF);
   pElement = pEDF->GetCurr();
   // printf(", %p", pElement);
   // printf("(%s)", pElement->getName(false));

   pChild = m_pCurr->child(pElement->getName(false));
   // printf(", %p\n", pChild);
   if(pChild == NULL)
   {
      if(pElement->getType() == EDFElement::BYTES)
      {
         // lValueLen = pElement->getValueByte(&pValue, false);
         // AddChild(pElement->getName(false), pValue, lValueLen);
         AddChild(pElement->getName(false), pElement->getValueBytes(false));
      }
      else if(pElement->getType() == EDFElement::INT)
      {
         AddChild(pElement->getName(false), pElement->getValueInt());
      }
      else if(pElement->getType() == EDFElement::FLOAT)
      {
         AddChild(pElement->getName(false), pElement->getValueFloat());
      }
      else
      {
         AddChild(pElement->getName(false));
      }
      return true;
   }

   if(pElement->getType() == EDFElement::BYTES)
   {
      // lValueLen = pElement->getValueByte(&pValue, false);
      // pChild->setValue(pValue, lValueLen);
      pChild->setValue(pElement->getValueBytes(false));
   }
   else if(pElement->getType() == EDFElement::INT)
   {
      pChild->setValue(pElement->getValueInt());
   }
   else if(pElement->getType() == EDFElement::FLOAT)
   {
      pChild->setValue(pElement->getValueFloat());
   }
   else
   {
      pChild->setValue((char *)NULL);
   }

   return true;
}

bool EDF::SetChild(EDF *pEDF, const char *szName, int iPosition)
{
	STACKTRACE
   bool bGetCopy = pEDF->GetCopy(false);
   int iType = 0;
   // long lValueLen = 0;
   long lValue = 0;
   double dValue = 0;
   bytes *pValue = NULL;

   // printf("EDF::SetChild %p '%s' %d\n", pEDF, szName, iPosition);

   iType = pEDF->TypeGetChild(szName, &pValue, &lValue, &dValue, iPosition);
   if(iType == -1)
   {
      pEDF->GetCopy(bGetCopy);

      return false;
   }

   if(iType == EDFElement::BYTES)
   {
      SetChild(szName, pValue);
      // delete[] pValue;
   }
   else if(iType == EDFElement::INT)
   {
      SetChild(szName, lValue);
   }
   else if(iType == EDFElement::FLOAT)
   {
      SetChild(szName, dValue);
   }

   pEDF->GetCopy(bGetCopy);

   return true;
}


bool EDF::AddChild(const char *szName, const char *szValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, false);
   pChild = new EDFElement(m_pCurr, szName, szValue, iPosition);

   return true;
}

bool EDF::AddChild(const char *szName, const bytes *pValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, false);
   pChild = new EDFElement(m_pCurr, szName, pValue, iPosition);

   return true;
}

bool EDF::AddChild(const char *szName, const int iValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, false);
   pChild = new EDFElement(m_pCurr, szName, (long)iValue, iPosition);

   return true;
}

bool EDF::AddChild(const char *szName, const bool bValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, false);
   pChild = new EDFElement(m_pCurr, szName, (long)bValue, iPosition);

   return true;
}

bool EDF::AddChild(const char *szName, const long lValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, false);
   pChild = new EDFElement(m_pCurr, szName, lValue, iPosition);

   return true;
}

bool EDF::AddChild(const char *szName, const double dValue, int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, false);
   pChild = new EDFElement(m_pCurr, szName, dValue, iPosition);

   return true;
}

bool EDF::AddChild(EDF *pEDF)
{
   STACKTRACE
   // long lValueLen = 0;
   // bytes *pValue = NULL;
   EDFElement *pElement = pEDF->GetCurr(), *pNew = NULL;

   // m_pCurr->print("EDF::AddChild entry");
   // pElement->print("EDF::AddChild source");

   if(pElement->getType() == EDFElement::BYTES)
   {
      // lValueLen = pElement->getValueByte(&pValue, false);
      // pNew = new EDFElement(m_pCurr, pElement->getName(false), pValue, lValueLen);
      pNew = new EDFElement(m_pCurr, pElement->getName(false), pElement->getValueBytes(false));
   }
   else if(pElement->getType() == EDFElement::INT)
   {
      pNew = new EDFElement(m_pCurr, pElement->getName(false), pElement->getValueInt());
   }
   else if(pElement->getType() == EDFElement::FLOAT)
   {
      pNew = new EDFElement(m_pCurr, pElement->getName(false), pElement->getValueFloat());
   }
   else
   {
      pNew = new EDFElement(m_pCurr, pElement->getName(false));
   }

   // m_pCurr->print("EDF::AddChild exit");
   return true;
}

bool EDF::AddChild(EDF *pEDF, const char *szName, int iPosition)
{
   return AddChild(pEDF, szName, szName, iPosition);
}

bool EDF::AddChild(EDF *pEDF, const char *szName, const char *szNewName, int iPosition)
{
	STACKTRACE
   bool bGetCopy = pEDF->GetCopy(false);
   int iType = 0;
   // long lValueLen = 0;
   long lValue = 0;
   double dValue = 0;
   bytes *pValue = NULL;
   EDFElement *pChild = NULL;

   iType = pEDF->TypeGetChild(szName, &pValue, &lValue, &dValue, iPosition);
   if(iType == -1)
   {
      pEDF->GetCopy(bGetCopy);

      return false;
   }

   if(iType == EDFElement::BYTES)
   {
      // add(szName, EDFElement::BYTES, pValue, lValueLen, 0, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szNewName, pValue);
      // delete[] pValue;
   }
   else if(iType == EDFElement::INT)
   {
      // add(szName, EDFElement::INT, NULL, 0, lValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szNewName, lValue);
   }
   else if(iType == EDFElement::FLOAT)
   {
      // add(szName, EDFElement::FLOAT, NULL, 0, lValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szNewName, dValue);
   }

   pEDF->GetCopy(bGetCopy);

   return true;
}


bool EDF::DeleteChild(const char *szName, const int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // m_pCurr->print("EDF::DeleteChild");

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   delete pChild;

   // m_pCurr->print("EDF::DeleteChild");

   return true;
}


int EDF::Children(const char *szName, const bool bRecurse)
{
	STACKTRACE
   return m_pCurr->children(szName, bRecurse);
}


bool EDF::Root()
{
	STACKTRACE
   m_pCurr = m_pRoot;

   return true;
}


bool EDF::Child(const char *szName, const int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   m_pCurr = pChild;

   return true;
}

bool EDF::Child(const char *szName, const char *szValue, const int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, szValue, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   m_pCurr = pChild;

   return true;
}

bool EDF::Child(const char *szName, bytes *pValue, const int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, pValue, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   m_pCurr = pChild;

   return true;
}

bool EDF::IsChild(const char *szName, const char *szValue, const int iPosition)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::IsChild '%s' '%s' %d\n", szName, szValue, iPosition);

   pChild = m_pCurr->child(szName, szValue, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   return true;
}

bool EDF::First(const char *szName)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, EDFElement::FIRST);
   if(pChild == NULL)
   {
      return false;
   }

   m_pCurr = pChild;

   return true;
}

bool EDF::Last(const char *szName)
{
	STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, EDFElement::LAST);
   if(pChild == NULL)
   {
      return false;
   }

   m_pCurr = pChild;

   return true;
}

bool EDF::Next(const char *szName)
{
	STACKTRACE
   int iChildNum = 0;
   EDFElement *pParent = NULL;

   pParent = m_pCurr->parent();
   if(pParent == NULL)
   {
      return false;
   }

   iChildNum = pParent->child(m_pCurr);

   do
   {
      iChildNum++;
   }
   // while(szName != NULL && iChildNum < pParent->children() && stricmp(pParent->child(iChildNum)->getName(false), m_pCurr->getName(false)) != 0);
   while(szName != NULL && iChildNum < pParent->children() && stricmp(pParent->child(iChildNum)->getName(false), szName) != 0);

   if(iChildNum == pParent->children())
   {
      return false;
   }

   m_pCurr = pParent->child(iChildNum);

   return true;
}

bool EDF::Prev(const char *szName)
{
	STACKTRACE
   int iChildNum = 0;
   EDFElement *pParent = NULL;

   pParent = m_pCurr->parent();
   if(pParent == NULL)
   {
      return false;
   }

   iChildNum = pParent->child(m_pCurr);

   do
   {
      iChildNum--;
   }
   // while(szName != NULL && iChildNum >= 0 && stricmp(pParent->child(iChildNum)->getName(false), m_pCurr->getName(false)) != 0);
   while(szName != NULL && iChildNum >= 0 && stricmp(pParent->child(iChildNum)->getName(false), szName) != 0);

   if(iChildNum < 0)
   {
      return false;
   }

   m_pCurr = pParent->child(iChildNum);

   return true;
}

bool EDF::Parent()
{
	STACKTRACE

   if(m_pCurr->parent() == NULL)
   {
      return false;
   }

   m_pCurr = m_pCurr->parent();

   return true;
}


bool EDF::Iterate(const char *szIter, const char *szStop, const bool bMatch, const bool bChild)
{
	STACKTRACE
   bool bIter = true, bNext = false;
   int iStopLen = 0;

   if(szStop != NULL)
   {
      iStopLen = strlen(szStop);
   }

   // printf("EDF::Iterate entry %s %d\n", m_pCurr->getName(false), m_pCurr->getType() == EDFElement::INT ? m_pCurr->getValueInt() : -1);

   // printf("EDF::Iterate entry %s, %s %s %s\n", szIter, szStop, BoolStr(bFullMatch), BoolStr(bChild));
   if(bChild == false || Child(szIter) == false)
   {
      // No child iterates, move to next iterate
      bNext = Next(szIter);
      // printf("EDF::Iterate move to next %s\n", BoolStr(bNext));

      while(bNext == false)
      {
         // printf("EDF::Iterate move %s %d\n", m_pCurr->getName(false), m_pCurr->getType() == EDFElement::INT ? m_pCurr->getValueInt() : -1);

         // No next iterate, move to one after parent
         if(szStop != NULL &&
            (bMatch == true && stricmp(m_pCurr->getName(false), szStop) == 0) ||
            (bMatch == false && strnicmp(m_pCurr->getName(false), szStop, iStopLen) == 0))
         {
            bNext = true;
            bIter = false;
         }
         else
         {
            bNext = Parent();

            // bNext is false if we're at the root (this will trap NULL values of szStop)
            if(bNext == false || (szStop != NULL &&
               (bMatch == true && stricmp(m_pCurr->getName(false), szStop) == 0) ||
               (bMatch == false && strnicmp(m_pCurr->getName(false), szStop, iStopLen) == 0)))
            {
               // Reached stop element
               bNext = true;
               bIter = false;

               // printf("EDF::Iterate stop iterating\n");
            }
            else
            {
               // Move to next iterate
               bNext = Next(szIter);
               // printf("EDF::Iterate move to next %s\n", BoolStr(bNext));
            }
         }
      }
   }
   /* else
   {
      debug("EDF::Iterate move to child\n");
   } */

   // printf("EDF::Iterate exit %s %d\n", m_pCurr->getName(false), m_pCurr->getType() == EDFElement::INT ? m_pCurr->getValueInt() : -1);

   return bIter;
}


bool EDF::SortReset(const char *szItems, const bool bRecurse)
{
   sortDelete(m_pSortRoot);

   m_pSortRoot = NULL;
   m_pSortCurr = NULL;

   return sortAdd(SORT_ITEM, szItems, bRecurse);
}

bool EDF::SortAddSection(const char *szName)
{
   return sortAdd(SORT_SECTION, szName, false);
}

bool EDF::SortAddKey(const char *szName, bool bAscending)
{
   return sortAdd(SORT_KEY, szName, bAscending);
}

bool EDF::SortParent()
{
   if(m_pSortCurr == m_pSortRoot)
   {
      return false;
   }

   m_pSortCurr = m_pSortCurr->m_pParent;

   return true;
}

bool EDF::Sort()
{
   // debugEDFPrint("EDF::Sort", this, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   return m_pCurr->sort(m_pSortRoot);
}

bool EDF::Sort(const char *szItems, const char *szKey, const bool bRecurse, const bool bAscending)
{
	STACKTRACE

   SortReset(szItems, bRecurse);
   if(SortAddKey(szKey, bAscending) == false)
   {
      return false;
   }

   return Sort();
}


long EDF::Read(const char *szData, int iProgress, int iOptions)
{
	STACKTRACE
   return Read((byte *)szData, strlen(szData));
}

long EDF::Read(const bytes *pData, int iProgress, int iOptions)
{
	STACKTRACE
   bytes *pTemp = (bytes *)pData;

   if(pTemp == NULL)
   {
      return 0;
   }

   return Read(pTemp->Data(false), pTemp->Length());
}

#define VALID_CHAR \
(pPos < pStop)

#define IS_SPACE \
(*pPos == ' ' || *pPos == '\r' || *pPos == '\n' || *pPos == '\t' || *pPos == '\0')

#define PRINT_POS \
isprint(*pPos) || *pPos == ' ' ? *pPos : '\0'

#define WHITE_SPACE(szParse) \
/*printf("EDF::Read white space %s %d '%c' [%d]\n", szParse != NULL ? szParse : "", pPos - pData, PRINT_POS, *pPos);*/\
while(VALID_CHAR && IS_SPACE)\
{\
	if(*pPos == '\n')\
	{\
		pLine = pPos + 1;\
		lLineNum++;\
	}\
	pPos++;\
}

#define PARSE_SIZE 10

#define PARSE_ERROR(szError, lError)\
{\
   debug(DEBUGLEVEL_DEBUG, "EDF::Read (%d, %ld, %d) '%c' [%d] %s\n", pPos - pData, lLineNum, pPos - pLine, PRINT_POS, *pPos, szError);\
}\
lReturn = lError;\
bLoop = false;\
bValid = false;

#define BYTES_VALUE \
dBytesVal = gettick(); \
pPos++; \
pStart = pPos; \
bLiteral = false; \
lValueLiterals = 0; \
while(VALID_CHAR && !(bLiteral == false && *pPos == '"')) \
{ \
	if(bLiteral == true) \
	{ \
      if(*pPos == '\\' || *pPos == '"') \
		{ \
			lValueLiterals++; \
		} \
		bLiteral = false; \
	} \
	else if(*pPos == '\\') \
	{ \
		bLiteral = true; \
	} \
	if(*pPos == '\n') \
	{ \
		pLine = pPos + 1; \
		lLineNum++; \
	} \
	pPos++; \
} \
lBytesVal += tickdiff(dBytesVal);

long EDF::Read(const byte *pData, const long lDataLen, const int iProgress, const int iOptions)
{
	STACKTRACE
	long lReturn = 0, lLineNum = 1, lValueLiterals = 0, lValueLen = 0;
   long lNew = 0, lSpace = 0, lSet = 0, lTick = 0, lName = 0, lBytesVal = 0, lValid = 0;
	int iDepth = 0, iNumElements = 0, iType = 0;
	bool bLoop = true, bLiteral = false, bParent = false, bValid = true, bSingleton = false, bDepth = true, bRootSet = false, bFloat = false, bXMLType = true, bXML = true;
	double dTick = gettick(), dBytesVal = 0, dNew = 0, dSpace = 0, dSet = 0, dName = 0, dValid = 0;
	char *szName = NULL, *szParse = NULL;
	byte *pPos = (byte *)pData, *pStop = pPos + lDataLen, *pStart = pPos, *pLine = pPos, *pParse = NULL, *pProgress = pPos, *pValue = NULL;
	EDFElement *pOldRoot = m_pRoot, *pOldCurr = m_pCurr, *pElement = NULL, *pEDFRoot = NULL;
	
	// if(m_bDebug == true)
	{
		debug(DEBUGLEVEL_DEBUG, "EDF::Read entry %p %ld %d\n", pData, lDataLen, iProgress);
	   // memprint("EDF::Read data", pData, lDataLen);
   }

   if(pData == NULL || lDataLen < 5)
   {
      // Has to be at least <></>
      return 0;
   }

   if(mask(iOptions, EDFElement::EN_XML) == true)
   {
      memprint(debugfile(), "EDF::Read XML parse", pData, lDataLen);

      if(strnicmp((char *)pData, "<?xml", 5) == 0 ||
         (lDataLen >= 6 && strnicmp((char *)pData, "<? xml", 6) == 0))
      {
         pPos = (byte *)pData + 5;
         while(bXML == true)
         {
            if(!VALID_CHAR)
            {
               lReturn = -2;
               bXML = false;
               bLoop = false;
            }
            else
            {
               pPos++;
               if(*pPos == '?')
               {
                  pPos++;
                  if(*pPos == '>')
                  {
                     pPos++;
                     bXML = false;
                  }
               }
            }
         }
      }

      memprint("EDF::Read XML parse", pPos, lDataLen - (pPos - pData));
   }
	
	m_pRoot = NULL;
	m_pCurr = NULL;

	while(bLoop == true)
	{
      STACKTRACEUPDATE

      if(iProgress != -1 && pPos > pProgress + iProgress)
      {
         debug(DEBUGLEVEL_DEBUG, "EDF::Read progress point %d after %ld ms\n", pPos - pData, tickdiff(dTick));
         // memprint(DEBUGLEVEL_DEBUG, debugfile(), NULL, pPos, 40, false);
         // debug(DEBUGLEVEL_DEBUG, "\n");
         pProgress = pPos;
      }

      dSpace = gettick();

		while(VALID_CHAR && *pPos != '<')
      {
			if(*pPos == '\n')
			{
				pLine = pPos;
				lLineNum++;
			}
         pPos++;
      }
		
		if(!VALID_CHAR)
		{
         lReturn = -2;
			bLoop = false;
		}
		else
		{
         STACKTRACEUPDATE

			// Start of element
			pPos++;
			
			WHITE_SPACE("before name")
			
			if(!VALID_CHAR)
         {
            PARSE_ERROR("EOD before name", -1)
         }
         else if(!(isalpha(*pPos) || *pPos == '=' || *pPos == '>' || *pPos == '/'))
			{
				PARSE_ERROR("before name", 0)
			}
			else
			{
            STACKTRACEUPDATE

				// Element name
				if(*pPos == '/')
				{
					bParent = true;
					pPos++;
				}
				else
				{
					bParent = false;
				}

            lSpace += tickdiff(dSpace);

            dName = gettick();
				
				pStart = pPos;
            // pPos++; // Skip over first chacter
				while(VALID_CHAR && (isalnum(*pPos) || *pPos == '-'))
				{
					pPos++;
				}

				if(!VALID_CHAR)
				{
					PARSE_ERROR("EOD during name", -1)
				}
				else
				{
               STACKTRACEUPDATE

               lName += tickdiff(dName);

					// Create element
					// szName = (char *)memmk(pStart, pPos - pStart);
               dNew = gettick();
               NEWCOPY(szName, pStart, pPos - pStart, char, byte);
               lNew += tickdiff(dNew);
               // printf("EDF::Read name '%s'\n", szName);
               dValid = gettick();
					if(EDFElement::validName(szName) == false)
					{
						PARSE_ERROR("at invalid name", 0)
					}
					else
					{
                  STACKTRACEUPDATE

                  lValid += tickdiff(dValid);
						if(bParent == false)
						{
                     bRootSet = true;

                     iNumElements++;
                     dNew = gettick();
                     STACKTRACEUPDATE
                     // debug(DEBUGLEVEL_DEBUG, "EDF::Read new element %p '%s' '%s'\n", m_pCurr, m_pCurr != NULL ? m_pCurr->getName(false) : NULL, szName);
                     pElement = new EDFElement(m_pCurr, szName);
                     STACKTRACEUPDATE
                     lNew += tickdiff(dNew);
                     // printf("EDF::Read new element %p, parent %p\n", pElement, m_pCurr);
                     if(m_pRoot == NULL)
                     {
                        m_pRoot = pElement;
                     }
                     m_pCurr = pElement;
                     // m_pCurr->print("EDF::Read element");
						}

                  STACKTRACEUPDATE

                  dSpace = gettick();

						if(IS_SPACE)
						{
							// After name
							WHITE_SPACE("after name")
						}

						if(!VALID_CHAR)
						{
							PARSE_ERROR("EOD after name", -1)
						}
						else
						{
                     STACKTRACEUPDATE

                     lSpace += tickdiff(dSpace);
							if(bParent == false)
							{
								if(mask(iOptions, EDFElement::EN_XML) == false && *pPos == '=')
								{
									// Value								
									pPos++;

                           dSpace = gettick();
									WHITE_SPACE("before value")
                           lSpace += tickdiff(dSpace);

									if(!VALID_CHAR)
									{
										PARSE_ERROR("EOD before value", -1)
									}
									else
									{
                              STACKTRACEUPDATE

										if(*pPos == '"')
										{
											// Byte value
											BYTES_VALUE

											if(!VALID_CHAR)
                                 {
                                    PARSE_ERROR("EOD during byte value", -1)
                                 }
                                 /* else if(*pPos != '"')
											{
												PARSE_ERROR("during byte value", 0)
											} */
											else
											{
												// Set byte value
                                    // printf("EDF::Read set byte value\n");
                                    dSet = gettick();
												m_pCurr->setValue(pStart, pPos - pStart, true);
                                    lSet += tickdiff(dSet);

												pPos++;
											}

											szParse = (char *)"after string value";
										}
										else if(isdigit(*pPos) || *pPos == '-')
										{
                                 STACKTRACEUPDATE

											// Number value
                                 pStart = pPos;
                                 bFloat = false;

                                 if(*pPos == '-')
											{
												pPos++;
											}

											while(VALID_CHAR && isdigit(*pPos))
											{
												pPos++;
                                    if(VALID_CHAR)
                                    {
                                       if(bFloat == false && *pPos == '.')
                                       {
                                          // debug("EDF::Read float value (decimal point)\n");

                                          bFloat = true;
                                          pPos++;
                                       }
                                       else if(*pPos == 'e')
                                       {
                                          // debug("EDF::Read float value (exponent)\n");

                                          bFloat = true;
                                          pPos++;

                                          if(VALID_CHAR && (*pPos == '+' || *pPos == '-'))
                                          {
                                             // debug("EDF::Read float value (exponent sign)\n");

                                             pPos++;
                                          }
                                       }
                                    }
											}

											if(!VALID_CHAR)
											{
												PARSE_ERROR("EOD during number value", -1)
											}
											else
											{
                                    STACKTRACEUPDATE

                                    // memprint("EDF::Read number value", pStart, pPos - pStart);

                                    if(bFloat == true)
                                    {
                                       // memprint("EDF::Read parse float", pStart, pPos - pStart);

                                       m_pCurr->setValue(atof((char *)pStart));
                                    }
                                    else
                                    {
                                       m_pCurr->setValue(atol((char *)pStart));
                                    }
                                 }

											szParse = (char *)"EOD after number value";
										}
									}
								}
								else if(mask(iOptions, EDFElement::EN_XML) == true && pPos + 2 <= pStop)
								{
                           bXMLType = true;

                           // memprint("EDF::Read XML value type", pPos, 7);

                           if(*pPos == 's' && *(pPos + 1) == 't' && *(pPos + 2) == 'r')
                           {
                              iType = EDFElement::BYTES;
                           }
                           else if(*pPos == 'n' && *(pPos + 1) == 'u' && *(pPos + 2) == 'm')
                           {
                              iType = EDFElement::INT;
                           }
                           else
                           {
                              bXMLType = false;
                           }

                           if(bXMLType == true)
                           {
									   pPos += 3;
   									
									   WHITE_SPACE("before XML value")
   									
									   if(!VALID_CHAR)
									   {
										   PARSE_ERROR("EOD before XML value", -1)
									   }
									   else
									   {
										   if(*pPos != '=')
										   {
											   PARSE_ERROR("Non-equals before XML value", -1)
										   }
										   else
										   {
                                    STACKTRACEUPDATE

											   pPos++;
   											
											   if(!VALID_CHAR)
											   {
												   PARSE_ERROR("EOD before XML value", -1)
											   }
											   else
											   {
                                       STACKTRACEUPDATE

												   if(*pPos != '"')
												   {
													   PARSE_ERROR("Non-quote before XML value", -1)
												   }
												   else
												   {
                                          STACKTRACEUPDATE

													   // Byte value
													   BYTES_VALUE

													   if(!VALID_CHAR)
                                 		   {
                                    		   PARSE_ERROR("EOD during XML value", -1)
                                 		   }
                                 		   /* else if(*pPos != '"')
													   {
														   PARSE_ERROR("during XML value", 0)
													   } */
													   else
													   {
                                             STACKTRACEUPDATE

														   pValue = memmk(pStart, (int)(pPos - pStart));
														   lValueLen = pPos - pStart;
														   // if(m_bDebug == true)
														   {
															   memprint(DEBUGLEVEL_DEBUG, "EDF::Read XML value", pValue, lValueLen);
														   }

														   pPos++;
   														
														   WHITE_SPACE("before XML value type")
   														
														   if(!VALID_CHAR)
														   {
															   PARSE_ERROR("EOD before XML value type", -1)
														   }
														   else if(*pPos == '/' || *pPos == '>')
														   {
                                                STACKTRACEUPDATE

                                                if(iType == EDFElement::BYTES)
                                                {
															      // printf("EDF::Read set byte value\n");
															      m_pCurr->setValue(pValue, lValueLen, true);
                                                }
                                                else
                                                {
                                                   STACKTRACEUPDATE

                                                   if(strchr((char *)pValue, '.') != NULL)
                                                   {
                                                      m_pCurr->setValue(atof((char *)pValue));
                                                   }
                                                   else
                                                   {
                                                      m_pCurr->setValue(atol((char *)pValue));
                                                   }
                                                }
														   }
														   delete[] pValue;
													   }

													   szParse = (char *)"after XML value";
												   }
											   }
                                 }
										}
									}
								}
								else
								{
									szParse = (char *)"EOD after name";
								}

                        if(VALID_CHAR)
                        {
                           dSpace = gettick();
								   WHITE_SPACE("before end of element")
                           lSpace += tickdiff(dSpace);

								   if(!VALID_CHAR)
								   {
									   PARSE_ERROR(szParse, -1)
								   }
                        }
							}
						}

                  // m_pCurr->print("EDF::Read element");

						if(VALID_CHAR)
						{
							if(bParent == false)
							{
								if(*pPos == '/')
								{
									bSingleton = true;
									pPos++;
								}
                        else
                        {
                           bSingleton = false;
                        }

                        dSpace = gettick();
								WHITE_SPACE("before end of element")
                        lSpace += tickdiff(dSpace);
							}

							if(!VALID_CHAR)
                     {
                        PARSE_ERROR("EOD before end of element", -1)
                     }
                     else if(*pPos != '>')
							{
								PARSE_ERROR("before end of element", 0)
							}
							else
							{
                        // printf("EDF::Read move to parent %s && (%s || %s)\n", BoolStr(bRootSet), BoolStr(bSingleton), BoolStr(bParent));
								if(bRootSet == true && (bSingleton == true || bParent == true))
								{
                           if(m_pCurr->parent() != NULL)
                           {
                              bDepth = true;
                              m_pCurr = m_pCurr->parent();
                           }
                           else
                           {
                              bDepth = false;
                           }
                           bLoop = bDepth;
                           /* if(m_bDebug == true)
                           {
                              debug("EDF::Read parent l=%s d=%d(%s) n=%s\n", BoolStr(bLoop), m_iDepth, BoolStr(bDepth), m_pCurr != NULL ? m_pCurr->getName(false) : "");
                           } */
								}
                        else if(bParent == true)
                        {
                           bLoop = false;
                           bValid = false;
                        }
								pPos++;
							}
						}
					}
               delete[] szName;
				}
			}
		}

      /* if(m_bDebug == true)
      {
         debug("EDF::Read loop, s='%s' p=%d d=%d\n", pPos, pPos - pData, m_iDepth);
      } */
   }

   // printf("EDF::Read %d elements\n", iNumElements);

   /* if(lReturn < 0)
   {
      debug("EDF::Read exit %d\n", lReturn);
      return lReturn;
   } */

   iDepth = Depth();
	if(mask(iOptions, EDFElement::EN_XML) == true)
	{
		debug(DEBUGLEVEL_DEBUG, "EDF::Read return %ld, parse v=%s, l=%s, d=%d(%s)\n", lReturn, BoolStr(bValid), BoolStr(bLoop), iDepth, BoolStr(bDepth));
	}
	if(lReturn < 0 || bValid == false || iDepth > 0 || (iDepth <= 0 && bDepth == true))
	{
      // printf(" EDF::Read use old root %p\n", pOldRoot);
      // if(m_bDebug == true)
      {
         int iParseLen = 0;

         if(pPos - pData > 200)
         {
            pParse = pPos - 200;
         }
         else
         {
            pParse = pPos;
         }
         if(pStop - pPos > 200)
         {
            iParseLen = 200;
         }
         else
         {
            iParseLen = pStop - pPos;
         }
         memprint(DEBUGLEVEL_DEBUG, "EDF::Read parse failed", pParse, pPos - pParse + iParseLen);
         // m_pRoot->print("EDF::Read parse failed");
      }

      // dValue = gettick();
		delete m_pRoot;
      // lDelete = tickdiff(dValue);
		
		m_pRoot = pOldRoot;
		m_pCurr = pOldCurr;

		// if(m_bDebug == true)
      lTick = tickdiff(dTick);
      /* if(lTick > 250)
		{
			debug("EDF::Read exit %ld, t=%ld ms\n", lReturn, lTick);
		} */
      if(mask(iOptions, EDFElement::EN_XML) == true)
      {
         debug("EDF::Read exit %ld, parse failed t=%ld ms\n", lReturn, lTick);
      }
		return lReturn;
	}
	else
	{
      // printf(" EDF::Read use new root %p\n", m_pRoot);

      // dValue = gettick();
		delete pOldRoot;
      // lDelete = tickdiff(dValue);

      // printf("EDF::Read delete %ld ms\n", lDelete);

      if(mask(iOptions, EDFElement::EN_XML) == true && mask(iOptions, EDFElement::EN_XML_EDFROOT) == true)
      {
         pEDFRoot = m_pRoot->child(0);
         if(pEDFRoot != NULL)
         {
            // printf("EDF::Read move root\n");
            pEDFRoot->print("EDF::Read move root", EDFElement::EL_CURR | EDFElement::PR_SPACE);

            m_pRoot->remove(0);
            delete m_pRoot;

            m_pRoot = pEDFRoot;
         }
      }
		
		Root();
	}
	
   // m_pRoot->print("EDF::Read");
	
	// if(m_bDebug == true)
   /* lTick = tickdiff(dTick);
   if(lTick > 250)
	{
		debug("EDF::Read exit %ld, t=%ld (new=%ld del=%ld set=%ld sp=%ld nm=%ld bv=%ld nv=%ld vld=%ld rem=%ld) ms\n", (long)(pPos - pData), lTick, lNew, lDelete, lSet, lSpace, lName, lBytesVal, lNumVal, lValid, lTick - lNew - lDelete - lSet - lSpace - lName - lBytesVal - lNumVal - lValid);
	} */
   if(mask(iOptions, EDFElement::EN_XML) == true)
   {
		debug("EDF::Read exit %ld, parse OK t=%ld (new=%ld set=%ld sp=%ld nm=%ld bv=%ld vld=%ld rem=%ld) ms\n", (long)(pPos - pData), lTick, lNew, lSet, lSpace, lName, lBytesVal, lValid, lTick - lNew - lSet - lSpace - lName - lBytesVal - lValid);
   }
	return pPos - pData;
}

bytes *EDF::Write(const bool bRoot, const bool bCurr, const bool bPretty, const bool bCRLF)
{
	STACKTRACE
	int iOptions = 0;

   if(bRoot == true)
   {
      iOptions |= EDFElement::EL_ROOT;
   }
	if(bCurr == true)
	{
		iOptions |= EDFElement::EL_CURR;
	}
   if(bPretty == true)
   {
      iOptions |= EDFElement::PR_SPACE;
   }
   if(bCRLF == true)
   {
      iOptions |= EDFElement::PR_CRLF;
   }

	return Write(iOptions);
}

bytes *EDF::Write(const int iOptions)
{
	STACKTRACE
   // long lReturn = 0;
   double dTick = gettick();
   EDFElement *pElement = NULL;
   bytes *pReturn = NULL;
	
	// if(m_bDebug == true)
	{
		debug(DEBUGLEVEL_DEBUG, "EDF::Write entry %d\n", iOptions);
	}

   if(mask(iOptions, EDFElement::EL_ROOT) == true)
   {
      pElement = m_pRoot;
   }
   else
   {
      pElement = m_pCurr;
   }
   pReturn = pElement->write(iOptions);

   // bytesprint("EDF::Write bytes", pReturn);

   // if(m_bDebug == true)
   {
      debug(DEBUGLEVEL_DEBUG, "EDF::Write exit %ld, %ld ms\n", pReturn->Length(), tickdiff(dTick));
   }
   return pReturn;
}


bool EDF::Copy(EDF *pEDF, const bool bSrcCurr, const bool bDestSet, const bool bRecurse)
{
	STACKTRACE
   int iChildNum = 0;
   // long lValueLen = 0;
   // bytes *pValue = NULL;
   EDFElement *pElement = pEDF->GetCurr();

   if(bSrcCurr == true)
   {
      if(bDestSet == true)
      {
         m_pCurr->set(pElement->getName(false));
         if(pElement->getType() == EDFElement::BYTES)
         {
            // lValueLen = pElement->getValueByte(&pValue, false);
            // m_pCurr->setValue(pValue, lValueLen);
            m_pCurr->setValue(pElement->getValueBytes(false));
         }
         else if(pElement->getType() == EDFElement::INT)
         {
            m_pCurr->setValue(pElement->getValueInt());
         }
         else if(pElement->getType() == EDFElement::FLOAT)
         {
            m_pCurr->setValue(pElement->getValueFloat());
         }
         else
         {
            m_pCurr->setValue((char *)NULL);
         }

         for(iChildNum = 0; iChildNum < pElement->children(); iChildNum++)
         {
            m_pCurr->copy(pElement->child(iChildNum), bRecurse);
         }
      }
      else
      {
         m_pCurr->copy(pElement, bRecurse);
      }
   }
   else
   {
      for(iChildNum = 0; iChildNum < pElement->children(); iChildNum++)
      {
         m_pCurr->copy(pElement->child(iChildNum), bRecurse);
      }
   }
   
   return true;
}


bool EDF::GetCopy(bool bCopy)
{
	STACKTRACE
   
   m_bGetCopy = bCopy;

   return true;
}

bool EDF::GetCopy()
{
	STACKTRACE
   
   return m_bGetCopy;
}


bool EDF::TempMark()
{
	STACKTRACE

   m_pTempMark = m_pCurr;

   return true;
}

bool EDF::TempUnmark()
{
	STACKTRACE


   if(m_pTempMark == NULL)
   {
      return false;
   }

   m_pCurr = m_pTempMark;

   return true;
}

EDFElement *EDF::GetCurr()
{
	STACKTRACE
   return m_pCurr;
}

bool EDF::SetCurr(EDFElement *pElement)
{
	STACKTRACE
   EDFElement *pCurr = NULL;

   // printf("EDF::SetCurr %p\n", pElement);

   if(pElement == NULL)
   {
      return false;
   }

   pCurr = pElement;
   while(pCurr->parent() != NULL)
   {
      pCurr = pCurr->parent();
   }

   // printf("EDF::SetCurr root %p %p\n", pCurr, m_pCurr);
   if(pCurr != m_pRoot)
   {
      return false;
   }

   m_pCurr = pElement;

   return true;
}

bool EDF::MoveTo(EDFElement *pElement, const int iPosition)
{
	STACKTRACE

   m_pCurr->moveTo(pElement, iPosition);

   return true;
}

bool EDF::MoveFrom(EDFElement *pElement, const int iPosition)
{
	STACKTRACE

   m_pCurr->moveFrom(pElement, iPosition);

   return true;
}


int EDF::Depth()
{
	STACKTRACE
   int iDepth = 0;
   EDFElement *pCurr = m_pCurr;

   if(m_pCurr == NULL)
   {
      return -1;
   }

   while(pCurr->parent() != NULL)
   {
      pCurr = pCurr->parent();
      iDepth++;
   }

   return iDepth;
}

int EDF::Position(const bool bName)
{
	STACKTRACE
   int iPosition = 0, iReturn = 0;
   EDFElement *pCurr = m_pCurr;

   if(m_pCurr == NULL)
   {
      return -1;
   }

   pCurr = pCurr->parent();
   if(pCurr == NULL)
   {
      return 0;
   }

   while(pCurr->child(iPosition) != m_pCurr)
   {
      if(bName == false || stricmp(pCurr->child(iPosition)->getName(false), m_pCurr->getName(false)) == 0)
      {
         iReturn++;
      }
      iPosition++;
   }

   return iReturn;
}

long EDF::Storage()
{
	STACKTRACE
   return m_pRoot->storage();
}

/* bool EDF::Debug(const bool bDebug)
{
	STACKTRACE
   bool bReturn = m_bDebug;

   m_bDebug = bDebug;

   return bReturn;
} */


void EDF::init()
{
	STACKTRACE

   m_pRoot = new EDFElement();
   m_pCurr = m_pRoot;
   m_pTempMark = NULL;

   m_bGetCopy = true;

   m_pSortRoot = NULL;
   m_pSortCurr = NULL;
}

bool EDF::sortAdd(int iType, const char *szName, bool bOption)
{
   SortElement *pElement = NULL, **pTemp = NULL;

   // debug("EDF::sortAdd %d %s %s, %s\n", iType, szName, BoolStr(bOption), m_pSortCurr != NULL ? m_pSortCurr->m_szName : "no parent");

   /* if(szName == NULL)
   {
      return false;
   } */

   pElement = new SortElement;
   pElement->m_iType = iType;
   pElement->m_szName = strmk(szName);
   if(pElement->m_iType == SORT_ITEM && bOption == true)
   {
      pElement->m_iType |= SORT_RECURSE;
   }
   else if(pElement->m_iType == SORT_KEY && bOption == true)
   {
      pElement->m_iType |= SORT_ASCENDING;
   }
   pElement->m_pChildren = NULL;
   pElement->m_iNumChildren = 0;

   pElement->m_pParent = m_pSortCurr;
   if(m_pSortRoot == NULL)
   {
      m_pSortRoot = pElement;
   }
   else
   {
      ARRAY_INSERT(SortElement *, m_pSortCurr->m_pChildren, m_pSortCurr->m_iNumChildren, pElement, m_pSortCurr->m_iNumChildren, pTemp);
   }
   m_pSortCurr = pElement;

   return true;
}

bool EDF::sortDelete(SortElement *pElement)
{
   int iChildNum = 0;

   if(pElement == NULL)
   {
      return false;
   }

   for(iChildNum = 0; iChildNum < pElement->m_iNumChildren; iChildNum++)
   {
      sortDelete(pElement->m_pChildren[iChildNum]);
   }
   delete[] pElement->m_pChildren;

   delete[] pElement->m_szName;
   delete pElement;

   return true;
}

/* #define childcomp(x, y, z) \
(stricmp(x->getName(false), y) == 0 && (z == NULL || (z != NULL && x->getType() == EDFElement::BYTES && stricmp(x->getValueStr(false), z) == 0)))

EDFElement *EDF::child(const char *szName, const char *szValue, int iPosition)
{
	STACKTRACE
   int iChildNum = 0, iNumMatches = 0;

   debug("EDF::child '%s' '%s' %d", szName, szValue, iPosition);

   if(m_pCurr->children() == 0)
   {
      debug(", NULL\n");
      return NULL;
   }

   if(szName == NULL)
   {
      if(iPosition == EDFElement::ABSFIRST || iPosition == EDFElement::FIRST)
      {
         debug(", %p (%s)\n", m_pCurr->child(0), m_pCurr->child(0)->getName(false));
         return m_pCurr->child(0);
      }
      else if(iPosition == EDFElement::ABSLAST || iPosition == EDFElement::LAST)
      {
         debug(", %p (%s)\n", m_pCurr->child(m_pCurr->children() - 1), m_pCurr->child(m_pCurr->children() - 1)->getName(false));
         return m_pCurr->child(m_pCurr->children() - 1);
      }
      else if(iPosition >= 0 && iPosition < m_pCurr->children())
      {
         debug(", %p (%s)\n", m_pCurr->child(iPosition), m_pCurr->child(iPosition)->getName(false));
         return m_pCurr->child(iPosition);
      }
   }
   else
   {
      if(iPosition == EDFElement::ABSFIRST || iPosition == EDFElement::FIRST)
      {
         while(iChildNum < m_pCurr->children() && childcomp(m_pCurr->child(iChildNum), szName, szValue) == false)
         {
            iChildNum++;
         }
      }
      else if(iPosition == EDFElement::ABSLAST || iPosition == EDFElement::LAST)
      {
         iChildNum = m_pCurr->children() - 1;
         while(iChildNum >= 0 && childcomp(m_pCurr->child(iChildNum), szName, szValue) == false)
         {
            iChildNum--;
         }
      }
      else if(iPosition >= 0 && iPosition < m_pCurr->children())
      {
         while(iChildNum < m_pCurr->children() && iNumMatches <= iPosition)
         {
            if(childcomp(m_pCurr->child(iChildNum), szName, szValue) == true)
            {
               iNumMatches++;
               if(iNumMatches == iPosition)
               {
                  debug(", %p (%s)\n", m_pCurr->child(iChildNum), m_pCurr->child(iChildNum)->getName(false));
                  return m_pCurr->child(iChildNum);
               }
            }
            iChildNum++;
         }
      }
   }

   if(iChildNum < 0 || iChildNum >= m_pCurr->children())
   {
      debug(", NULL\n");
      return NULL;
   }

   debug(", %p (%s)\n", m_pCurr->child(iChildNum), m_pCurr->child(iChildNum)->getName(false));
   return m_pCurr->child(iChildNum);
} */

/* bool EDF::add(const char *szName, const int vType, const byte *pValue, const long lValueLen, const long lValue, const int iPosition, const bool bMoveTo)
{
	STACKTRACE
   EDFElement *pNew = NULL;

   debug("EDF::add '%s' %d %p %ld %ld %d %s, %p\n", szName, vType, pValue, lValueLen, lValue, iPosition, BoolStr(bMoveTo), m_pCurr);

   if(vType == EDFElement::BYTES)
   {
      pNew = new EDFElement(m_pCurr, szName, pValue, lValueLen, iPosition);
   }
   else if(vType == EDFElement::INT)
   {
      pNew = new EDFElement(m_pCurr, szName, lValue, iPosition);
   }
   else
   {
      pNew = new EDFElement(m_pCurr, szName, iPosition);
   }

   if(bMoveTo == true)
   {
      m_pCurr = pNew;
   }

   return true;
} */

bool EDFToFile(EDF *pEDF, const char *szFilename, int iOptions)
{
	STACKTRACE
   // long lWriteLen = 0;
   bytes *pWrite = NULL;

	if(iOptions == -1)
	{
		iOptions = EDFElement::EL_ROOT | EDFElement::EL_CURR | EDFElement::PR_SPACE;
	}

   debug("EDFToFile options %d\n", iOptions);
   // EDFPrint(pEDF, true, true);
   pWrite = pEDF->Write(iOptions);
   // memprint("EDFToFile write", pWrite, lWriteLen);
   if(FileWrite(szFilename, pWrite->Data(false), pWrite->Length()) == -1)
   {
      delete pWrite;
      return false;
   }

   delete pWrite;

   return true;
}

EDF *FileToEDF(const char *szFilename, size_t *lReadLen, const int iProgress, const int iOptions)
{
	STACKTRACE
   long lEDF = 0;
	size_t lDataLen = 0;
	byte *pData = NULL;
	EDF *pEDF = NULL;

   // printf(" FileToEDF entry %s\n", szFilename);

   pData = FileRead(szFilename, &lDataLen);
   if(pData == NULL)
   {
      return NULL;
   }

   pEDF = new EDF();
   lEDF = pEDF->Read(pData, lDataLen, iProgress, iOptions);
   delete[] pData;

   if(lEDF <= 0)
   {
      delete pEDF;
      return NULL;
   }
	
   // printf(" FileToEDF exit %p\n", pEDF);
   return pEDF;
}

bool EDFPrint(EDF *pEDF, int iOptions)
{
	return EDFPrint(NULL, NULL, pEDF, iOptions);
}

bool EDFPrint(EDF *pEDF, const bool bRoot, const bool bCurr)
{
	int iOptions = EDFElement::PR_SPACE;
	
	if(bRoot == true)
	{
		iOptions |= EDFElement::EL_ROOT;
	}
	if(bCurr == true)
	{
		iOptions |= EDFElement::EL_CURR;
	}
	
	return EDFPrint(NULL, NULL, pEDF, iOptions);
}

bool EDFPrint(const char *szTitle, EDF *pEDF, const int iOptions)
{
	return EDFPrint(NULL, szTitle, pEDF, iOptions);
}

/* void EDFPrint(const char *szTitle, EDF *pEDF, const bool bRoot, const bool bCurr)
{
	int iOptions = EDFElement::PR_SPACE;
	
	if(bRoot == true)
	{
		iOptions |= EDFElement::EL_ROOT;
	}
	if(bCurr == true)
	{
		iOptions |= EDFElement::EL_CURR;
	}

	EDFPrint(NULL, szTitle, pEDF, iOptions);
} */

bool EDFPrint(FILE *fOutput, const char *szTitle, EDF *pEDF, int iOptions)
{
	STACKTRACE
   // long lWriteLen = 0;
   bytes *pWrite = NULL;

   if(fOutput == NULL)
   {
      fOutput = stdout;
   }
	
	if(iOptions == -1)
	{
		iOptions = EDFElement::EL_ROOT | EDFElement::EL_CURR | EDFElement::PR_SPACE;
	}

   // printf("EDFPrint options %d\n", iOptions);

   if(pEDF != NULL)
   {
      pWrite = pEDF->Write(iOptions);
   }
   if(szTitle != NULL)
   {
      fprintf(fOutput, "%s:\n", szTitle);
   }
   if(pWrite != NULL)
   {
      fwrite(pWrite->Data(false), 1, pWrite->Length(), fOutput);
      if(pWrite->Length() > 0)
      {
         fprintf(fOutput, "\n");
      }
   }
   else
   {
      fwrite("NULL\n", 1, 5, fOutput);
   }

   delete pWrite;

   return true;
}

bool EDFPrint(FILE *fOutput, const char *szTitle, EDF *pEDF, const bool bRoot, const bool bCurr)
{
	int iOptions = EDFElement::PR_SPACE;
	
	if(bRoot == true)
	{
		iOptions |= EDFElement::EL_ROOT;
	}
	if(bCurr == true)
	{
		iOptions |= EDFElement::EL_CURR;
	}

	return EDFPrint(fOutput, szTitle, pEDF, iOptions);
}

bool debugEDFPrint(EDF *pEDF, const int iOptions)
{
	return debugEDFPrint(0, NULL, pEDF, iOptions);
}

bool debugEDFPrint(EDF *pEDF, const bool bRoot, const bool bCurr)
{
	int iOptions = EDFElement::PR_SPACE;
	
	if(bRoot == true)
	{
		iOptions |= EDFElement::EL_ROOT;
	}
	if(bCurr == true)
	{
		iOptions |= EDFElement::EL_CURR;
	}
	
	return debugEDFPrint(0, NULL, pEDF, iOptions);
}

bool debugEDFPrint(int iLevel, EDF *pEDF, const int iOptions)
{
	return debugEDFPrint(iLevel, NULL, pEDF, iOptions);
}

bool debugEDFPrint(const char *szTitle, EDF *pEDF, const int iOptions)
{
	return debugEDFPrint(0, szTitle, pEDF, iOptions);
}

bool debugEDFPrint(int iLevel, const char *szTitle, EDF *pEDF, const int iOptions)
{
   if(iLevel > debuglevel())
   {
      return false;
   }

	return EDFPrint(debugfile(), szTitle, pEDF, iOptions);
}

/* void debugEDFPrint(const char *szTitle, EDF *pEDF, const bool bRoot, const bool bCurr)
{
	int iOptions = EDFElement::PR_SPACE;
	
	if(bRoot == true)
	{
		iOptions |= EDFElement::EL_ROOT;
	}
	if(bCurr == true)
	{
		iOptions |= EDFElement::EL_CURR;
	}

	EDFPrint(debugfile(), szTitle, pEDF, iOptions);
} */

bool XMLPrint(const char *szTitle, EDF *pEDF, int iOptions)
{
   if(iOptions == -1)
   {
      iOptions = EDFElement::EL_ROOT | EDFElement::EL_CURR | EDFElement::PR_SPACE | EDFElement::EN_XML;
   }
   else
   {
      iOptions |= EDFElement::EN_XML;
   }

	return EDFPrint(NULL, szTitle, pEDF, iOptions);
}

int EDFMax(EDF *pEDF, const char *szName, bool bRecurse)
{
   int iID = 0, iMaxID = 0, iValue = 0;
   bool bLoop = false;

   // printf("EDFMax entry %p %s %s, %d children\n", pEDF, szName, BoolStr(bRecurse), pEDF->Children(szName));

   bLoop = pEDF->Child(szName);
   while(bLoop == true)
   {
      pEDF->Get(NULL, &iID);
      if(iID > iMaxID)
      {
         iMaxID = iID;
      }

      if(bRecurse == true)
      {
         iValue = EDFMax(pEDF, szName, true);
         if(iValue > iMaxID)
         {
            iMaxID = iValue;
            // debug("EDFMax %s %d\n", szName, iMaxID);
         }
      }

      bLoop = pEDF->Next(szName);
      if(bLoop == false)
      {
         pEDF->Parent();
      }
   }

   // printf("EDFMax exit %d\n", iMaxID);
   return iMaxID;
}

int EDFSetStr(EDF *pDest, const char *szName, bytes *pBytes, int iMax, int iOptions)
{
   int iLen = 0;
   bytes *pTemp = NULL;

   if(pBytes != NULL)
   {
      iLen = pBytes->Length();
      if(iLen > iMax)
      {
         debug("EDFSetStr cut string %ld to %d\n", pBytes->Length(), iMax);
         // szTemp = strmk(szValue, 0, iMax);
         pTemp = new bytes(pBytes, iMax);
      }
      else
      {
         // szTemp = strmk(szValue);
         pTemp = new bytes(pBytes);
      }

      pDest->SetChild(szName, pTemp);

      delete pTemp;
   }
   else if(mask(iOptions, EDFSET_REMOVEIFNULL) == true)
   {
      // printf("EDFSetStr NULL remove\n");
      pDest->DeleteChild(szName);
   }

   return iLen;
}

int EDFSetStr(EDF *pDest, EDF *pSrc, const char *szName, int iMax, int iOptions, const char *szDestName)
{
   STACKTRACE
   int iLen = -1;
   bytes *pValue = NULL;

   // printf("EDFSetStr %s %d %d\n", szName, iMax, iOptions);
   // EDFPrint(pSrc, false);

   if(pSrc->Child(szName) == true)
   {
      pSrc->Get(NULL, &pValue);
      iLen = EDFSetStr(pDest, szDestName != NULL ? szDestName : szName, pValue, iMax, iOptions);
      pSrc->Parent();
      delete pValue;
   }
   else if(mask(iOptions, EDFSET_REMOVEIFMISSING) == true)
   {
      // printf("EDFSetStr missing remove\n");
      pDest->DeleteChild(szName);
   }

   return iLen;
}

bool EDFFind(EDF *pEDF, const char *szName, int iID, bool bRecurse)
{
   bool bLoop = true;
   EDFElement *pElement = NULL;

   // debug("EDFFind entry %p %s %d %s\n", pEDF, szName, iID, BoolStr(bRecurse));

   bLoop = pEDF->Child(szName);
   while(bLoop == true)
   {
      pElement = pEDF->GetCurr();
      if(pElement->getType() == EDFElement::INT || pElement->getType() == EDFElement::FLOAT)
      {
         // debug("EDFFind value %ld\n", pElement->getValueInt());
         if(pElement->getValueInt() == iID)
         {
            // debug("EDFFind exit true, %p\n", pElement);
            return true;
         }
      }

      if(bRecurse == true)
      {
         if(EDFFind(pEDF, szName, iID, true) == true)
         {
            // debug("EDFFind exit true\n");
            return true;
         }
      }

      bLoop = pEDF->Next(szName);
      if(bLoop == false)
      {
         pEDF->Parent();
      }
   }

   // debug("EDFFind exit false\n");
   return false;
}

int EDFDelete(EDF *pEDF, char *szName, bool bRecurse)
{
   int iReturn = 0;
   bool bLoop = false;

   if(bRecurse == true)
   {
      bLoop = pEDF->Child();
      while(bLoop == true)
      {
         iReturn += EDFDelete(pEDF, szName, true);

         bLoop = pEDF->Next();
         if(bLoop == false)
         {
            pEDF->Parent();
         }
      }
   }

   while(pEDF->DeleteChild(szName) == true)
   {
      iReturn++;
   }

   return iReturn;
}
