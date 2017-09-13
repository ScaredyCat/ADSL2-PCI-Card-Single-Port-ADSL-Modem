clean:
	$(MAKE) -C ./ixml clean
	$(MAKE) -C ./threadutil/src clean
	$(MAKE) -C ./threadutil clean
	$(MAKE) -C ./upnp clean
	$(MAKE) -C ./upnp/sample/tvctrlpt/linux clean
	$(MAKE) -C ./upnp/sample/tvdevice/linux clean
	$(MAKE) -C ./upnp/src/gena clean
	$(MAKE) -C ./upnp/src/genlib/client_table clean
	$(MAKE) -C ./upnp/src/genlib/miniserver clean
	$(MAKE) -C ./upnp/src/genlib/net/http clean
	$(MAKE) -C ./upnp/src/genlib/net clean
	$(MAKE) -C ./upnp/src/genlib/service_table clean
	$(MAKE) -C ./upnp/src/genlib/util clean
	$(MAKE) -C ./upnp/src/urlconfig clean
	@if [ -f "./ixml/src/libixml.a" ]; then rm ./ixml/src/libixml.a; fi
	@if [ -f "./threadutil/src/libthreadutil.a" ]; then rm ./threadutil/src/libthreadutil.a; fi
	@if [ -f "./upnp/bin/libupnp.a" ]; then rm ./upnp/bin/libupnp.a; fi
	@if [ -f "./upnp/inc/FreeList.h" ]; then rm  -f ./upnp/inc/FreeList.h; fi
	@if [ -f "./upnp/inc/LinkedList.h" ]; then rm -f ./upnp/inc/LinkedList.h; fi
	@if [ -f "./upnp/inc/ThreadPool.h" ]; then rm -f ./upnp/inc/ThreadPool.h; fi
	@if [ -f "./upnp/inc/TimerThread.h" ]; then rm -f ./upnp/inc/TimerThread.h; fi
	@if [ -f "./upnp/inc/iasnprintf.h" ]; then rm -f ./upnp/inc/iasnprintf.h; fi
	@if [ -f "./upnp/inc/ithread.h" ]; then rm -f ./upnp/inc/ithread.h; fi
	@if [ -f "./upnp/inc/ixml.h" ]; then rm -f ./upnp/inc/ixml.h; fi
	@if [ -d "./upnp/inc/inc" ]; then rm -rf ./upnp/inc/inc; fi
	@if [ -d "./threadutil/src/.depsrm" ]; then -rf ./threadutil/src/.deps; fi
	@if [ -f "./threadutil/src/libthreadutil_dbg.so" ]; then rm -f ./threadutil/src/libthreadutil_dbg.so; fi
#	@if [ -d "./upnp/lib" ]; then rm -rf ./upnp/lib; fi
	