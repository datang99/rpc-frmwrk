'''This test case demonstrates 
    . Double-direction big data transfer via
    streams

    . How to enable python's built-in
    serialization

    . Multiple interfaces support

    . Note: this server is dedicated for python
    client due to python specific serilization
'''
import sys
import time
import numpy as np
import threading as tr
import getpass
import errno
import os
import io
from rpcfrmwrk import *

import types
sys.path.insert(0, '../../')
from proxy import PyRpcContext, PyRpcProxy
from proxy import ErrorCode as EC
from typing import Union

from sfcommon import CTransContext, CFileInfo
from sfcommon import PyFileTransfer, PyFileTransferBase

# define the RPC interface to support
class CEchoClient:
    """mandatory class member to define the
    interface name, which will be used to invoke
    the event handler if any
    """
    ifName = "CEchoServer"

    def Echo(self, text ):
        return self.sendRequest(
            CEchoClient.ifName, "Echo", text )

# or inherit an RPC interface, and implement the
# RPC methods
class PyFileTransClient( PyFileTransferBase ):

    def __init__(self ) :
        PyFileTransferBase.__init__( self )
        self.sem = tr.Semaphore( 0 )

    def OnTransferDone( self, hChannel ) :
        PyFileTransferBase.OnTransferDone(
            self, hChannel )
        self.NotifyComplete()

    def WaitForComplete( self ) :
        self.sem.acquire()

    def NotifyComplete( self ) :
        self.sem.release()
        
    ''' rpc method impl
    '''
    def UploadFile( self,
        fileName:   str,
        chanHash:   np.uint64, 
        offset:     np.uint64,
        size:       np.uint64 )->[ int, list ]:

        resp = self.PySendRequest(
            PyFileTransClient.ifName, "UploadFile",
            fileName, chanHash,
            offset, size )

        return resp

    ''' rpc method impl
    '''
    def GetFileInfo( self,
        fileName : str,
        bRead : bool = True ) -> [ int, list ] :
        return self.PySendRequest(
           PyFileTransClient.ifName, "GetFileInfo", 
            fileName, bRead )

    ''' rpc method impl
    '''
    def DownloadFile( self,
        fileName:   str,
        chanHash:   np.uint64,
        offset:     np.uint64,
        size:       np.uint64 )->[ int, list ]:

        resp = self.PySendRequest(
           PyFileTransClient.ifName, "DownloadFile", 
           fileName, chanHash, offset, size )

        return resp

    '''OnStmReady is a system defined event
    handler, called when the stream channel is
    ready to send/receive. There must be an
    implementation of OnStmReady for the server to
    support streaming related things.
    '''
    def OnStmReady( self, hChannel ) :
        self.SetTransCtx( hChannel,
            CTransContext() )
        self.NotifyComplete()

    '''OnStmClosing is a system defined event
    handler, called when the stream channel has
    been closed by the peer, or this proxy/server
    will shutdown, or actively calls `CloseStream'
    '''
    def OnStmClosing( self, hChannel ) :
        self.OnTransferDone( hChannel )
        self.mapChannels.pop( hChannel )

    def DoUploadFile( self,
        fileName:   str,
        hChannel:   np.uint64, 
        offset: np.uint64=0,
        size :  np.uint64=0 )->[ int, list ]:
        resp = [ EC.STATUS_SUCCESS, list() ]
        while True :
            if not os.access( fileName, os.R_OK ) :
                resp[ 0 ] = -errno.EACCES
                break

            if hChannel not in self.mapChannels :
                resp[ 0 ] = -errno.ENOENT
                break

            oCtx = self.mapChannels[ hChannel ]

            if oCtx.fp is not None:
                resp[ 0 ] = -errno.EBUSY
                break

            try:
                fp = open( fileName, "rb+" )
                iSize = fp.seek( 0, os.SEEK_END )
                if iSize == 0 :
                    resp[ 0 ] = -errno.ENODATA
                    break
                if iSize < offset + size :
                    resp[ 0 ] = -errno.ERANGE
                    break
                fp.seek( offset, os.SEEK_SET )
                if size == 0 :
                    size = iSize

            except OSError as err : 
                print( "Error open file", err )
                resp[ 0 ] = -err.errno
                break

            if resp[ 0 ] < 0 :
                break

            ret = self.oInst.GetPeerIdHash( hChannel )
            if ret[ 0 ] < 0 :
                resp[ 0 ] = ret[ 0 ]
                break;

            chanHash = ret[ 1 ]
            ret = self.UploadFile( fileName,
                chanHash, offset, size )
            if ret[ 0 ] < 0 :
                resp[ 0 ] = ret[ 0 ]
                break;

            oCtx.fp = fp
            oCtx.iOffset = offset
            oCtx.iSize = size
            oCtx.iBytesLeft = size
            oCtx.byDir = 'u'
            oCtx.strPath = fileName

            '''start the transfer a bit later
            '''
            ret = self.DeferCall(
                self.ReadFileAndSend, hChannel )
            if ret < 0 :
                resp[ 0 ] = ret;
            else :
                '''transfer will start immediately
                and complete this request with
                success
                '''
                resp[ 0 ] = EC.STATUS_PENDING

            break

        return resp

    def DoDownloadFile( self,
        fileName:   str,
        hChannel:   np.uint64,
        offset:     np.uint64,
        size:       np.uint64 )->[ int, list ]:
        resp = [ 0, list() ]
        while True :
            if not os.access( fileName, os.W_OK ) :
                resp[ 0 ] = -errno.EACCES
                break

            oCtx = self.mapChannels[ hChannel ]
            if oCtx is None :
                resp[ 0 ] = -errno.ENOENT
                break;

            if size == 0 :
                resp[ 0 ] = -errno.EINVAL
                break;

            if oCtx.fp is not None:
                resp[ 0 ] = -error.EBUSY
                break
            
            try:
                fp = open( fileName, "wb+" )
                fp.truncate( offset )
                fp.seek( offset, os.SEEK_SET ) 

            except OSError as err : 
                resp[ 0 ] = -err.errno
                break

            if resp[ 0 ] < 0 :
                break

            ret = self.oInst.GetPeerIdHash( hChannel )
            if ret[ 0 ] < 0 :
                resp[ 0 ] = ret[ 0 ]
                break;

            chanHash = ret[ 1 ]
            ret = self.DownloadFile( fileName,
                chanHash, offset, size )
            if ret[ 0 ] < 0 :
                resp[ 0 ] = ret[ 0 ]
                break;

            oCtx.fp = fp
            oCtx.iOffset = offset
            oCtx.iSize = size
            oCtx.iBytesLeft = size
            oCtx.byDir = 'd'
            oCtx.strPath = fileName

            '''start the transfer a bit later
            '''
            pBuf = bytearray()
            ret = self.DeferCall(
                self.WriteFileAndRecv,
                hChannel, pBuf )
            if ret < 0 :
                resp[ 0 ] = ret;
            else :
                '''transfer will start immediately
                Notify the caller with an PENDING
                status
                '''
                resp[ 0 ] = EC.STATUS_PENDING

            break

        return resp

class PyFileTransProxy(
    CEchoClient, PyFileTransClient, PyRpcProxy):

    def __init__(self, pIoMgr, strCfgFile, strObjName) :
        PyRpcProxy.__init__( self,
            pIoMgr, strCfgFile, strObjName )
        PyFileTransClient.__init__( self )
        CEchoClient.__init__( self )


def test_main() : 
    while( True ) :
        '''create the transfer context, and start
        it'''
        oContext = PyRpcContext();
        ret = oContext.Start();
        if ret < 0 :
            break;

        '''create the server object, and start
        it'''
        print( "start to work here..." )
        oProxy = PyFileTransProxy( oContext.pIoMgr,
            "./sfdesc.json",
            "PyFileTransSvr" )

        ret = oProxy.GetError() 
        if ret < 0 :
            break

        ret = oProxy.Start()
        if ret < 0 :
            break

        '''to setup a stream channel between the
        client and the server '''
        hChannel = oProxy.StartStream() 
        if hChannel == 0 :
            ret = EC.ERROR_FAIL
            break;

        '''Wait till OnStmReady notifies.
        '''
        oProxy.WaitForComplete()

        '''Check if server is ready
        '''
        tupRet = oProxy.ReadStream( hChannel )
        ret = tupRet[ 0 ]
        if ret < 0 :
            break
        
        pBuf = tupRet[ 1 ].decode(
            sys.stdout.encoding )

        if not pBuf[ :3 ] == "rdy" :
            ret = EC.ERROR_FAIL
            break

        ''' upload a file
        '''
        print( "Uploading file..." )
        tupRet = oProxy.DoUploadFile(
            "./f100M.dat", hChannel )
         
        ret = tupRet[ 0 ]
        if ret < 0 :
            break

        '''Wait till OnTransferDone notify
        '''
        oProxy.WaitForComplete()

        '''confirm by check if server send an
        "over" token
        '''
        tupRet = oProxy.ReadStream( hChannel )
        ret = tupRet[ 0 ]
        if ret < 0 :
            break
        
        pBuf = tupRet[ 1 ].decode(
            sys.stdout.encoding )

        if not pBuf[ :4 ] == "over" :
            ret = EC.ERROR_FAIL
            break
        
        ''' Download a file
        '''
        print( "Downloading file..." )
        tupRet = oProxy.GetFileInfo( "f100M.dat" )
        ret = tupRet[ 0 ]
        if ret < 0 :
            break

        fileInfo = tupRet[ 1 ][ 0 ] 
        fileSize = fileInfo.size

        tupRet = oProxy.DoDownloadFile(
            "f100M.dat", hChannel, 0, fileSize )

        ret = tupRet[ 0 ]
        if ret < 0 :
            break

        oProxy.WaitForComplete()

        break #while True

    ''' Stop the guys'''
    if oProxy is not None :
        oProxy.Stop();

    if oContext is not None:
        oContext.Stop()

    return ret;


ret = test_main()
quit( ret )

