# Generated by RIDLC, make sure to backup before recompile
# ridlc -spf -O . ../../../../examples/testypes.ridl 
import sys
from rpcf import iolib
from rpcf import serijson
import errno
from rpcf.proxy import ErrorCode as Err
from typing import Union, Tuple, Optional
import os
import time
import threading
import select
from TestTypesSvcsvr import CTestTypesSvcServer

class SvrReqThread(threading.Thread):
    def __init__(self , threadName, oSvrs ):
        super(SvrReqThread,self).__init__(name=threadName)
        self.m_oSvrs = oSvrs
        self.m_bExit = False
        self.m_oLock = threading.Lock()
    
    def IsExiting( self ) -> bool:
        self.m_oLock.acquire()
        bExit = self.m_bExit
        self.m_oLock.release()
        return bExit; 
    
    def SetExit( self ) -> bool:
        self.m_oLock.acquire()
        self.m_bExit = True
        self.m_oLock.release()
    
    
    def run(self):
        return self.handleReqs()
    
    def handleReqs( self ):
        try:
            fps = []
            fMap = dict()
            oSvr = self.m_oSvrs[ 0 ]
            fp = oSvr.getReqFp()
            fps.append( fp )
            fMap[ fp.fileno() ] = 0

            while True:
                ioevents = select.select( fps, [], [], 1 )
                readable = ioevents[ 0 ]
                for s in readable:
                    ret = iolib.recvMsg( s )
                    if ret[ 0 ] < 0:
                        raise Exception( "Error read @%d" % s.fileno() )
                    for oMsg in ret[ 1 ] :
                        idx = fMap[ s.fileno() ]
                        self.m_oSvrs[ idx ].DispatchMsg( oMsg )
                bExit = self.IsExiting()
                if bExit: 
                    break
        except Exception as err:
            print( err )
            return
        
def Usage() :
    print( "Usage: python3 mainsvr.py < SP1 Path > < SP2 Path >..." )
    print( "\t< SP1 path > is the path to the first service's service point. The order" )
    print( "\tof the < SP path > is the same as services declared in the ridl file" )
    
def mainsvr() :
    error = 0
    oSvrThrd = None
    try:
        oSvrs = []
        if len( sys.argv ) < 2 :
            Usage()
            return -errno.EINVAL
        num = 0
        if len( sys.argv ) >= 3 :
            num = int( sys.argv[ 2 ] )
        '''
        Note: this is a reference design
        you are encouraged to make changes
        for your own purpse
        '''
        
        print( "Start to work here..." )
        strSvcPt = sys.argv[ 1 ]
        oSvr = CTestTypesSvcServer(
            strSvcPt, num )
        oSvrs.append( oSvr )
        
        oSvrThrd = SvrReqThread( "TestTypesSvrThrd", oSvrs )
        oSvrThrd.start()
        
        '''
        adding your code here
        '''
        counter = 0
        while True:
            oSvr.OnHelloWorld( "hello, proxy! " + str( counter ) )
            counter += 1
            time.sleep( 5 )
            
    except Exception as err:
        print( err )
        if error < 0 :
            return error
        return -errno.EFAULT
    finally:
        if oSvrThrd is not None:
            oSvrThrd.SetExit()
            threading.Thread.join( oSvrThrd )
    return 0
    
ret = mainsvr()
quit( ret )
