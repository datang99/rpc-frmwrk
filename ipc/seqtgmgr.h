/*
 * =====================================================================================
 *
 *       Filename:  seqtgmgr.h
 *
 *    Description:  declarations of classes and  functions for sequential tasks
 *
 *        Version:  1.0
 *        Created:  10/07/2023 08:46:09 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once
#include "ifhelper.h"

struct SEQTG_ELEM
{
    TaskGrpPtr m_pSeqTasks;
    EnumTaskState m_iState = stateStarting;
    SEQTG_EMEM( const SEQTG_ELEM& elem )
    {
        m_pSeqTasks = elem.m_pSeqTasks;
        m_iState = elem.m_iState;
    }
};

template< class Handle, class HostClass >
struct CSeqTaskGrpMgr : public CObjBase
{
    HostClass* m_pParent;
    std::hashmap< Handle, SEQTG_ELEM > m_mapSeqTgs;
    EnumIfState m_iMgrState = stateStarted;

    typedef CObjBase super;
    CSeqTaskGrpMgr() : super()
    {}

    inline void SetParent( HostClass* pParent )
    { m_pParent = pParent; }

    inline HostClass* GetParent()
    { return m_pParent; }

    inline stdrmutex& GetLock()
    { return m_pParent->GetLock(); }

    inline void SetState( EnumIfState iState )
    {
        CStdRMutex oLock( GetLock() )
        m_iMgrState = iState;
    }

    gint32 AddStartTask(
        Handle htg, TaskletPtr& pTask )
    {
        if( htg == 0 || pTask.IsEmpty() )
            return -EINVAL;

        gint32 ret = 0;
        CStdRMutex oLock( GetLock() );
        if( m_iMgrState != stateStarted )
        {
            ret = ERROR_STATE;
            break;
        }
        auto itr = m_mapSeqTgs.find( htg );
        if( itr != m_mapSeqTgs.end() )
            return -EEXIST;

        SEQTG_ELEM otg;
        m_mapSeqTgs[ htg ] = otg;
        itr = m_mapSeqTgs.find( htg );
        auto& pSeqTasks =
            itr->second.m_pSeqTasks;
        oLock.Unlock();

        ret = AddSeqTaskIf(
            GetParent(), pSeqTasks, pTask );

        oLock.Lock();
        if( ERROR( ret ) )
        {
            m_mapSeqTgs.erase( htg );
        }
        else
        {
            itr->second.m_iState = stateStarted;
        }
        return ret;
    }

    gint32 AddSeqTask( Handle htg,
        TaskletPtr& pTask )
    {
        if( htg == 0 || pTask.IsEmpty() )
            return -EINVAL;

        gint32 ret = 0;
        do{
            CStdRMutex oLock( GetLock() );
            if( m_iMgrState != stateStarted )
            {
                ret = ERROR_STATE;
                break;
            }
            auto itr = m_mapSeqTgs.find( htg );
            if( itr == m_mapSeqTgs.end() )
            {
                ret = -ENOENT;
                break;
            }

            if( itr->second.m_iState != stateStarted )
            {
                ret = ERROR_STATE;
                break;
            }
            oLock.Unlock();
            ret = AddSeqTaskIf( GetParent(),
                itr->second.m_pSeqTasks, pTask );
        }while( 0 );

        return ret;
    }

    gint32 AddStopTask( 
        IEventSink* pCallback, Handle htg,
        TaskletPtr& pTask )
    {
        if( htg == 0 || pTask.IsEmpty() )
            return -EINVAL;

        gint32 ret = 0;
        do{
            CStdRMutex oLock( GetLock() );
            if( m_iMgrState == stateStopped )
            {
                ret = ERROR_STATE;
                break;
            }
            auto itr = m_mapSeqTgs.find( htg );
            if( itr == m_mapSeqTgs.end() )
            {
                ret = -ENOENT;
                break;
            }

            if( itr->second.m_iState != stateStarted )
            {
                ret = ERROR_STATE;
                break;
            }

            TaskGrpPtr& pSeqTasks =
                itr->second.m_pSeqTasks;

            itr->second.m_iState = stateStopped;

            oLock.Unlock();

            ret = AddSeqTaskIf(
                GetParent(), pSeqTasks, pTask );

            if( ERROR( ret ) )
                break;

            gint32 (*func)( ObjPtr&,
                Handle, IEventSink* ) =

            ([]( ObjPtr& ptgm, Handle htg,
                IEventSink* pCb )->gint32
            {
                gint32 ret = 0;
                do{
                    if( ptgm.IsEmpty() || htg == 0 )
                    {
                        ret = -EINVAL;
                        if( pCb != nullptr )
                        {
                            pCb->OnEvent(
                                eventTaskComp,
                                -EINVAL, 0, 0 );
                        }
                        break;
                    }
                    CSeqTaskGrpMgr* pTgMgr = ptgm;
                    CRpcServices* pSvc =
                        pTgMgr->GetParent();

                    CIoManager* pMgr;
                    TaskletPtr pRemove;
                    ret = DEFER_OBJCALL_NOSCHED(
                        pRemove, pTgMgr,
                        &CSeqTaskGrpMgr::RemoveTg,
                        htg );
                    if( ERROR( ret ) )
                        break;

                    if( pCb != nullptr )
                    {
                        CIfRetryTask* pRetry = pRemove;
                        pRetry->SetClientNotify( pCb );
                    }

                    TaskletPtr pManaged;
                    ret = DEFER_CALL_NOSCHED(
                        pManaged, pSvc,
                        &CRpcServices::RunManagedTask,
                        pRemove, false );

                    if( ERROR( ret ) )
                        ( *pRemove )( eventCancelTask );

                    ret = pMgr->RescheduleTaskByTid(
                        pManaged );
                }while( 0 );
                return ret;
            });

            TaskletPtr pCleanup;
            gint32 iRet = NEW_FUNCCALL_TASK(
                pCleanup, pMgr, func,
                ObjPtr( this ), htg,
                pCallback );

            if( SUCCEEDED( iRet ) )
            {
                iRet = AddSeqTaskIf( GetParent(),
                    pSeqTasks, pCleanup );
            }

        }while( 0 );

        return ret;
    }

    gint32 RemoveTg( Handle htg )
    {
        if( htg == 0 )
            return -EINVAL;
        gint32 ret = 0;
        do{
            CStdRMutex oLock( GetLock() );
            auto itr = m_mapSeqTgs.find( htg );
            if( itr == m_mapSeqTgs.end() )
                break;
            if( itr->second.m_iState != stateStopped )
            {
                ret = ERROR_STATE;
                break;
            }
            SEQTG_ELEM otg = itr->second;
            m_mapSeqTgs.erase( itr );
            auto& pGrp = otg.m_pSeqTasks;
            EnumTaskState iState = pGrp->GetTaskState();
            oLock.Unlock();
            if( !pGrp.IsEmpty() &&
                !pGrp->IsStopped( iState ) )
                ( *pGrp )( eventCancelTask );

        }while( 0 );

        return ret;
    }

    virtual gint32 GetStopTask(
        Handle htg, TaskletPtr& pStopTask ) = 0;

    typedef HTPAIR std::pair< Handle, TaskletPtr >;
    gint32 Stop( IEventSink* pCallback )
    {
        gint32 ret = 0;
        do{
            CParamList oParams;
            oParams.SetPointer(
                propIfPtr, GetParent() );
            TaskGrpPtr pGrp;
            ret = pGrp->NewObj(
                clsid( CIfParallelTaskGrp ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            CStdRMutex oLock( GetLock() );
            if( m_mapSeqTgs.empty() )
            {
                m_iMgrState = stateStopped;
                break;
            }
            if( m_iMgrState != stateStarted )
            {
                ret = ERROR_STATE;
                break;
            }
            m_iMgrState = stateStopping;
            std::vector< HTPAIR > vecTasks;
            for( auto& elem : m_mapSeqTgs )
            {
                if( elem.second.m_iState ==
                    stateStopped )
                    continue;
                TaskletPtr pStopTask;
                ret = GetStopTask(
                    elem.first, pStopTask );
                if( ERROR( ret ) )
                    continue;
                vecTasks.push_back(
                    { elem.first, pStopTask } );
            }
            oLock.Unlock();
            if( vecTasks.empty() )
            {
                ret = -ENOENT;
                break;
            }
            auto pMgr = GetParent()->GetIoMgr();

            gint32 (*func)( IEventSink* ) =
            ([]( IEventSink* pSvc )
            { return 0; } );
            
            std::vector< TaskletPtr > vecNotifies;
            for( auto& elem : vecTasks )
            {
                TaskletPtr pNotify;
                NEW_COMPLETE_FUNCALL( -1,
                    pNotify, pMgr,
                    func, GetParent() );
                pGrp->AppendTask( pNotify );
                vecNotifies.push_back( pNotify );
            }

            TaskletPtr pSync;
            CIfRetryTask* pRetry = pGrp;
            if( pCallback != nullptr )
            {
                gint32 (*func1)( IEventSink*, IEventSink*, ObjPtr& ) =
                ([]( IEventSink* pCb,
                    IEventSink* pUserCb,
                    ObjPtr& pseqmgr )
                {
                    CCfgOpenerObj oCbCfg( pCb );
                    IConfigDb* pResp;
                    CSeqTaskGrpMgr* pSeqMgr = pseqmgr;
                    if( pSeqMgr != nullptr )
                        pSeqMgr->SetState( stateStopped );

                    gint32 ret = pResp->GetPointer(
                        propRespPtr, pResp );
                    if( SUCCEEDED( ret ) )
                    {
                        CCfgOpener oResp( pResp );
                        gint32 iRet = oResp.GetIntProp(
                            propReturnValue, ret );
                        if( ERROR( iRet ) )
                            ret = iRet;
                    }
                    ret = pUserCb->OnEvent(
                        eventTaskComp, ret, 0, nullptr );
                });
                TaskletPtr pWrapper;
                NEW_COMPLETE_FUNCALL( 0, pWrapper,
                    func1, nullptr, pCallback,
                    ObjPtr( this ) );
                pRetry->SetClientNotify( pWrapper );
            }
            else
            {
                pSync.NewObj( clsid( CSyncCallback ) );
                pRetry->SetClientNotify( pSync );
            }

            GetParent()->RunManagedTask( pGrp );

            for( int i = 0; i < vecNotifies.size(); i++ )
            {
                AddStopTask( vecNotifies[ i ], 
                    vecTasks[ i ].first,
                    vecTasks[ i ].second );
            }

            if( !pSync.IsEmpty() )
            {
                pSync->WaitForCompleteWakable();
                ret = pSync->GetError();
                this->SetState( stateStopped );
            }
            else
            {
                ret = STATUS_PENDING;
            }

        }while( 0 );
        return ret;
    }
};
