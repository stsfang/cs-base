## MySQL必知必会

### 13、 分组数据

#### 分组和过滤分组

HAVING和WHERE类似

- WHERE用于过滤行记录
- HAVING用于过滤分组（和GROUP BY搭配使用），HAVING支持所有的WHERE操作

或者说，WHERE在数据分组前进行过滤，HAVING在数据分组后进行过滤。

#### 分组和排序

GROUP BY不保证分组之后，各组之间是排序的，让各组之间是有序的，还得在 GROUP BY 之后 使用  ORDER BY



#### SELECT子句的顺序

SELECT、FROM、WHERE、GROUP BY、HAVING、ORDER BY、LIMIT



### 14、子查询

#### 嵌套子查询，由内向外

- 单个列与单例列匹配
- 使用多个列
- 使用 IN 操作符，或者测试等于（=），不等于（<>）

#### 子查询的性能问题



#### 相关子查询

必须使用完全限定的列名，对于相同的字段名，分别指明来自哪个表，防止二义性。



### 15、连接表



外键：表B中的一列，包含了表A的主键值，外键定义了两个表之间的依赖关系；数据库需要维护引用的完整性

#### 等值连接 INNER JOIN

等值连接，也叫【内部连接，INNER JOIN】，是基于两个表之间的相等列的连接。

```sql
SELECT vend_name, prod_name, prod_price
FROM vendors, products
WHERE vendors.vend_id = products.vend_id
ORDER BY vend_name, prod_name;

----
```

这里的 WHERE 子句起到了过滤的作用：从vendors和products的完全连接（size是N1*N2的，笛卡尔积）中过滤出满足WHERE条件的那些行，所以表现为联结条件。

> 如何避免表连接的时候出现笛卡尔积？

显式地提供【联结条件】避免出现笛卡尔积

如果没有WHERE子句，将得到的是交叉联结（cross-join）的笛卡尔积的联结类型

使用 INNER JOIN - ON 的语法，相当于显式地指出内部联结关系

```sql
SELECT vend_name, prod_name, prod_price
FROM vendors INNER JOIN products
ON vendors.vend_id = products.vend_id;
```

> 使用隐式的联结+WHERE子句过滤，和 使用INNER JOIN - ON 之间，有性能区别吗？



联结多个表

如果使用内部连接的方式，连接多个表，空间复杂度将成规模增长（`N1*N2*N3....*Nn`），所以要避免连接不必要的表。

```sql
SELECT prod_name, vend_name, prod_price, quantify 
FROM orderitems, products, vendors
WHERE poducts.vend_id = vendors.vend_id
	AND orderitems.prod_id = products.prod_id
	AND order_num = 20005;
```

多表查询，有时候可以使用嵌套子查询、或者连接查询，选择哪一种更高效呢？

> 影响嵌套子查询，和内部连接查询的性能因素？

- 表数据量的大小
- 是否有索引
- 等等

#### 自联结

顾名思义，就是在同一张表上的自己联结

```sql
-- 子查询
SELECT prod_id, prod_name
FROM products
WHERE vend_id = (SELECT vend_id 
                FROM products 
                WHERE prod_id = 'DTNTR');

-- 内部联结
-- 使用表别名
SELECT prod_id, prod_name
FROM products AS p1,products AS p2 
WHERE p1.vend_id = p2.vend_id 
AND p2.prod_id = 'DTNRT';
```



#### 自然联结

自然联结的概念，多表联结的时候，相同列的出现次数只有一次；例如，表A和表B具有相同的列C，如果直接使用内部联结的话，列C在联结结果表中就会出现2次；

为了使得联结结果表中的列只出现一次，只能自己手动选择哪些列，一般用到通配符 SELECT  T1.* ，显式地指明查询的列，防止重复出现。



####  外部联结

> 什么场景下使用内部联结？什么场景下什么外部联结？

当查询的结果要求，不仅是匹配联结条件的那些记录，同时要查出没有匹配的记录。

- 如对每个客户下了多少订单进行计数，同时包括那些至今尚未下订单的客户（订单数为0，没有出现在订单表）

  ```sql
  -- 内部联结只能查出有订单的客户
  SELECT customers.cust_id, orders.order_num
  FROM customers INNER JOIN orders
  ON customers.cust_id = orders.cust_id;
  
  -- 外部联结:左联结,可以同时查出没订单（订单为空）的客户
  SELECT customers.cust_id, orders.order_num
  FROM customers LEFT OUTER JOIN orders
  ON customers.cust_id = orders.cust_id;
  
  ```

  左联结：左表X右表，左表部分一定不存在NULL，右表部分可能有NULL，所以左表的行可能对应NULL

  右连接：反之。

#### 带聚集函数的联结

内部联结：查询每个客户的订单量（大于0)，即对客户进行**分组**

```sql
SELECT customers.cust_name, customers.cust_id, COUNT(orders.order_num) AS num_ord
FROM customres INNER JOIN orders
ON customres.cust_id = orders.cust_id
GROUP BY customers.cust_id;
```

外部联结：查询每个客户的订单量（大于等于0）

```sql
SELECT customers.cust_name, customers.cust_id, COUNT(orders.order_num) AS num_ord
FROM customres LEFT OUTER JOIN orders
ON customers.cust_id = orders.cust_id
GROUP BY customers.cust_id;
```



### 16、组合查询

组合查询，说白，就是多个查询结果的并集，也叫复合查询。

```sql
-- 使用 UNION 关键字
-- GROUP BY 子句作用于复合查询的结果集

SELECT vend_id, prod_id, prod_price
FROM products
WHERE prod_price <= 5
UNION
SELECT vend_id, prod_id, prod_price
FROM products
WHERE vend_id IN (1001,1002)
GROUP BY vend_id,prod_price;

-- 使用 OR 关键字 

SELECT vend_id, prod_id, prod_price
FROM products
WHERE prod_price <= 5
OR vend_id IN (1001,1002);
```

可见，复合查询，在同一个表上，多次查询的列、表达式或聚集函数，应该是一样的才行（列出现的次序可以不一样，但是列数据类型必须是兼容的）

`UNION` 并集，自动去除了重复的行记录。如果要保留重复的行，那么使用 `UNION ALL`



### 17、全文本搜索

MySQL包括2种存储引擎：MyISAM和InnoDB

- MyISAM：支持全文本搜索
- InnoDB：不支持全文本搜索



### 22、视图CRUD

#### 创建视图

> 什么是视图？

视图可以理解为是对复杂SQL语句的封装，一般是复杂查询SQL；

- 使用视图，可以复用SQL查询
- 重新格式化检索数据结果
- 过滤数据（只保留想要的列）
- 将计算字段作为视图的列，简化计算字段的使用

视图不是指查询结果的数据集，每次使用视图的时候都需要从其他表中查询

- 视图名字必须唯一，不能与表名和其他视图名重复
- 视图数量无限制
- 视图可以嵌套
- 可以使用ORDER BY，外部的ORDER BY将覆盖视图的ORDER BY
- 视图无索引，无触发器和默认值
- 视图可以和表一起使用（视图本质是SQL语句嘛）

```sql
--- 视图，就是对一次查询SQL语句的封装

CREATE VIEW productcustomers AS
SELECT cust_name, cust_contact, prod_id
FROM customers, orders, orderitems
WHERE customers.cust_id = orders.cust_id
	AND orderitems,order_num = orders.order_num；
	
----

SELECT * FROM productcustomers;
----
SELECT cust_name, cust_contact
FROM productcustomers
WHERE prod_id = 'TNT2';

--- 格式化检索结果
SELECT Concat(RTrim(vend_name), '(', RTrim(vend_country), ')')
	AS vend_tiitle
FROM vendors
ORDER BBY vend_name;

---创建视图
CREATE VIEW vendorlocations AS
SELECT Concat(RTrim(vend_name), '(', RTrim(vend_country), ')')
	AS vend_tiitle
FROM vendors
ORDER BBY vend_name;

--- 简化计算字段
SELECT prod_id, 
		quantity, 
		item_price, quantity*item_price AS expanded_price
FROM orderitems
WHERE order_num = 20005;

---创建视图
CREATE VIEW orderitemsexpanded AS
SELECT prod_id, 
		quantity, 
		item_price, quantity*item_price AS expanded_price
FROM orderitems
WHERE order_num = 20005;

```

#### 更新视图

更新视图将会更新视图的基表，对视图的INSERT、UPDATE、DELETE实际上是操作视图的基表。

> 哪些视图不能更新？

- 视图中包括分组操作GROUP BY 和 HAVING
- 视图中包含多表联结的操作
- 视图中包括子查询
- 视图中包括并集UNION
- 视图中包括聚集函数Min()/Count()/Sum()等
- 视图中包括DISTINCT
- 视图中包括计算字段的列

可见视图的更新操作是非常有限的，因为视图的主要工作是数据检索，而不是数据更新。



### 23、存储过程

> 什么是存储过程，如何使用存储过程？

存储过程就是多个MySQL语句的集合，把多个处理封装为一个完整的过程，类似批处理，存储过程表现为函数。

- 简单：封装操作过程，复用性
- 安全：通过存储过程，限制对基础数据的直接访问，提高安全性
- 高性能：提高性能，存储过程比多个单独的SQL语句要快。



#### 创建存储过程



```sql
-- 创建存储过程
-- 需要临时修改语句分隔符，否则MySQL命令行客户端遇到 ; 就认为存储过程结束了
DELIMITER $
CREATE PROCEDURE productpricing()
BEGIN
	SELECT AVG(prod_prices) AS priceaverage
	FROM products;
END$
--- 恢复原始分隔符
DELIMITER ;

--- 执行存储过程
CALL productpricing()

--- 删除存储过程
DROP PROCEDURE productpricing;

```



#### 存储过程的参数

> 有哪些参数类型？如何使用

- IN 传入给存储过程
- OUT 从存储过程传出，使用INTO 关键字将查询结果赋值给 OUT 类型的参数
- INOUT 对存储过程传入和传出



示例1：

 ```sql
DELIMITER $
CREATE PROCEDURE productpricing (
	OUT p1 DECIMAL(8,2),
    OUT ph DECIMAL(8,2),
    OUT pa DECIMAL(8,2)
)
BEGIN
	SELECT MIN(prod_price) INTO p1 FROM products;
	SELECT MAX(prod_price) INTO ph FROM products;
	SELECT AVG(prod_price) INTO pa FROM products;
END $
DELIMITER ;

-- 调用存储过程
CALL productpricing(@pricelow, @pricehigh, @priceaverage);

-- 使用存储过程的执行结果
SELECT @pricehigh, @pricelow, @priceaverage;

 ```



示例2：

```sql
CREATE PROCEDURE ordertotal (
	IN in_number INT,
    OUT out_total DECIMAL(8,2)
)
BEGIN
	SELECT SUM(item_price*quantity)
	FROM orderitems
	WHERE order_num = in_number
	INTO out_total;
END;

-- 
CALL ordertotal(20005, @total);
SELECT @total;


```

示例3：

```sql
--

CREATE PROCEDURE ordertotal(
	IN in_number INT,
    IN taxable BOOLEAN,
    OUT out_total DECIMAL(8,2)
) COMMENT 'Obtain order total, optionally adding tax'
BEGIN
	-- 声明局部变量
	DECLARE total DECIMAL(8,20);
	DECLARE taxrate INT DEFAULT 6;
	
	SELECT SUM(item_price*quantity)
	FROM orderitems
	WHERE order_num = int_number
	INTO total;
	
	IF taxable THEN
		SELECT total+(total/100*taxrate) INTO total;
	END IF;
	-- 赋值
	SELECT total INTO out_total;

END;


-- 调用

CALL ordertotal(20005, 0, @total);
SELECT @total;

CALL ordertotal(20005, 1, @total);
SELECT @total;

```



### 24、游标

> 什么是游标？如何使用？

游标是存储在mysql服务器上的数据库查询结果集，通过游标可以在查询结果上进行滚动浏览。

MySQL中的游标只能用于存储过程。

使用游标，可以在存储过程中对查询结果的每一行进行处理。



#### 使用游标

- 使用repeat-until 循环读取

类似do - while 的循环结构

```sql
CREATE PROCEDURE processorders()
BEGIN
	DECLARE local INT;
	-- 定义一个查询结果集上的游标
	DECLARE ordernumbers CURSOR
	FOR
	SELECT order_num FROM orders;
	
	-- 定义一个 repeat handler
	DECLARE done BOOLEAN DEFAULT 0;
	-- SQL state 02000 是一个条件表示‘未找到’
	DECLARE CONTINUE HANDLER FOR SQLSTATE '02000' SET done=1;
	
	-- 创建一个表存储结果
	CREATE TABLE IF NOT EXISTS ordertotals
		(order_num INT, total DECIMAL(8,2));
	
	-- 打开游标
	OPEN ordernumbers;
	
	-- 循环读取
	REPEAT
		-- 游标读取
		-- FETCH 隐式地将移动 游标的内部指针到下一个行
		FETCH ordernumbers INTO local;
		
		-- 获取total, t是每个订单的价格合计（可含税）
		CALL ordertotal(local, 1, t);
		
		-- 插入到表
		INSERT INTO ordertotals (order_num, total) 
		VALUES(local, t);
		-- 
	-- 直到 done == 1停止repeat
	UNTIL done END REPEAT;
	
	-- 关闭游标
	CLOSE ordernumbers;
END;
```



### 25、触发器

> 什么是触发器？

指在表发生变动的时候自动处理某个事件。一般是响应如下改变表的语句（或者位于BEGIN和END之间的一组语句），MySQL只支持以下语句的触发器

- INSERT、UPDATE、DELETE



只有表才支持触发器、视图不支持触发器，临时表也不支持。

#### 创建触发器

1. 唯一的触发器名（最好是数据库范围内唯一，而不仅是表范围的唯一）
2. 触发器关联的表
3. 触发器监听的活动（INSERT/UPDATE/DELETE）
4. 触发器执行的时机(AFTER / BEFORE)

> 一个表上最多支持多少个触发器？

从触发器监听的活动和执行的时间来组合，最多有 6  个不同的触发器。每个活动有 AFTER 和 BEFORE 两个执行时机。

> 如果触发器执行失败呢？

- 如果BEFORE触发器执行失败，对应的活动将不会被执行。
- 如果BEFORE触发器或者语句本身失败，AFTER触发器也不会被执行。





- 示例1：

```sql
-- 触发器名：newproduct
-- 关联的表：products
-- 监听: INSERT
-- 执行时机：AFTER INSERT
-- 执行动作：FOR EACH ROW 回显 'Product added' 表示product添加成功
CREATE TRIGGER newproduct AFTER INSERT ON products
FOR EACH ROW SELECT 'Product added';

```

#### 删除触发器

```sql
DROP TRIGGER newproduct;
```



#### 使用触发器

> 什么的 NEW 虚拟表？有什么用？



- INSERT触发器
  1. INSERT触发器的代码内，可引用 `NEW` 虚拟表，访问被插入的行记录；
  2. 如果是 BEFORE INSERT 触发器，`NEW`中的值也可以被更新（允许更新被插入的值）
  3. 对于AUTO_INCREMENT列，`NEW`在实际INSERT执行之前的值是0，在INSERT执行之后才自动生成值。

```sql
-- AFTER INSERT触发器
-- 如果是BEFORE INSERT，则order_num未生成

CREATE TRIGGER neworder AFTER INSERT ON orders
FOR EACH ROW SELECT NEW.order_num;

-- use trigger
INSERT INTO orders(order_date, cust_id)
VALUES(Now(), 10001);


```

- DELETE触发器
  1. DELETE触发器的代码内，可引用一个`OLD`的虚拟表，访问被删除的行；
  2. `OLD`中的值都是只读的，不能写；

BEFORE DELETE触发器的好处：

1. 在删除之前先存档；如果存档失败，即BEFORE DELETE触发器执行失败，删除操作将不会被执行；

```sql
-- 使用OLD保存将被删除的行到一个存档表中
CREATE TRIGGER deleteorder BEFORE DELETE ON orders
FOR EACH ROW
BEGIN
	-- 需要事先创建 archive_orders 表
	INSERT INTO archive_orders(order_num, order_date, cust_id)
	VALUES(OLD.order_num, OLD.order_date, OLD.cust_id);
END;
```



- UPDATE触发器

  1. 在UPDATE触发器的代码内，可引用2个虚拟表：`NEW`表和`OLD`表，`OLD`是只读的；

     在`NEW`表访问UPDATE前的值；在`OLD`表访问UPDATE后的值。

  2. BEFORE UPDATE触发器中，通过`NEW`表可以更新将被更新的值。

  ```sql
  -- 在实际UPDATE之前，通过NEW表，将vend_state改为大写
  CREATE TRIGGER updatevendor BEFORE UPDATE ON vendors
  FOR EACH ROW SET NEW.vend_state = Upper(NEW.vend_state);
  ```

> MYSQL 5 的触发器有哪些优缺点?

1. 缺点：不支持CALL语句，所以在触发器代码内无法直接调用存储过程，必须将存储过程定义在触发器代码内才行
2. 优点：触发器可用于创建审计跟踪，把更改记录到另一个存档表中。





### 26、事务管理

事务处理用来维护数据库的完整性，保证成批的MySQL操作要么完全执行，要么完全不执行。

- 事务 transaction：一组SQL语句
- 回退 rollback：回滚操作
- 提交 commit：持久化事务过程，写入数据库表
- 保留点 save-point：事务回滚时的一系列保存点，指定回滚的粒度，支持部分提交和回滚。

> 如何讲一组SQL语句作为事务来处理？

需要将SQL语句划分为不同的逻辑块，并明确规定什么时候该回退，什么时候不回退。

> 哪些操作能够别回滚，哪些操作不能回滚？

- INSERT、UPDATE、DELETE语句可以被回滚
- SELECT语句不能回退（没意义）
- CREATE、DROP 也无法回退



#### 使用事务

使用事务之前，要对当前的数据库连接设置【非自动提交】；因为默认提交的设置会忽略`COMMIT`。

注：autocommit标志的有效范围是 一个MySQL连接，而不是整个MySQL服务。

```sql
-- 关闭默认的自动提交
SET autocommit=0;
```



示例1：

```sql
SELECT * FROM ordertotals;

-- 开启事务
START TRANSACTION
DELETE FROM ordertotals;
SELECT * FROM ordertotals;

-- 回退事务，隐式关闭事务
ROLLBACK;

SELECT * FROM ordertotals;

COMMIT;


```



#### 使用保留点

支持回滚部分事务，在事务处理块中合适的位置放置保留点，需要需要回退，只回退到某个保留点。

保留点越多，对事务的控制就更灵活，回滚的粒度就越细。

> 如何设置和释放保留点？

- SAVEPOINT：

- RELEASE SAVEPOINT：保留点在事务处理完成之后自动释放，也可以手动释放。





### 27、全球化和本地化

考虑到不同语言和字符集，数据库表的字符操作跟以下几方面有关：

- 字符集 character set
- 字符编码 
- 字符比较Collate：跟排序相关，ORDER BY子句

为了显式地说明表应该使用的字符集和字符比较方式，在创建表的时候指定字符集和校对方式

设置字符集的粒度

- 设置表的字符集和校对方式
- 设置列的字符集和校对方式（覆盖表的字符集和校对方式）



### 28、安全管理

#### 访问控制

只提供用户该有的访问权，不要赋予多余的权限。

- 用户角色
- 开发人员角色
- 管理员角色



#### 用户管理

MySQL管理用户的数据库表：mysql.user

- 创建用户、修改密码

  ```sql
  -- 创建
  CREATE USER steve IDENTIFIED BY 'password';
  
  -- 修改密码
  -- Password函数用于加密
  SET PASSWORD FOR steve = Password('jihuygyfy');
  
  -- 重命名
  RENAME USER steve TO steve2;
  
  -- 删除
  DROP USER steve2;
  
  
  ```

- 设置访问权限，使用GRANT语句，参数有：

  1. 需要授予的权限
  2. 被授予访问权限的数据库或表；
  3. 用户名：username@host

  ```sql
  -- 授予权限
  GRANT SELECT ON crashcourse.* TO steve;
  
  -- 查看权限
  SHOW GRANTS FOR steve;
  
  -- 移除权限
  REVOKE SELECT ON crashcourse.* FROM steve;
  
  
  ```

- 移除访问权限，使用REVOKE语句，GRANT的反向操作

- 权限的控制粒度

  1. 整个服务器
  2. 整个数据库：数据库级别的权限，如创建表
  3. 特定的表：表级别的权限，如表查询
  4. 特定的列：
  5. 特定的存储过程

  

### 29、数据库维护

- 数据备份

- 表检查

  ANALYZE TABLE：检查表上的键是否正确。

  CHECK TABLE ：

- 诊断启动问题

  mysqld --

- 查看日志文件：位于 /data/目录

  错误日志：hostname.err

  查询日志：hostname.log，记录MySQL活动，

  二进制日志：hostname-bin，记录更新过数据的所有语句。

- 缓慢查询日志：hostname.slow.log，记录执行缓慢的任何查询；用于**辅助排查数据库在什么地方需要优化**。