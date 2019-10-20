


MYSQL 技术圈

复制代码
有哪些做得好，又注重分享的公司： Oracle MySQL， MariaDB， Percona，Google， FB， Twitter， Taobao， NetEase…

有哪些值得关注的个人： Mark Callaghan、 Jeremy Cole、 Dimitri、 Peter ,Zaitsev、 Yoshinori Matsunobu …

微博上有哪些值得关注的账号： @姜承尧、 @淘宝丁奇、 @plinux、 @那海蓝蓝 …

业界有哪些好的会议： Percona Live、 FOSDEM、 MySQL Connect …

哪里去提问和找答案： Google、 StackOverflow …
复制代码
跟踪MySQL每个发行版和Bugs


霸爷说：在过去几年，跟踪Erlang，把Erlang语言每个版本/每个提交的变
更都看了一遍；
；

一个关于Oracle DBA的典故；
曾经有一位Oracle DBA，被Oracle公司请去对其公司内的Oracle研发人员
做关于Oracle系统的培训
复制代码
 

 哪些地方可以获取这些资料？

复制代码
WorkLogs
MariaDB： https://mariadb.atlassian.net/secure/Dashboard.jspa
MySQL： https://dev.mysql.com/worklog/
Percona： https://launchpad.net/percona-server
Bug 库
MySQL Bugs Home： http://bugs.mysql.com/
Percona Bugs Home： https://bugs.launchpad.net/percona-server
各发行版本
历史版本： http://downloads.mysql.com/archives/community/
当前版本： http://dev.mysql.com/downloads/mysql/
复制代码
 

我的经验
 
——定期更新自己的前期知识

复制代码

随着对MySQL系统理解的深入，此时应该定期回过头来看看自己早期

整理的笔记，撰写的文章，相信我，你一定会发现很多错误，嗤之以

鼻的想法。

无须沮丧，这说明你的能力提高了，更正他们。

个人经验

就InnoDB的锁实现一个功能，近三年内，每当有点新的思路，想

法，我就会去重新做测试，看代码，不断纠正自己的想法。

最近的一篇文章： 《 MySQL加锁处理分析》

但在我现在看来，这篇文章中仍旧有不正确的地方…

复制代码

——注重发散知识的积累

复制代码

看懂MySQL源码不是最终目标，当你觉得很多你看懂了，你就会有新的追求，此时，

也就意味着需要积累新的知识；

对MySQL的并发处理不满意？ Kernel_mutex？

需要学习并发编程的相关知识；

对MySQL单线程复制不满意？延迟严重？

需要学习MySQL现有复制的实现，进行多线程改造；

对MySQL压缩功能不满意？

了解业界成熟的压缩算法，尝试实现并替换；

对InnoDB引擎不满意？

自己做一个引擎，你需要进一步了解其他数据库/NoSQL/NewSQL的优点；

复制代码

——写在最后的建议

复制代码

能坚持到/看到这里的，那绝壁是真爱！！

赠送两个小小的建议


建议一： 从handler出发

MySQL插件式引擎，连接MySQL Server与各种存储引擎的，是其Handler

模块 —— hanlder模块是灵魂；

以InnoDB引擎为例，从ha_innodb.cc文件出发，理解其中的每一个接口的

功能，能够上达MySQL Server，下抵InnoDB引擎的内部实现；

建议二： 不放过源码中的每一处注释

MySQL/InnoDB源码中，有很多注释，一些注释相当详细，对理解某一个

函数/某一个功能模块都相当有用；
