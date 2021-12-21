#Generated by RIDLC, make sure to backup before running RIDLC again
from typing import Tuple
from rpcf.rpcbase import *
from rpcf.proxy import *
from seribase import CSerialBase
from evteststructs import *
import errno

from EventTestsvr import CEventTestServer
import os
import time

def mainsvr() :
    ret = 0
    oContext = PyRpcContext( 'PyRpcServer' )
    with oContext :
        print( "start to work here..." )
        strPath_ = os.path.dirname( os.path.realpath( __file__ ) )
        strPath_ += '/evtestdesc.json'
        oServer = CEventTestServer( oContext.pIoMgr,
            strPath_, 'EventTest' )
        ret = oServer.GetError()
        if ret < 0 :
            return ret
        
        with oServer :
            '''
            made change to the following code
            snippet for your own purpose
            '''
            while ( cpp.stateConnected ==
                oServer.oInst.GetState() ):
                time.sleep( 3 )
                oServer.OnHelloWorld( None, "Hello, World!")
            
    return ret
    
ret = mainsvr()
quit( ret )
