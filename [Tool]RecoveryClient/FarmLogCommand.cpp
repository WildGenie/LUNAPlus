#include "stdafx.h"
#include "FarmLogCommand.h"
#include "client.h"


CFarmLogCommand::CFarmLogCommand( CclientApp& application, const TCHAR* title ) :
CCommand( application, title )
{}


void CFarmLogCommand::SaveToExcel( DWORD serverIndex, const CListCtrl& listCtrl ) const
{
	CString textHead;
	textHead.LoadString( IDS_STRING362 );

	CString textLog;
	textLog.LoadString( IDS_STRING10 );

	CclientApp::SaveToExcel(
		textHead + textLog + _T( "-" ) + mApplication.GetServerName( serverIndex ),
		textLog,
		listCtrl );
}


void CFarmLogCommand::Initialize( CListCtrl& listCtrl ) const
{
	CRect	rect;
	int		step = -1;

	listCtrl.GetClientRect( rect );

	listCtrl.InsertColumn( ++step,  _T( "log index" ), LVCFMT_LEFT, 0 );

	CString textDate;
	textDate.LoadString( IDS_STRING3 );
	listCtrl.InsertColumn( ++step, textDate, LVCFMT_LEFT, int( rect.Width() * 0.25f ) );

	CString textType;
	textType.LoadString( IDS_STRING4 );
	listCtrl.InsertColumn( ++step, textType, LVCFMT_LEFT, int( rect.Width() * 0.25f ) );

	CString textFarmIndex;
	textFarmIndex.LoadString( IDS_STRING11 );
	listCtrl.InsertColumn( ++step, textFarmIndex, LVCFMT_RIGHT, int( rect.Width() * 0.25f ) );

	listCtrl.InsertColumn( ++step, _T( "Zone" ), LVCFMT_LEFT, int( rect.Width() * 0.25f ) );

	CString textPlayer;
	textPlayer.LoadString( IDS_STRING54 );
	listCtrl.InsertColumn( ++step, textPlayer,	LVCFMT_LEFT, int( rect.Width() * 0.25f ) );

	CString textValue;
	textValue.LoadString( IDS_STRING152 );
	listCtrl.InsertColumn( ++step, textValue + _T( " 1" ),	LVCFMT_RIGHT, int( rect.Width() * 0.25f ) );
	listCtrl.InsertColumn( ++step, textValue + _T( " 2" ),	LVCFMT_RIGHT, int( rect.Width() * 0.25f ) );
}


void CFarmLogCommand::Find( DWORD serverIndex, const TCHAR* beginTime, const TCHAR* endTime )
{
	MSG_RM_LOG_REQUEST message;
	ZeroMemory( &message, sizeof( message ) );

	message.Category		= MP_RM_FARM_LOG;
	message.Protocol		= MP_RM_FARM_LOG_SIZE_SYN;
	message.mRequestTick	= mTickCount = GetTickCount();

	strncpy( message.mBeginDate,	CW2AEX< MAX_PATH >( beginTime ),	sizeof( message.mBeginDate ) );
	strncpy( message.mEndDate,		CW2AEX< MAX_PATH >( endTime ),		sizeof( message.mEndDate ) );

	mApplication.Send( serverIndex, message, sizeof( message ) );
}


void CFarmLogCommand::Stop( DWORD serverIndex ) const
{
	MSGROOT message;
	ZeroMemory( &message, sizeof( message ) );

	message.Category	= MP_RM_FARM_LOG;
	message.Protocol	= MP_RM_FARM_LOG_STOP_SYN;

	mApplication.Send( serverIndex, message, sizeof( message ) );
}


void CFarmLogCommand::Parse( const MSGROOT* message, CListCtrl& listCtrl, CProgressCtrl& progressCtrl, CStatic& resultStatic, CButton& findButton, CButton& stopButton ) const
{
	switch( message->Protocol )
	{
	case MP_RM_FARM_LOG_ACK:
		{
			// 080401 LUJ, ���� �α׿� ���� �ε��� ��Ʈ�� ����� �ߺ����� �ʵ��� üũ�Ѵ�
			std::set< DWORD > indexSet;
			{
				for( int row = 0; row < listCtrl.GetItemCount(); ++row )
				{
					indexSet.insert( _ttoi( listCtrl.GetItemText( row, 0 ) ) );
				}
			}

			const MSG_RM_FARM_LOG* m = ( MSG_RM_FARM_LOG* )message;

			if( m->mRequestTick != mTickCount )
			{
				break;
			}

			for( DWORD i = 0; i < m->mSize; ++i )
			{
				const MSG_RM_FARM_LOG::Log&	data	= m->mLog[ i ];
				const DWORD					row		= listCtrl.GetItemCount();
				int							step	= 0;

				if( indexSet.end() != indexSet.find( data.mIndex ) )
				{
					continue;
				}

				CString text;
				text.Format( _T( "%d" ), data.mIndex );
				listCtrl.InsertItem( row, text, 0 );

				listCtrl.SetItemText( row, ++step, CA2WEX< sizeof( data.mDate ) >( data.mDate ) );
				listCtrl.SetItemText( row, ++step, mApplication.GetText( FARM_LOG_KIND( data.mKind ) ) );
				
				text.Format( _T( "%d" ), data.mFarmIndex );
				listCtrl.SetItemText( row, ++step, text );

				text.Format( _T( "%d" ), data.mZone );
				listCtrl.SetItemText( row, ++step, text );

				text.Format( _T( "%d" ), data.mPlayerIndex );
				listCtrl.SetItemText( row, ++step, text );

				// 080423 LUJ,	���� ���� �α��� ���� ���� ǥ��
				//
				//		����:	�αװ� �߸� ǥ�õǰ� ����. �ҽ��� ��ġ�� ���� �αװ� �ջ�ǹǷ�, ���������� ������
				//				�̿� ���� �α� Ÿ�԰� ���� ǥ�õǴ� �ؽ�Ʈ�� �ٸ� �� ����
				//
				//		���� -> ����, ��Ÿ�� -> ���, �� -> ��Ÿ��, â�� -> ��, ��� -> â��
				if( FARM_LOG_KIND_UPGRADE == data.mKind )
				{
					switch( data.mValue1 )
					{
					case FARM_UPGRADE_LOG_KIND_GARDEN:
						{
							CString textGarden;
							textGarden.LoadString( IDS_STRING368 );

							listCtrl.SetItemText( row, ++step, textGarden );
							break;
						}
					case FARM_UPGRADE_LOG_KIND_FENCE:
						{
							CString textCage;
							textCage.LoadString( IDS_STRING367 );

							listCtrl.SetItemText( row, ++step, textCage );
							break;
						}
					case FARM_UPGRADE_LOG_KIND_HOUSE:
						{
							CString textFence;
							textFence.LoadString( IDS_STRING366 );

							listCtrl.SetItemText( row, ++step, textFence );
							break;
						}
					case FARM_UPGRADE_LOG_KIND_WAREHOUSE:
						{
							CString textHouse;
							textHouse.LoadString( IDS_STRING369 );

							listCtrl.SetItemText( row, ++step, textHouse );
							break;
						}
					case FARM_UPGRADE_LOG_KIND_ANIMALCAGE:
						{
							CString textWarehouse;
							textWarehouse.LoadString( IDS_STRING282 );

							listCtrl.SetItemText( row, ++step, textWarehouse );
							break;
						}
					default:
						{
							listCtrl.SetItemText( row, ++step, _T( "?" ) );
							break;
						}
					}
				}
				else
				{
					text.Format( _T( "%d" ), data.mValue1 );
					listCtrl.SetItemText( row, ++step, text );
				}				

				text.Format( _T( "%d" ), data.mValue2 );
				listCtrl.SetItemText( row, ++step, text );
			}

			{
				int minRange;
				int maxRange;
				progressCtrl.GetRange( minRange, maxRange );

				progressCtrl.SetPos( progressCtrl.GetPos() + m->mSize );

				CString text;
				text.Format( _T( "%d/%d" ), progressCtrl.GetPos(), maxRange );
				resultStatic.SetWindowText( text );

				// 080523 LUJ, ��ư Ȱ��ȭ üũ�� ���������� �ǵ��� ������
				if( progressCtrl.GetPos() == maxRange )
				{
					findButton.EnableWindow( TRUE );
					stopButton.EnableWindow( FALSE );
				}
			}

			break;
		}
	case MP_RM_FARM_LOG_SIZE_ACK:
		{
			const MSG_DWORD* m = ( MSG_DWORD* )message;

			const DWORD size = m->dwData;

			CString text;
			text.Format( _T( "0/%d" ), size );

			resultStatic.SetWindowText( text );
			progressCtrl.SetRange32( 0, size );
			progressCtrl.SetPos( 0 );
			findButton.EnableWindow( FALSE );
			stopButton.EnableWindow( TRUE );

			listCtrl.DeleteAllItems();
			break;
		}
	case MP_RM_FARM_LOG_SIZE_NACK:
		{
			CString textThereIsNoResult;
			textThereIsNoResult.LoadString( IDS_STRING1 );
			MessageBox( 0, textThereIsNoResult, _T( "" ), MB_ICONERROR | MB_OK );
			break;
		}
	case MP_RM_FARM_LOG_STOP_ACK:
		{
			findButton.EnableWindow( TRUE );
			stopButton.EnableWindow( FALSE );

			CString textSearchWasStopped;
			textSearchWasStopped.LoadString( IDS_STRING2 );

			MessageBox( 0, textSearchWasStopped, _T( "" ), MB_ICONERROR | MB_OK );
			break;
		}
	case MP_RM_FARM_LOG_NACK_BY_AUTH:
		{
			CString textYouHaveNoAuthority;
			textYouHaveNoAuthority.LoadString( IDS_STRING18 );
			MessageBox( 0, textYouHaveNoAuthority, _T( "" ), MB_OK | MB_ICONERROR );
			break;
		}
	default:
		{
			ASSERT( 0 );
			break;
		}
	}
}