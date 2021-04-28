



### 1、数据库的使用



redis client 默认使用 0 号数据库

```c
struct redisServer {
    redisDB *db;	// redis server DB 数组
    int dbnum;		// DB 个数
};

struct redisClient {
    redisDB *db;	//指向redis server的目标DB
};
```



```shell
# 查看数据库个数
> config get databases	

# 选择数据库
> select 0

```

#### 数据库键空间

可以理解为是对象空间，键值对是<对象名, 对象>，对象名总是字符串对象类型的。

expires 字典的键是 键空间中对象的指针，而值是long long 类型的整数，毫秒级别的UNIX时间戳

```c
struct redisDB {
    dict	*dict;	//数据库键空间，保存数据库中所有的键值对，键是对象的名字，值是数据对象如列表对象
    dict	*expires;	//过期字典，数据库中全部键的过期时间
    
};
```

> redis数据库的键空间，如果通过对象名快速定位到对象在字典中的位置？



#### 生存时间和过期时间

- EXPIRE、EXPIREAT、PEXPIRE ：设置过期时间
- PERSIST：移除过期时间

- TTL：以秒为单位计算键的剩余生存时间
- PTTL：以毫秒为单位计算键的剩余生存时间

> 如果检查过期键？如果键过期了，它是在什么时候被移除的？

Redis服务并没有专门用一个线程去检查过期键，检查键是否过期，发生在对该键的请求访问

- 检查键是否过期

  1. 检查请求的键是否存在于过期字典，如果存在，取出键的过期时间
  2. 检查当前的UNIX世家你戳是否大于键的过期时间戳；

- 过期键的删除策略

  1. 定时删除：设置过期时间同时创建定时器，让定时器触发执行删除操作；（主动删除）

     创建定时器，需要维护定时事件，查找定时事件的复杂度是O(N)，并不能高效处理大量的定时器。

  2. 惰性删除：过期也不管，仅当访问时如果过期就删除；（主动删除）

     对CPU时间友好，不需要要花费额外的CPU时间去做过期键的删除；对内存不友好，过期键值依然留在内存中占据资源。如果过期键用于不被访问，那就相当于是【内存泄漏】

  3. 定期删除：每隔一段时间检查一次expires字典，删除过期的键；（被动删除）

     是【定时删除】和【延迟删除】 的折中，限制删除操作的执行时长或频率来减少操作对CPU时间的影响

     > 如何确定定期删除的时长和频率？

- Redis过期键删除策略的实现

  1. 惰性删除
  2. 定期删除

#### AOF和RDB

- RDB文件：RDB是对redis数据库的一次快照（snapshot），类似 dump 当前键空间。

  1. RDB持久化：执行`SAVE`或者`BGSAVE`，创建RDB文件，将数据库中的键空间（不包含过期的）保存到文件中持久化。

  2. **RDB加载**：重启Redis服务时，如果开启了RDB，就会载入RDB文件，载入过程也要检查键是否过期，已经过期的不被添加到键空间。如果是Slave加载Master的RDB文件，那么就不会检查是否过期，直接全部加载。Slave服务器的键空间会在主-从同步的时候被清除。

- AOF文件：

  1. AOF持久化：不检查是否过期，直接dump out；当访问过期键触发惰性删除或者定期删除的时候，Redis服务向AOF文件追加一条删除命令，显式地记录该键被删除。
  2. AOF重写：执行AOF重写，检查过期键，已过期的键不会被保存到AOF



#### 过期键的主从一致性

当服务器运行在复制模式（主-从模式），从服务器上的过期键的删除由主服务器控制。

- 主服务器删除过期键的同时，向从服务器发送DEL命令，通知从服务器删除过期键；
- 仅当主服务器通知删除过期键时，从服务器才删除；否则即使访问从服务器的过期键，也是有效的；



#### 订阅事件



#### 发送通知



### 2、RDB持久化

将Redis在内存中的数据库状态保存到磁盘，避免数据意外丢失。

RDB持久化可以手动执行或者定期自动执行。RDB文件是一个经过压缩的二进制文件。

- 手动执行

  1）SAVE：Redis服务器或陷入阻塞，直到RDB文件创建完成；期间的client请求会被拒绝

  2）BGSAVE：会创建一个子进程，然后子进程负责创建RDB文件；（为什么是子进程而不是子线程？）

  1. 执行期间，拒绝client的SAVE命令，避免服务器SAVE和BGSAVE同时执行rdbSave()调用，防止竞争冲突
  2. 执行期间，拒绝client的BGSAVE，避免两个BGSAVE子进程同时执行产生竞争冲突
  3. 执行期间，收到client的BGREWRITEAOF命令，将按序执行；反之，如果是先收到BGREWRITEAOF，AOF执行过程将拒绝BGSAVE命令（出于性能的考虑）

  > RDB文件载入期间，Redis服务器也会阻塞，直到载入完毕。

- 间隔自动保存

  1）使用BGSAVE，间隔性执行一次BGSAVE，不会阻塞Redis服务

  2）配置服务器的save选项，设置BGSAVE的触发条件；可以设置多个条件，只要有一个成立就行

  ```c
  struct saveparam {
      time_t seconds;		//间隔时间 / 秒
      int changes;		//修改次数
  };
  
  struct redisServer {
  	
      struct saveparam *saveparams;	//save选项，记录保存条件的数组
      long long dirty;				//上次save之后，修改次数的计数器
      time_t lastsave;				//上次执行save的时间
  };
  ```

> 当Redis服务器开启AOF的时候，则会优先采用 AOF 持久化；

- RDB文件结构

  全大写是常量，全小写是变量和数据；RDB文件保存的是二进制数据而不是C字符串。

  ```c
  | 'REDIS' | db_version | databases | EOF | check_sum |
  |   5字节  |  4字节     | 取决于DB大小 | 1字节 | 8字节    | 
      
  --- RDB文件中 非空数据库 databases 的结构
      
  | SELECTDB | db_number | key_value_pairs | 
  | 1 字节    |  DB编号    |   键空间         |
  ```

- 分析RDB文件 dump.rdb，

  1. 使用linux的 `od`工具

  ```bash
  od -c dump.rdb
  ```

  2. 使用Redis自带的RDB文件检查工具： `redis-check-dump`

- 



### 3、AOF持久化

与RDB持久化dump数据库不同，AOF持久化是记录客户端的操作记录，所以是纯文本的。

AOF持久化启动，服务器执行完一个命令后就会将该命令以某种协议格式，追加到服务器状态 的AOF缓冲区

```c
struct redisServer {	//服务器状态
    
    sds aof_buf;		//AOF缓冲区
    sds aof_rewrite_buf;	//AOF重写缓冲区
};
```

然后再定期地把AOF缓冲区写出到AOF文件；但，系统调用write，实际上是先写到内核缓冲区，然后再从内核缓冲写出文件；为了强制让操作系统立马将缓冲区写出文件而不驻留在内核，系统提供了 `fsync`和`fdatasync`的文件写入同步。

#### AOF文件的载入和数据还原

因为AOF保存的是一系列的操作，通过AOF还原数据库，也就是重新将这些命令执行一遍，类似mysql的redo log。



所以AOF的载入的时候，redis会创建一个伪客户端用于执行这些命令。

#### AOF重写

随着客户端操作越来越多，AOF文件的体积自然就会不断增大，导致，AOF文件载入的时候就要花费更多时间。AOF文件中，实际上会存在很多冗余的命令，比如SET key ; DEL key 这时key就不存在了，没必要保存在AOF文件。

>  AOF重写的原理

通过读取各个数据库键空间的每一个键值对，通过一条插入命令记录每个键对象，写入到新的AOF文件，相当于，合并了以前的多条命令，只根据最终结果，生成一条插入命令。

- AOF后台重写

  1）Redis使用子进程，而不是子线程来执行 `aof_rewrite`：

  1. 防止redis服务阻塞
  2. 子进程具有父进程的数据副本，可以避免使用锁

  2）AOF重写期间，服务器收到新的操作命令，同时写入到 `aof_buf`和`aof_rewritw_buf`；当子进程写完`aof_buf`之后，再检查`aof_rewrite_buf`，不为空，再把`aof_rewrite_buf`追加到AOF文件。

  

### 4、事件驱动

Redis服务器是一个由事件驱动的程序，支持和处理两类事件

1. 文件事件：socket上的事件，accept、read、write、close等
2. 时间事件：Redis服务器上的定时器事件，周期性事件等

#### I/O多路复用

Redis是基于Reactor模式开发了网络事件处理器，也叫文件事件处理器。

网络事件处理器，使用I/O多路复用同时监听多个connected socket上的不同事件，为每一种事件绑定一个事件处理器；当scoket上有文件事件来到，文件事件处理器就会针对该事件调用关联的事件处理器（event handler)。

文件事件处理器运行在单线程上，并且与Redis服务器上的其他单线程模块对接。

所以Redis服务不是单进程单线程服务，而是单进程具有多个线程的服务，只不过Redis服务器有多个单线程的模块

#### 文件事件

#### 时间事件

#### 事件的调度与执行



### 5、客户端

每个连接的客户端，redis服务器都为之建立了 `redis.h/redisClient`结构来保存客户端状态。

```c
struct redisClient {
    in socket;
    char clientname[];
    int flag;
    redisDB * db;
    int DB_num;
    char* in_buf;
    char* out_buf;
    ....
};
```

Redis服务器还需要为所有已连接的客户端维护一个`redisClient`节点的链表 `clients`，保存所有已连接的客户端信息。

```c
struct redisServer {
    
    list	*clients;
    redisClient	*lua_client;		//记录lua脚本的伪客户端
    redisClient	*aof_client;		//AOF文件载入的时候创建的伪客户端
    
};
```



#### 客户端属性详解

属性可以分为两类：

1. 通用属性：客户端执行任何工作都需要用到的属性
2. 特定属性：执行特定功能用到的属性，如db属性/dictid属性/事务的mstate属性/WATCH命令的watched_keys属性



- 通用属性

  ```c
  
  struct redisClient {
      
      int 	fd;				///< accepted client socket fd
      robj	*name;			///< client's name, string object
      int 	flags;			///< client's role and status
      
      sds		*querybuf;		///< input buffer
      
      robj	**argv;			///< command line parameter
      int		argc;			
      
      //command
      struct redisCommand	*cmd;	///< the command ref. by argv[0]
      
      // output buffer
      char	buf[REDIS_REPLY_CHUNK_BYTES];	///< output buffer with fixed size
      int		bufpos;
      list	*reply;				///< output buffer with changable size, string list
      
      int authenticated;			///< record client's verified symbol, 1 or 0
      
      //time attrs
      time_t	ctime;				///< creating time of client
      time_t	lastinteraction;	///< last interacting time of client
      time_t	obuf_soft_limit_reached_time;	///< 
      
  };
  ```

  - fd：伪客户端则是 -1，普通客户端则大于 -1；`CLIENT list`查看当前已连接的client info
  - name：客户端名字，通过 `CLIENT set xxx`设置，一般设置为与客户端执行功能相关
  - flag：客户端角色和状态，flag1 | flag2 | ...；角色如主、从服务器、伪客户端；状态如客户端正在执行的工作，如连接、阻塞、事务、监听、待关闭、AOF持久化等
  - querybuf：输入缓冲区，动态增长，最大不超过1G
  - argc 和 argv ：客户端请求包含的参数个数，argv包括要执行的命令及其参数列表；
  - cmd：argv[0]即要执行的命令，绑定一个redisCommand的指针；redisCommand来自一个命令表，该表是一个字典，根据argv[0]查找到对应的redisCommand结构；
  - authenticated：标志是否通过身份验证；AUTH / PING
  - 
  - 

  

- 特定属性



#### 客户端的创建和关闭

- 创建

- 关闭：主动关闭（CLIENT KILL）和被动关闭，在以下情况下，客户端可能被服务器被动关闭：

  1. 网络连接断开

  2. 命令请求不符合协议格式

  3. timeout（不包括主-从模式）

  4. 发送请求size超过输入缓冲区大小1G

  5. 回复内容超过输出缓冲区大小限制（避免占用过多的服务器资源）

     - 硬性限制：输出缓冲区大小超过了硬性限制所设置的大小，立即关闭客户端；

     - 软性限制：超过软性限制所设置的大小，但没超过硬性限制，记录超出软性限制大小的时间；如果客户端在一定时间内再次超过软性限制，即关闭它。

       `client-output-buffer-limit <class> <hard limit> <soft limit> <soft seconds>`命令



### 6、服务器



#### 命令请求执行过程

```c
/*
client
---> send command request
    --> server read command rquest
    --> server parse command request
    --> server find concrete command to execute
    --> server execute preparing operation
    --> server execute command function
    --> server execute extra task
    --> server send back reply to client
client 
<-- receive reply

*/

//// 
/*
extra task
    - 服务器检查是否开启了慢查询，慢查询需要添加一条慢查询日志
    - 将命令执行的耗时时长更新到rediCommand结构的milliseconds属性，将calls属性+1
    - 检查是否开启了AOF持久化，是否需要将执行完的命令写入到AOF缓冲区
    - 检查是否有从服务器正在同步复制，需要将刚执行的命令传播给所有从服务器
   
*/


```



#### serverCron函数

每个100毫秒执行一次，负责管理redis服务器的资源，保证redis服务器良好运转。

- 更新服务器时间的缓存

  避免频繁调用获取系统时间的系统调用，每100毫秒更新一次 unixtime和mstime，所以这2个属性的时间精度并不高。

  - 适用于 log打印、更新服务器LRU时钟、是否执行持久化任务、计算服务器上线时间等精确度不是很高的功能；
  - 不适用于 设置过期时间、添加慢查询日志等高精确度时间的功能，对于这类功能，服务器或执行系统调用来获取最新的系统时间戳。

```c
struct redisServer {
    time_t unixtime;		///< 秒级精度的UNIX时间戳
    long long mstime;		///< 毫秒级别的UNIX时间戳
    
    unsigned lruclock:22;	///< update per 10s,计算键的空转时长
    
    long long ops_sec_last_sample_time;	///< 上一次抽样的时间
    long long ops_sec_last_sample_ops;	///< 上一次抽样，每秒执行的命令数
    // 最近REDIS_OPS_SEC_SAMPLES次 抽样的结果，环形数组
    long long ops_sec_samples[REDIS_OPS_SEC_SAMPLES];
    int ops_sec_idx;		///< ops_sec_samples 的下标，当前的抽样
    
    size_t stat_peak_memory;	///< 内存峰值
    
    int shutdown_asap;			///< 关闭服务器的标识,1:close / 0: nothing
    
    pid_t rdb_child_pid;		///< RDB持久化的子进程号
    pid_t aof_child_pid;		///< AOF持久化的子进程号
    
    int cronloops;
};

struct redisObject {
    unsigned lru:22;		///< 保存对象最后一次被访问的时间
};
```

- 更新LRU时钟

  server的属性 lruclock 保存了服务器的LRU时钟，保存当前时间戳，只不过是10s更新一次。

  计算redis键对象的空转时间，就用服务器的 lruclock 时间 - 键对象的lru时间。

- 更新服务器每秒执行命令次数

- 更新服务器内存峰值记录

- 处理SIGTERM信号，检查shutdown_asap属性；在处理SIGTERM之前，进行RDB持久化操作，持久化完成后才能关闭服务器。

- 管理客户端资源

  - 如果客户端与服务器的连接已经超时，那就释放掉该客户端
  - 检查客户端上次执行完的请求是否有输入缓冲区超出限制，如有，释放掉，然后新建。

- 管理数据库资源
  - 删除过期键
  - 如有需要，对字典进行收缩操作

- 执行被延迟的BGREWRITEAOF
  - 如果BGSAVE执行期间有 BGREWRITEAOF命令到来，该命令被延迟到BGSAVE完成后才执行
- 检查持久化操作的运行状态
  - 检查rdb_child_pid 和 apf_child_pid 属性，如果不为 -1， 那么需要等待一下，检查是否有子进程发送信号给父进程，然后处理信号。
- 将AOF缓冲区的内容写入AOF文件
- 关闭异步的客户端
  - 关闭那些输出缓冲区超过限制的客户端
- 增加cronloops计数器的值
  - cronloops属性记录了 serveCron 函数执行的次数

#### 初始化服务器

1）初始化服务器状态数据结构 `redisServer`

每个redis服务实例都创建一个，并设置其中的各个属性的默认值

2）载入配置选项

指定配置文件，修改服务器属性的默认值

3）初始化服务器数据结构

在载入配置文件后才执行初始化

4）还原数据库状态

载入RDB文件或AOF文件，根据文件还原数据库状态

5）执行事件循环，开始监听客户端的连接





