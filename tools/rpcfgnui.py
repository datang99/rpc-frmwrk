import json
import os
import sys 
from shutil import move
from copy import deepcopy
from urllib.parse import urlparse
from typing import Dict
import errno
from updcfg import *

if len( sys.argv ) < 2 :
    print( "Usage: python3 rpcfg.py <init file>" )
    print( "\t<init file> can be generated by <SaveAs>" )
    print( "\tfrom rpcfgui.py on a host with GUI" )
    quit( -errno.EINVAL )

ret = 0
initFile = sys.argv[ 1 ]
try:
    ret = Update_InitCfg( initFile, True, None )
except Exception as err:
    text = "Failed to update the config files:" + str( err )
    exc_type, exc_obj, exc_tb = sys.exc_info()
    fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
    second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
    print( text, second_text )
    ret = -errno.EFAULT

quit( ret )
