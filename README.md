# rpc-frmwrk

这是一个嵌入式的RPC实现，关注于跨网络，跨协议，跨平台的互联互通。本项目欢迎有兴趣的人士加入!   
This is an asynchronous and event-driven RPC implementation for embeded system with small system footprint. It is targeting at the IOT platforms, high-throughput, and high availability over hybrid network. Welcome to join!  

#### Concept
[`Here`](https://github.com/zhiming99/rpc-frmwrk/blob/master/Concept.md) is an introduction about some concepts that help to understand this project.

#### Dependency:  
This framework depends on the following packags to build:  
1. `dbus-1.0 (dbus-devel)`
2. `libjson-cpp (jsoncpp-devel)` 
3. `lz4 (lz4-devel)`   
4. `cppunit-1 (for the sample code, cppunit and cppunit-devel)`   
5. `glib-2.0 (for compile only,glib2-devel)`   
6. `openssl-1.1 for SSL communication. ( openssl-devel, optional )`
7. `Kerberos 5 for authentication and access control. ( rpm: krb5-libs, krb5-devel, or deb: libkrb5-3, libkrb5-dev )`
8. `c++11` is required, and make sure the GCC is 5.x or higher.

#### Features:   
1. `Support for multiple interfaces on a single object (COM alike).`   
2. `Support for synchronous/Asynchronous requests and handling.`   
3. `Active canceling.`   
4. `Server-push events`   
5. `Keep-alive for time-consuming request.`   
6. `Pausable/Resumable interface.`
7. `Support RPC from remote machine, local system, and in-process.` 
8. `Transparent support for different types of remote communications.`
9. `Peer online/offline awareness.`
10. `Streaming support to provide double-direction stream transfer`
11. [`Secure Socket Layer (SSL) support`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/sslport/Readme.md)
12. [`Websocket support`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/wsport/Readme.md)
13. [`Object access via Multihop routing`](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support)
14. [`Authentication support with Kerberos 5`](https://github.com/zhiming99/rpc-frmwrk/tree/master/rpc/security)

#### Building `rpc-frmwrk`:   
Please refer to this article [`How to build rpc-frmwrk`](https://github.com/zhiming99/rpc-frmwrk/wiki/How-to-build-%60rpc-frmwrk%60) for details.

---
[`Tue 15 Sep 2020 11:37:29 AM Beijing`]   
1. Fixed a bunch of bugs. Now the system works stable. After almost 5 months, Kerberos authentication support is finally ready! :) 

[`Fri 11 Sep 2020 08:25:18 PM Beijing`]   
1. Encountered some concurrent bugs, and fixed some of them. There are still some bugs known to fix, which should be less difficult than the ones fixed this week.   

[`Wed 02 Sep 2020 07:30:53 PM Beijing`]   
1. fixed many bugs, and the test case `sftest` has passed. There are still some test cases to go. Kerberos support is almost ready.

[`Tue 25 Aug 2020 08:19:12 PM Beijing`]   
1. `kinit` now works with rpc-frmwrk through the rpc connection, that is, if the kerberos kdc is behind the firewall, and not open to the internet, the client still has a way to get authenticated.   

[`Tue 11 Aug 2020 10:02:46 PM Beijing`]  
1. The test case `iftest` has passed. There should still be some bugs. Bug fixing and tests continues...   

[`Thu 06 Aug 2020 11:02:28 PM Beijing`]   
1. Still debugging the authentication module. And some progress are made. Many disturbing events happened in the last two weeks, such as 30-hour black-out, and buggy system update degraded the computer performance to 800MHz. Fortunately, everything returns to normal now. Hopefully, next week we will see the authentication support able to work.

[`Sun 26 Jul 2020 08:12:16 PM Beijing`]   
1. Submitted the Kerberos support to the main branch. It is not debugged yet. So it surely cannot work for now. And if you want to give a shot to the framework, please try version 0.3. :)   

[`Sun 12 Jul 2020 11:23:27 AM Beijing`]   
1. still battling with some technical difficulties. The "relative simple" design has turned not as simple as it looks. Anyway, authentication support is coming...  

[`Mon 22 Jun 2020 05:40:08 PM Beijing`]   
1. Discarded the old authentication design, and chose a relative simple design to start over now, `Kerberos` authentication.      

[`Fri 12 Jun 2020 04:58:06 PM Beijing`]   
1. Designing is still in process. Details has made the progress slow.   

[`Sun 24 May 2020 04:49:09 PM Beijing`]   
1. It turns out security stuffs never disappoint you by complexity. Still researching on the different authentication approaches. It would probably take another month to get it done.

[`Wed 13 May 2020 05:23:01 PM Beijing`]   
1. Preferable to `File ACL` as the `access control` model.
2. `Kerberos` requires a lot of resources for deployment, and I will also try to add the `NTLM` as a low-cost alternative.

[`Mon 11 May 2020 09:23:20 PM Beijing`]   
1. Trying to use `GSSAPI's Kerberos implementation` as the authentication mechanism. There are still some technical issues to investigate at this point.
2. It is not decided yet whether to use `File ACL` or `RBAC` as the `access control` model.

[`Tue 28 Apr 2020 02:12:02 PM Beijing`]   
1. The next task is to implement `Authentication and access control` for `rpc-frmwrk`.

[`Sun 26 Apr 2020 06:19:24 PM Beijing`]   
1. `Multihop` is almost completed, but testing will continue to cover more test cases.
2. The next to do could be either `Python support` or `Authentication and access control`. Not decided yet. 

[`Fri 24 Apr 2020 08:15:48 PM Beijing`]   
1. Fixed some major bugs. There are still two or three known bugs, but multihop support is near complete.   

[`Tue 07 Apr 2020 11:15:32 AM Beijing`]   
1. fixed some bugs in the multihop code. It should be more stable now. and more testing and tuning needed.

[`Fri 03 Apr 2020 10:03:06 PM Beijing`]   
1. Streaming for multihop almost works. the performance is not ideal yet. More testing and tuning needed.   

[`Tue 31 Mar 2020 10:11:09 PM Beijing`]   
1. still debugging the multihop related stuffs. Request forwarding and event forwarding work now. Next task is to get streaming to work.   

[`Sat 21 Mar 2020 01:47:35 PM Beijing`]   
1. Updated some `autoconf` releated stuffs to make the cross-compile more efficient.

[`Fri 20 Mar 2020 11:00:16 PM Beijing`]   
1. Submitted a compilable version with multihop support. It will take one or two weeks to get it work.   

[`Sat 14 Mar 2020 09:18:01 PM Beijing`]   
1. Code complete the streaming support for multihop, about 70% done.    

[`Mon 02 Mar 2020 05:36:15 PM Beijing`]
1. Splitted the `CRpcRouter` to smaller classes.   
 
[`Sun 23 Feb 2020 11:22:40 AM Beijing`]   
1. Splitting the CRpcRouter class to four smaller classes to allow better management of shared resources.   

[`Sat 15 Feb 2020 12:02:14 AM Beijing`]   
1. Fixed a bug in the streaming interface.
2. Added [`Concept.md`](https://github.com/zhiming99/rpc-frmwrk/blob/master/Concept.md) as an introduction about the `rpc-frmwrk`.

[`Wed 12 Feb 2020 02:09:38 PM Beijing`]   
1. The `WebSocket` support is completed. And the instructions about the usage is updated in the [`Readme.md`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/wsport/Readme.md).   
2. Next task is `multihop` support, the very important feature!   

[`Mon 10 Feb 2020 02:14:10 PM Beijing`]   
1. `Websocket` support is comming. The new update delivers a working version of `Websocket` port. However, the  `transparent proxy traversal` remains to test.   

[`Fri 07 Feb 2020 07:31:31 PM Beijing`]  
1. Merged the `multihop` branch to `master`.   
2. Added the `WebSocket` support, though not working yet.   
3. After the `WebSocket` is done, I will then choose one to implement between `multihop` and `connection recovery`.

[`Sun 02 Feb 2020 08:30:52 PM Beijing`]   
1. Created a new branch `multihop` for the newly added changes. It will take some time to get stable, and then I will merge back to the `master` branch.

[`Tue 21 Jan 2020 06:53:13 PM Beijing`]   
1. The replacement of `ip address` with `connection handle` has turned out not a trivial change, and has escalated to a major change. I need to bring the pirority of `multihop routing` ahead of the `websocket` support for now. This is a major upgrade of the router module, which enables associations of two or more devices/controlers in a hierarchical tree, and enable the client to `RPC` to a number of remote servers via a `path` string.

[`Mon 06 Jan 2020 12:04:26 PM Beijing`]   
1. I need to replace the `ip address` with a opaque `connection handle` for the upcoming support of websocket, since `ip address` is no longer the only address format to locate the server. It is the bottom half of an earlier change which replaced `ip address` with the `port id` on the `bridge` side. The `reqfwdr` and `dbusprxy` are the target modules for this time.   

[`Wed 01 Jan 2020 09:05:21 AM Beijing`]   
1. Happy New Year! 2020!   

[`Mon 30 Dec 2019 12:57:19 PM Beijing`]   
1. Obosoleted some bad code.

[`Sat 28 Dec 2019 04:11:29 PM Beijing`]   
1. The SSL related bug turns out to be the usage of global buffer without protection. 
2. Now all the known bugs in `sslfido` are fixed, and it should be safe to say the SSL support is more stable than yesterday. :)   

[`Fri 27 Dec 2019 08:51:54 PM Beijing`]   
1. Fixed a bug related to streaming channel setup, which could drop the reponse message and, thus, caused the proxy to wait till timeout. It happens only when the system load is very high.   
2. Also fixed two regression bugs due to earlier code changes, one is caused by no dedicated `irp completion thread` and the other is because the newly added `CTcpStreamPdo2` with incompitable interface to the old `CTcpStreamPdo`. It seems to take a longer time to make the new `tcp port stack` stable.

[`Thu 26 Dec 2019 08:17:28 PM Beijing`]   
1. Further testing between virtual machines revealed a significant performance issue and a SSL bug. The performance issue is fixed. the SSL bug is yet to fix, which should be a concurrent problem.   
2. Websocket support has to put off for several days. 

[`Fri 20 Dec 2019 10:29:02 PM Beijing`]   
1. Now there should still be some minor bugs in `sslfido`, related to the renegotiation. And there are some optimization and enhancement to do. But it does not matter that we can move on the add the support for websocket.   

[`Wed 18 Dec 2019 11:08:13 AM Beijing`]   
1. Finally, get over the last major bug in SSL support. It turned out to be a random packet loss in the CTcpStreamPdo2.  But the error report from OpenSSL as `mac failed`, was confusing and misleading for bug fixing. And most of the time was taken to find out why OpenSSL cannot work multi-threaded, in vain.

[`Fri 13 Dec 2019 07:47:35 PM Beijing`]   
1. Stucked with a bug that SSL randomly failed to decrypt the message and kill the SSL session. It is a bit difficult to fix...    

[`Wed 11 Dec 2019 09:48:08 AM Beijing`]   
1. OpenSSL support is almost done. And there are some minor bugs to fix. 
2. Next task is websocket support.   

[`Sat 30 Nov 2019 02:33:06 PM Beijing`]   
1. Still writing openssl support, and stucked with the SSL's renegotiation. It should be done next week.   

[`Mon 25 Nov 2019 03:58:57 PM Beijing`]   
1. added LZ4 compression option to the router traffic. the `rpcrouter` now accepts command line option `-c` to enable compression on the outbound packets from the CRpcNativeProtoFdo port.
2. SSL support next...

[`Sat 23 Nov 2019 01:38:46 PM Beijing`]   
1. Finally, it took 20 days to make the new `tcp port stack` in place. Both the old `tcp port stack` and the new stack can be useful in different use cases. And therefore let's keep both alive for a period.
2. the lesson learned is that it is always better to make full research in advance than to rewrite afterward.
3. Now we can move on to add the following features, compression, SSL, websocket/http2 support.

[`Sat 09 Nov 2019 10:09:12 AM Beijing`]   
1. Refacting the tcpport. I did not realize that through the days, the tcpport has grown to a big module with about 10000 loc.
2. Continue the design of the support for websock/http.
3. Object addressing and multiple hops between routers are also in the research.

[`Sun 03 Nov 2019 02:05:39 PM Beijing`]   
1. To add the support for websocket, SSL and other unknown protocols, I need to refact the current tcp port implementation, because the present design is too closely coupled without room for the new protocols. It is a bitter decision, but it should worth the effort. Sometime, you may need one step back for 2 steps forward. The good part is that the tcp port implementation is relatively isolated with little changes to other functions.

[`Fri 01 Nov 2019 04:47:07 PM Beijing`]   
1. Changes for Raspberry PI/2 PI is commited. An ARM platform is supported!   

[`Thu 31 Oct 2019 07:36:50 PM Beijing`]   
1. Just get the Raspberry PI/2 to work. the performance is about 15ms per request. Anyway the ARM support is coming soon.
2. The ideal http support is not a trivial task. It may take two months to get it run. Preparing to get hand dirty...
3. Some wonderful feature is brewing, hehe...

[`Thu 24 Oct 2019 10:31:14 AM Beijing`]   
1. IPv6 support is done.
2. It turns out http support depends on the presence of some new features. It needs some time to have a full understanding of the task.
   * Object addressing mechanism
   * multihop routing
   * Session management and access control
   * Site registration and discovery.

[`Mon 21 Oct 2019 05:34:24 PM Beijing`]   
1. Reduced the size of the request packets, by removing some redudant properties from the request configdb.
2. Having difficulty choosing between websocket or http/2, as well as the session manager. So let me add the support for IPv6 first as a warm-up execise.   

[`Thu 17 Oct 2019 07:36:03 PM Beijing`]   
1. Worked a `helloworld` with built-in router as the `btinrt` test, for the curiosity to know the performance of removal of dbus traffic. The outcome shows improvement of the latency from about 1.5ms to about 1ms per echo, as about 30% faster, in sacrifice of the flexibility. Anyway, well worth the effort. Also fixed a hanging bug in router and a segment fault in taskgroup which cannot reproduce in the stand-alone router setup.

[`Wed 09 Oct 2019 09:11:53 AM Beijing`]   
1. Upgraded the synchronous proxy code generation with the new macro, before the start of session manager design. It is now an easy task to write a proxy with just a few lines, and help the developers to focus on server side design. For detail information, please refer to test/helloworld.   

[`Mon 30 Sep 2019 02:53:58 PM Beijing`]
1. Finished rewriting the `sftest` and `stmtest` tests with the stream interface. And fixed many bugs.   
2. Next target is to get session manager to work. It is a premise task before the support for the `websocket` connection.   
3. Happy National Day! :)

[`Tue 17 Sep 2019 09:56:09 PM Beijing`]   
1. Major improvement! This project supports 64bit X86_64 now. Yeah!   

[`Wed 11 Sep 2019 07:16:37 PM Beijing`]   
1. fixed some bugs in the streaming module. It should be more stable now. There should still be a few bugs to fix.       

[`Sun 01 Sep 2019 09:11:20 AM Beijing`]   
1. Still writing some helper classes for the stream interface.

[`Wed 21 Aug 2019 10:21:16 AM Beijing`]   
1. The major streaming bugs are cleared so far. Next is to improve the stream interface for the proxy/server because current API is raw and difficult for third-party development.
2. the second task is to rewrite the file upload/download support via the streaming interface.
3. And the third one is to enable the project 64bit compatible in the next month.

[`Wed 21 Aug 2019 08:13:14 AM Beijing`]   
1. Encountered a concurrency problem in the router's stream flow-control these days. However it should be soon to be fixed.

[`Fri 16 Aug 2019 09:04:06 PM Beijing`]   
1. The stream channel can finally flow smoothly. There should still be some bugs around. Still need sometime to get it stable. Congratulations!   

[`Sun 11 Aug 2019 07:11:55 PM Beijing`]
1. FetchData works now. And moving on to the streaming data transfer over the tcp connection. I can see the light at the end of the tunnel...   

[`Wed 07 Aug 2019 09:48:22 PM Beijing`]   
1. OpenStream works now. Next to get the FetchData to work over the Tcp connection. It should be ok very soon...

[`Fri 02 Aug 2019 01:25:29 PM Beijing`]   
1. Start debugging the remote streaming now. Impressed by the streaming performance, and thinking if it is possible to route the RPC traffic via a streaming channel...   

[`Sat 27 Jul 2019 09:36:00 PM Beijing`]   
1. Made the local streaming work now. There remains some performance issue to solve. And afterwards to move on to the streaming over the router. It should last longer than local streaming.   

[`Sun 21 Jul 2019 02:25:01 PM Beijing`]   
1. Coding is fun and debugging is painful. ...   

[`Sat 20 Jul 2019 01:50:34 PM Beijing`]
1. Streaming support code complete finally. The next step is to get it run.

[`Mon 15 Jul 2019 06:02:27 PM Beijing`]
1. The last piece of code for streaming support is growing complicated than expected. Still trying to weld the two ends between tcp sock and unix sock gracefully inside the router. And struggling with many duplicated code with trivial differences.   

[`Wed 10 Jul 2019 04:02:20 PM Beijing`]
1. Almost code complete with the streaming support, remaining start/stop logics for the streaming channel in the bridge object. 

[`Sat 22 Jun 2019 02:03:53 PM Beijing`]   
1. Still busy designing the `flow control` for the streaming support. It turns out to be more complicated than expected. Just swaying between a simple solution and a complex one.   

[`Thu 13 Jun 2019 06:53:01 AM Beijing`]   
1. the CStreamServer and CStreamProxy are undergoing rewritten now. The unix socket operations will be put into a new port object, and all the I/O related stuffs will go to the port object. and the CStreamServer and CStreamProxy are no longing watching all the unix sockets, and instead, associate with a unix socket port object. The port object handles all the events related to the socket.   

[`Mon 03 Jun 2019 02:24:31 PM Beijing`]   
1. The earlier implementaion of `SEND/FETCH DATA` does not work for `TCP` connection. I need to tear it apart and rewrite it. The issues are:
*    Security problem with `SEND_DATA`. That is the server cannot filter the request before all the data has already uploaded to the server.
*    Both `SEND_DATA` and `FETCH_DATA` are difficult to cancel if the request is still on the way to the server, which is likely to happen over a slow connection.
*    `KEEP_ALIVE` and `OnNotify` becomes complicated.
2. The solution is to use the streaming interface to implement the `SEND_DATA/FETCH_DATA`, so that both the server and the proxy can control the traffic all the time.   
   

[`Sat 01 Jun 2019 05:48:48 PM Beijing`]   
1. Added a Wiki page for performance description.   

[`Mon 27 May 2019 08:13:24 PM Beijing`]   
1. Got sick last week, so long time no update.
2. The test cases, `Event broadcasting`, `KEEP-ALIVE`, `Pause-resume`, `Active-canceling`, and `Async call` are working now over both IRC and RPC, as well as `helloworld`.
3. Next move on to the support for SendData/FetchData RPC. 

[`Sun May 12 14:03:47 Beijing 2019`]   
1. The `communication channel setup` (`EnableRemoteEvent`) is almost OK for tcp connection. There should still be a few `hidden` bugs to fix in the future testing. Anyway the `helloworld` can now work more stable and faster. And both the `bridge` and `forwarder` are more torlerable on the error condtion.   
2. Next I will try to get `File/Big data transfer` work. It should be the last big section of un-debugged part so far.   
3. I will also get the other examples to run over TCP connection.   

[`Fri May 03 13:15:23 Beijing 2019`]   
1. Now the `helloworld` can work over the tcp connection. **Great!**   
2. There are some crash bugs and wrong behavor to fix yet.   
3. Happy International Labour Day 2019.   

[`Wed Apr 24 18:10:46 Beijing 2019`]   
1. The `bridge` side is now in good shape. Next I will take some time to clear some bugs on the `forwarder` side, before moving on to run the helloworld over the tcp connection.   

[`Sun Apr 21 12:36:37 Beijing 2019`]   
1. There is one last memory leak to fix yet. After done, we can wrap up the `EnableRemoteEvent` and move on to debugging client requests.   

[`Thu Apr 11 14:06:43 Beijing 2019`]   
1. Fixed some fatal memory leaks and concurrency bugs. Hopefully, the biggest memory leak will be fixed in the next check-in. But the progress is a bit lagging :(    

[`Wed Mar 27 17:56:33 Beijing 2019`]   
1. Fixed some bugs in the code. There are still some memory leak and concurrency bugs to fix in the `EnableRemoteEvent` and connection setup.

[`Thu Mar 14 20:47:45 Beijing 2019`]   
1. The `EnableRemoteEvent` works on the `Forwarder` side. and The bridge side has successfully received the request. Move on to get the `Bridge` side `EnableRemoteEvent` to work. This should be the last part work before getting through the whole request/response path.   

[`Thu Mar 07 23:35:20 Beijing 2019`]   
1. Fixed the memory leak issues. Move on the get the CRpcControlStream to work.   

[`Tue Mar 05 14:05:40 Beijing 2019`]   
1. Encountered and fixed some bugs in the `CRpcRouter`. There are still some memory leaks to fix. One known bug is that, the `OpenRemotePort` and `CloseRemotePort` need to be sequentially executed rather than in random order. Otherwise, the `CRpcTcpBridgeProxyImpl` and the `CTcpStreamPdo` may not be able to be released properly.   
2. After the leaks are fixed, I will get `EnableRemoteEvent` to work.   

[`Sun Feb 24 14:52:28 Beijing 2019`]   
1. This week, I had some emergency thing to handle. I will get back next week.   

[`Sat Feb 16 17:35:54 Beijing 2019`]   
1. `OpenRemotePort` is done now. The CDBusProxyPdo and CDBusProxyFdo are found lack of the `online/offline` handler for both remote server and remote module, and need to add some code next week. And after that, we can move on to debug `EnableEvent` related stuffs.   

[`Thu Feb 07 21:17:48 Beijing 2019`]   
1. Still debugging the CRpcRouter. The `OpenRemotePort` is almost completed now. And there still need a disconnection handler on the `reqfwdr` side to clean up the context for the unexpectedly disconnected proxy.   

[`Thu Jan 24 17:03:14 Beijing 2019`]   
1. Continued to refact part of the CRpcRouter. Now I expect there are some trivial patches need to be done in the coming days of this week before the refact task is done.   

[`Sat Jan 19 23:43:28 Beijing 2019`]   
1. The redesign and refact work are almost done. Next week we can start the debugging of RPC module hopefully.   

[`Fri Jan 11 08:32:13 Beijing 2019`]   
1. There are several places in the router to redesign for new requirements/discoveries. And the details have appended to the todo.txt.   

[`Tue Jan 01 20:53:12 Beijing 2019`]   
1. This week, I need to redesign the management part of the router for the bridge and reqfwdr's lifecycle management, and add new command to the tcp-level protocol for connection resiliance.   
2. Note that, if you want to run `helloworld`, please use the earlier version `bf852d55ae2342c5f01cc499c59885f50780c550` of `echodesc.json` instead. The latest version is modified for RPC debugging purpose.

[`Tue Dec 18 21:35:54 Beijing 2018`]   
1. Made the proxy ports loaded and work. Next step is to get the req-forwarder to run in a stand-alone process.
2. We have two approaches to deploy the proxy, the compact way, that is, the proxy and the req-forwarder works in the same process, and the normal way, that an rpc-router process runs as a router for all the proxys system-wide. I will first make the normal way work. The significance of compact way is to have better security in communication than the normal way if the device is to deploy in a less trustworthy environment.

[`Fri Dec  7 22:45:23 Beijing 2018`]
1. Finally, I have wrapped up local IPC testing. Now I will proceed to RPC debugging and testing.

[`Mon Dec 03 17:47:55 Beijing 2018`]
1. Added local stream support. It is yet to debug.   

[`Tue Nov 20 13:02:20 BeijingBeijing 2018`]
1. Added message filter support and multiple-interface support. Sorry for not starting the RPC debugging still, because I need to dig more in streaming support which will affect RPC module a bit.   

[`Wed Nov 14 11:24:42 Beijing 2018`]
1. Local `Send/Fetch` is done. And now it is time to move on to the RPC module. For security purpose, I will add the authentication interface sometime after the RPC module is done.   

[`Fri Nov  2 18:24:51 Beijing 2018`]   
1. This week, the `keep-alive` and `Pause/Resume` works.
2. Next week, I will try to get `Send/Fetch file` to work, before I can move on to RPC module.   
   
[ `Tue Oct 23 19:59:39 Beijing 2018` ]   
1. This week, I will get keep-alive to work and move on to make RPC module to work.   
   
[ `Mon Oct 15 18:51:48 Beijing 2018` ]  
1. The dependency of `g_main_loop` from `glib-2.0` is replaced with a simplified `mainloop`.   
2. the glib headers are still needed at compile time. But the glib shared libraries are not required at deploy time.   
   
[ `Thu Oct  4 12:24:00 Beijing 2018` ]
1. After some tests, I cannot find a way to put `libev` in the concurrent environment flawlessly.
So `libev` is not an option any more. [(Reason)](https://github.com/zhiming99/rpc-frmwrk/wiki/Why-libev-cannot-be-used-in-rpc-frmwrk%3F)  
2. I will write a simple mainloop with `poll` to fix it.   

