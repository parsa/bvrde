#if !defined(AFX_ATLCOLLECTIONS_H__20030722_E44A_A98A_2167_0080AD509054__INCLUDED_)
#define AFX_ATLCOLLECTIONS_H__20030722_E44A_A98A_2167_0080AD509054__INCLUDED_

/////////////////////////////////////////////////////////////////////////////
// Additional Maps and Collection data-types
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2003 Bjarke Viksoe.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//

#pragma once

#ifndef __cplusplus
   #error ATL requires C++ compilation (use a .cpp suffix)
#endif


/////////////////////////////////////////////////////////////////////////////
// Simple List

template< class T > 
class CSimpleList
{
public:
   T* m_pHead;

   CSimpleList() : m_pHead(NULL)
   {
   }

   ~CSimpleList()
   {
      RemoveAll();
   }

   void RemoveAll()
   {
      while( !IsEmpty() ) Remove(m_pHead);
      m_pHead = NULL;
   }
   
   BOOL IsEmpty() const
   {
      return m_pHead == NULL;
   }
   
   T* Unlink(T* Arg)
   {
      if( Arg && m_pHead ) {
         if( Arg == m_pHead ) {
           m_pHead = Arg->m_pNext;
           return Arg;
         }
         else {
           T* p = m_pHead;
           while( p && p->m_pNext != Arg ) p = p->m_pNext;
           if( p ) {
             p->m_pNext = Arg->m_pNext;
             return Arg;
           }
         }
      }
      return NULL;
   }
   
   BOOL Remove(T* Arg)
   {
      Arg = Unlink(Arg);
      if( Arg ) delete Arg;
      return Arg != NULL;
   }
   
   T* Append(T* Arg)
   {
      if( Arg ) {
         Arg->m_pNext = NULL;
         if( IsEmpty() ) {
            m_pHead = Arg;
         }
         else {
            T* p = m_pHead;
            while( p->m_pNext ) p = p->m_pNext;
            p->m_pNext = Arg;
         }
      }
      return Arg;
   }
   
   T* Push(T* Arg)
   {
    if( Arg ) {
      Arg->m_pNext = m_pHead;
      m_pHead = Arg;
    }
    return Arg;
   }
   
   template< typename TValue >
   T* Find(TValue Value) const
   {
      T* p = m_pHead;
      while( p && !p->Compare(Value) ) p = p->m_pNext;
      return p;
   }
}; 


/////////////////////////////////////////////////////////////////////////////
// Hash key generator

inline UINT HashKey(LPCTSTR Key)
{
   int len = ::lstrlen(Key);
   UINT i = 0;
   while( len-- ) i = (i << 5) + i + Key[len];
   return i;
};

#if defined(__ATLSTR_H__) || defined(_WTL_USE_CSTRING)

inline UINT HashKey(const CString& Key)
{
   return HashKey((LPCTSTR)Key);
};

#endif

template< class TKey >
inline UINT HashKey(const TKey& Key)
{
   return (UINT) Key;
};


/////////////////////////////////////////////////////////////////////////////
// Hash Map

template< class TKey, class TItem >
class CHashMap
{
public:
   typedef struct ITEM
   {
      TKey Key;
      TItem Data;
      struct ITEM* pNext;
   };
   ITEM** m_aT;
   int m_nBuckets;

   CHashMap(int nSize = 83)
   {
      m_nBuckets = nSize;
      m_aT = new ITEM*[nSize];
      memset(m_aT, 0, nSize * sizeof(ITEM*));
   }

   ~CHashMap()
   {
      int len = m_nBuckets;
      while( len-- ) {
         ITEM* pItem = m_aT[len];
         while( pItem ) {
            ITEM* pKill = pItem;
            pItem = pItem->pNext;
            delete pKill;
         }
      }
      delete [] m_aT;
   }
   
   void Resize(int nSize)
   {
      ATLASSERT(GetSize()==0);
      delete [] m_aT;
      m_aT = new ITEM*[nSize];
      memset(m_aT, 0, nSize * sizeof(ITEM*));
      m_nBuckets = nSize;
   }
   
   bool Find(const TKey& Key, TItem** ppData = NULL) const
   {
      UINT slot = HashKey(Key) % m_nBuckets;
      for( const ITEM* pItem = m_aT[slot]; pItem; pItem = pItem->pNext ) {
         if( pItem->Key == Key ) {
            if( ppData ) *ppData = &pItem->Data;
            return true;
         }        
      }
      return false;
   }
   
   bool Lookup(const TKey& Key, TItem& Data) const
   {
      UINT slot = HashKey(Key) % m_nBuckets;
      for( const ITEM* pItem = m_aT[slot]; pItem; pItem = pItem->pNext ) {
         if( pItem->Key == Key ) {
            Data = pItem->Data;
            return true;
         }
      }
      return false;
   }
   
   bool Insert(const TKey& Key, const TItem& Item)
   {
      // Cannot accept duplicates
      TItem temp;
      if( Lookup(Key, temp) ) {
         ATLASSERT(false);
         return false;
      }
      // Add first in bucket
      UINT slot = HashKey(Key) % m_nBuckets;
      ITEM* pItem = new ITEM;
      pItem->Key = Key;
      pItem->Data = Item;
      pItem->pNext = m_aT[slot];
      m_aT[slot] = pItem;
      return true;
   }
   
   bool Set(const TKey& Key, const TItem& Data)
   {
      UINT slot = HashKey(Key) % m_nBuckets;
      // Modify existing item
      for( ITEM* pItem = m_aT[slot]; pItem; pItem = pItem->pNext ) {
         if( pItem->Key == Key ) {
            pItem->Data = Data;
            return true;
         }
      }
      // Insert new instead
      return Insert(Key, Data);
   }
   
   bool Remove(const TKey& Key)
   {
      UINT slot = HashKey(Key) % m_nBuckets;
      ITEM** ppItem = &m_aT[slot];
      while( *ppItem ) {
         if( (*ppItem)->Key == Key ) {
            ITEM* pKill = *ppItem;
            *ppItem = (*ppItem)->pNext;
            delete pKill;
            return true;
         }
         ppItem = &((*ppItem)->pNext);
      }
      return false;
   }
   
   int GetSize() const
   {
      int nCount = 0;
      int len = m_nBuckets;
      while( len-- ) {
         for( const ITEM* pItem = m_aT[len]; pItem; pItem = pItem->pNext ) nCount++;
      }
      return nCount;
   }
   
   bool GetItemAt(int iIndex, TKey& Key, TItem& Data) const
   {
      int pos = 0;
      int len = m_nBuckets;
      while( len-- ) {
         for( const ITEM* pItem = m_aT[len]; pItem; pItem = pItem->pNext ) {
            if( pos++ == iIndex ) {
               Key = pItem->Key;
               Data = pItem->Data;
               return true;
            }
         }
      }
      return false;
   }
};


#endif // !defined(AFX_ATLCOLLECTIONS_H__20030722_E44A_A98A_2167_0080AD509054__INCLUDED_)

