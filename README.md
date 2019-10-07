# p2pdbg-通用android app调试器

#### 项目介绍
为啥要做这个东东呢，是因为之前做了一个android项目，需要大量的适配和调试，但是需要调试手机又经常不在身边，自己恰好熟悉windows和服务器开发的知识，于是于个项目就诞生了。开发的过程中接口尽可能的保证了接口的通用性，所以这个调试器可以应用于所有的android项目。


这个项目是一个在windows系统上对android程序进行调试的调试器，调试方式是通过运行在windows系统上的控制端通过网络向位于android系统的被调试端发送调试指令，android端收到调试指令后执行相应的调试动作，执行完成后将结果通过调试模块的sdk接口返回给调试器端。
整个的调试过程仅仅依赖于网络，即仅需要运行windows调试器的pc能连接互联网并且被调试的android端能够访问互联网即可。

运行在windows系统上的调试端截图
![主界面截图](https://images.gitee.com/uploads/images/2019/1007/171542_59a65864_498054.png "1111.png")

#### 软件架构

```
这个项目分为三个部分，部署在公网的服务端，运行在windows系统的调试发起端和运行在android的调试sdk。
服务器和调试器都是使用vs2008编译得，android的sdk是用android studio开发的android aar包。

调试器和服务端项目结构
├─ComLib     项目公共动态库，为p2pdbg项目和p2pserv工程提供通用的函数接口
├─p2pdbg     运行在windows系统上的调试器
└─p2pserv    部署公网的服务器（需要调试器和android app都能访问到）

android端调试sdk是一个封装好的aar包，为需要调用的android提供注册调试服务的接口
└─com.example.administrator.dbglibrary
    ├─CmdDef          调试协议标识定义文件    
    ├─DbgActivity     调试控制activity
    ├─DbgCmd          调试命令管理实现
    ├─DbgCompress     传输文件夹需要用到的压缩接口
    ├─DbgLogic        aar库对外暴露的接口，启动调试服务和注册调试命令等等
    └─DbgServ         网络服务，用于和服务器进行数据交换
```

#### 使用说明
需要动态调试的android app需要将调试库的aar包集成到工程里，集成方法和其他的aar包一样，首先将DbgLib.aar复制到libs目录下，然后修改app下的build.gradle配置文件加入如下字段，然后同步下工程就可以了：
![输入图片说明](https://images.gitee.com/uploads/images/2019/1007/181249_028e7af1_498054.png "2222.png")

然后如下注册调试命令，并在需要的时候启动调试服务：
![输入图片说明](https://images.gitee.com/uploads/images/2019/1007/181511_56b223b6_498054.png "3333.png")

最重要的接口有三个：

```
//注册调试命令，收到调试器发过来的命令后执行注册的handler方法
boolean registerCmd(String strCmd, String strDesc, DbgCmd.DbgCmdHandler handler)

//初始化调试sdk环境，strUnique是用来唯一标识一个调试终端的识别码，调试器通过这个unique来连接感兴趣的终端
boolean initDbgServ(String strUnique, Context ctx) 

//开始连接调试服务器接口，用于连接位于公网的服务器
boolean startDbgServ()
```

#### 参与贡献
