## 2021-11-04
在今天，终于搞明白的 IO复用 和 定时器 和 信号 的使用及作用。
IO 复用，EPOLL 的几个事件，如何使用，设置 和 触发。
信号 是如何设置和触发的。
定时器 是如何工作的，如何触发的。

理解这几个原理非常的关键，和有作用。
主要是一个工作流程，理解程序的逻辑，这个非常重要。

明天，首先，自己梳理一下 一个 简单的 http server 的工作流程。

## 学习过程：
简单 socket 连接过程。
IO 复用学习，EPOLL 实现简单 http server。
处理 CORNER CASE，如何编码。
实现 http 协议的简单解析和发送
实现线程池，加速性能
实现 log 系统

## 代理软件
客户端：建立一个本地服务器，接收数据然后转发给 远程服务器。
本地是一个 socks5协议 的服务器，远程随便定义就好。
本地客户端启动时，直接和远程服务器建立一个长连接，使用 tls 传输数据就好了。
可以设置多个 文件描述符，每一个描述符对应不同的服务器，然后客户端根据不同的地址发送到不同的地址。

## 一些 BUG
accept invalid argument: 就是参数有问题。一开始第三个参数没有初始化，所以出错，并且出现有时好有时坏的情况

2021-11-13 01：08
如果一个代码不是自己写的，出 bug 都不知道怎么排除。
幸好可以打 log，然后自己分析，是哪里出现了问题。

2021-12-13
不知不觉一个月了，一个能用的 socks5 的软件还是没有写出来。不知道自己在干什么。
socks5 握手有两种做法
一种是阻塞，考虑到本地超级好的网络环境，使用阻塞也没有什么，而且握手的过程也很小；
另一种是非阻塞，为每一个连接新建一个 struct，存储 stage，每一步对应一个 stage，
    connect remote 也算是一个 stage，总计四个 send recv 都算是 stage，
    然后是 数据传输 也是一个 stage。
#### 2021-12-15
linux/openssl_shadowsocks下
有一个完整的能够接上 openssl 的，能够在 windows 下工作的代理软件，但是有严重的内存泄漏。

## 2022-02-21
在搞了很多乱七八糟很杂的东西之后，又再一次开始了自己的 http server 之旅。
想要的是，通过这一个项目，实现一个什么技术，而不是什么功能。
技术点：
无锁队列：多线程争用，多线程读写操作
日志模块：高性能 log 的实现，使用 linux api
配置模块：json parser，或者 yaml parser，高性能，或者针对性优化
协程模块：这只是回调的一种优化实现，性能会降低一点点，线程级别的子任务的异步执行
tcpserver 模块：一个类
线程模块：一个简答的线程池
http server 模块：功能实现
http connection 模块：connector 类，暂时不需要
socket 模块：暂时也不需要
其它的东西：我需要通过这一个项目，学习到什么？首先，其实我需要将这些东西先实现了

## 2022-02-24
每一次调用 LOG，都需要初始化类，然后调用函数，然后析构？那会损失多少性能啊
这个双缓冲，和环形队列缓冲区，貌似没有太大的区别
环形缓冲区，貌似区别也是太大啊，直接设置 fopen 的缓冲区了，然后 flush 就是写磁盘
log 线程负责将环形缓冲区的内容写入 fopen 缓冲区，然后一次性 flush
可能双缓冲好一点，环形队列可以使用无锁结构，虽然我到现在都没有写过无锁数据结构的代码

## 2022-03-14
颓废了这么多天，今天，需要将最基本的 log file 的雏形完结，做到功能有效就行了
benchmark 以后再做，所有的功能都完善之后，一次性做。
然后就是 webserver 的功能，再支持 SSL 就可以了。
