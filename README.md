#                         code-review
[![Build Status](https://travis-ci.com/loodao/helloworld.svg?branch=master)](https://travis-ci.com/loodao/helloworld)

<a href="https://scan.coverity.com/projects/soway-code-direct-sensor">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/20845/badge.svg"/>
</a>
（上面的build|passing图标说明： 当passing是绿色，证明你代码提交上来是已经通过一系列的自动集成构建测试；当你提交代码之后passing颜色是红色，就是你的代码没有通过审查，你的邮件会收到不通过的原因并指出问题所在的代码行）

####  增加文档目录(目前正把文档移到 https://github.com/Soway-code/training)

​	documents : 放置与开发有关的资料，图片，说明文档等;

~~~
1. doxygen注释语法
2. 代码提交指引
3. 何为代码持续集成自动化部署？
~~~

#### 代码目录

​	提交每一个功能模块都建立一个目录，请看项目中的course_2_2就是一个功能模块；

一个功能模块相对独立，原则一个.c文件和一个.h文件

\src  --------->放置源代码.c文件

\include -------->放置源代码.h文件

#### 服务器端的代码风格自动审查

​	    在服务器端部署了google C/C++ 代码规则审查，并在其基础上修改符合我们公司实际开发的定义规则。

 现在我正测试用那个方式更便于大家推送的; 现在我正测试用那个方式更便于大家推送的（居于目前部署在github的连线速度，我正测试在码云gitee上实现，本周大家就可以两个架构都可以推送）

经过在gitee码云部署travis CI，在上传代码时候触发机制不能实现（经过与码云的后端运维客服沟通，原因时他们对travis CI的触发模式还没有支持），需要经过gitlab配置再触发，简单说明就是代码放在gitee，触发机制部署再gitlab<---->gitee联动，gitee运行持续集成系统；这样还不如直接gitlab集成（这又回到github一样吗，gitlab也是国外的，国内注册要耍小手段才行）；现在试下jenkins + gitee 

这个测试CI仓库部分已经可以可以与码云（gitee）同步；码云链接：https://gitee.com/strong_code/helloworld
