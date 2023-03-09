#Generated by RIDLC, make sure to backup before running RIDLC again
from typing import Tuple
from rpcf.rpcbase import *
from rpcf.proxy import *
from seribase import CSerialBase
from stmteststructs import *
import errno

from StreamTestcli import CStreamTestProxy
import os
import time

def maincli() :
    ret = 0
    oContext = PyRpcContext( 'PyRpcProxy' )
    with oContext :
        print( "start to work here..." )
        strPath_ = os.path.dirname( os.path.realpath( __file__ ) )
        strPath_ += '/stmtestdesc.json'
        oProxy = CStreamTestProxy( oContext.pIoMgr,
            strPath_, 'StreamTest' )
        ret = oProxy.GetError()
        if ret < 0 :
            return ret
        

        with oProxy :

            state = oProxy.oInst.GetState()
            while state == cpp.stateRecovery :
                time.sleep( 1 )
                state = oProxy.oInst.GetState()
            if state != cpp.stateConnected :
                return ErrorCode.ERROR_STATE
            '''
            adding your code here
            Calling a proxy method like
            '''
            while True:
                #Echo
                pret = oProxy.Echo( "Hello, stmtest" )
                if pret[ 0 ] < 0:
                    ret = pret[ 0 ]
                    break
                OutputMsg( "Echo completed with response " +
                    pret[ 1 ][ 0 ] )

                hChannel = oProxy.StartStream()
                if hChannel == ErrorCode.INVALID_HANDLE:
                    ret = -errno.EFAULT
                    break

                for i in range(100) :
                    strMsg = "a message to server " + str( i )
                    ret = oProxy.WriteStream(
                        hChannel, strMsg.encode() )
                    if ret < 0:
                        break
                    #read response from server synchronously
                    pret = oProxy.ReadStream( hChannel )
                    if pret[ 0 ] < 0 :
                        ret = pret[ 0 ]
                        break
                    byResp = pret[ 1 ]
                    OutputMsg( "Server says (sync): " + byResp.decode() )
                    dblVal = i + .1
                    strMsg = "a message to server " + str( dblVal )
                    ret = oProxy.WriteStreamAsync2(
                        hChannel, strMsg.encode() )
                    if ret < 0 :
                        break
                    if ret == ErrorCode.STATUS_PENDING :
                        oProxy.WaitForComplete()
                        ret = oProxy.GetError()
                        if ret < 0:
                            break
                
                    pret = oProxy.ReadStreamAsync2( hChannel, 0 )
                    if pret[ 0 ] < 0 :
                        ret = pret[ 0 ]
                        break

                    if pret[ 0 ] == ErrorCode.STATUS_PENDING:
                        oProxy.WaitForComplete()
                        ret = oProxy.GetError()
                        if ret < 0:
                            break
                    else:
                        byResp = pret[ 1 ]
                        OutputMsg( "Server says(async): " + byResp.decode() )
                break
                    
        oProxy = None
    oContext = None
    return ret
try:    
    ret = maincli()
except Exception as e :
    print(str(e))
quit( -ret )
