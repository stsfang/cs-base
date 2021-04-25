

### 1、简单字符串SDS

```c
struct sdshdr {
    int len;	///< 已使用的长度 
    int free;	///< 未使用的长度
    char buf[];
};
```

> C语言的简单字符串 和 SDS 有什么区别？

SDS可以理解为C语言字符串的简单封装，就是加上了len和free字段：对比C++ string，SDS可以理解为string的简化版，SDS也是动态字符串。

- SDS记录字符串长度，O(1)时间获取字符串长度；strlen(sds)的时间是O(1)
- SDS记录free字段，防止buffer overflow；C语言字符串不检查边界，容易buffer overflow；
- SDS插入/修改字符串的时候先检查free空间是否足够，不够就先扩展free空间，然后才执行修改；类似C++ string的动态分配；
- SDS的len字段和free字段，在字符串伸缩的时候，起到减少【内存分配和回收】的开销

#### 空间优化策略

- 预分配内存：就是额外多分配一点free空间，减少字符串修改/增长带来的内存重分配次数
  - 比如修改的时候，让free属性和len属性保持一样（len<1MB）；
  - 如果len>=1MB，那么free就设置为1MB;
- 延迟释放内存：内存释放，发生在SDS的字符串缩短的时候；字符串缩短的时候，SDS并不释放掉多出来的内存，而是暂时修改len和free属性将修改后的长度和空闲部分记录起来，以备将来再次使用。

#### 字节存储二进制数据

C语言的字符串遇到`\0`或者空字符，就认为字符串已结束；而SDS并不是根据`\0`来判断字符串结束，而是`len`属性。所以，SDS的字节数组可以保存`\0`；

所以，SDS的`buf`是字节数组，可以保存一系列的二进制数据，这使得SDS不仅可以保存文本数据，还可以保存任意格式的二进制数据，如图片、音频、视频和压缩文件等。

- 保存文本数据的SDS可以复用一部分C语言的字符串库函数



### 2、链表 Linked List

> Redis的链表可支持哪些功能？

- 普遍链表存储节点
- 发布与订阅
- 慢查询
- 监视器

```c
typedef struct listNode {
    struct listNode* prev;
    struct listNode* next;
    void* value;
} listNode;

typedef struct list {
    listNode* head;							///< 头结点
    listNode* tail;							///< 尾节点
    int len;								///< 双端链表的长度
    void *(*dup)(void *ptr);				///< 节点复制
    void (*free)(void* ptr);				///< 节点值释放
    void (*match)(void *ptr, void *key);	///< 节点值比较
};
```



### 3、字典 Dict

字典，也叫映射(map)，也叫哈希表（Hashmap)，用于保存键值对的数据结构。

Redis的字典，使用哈希表作为底层实现。

```c
/// 可见Redis的哈希表使用链表作为冲突解决方式
typedef struct dictEntry {
    void *key;	///< 键
    union {
        void *val;
        uint64_tu64;
        int64_ts64;
    } v;
    struct dictEntry *next;	///< 下个哈希表节点
};

typedef struct dictht {
    dictEntry **table;		///< 哈希表，指向dictEntry* 数组
    unsigned long size;		///< 哈希表大小
    unsigned long sizemask;	///< 哈希表掩码 == size-1
    unsigned long used;		///< 哈希表长度
} dictht;

// 函数集合
typedef struct dictType {
    ...
}dicType;

typedef struct dict {
    dictType *type;	///< 特定类型函数,多态
    void *privdata;	///< 私有数据，保存传给特定类型函数的可选参数
    dictht ht[2];	///< 使用两个哈希表，ht[1]在ht[0]进行rehash时使用
    int trehashidx;	///< rehash进度，if -1, rehashing not in progress
} dict;
```

#### 哈希算法



#### 解决键冲突

链地址法



#### 重哈希rehash

- 负载因子 

```c
load_factor = ht[0].used / ht[0].size;
```

- 渐进式rehash

  > 为什么需要渐进式地rehash？

  因为当hash table上的数据量很大的时候，一次性rehash可能导致redis短时间内停止服务，全部工作用在rehash，这严重降低了redis的性能；所以redis服务器并不是一次性将ht[0]全部键值rehash到ht[1]，而是分多次、渐进式地rehash。

  渐进式rehash，用rehashidx指示每次rehash的数组索引，完成该索引下的全部键值对的rehash，然后将rehashidx+1;

  rehash首次触发，只是为ht[1]分配空间，此时dict同时持有ht[0]和ht[1]两个哈希表；rehashidx=0;

  由后续的CRUD操作触发继续rehash，每次完成rehashidx索引下的全部键值对的rehash；

  直到 rehashidx = size；将rehashidx置为-1，表示rehash已完成

  渐进式rehash将rehash工作的性能损耗，分派到多个CRUD操作上，平摊rehash带来的性能下降。

  

  

#### 哈希表的伸缩

- 扩展条件（满足其一）
  1. 服务器没有 BGSAVE（后台异步保存）、BGREWRITEAOF（后台异步写时复制），且 load_factor >= 1
  2. 服务器有 BGSAVE，或有 BGREWRITEAOF，且load_factor >= 0.5



### 4、跳跃表 Skip List

> 跳跃表的数据结构是怎么样的？

Skiplist是一种有序的数据结构，通过在每个结点中维持多个指向其他结点的指针，从而达到快速访问结点的目的。

跳跃表支持平均O(logN)、最坏O(N)复杂度的结点查找；还可以通过顺序行操作来批量处理节点。

这相当于是说，在大部分情况下，跳跃表的效率可以和平衡树对比，但是跳跃表的实现比平衡树简单。

Redis仅在2个地方用到Skip List：

- Redis使用跳跃表作为集群节点的内部数据结构；

- Redis使用跳跃表作为**有序集合键**的底层实现之一，这里的有序集合键可以类比C++的map：
  1. 当有序集合的元素比较多，或者是比较长的字符串，Redis就会使用跳跃表作为有序集合键的底层实现



```c
typedef struct zskiplistNode {
    struct zskiplistLevel {
        struct zskiplistNode *forward;	///< 前进指针
        unsigned int span;				///< 跨度
    } level[];
    struct zskiplistNode *backward;		///< 后退指针
    double score;						///< 分数
    robj *obj;							///< 保存的对象
} zskiplistNode;

```

> 前进指针和跨度的作用分别是什么？



### 5、整数集合Intset

整数集合，是集合键的底层实现之一；当一个集合只包含整数元素，并且整数元素不多，Redis就会选择整数集合作为集合键的底层实现。对比C++中的 `set<int>`；整数集合支持的整数类型有：int16_t、int32_t和int64_t。

整数集合是有序集合，且不能重复元素。

```c
typedef struct intset {
    uint32_t encoding;	///< 决定contents实际表示的是什么整数类型
    uint32_t length;	///< 元素数量
    int8_t contents[];	///< 保存元素的数组，以
} intset;


```

> 整数集合，如何保证元素不重复？

最笨的方式当然是查找，平均是O(N)的时间开销。



> 整数集合，如何实现元素插入有序？

最笨的方式当然是插入时比较，典型的插入排序，找到合适的位置，移动后面的元素，插入该位置。



#### 类型升级

将一个新元素的类型比整数集合现有元素类型的长度更大，如现有类型 int32_t，新元素类型int64_t，将会导致整数集合先进行类型升级，将全部int32_t元素转为int64_t的大小，然后才插入新元素。

类型升级，整数集合的contents数组需要【扩展】数组大小，然后将旧元素按新类型放到正确的位置上（移动元素），这得益于contents数组的单位是int8_t。

类型升级的好处，节约内存，仅当需要使用更大的类型存储的时候才开辟更大的空间。

但是，类型升级是不可逆的，即不存在类型降级，一旦升级，就会保留升级后的类型。



### 6、压缩列表Ziplist

压缩列表是列表和哈希表的底层实现之一。（列表或由双端链表实现，哈希表或由字典实现）

1. 当列表键只有少量列表项，且列表项是小整数值或长度比价短的字符串，Redis使用ziplist实现列表键
2. 当哈希键只有少量键值对，且键值对是小整数值或长度比价短的字符串，Redis使用ziplist实现哈希键

> 压缩列表是如何实现压缩的？

ziplist由一系列特殊编码的连续内存块组成的顺序型数据结构。目的是节约内存。

ziplist的entry可以保存一个字节数组、或者整数值

```c
struct entry {
    int?_t previous_entry_length;		///< 记录上一个节点的字节长度,1字节/5字节
    int?_t encoding;					///< 编码：表示数据类型及长度，1字节/2字节/5字节 
    void* content;						///< 保存节点的值
};

struct ziplist {
    uint32_t zlbytes;	///< 整个ziplist的内存字节数
    uint32_t zltail;	///< 尾结点到起始地址的字节偏移
    uint16_t zllen;		///< 节点数量
    void* entryX;		///< 不定长度，节点长度由该节点的内容决定
    uint8_t zlend;		///< 标记ziplist的末端
};
```

> 为什么entry中的previous_entry_length使用1字节或者5字节？该属性的作用有哪些？

- 当前一个结点的长度小于255字节的时候，使用1字节足以表示该长度；当大于255字节的时候，使用5字节，其中第一个字节标识0xFE（254)，而之后的4字节保存上一节点的长度；
- 通过该属性，当前结点的指针pCur - pCur->previous_entry_length就得到上一个节点的起始地址，从而，从ziplist尾结点，可以倒序遍历压缩列表中的结点。

> previous_entry_length记录上一个结点的长度，这可能导致什么问题？

可能导致【连锁更新】问题，即当entry1的长度发生变化，entry2的previous_entry_length就会更新，而entry2的更新，e2长度变化可能导致entry3的更新,...., 从而引发连锁更新。

连锁更新发生在结点插入或结点删除的时候，

1. 由于e1~eN的大小都是介于[250, 253]字节，更新e1的长度为介于[254,257]字节之间，就会触发e2~eN更新previous_entry_length；（插入结点）
2. 同理，删除导致连锁更新

所以，避免连锁更新，就要避免有过多的连续的、长度介于250~253字节的结点。



> 既然ziplist中的entry大小是不定的，那么ziplist的大小一开始该如何确定？



### 7、Redis对象系统

Redis并非直接使用数据结构实现键值对数据库，而是基于这些数据结构搭建一个对象系统，包括：字符串对象、列表对象、哈希对象、集合对象和有序集合对象。

使用对象的好处：

1. 根据不同场景，给对象设置某种底层实现方式，从而优化对象的使用效率；
2. 根据对象的类型判断给定命令是否可以执行；
3. 内存管理，基于引用计数的内存回收机制
4. 内存共享，基于引用计数实现对象的共享机制
5. 生命期管理，对象带有访问时间记录信息，空转时长较大的键会被优先删除。

#### 

创建键值对的时候，会创建键对象和值对象。

```c
typedef struct redisObject {
    unsigned type:4;
    unsigned encoding:4;
    void *ptr;
    //...
} robj;
```

Redis数据库的键对象类型，总是字符串对象；值对象类型，可以是任何其他类型。

使用`OBJECT ENCODING key`查看key对应的值对象的编码类型



#### 字符串对象

根据值对象的字符串长度选择不同的数据结构

- 字符串长度 > 32字节，选择 SDS，encoding 为raw
- 字符串长度 <= 32字节，选择SDS，encoding为embstr



值对象是以下类型的，对应的编码类型

- 可以用Long表示的：int

- long double的浮点数也是使用字符串保存的：embstr or raw
- 长度大于long double的表示范围的：embstr or raw

使用int编码的，也可以被转raw类型，如执行APPEND操作

embstr值对象，实际上是只读的，当对embstr执行修改的时候，embstr会转为raw。



字符串类型的值对象的操作命令集合：（待补充）



#### 列表对象

列表对象的编码类型可以是 ziplist 或者 linkedlist

- 编码转换

使用ziplist的条件：（注，通过修改redis配置文件可以修改这2个上限值）

1. 列表对象保存的所有字符串元素的长度都小于64字节
2. 列表对象保存的元素数量小于512个

不满足以上2个条件的，使用linkedlist

不管是使用ziplist还是linkedlist，都是可以双向操作的。



列表对象的操作命令集合：

（待补充）



#### 哈希对象

哈希对象的编码可以是 ziplist 或者 hashtable （字典）

- 如果使用ziplist，每次insert键值对的时候，先往尾巴放入 键对象，然后再放入值对象。
- 如果使用dict，则键值对都是使用字典键值对来保存



编码转换的条件，当同事满足以下2个条件时使用ziplist，否则使用dict

1. 哈希对象所有键值对中，键和值的长度都小于 64  字节；
2. 哈希对下你给的键值对数量 < 512个；

注：同样可以通过修改redis配置文件来修改上限值



哈希对象的命令集合：

（待补充）



#### 集合对象

集合对象的编码可以是  `intset` 或者  `hashtable`。

- 当使用整数集合实现
- 当使用hashtable实现，字典键是字符串对象（集合元素），字典的值都被置为NULL



编码转换

满足如下2个条件时才使用 intset 编码

1. 集合对象保存的所有元素都是整数值
2. 集合对象保存的元素数量不超过512个



集合对象的操作命令集合：（待补充）



#### 有序集合对象

使用的编码可以是 ziplist 或者  skiplist

为实现有序

- 当使用ziplist编码，使用压缩列表作为底层实现，每个集合元素需要使用2个连续的压缩列表节点来保存，1个用于保存元素本身，另一个用于保存元素的分数（用于排序，分数小的在前）

- 当使用skiplist编码，有序集合对象使用zset结构作为底层实现

  ```c
  typedef struct zset {
      zskiplist 	*zsl;		///< 跳跃表
      dict 		*dict;		///< 字典
  } zset;
  ```

  > zset中的zskiplist和dict的作用分别是什么？

  - zskiplist就是按分数有序地存储元素值
  - dict就是将元素值和分数作为映射，方便在O(1)时间内获取元素值的分数



编码转换，使用ziplist需要满足2个条件

1. 有序集合保存的元素个数 < 128 个
2. 有序集合保存的所有元素的长度都小于 64 字节



有序集合的操作命令：（待补充）



#### 类型检查和命令多态

命令多态：

Redis操作键的命令可分为2类：

- 公共类型命令：所有键类型都可以使用，如 DEL、EXPIRE、RENAME、TYPE、OBJECT

- 特定类型命令，如：

  | 类型         | 命令                       |
  | ------------ | -------------------------- |
  | 字符串对象   | SET、GET、APPEND、STRLEN   |
  | 哈希对象     | HDEL、HSET、HGET、HLEN     |
  | 列表对象     | RPUSH、LPOP、LINSERT、LLEN |
  | 集合对象     | SADD、SPOP、SINTER、SCARD  |
  | 有序集合对象 | ZADD、ZCARD、ZRANK、ZSCORE |



类型检查



#### 内存回收和对象共享



#### 对象的空转时长

```c
struct redisObject {
    type;
    encoding;
    redcount;
    ptr;
    lru;	///< 记录该对象最后一次被访问的时间
};
```

当对象被再次访问的时候，lru就被重置为最新的访问时间

使用`OBJECT IDLETIME key`来查看该对象的空转时长







