## 多Redis实例的实现



### 1、主从复制

`SLAVEOF master_host master_port`通过slaveof命令来实现从服务器 复制 主服务器，并且建立主-从服务的关系：主服务器的更新将会同步更新到从服务器，实现主从数据库的状态一致。

- 数据库同步
- 命令传播



1）旧版本的主从复制的缺陷 - SYNC 命令的缺陷

- 初次复制
- 断线后复制：处于命令传播阶段的主-从服务中断了复制，从服务器自动重连主服务器，然后继续复制主服务器，但效率低下（因为从头复制一遍）。



2）新版本的主从复制改进 - PSYNC命令

- 完整重同步  full resync：初次复制
- 部分重同步 partial resync：断连后复制



#### 1.1 PSYNC的实现

#### 1.2 主从复制过程

#### 1.3 心跳检测

命令传播阶段，从服务器每秒一次，向主服务器发送

`REPLCONF ACK <replication_offset>`

- 检测主-从服务器的网络连接状态

  如果主服务器超过1s没收到从服务器发来的 replconf ack 命令，主服务器认为主-从连接出现问题

  ```bash
  
  127.0.0.1:6379> INFO replication
  # Replication
  role:master		<-- 
  connected_slaves:1
  slave0:ip=127.0.0.1,port=6380,state=online,offset=3236,lag=0 <- lag:间隔上次心跳检测的秒数
  master_repl_offset:3236
  repl_backlog_active:1
  repl_backlog_size:1048576
  repl_backlog_first_byte_offset:2
  repl_backlog_histlen:3235
  
  --- 
  
  127.0.0.1:6380> INFO replication
  # Replication
  role:slave		<-- 
  master_host:127.0.0.1
  master_port:6379
  master_link_status:up
  master_last_io_seconds_ago:5
  master_sync_in_progress:0
  slave_repl_offset:3166
  slave_priority:100
  slave_read_only:1
  connected_slaves:0
  master_repl_offset:0
  repl_backlog_active:0
  repl_backlog_size:1048576
  repl_backlog_first_byte_offset:0
  repl_backlog_histlen:0
  ```

  

- 辅助实现min-slaves选项

  防止主服务器在不安全的情况下执行写命令，如有3个从服务的lag值超过10秒，此时主服务器写操作将可能无法同步到这3个从服务器，导致主-从数据库不一致。

  ```bash
  min-slave-to-write 3 #至少3个从服务器需要同步写 
  min-slave-max-lag 10 #至少有3个从服务的lag值都大于等于10秒
  ```

  

- 检测命令丢失

  如果主从服务发送给从服务器的命令丢失，那么当从服务器下一次发送replconf ack的时候，主服务器将会从replication_offset中发现有命令丢失。ACK确认机制就是可以在丢失的情况下实现重传。

  `REPLCONF ACK <replication_offset>`



### 2、Sentinel 哨兵系统

哨兵系统是保证Redis高可用行的解决方案。

