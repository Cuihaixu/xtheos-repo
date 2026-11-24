//
//  XTPath.h
//  XTCommand —— 把"用户在命令行里输入的路径"转成本进程能直接访问的真实路径
//
//  只在 roothide 上会变:越狱终端里 / 是 jbroot,而 xtheos 打的命令行程序看到的是真实文件系统。
//    相对路径(a、./b)        → 不变(进程当前目录本来就是真实路径)
//    /rootfs/…                → 去掉 /rootfs(终端里访问真实系统根的写法)
//    已是 jbroot 真实路径      → 不变(含 /.jbroot-)
//    其它绝对路径             → <jbroot>/…
//  rootful / rootless:原样(终端和程序看到的是同一个文件系统)。
//
//  文件类型参数(XTValueInputPath / OutputPath / …Directory)和 --config 已自动转换,handler 拿到的就是真实路径;
//  只有你自己声明成 XTValueString 的路径才需要手动调。
//
//  注意:这不是 xtheos_jbroot() —— 那个是给代码里写的"逻辑路径"用的(rootless 上会加 /var/jb)。
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

FOUNDATION_EXPORT NSString *XTResolveUserPath(NSString *path);

NS_ASSUME_NONNULL_END
