// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2023  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ridlc -bsO . ../../testypes.ridl 
#include "rpc.h"
#include "proxy.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "TestTypesSvccli.h"

ObjPtr g_pIoMgr;
std::set< guint32 > g_setMsgIds;

FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CTestTypesSvc_CliImpl );
    INIT_MAP_ENTRYCFG( CTestTypesSvc_CliSkel );
    INIT_MAP_ENTRYCFG( CTestTypesSvc_ChannelCli );
    
    INIT_MAP_ENTRY( FILE_INFO );
    
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

static std::string g_strDrvPath;
static std::string g_strObjDesc;
static std::string g_strRtDesc;
static std::string g_strInstName( "TestTypes_rt_" );
static std::string g_strSaInstName;
static std::string g_strIpAddr;
static std::string g_strPortNum;
static bool g_bAuth = false;
static bool g_bKProxy = false;
static bool g_bAsync = false;
static std::string g_strUserName;
static ObjPtr g_pRouter;
char g_szKeyPass[ SSL_PASS_MAX + 1 ] = {0};

namespace rpcf{
extern "C" gint32 CheckForKeyPass(
    bool& bPrompt );
}
void Usage( char* szName )
{
    fprintf( stderr,
        "Usage: %s [OPTION]\n"
        "\t [ -a to enable authentication. ]\n"
        "\t [ -d to run as a daemon. ]\n"
        "\t [ -i <ip address> to specify the destination ip address. ]\n"
        "\t [ -p <port number> to specify the tcp port number to. ]\n"
        "\t [ -k to run as a kinit proxy. ]\n"
        "\t [ -l <user name> login with the user name and then quit. ]\n"
        "\t [ --driver <path> to specify the path to the customized 'driver.json'. ]\n"
        "\t [ --objdesc <path> to specify the path to the object description file. ]\n"
        "\t [ --router <path> to specify the path to the customized 'router.json'. ]\n"
        "\t [ --instname <name> to specify the server instance name to connect'. ]\n"
        "\t [ --sainstname <name> to specify the stand-alone router instance name to connect'. ]\n"
        "\t [ -h this help ]\n", szName );
}

#include <getopt.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
std::atomic< bool > s_bExit( false );
void SignalHandler( int signum )
{ s_bExit = true; }

int KProxyLoop()
{
    if( g_strUserName.size() )
    {
        std::string strCmd = "kinit -ki ";
        strCmd += g_strUserName;
        gint32 ret = system( strCmd.c_str() );
        return -ret;
        
    }
    signal( SIGINT, SignalHandler );
    while( !s_bExit )
        sleep( 3 );
    printf( "\n" );
    return 0;
}
int _main( int argc, char** argv);
int main( int argc, char** argv )
{
    bool bDaemon = false;
    int opt = 0;
    int ret = 0;
    do{
        gint32 iOptIdx = 0;
        struct option arrLongOptions[] = {
            {"driver",   required_argument, 0,  0 },
            {"objdesc",  required_argument, 0,  0 },
            {"router",   required_argument, 0,  0 },
            {"instname", required_argument, 0,  0 },
            {"sainstname", required_argument, 0,  0 },
            {0,             0,                 0,  0 }
        };            
        while( ( opt = getopt_long( argc, argv, "hadkl:i:p:s",
            arrLongOptions, &iOptIdx ) ) != -1 )
        {
            switch( opt )
            {
            case 0:
                {
                    if( iOptIdx < 3 )
                    {
                        struct stat sb;
                        ret = lstat( optarg, &sb );
                        if( ret < 0 )
                        {
                            perror( strerror( errno ) );
                            ret = -errno;
                            break;
                        }
                        if( ( sb.st_mode & S_IFMT ) != S_IFLNK &&
                            ( sb.st_mode & S_IFMT ) != S_IFREG )
                        {
                            fprintf( stderr, "Error invalid file '%s'.\n", optarg );
                            ret = -EINVAL;
                            break;
                        }
                    }
                    if( iOptIdx == 0 )
                        g_strDrvPath = optarg;
                    else if( iOptIdx == 1 )
                        g_strObjDesc = optarg;
                    else if( iOptIdx == 2 )
                        g_strRtDesc = optarg;
                    else if( iOptIdx == 3 || iOptIdx == 4 )
                    {
                        if( !IsValidName( optarg ) )
                        {
                            fprintf( stderr, "Error invalid instance name '%s'.\n", optarg );
                            ret = -EINVAL;
                            break;
                        }
                        if( iOptIdx == 3 )
                           g_strInstName = optarg;
                        else
                            g_strSaInstName = optarg;
                        if( g_strSaInstName.size() &&
                            g_strInstName != "TestTypes_rt_" )
                        {
                            fprintf( stderr, "Error specifying both 'instname' "
                                "and 'sainstname' at the same time"
                                " '%s'.\n", optarg );
                            ret = -EINVAL;
                            break;
                        }
                    }
                    else
                    {
                        fprintf( stderr, "Error invalid option.\n" );
                        Usage( argv[ 0 ] );
                        ret = -EINVAL;
                    }
                    break;
                }
            case 'i':
                {
                    std::string strTemp;
                    ret = NormalizeIpAddrEx(
                        optarg, strTemp );
                    if( ERROR( ret ) )
                    {
                        ret = -EINVAL;
                        fprintf( stderr,
                            "Error invalid ip address.\n" );
                    }
                    g_strIpAddr = optarg;
                    break;
                }
            case 'p':
                {
                    std::string g_strPortNum = optarg;
                    guint32 dwPort = strtoul(
                        g_strPortNum.c_str(), nullptr, 10 );
                    if( dwPort > 65535 || dwPort < 1024 )
                    {
                        ret = -EINVAL;
                        fprintf( stderr,
                            "Error invalid tcp port number.\n" );
                    }
                    break;
                }
            case 'l':
            case 'k':
                {
                    g_bKProxy = true;
                    if( opt == 'l' )
                        g_strUserName = optarg;
                    break;
                }
            case 'a':
                { g_bAuth = true; break; }
            case 'd':
                { bDaemon = true; break; }
            case 's':
                { g_bAsync = true; break; }
            case 'h':
            default:
                { Usage( argv[ 0 ] ); exit( 0 ); }
            }
        }
        if( ERROR( ret ) )
            break;
        if( !g_bAuth && g_bKProxy )
        {
            fprintf( stderr,
                "Error '-k' is only valid with '-a' option.\n" );
            ret = -EINVAL;
        }
        if( true )
        {
            bool bPrompt = false;
            bool bExit = false;
            ret = CheckForKeyPass( bPrompt );
            while( SUCCEEDED( ret ) && bPrompt )
            {
                char* pPass = getpass( "SSL Key Password:" );
                if( pPass == nullptr )
                {
                    bExit = true;
                    ret = -errno;
                    break;
                }
                size_t len = strlen( pPass );
                len = std::min(
                    len, ( size_t )SSL_PASS_MAX );
                memcpy( g_szKeyPass, pPass, len );
                break;
            }
            if( bExit )
                break;
        }
        // update config files
        do{
            CCfgOpener oCfg;
            oCfg[ 101 ] = 1;
            oCfg[ 102 ] = ( bool& )g_bAuth;
            oCfg[ 104 ] = "TestTypes";
            if( g_strInstName != "TestTypes_rt_" )
                oCfg[ 107 ] = g_strInstName;
            if( g_strSaInstName.size() )
                oCfg[ 108 ] = g_strSaInstName;
            if( g_strIpAddr.size() )
                oCfg[ 109 ] = g_strIpAddr;
            if( g_strPortNum.size() )
                oCfg[ 110 ] = g_strPortNum;
            
            std::string strDesc;
            if( g_strObjDesc.empty() )
                strDesc = "./TestTypesdesc.json";
            else
                strDesc = g_strObjDesc;
            std::string strNewDesc;
            ret = UpdateObjDesc( strDesc,
                oCfg.GetCfg(), strNewDesc );
            if( ERROR( ret ) )
                break;
            if( strNewDesc.size() )
                g_strObjDesc = strNewDesc;
            if( g_strInstName == "TestTypes_rt_" &&
                g_strSaInstName.empty() )
                oCfg.GetStrProp( 107, g_strInstName );
            else if( g_strSaInstName.size() )
                g_strInstName = g_strSaInstName;
        }while( 0 );
        if( ERROR( ret ) )
            break;
        
        if( bDaemon )
        {
            ret = daemon( 1, 0 );
            if( ret < 0 )
            { ret = -errno; break; }
        }
        ret = _main( argc, argv );
    }while( 0 );
    // cleanup
    do{
        if( g_strObjDesc.substr( 0, 12 ) ==
            "/tmp/rpcfod_" )
            unlink( g_strObjDesc.c_str() );
    }while( 0 );
    return ret;
}

gint32 InitContext()
{
    gint32 ret = CoInitialize( 0 );
    if( ERROR( ret ) )
        return ret;
    do{
        // load class factory for 'TestTypes'
        FactoryPtr p = InitClassFactory();
        ret = CoAddClassFactory( p );
        if( ERROR( ret ) )
            break;
        
        CParamList oParams;
        oParams.Push( "TestTypescli" );

        // adjust the thread number if necessary
        guint32 dwNumThrds =
            ( guint32 )std::max( 1U,
            std::thread::hardware_concurrency() );
        if( dwNumThrds > 1 )
            dwNumThrds = ( dwNumThrds >> 1 );
        oParams[ propMaxTaskThrd ] = dwNumThrds;
        oParams[ propMaxIrpThrd ] = 0;
        if( g_strDrvPath.size() )
            oParams[ propConfigPath ] = g_strDrvPath;
        
        ret = g_pIoMgr.NewObj(
            clsid( CIoManager ), 
            oParams.GetCfg() );
        if( ERROR( ret ) )
        {
            OutputMsg( ret, "Error create iomgr" );
            break;
        }

        CIoManager* pSvc = g_pIoMgr;
        pSvc->SetCmdLineOpt(
            propRouterRole, 1 );
        pSvc->SetCmdLineOpt(
            propBuiltinRt, true );
        ret = pSvc->Start();
        if( ERROR( ret ) )
        {
            OutputMsg( ret, "Error start iomgr" );
            break;
        }
        
        if( g_bKProxy )
            pSvc->SetCmdLineOpt(
                propKProxy, g_bKProxy );
        // create and start the router
        std::string strRtName = "TestTypes_rt_";
        if( !g_bKProxy )
            pSvc->SetRouterName( strRtName +
                std::to_string( getpid() ) );
        else
            pSvc->SetRouterName( MODNAME_RPCROUTER );
        std::string strDescPath;
        if( g_strRtDesc.size() )
            strDescPath = g_strRtDesc;
        else if( g_bAuth )
            strDescPath = "./rtauth.json";
        else
            strDescPath = "./router.json";
        
        if( g_bAuth )
        {
            pSvc->SetCmdLineOpt(
                propHasAuth, g_bAuth );
        }
        CCfgOpener oRtCfg;
        oRtCfg.SetStrProp( propSvrInstName,
            g_strInstName );
        oRtCfg[ propIoMgr ] = g_pIoMgr;
        CIoManager* pMgr = g_pIoMgr;
        pMgr->SetCmdLineOpt( propSvrInstName,
             g_strInstName );
        
        ret = CRpcServices::LoadObjDesc(
            strDescPath,
            OBJNAME_ROUTER,
            true, oRtCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        oRtCfg[ propIfStateClass ] =
            clsid( CIfRouterMgrState );
        oRtCfg[ propRouterRole ] = 1;
        ret =  g_pRouter.NewObj(
            clsid( CRpcRouterManagerImpl ),
            oRtCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        CInterfaceServer* pRouter = g_pRouter;
        if( unlikely( pRouter == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        ret = pRouter->Start();
        if( ERROR( ret ) )
        {
            OutputMsg( ret, "Error start router" );
            pRouter->Stop();
            g_pRouter.Clear();
            break;
        }
    }while( 0 );

    return ret;
}

gint32 DestroyContext()
{
    if( !g_pRouter.IsEmpty() )
    {
        IService* pRt = g_pRouter;
        pRt->Stop();
        g_pRouter.Clear();
    }
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

gint32 maincli( CTestTypesSvc_CliImpl* pIf, int argc, char** argv );

int _main( int argc, char** argv )
{
    gint32 ret = 0;
    do{
        std::string strDesc;
        if( g_strObjDesc.empty() )
            strDesc = "./TestTypesdesc.json";
        else
            strDesc = g_strObjDesc;
        std::string strInstId = InstIdFromObjDesc(
            strDesc, "TestTypesSvc" );
        if( strInstId.empty() )
        { ret = -EINVAL; break;}
        if( g_strInstName == "TestTypes_rt_" )
            g_strInstName += strInstId;
        
        ret = InitContext();
        if( ERROR( ret ) )
        {
            OutputMsg( ret, "Error InitContext" );
            break;
        }
        if( g_bKProxy )
        {
            ret = KProxyLoop();
            break;
        }

        CRpcServices* pSvc = nullptr;
        InterfPtr pIf;
        do{
            CParamList oParams;
            oParams.Clear();
            oParams[ propIoMgr ] = g_pIoMgr;
            
            ret = CRpcServices::LoadObjDesc(
                strDesc, "TestTypesSvc",
                false, oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            ret = pIf.NewObj(
                clsid( CTestTypesSvc_CliImpl ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            pSvc = pIf;
            ret = pSvc->Start();
            if( ERROR( ret ) )
            {
                OutputMsg( ret, "Error start the proxy" );
                break;
            }
            while( pSvc->GetState()== stateRecovery )
                sleep( 1 );
            
            if( pSvc->GetState() != stateConnected )
            {
                ret = ERROR_STATE;
                break;
            }
        }while( 0 );
        
        if( SUCCEEDED( ret ) )
            ret = maincli( pIf, argc, argv );
            
        // Stopping the object
        if( !pIf.IsEmpty() )
            pIf->Stop();
    }while( 0 );

    DestroyContext();
    return ret;
}

//-----Your code begins here---

std::atomic< int > idx( 0 );
std::atomic< int > count( 1000 );
std::atomic< int > failures( 0 );
gint32 SyncEcho( CTestTypesSvc_CliImpl* pIf )
{
    gint32 ret = 0;
    for( int i = 0; i < 1000; i++ )
    {
        stdstr strResp;
        ret = pIf->Echo( "Hello, World", strResp );
        if( ERROR( ret ) )
            break;
        OutputMsg( ret,
            "Server resp( %d ): %s", i, strResp.c_str() );
    }
    return ret;
}

gint32 AsyncEcho( CTestTypesSvc_CliImpl* pIf )
{
    gint32 ret = 0;
    do{
        CParamList oParams;
        // counters shared with the callback
        oParams.Push( ( intptr_t )&idx );
        oParams.Push( ( intptr_t )&count );
        oParams.Push( ( intptr_t )&failures );

        // synchronization task between this function
        // and the callback
        TaskletPtr pSyncTask;
        ret = pSyncTask.NewObj(
            clsid( CSyncCallback ) );
        if( ERROR( ret ) )
            break;
        oParams.Push( ObjPtr( pSyncTask ) );

        for( int i = 0; i < 1000; i++ )
        {
            stdstr strResp;
            stdstr strText = "Hello, World ";
            strText += std::to_string( i );
            ret = pIf->Echo3( oParams.GetCfg(),
                strText, strResp );
            if( ret == STATUS_PENDING )
                continue;
            count--;
            if( ERROR( ret ) )
            {
                OutputMsg( ret,
                    "Error sending Echo3 request, ( %d )", i );
            }
            else
            {
                OutputMsg( ret,
                    "Succeeded Echo3 request "
                    "immediately, ( %d ), %s",
                    i, strResp.c_str() );
            }
        }

        if( count == 0 )
            break;

        CSyncCallback* pSync = pSyncTask;
        // the requests are sent, wait for responses
        ret = pSync->WaitForCompleteWakable();

    }while( 0 );

    return ret;
}

gint32 maincli( CTestTypesSvc_CliImpl* pIf, int argc, char** argv )
{
    gint32 ret = 0;
    if( g_bAsync )
        ret = AsyncEcho( pIf );
    else
        ret = SyncEcho( pIf );
    if( SUCCEEDED( ret ) )
        OutputMsg( ret,
            "Echo test is completed");
    else
        OutputMsg( ret,
            "Echo test is failed");
    return ret;
}
