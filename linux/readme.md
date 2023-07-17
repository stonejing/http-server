# DOC

## TEST
start webserver in the Linux using ./webserver command, and input localhost:2345/index.html, if is shows "test", then it proved the webserver can work.

server.cpp: bind, listen, accept, 主线程

## EPOLL
关于 epoll, 使用 ET，每次都将数据读完。
只有两种状态，EPOLLIN 和 EPOLLOUT
EPOLLOUT 只在写大量数据的时候，写缓冲区满的时候，write 返回 errno 的时候，才需要手动设置 EPOLLOUT 状态
每个 FD 都需要一个单独的 buffer 

看 ET 还是 LT, 使用 ET 模式，write 和 read 都要程序管理
只有两个状态，EPOLLIN 和 EPOLLOUT
oneshot 并不需要，因为 socket 只由一个线程管理

## Channel
epoll 获得 epoll fd 和 event。
channel 包含 fd 的一些东西？？？
每一个 socket 都有一个对应的 class，包含很多东西，这个 fd 上的 http 类，buffer，定时器等等

## Log
因为有 Log 的全局 IO 锁在，因此只要调用了 Log ，就是单线程