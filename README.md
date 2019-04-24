# rpc-frmwrk
An effort for embedded RPC framework, and hope it could find its usage in IOT or other embedded systems.   
#### Dependency:  
This framework depends on the following packags:  
1. `dbus-1.0 (dbus-devel.i686)`  
2. `libjson-cpp (jsoncpp-devel.i686)`  
3. `cppunit-1 (for the sample code, cppunit.i686 and cppunit-devel.i686)`   
4. `glib-2.0 (for compile only,glib2-devel.i686)`   
#### Features:   
1. `Multiple interfaces on a single object (COM alike).`   
2. `Synchronous/Asynchronous request and handling.`   
3. `Active canceling.`   
4. `Server side event broadcasting.`   
5. `Keep-alive for time-consuming request.`   
6. `Pausable/Resumable interface.`
7. `Support RPC from remote machine, local system, and in-process.` 
8. `Transparent support for different kinds of remote communications.`
9. `File/Big data transfer.`
10. `Peer online/offline awareness.`
11. `Streaming support to provide double-direction stream transfer`

---
[`Wed Apr 24 18:10:46 CST 2019`]   
1. The `bridge` side is now in a good shape now. Next I will take some time to clear some of the `forwarder` side bugs, before moving on to run the helloworld over the tcp connection.   

[`Sun Apr 21 12:36:37 CST 2019`]   
1. There is one last memory leak to fix yet. After done, we can wrap up the `EnableRemoteEvent` and move on to debugging client requests.   

[`Thu Apr 11 14:06:43 CST 2019`]   
1. Fixed some fatal memory leaks and concurrency bugs. Hopefully, the biggest memory leak will be fixed in the next check-in. But the progress is a bit lagging :(    

[`Wed Mar 27 17:56:33 CST 2019`]   
1. Fixed some bugs in the code. There are still some memory leak and concurrency bugs to fix in the `EnableRemoteEvent` and connection setup.

[`Thu Mar 14 20:47:45 CST 2019`]   
1. The `EnableRemoteEvent` works on the `Forwarder` side. and The bridge side has successfully received the request. Move on to get the `Bridge` side `EnableRemoteEvent` to work. This should be the last part work before getting through the whole request/response path.   

[`Thu Mar 07 23:35:20 CST 2019`]   
1. Fixed the memory leak issues. Move on the get the CRpcControlStream to work.   

[`Tue Mar 05 14:05:40 CST 2019`]   
1. Encountered and fixed some bugs in the `CRpcRouter`. There are still some memory leaks to fix. One known bug is that, the `OpenRemotePort` and `CloseRemotePort` need to be sequentially executed rather than in random order. Otherwise, the `CRpcTcpBridgeProxyImpl` and the `CTcpStreamPdo` may not be able to be released properly.   
2. After the leaks are fixed, I will get `EnableRemoteEvent` to work.   

[`Sun Feb 24 14:52:28 CST 2019`]   
1. This week, I had some emergency thing to handle. I will get back next week.   

[`Sat Feb 16 17:35:54 CST 2019`]   
1. `OpenRemotePort` is done now. The CDBusProxyPdo and CDBusProxyFdo are found lack of the `online/offline` handler for both remote server and remote module, and need to add some code next week. And after that, we can move on to debug `EnableEvent` related stuffs.   

[`Thu Feb 07 21:17:48 CST 2019`]   
1. Still debugging the CRpcRouter. The `OpenRemotePort` is almost completed now. And there still need a disconnection handler on the `reqfwdr` side to clean up the context for the unexpectedly disconnected proxy.   

[`Thu Jan 24 17:03:14 CST 2019`]   
1. Continued to refact part of the CRpcRouter. Now I expect there are some trivial patches need to be done in the coming days of this week before the refact task is done.   

[`Sat Jan 19 23:43:28 CST 2019`]   
1. The redesign and refact work are almost done. Next week we can start the debugging of RPC module hopefully.   

[`Fri Jan 11 08:32:13 CST 2019`]   
1. There are several places in the router to redesign for new requirements/discoveries. And the details have appended to the todo.txt.   

[`Tue Jan 01 20:53:12 CST 2019`]   
1. This week, I need to redesign the management part of the router for the bridge and reqfwdr's lifecycle management, and add new command to the tcp-level protocol for connection resiliance.   
2. Note that, if you want to run `helloworld`, please use the earlier version `bf852d55ae2342c5f01cc499c59885f50780c550` of `echodesc.json` instead. The latest version is modified for RPC debugging purpose.

[`Tue Dec 18 21:35:54 CST 2018`]   
1. Made the proxy ports loaded and work. Next step is to get the req-forwarder to run in a stand-alone process.
2. We have two approaches to deploy the proxy, the compact way, that is, the proxy and the req-forwarder works in the same process, and the normal way, that an rpc-router process runs as a router for all the proxys system-wide. I will first make the normal way work. The significance of compact way is to have better security in communication than the normal way if the device is to deploy in a less trustworthy environment.

[`Fri Dec  7 22:45:23 CST 2018`]
1. Finally, I have wrapped up local IPC testing. Now I will proceed to RPC debugging and testing.

[`Mon Dec 03 17:47:55 CST 2018`]
1. Added local stream support. It is yet to debug.   

[`Tue Nov 20 13:02:20 CST 2018`]
1. Added message filter support and multiple-interface support. Sorry for not starting the RPC debugging still, because I need to dig more in streaming support which will affect RPC module a bit.   

[`Wed Nov 14 11:24:42 CST 2018`]
1. Local `Send/Fetch` is done. And now it is time to move on to the RPC module. For security purpose, I will add the authentication interface sometime after the RPC module is done.   

[`Fri Nov  2 18:24:51 CST 2018`]   
1. This week, the `keep-alive` and `Pause/Resume` works.
2. Next week, I will try to get `Send/Fetch file` to work, before I can move on to RPC module.   
   
[ `Tue Oct 23 19:59:39 CST 2018` ]   
1. This week, I will get keep-alive to work and move on to make RPC module to work.   
   
[ `Mon Oct 15 18:51:48 CST 2018` ]  
1. The dependency of `g_main_loop` from `glib-2.0` is replaced with a simplified `mainloop`.   
2. the glib headers are still needed at compile time. But the glib shared libraries are not required at deploy time.   
   
[ `Thu Oct  4 12:24:00 CST 2018` ]
1. After some tests, I cannot find a way to put `libev` in the concurrent environment flawlessly.
So `libev` is not an option any more. [(Reason)](https://github.com/zhiming99/rpc-frmwrk/wiki/Why-libev-cannot-be-used-in-rpc-frmwrk%3F)  
2. I will write a simple mainloop with `poll` to fix it.   

