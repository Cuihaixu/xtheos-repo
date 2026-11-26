#ifndef CONDUIT_BOOTSTRAP_H
#define CONDUIT_BOOTSTRAP_H

/*
 * L1：服务名与 mach 端口的对应，沙盒进程同样可用。
 *
 * 与 bootstrap_* 一一对应，参数顺序照抄，只去掉首个 bootstrap_port。
 * 本层只管「谁在哪」：拿到端口后直接 mach_msg 收发是正当用法。
 * 需要连接、字典载荷与超时的用 <Conduit/connection.h>。
 */

#include <Conduit/defs.h>
#include <mach/mach.h>
#include <dispatch/dispatch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* service_port 须持有接收权；本调用交出其发送权，接收权仍归调用方。 */
CONDUIT_API conduit_code_t conduit_bootstrap_register(const char *service_name, mach_port_t service_port);

/* 注销:无此接口。服务名绑定在接收权上——释放接收权(mach_port_mod_refs RECEIVE -1)或进程退出,
 * 系统自动回收该名字(mach 模型,无"保留运行却摘名"的调用)。见 design.md 九之二。 */

/* 成功时 output_port 得到一份发送权，归调用方所有，用完 mach_port_deallocate。 */
CONDUIT_API conduit_code_t conduit_bootstrap_look_up(const char *service_name, mach_port_t *output_port);

/* 【预留接口，非主 API】由系统(launchd)建名并交出接收权。broker 模型的主路径不用它——
 * 真沙盒里经宿主 look_up 仍会被宿主自身沙盒 deny；保留备将来类 XPC(预声明 + 按需拉起)语义。
 * 主用 register / look_up。成功时 output_port 得一份接收权，归调用方，用完 mach_port_mod_refs(..., RECEIVE, -1)。 */
CONDUIT_API conduit_code_t conduit_bootstrap_check_in(const char *service_name, mach_port_t *output_port);

/* ---- 端口死亡监控（裸 mach，MACH_NOTIFY_DEAD_NAME 的封装；断线重连由上层 L2 据此构建）---- */

/* 订阅句柄，不透明。cancel 前有效；cancel 后不得再用。 */
typedef struct conduit_dead_notify_s *conduit_dead_notify_t;

/* port 死亡时的回调；dead_port 为刚死的端口名。回调在 conduit_port_notify_dead 指定的 queue 上执行一次。 */
typedef void (^conduit_dead_handler_t)(mach_port_t dead_port);

/* 监控 port（发送权）的死亡：对端接收权消亡时，在 queue 上回调 on_dead 一次。
 * 返回订阅句柄（失败返回 NULL）。port 为 MACH_PORT_NULL、已是死名或 on_dead 为 NULL 即失败。
 * queue 为 NULL 时用库内部队列。本调用不接管 port 的所有权。
 * 同一端口可被多处订阅，互不影响。回调时 port 已成死名：调用方只释放自己持有的引用，
 * 死亡通知额外带来的那份由库处理。 */
CONDUIT_API conduit_dead_notify_t conduit_port_notify_dead(mach_port_t port,
                                                           dispatch_queue_t queue,
                                                           conduit_dead_handler_t on_dead);

/* 取消监控并释放句柄（= 唯一释放路径，MUST 调用一次，即使 on_dead 已触发过）。
 * handle 为 NULL 或已 cancel 的句柄：安全空操作。 */
CONDUIT_API void conduit_dead_notify_cancel(conduit_dead_notify_t handle);

#ifdef __cplusplus
}
#endif

#endif /* CONDUIT_BOOTSTRAP_H */
