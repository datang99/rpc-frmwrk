import json
import os
import sys
import errno
import getopt

def usage():
    print( "Description: to update the initcfg.json on the target system" )
    print( "Usage: python3 updinitcfg.py [options] <dest dir> <init file>" )
    print( "\t<key dir> the directory storing the key files" )
    print( "\t<init file> initcfg file to update" )
    print( "\t-c this is a client host" )
    print( "\t-h print this help" )

def upd_initcfg( strKeyDir : str, cfgPath : str, bServer : bool ):
    ret = 0
    try:
        fp = open( cfgPath, "r" )
        initCfg = json.load( fp )
        fp.close()

        oSecurity = initCfg[ "Security" ]
        sslFiles = oSecurity[ "SSLCred" ]
        bCfgSvr = False
        if initCfg[ "IsServer" ] == 'true' :
            bCfgSvr = True
        bGmSSL = False
        if 'UsingGmSSL' in sslFiles:
            usingGmSSL = sslFiles[ "UsingGmSSL" ]
            if usingGmSSL == "true" :
                bGmSSL = True

        if bServer == bCfgSvr :
            strVal = sslFiles[ "KeyFile"]
            if len( strVal ) > 0:
                sslFiles[ "KeyFile"] = \
                    strKeyDir + "/" + os.path.basename( strVal )

            strVal = sslFiles[ "CertFile"]
            if len( strVal ) > 0:
                sslFiles[ "CertFile"] = \
                    strKeyDir + "/" + os.path.basename( strVal )

            if 'CACertFile' in sslFiles :
                strVal = sslFiles[ "CACertFile"]
                if len( strVal ) > 0:
                    sslFiles[ "CACertFile"] = \
                        strKeyDir + "/" + os.path.basename( strVal )
            if 'SecretFile' in sslFiles :
                strVal = sslFiles[ "SecretFile"]
                if len( strVal ) > 0 and \
                    strVal != "console" and strVal != "1234":
                    sslFiles[ "SecretFile"] = \
                        strKeyDir + "/" + os.path.basename( strVal )
        elif not bServer :
            strVal = sslFiles[ "KeyFile" ]
            keyName = os.path.basename( strVal )
            if keyName == "signkey.pem" :
                '''generated by rpcfg'''
                sslFiles[ "KeyFile"] = strKeyDir + "/clientkey.pem"
                sslFiles[ "CertFile"] = strKeyDir + "/clientcert.pem"
                if bGmSSL:
                    sslFiles[ "CACertFile"] = strKeyDir + "/rootcacert.pem"
                else:
                    sslFiles[ "CACertFile"] = strKeyDir + "/certs.pem"
            else :
                raise Exception( "error bad initcfg.json" )
        else:
            raise Exception( "error bad initcfg.json" )

        fp = open( cfgPath, "w" )
        json.dump( initCfg, fp, indent=4)
        fp.close()

    except Exception as err:
        text = "Failed to update the initcfg.json:" + str( err )
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
        print( text, second_text )
        ret = -errno.EFAULT

    return ret

def main():
    bServer = True
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hc" )
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(-errno.EINVAL)

    for o, a in opts:
        if o == "-h" :
            usage()
            sys.exit( 0 )
        elif o == "-c":
            bServer = False
        else:
            assert False, "unhandled option"
 
    if len( args ) < 2 :
        usage()
        sys.exit( -errno.EINVAL )
    

    ret = 0
    keyDir = args[ 0 ]
    cfgPath = args[ 1 ]

    ret = upd_initcfg( keyDir, cfgPath, bServer )
    quit( -ret )

if __name__ == "__main__":
    main()
