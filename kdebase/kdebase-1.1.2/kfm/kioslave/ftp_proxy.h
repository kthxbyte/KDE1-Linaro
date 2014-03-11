// File: ftp_proxy.cpp by Martin Zumkley (uc2n@rzstud1.rz.uni-karlsruhe.de)

#ifndef _ftp_proxy_h
#define _ftp_proxy_h "$Id: ftp_proxy.h,v 1.4 1999/01/09 22:58:20 garbanzo Exp $"

#include "http.h"

class KProtocolProxyFTP :public KProtocolHTTP
{
    Q_OBJECT
 protected:
    virtual int OpenProxy(KURL *url, int mode, bool reload);

 public:
    KProtocolProxyFTP();
    ~KProtocolProxyFTP();

    
    virtual int OpenDir(KURL *url);    
    virtual int Open(KURL *url, int mode);
    virtual int ReOpen(KURL *url, int mode);
    virtual int ProcessHeader();
    virtual bool isHTML(){
	    if ( strcmp(url.right(1),"/") == 0 )
		return 1;
	    else
		return 0;
	};
	
    // EmitData liest die Antwort vom Proxy
    virtual void EmitData( KIOSlaveIPC* );
};

#endif
