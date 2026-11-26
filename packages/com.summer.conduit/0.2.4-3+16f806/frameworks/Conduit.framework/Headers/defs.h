#ifndef CONDUIT_DEFS_H
#define CONDUIT_DEFS_H

/*
 * Conduit 地基头：版本、导出宏、全局返回码。
 * 所有公开头以它开头，不含任何业务声明。
 */

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONDUIT_VERSION_MAJOR 0
#define CONDUIT_VERSION_MINOR 2
#define CONDUIT_VERSION_PATCH 9

/* 函数与 ObjC 类用 CONDUIT_API，常量用 CONDUIT_EXTERN——后者带 extern，
   否则头文件里的常量声明会成为暂定定义，多个编译单元引用即链接冲突。 */
#define CONDUIT_API __attribute__((visibility("default")))
#define CONDUIT_EXTERN extern __attribute__((visibility("default")))

/* 接口已定型、尚未实现：调用即编译失败，实现后去掉。 */
#define CONDUIT_UNIMPLEMENTED __attribute__((unavailable("Conduit: 尚未实现")))

/* 服务名上限，含结尾 '\0'。超长一律以 CONDUIT_BAD_NAME 拒绝，不截断。 */
#define CONDUIT_SERVICE_NAME_MAX 128

/*
 * 全库统一返回码。分段：
 *   -1xx 服务名与引荐   -2xx 连接与传输   -3xx 载荷   -9xx 库内部
 * 各层共用同一套，不按操作另立枚举。
 */
typedef enum {
    CONDUIT_OK                  =    0,

    CONDUIT_NO_HOST             = -100, /* 插件未安装、未生效，或宿主进程不在 */
    CONDUIT_BAD_NAME            = -101,
    CONDUIT_BAD_PORT            = -102,
    CONDUIT_NAME_IN_USE         = -103,
    CONDUIT_UNKNOWN_SERVICE     = -104,
    CONDUIT_NOT_PRIVILEGED      = -105,

    CONDUIT_PEER_GONE           = -200, /* 对端进程已退出 */
    CONDUIT_PEER_SUSPENDED      = -201, /* 保留，暂不使用：不识别挂起，只看队列满 */
    CONDUIT_PEER_NOT_RECEIVING  = -202, /* 对端队列已满，发不进去 */
    CONDUIT_TIMED_OUT           = -203,
    CONDUIT_INVALIDATED         = -204, /* 连接已被任一方关闭 */
    CONDUIT_VERSION_MISMATCH    = -205,
    CONDUIT_MODE_MISMATCH       = -206, /* 连接模式与服务端不符 */
    CONDUIT_NO_HANDLER          = -207, /* 无人处理：对端未回复即结束处理；或本端激活服务时未设置 handler */
    CONDUIT_REJECTED            = -208, /* 服务端拒绝接手这条连接 */
    CONDUIT_WOULD_DEADLOCK      = -209, /* 在本连接自己的队列上同步发送，回复无人接收；改用异步或换队列调用 */
    CONDUIT_PEER_CLOSED         = -210, /* 失效原因（userInfo cdt.cause）：对端主动关闭 */
    CONDUIT_CLOSED              = -211, /* 失效原因（userInfo cdt.cause）：本端关闭 */
    CONDUIT_UNSUPPORTED_CONFIG  = -212, /* 服务配置含本模式不支持的项（如通道型设了处理队列） */

    CONDUIT_BAD_PAYLOAD         = -300, /* 含非 plist 类型的值 */
    CONDUIT_PAYLOAD_TOO_LARGE   = -301,

    CONDUIT_ACCESS_DENIED       = -400, /* 偏好：本进程不在该键的写名单里 */
    CONDUIT_DOMAIN_FULL         = -401, /* 偏好：写入后该域总量超过上限 */

    CONDUIT_INTERNAL            = -900,
} conduit_code_t;

/* 返回静态字符串，不归调用方所有。 */
CONDUIT_API const char *conduit_code_get_name(conduit_code_t code);

CONDUIT_API bool conduit_available(void);

#ifdef __cplusplus
}
#endif

#endif /* CONDUIT_DEFS_H */
