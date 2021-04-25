



### 数据库的使用



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

```c
struct redisDB {
    dict	*dict;	//数据库键空间，保存数据库中所有的键值对，键是对象的名字，值是数据对象如列表对象
};
```

> redis数据库的键空间，如果通过对象名快速定位到对象在字典中的位置？



#### 生存时间和过期时间

