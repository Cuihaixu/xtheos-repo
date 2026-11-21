#ifndef CONDUIT_H
#define CONDUIT_H

/*
 * Conduit 伞头：一次引入全部三层。
 * 只用其中一层时直接引对应的头：
 *   <Conduit/bootstrap.h>   L1 服务名与端口
 *   <Conduit/connection.h>  L2 连接与消息收发
 *   <Conduit/objc.h>        L3 请求型与长连接型
 */

#include <Conduit/defs.h>
#include <Conduit/bootstrap.h>
#include <Conduit/connection.h>

#ifdef __OBJC__
#import <Conduit/objc.h>
#endif

#endif /* CONDUIT_H */
