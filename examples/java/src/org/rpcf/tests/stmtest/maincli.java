// GENERATED BY RIDLC. MAKE SURE TO BACKUP BEFORE RE-COMPILING.
package org.rpcf.tests.stmtest;

import org.rpcf.rpcbase.JRetVal;
import org.rpcf.rpcbase.JavaRpcContext;
import org.rpcf.rpcbase.RC;
import org.rpcf.rpcbase.rpcbase;

import java.nio.charset.StandardCharsets;
import java.util.concurrent.TimeUnit;
public class maincli
{
    public static JavaRpcContext m_oCtx;
    public static String getDescPath( String strName )
    {
        String strDescPath =
            maincli.class.getProtectionDomain().getCodeSource().getLocation().getPath();
        String strDescPath2 = strDescPath + "/org/rpcf/tests/stmtest/" + strName;
        java.io.File oFile = new java.io.File( strDescPath2 );
        if( oFile.isFile() )
            return strDescPath2;
        strDescPath += "/" + strName;
        oFile = new java.io.File( strDescPath );
        if( oFile.isFile() )
            return strDescPath;
        return "";
    }
    public static void main( String[] args )
    {
        m_oCtx = JavaRpcContext.createProxy(); 
        if( m_oCtx == null )
            System.exit( RC.EFAULT );

        int ret = 0;
        StreamTestcli oSvcCli = null;
        do{
            String strDescPath =
                getDescPath( "stmtestdesc.json" );
            if( strDescPath.isEmpty() )
            {
                ret = -RC.ENOENT;
                break;
            }
            // create the service object
            oSvcCli = new StreamTestcli(
                m_oCtx.getIoMgr(),
                strDescPath,
                "StreamTest" );

            // check if there are errors
            if( RC.ERROR( oSvcCli.getError() ) ) {
                ret = oSvcCli.getError();
                break;
            }

            // start the proxy
            ret = oSvcCli.start();
            if( RC.ERROR( ret ) )
                break;
        
            // test remote server is not online
            while( oSvcCli.getState() == RC.stateRecovery )
            try{
                TimeUnit.SECONDS.sleep(1);
            }
            catch( InterruptedException e ){};
            
            if( oSvcCli.getState() != RC.stateConnected )
            { ret = RC.ERROR_STATE;break;}
            
            /*// request something from the server*/
            String i0 = "Hello, stmtest";
            JRetVal jret = oSvcCli.Echo(i0);
            if( jret.ERROR() )
            { ret = jret.getError();break; }
            String i0r = (String)jret.getAt(0);
            rpcbase.JavaOutputMsg(
                    "Echo completed with response " + i0r);
            jret = oSvcCli.startStream(null);
            if(jret.ERROR())
            {
                ret = jret.getError();
                break;
            }
            long hChannel = jret.getAtLong(0);
            for(int i = 0; i < 100; i++)
            {
                String strMsg = String.format("a message to server %d", i);
                ret = oSvcCli.writeStream(
                        hChannel, strMsg.getBytes(StandardCharsets.UTF_8));
                if(RC.ERROR(ret))
                    break;
                jret = oSvcCli.readStream(hChannel);
                if(jret.ERROR())
                {
                    ret = jret.getError();
                    break;
                }
                byte[] byResp = (byte[]) jret.getAt(0);
                rpcbase.JavaOutputMsg(
                        "Server says (sync): " + new String(byResp,StandardCharsets.UTF_8));
                double b = i + .1;
                strMsg = String.format("a message to server %g", b);
                ret = oSvcCli.writeStreamAsync(
                        hChannel,strMsg.getBytes(StandardCharsets.UTF_8));
                if(RC.ERROR(ret))
                    break;
                if(RC.isPending(ret)) {
                    // for simplicity, we don't transmit multiple data blocks simultaneously
                    do {
                        try {
                            oSvcCli.m_sem.acquire();
                            break;
                        } catch (InterruptedException e) {
                            continue;
                        }
                    } while (true);
                    ret = oSvcCli.getError();
                    if (RC.ERROR(ret))
                        break;
                }
                jret = oSvcCli.readStreamAsync(hChannel,0);
                if(jret.ERROR())
                {
                    ret = jret.getError();
                    break;
                }
                if(jret.isPending()) {
                    // for simplicity, we don't issue many request in parallel.
                    do {
                        try {
                            oSvcCli.m_sem.acquire();
                            break;
                        } catch (InterruptedException e) {
                            continue;
                        }
                    } while (true);
                    ret = oSvcCli.getError();
                    if(RC.ERROR(ret))
                        break;
                }
                else
                {
                    byResp = (byte[])jret.getAt(0);
                    rpcbase.JavaOutputMsg(
                            "Server says(async): " + new String(byResp,StandardCharsets.UTF_8));
                }
            }
        }while(false);
        rpcbase.JavaOutputMsg(
                 "Quit with status: " + ret);
        if( oSvcCli != null)
            oSvcCli.stop();
        m_oCtx.stop();
        System.exit(-ret);
    }
}
