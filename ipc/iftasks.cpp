/*
 * =====================================================================================
 *
 *       Filename:  iftasks.cpp
 *
 *    Description:  implementation of the tasks involved
 *                  in the interface operations
 *
 *        Version:  1.0
 *        Created:  02/12/2017 09:20:55 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#include <deque>
#include <vector>
#include <string>
#include <semaphore.h>
#include "defines.h"
#include "frmwrk.h"
#include "emaphelp.h"
#include "connhelp.h"
#include "proxy.h"
#include "ifhelper.h"

#define IF_GENERIC_RETIRES         3
#define IF_GENERIC_INTERVAL        60

using namespace std;

CIfStartRecvMsgTask::CIfStartRecvMsgTask(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    gint32 ret = 0;

    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    // parameter in the config is
    // propIfPtr: the pointer the interface object
    SetClassId( clsid( CIfStartRecvMsgTask ) );

    // retry after one second
    ret = oCfg.SetIntProp( propIntervalSec, 1 ); 
    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CIfStartRecvMsgTask ctro" );
        throw std::runtime_error( strMsg );
    }
}

template< typename T1, 
    typename T=typename std::enable_if<
        std::is_same<T1, CfgPtr>::value ||
        std::is_same<T1, DMsgPtr>::value, T1 >::type >
gint32 CIfStartRecvMsgTask::HandleIncomingMsg(
    ObjPtr& ifPtr, T1& pMsg )
{
    gint32 ret = 0;

    // servicing the incoming request/event
    // message
    do{
        CRpcServices* pIf = ifPtr;

        if( pIf == nullptr || pMsg.IsEmpty() )
            return -EFAULT;

        if( pIf->GetState() != stateConnected )
        {
            // NOTE: this is not a serious state
            // check
            ret = ERROR_STATE;
            break;
        }

        CParamList oParams;
        ret = oParams.SetObjPtr( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        BufPtr pBuf;
        pBuf.NewObj();
        *pBuf = pMsg;
        ret = oParams.SetProperty( propMsgPtr, pBuf );
        if( ERROR( ret ) )
            break;

        // copy the match ptr if this task is for
        // a proxy interface
        oParams.CopyProp( propMatchPtr, this );

        TaskletPtr pTask;

        ret = pTask.NewObj(
            clsid( CIfInvokeMethodTask ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        // put the InvokeMethod in a managed
        // environment instead of letting it run
        // randomly
        ret = pIf->AddAndRun( pTask );
        if( ERROR( ret ) )
            break;

        ret = pTask->GetError();
        if( ret == STATUS_PENDING )
        {
            // CIfStartRecvMsgTask is done, the
            // CIfInvokeMethodTask will take over
            ret = 0;
        }

    }while( 0 );

    return ret; 
}

gint32 CIfStartRecvMsgTask::OnIrpComplete(
    PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = pIrp->GetStatus(); 
    if( ERROR( ret ) )
    {
        if( ret != -EAGAIN )
        {
            // fatal error, just quit
            DebugPrint( ret,
                "Cannot continue to receive message" );
            return ret;
        }
        ret = 0;
    }

    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    ObjPtr pObj;
    
    ret = oCfg.GetObjPtr( propIfPtr, pObj );
    if( ERROR( ret ) )
        return ret;

    CRpcServices* pIf = pObj;
    if( pIf == nullptr )
        return -EFAULT;

    do{
        if( pIf->GetState() != stateConnected )
        {
            ret = ERROR_STATE;
            break;
        }

        CIoManager* pMgr = pIf->GetIoMgr();
        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        do{
            // continue receiving the message
            CParamList oParams;

            // clone all the parameters in this
            // task
            oParams[ propIfPtr ] = pObj;
            oParams.CopyProp( propMatchPtr, this );

            // start another recvmsg request
            TaskletPtr pTask;
            ret = pTask.NewObj(
                clsid( CIfStartRecvMsgTask ),
                oParams.GetCfg() );

            // add an concurrent task
            ret = pIf->AddAndRun( pTask );

            if( ERROR( ret ) )
                break;

            ret = pTask->GetError();

            // succeeded, let's repeat by spawning
            // new CIfStartRecvMsgTask for the
            // next pending message in the port
            if( SUCCEEDED( ret ) )
                continue;

            if( ERROR( ret ) )
            {
                DebugPrint( ret, "The new RecvMsgTask failed" );
            }
            // whether error or pending, we stop
            // further receiving
            break;

        }while( 1 );

        if( ret == ERROR_STATE )
        {
            // interface is not working, don't continue
            break;
        }

        // whether error or not, we don't care. we need
        // to handle the incoming irp now

        // proceed to complete the irp
        ret = pIrp->GetStatus();
        if( ret == -EAGAIN )
        {
            // no valid information to handle, just
            // quit
            ret = 0;
            break;
        }

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        if( !pCtx->IsNonDBusReq() )
        {
            DMsgPtr pMsg = *pCtx->m_pRespData;
            if( pMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            HandleIncomingMsg( pObj, pMsg );
        }
        else
        {
            CfgPtr pCfg;
            ret = pCtx->GetReqAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            HandleIncomingMsg( pObj, pCfg );
        }

    }while( 0 );

    // this task is retired
    return 0;
}

gint32 CIfStartRecvMsgTask::RunTask()
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ObjPtr pObj;
        
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;

        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        IMessageMatch* pMatch = nullptr;

        ret = oCfg.GetObjPtr(
            propMatchPtr, pObj );

        if( ERROR( ret ) )
            break;

        pMatch = pObj;
        ret = pIf->StartRecvMsg( this, pMatch );

    }while( 0 );

    return ret;
}

gint32 CIfStartRecvMsgTask::OnCancel(
    guint32 dwContext )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ObjPtr pObj;

        ret = oCfg.GetObjPtr( propIrpPtr, pObj );
        if( ERROR( ret ) )
            break;

        IrpPtr pIrp = pObj;

        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;
        if( unlikely( pIf == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        CIoManager* pMgr = pIf->GetIoMgr();
        pMgr->CancelIrp( pIrp );

    }while( 0 );

    return ret;
}

CIfRetryTask::CIfRetryTask(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    gint32 ret = 0;
    Sem_Init( &m_semWait, 0, 0 );
    m_pParentTask = nullptr;
    SetError( STATUS_PENDING );

    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CIoManager* pMgr = pIf->GetIoMgr();
        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = oCfg.SetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        if( !oCfg.exist( propRetries ) )
        {
            ret = oCfg.SetIntProp(
                propRetries, IF_GENERIC_RETIRES );
        }
        if( ERROR( ret ) )
            break;

        if( !oCfg.exist( propIntervalSec ) )
        {
            ret = oCfg.SetIntProp(
                propRetries, IF_GENERIC_INTERVAL );
        }
        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg(
            ret, "Error occurs in CIfRetryTask ctor" );
        SetError( ret );
        throw std::runtime_error( strMsg );
    }
}

gint32 CIfRetryTask::OnEvent(
    EnumEventId iEvent,
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData )
{ 
    gint32 iPropId = 0;
    switch( iEvent )
    {
    case eventTimeout:
        {
            // FIXME: this ease the issue of paramlist race
            // condition, but not fixed it
            iPropId = propTimerParamList;
            break;
        }
    case eventRpcNotify:
        {
            iPropId = propNotifyParamList;
            break; 
        }
    default:
        {
            iPropId = propParamList;
            break;
        }
    }

    IntVecPtr pVec( true );

    ( *pVec )().push_back( ( guint32 )iEvent );
    ( *pVec )().push_back( ( guint32 )dwParam1 );
    ( *pVec )().push_back( ( guint32 )dwParam2 );
    ( *pVec )().push_back( ( guint32 )pData);

    CStdRTMutex oTaskLock( GetLock() );
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    oCfg.SetObjPtr( iPropId, ObjPtr( pVec ) );
    oTaskLock.Unlock();

    ( *this )( iEvent );

    oTaskLock.Lock();
    oCfg.RemoveProperty( iPropId );

    return GetError();
}

gint32 CIfRetryTask::OnRetry()
{
    return RunTask();
}

gint32 CIfRetryTask::OnCancel(
    guint32 dwContext )
{
    super::OnCancel( dwContext );
    return 0;
}

gint32 CIfRetryTask::OnIrpComplete(
    IRP* pIrp )
{
    gint32 ret = 0;

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
    {
        ret = -EFAULT;
    }
    else
    {
        ret = pIrp->GetStatus();
        if( ret == -EAGAIN )
        {
            // schedule to retry
            ret = ERROR_FAIL;
            if( CanRetry() )
                ret = STATUS_MORE_PROCESS_NEEDED;
        }
    }

    return ret;
}

gint32 CIfRetryTask::RunTask() 
{
    return 0;
}

gint32 CIfRetryTask::OnComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;
    
    CCfgOpener oParams(
        ( IConfigDb* )GetConfig() );

    do{
        ObjPtr pObj;
        bool bNotify = false;

        ret = oParams.GetBoolProp(
            propNotifyClient, bNotify );

        if( ERROR( ret ) )
        {
            bNotify = false;
            ret = 0;
        }

        if( bNotify && IsPending() )
        {
            ret = oParams.GetObjPtr(
                propEventSink, pObj );

            if( SUCCEEDED( ret ) )
            {
                IEventSink* pEvent = pObj;
                if( pEvent == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }

                CObjBase* pObjBase = this;
                guint32* pData =
                    ( guint32* ) pObjBase;
                pEvent->OnEvent( eventTaskComp,
                    iRetVal, 0, pData );
                
            }
            else
            { 
                // a synchronous call, trigger a
                // semaphore
                Sem_Post( &m_semWait );
            }
        }

        if( m_pParentTask != nullptr )
        {
            CIfTaskGroup* pTaskGrp = static_cast
                < CIfTaskGroup* >( m_pParentTask );

            m_pParentTask = nullptr;
            if( pTaskGrp != nullptr )
            {
                pTaskGrp->OnChildComplete(
                    iRetVal, this );
                break;
            }
        }

    }while( 0 );

    // clear all the objPtr we used
    RemoveProperty( propIfPtr );
    RemoveProperty( propEventSink );
    RemoveProperty( propNotifyClient );
    RemoveProperty( propIrpPtr );

    // NOTE: Moved task status change to the
    // bottom due to a concurrent bug, when the
    // client is hoping the synchronization occur
    // in the OnEvent. But the whole thing happens
    // in a weird order that the async caller gets
    // the task status even after this OnComplete
    // is entered, but before the OnEvent is
    // called. ( Usually the async caller should
    // get a pending status very fast before the
    // server side enters OnComplete.)
    //
    // If the task status is set at the very
    // beginning of this method, the caller will
    // not wait for the sync signal, but proceed
    // to handle the call as an immediate return
    // instead, while the response is yet to set
    // in the upcoming OnEvent call. Thus the
    // client code will grab an empty response
    // sometimes, in CInterfaceProxy::SyncCallEx.
    //
    // This situation usually happens when the
    // client and server are both in a single
    // process
    // 
    SetError( iRetVal );
    return ret; 
}

gint32 CIfRetryTask::Process( guint32 dwContext )
{
    gint32 ret = 0;
    EnumEventId iEvent = ( EnumEventId )dwContext;

    do{
        switch( iEvent )
        {
        case eventZero:
            // the task is run manually
        case eventTaskThrdCtx:
            // the task is run from the task
            // thread
        case eventOneShotTaskThrdCtx:
            // the task is run from a one-shot
            // task thread
            {
                ret = RunTask();
                break;
            }
        case eventTaskComp:
            {
                vector< guint32 > vecParams;
                ret = GetParamList( vecParams );
                if( ERROR( ret ) )
                    break;

                guint32 dwContext = vecParams[ 1 ];
                ret = OnTaskComplete( dwContext );
                break;
            }
        case eventIrpComp:
            {   
                IrpPtr pIrp;

                ret = GetIrpFromParams( pIrp );
                if( ERROR( ret ) )
                    break;

                ret = OnIrpComplete( pIrp );

                break;
            }
        case eventTimeout:
            {
                vector< guint32 > vecParams;
                ret = GetParamList( vecParams,
                    propTimerParamList );

                if( ERROR( ret ) )
                    break;

                if( vecParams[ 1 ] == eventRetry )
                {
                    ret = OnRetry();
                }
                break;
            }
        case eventTimeoutCancel:
        case eventCancelTask:
            {
                OnCancel( iEvent );
                ret = -ECANCELED;
                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }

        if( ret == STATUS_PENDING )
        {
            break;
        }
        else if( ret == STATUS_MORE_PROCESS_NEEDED )
        {
            if( CanRetry() )
            {
                break;
            }
            ret = ERROR_FAIL;
        }
        else if( ret == -EAGAIN )
        {
            if( CanRetry() )
            {
                ret = STATUS_MORE_PROCESS_NEEDED;
                break;
            }
            ret = ERROR_FAIL;
        }

        OnComplete( ret );
        break;

    }while( 0 );

    if( ret == STATUS_PENDING ||
        ret == STATUS_MORE_PROCESS_NEEDED )
        MarkPending();

    return ret;
}

gint32 CIfRetryTask::WaitForComplete()
{
    // NOTE: please call this method from a thread
    // outside of the rpc subsystem
    gint32 ret = 0; 

    // warning: no locking here
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

    guint32 dwInterval = 0;

    ret = oCfg.GetIntProp(
        propIntervalSec, dwInterval );

    if( ERROR( ret ) )
        return ret;

    if( dwInterval > 100000 )
        return -EINVAL;

    do{
        ret = Sem_TimedwaitSec( &m_semWait, dwInterval );

        if( ret == -EAGAIN )
            continue;

        // whether succeeded or error, end the loop
        break;

    }while( 1 );

    return ret;
}

gint32 CIfRetryTask::GetProperty( gint32 iProp,
        CBuffer& oBuf ) const
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propParentTask:
        {
            if( m_pParentTask == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            oBuf = ObjPtr( m_pParentTask );
            break;
        }
    default:
        {
            ret = super::GetProperty(
                iProp, oBuf );
                break;
        }
    }

    return ret;
}
gint32 CIfRetryTask::SetProperty(
    gint32 iProp, const CBuffer& oBuf )
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propParentTask:
        {
            m_pParentTask = ( ObjPtr& )oBuf;
            break;
        }
    default:
        {
            ret = super::SetProperty(
                iProp, oBuf );
                break;
        }
    }

    return ret;
}
gint32 CIfRetryTask::RemoveProperty(
    gint32 iProp )
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propParentTask:
        {
            m_pParentTask = nullptr;
            break;
        }
    default:
        {
            ret = super::RemoveProperty( iProp );
                break;
        }
    }

    return ret;
}

gint32 CIfRetryTask::EnumProperties(
    std::vector< gint32 >& vecProps ) const
{
    gint32 ret = 0;
    ret = super::EnumProperties( vecProps );
    if( ERROR( ret ) )
        return ret;

    vecProps.push_back( propParentTask );

    return ret;
}

gint32 CIfRetryTask::GetPropertyType(
    gint32 iProp, gint32& iType ) const
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propParentTask:
        {
            iType = typeObj;
            break;
        }
    default:
        {
            ret = super::GetPropertyType(
                iProp, iType );
                break;
        }
    }

    return ret;
}

CIfEnableEventTask::CIfEnableEventTask(
    const IConfigDb* pCfg ) :
        super( pCfg )
{
    gint32 ret = 0;
    SetClassId( clsid( CIfEnableEventTask ) );
    CParamList oParams( ( IConfigDb* )GetConfig() );

    do{
        ret = oParams.SetIntProp(
            propRetries, IF_GENERIC_RETIRES ); 

        if( ERROR( ret ) )
            break;

        ret = oParams.SetIntProp(
            propIntervalSec, IF_GENERIC_INTERVAL ); 

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CIfEnableEventTask ctro" );

        throw std::runtime_error( strMsg );
    }
}

gint32 CIfEnableEventTask::OnIrpComplete( IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        ret = super::OnIrpComplete( pIrp ); 

        if( SUCCEEDED( ret ) )
        {
            ObjPtr pObj;
            CParamList oParams( ( IConfigDb* )GetConfig() );
            
            ret = oParams.GetObjPtr( propIfPtr, pObj );
            if( ret != -ENOTCONN && ERROR( ret ) )
                break;

            CRpcInterfaceBase* pIf = pObj;
            if( pIf == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            bool bChgStat = true;
            oParams.GetBoolProp( 1, bChgStat );

            if( !bChgStat )
                break;

            IrpCtxPtr pCtx = pIrp->GetTopStack();
            guint32 dwCtrlCode = pCtx->GetCtrlCode();;
            if( ret == -ENOTCONN )
            {
                ret = pIf->SetStateOnEvent(
                    eventModOffline );
            }
            else
            {
                if( dwCtrlCode == CTRLCODE_REG_MATCH )
                {
                    ret = pIf->SetStateOnEvent(
                        cmdEnableEvent );
                }
                else
                {
                    ret = -ENOTSUP;
                }
            }
        }
        else
        {
            if( ret == -EAGAIN )
            {
                // schedule to retry
                ret = ERROR_FAIL;
                if( CanRetry() )
                    ret = STATUS_MORE_PROCESS_NEEDED;
            }
            // remain in the stateStarted
        }

    }while( 0 );

    return ret;
}

gint32 CIfEnableEventTask::RunTask()
{
    gint32 ret = 0;
    
    do{
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        ret = oParams.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        bool bEnable = true;

        // note: we do not use pop, because we want to
        // retry when failed
        ret = oParams.GetBoolProp( 0, bEnable );
        if( ERROR( ret ) )
            break;

        bool bChgStat = true;
        oParams.GetBoolProp( 1, bChgStat );

        IMessageMatch* pMatch = nullptr;

        ret = oParams.GetObjPtr(
            propMatchPtr, pObj );

        if( SUCCEEDED( ret ) )
        {
            pMatch = pObj;
            if( pMatch == nullptr )
            {
                ret = -EINVAL;
                break;
            }
        }
    
        if( bEnable )
        {
            ret = pIf->EnableEvent(
                pMatch, this );
        }
        else
        {
            ret = pIf->DisableEvent(
                pMatch, this );
        }

        if( ret == STATUS_PENDING )
            break;

        if( ret == -EAGAIN )
        {
            ret = ERROR_FAIL;
            if( CanRetry() )
                ret = STATUS_MORE_PROCESS_NEEDED;
            break;
        }
        else if( ret == -ENOTCONN )
        {
            // let the interface enter the recovery
            // mode, and waiting for the server online
            // event
            ret = pIf->SetStateOnEvent(
                eventModOffline );
        }
        else if( SUCCEEDED( ret ) )
        {
            // finished immediately
            if( !bChgStat )
                break;

            if( bEnable )
            {
                ret = pIf->SetStateOnEvent(
                    cmdEnableEvent );
            }
        }
        else if( ERROR( ret ) )
        {
            // fatal error
            break;
        }

    }while( 0 );

    return ret;
}

CIfOpenPortTask::CIfOpenPortTask(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CIfOpenPortTask ) );
}

gint32 CIfOpenPortTask::OnIrpComplete(
    PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        ObjPtr pObj;
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );        

        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        HANDLE hPort = 0;
        EnumEventId iEvent = eventPortStarted;

        if( ERROR( pIrp->GetStatus() ) )
        {
            iEvent = eventPortStartFailed;
        }
        else
        {
            IrpCtxPtr& pCtx = pIrp->GetTopStack();
            hPort = ( guint32& )*pCtx->m_pRespData;
        }
        
        ret = pIf->OnPortEvent( iEvent, hPort );

    }while( 0 );
    
    return ret;
}

gint32 CIfOpenPortTask::RunTask()
{
    gint32 ret = 0;
    
    do{
        ObjPtr pObj;
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        ret = oParams.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIf->OpenPort( this );
        if( ret == -EAGAIN )
        {
            // possibly the port is starting by someone
            // else
            if( CanRetry() )
            {
                ret = STATUS_MORE_PROCESS_NEEDED;
                break;
            }
            // otherwise, just quit with error.
        }

    }while( 0 );

    return ret;
}

void CIfTaskGroup::PopTask()
{
    if( !m_queTasks.empty() )
        m_queTasks.pop_front();
    else
    {
        DebugPrint( -ERANGE, "no more emelemt" );
    }
}

bool CIfTaskGroup::IsCanceling() const
{
    if( GetConfig()->exist( propCanceling ) )
        return true;

    return false;
}

void CIfTaskGroup::SetCanceling( bool bCancel )
{
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    if( bCancel )
    {
        oCfg[ propCanceling ] = true;
        oCfg[ propCancelTid ] = GetTid();
    }
    else
    {
        oCfg.RemoveProperty( propCanceling );
        oCfg.RemoveProperty( propCancelTid );
    }
}

gint32 CIfTaskGroup::AppendTask(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    CStdRTMutex oTaskLock( GetLock() );

    CCfgOpenerObj oCfg( ( CObjBase* )pTask );

    gint32 ret = oCfg.SetObjPtr(
        propParentTask, ObjPtr( this ) );

    if( ERROR( ret ) )
        return ret;

    if( IsCanceling() )
        return ERROR_STATE;

    m_queTasks.push_back( pTask );

    return 0;
}

gint32 CIfTaskGroup::AppendAndRun(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    bool bImmediate = false;

    do{
        CStdRTMutex oTaskLock( GetLock() );

        ret = AppendTask( pTask );
        if( ERROR( ret ) )
            break;

        if( GetTaskCount() == 1 )
        {
            bImmediate = true;
            m_vecRetVals.clear();
        }

    }while( 0 );

    if( SUCCEEDED( ret ) &&
        bImmediate == true )
    {
        ret = ( *this )( eventZero );
    }
    else if( SUCCEEDED( ret ) )
    {
        // there is some other tasks running
        ret = STATUS_PENDING;
    }
    
    return ret;
}

gint32 CIfTaskGroup::CancelRemainingTasks()
{
    gint32 ret = 0;

    CStdRTMutex oTaskLock( GetLock() );
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

    oCfg[ propContext ] = eventCancelTask;
    oCfg[ propNoResched ] = true;

    do{
        TaskletPtr pTask;
        if( m_queTasks.empty() )
            break;

        pTask = m_queTasks.front();

        // release the lock to avoid nested chain of
        // locks along the taskgroup tree
        oTaskLock.Unlock();

        ( *pTask )( eventCancelTask );

        oTaskLock.Lock();

    }while( 1 );

    oCfg.RemoveProperty( propNoResched );
    oCfg.RemoveProperty( propContext );

    return ret;
}

gint32 CIfTaskGroup::RunTask()
{
    gint32 ret = 0;
    guint32 dwContext = eventZero;

    CStdRTMutex oTaskLock( GetLock() );

    if( IsCanceling() )
    {
        // we are coming here probably because the
        // ::Process is prempted by the canceling
        // call
        return STATUS_PENDING;
    }

    if( !m_vecRetVals.empty() )
    {
        // last task's return value
        gint32 iRet = m_vecRetVals.back();

        if( ERROR( iRet ) &&
            GetRelation() == logicAND )
        {

            oTaskLock.Unlock();
            CancelRemainingTasks();
            return iRet;
        }

        if( SUCCEEDED( iRet ) &&
            GetRelation() == logicOR )
        {
            m_queTasks.clear();
            return 0;
        }
    }

    while( !m_queTasks.empty() )
    {
        TaskletPtr pTask;

        if( IsCanceling() )
        {
            // we are coming here because the Process()
            // is prempted by the canceling call
            ret = STATUS_PENDING;
            break;
        }

        pTask = m_queTasks.front();
        if( pTask.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

        // tell this->OnChildComplete not to reschedule
        // this taskgrp because it is running
        oCfg.SetBoolProp( propNoResched, true );

        // release the lock to avoid nested chain of
        // locking
        oTaskLock.Unlock();

        ( *pTask )( dwContext );
        ret = pTask->GetError();

        oTaskLock.Lock();
        oCfg.RemoveProperty( propNoResched );

        if( ret == STATUS_PENDING )
            break;

        if( ret == STATUS_MORE_PROCESS_NEEDED )
        {
            // retry will happen on the child task,
            // let's pending
            // Taskgroup will not return
            // STATUS_MORE_PROCESS_NEEDED
            ret = STATUS_PENDING;
            break;
        }

        if( ERROR( ret ) &&
            GetRelation() == logicAND )
        {
            // no need to go further if AND
            // relationship between tasks
            //
            // all the remaining tasks in this
            // task group will be canceled
            if( m_queTasks.size() > 0 )
            {
                oTaskLock.Unlock();
                CancelRemainingTasks();
            }
            break;
        }

        if( SUCCEEDED( ret ) &&
            GetRelation() == logicOR )
        {
            m_queTasks.clear();
            break;
        }
    }

    return ret;
}

gint32 CIfTaskGroup::OnChildComplete(
    gint32 iRet, CTasklet* pChild )
{
    gint32 ret = 0;
    if( pChild == nullptr )
        return -EINVAL;

    do{
        bool bSync = false;

        CStdRTMutex oTaskLock( GetLock() );

        CfgPtr pCfg = GetConfig();
        CCfgOpener oCfg(
            ( IConfigDb* )pCfg );

        if( IsCanceling() )
        {
            guint32 dwTid = oCfg[ propCancelTid ];
            if( dwTid != GetTid() )
            {
                // an active canceling is going on from
                // anther thread while an async task
                // completion happens
                ret = ERROR_STATE;
                break;
            }
            else
            {
                // we are within the canceling process
                // just move on
            }
        }

        TaskletPtr pHead = m_queTasks.front();
        if( pHead->GetObjId() != pChild->GetObjId() )
        {
            // Possibly racing in this function between
            // a canceling thread and an async task
            // completion.
            return -ENOENT;
        }

        if( pCfg->exist( propNoResched ) )
            bSync = true;

        PopTask();
        m_vecRetVals.push_back( iRet );

        if( bSync )
            break;

        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        oTaskLock.Unlock();

        CIoManager* pMgr = pIf->GetIoMgr();

        // regain control by rescheduling this task
        TaskletPtr pThisTask( this );
        ret = pMgr->RescheduleTask( pThisTask );

    }while( 0 );

    return ret;
}

gint32 CIfTaskGroup::Process(
    guint32 dwContext )
{
    gint32 ret = 0;

    {
        CStdRTMutex oTaskLock( GetLock() );
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        guint32 dwCtx = 0;

        ret = oCfg.GetIntProp(
            propContext, dwCtx );

        // override the original dwContext if there is
        // a propContext property in the config
        if( SUCCEEDED( ret ) )
            dwContext = dwCtx;
    }

    ret = super::Process( dwContext );

    return ret;
}

// wait to cancel the running child task
gint32 CIfTaskGroup::WaitingToCancel(
    TaskletPtr& pTask )
{
    gint32 ret = 0;
    if( pTask.IsEmpty() )
        return -EINVAL;

    do{
        // FIXME: we are stucking here
        gint32 iRetry = 3;
        while( pTask->IsInProcess() && iRetry > 0 )
        {
            // wait for 3 seconds before canceling
            sleep( 1 );
            iRetry--;
        }

        if( pTask->GetError() ==
            STATUS_PENDING )
        {
            ( *pTask )( eventCancelTask );
        }

    }while( 0 );

    return ret;
}

gint32 CIfTaskGroup::OnCancel(
    guint32 dwContext )
{
    do{
        CStdRTMutex oTaskLock( GetLock() );
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        if( IsCanceling() )
        {
            // reentrancy, possibly a reschedule
            // happens while canceling starts
            return STATUS_PENDING;
        }

        if( GetConfig()->exist( propNoResched ) )
        {
            // RunTask is active
            oTaskLock.Unlock();    
            usleep( 100000 );
            continue;
        }

        if( m_queTasks.empty() )
            break;

        SetCanceling( true );

        // NOTE: by setting propContext to
        // eventCancelTask, RunTask will not be entered
        // again
        oCfg[ propContext ] = eventCancelTask;
        oCfg[ propNoResched ] = true;

        TaskletPtr pHead;
        if( m_queTasks.front()->IsInProcess() )
        {
            pHead = m_queTasks.front();
            m_queTasks.pop_front();
        }

        do{
            TaskletPtr pTask;
            if( m_queTasks.empty() )
                break;

            pTask = m_queTasks.front();

            // release the lock to avoid nested chain of
            // locks along the taskgroup tree
            oTaskLock.Unlock();

            ( *pTask )( eventCancelTask );

            oTaskLock.Lock();

        }while( 1 );

        if( !pHead.IsEmpty() )
        {
            m_queTasks.push_back( pHead );
            oTaskLock.Unlock();

            WaitingToCancel( pHead );

            oTaskLock.Lock();
            if( m_queTasks.back() == pHead )
                m_queTasks.pop_back();
        }

        oCfg.RemoveProperty( propNoResched );
        oCfg.RemoveProperty( propContext );

        SetCanceling( false );
        break;

    }while( 1 );

    return -ECANCELED;
}

bool CIfTaskGroup::exist( TaskletPtr& pTask )
{
    CStdRTMutex oTaskLock( GetLock() );

    deque< TaskletPtr >::iterator itr;
    itr = std::find( m_queTasks.begin(),
        m_queTasks.end(), pTask );

    if( itr == m_queTasks.end() )
        return false;

    return true;
}

gint32 CIfTaskGroup::RemoveTask(
    TaskletPtr& pTask )
{
    CStdRTMutex oTaskLock( GetLock() );

    deque< TaskletPtr >::iterator itr;
    itr = std::find( m_queTasks.begin(),
        m_queTasks.end(), pTask );

    if( itr != m_queTasks.end() )
    {
        m_queTasks.erase( itr );
        return 0;
    }
    return -ENOENT;
}

gint32 CIfTaskGroup::FindTaskByRmtId(
    guint64 iTaskId, TaskletPtr& pRet )
{
    gint32 ret = -ENOENT;

    CStdRTMutex oTaskLock( GetLock() );
    for( auto&& pTask : m_queTasks )
    {
        const IConfigDb* pTaskCfg =
            ( IConfigDb* )*pTask;

        if( pTaskCfg == nullptr )
            continue;

        if( pTask->GetClsid() ==
            clsid( CIfInvokeMethodTask ) )
        {
            CCfgOpener oCfg( pTaskCfg );
            guint64 qwRmtId = 0;

            ret = oCfg.GetQwordProp(
                propRmtTaskId, qwRmtId );

            if( ERROR( ret ) )
                continue;

            if( qwRmtId == iTaskId )
            {
                pRet = pTask;
                break;
            }
            continue;
        }
        
        CCfgOpener oCfg( pTaskCfg );
        CIfTaskGroup* pGrp = pTask;
        if( pGrp != nullptr )
        {
            ret = pGrp->FindTaskByRmtId(
                iTaskId, pRet );

            if( SUCCEEDED( ret ) )
                break;
        }
    }
    return ret;
}

gint32 CIfTaskGroup::FindTask(
    guint64 iTaskId, TaskletPtr& pRet )
{
    gint32 ret = -ENOENT;

    CStdRTMutex oTaskLock( GetLock() );
    for( auto&& pTask : m_queTasks )
    {
        if( pTask->GetObjId() == iTaskId )
        {
            pRet = pTask;
            ret = 0;
            break;
        }

        CIfTaskGroup* pGrp = pTask;
        if( pGrp != nullptr )
        {
            ret = pGrp->FindTask( iTaskId, pRet );
            if( SUCCEEDED( ret ) )
                break;
        }
    }
    return ret;
}

gint32 CIfRootTaskGroup::OnComplete(
    gint32 iRetVal )
{
    return 0;
}

gint32 CIfRootTaskGroup::GetHeadTask(
    TaskletPtr& pHead )
{
    CStdRTMutex oTaskLock( GetLock() );
    if( m_queTasks.empty() )
        return -ENOENT;
    pHead = m_queTasks.front();
    return 0;
}

gint32 CIfRootTaskGroup::GetTailTask(
    TaskletPtr& pTail )
{
    CStdRTMutex oTaskLock( GetLock() );
    if( m_queTasks.empty() )
        return -ENOENT;

    pTail = m_queTasks.back();
    return 0;
}

gint32 CIfStopTask::RunTask()
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;

        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;
        if( unlikely( pIf == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        if( pIf->GetState() == stateStopped )
        {
            ret = ERROR_STATE;
            break;
        }

#ifdef DEBUG
        guint32 dwServer = 0;
        if( pIf->IsServer() )
            dwServer = 1;

        DebugPrint( dwServer,
            "About to close port, %d", GetTid() );

#endif
        ret = pIf->ClosePort();

    }while( 0 );

    return ret;
}

gint32 CIfStopRecvMsgTask::RunTask()
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ObjPtr pObj;
        
        ret = oCfg.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;

        CIoManager* pMgr = pIf->GetIoMgr();
        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pIf->CancelRecvMsg();

    }while( 0 );

    return ret;
}

gint32 CIfPauseResumeTask::RunTask()
{
    gint32 ret = 0;
    do{
        CParamList oParams( ( IConfigDb* )GetConfig() );
        ObjPtr pObj;
        
        ret = oParams.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;

        CIoManager* pMgr = pIf->GetIoMgr();
        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        IEventSink* pCallback = nullptr;

        ret = oParams.GetObjPtr(
            propEventSink, pObj );

        if( SUCCEEDED( ret ) )
            pCallback = pObj;


        bool bResume = false;

        ret = oParams.GetBoolProp( 0, bResume );
        if( ERROR( ret ) )
            break;

        if( bResume )
        {
            ret = pIf->DoResume( pCallback );
        }
        else
        {
            ret = pIf->DoPause( pCallback );
        }

    }while( 0 );

    return ret;
}

CIfCpEventTask::CIfCpEventTask(
    const IConfigDb* pCfg )
    : super( pCfg )
{

}

gint32 CIfCpEventTask::OnIrpComplete(
    PIRP pIrp )
{
    if( pIrp == nullptr )
        return -EINVAL;

    return 0;
}

gint32 CIfCpEventTask::RunTask()
{

    gint32 ret = 0;
    do{

        CParamList oParams(
           ( IConfigDb* )GetConfig() ); 

        EnumEventId iEvent;

        guint32 iVal = 0;
        ret = oParams.GetIntProp( 0, iVal );

        if( ERROR( ret ) )
            break;
        iEvent = ( EnumEventId )iVal;

        ObjPtr pObj;
        ret = oParams.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcBaseOperations* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        switch( iEvent )
        {
        case eventModOffline:
        case eventModOnline:
            {
                string strModName;
                ret = oParams.GetStrProp(
                    1, strModName );

                if( ERROR( ret ) )
                    break;

                ret = pIf->DoModEvent(
                    iEvent, strModName );
                break;

            }
        case eventRmtModOffline:
        case eventRmtModOnline:
            {
                string strModName;
                ret = oParams.GetStrProp(
                    1, strModName );

                if( ERROR( ret ) )
                    break;

                // FIXME: we don't need to check
                // ip addr at this point
                CCfgOpenerObj oCfg( pIf );
                string strIpAddr;
                ret = oCfg.GetStrProp(
                    propIpAddr, strIpAddr );

                if( ERROR( ret ) )
                    break;

                ret = pIf->DoRmtModEvent(
                    iEvent, strModName, strIpAddr );
                break;
            }
        default:
            ret = -ENOTSUP;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CIfParallelTaskGrp::RemoveTask(
    TaskletPtr& pTask )
{
    gint32 ret = 0;

    ret = m_setTasks.erase( pTask );
    if( ret == 0 )
        ret = m_setPendingTasks.erase( pTask );

    if( ret == 0 )
        return -ENOENT;

    return 0;
}

gint32 CIfParallelTaskGrp::OnChildComplete(
    gint32 iRet, CTasklet* pChild )
{
    gint32 ret = 0;

    if( pChild == nullptr )
        return -EINVAL;

    do{
        CStdRTMutex oTaskLock( GetLock() );

        CfgPtr pCfg = GetConfig();
        CCfgOpener oCfg( ( IConfigDb* )pCfg );

        if( IsCanceling() )
        {
            guint32 dwTid = oCfg[ propCancelTid ];
            if( dwTid != GetTid() )
            {
                // an active canceling is going on from
                // anther thread
                ret = ERROR_STATE;
                break;
            }
            else
            {
                // we are within the canceling process
                // just move on
            }
        }

        TaskletPtr taskPtr = pChild;
        RemoveTask( taskPtr );

        if( pCfg->exist( propNoResched ) )
        {
            ret = 0;
            break;
        }

        if( GetPendingCount() == 0 &&
            GetTaskCount() > 0 )
        {
            // NOTE: this check will effectively reduce
            // the possibility RunTask to run on two
            // thread at the same time, and lower the
            // probability get blocked by reentrance lock
            // no need to reschedule
            ret = 0;
            break;
        }

        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CIoManager* pMgr = pIf->GetIoMgr();

        // regain control by rescheduling this
        // task
        TaskletPtr pThisTask( this );
        ret = pMgr->RescheduleTask( pThisTask );

    }while( 0 );

    return ret;
}

gint32 CIfParallelTaskGrp::RunTask()
{
    gint32 ret = 0;
    CStdRTMutex oTaskLock( GetLock() );
    do{
        std::set< TaskletPtr > setTasksToRun;
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

        oCfg[ propNoResched ] = true;

        if( !oCfg.exist( propRunning ) )
            oCfg[ propRunning ] = true;

        setTasksToRun = m_setPendingTasks;

        m_setTasks.insert(
            m_setPendingTasks.begin(),
            m_setPendingTasks.end() );

        m_setPendingTasks.clear();

        oTaskLock.Unlock();

        while( !setTasksToRun.empty() )
        {
            CTasklet* pTask =
                *setTasksToRun.begin(); 

            if( pTask != nullptr )
                ( *pTask )( eventZero );

            setTasksToRun.erase(
                setTasksToRun.begin() );
        }

        oTaskLock.Lock();
        ret = oCfg.RemoveProperty( propNoResched );

        // new tasks come in
        if( GetPendingCount() > 0 )
            continue;

        // pending means there are tasks running, does
        // not indicate a specific task's return value
        if( GetTaskCount() > 0 )
            ret = STATUS_PENDING;

        break;

    }while( 1 );

    // we will be removed from the root task
    // group
    return ret;
}

gint32 CIfParallelTaskGrp::AddAndRun(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CStdRTMutex oTaskLock( GetLock() );
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

        ObjPtr pIf;
        ret = oCfg.GetObjPtr( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pService = pIf;
        if( pService == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CIoManager* pMgr = pService->GetIoMgr();

        bool bRunning = IsRunning();

        CCfgOpenerObj oChildCfg(
            ( CObjBase* )pTask );

        oChildCfg.SetObjPtr(
            propParentTask, ObjPtr( this ) );

        m_setPendingTasks.insert( pTask );
        oTaskLock.Unlock();

        if( bRunning )
        {
            // reschedule this task immediately
            ret = ( *this )( eventZero );

            if( ret == -EDEADLK )
            {
                TaskletPtr pThisGrp( this );
                pMgr->RescheduleTask( pThisGrp );
                ret = STATUS_PENDING;
            }
            break;
        }
        else
        {
            ret = STATUS_PENDING;
        }

    }while( 0 );

    return 0;
}

bool CIfParallelTaskGrp::IsRunning()
{
    CStdRTMutex oTaskLock( GetLock() );
    return GetConfig()->exist( propRunning );
}

gint32 CIfParallelTaskGrp::AppendTask(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CStdRTMutex oTaskLock( GetLock() );
        if( IsCanceling() )
        {
            ret = ERROR_STATE;
            break;
        }
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

        CCfgOpenerObj oChildCfg(
            ( CObjBase* )pTask );

        oChildCfg.SetObjPtr(
            propParentTask, ObjPtr( this ) );

        m_setPendingTasks.insert( pTask );

    }while( 0 );

    return ret;
}

gint32 CIfParallelTaskGrp::FindTask(
    guint64 iTaskId, TaskletPtr& pRet )
{
    gint32 ret = -ENOENT;

    CStdRTMutex oTaskLock( GetLock() );

    for( auto&& pTask : m_setTasks )
    {
        if( pTask->GetObjId() == iTaskId )
        {
            pRet = pTask;
            ret = 0;
            break;
        }

        CIfTaskGroup* pGrp = pTask;
        if( pGrp != nullptr )
        {
            ret = pGrp->FindTask( iTaskId, pRet );
            if( SUCCEEDED( ret ) )
                break;
        }
    }

    for( auto&& pTask : m_setPendingTasks )
    {
        if( pTask->GetObjId() == iTaskId )
        {
            pRet = pTask;
            ret = 0;
            break;
        }

        CIfTaskGroup* pGrp = pTask;
        if( pGrp != nullptr )
        {
            ret = pGrp->FindTask( iTaskId, pRet );
            if( SUCCEEDED( ret ) )
                break;
        }
    }
    return ret;
}

gint32 CIfParallelTaskGrp::FindTaskByRmtId(
    guint64 iTaskId, TaskletPtr& pRet )
{
    gint32 ret = 0;

    gint32 i = 0;
    bool bFound = false;

    CStdRTMutex oTaskLock( GetLock() );
    std::set< TaskletPtr >* psetTasks = &m_setTasks;
    do{
        for( auto&& pTask : *psetTasks )
        {
            const IConfigDb* pTaskCfg =
                ( IConfigDb* )*pTask;

            if( pTaskCfg == nullptr )
                continue;

            if( pTask->GetClsid() ==
                clsid( CIfInvokeMethodTask ) )
            {
                CCfgOpener oCfg( pTaskCfg );
                guint64 qwRmtId = 0;

                ret = oCfg.GetQwordProp(
                    propRmtTaskId, qwRmtId );

                if( ERROR( ret ) )
                    continue;

                if( qwRmtId == iTaskId )
                {
                    pRet = pTask;
                    bFound = true;
                    break;
                }
                continue;
            }
            
            CCfgOpener oCfg( pTaskCfg );
            CIfTaskGroup* pGrp = pTask;
            if( pGrp != nullptr )
            {
                ret = pGrp->FindTaskByRmtId(
                    iTaskId, pRet );

                if( SUCCEEDED( ret ) )
                {
                    bFound = true;
                    break;
                }
            }
        }

        if( bFound )
            break;
        ++i;
        psetTasks = &m_setPendingTasks;

    }while( i < 2 );

    return ret;
}

gint32 CIfParallelTaskGrp::OnCancel(
    guint32 dwContext )
{
    gint32 ret = 0;
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    dwContext = eventCancelTask;

    CStdRTMutex oTaskLock( GetLock() );
    oCfg[ propNoResched ] = true;
    SetCanceling( true );

    do{
        std::vector< TaskletPtr > vecTasks;

        // when canceling, no re-schedule of this
        // task on child task completion
        if( m_setPendingTasks.size() > 0 )
        {
            vecTasks.insert(
                vecTasks.end(),
                m_setPendingTasks.begin(),
                m_setPendingTasks.end() );

            m_setPendingTasks.clear();
        }

        if( m_setTasks.size() > 0 )
        {
            vecTasks.insert(
                vecTasks.end(),
                m_setTasks.begin(),
                m_setTasks.end() );

            m_setTasks.clear();
        }

        oTaskLock.Unlock();

        for( auto elem : vecTasks )
        {
            TaskletPtr& pTask = elem;
            CIfParallelTask* pParaTask = pTask;

            if( pParaTask != nullptr )
            {
                // for parallel task
                ( *pTask )( eventTryLock | dwContext );
            }
            else
            {
                ( *pTask )( dwContext );
            }
        }

        // call the super class to cleanup
        // super::OnCancel( dwContext );
        oTaskLock.Lock();

    }while( 0 );
    RemoveProperty( propNoResched );
    SetCanceling( false );

    return ret;
}

gint32 CIfCleanupTask::OnIrpComplete(
    PIRP pIrp )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ObjPtr pObj;
        
        ret = oCfg.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcServices* pIf = pObj;

        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        oCfg.GetObjPtr( propEventSink, pObj );
        IEventSink* pEvent = pObj;

        // call the user's callback
        if( pEvent != nullptr && IsPending() )
        {
            pEvent->OnEvent(
                eventTaskComp, ret, 0, nullptr );
        }
        ret = RunTask();

    }while( 0 );

    return ret;
}

gint32 CIfCleanupTask::RunTask()
{
    
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ObjPtr pObj;
        
        ret = oCfg.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcServices* pIf = pObj;

        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIf->OnPostStop( this );

    }while( 0 );

    return ret;
}

gint32 CIfCleanupTask::OnTaskComplete(
    gint32 iRetVal )
{
    return RunTask();
}

CIfParallelTask::CIfParallelTask(
    const IConfigDb* pCfg ) :
    super( pCfg ),
    m_iTaskState( stateStarting )
{

    // parameter in the config is
    // propIfPtr: the pointer the interface object
    gint32 ret = 0;
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

    // retry after one second
    ret = oCfg.SetIntProp( propIntervalSec, 3 ); 
    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CIfParallelTask ctor" );
        throw std::runtime_error( strMsg );
    }
}


/**
* @name CIfParallelTask::operator()
* the main entery for a CIfParallelTask task. if
* you returns from RunTask with the
* STATUS_PENDING, you should be aware that the
* OnCancel could be called sometime when the
* timeout, or sys admin event can come to cancel
* the task. if RunTask is a synchronous call, you
* don't bother, because the oncancel has not
* chance to execute before RunTask returns
* @{ */
/**  @} */

gint32 CIfParallelTask::operator()(
    guint32 dwContext )
{
    // Lock to prevent re-entrance 
    guint32 dwTryMs = 0;
    gint32 ret = 0;

    if( dwContext & eventTryLock )
    {
        // wait 1 second before lock failed
        dwTryMs = 1000;
        dwContext &= ~eventTryLock;
    }

    try{
        CStdRTMutex oTaskLock( GetLock(), dwTryMs );

        if( m_iTaskState == stateStopped )
            return ERROR_STATE;

        if( m_iTaskState == stateStarting )
        {
            switch( dwContext )
            {
            case eventZero:
            case eventTaskThrdCtx:
            case eventOneShotTaskThrdCtx:
            case eventIrpComp:
            case eventTimeout:
                {
                    m_iTaskState = stateStarted;
                    break;
                }
            case eventTaskComp:
            case eventCancelTask:
            default:
                {
                    break;
                }
            }
        }
        ret = super::operator()( dwContext );
    }
    catch( std::runtime_error& e )
    {
        // lock failed
        ret = -EACCES;
    }
    return ret;
}

gint32 CIfParallelTask::OnComplete(
    gint32 iRet )
{
    m_iTaskState = stateStopped;
    return super::OnComplete( iRet );
}

gint32 CIfParallelTask::OnCancel(
    guint32 dwContext )
{
    if( m_iTaskState == stateStopped )
        return 0;

    CancelIrp();
    super::OnCancel( dwContext );
    return 0;
}

gint32 CIfParallelTask::CancelIrp()
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIrpPtr, pObj );
        if( SUCCEEDED( ret ) )
        {
            RemoveProperty( propIrpPtr );

            IrpPtr pIrp = pObj;
            if( pIrp.IsEmpty()
                || pIrp->GetStackSize() == 0 )
            {
                ret = -EFAULT;
                break;
            }

            CIoManager* pMgr = nullptr;
            ret = oCfg.GetPointer( propIoMgr, pMgr );
            if( ERROR( ret ) )
                break;

            ret = pMgr->CancelIrp( pIrp );
        }

    }while( 0 );

    return ret;
}


gint32 CIfParallelTask::SetReqCall(
    IConfigDb* pReq )
{
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    return oCfg.SetObjPtr(
        propReqPtr, ObjPtr( pReq ) );
}

gint32 CIfParallelTask::SetRespData(
    IConfigDb* pResp )
{
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    CfgPtr pCfg;
    if( pResp == nullptr )
    {
        pCfg.NewObj();
    }
    else
    {
        pCfg = pResp;
    }
    return oCfg.SetObjPtr( propRespPtr,
        ObjPtr( ( IConfigDb* )pCfg ) );
}

gint32 CIfParallelTask::GetReqCall(
    CfgPtr& pReq )
{
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

    ObjPtr pObj;
    gint32 ret = oCfg.GetObjPtr(
        propReqPtr, pObj );

    if( ERROR( ret ) )
        return ret;

    pReq = pObj;
    if( pReq.IsEmpty() )
        return -EFAULT;

    return 0;
}

gint32 CIfParallelTask::GetRespData(
    CfgPtr& pResp )
{
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    ObjPtr pObj;
    gint32 ret = oCfg.GetObjPtr(
        propRespPtr, pObj );

    if( ERROR( ret ) )
        return ret;

    pResp = pObj;
    if( pResp.IsEmpty() )
        return -EFAULT;

    return 0;
}

gint32 CIfParallelTask::Process(
    guint32 dwContext )
{
    gint32 ret = 0;
    switch( dwContext )
    {
    case eventKeepAlive:
        {
            // handle eventKeepAlive from a task
            // call
            ret = OnKeepAlive( eventKeepAlive );
            break;
        }
    case eventTimeout:
        {
            // handle eventKeepAlive on timeout
            // timer expired
            vector< guint32 > vecParams;

            ret = GetParamList( vecParams,
                propTimerParamList );

            if( ERROR( ret ) )
                break;

            if( vecParams.size() < 2 )
            {
                ret = -EINVAL;
                break;
            }

            if( vecParams[ 1 ] == eventKeepAlive )
            {
                ret = OnKeepAlive( eventTimeout );
                break;
            }
            else if( vecParams[ 1 ] == eventTimeoutCancel ||
                vecParams[ 1 ] == eventCancelTask )
            {
                // redirect to CIfRetryTask::Process
                dwContext = vecParams[ 1 ];
            }
            ret = -ENOTSUP;
            break;
        }
    case eventRpcNotify:
        {
            vector< guint32 > vecParams;
            ret = GetParamList( vecParams,
                propTimerParamList );

            if( ERROR( ret ) )
                break;

            return OnNotify( eventRpcNotify,
                vecParams[ 1 ],
                vecParams[ 2 ],
                ( guint32* ) vecParams[ 3 ]);

            // we don't mean to complete
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    if( ret == -ENOTSUP )
    {
        return super::Process( dwContext );
    }

    do{
        if( ret == STATUS_PENDING )
        {
            break;
        }
        else if( ret == STATUS_MORE_PROCESS_NEEDED )
        {
            if( CanRetry() )
            {
                break;
            }
            ret = ERROR_FAIL;
        }
        else if( ret == -EAGAIN )
        {
            if( CanRetry() )
            {
                ret = STATUS_MORE_PROCESS_NEEDED;
                break;
            }
            ret = ERROR_FAIL;
        }

        OnComplete( ret );

    }while( 0 );

    if( ret == STATUS_PENDING ||
        ret == STATUS_MORE_PROCESS_NEEDED )
        MarkPending();

    return ret;
}

/**
* @name SubmitIoRequest: build the dbus message
* with the pReq and send an irp down to the
* transport layer. and the return values are
* returned in pResp. it has special processing for
* SendData and FetchData, which does not require a
* pEventSink as callback if it is asynchronous.
* instead, the callback is OnSendDataComplete and
* OnFetchDataComplete for proxy interface.
*
* params:
*   pReq: contains all the necessary input
*   parameters for the server to handle the
*   request.
*
*   the properties in this package includes
* 
*   propReturnValue: the int value as the method
*   return. ( only in pResp )
*
*   property 0..n: the parameters of the method to
*   call ( both in the pReq and pResp )
*
*   propMethodName: the method name to call
*
*   propCallOptions: the config db ptr for all the
*   options, or extra informations for this call
*
*       propCallFlags: the options for this method
*           call
*
*           the flags are:
*               keep-alive
*               with-reply
*               sync/async
*               msg-type: DBUS_MESSAGE_TYPE_XXX
*
*       propKeepAliveSec: if the keep-alive is
*       set, the heartbeat will be sent at the
*       rate of half of this value. ( in pReq only
*       )
*
*       propTimeoutSec: the seconds before the
*       request is canceled. it is effective for
*       asynchronous service request, that is the
*       RunTask returns STATUS_PENDING on return.
*
*   propEventSink: the pointer to the IEventSink
*   as the callback from the user. if it does not
*   exist, the call is a synchronous call and the
*   semaphore will be posted at the end of the
*   method call, whether completed or canceled.(
*   only in pReq )
*
*   propNotifyClient: the bool value to indicate
*   whether to notify the caller of this request
*   ( only in pReq )
*
*   propIpAddr: the remote server's ip address if
*   this interface is a remote proxy client.(
*   optional) ( in pReq, it is the destination ip
*   address, and in pResp, it is the source ip
*   address )
*
*   propMsgPtr: the pointer to req message if the
*   request is to send a response message.
*
*   propRespPtr:
*       the pointer to receive the response data
*
*   propReqPtr:
*       the pointer to the io request information
*   
* @{ */
/**  @} */

gint32 CIfIoReqTask::SubmitIoRequest()
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

        CfgPtr pReq;
        ret = GetReqCall( pReq );
        if( ERROR( ret ) )
            break;

        CfgPtr pResp;
        ret = GetRespData( pResp );
        if( ERROR( ret ) )
            break;

        if( pResp.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ObjPtr pObj;
        CRpcServices* pIf = nullptr;

        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pIf->SendMethodCall(
            pReq, pResp, this );

        if( ret == -EAGAIN )
        {
            ret = ERROR_FAIL;
            if( CanRetry() )
                ret = STATUS_MORE_PROCESS_NEEDED;
        }

    }while( 0 );

    return ret;
}

gint32 CIfIoReqTask::OnCancel(
    guint32 dwContext )
{
    if( m_iTaskState != stateStarted )
        return 0;

    gint32 ret = 0;

    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        CRpcServices* pIf = nullptr;
        
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;

        CfgPtr pResp;
        ret = GetRespData( pResp );
        if( ERROR( ret ) )
            break;

        if( pIf == nullptr || pResp.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIf->OnCancel(
            ( IConfigDb* )pResp, this );

    }while( 0 );

    super::OnCancel( dwContext );

    return ret;
}

gint32 CIfIoReqTask::RunTask()
{
    gint32 ret = SubmitIoRequest();
    return ret;
}

gint32 CIfIoReqTask::OnIrpComplete(
    IRP* pIrp )
{
    // for asynchronous call
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        gint32 iRet = pIrp->GetStatus();

        ObjPtr pObj;
        CParamList oCfg( GetConfig() );

        CfgPtr pResp;
        ret = GetRespData( pResp );
        if( SUCCEEDED( ret ) )
        {
            CCfgOpener oRespCfg(
                ( IConfigDb* )pResp );

            oRespCfg[ propReturnValue ] = iRet;

            if( ERROR( iRet ) )
                break;
        }
        else
        {
            CCfgOpener oRespCfg;
            oRespCfg[ propReturnValue ] = iRet;

            pResp = oRespCfg.GetCfg();
            SetRespData( pResp );

            if( ERROR( iRet ) )
                break;
        }

        // for out transfer, we only need to
        // return the pIrp->GetStatus()
        IrpCtxPtr& pCtx =
            pIrp->GetTopStack();

        guint32 dwIoDir =
            pCtx->GetIoDirection();

        if(  dwIoDir == IRP_DIR_OUT )
            break;

        ret = oCfg.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcServices* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pIf->FillRespData( pIrp, pResp );
        if( SUCCEEDED( ret ) )
        {
            SetRespData( pResp );
        }

    }while( 0 );

    return ret;
}

gint32 CIfIoReqTask::OnComplete(
    gint32 iRet )
{
    CParamList oParams( GetConfig() );
    ObjPtr pObj;

    super::OnComplete( iRet );

    oParams.RemoveProperty( propRespPtr );
    oParams.RemoveProperty( propReqPtr );

    return iRet;
}

gint32 CIfIoReqTask::OnKeepAlive(
    guint32 dwContext )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ObjPtr pObj;

        
        vector< guint32 > vecParams;
        ret = GetParamList( vecParams );
        if( ERROR( ret ) )
            break;

        if( vecParams[ 1 ] ==
            CRpcServices::KATerm )
        {
            ret = oCfg.GetObjPtr(
                propIrpPtr, pObj );

            if( ERROR( ret ) )
                break;

            IRP* pIrp = pObj;
            if( pIrp == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            pIrp->ResetTimer();
        }
        else if( vecParams[ 1 ] ==
            CRpcServices::KARelay )
        {
            ret = oCfg.GetObjPtr(
                propEventSink, pObj );

            if( ERROR( ret ) )
                break;

            TaskletPtr pTask;
            pTask = pObj;
            if( pTask.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            EnumEventId iEvent = ( EnumEventId )
                ( eventTryLock | eventKeepAlive );

            ret = pTask->OnEvent( iEvent,
                CRpcServices::KARelay,
                0, nullptr );
        }

    }while( 0 );

    return ret;
}

gint32 CIfIoReqTask::OnNotify( guint32 event,
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData )
{
    gint32 ret = 0;
    switch( ( EnumEventId )event )
    {
    case eventRpcNotify:
        {
            CCfgOpener oCfg(
                ( IConfigDb* )GetConfig() );

            ObjPtr pObj;

            ret = oCfg.GetObjPtr(
                propEventSink, pObj );

            if( ERROR( ret ) )
                break;

            IEventSink* pEvent = pObj;
            if( pEvent == nullptr )
                break;

            ObjPtr pIf;
            ret = oCfg.GetObjPtr( propIfPtr, pIf );
            if( ERROR( ret ) )
                break;

            CRpcServices* pService = pIf;
            if( pService == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            CIoManager* pMgr = pService->GetIoMgr();

            // forward this notification
            // NOTE: to avoid the lock nesting between
            // the tasks we defer the call
            //
            ret = DEFER_CALL( pMgr,
                ObjPtr( pEvent ),
                &IEventSink::OnEvent,
                ( EnumEventId )event,
                dwParam1,
                dwParam2,
                pData );

            break;
        }
    default:
        break;
    }
    return STATUS_PENDING;
}

CIfInvokeMethodTask::CIfInvokeMethodTask(
    const IConfigDb* pCfg ) : super( pCfg ),
    m_iKeepAlive( 0 )
{
    SetClassId( clsid( CIfInvokeMethodTask ) );
}

gint32 CIfInvokeMethodTask::RunTask()
{
    gint32 ret = 0;

    // servicing the incoming request/event
    // message
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        CRpcServices* pIf = nullptr;
        
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;

        gint32 iType = 0;

        ret = GetConfig()->GetPropertyType(
            propMsgPtr, iType );

        if( ERROR( ret ) )
            break;

        if( iType == typeDMsg )
        
        {
            DMsgPtr pMsg;
            ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
            if( ERROR( ret ) )
                break;

            if( pIf == nullptr || pMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            if( pIf->GetState() != stateConnected )
            {
                // NOTE: this is not a serious state
                // check
                ret = ERROR_STATE;
                break;
            }

            std::string strMethod = pMsg.GetMember();

            // set the interface name in case the
            // method be called via the handler map
            oCfg[ propIfName ] = pMsg.GetInterface();

            // WOW, finally we are about to go to the
            // user code
            //
            // since it could be very slow in the
            // InvokeMethod, we run this method after
            // the StartRecvMsgTask. it is possible in
            // multi-processor environment, the Task
            // could be scheduled ahead of the
            // InvokeMethod
            ret = pIf->InvokeMethod< DBusMessage >
                ( ( DBusMessage* )pMsg, this );
        }
        else
        {

            ObjPtr pObj;
            ret = oCfg.GetObjPtr( propMsgPtr, pObj );
            if( ERROR( ret ) )
                break;

            CfgPtr pMsg = pObj;
            if( pMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            if( pIf == nullptr || pMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            if( pIf->GetState() != stateConnected )
            {
                // NOTE: this is not a serious state
                // check
                ret = ERROR_STATE;
                break;
            }

            IConfigDb* pReq = pMsg;
            ret = pIf->InvokeMethod< IConfigDb >
                ( pReq, this );

            if( ret == STATUS_PENDING )
            {
                SET_RMT_TASKID( pReq, oCfg );
            }
        }

        if( ret == STATUS_PENDING )
            break;

        if( ret == -EAGAIN ||
            ret == STATUS_MORE_PROCESS_NEEDED )
        {
            ret = ERROR_FAIL;
            if( CanRetry() )
                ret = STATUS_MORE_PROCESS_NEEDED;
            // schedule a retry
            break;
        }

        OnTaskComplete( ret );

    }while( 0 );

    return ret;
}

gint32 CIfInvokeMethodTask::OnCancel(
    guint32 dwContext )
{
    gint32 ret = 0;

    super::OnCancel( dwContext );

    if( dwContext == eventTimeoutCancel )
    {
        // we are in the the timeout context, the
        // m_iTimoutId should be simply zeroed
        m_iTimeoutId = 0;
    }

    // cancel this task
    do{
        if( !IsPending() )
        {
            // how did we come here?
            // Because the lock on CIfParallelTask
            // is already acquired at this point.
            // it is not possible to be landing
            // here
            ret = ERROR_FAIL;
            break;
        }
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        CRpcServices* pIf = nullptr;
        
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;
        gint32 iType = 0;
        ret = GetConfig()->GetPropertyType(
            propMsgPtr, iType );

        if( ERROR( ret ) )
            break;

        if( iType == typeDMsg )
        {
            DMsgPtr pMsg;
            ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
            if( ERROR( ret ) )
                break;

            if( pIf == nullptr || pMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            ret = pIf->OnCancel( pMsg, this );
        }
        else if( iType == typeObj )
        {
            ObjPtr pObj;
            CfgPtr pMsg;
            ret = oCfg.GetObjPtr( propMsgPtr, pObj );
            if( ERROR( ret ) )
                break;

            pMsg = pObj;
            if( pIf == nullptr || pMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            ret = pIf->OnCancel(
                ( IConfigDb* )pMsg, this );
        }
        else
        {
            ret = -ENOTSUP;
            break;
        }

        OnTaskComplete( -ECANCELED );

    }while( 0 );

    return ret;
}

gint32 CIfInvokeMethodTask::OnTaskComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propRespPtr, pObj );

        if( ERROR( ret ) )
            break;

        CfgPtr pResp;
        pResp = pObj;
        if( pResp.IsEmpty() )
        {
            ret = 0;
            break;
        }

        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcServices* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        
        CInterfaceServer *pServer =
            dynamic_cast< CInterfaceServer* >( pIf );

        bool bServer = true;

        if( pServer == nullptr )
            bServer = false;

        gint32 iType = 0;
        ret = GetConfig()->GetPropertyType(
            propMsgPtr, iType );

        if( ERROR( ret ) )
            break;

        if( iType == typeDMsg )
        {
            DMsgPtr pMsg;
            ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
            if( ERROR( ret ) )
                break;

            gint32 iType = pMsg.GetType();
            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_SIGNAL:
                {
                    bServer ? ret = -EINVAL : 0;
                    break;
                }
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
                {
                    ret = pIf->SendResponse(
                        pMsg, pResp );
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
        }
        else if( iType == typeObj )
        {
            ObjPtr pObj;
            ret = oCfg.GetObjPtr(
                propMsgPtr, pObj );

            if( ERROR( ret ) )
                break;

            IConfigDb* pMsg = pObj;
            if( pMsg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            CReqOpener oCfg( pMsg );
            ret = oCfg.GetReqType(
                ( guint32& )iType );

            if( ERROR( ret ) )
                break;

            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_SIGNAL:
                {
                    bServer ? ret = -EINVAL : 0;
                    break;
                }
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
                {
                    ret = pIf->SendResponse(
                        pMsg, pResp );
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
        }

    }while( 0 );

    return 0;
}

gint32 CIfInvokeMethodTask::OnComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;

    if( m_iKeepAlive > 0 )
    {
        RemoveTimer( m_iKeepAlive );
        m_iKeepAlive = 0;
    }
    if( m_iTimeoutId > 0 )
    {
        RemoveTimer( m_iTimeoutId );
        m_iTimeoutId = 0;
    }

    // servicing the next incoming request or
    // event message
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );
        CRpcServices* pIf = nullptr;
        
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        if( pIf->IsQueuedReq() )
        {
            if( pIf->GetQueueSize() == 0 )
                break;

            CParamList oParams;
            oParams.CopyProp( propIfPtr, this );

            // move on to the next request if
            // incoming requests are queued
            CIoManager* pMgr = pIf->GetIoMgr();
            if( pMgr == nullptr )
            {
                // fatal error, cannot recover
                break;
            }

            // NOTE: we use another task instead
            // of InvokeMethod in this context to
            // avoid the parallel tasks nested in
            // each other's context
            ret = pMgr->ScheduleTask(
                clsid( CIfServiceNextMsgTask ),
                oParams.GetCfg() );
        }

    }while( 0 );

    ret = super::OnComplete( iRetVal );

    RemoveProperty( propMsgPtr );
    RemoveProperty( propCallFlags );
    RemoveProperty( propRespPtr );
    RemoveProperty( propMatchPtr );

    return ret;
}

gint32 CIfInvokeMethodTask::OnKeepAlive(
    guint32 dwContext )
{
    if( dwContext == eventTimeout )
        OnKeepAliveOrig();

    else if( dwContext == eventKeepAlive )
        OnKeepAliveRelay();

    return STATUS_PENDING;
}

gint32 CIfInvokeMethodTask::OnKeepAliveOrig()
{
    gint32 ret = 0;
    // NOTE: this method should be called by the
    // timer only
    // Send the heartbeat, schedule new timer,
    // and return status_pending for the next
    do{
        ObjPtr pObj;
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() );
        guint32 dwTimeoutSec = 0;

        if( !IsKeepAlive() )
            break;

        ret = GetKeepAliveSec( dwTimeoutSec );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }

        ret = oTaskCfg.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        // keep alive can only be sent out by
        // interface server
        CInterfaceServer* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIf->OnKeepAlive( this,
            CRpcServices::KAOrigin ); 

        if( ret != STATUS_PENDING )
            break;

        // schedule the next keep-alive event
        CIoManager* pMgr = pIf->GetIoMgr();
        CUtilities& oUtils = pMgr->GetUtils();

        CTimerService& oTimerSvc =
            oUtils.GetTimerSvc();

        m_iKeepAlive = oTimerSvc.AddTimer(
            dwTimeoutSec, this,
            ( guint32 )eventKeepAlive );

        if( m_iTimeoutId > 0 )
        {
            oTimerSvc.ResetTimer( m_iTimeoutId );
        }
        
    }while( 0 );

    // return status_pending to avoid the task
    // from completed
    return STATUS_PENDING;
}

gint32 CIfInvokeMethodTask::OnKeepAliveRelay()
{
    gint32 ret = 0;
    // NOTE: for the rest CIfInvokeMethodTask's
    // along the forward-request path, this method
    // is called
    do{
        ObjPtr pObj;
        CCfgOpener oTaskCfg( ( IConfigDb* )GetConfig() );

        ret = oTaskCfg.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        // keep alive can only be sent out by
        // interface server
        CInterfaceServer* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIf->OnKeepAlive(
            this, CRpcServices::KARelay );
                
    }while( 0 );

    // return status_pending to avoid the task
    // from completed
    return STATUS_PENDING;
}

gint32 CIfInvokeMethodTask::GetCallOptions(
    CfgPtr& pCfg ) const
{
    const IConfigDb* pcCfg = GetConfig();

    CCfgOpener oCfg( pcCfg );
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        ret = oCfg.GetObjPtr(
            propCallOptions, pObj );

        if( ERROR( ret ) )
            break;

        CfgPtr pSrcCfg = pObj;
        if( pSrcCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        // make the copy of the ptr
        pCfg = pSrcCfg;

    }while( 0 );

    return ret;
}

gint32 CIfInvokeMethodTask::GetCallFlags(
    guint32& dwMsgType ) const
{
    gint32 ret = 0;
    CfgPtr pCfg;
    do{
        ret = GetCallOptions( pCfg );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg( ( IConfigDb* )pCfg );
        guint32 dwFlags = 0;
        ret = oCfg.GetIntProp(
            propCallFlags, dwFlags );

        if( ERROR( ret ) )
            break;

        dwMsgType = dwFlags;
        
    }while( 0 );

    return ret;
}

gint32 CIfInvokeMethodTask::GetTimeoutSec(
    guint32& dwTimeoutSec ) const 
{
    gint32 ret = 0;
    CfgPtr pCfg;
    do{
        ret = GetCallOptions( pCfg );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg( ( IConfigDb* )pCfg );
        guint32 dwTimeout = 0;
        ret = oCfg.GetIntProp(
            propTimeoutSec, dwTimeout );

        if( ERROR( ret ) )
            break;

        dwTimeoutSec = dwTimeout;
        
    }while( 0 );

    return ret;
}

bool CIfInvokeMethodTask::IsKeepAlive() const
{
    guint32 dwFlags = 0;

    gint32 ret = GetCallFlags( dwFlags );

    // by default, no keep-alive
    if( ERROR( ret ) )
        return false;

    if( dwFlags & CF_KEEP_ALIVE )
        return true;

    return false;
}

gint32 CIfInvokeMethodTask::GetKeepAliveSec(
    guint32& dwTimeoutSec )  const
{
    CfgPtr pCfg;

    gint32 ret = GetCallOptions( pCfg );
    if( ERROR( ret ) )
        return ret;

    guint32 dwSeconds;
    CCfgOpener oCfg( ( IConfigDb* )pCfg );

    ret = oCfg.GetIntProp(
        propKeepAliveSec, dwSeconds );

    if( ERROR( ret ) )
        return ret;

    dwTimeoutSec = dwSeconds;

    return ret;
}

bool CIfInvokeMethodTask::HasReply() const
{
    guint32 dwFlags = 0;

    gint32 ret = GetCallFlags( dwFlags );

    // by default, no reply
    if( ERROR( ret ) )
        return false;

    if( dwFlags & CF_WITH_REPLY )
        return true;

    return false;
}

/**
* @name SetAsyncCall, set propCallFlags to tell the
* on-going method is whether a sync call or not.
* This flag is set when the pIf->InvokeMethod
* returns STATUS_PENDING. However, this method
* does more than just set a flag. if this method
* is an async call, the task will set the
* keep-alive timer and the request timeout timer
* too. The two timers are set only when the
* propKeepAliveSec or propTimeoutSec is set
* respectively.
* @{ */
/**  @} */

gint32 CIfInvokeMethodTask::SetAsyncCall(
    bool bAsync )
{
    CfgPtr pCfg;
    gint32 ret = 0;

    while( bAsync )
    {
        // let's schedule an expire timer if
        // required
        
        ObjPtr pObj;
        CCfgOpener oTaskCfg( ( IConfigDb* )GetConfig() );
        guint32 dwTimeoutSec = 0;

        ret = GetTimeoutSec( dwTimeoutSec );
        if( ERROR( ret ) )
        {
            // fine, we are ok with it
            ret = GetKeepAliveSec( dwTimeoutSec );
            if( ERROR( ret ) )
            {
                ret = 0;
                break;
            }
            // let's use 2 * KeepAliveSec as
            // the timeout value
            dwTimeoutSec *= 2;
        }

        ret = oTaskCfg.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcServices* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CIoManager* pMgr = pIf->GetIoMgr();
        CUtilities& oUtils = pMgr->GetUtils();
        CTimerService& oTimerSvc =
            oUtils.GetTimerSvc();

        m_iTimeoutId = oTimerSvc.AddTimer(
            dwTimeoutSec, this,
            ( guint32 )eventTimeoutCancel );

        if( !IsKeepAlive() )
            break;

        ret = GetKeepAliveSec( dwTimeoutSec );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }
        m_iKeepAlive = oTimerSvc.AddTimer(
            dwTimeoutSec, this,
            ( guint32 )eventKeepAlive );

        break;
    }
    
    return ret;
}

gint32 CIfServiceNextMsgTask::RunTask()
{
    gint32 ret = 0;
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    ObjPtr pObj;
    
    ret = oCfg.GetObjPtr( propIfPtr, pObj );
    if( ERROR( ret ) )
        return ret;

    CRpcServices* pIf = pObj;
    if( pIf == nullptr )
        return -EFAULT;

    pIf->InvokeNextMethod();

    return 0;
}

gint32 CopyFile( gint32 iFdSrc, gint32 iFdDest, ssize_t& iSize );

gint32 CIfFetchDataTask::RunTask()
{
    gint32 ret = 0;
    CParamList oCfg( ( IConfigDb* )GetConfig() );
    ObjPtr pObj;

    gint32 iFdSrc = -1;
    gint32 iFdDest = -1;
    
    do{
        guint32 dwOffset = 0;
        guint32 dwSize = 0;

        ret = oCfg.GetObjPtr( 0, pObj );
        if( ERROR( ret ) )
            break;

        CfgPtr pDesc = pObj;
        if( pDesc.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpener oDesc;
        *oDesc.GetCfg() = *pDesc;

        ret = oCfg.GetIntProp( 1, dwOffset );
        if( ERROR( ret ) )
            break;

        ret = oCfg.GetIntProp( 2, dwSize );
        if( ERROR( ret ) )
            break;

        guint32 dwVal;
        ret = oCfg.GetIntProp( 3, dwVal );
        if( ERROR( ret ) )
            break;

        iFdSrc = *( gint32* )&dwVal;

        ret = oCfg.GetIntProp( 4, dwVal );
        if( ERROR( ret ) )
            break;

        iFdDest = *( gint32* )&dwVal;

        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CInterfaceServer* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = oCfg.GetObjPtr( propEventSink, pObj );
        if( ERROR( ret ) )
            break;

        IEventSink* pCallback = pObj;
        if( pCallback == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ssize_t iSize = dwSize;
        ssize_t iBytes = 4 * MAX_BYTES_PER_TRANSFER > iSize
            ? iSize : 4 * MAX_BYTES_PER_TRANSFER;

        while( iSize > 0 && !m_bExit )
        {
            ret = CopyFile( iFdSrc, iFdDest, iBytes );
            if( iSize > iBytes )
            {
                iSize -= iBytes;
                if( iSize < iBytes )
                    iSize = iBytes;
            }
            else
            {
                iSize = 0;
                break;
            }
        }
        if( m_bExit.load( std::memory_order_acquire ) )
        {
            ret = -ECANCELED;
            break;
        }

        CParamList oResp;
        oResp.SetIntProp( propReturnValue, ret );

        if( SUCCEEDED( ret ) )
        {
            dwSize = iSize;
            oResp.Push( ObjPtr( oDesc.GetCfg() ) );
            oResp.Push( iFdDest ); // fd with the data
            oResp.Push( dwOffset ); // offset
            oResp.Push( dwSize );   // offset
        }

        ret = pIf->OnServiceComplete(
            oResp.GetCfg(), pCallback );

    }while( 0 );

    if( iFdSrc >= 0 )
        close( iFdSrc );
    
    if( iFdDest >= 0 )
        close( iFdDest );

    return ret;
}

gint32 CIfFetchDataTask::OnCancel(
    guint32 dwContext )
{
    m_bExit.store( true, std::memory_order_release );
    while( m_bEnded.load( std::memory_order_acquire ) )
        usleep( 100000 );

    return 0;
}

CIfDummyTask::CIfDummyTask( const IConfigDb* pCfg )
   :super( pCfg ) 
{
    SetClassId( clsid( CIfDummyTask ) );
}

gint32 CIfDummyTask::RunTask()
{
    DebugPrint( 0, "CIfDummyTask is called" );
    return 0;
}

gint32 CIfDummyTask::operator()(
    guint32 dwContext )
{ return 0; }

gint32 CIfDummyTask::OnEvent(
    EnumEventId iEvent,
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData )
{ return 0; }

gint32 CIoReqSyncCallback::operator()(
    guint32 dwContext )
{
    if( dwContext != eventTaskComp )
        return -ENOTSUP;

    gint32 ret = 0;
    switch( ( EnumEventId )dwContext )
    {
    case eventTaskComp:
        {
            std::vector< guint32 > vecParams;
            do{
                ret = GetParamList(
                    vecParams, propParamList );

                if( ERROR( ret ) )
                    break;

                // the result of the io request
                ret = vecParams[ 1 ];
                if( ERROR( ret ) )
                    break;

                // the task could be the CIfIoReqTask
                CObjBase* pTask =
                    ( CObjBase* ) vecParams[ 3 ];

                if( pTask == nullptr )
                    break;

                TaskletPtr pT =
                    static_cast< CTasklet* >( pTask );

                if( pT.IsEmpty() )
                    break;

                CParamList oParams(
                    ( IConfigDb* )pT->GetConfig() );

                if( !oParams.exist( propRespPtr ) )
                {
                    DebugPrint( -ENOENT, "no Resp ptr" );
                    ret = -EINVAL;
                    break;
                }

                IConfigDb* pCfg = nullptr;
                ret = oParams.GetPointer(
                    propRespPtr, pCfg );

                if( ERROR( ret ) )
                    break;

                CCfgOpener oCfg( pCfg );
                if( !oCfg.exist( propReturnValue ) )
                {
                    ret = -EINVAL;
                    break;
                }

                CCfgOpener oTaskCfg(
                    (IConfigDb*)GetConfig() );

                oTaskCfg[ propRespPtr ] =
                    ObjPtr( pCfg );

                ret = 0;

            }while( 0 );

            SetError( ret );
            Sem_Post( &m_semWait );
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    return ret;
}

