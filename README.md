# rpc-frmwrk
An effort for embedded RPC framework, and hope it could find its usage in IOT or other embedded systems.

---
[ Mon Oct 15 18:51:48 CST 2018 ]
## Dependency:  
This framework depends on the following packags:  
1. `dbus-1.0`  
2. `libjson-cpp`  
3. `cppunit-1 (for the sample tests only)`   
4. `glib-2.0 ( for compile only )`   
The dependency of `glib-2.0` is replaced with a simplified `mainloop`. Now the glib headers are still needed at compile time. But the glib `so` files are not required at deploy time.

[ Thu Oct  4 12:24:00 CST 2018 ]   
1. After some tests, I cannot find a way to put `libev` in the concurrent environment flawlessly.
So `libev` is abandoned. [(Reason)](https://github.com/zhiming99/rpc-frmwrk/wiki/Why-libev-cannot-be-used-in-rpc-frmwrk%3F)  
2. I will write a simple mainloop with `poll` to fix it.   

