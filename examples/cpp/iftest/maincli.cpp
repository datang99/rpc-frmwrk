// Generated by ridlc
#include "rpc.h"
#include "proxy.h"
using namespace rpcf;
#include "IfTestcli.h"

ObjPtr g_pIoMgr;

extern FactoryPtr InitClassFactory();
gint32 InitContext()
{
    gint32 ret = CoInitialize( 0 );
    if( ERROR( ret ) )
        return ret;
    do{
        // load class factory for 'iftest'
        FactoryPtr p = InitClassFactory();
        ret = CoAddClassFactory( p );
        if( ERROR( ret ) )
            break;
        
        CParamList oParams;
        oParams.Push( "iftestcli" );

        // adjust the thread number if necessary
        oParams[ propMaxIrpThrd ] = 0;
        oParams[ propMaxTaskThrd ] = 1;
        
        ret = g_pIoMgr.NewObj(
            clsid( CIoManager ), 
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;
        
        IService* pSvc = g_pIoMgr;
        ret = pSvc->Start();
        
    }while( 0 );

    if( ERROR( ret ) )
    {
        g_pIoMgr.Clear();
        CoUninitialize();
    }
    return ret;
}

gint32 DestroyContext()
{
    IService* pSvc = g_pIoMgr;
    if( pSvc != nullptr )
    {
        pSvc->Stop();
        g_pIoMgr.Clear();
    }

    CoUninitialize();
    DebugPrintEx( logErr, 0,
        "#Leaked objects is %d",
        CObjBase::GetActCount() );
    return STATUS_SUCCESS;
}


FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CIfTest_CliImpl );
    
    INIT_MAP_ENTRY( GlobalFeature );
    INIT_MAP_ENTRY( GlobalFeatureList );
    
    END_FACTORY_MAPS;
}

extern "C"
gint32 DllLoadFactory( FactoryPtr& pFactory )
{
    pFactory = InitClassFactory();
    if( pFactory.IsEmpty() )
        return -EFAULT;
    return STATUS_SUCCESS;
}

gint32 maincli(
    CIfTest_CliImpl* pIf,
    int argc, char** argv );

int main( int argc, char** argv)
{
    gint32 ret = 0;
    do{
        ret = InitContext();
        if( ERROR( ret ) )
            break;
        
        InterfPtr pIf;
        CParamList oParams;
        oParams[ propIoMgr ] = g_pIoMgr;
        ret = CRpcServices::LoadObjDesc(
            "./iftestdesc.json",
            "IfTest",
            false, oParams.GetCfg() );
        if( ERROR( ret ) )
            break;
        
        ret = pIf.NewObj(
            clsid( CIfTest_CliImpl ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;
        
        CIfTest_CliImpl* pSvc = pIf;
        ret = pSvc->Start();
        if( ERROR( ret ) )
            break;
        
        while( pSvc->GetState()== stateRecovery )
            sleep( 1 );
        
        if( pSvc->GetState() != stateConnected )
        {
            ret = ERROR_STATE;
            break;
        }

        maincli( pSvc, argc, argv );
        
        // Stopping the object
        ret = pSvc->Stop();
        
    }while( 0 );

    DestroyContext();
    return ret;
}

//-----Your code begins here---
gint32 maincli(
    CIfTest_CliImpl* pIf,
    int argc, char** argv )
{
    //-----Adding your code here---
    GlobalFeatureList i0;
    i0.extendUserFeature =
        {{2, "extendUserFeature"},
         {3, "extendFeatureUser"}};

    i0.userFeature = {
        {1, "userFeature"},
        {10, "userFeature10"}
    };

    i0.userOnlineFeature = {
        {"hello", "userOnlineFeature"},
        {"hallow", "userFeatureOnline"}
    };

    i0.userHistoricalStatisticsFeature = {
        {3,5},{8,9}
    };

    i0.sceneFeature = {
        {6, "sceneFeature"},
        {66, "FeatureScene"}
    };

    i0.formL24Exposure = {
        { "form24Exposure", 7 },
        { "form24Expoe", 71 },
    };

    i0.industryL24Exposure = {
        {"industryL24Exposure", 8},
        {"industryL24Exposure", 18}
    };
    
    i0.flightId = "sz2680";
    i0.requestId = "haha9527";
    i0.time = 23456;
    i0.flag = 9;
    GlobalFeature a;
    i0.body.push_back(a);
    i0.body.push_back(a);

    i0.body[0].adFeature = {
        {11, "adFeature"},
        {111, "adfeature"}
    };

    i0.body[0].extendAdFeature = {
        { 11, "extendAdFeature" },
        { 12, "extendadfeature" },
    };

    i0.body[0].adHistoricalStatisticsFeature = {
        { 12, 13 }
    };

    i0.body[0].userRealtimeStatisticsFeature = {
        { 14, 15 }
    };

    i0.body[0].extenduserRealtimeStatisticsFeature = {
        { 16, 17 }
    };

    i0.body[0].adRealtimeStatisticsFeature = {
        { 18, 19 }
    };
    i0.body[0].adFormId = "bobo";
    i0.body[0].materialId = "steel";
    i0.body[0].creativeId = "cici";
    i0.body[0].sponsorId = "dudu";

    i0.body[1].adFeature = {
        {21, "adFeature"},
        {211, "adfeature"}
    };

    i0.body[1].extendAdFeature = {
        { 21, "extendAdFeature" },
        { 32, "extendadfeature" },
    };

    i0.body[1].adHistoricalStatisticsFeature = {
        { 32, 13 }
    };

    i0.body[1].userRealtimeStatisticsFeature = {
        { 34, 15 }
    };

    i0.body[1].extenduserRealtimeStatisticsFeature = {
        { 46, 17 }
    };

    i0.body[1].adRealtimeStatisticsFeature = {
        { 58, 19 }
    };
    i0.body[1].adFormId = "fofo";
    i0.body[1].materialId = "plastic";
    i0.body[1].creativeId = "giogio";
    i0.body[1].sponsorId = "jojo";

    GlobalFeatureList i0r;
    gint32 ret = pIf->Echo( i0, i0r );
    if( ERROR( ret ))
    {
        OutputMsg( ret, "Echo failed with error " );
        return ret;
    }

    OutputMsg( ret, "Echo completed successfully" );
    return STATUS_SUCCESS;
}

