#Generated by RIDLC, make sure to backup before running RIDLC again
from typing import Tuple
from rpcf.rpcbase import *
from rpcf.proxy import *
from seribase import CSerialBase
import errno

from StreamSvcsvrbase import *
from examplestructs import *

def BuildFF( fileName : str ) :
    fi = FILE_INFO()
    fi.szFileName = fileName
    fi.fileSize = 123
    fi.bRead = True
    fi.fileHeader.extend(
        ( fileName + "Header" ).encode() )
    listStrs = list()
    listStrs.append( "alem00" );
    listStrs.append( "alem01" );
    fi.vecLines.append( listStrs )
    listStrs1 = list()
    listStrs1.append( "alem10" );
    listStrs1.append( "alem11" );
    fi.vecLines.append( listStrs1 )

    val = bytearray()
    val.extend( "bibi".encode() )
    fi.vecBlocks[ 0 ] = val
        
    val1 = bytearray()
    val1.extend( "nono".encode() )
    fi.vecBlocks[ 1 ] = val1

    return fi

class CIFileTransfersvr( IIFileTransfer_SvrImpl ):

    '''
    Asynchronous request handler
    '''
    def UploadFile( self, callback : cpp.ObjPtr,
        szFilePath : str,
        hChannel : int,
        offset : int,
        size : int
        ) -> Tuple[ int, None ] :

        self.curTest = 1
        self.curSize = size
        self.numTest += 1
        self.totalSize = size

        return [ 0, None ]
        
    '''
    This method is called when the async
    request is cancelled due to timeout
    or user request. Add your own cleanup
    code here
    '''
    def OnUploadFileCanceled( self,
        callback : cpp.ObjPtr, iRet : int,
        szFilePath : str,
        hChannel : int,
        offset : int,
        size : int ):

        self.curTest = 0
        self.curSize = 0
        self.totalSize = 0
        
    '''
    Asynchronous request handler
    '''
    def GetFileInfo( self, callback : cpp.ObjPtr,
        szFileName : str,
        bRead : int
        ) -> Tuple[ int, list ] :
        '''
        parameters in the response includes
        fi : FILE_INFO
        '''
        #Implement this method here
        return [ ErrorCode.ERROR_NOT_IMPL, None ]
        
    '''
    This method is called when the async
    request is cancelled due to timeout
    or user request. Add your own cleanup
    code here
    '''
    def OnGetFileInfoCanceled( self,
        callback : cpp.ObjPtr, iRet : int,
        szFileName : str,
        bRead : int ):
        pass
    '''
    Asynchronous request handler
    '''
    def DownloadFile( self, callback : cpp.ObjPtr,
        szFileName : str,
        hChannel : int,
        offset : int,
        size : int
        ) -> Tuple[ int, None ] :

        self.curTest = 2
        self.curSize = size
        self.numTest += 1
        self.totalSize = size

        return [ 0, None ]
        
    '''
    This method is called when the async
    request is cancelled due to timeout
    or user request. Add your own cleanup
    code here
    '''
    def OnDownloadFileCanceled( self,
        callback : cpp.ObjPtr, iRet : int,
        szFileName : str,
        hChannel : int,
        offset : int,
        size : int ):

        self.curTest = 0
        self.curSize = 0
        self.totalSize = 0
        
    
class CIChatsvr( IIChat_SvrImpl ):

    '''
    Synchronous request handler
    '''
    def SpecifyChannel( self, callback : cpp.ObjPtr,
        hChannel : int
        ) -> Tuple[ int, list ] :
        '''
        parameters in the response includes
        fis : STRMATRIX
        '''
        if hChannel == 0 :
            print( "hChannel is zero" )
            return [ -errno.EINVAL, None ]

        self.hChannel = hChannel
        self.curTest = 0
        fis = dict()
        fis[ "hello" ] = BuildFF( "SpecifyChannel" )
        #Implement this method here
        return [ 0, [ fis, ] ]
        
    
class CStreamSvcServer(
    CIFileTransfersvr,
    CIChatsvr,
    PyRpcServer ) :
    def __init__( self, pIoMgr, strDesc, strObjName ) :
            PyRpcServer.__init__( self,
                pIoMgr, strDesc, strObjName )
            self.hChannel = 0
            self.curTest = 0
            self.curSize = 0
            self.numTest = 0
            self.totalSize = 0

    def AcceptNewStream( self, pDataDesc: cpp.ObjPtr ):
        try:
            print( "AccpetNewstream ", pDataDesc )
        except:
            return -errno.EFAULT
        #return ErrorCode.STATUS_SUCCESS for
        #acception
        return 0

    def OnStmReady( self, hChannel ) :
        print( "OnStmReady ",
            hChannel, self.hChannel )
        #notify client we are ready
        self.WriteStream( hChannel, "RDY" )

    def OnStmClosing( self, hChannel ) :
        if self.hChannel == hChannel :
            print( "OnStmClosing ", hChannel )
            self.hChannel = 0
        else :
            print( self.hChannel, hChannel )
