#if !defined(AFX_UPDATEUI_H__20030329_21E9_E8B0_7BF5_0080AD509054__INCLUDED_)
#define AFX_UPDATEUI_H__20030329_21E9_E8B0_7BF5_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Implementation of the WTL CUpdateUI with
// dynamic allocation of a range of IDs.
template< class T >
class CUpdateDynamicUI : public CUpdateUIBase
{
public:
   CSimpleValArray<SIZE> m_ranges;
   _AtlUpdateUIMap* m_pNewMap;

   CUpdateDynamicUI() : m_pNewMap(NULL)
   {
   }
   
   virtual ~CUpdateDynamicUI()
   {
      RemoveData();
   }
   
   BOOL ReserveUIRange(UINT nStart, UINT nEnd)
   {
      // Check for overlapping ranges
      int i;
      for( i = 0; i < m_ranges.GetSize(); i++ ) {
         if( nStart < (UINT) m_ranges[i].cy && nEnd > (UINT) m_ranges[i].cx ) return FALSE;
      }
      // Add new range
      SIZE sz = { nStart, nEnd };
      m_ranges.Add(sz);

      // Remove previously created map
      RemoveData();

      // Count how many element there are now
      T* pT = static_cast<T*>(this); pT;
      const _AtlUpdateUIMap* pOrigMap = pT->GetUpdateUIMap();
      int nOrigCount = 0;
      while( pOrigMap->m_nID != (WORD) -1 ) {
         pOrigMap++;
         nOrigCount++;
      }
      int nAdditional = 0;
      for( i = 0; i < m_ranges.GetSize(); i++ ) nAdditional += m_ranges[i].cy - m_ranges[i].cx + 1;

      _AtlUpdateUIMap* pMap = NULL;
      ATLTRY(pMap = new _AtlUpdateUIMap[nOrigCount + nAdditional + 1]);
      ATLASSERT(pMap);

      delete [] m_pUIData;
      ATLTRY(m_pUIData = new _AtlUpdateUIData[nOrigCount + nAdditional + 1]);
      ATLASSERT(m_pUIData);

      // Fill out element map; start with original table
      memcpy(pMap, const_cast<_AtlUpdateUIMap*>(pT->GetUpdateUIMap()), sizeof(_AtlUpdateUIMap) * nOrigCount);
      int iIndex = nOrigCount;
      for( i = 0; i < m_ranges.GetSize(); i++ ) {
         for( int j = m_ranges[i].cx; j <= m_ranges[i].cy; j++ ) {
            pMap[iIndex].m_nID = (WORD) j;
            pMap[iIndex].m_wType = UPDUI_MENUPOPUP | UPDUI_TOOLBAR;
            iIndex++;
         }
      }
      pMap[iIndex].m_nID = (WORD) -1;
      pMap[iIndex].m_wType = 0;

      m_pUIMap = m_pNewMap = pMap;
      memset(m_pUIData, 0, sizeof(_AtlUpdateUIData) * (nOrigCount + nAdditional));
      return TRUE;
   }
   
   void RemoveData()
   {
      if( m_pNewMap == NULL ) return;
      T* pT = static_cast<T*>(this);
      delete [] const_cast<_AtlUpdateUIMap*>(m_pUIMap);
      m_pUIMap = pT->GetUpdateUIMap();
      m_pNewMap = NULL;
   }
   
   void UIClear()
   {
      for( int i = 0; i < m_ranges.GetSize(); i++ ) {
         for( int j = m_ranges[i].cx; j <= m_ranges[i].cy; j++ ) {
            UIEnable(j, FALSE, TRUE);
            UISetCheck(j, FALSE, TRUE);
         }
      }
   }
};


#endif // !defined(AFX_UPDATEUI_H__20030329_21E9_E8B0_7BF5_0080AD509054__INCLUDED_)
