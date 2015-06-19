// ItemSlot.cpp: implementation of the CItemSlot class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ItemSlot.h"
#include "Purse.h"
#include "ItemManager.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define ClearItemBase(a)		a.dwDBIdx = 0;	\
								a.wIconIdx = 0;	\
								a.Position = 0;	\
								a.QuickPosition = 0;	\
								a.Durability = 0;	\
								a.nSealed = eITEM_TYPE_SEAL_NORMAL;	\
								a.nRemainSecond = 0;

#define ClearSlotInfo(b)		b.bLock = 0;	\
								b.wState = 0;
								
CItemSlot::CItemSlot()
{
	m_ItemBaseArray = NULL;
	m_SlotInfoArray = NULL;
	m_pPurse		= NULL;
}

CItemSlot::~CItemSlot()
{
	Release();
}

void CItemSlot::Init(POSTYPE startAbsPos, POSTYPE slotNum, ITEMBASE * pInfo, SLOTINFO * pSlotInfo)
{
	m_StartAbsPos		= startAbsPos;
	m_SlotNum			= slotNum;
	m_ItemBaseArray		= pInfo;
	m_SlotInfoArray		= pSlotInfo;
}

BOOL CItemSlot::CreatePurse( CPurse* pNewPurse, void* pOwner, MONEYTYPE money, MONEYTYPE max)
{
	if(!m_pPurse)
	m_pPurse = pNewPurse;
	return m_pPurse->InitPurse( pOwner, money, max );
}

void CItemSlot::Release()
{
	m_pPurse		= NULL;
}

ITEMBASE* CItemSlot::GetItemInfoAbs(POSTYPE absPos)
{ 
	if( !IsPosIn(absPos) )	return NULL;

	return  &m_ItemBaseArray[absPos];	
}


void CItemSlot::GetItemInfoAll(ITEMBASE * pItemBaseAll, size_t size) const
{
	if(size < m_SlotNum)
	{
		OutputDebugString(
			"error: CItemSlot::GetItemInfoAll() has overflowed copy\n");
		return;
	}

	memcpy(pItemBaseAll, &m_ItemBaseArray[m_StartAbsPos], sizeof(ITEMBASE)*m_SlotNum);
}


void CItemSlot::SetItemInfoAll(ITEMBASE * pItemBaseAll)
{
	memcpy(&m_ItemBaseArray[m_StartAbsPos], pItemBaseAll, sizeof(ITEMBASE)*m_SlotNum);
}


ERROR_ITEM CItemSlot::InsertItemAbs(CPlayer * pPlayer, POSTYPE absPos, ITEMBASE * pItem, WORD state)
{
	if( ! IsPosIn( absPos ) )
	{
		return EI_OUTOFPOS;
	}

	/*
	071211 LUJ

	if( !IsEmptyInner(absPos) )	return EI_EXISTED;
	
	������: ���� ���ԵǴ� ������ ���¸� SS_PREINSERT�� ���ý�Ű�� DB���� ó�� ����� �ö����� ����Ѵ�. 
			DBó���� ������ ��쿡�� ����Ǿ� �����Ƿ� �ش� ���Կ� ������ ���ؾ� �����̴�. �׷���
			IsEmptyInner() �Լ��� SS_NONE�� �ƴϸ� ���� ��ȯ�ϹǷ� SS_PREINSERT�� ������ ���� ��ȿȭ�ȴ�.
			
	�ذ�:	IsEmpty()�� ���� �ùٷ� ó���� �� �ִ�. ���� DB ���� �� ó���� InsertItemAbs( CPlayer*, ITEMBASE& )���� �Ѵ�
	�ǹ�:	�� IsEmptyInner()�� ������� �� �� ����. �Ƹ��� DB���� ������ �� �Ŀ��� InsertItemAbs�� ����
			�ֱ� �����̶�� ������. 
	*/
	if( ! IsEmpty( absPos ) )
	{
		return EI_EXISTED;
	}
	else if( !( SS_LOCKOMIT & state ) && IsLock( absPos ) )
	{
		return EI_LOCKED;
	}
	// 071227 LUJ, DB ���� ������� ������ ���� ����
	else if( SS_PREINSERT & m_SlotInfoArray[absPos].wState )
	{
		return EI_LOCKED;
	}

	m_ItemBaseArray[absPos] = *pItem;

	//---KES ItemDivide Fix 071020
	//---DBIDXüũ �߰�
	m_SlotInfoArray[absPos].wState = ( state & ~(SS_LOCKOMIT|SS_CHKDBIDX) );
	//----------------------------
	
	m_SlotInfoArray[absPos].bLock = FALSE;

	return EI_TRUE;
}

ERROR_ITEM CItemSlot::UpdateItemAbs(CPlayer * pPlayer, POSTYPE whatAbsPos, DWORD dwDBIdx, DWORD wItemIdx, POSTYPE position, POSTYPE quickPosition, DURTYPE Dur, WORD flag, WORD state)
{
	if( !IsPosIn(whatAbsPos) )	return EI_OUTOFPOS;
	if( (flag & UB_ABSPOS) && (position != whatAbsPos) )
	{
		//YH
		//return EI_NOTEQUALDATA;
		position = whatAbsPos;
	}
	if( !( SS_LOCKOMIT & state ) && IsLock(whatAbsPos) )  return EI_LOCKED;

	//---KES ItemDivide Fix 071020
	//---DBIDX�� ������ üũ
	if( SS_CHKDBIDX & state )
	{
		if( dwDBIdx != m_ItemBaseArray[whatAbsPos].dwDBIdx )
			return EI_NOTEQUALDATA;
	}
	 // 071227 LUJ, DB ���� ������� ������ ���� ����
	else if( SS_PREINSERT & m_SlotInfoArray[ whatAbsPos ].wState )
	{
		return EI_LOCKED;
	}
	
	if(flag & UB_ICONIDX)
	m_ItemBaseArray[whatAbsPos].wIconIdx = wItemIdx;
	if(flag & UB_QABSPOS)
	m_ItemBaseArray[whatAbsPos].QuickPosition = quickPosition;
	if(flag & UB_ABSPOS)
	m_ItemBaseArray[whatAbsPos].Position = position;
	
	//////////////////////////////////////////////////////////////////////////
	//itemmaxcheck
	// 091215 ONS ������Stack�ִ���� �����ۺ��� ������ ������ ó���Ѵ�.
	const WORD wItemStackNum = ITEMMGR->GetItemStackNum( m_ItemBaseArray[whatAbsPos].wIconIdx );
	if( ITEMMGR->IsDupItem(m_ItemBaseArray[whatAbsPos].wIconIdx) && m_ItemBaseArray[whatAbsPos].Durability > wItemStackNum )
		return EI_NOTEQUALDATA;
	//////////////////////////////////////////////////////////////////////////
	
	if(flag & UB_DURA)
	m_ItemBaseArray[whatAbsPos].Durability = Dur;
	
	//---KES ItemDivide Fix 071020
	//---DBIDXüũ �߰�
	m_SlotInfoArray[whatAbsPos].wState = ( state & ~(SS_LOCKOMIT|SS_CHKDBIDX) );
	//----------------------------

	m_SlotInfoArray[whatAbsPos].bLock = FALSE;

	return EI_TRUE;
}

ERROR_ITEM CItemSlot::DeleteItemAbs(CPlayer * pPlayer, POSTYPE absPos, ITEMBASE * pItemOut, WORD state)
{
	if( !IsPosIn(absPos) )	return EI_OUTOFPOS;
	//if( IsEmptyInner(absPos) )	return EI_NOTEXIST;
	if( IsEmpty( absPos ) )
	{
		return EI_NOTEXIST;
	}

	if( !(absPos >= TP_SHOPITEM_START && absPos < TP_SHOPITEM_END))
		if( !( SS_LOCKOMIT & state ) && IsLock(absPos) )  return EI_LOCKED;
	else if( SS_PREINSERT & m_SlotInfoArray[ absPos ].wState )
	{
		return EI_LOCKED;
	}

	if(pItemOut)
		*pItemOut = m_ItemBaseArray[absPos];

	ClearItemBase(m_ItemBaseArray[absPos]);
	ClearSlotInfo(m_SlotInfoArray[absPos]);
	return EI_TRUE;
}

/* Position�� ���� Slot�� ���ԵǴ� Position���� Check*/
BOOL CItemSlot::IsPosIn(POSTYPE absPos)
{
	if( m_StartAbsPos > absPos || absPos >= m_StartAbsPos + m_SlotNum )
		return FALSE;
	else
		return TRUE;
}

BOOL CItemSlot::IsEmpty(POSTYPE absPos)
{
	if( !m_SlotInfoArray[absPos].bLock && m_SlotInfoArray[absPos].wState == SS_NONE && m_ItemBaseArray[absPos].dwDBIdx == 0)
		return TRUE;
	else
		return FALSE;
}
BOOL CItemSlot::IsLock(POSTYPE absPos)
{
	if( m_SlotInfoArray[absPos].bLock )
		return TRUE;
	else
		return FALSE;
}

void CItemSlot::SetLock(POSTYPE absPos, BOOL val)
{
	m_SlotInfoArray[absPos].bLock = val;
}

MONEYTYPE CItemSlot::GetMoney()
{
	if( !m_pPurse ) return 0;
	return m_pPurse->GetPurseCurMoney();
}

WORD CItemSlot::GetItemCount()
{
	POSTYPE EndPos = m_StartAbsPos + m_SlotNum;
	WORD ItemNum = 0;

	for( WORD i = m_StartAbsPos ; i < EndPos ; ++i )
	{
		if( IsEmpty(i) ) continue;

		++ItemNum;
	}

	return ItemNum;	
}


ERROR_ITEM CItemSlot::InsertItemAbsFromDb( CPlayer* player, const ITEMBASE& item )
{
	if( ! IsPosIn( item.Position ) )
	{
		return EI_OUTOFPOS;
	}

	SLOTINFO& slot = m_SlotInfoArray[ item.Position ];

	if( ! ( slot.wState & SS_PREINSERT ) )
	{
		ASSERT( 0 );
		return EI_EXISTED;
	}	

	slot.wState &=	~( SS_PREINSERT | SS_LOCKOMIT | SS_CHKDBIDX );
	slot.bLock	=	FALSE;

	m_ItemBaseArray[ item.Position ] = item;
	
	return EI_TRUE;
}

// 080621 LYW --- ItemSlot : �����۰� ���� ������ �ʱ�ȭ �ϴ� �Լ� �߰�.
void CItemSlot::ClearItemBaseAndSlotInfo(POSTYPE absPos)
{
	if( !IsPosIn(absPos) )	return ;

	ClearItemBase(m_ItemBaseArray[absPos]) ;
	ClearSlotInfo(m_SlotInfoArray[absPos]) ;
}

void CItemSlot::ForcedUnseal(POSTYPE absPos)
{
	m_ItemBaseArray[absPos].nSealed = eITEM_TYPE_SEAL_NORMAL;
}

// 090122 LUJ, ���� ������ �������� �����Ѵ�
void CItemSlot::ForcedSeal(POSTYPE absPos)
{
	ITEMBASE& itemBase = m_ItemBaseArray[ absPos ];

	itemBase.nSealed = eITEM_TYPE_SEAL;
}