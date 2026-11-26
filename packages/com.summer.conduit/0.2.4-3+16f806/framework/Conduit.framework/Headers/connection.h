#ifndef CONDUIT_CONNECTION_H
#define CONDUIT_CONNECTION_H

/*
 * L2 传输层：连接、消息收发、生命周期。
 *
 * 端口、dead-name 通知、回复端口复用全在本层内部，公开面不出现。
 * 连接不可逆：对端没了即 INVALIDATED，不复活，库不自动重连；要继续通信就新建连接。
 * 需要直接操作端口的用 <Conduit/bootstrap.h>。
 *
 * 两个方向不要混：主动发用连接（conduit_connection_send），
 * 回复某次请求用消息（conduit_message_reply）。
 *
 * 事件 handler 在本连接的串行队列上执行（通道型服务端每个 peer 各一条，peer 之间并发），只宜做轻量分发，
 * 耗时工作自行 dispatch 出去。只有异步发送可以指定回调队列，不指定即本连接的串行队列。
 */

#import <Foundation/Foundation.h>
#include <Conduit/defs.h>
#include <dispatch/dispatch.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct conduit_connection_s *conduit_connection_t;

/* 事件基类。conduit_message_t / conduit_error_t 可直接转为它。 */
typedef struct conduit_object_s *conduit_object_t;
typedef struct conduit_message_s *conduit_message_t;
typedef struct conduit_error_s *conduit_error_t;

typedef enum {
    CONDUIT_TYPE_MESSAGE     = 1,
    CONDUIT_TYPE_PEER        = 2, /* 新客户端接入，仅服务端 */
    CONDUIT_TYPE_INVALIDATED = 3, /* 连接终结，此后不再有任何事件 */
    CONDUIT_TYPE_BUSY        = 4, /* 对端队列满（状态通知，与发送标志无关） */
    CONDUIT_TYPE_READY       = 5, /* 对端队列腾出空位，又能发了；只跟在 BUSY 之后 */
} conduit_type_t;

/* 服务的模式在创建时定死，两端 MUST 一致，不一致得 CONDUIT_MODE_MISMATCH。 */
typedef enum {
    CONDUIT_MODE_REQUEST = 1, /* 请求型：客户端发起，服务端应答，服务端不能主动发 */
    CONDUIT_MODE_CHANNEL = 2, /* 通道型：长期存在，两端都能主动发 */
} conduit_mode_t;

/* timeout_ms 传 0 时采用 */
#define CONDUIT_TIMEOUT_DEFAULT_MS 5000

#ifdef __BLOCKS__
typedef void (^conduit_handler_t)(conduit_object_t object);
typedef void (^conduit_reply_t)(conduit_object_t result);
#endif

typedef void (*conduit_handler_func_t)(conduit_object_t object, void *context);
typedef void (*conduit_reply_func_t)(conduit_object_t result, void *context);

/* ---- 消息 ---- */

/* body 含非 plist 类型时返回 NULL。可复用：同一条消息可发给多个对端。
   单条数据编码后最多 10 MB（库自用的部分另算，不占这 10 MB）；编码在发送时做，超限由发送报 PAYLOAD_TOO_LARGE。 */
CONDUIT_API conduit_message_t conduit_message_create(NSDictionary *body);

CONDUIT_API NSDictionary *conduit_message_get_body(conduit_message_t self);
CONDUIT_API bool conduit_message_needs_reply(conduit_message_t self);

/* 只能成功一次；再次调用得 CONDUIT_INVALIDATED。
   因 reply 载荷不合法 / 超限而失败（BAD_PAYLOAD / PAYLOAD_TOO_LARGE）不消耗回复机会，可改后再回。
   延迟回复需先 conduit_object_retain(self) 住这条消息。 */
CONDUIT_API conduit_code_t conduit_message_reply(conduit_message_t self, conduit_message_t reply);

/* ---- 错误 ---- */

/*
 * 错误 = domain + code + userInfo，与线上 cdt.error、L3 NSError 同构。
 * userInfo 必有 CONDUIT_ERROR_DESCRIPTION_KEY（人读描述）；code 为 INVALIDATED 时另有
 * CONDUIT_ERROR_CAUSE_KEY（失效原因：PEER_GONE / PEER_CLOSED / CLOSED）；其余键为对端业务给的原因。
 */
#define CONDUIT_ERROR_DOMAIN @"com.summer.conduit"
#define CONDUIT_ERROR_DESCRIPTION_KEY @"cdt.description"
#define CONDUIT_ERROR_CAUSE_KEY @"cdt.cause"

CONDUIT_API NSString *conduit_error_get_domain(conduit_error_t self);
CONDUIT_API conduit_code_t conduit_error_get_code(conduit_error_t self);
CONDUIT_API NSDictionary *conduit_error_get_user_info(conduit_error_t self);

/* ---- 事件对象 ---- */

CONDUIT_API conduit_type_t conduit_object_get_type(conduit_object_t self);
CONDUIT_API conduit_connection_t conduit_object_get_connection(conduit_object_t self);

/* 类型不符返回 NULL。事件中只有 INVALIDATED 可 as_error；发送与激活的失败结果本身即 error。 */
CONDUIT_API conduit_message_t conduit_object_as_message(conduit_object_t self);
CONDUIT_API conduit_error_t conduit_object_as_error(conduit_object_t self);

/*
 * 消息与错误同为事件基类的子类型，引用计数统一走这两个函数。
 * 重载形式使子类型可直接传入，不必强转：conduit_object_retain(message) 即可。
 * 延迟回复即：retain 住带走，回复后 release。
 */
__attribute__((overloadable)) CONDUIT_API void conduit_object_retain(conduit_object_t self);
__attribute__((overloadable)) CONDUIT_API void conduit_object_retain(conduit_message_t self);
__attribute__((overloadable)) CONDUIT_API void conduit_object_retain(conduit_error_t self);

__attribute__((overloadable)) CONDUIT_API void conduit_object_release(conduit_object_t self);
__attribute__((overloadable)) CONDUIT_API void conduit_object_release(conduit_message_t self);
__attribute__((overloadable)) CONDUIT_API void conduit_object_release(conduit_error_t self);

/* ---- 服务配置 ---- */

/* 只用于 conduit_connection_create_service。创建服务时库拷贝一份，之后再改不影响已创建的服务。 */
typedef struct conduit_service_config_s *conduit_service_config_t;

CONDUIT_API conduit_service_config_t conduit_service_config_create(void);
CONDUIT_API void conduit_service_config_retain(conduit_service_config_t self);
CONDUIT_API void conduit_service_config_release(conduit_service_config_t self);

/*
 * 请求型服务处理请求的队列。请求按到达顺序投递：串行队列即逐条保序，并发队列即同时处理、不保序。
 * 不设置时用库为该服务建的并发队列。同时处理中的请求至多 16 个，满即暂停接收、由内核排队。
 * 通道型不支持：每条连接自带串行队列（连接内有序、连接间并发），设了即 activate 返回 UNSUPPORTED_CONFIG。
 */
CONDUIT_API void conduit_service_config_set_queue(conduit_service_config_t self, dispatch_queue_t queue);

/* ---- 连接 ---- */

/*
 * create 只构造对象：不注册服务名、不建连、不收消息，故无失败可言，
 * 参数非法直接断言。配置完 handler 后调 activate 才真正开始工作。
 * config 为 NULL 即全部默认；创建时拷贝，之后再改不影响本服务。handler 未设即 activate 返回 NO_HANDLER。
 */
CONDUIT_API conduit_connection_t conduit_connection_create_service(const char *service_name,
                                                                   conduit_mode_t mode,
                                                                   conduit_service_config_t config);

/* context 随握手交给服务端，可为 NULL。 */
CONDUIT_API conduit_connection_t conduit_connection_create(const char *service_name,
                                                           conduit_mode_t mode,
                                                           NSDictionary *context);

/*
 * 激活。服务端注册服务名，客户端同步完成引荐查询与握手。
 * 返回 NULL 表示成功；非 NULL 是 error 对象，归调用方所有，用完 conduit_object_release。
 * 服务端拒绝时，对端给的原因在 error 的 userInfo 里。
 * 建立期的全部失败都在这里报：
 *   服务端：NO_HANDLER（未设 handler）/ UNSUPPORTED_CONFIG（配置含本模式不支持的项）/ NAME_IN_USE / NOT_PRIVILEGED / NO_HOST；
 *   客户端：UNKNOWN_SERVICE / NO_HOST / MODE_MISMATCH / VERSION_MISMATCH / REJECTED，
 *   握手的 PEER_GONE / PEER_NOT_RECEIVING（没送到）/ TIMED_OUT（送到了但没回复，结果未知），
 *   context 不合法 / 超限的 BAD_PAYLOAD / PAYLOAD_TOO_LARGE（不查询、不联系服务端即返回）。
 * 客户端握手总共至多等 CONDUIT_TIMEOUT_DEFAULT_MS。
 *
 * 激活前不会投递任何事件，故 handler MUST 在此之前设好；激活后 handler 不可更改。
 *
 * 只适用于自己创建的连接（服务端监听、客户端连接）。PEER 事件里拿到的 peer 连接
 * 由库创建、天然已激活，不需要也不应调用本函数——接受与否靠引用计数，见下。
 */
CONDUIT_API conduit_object_t conduit_connection_activate(conduit_connection_t self);

CONDUIT_API void conduit_connection_retain(conduit_connection_t self);
CONDUIT_API void conduit_connection_release(conduit_connection_t self);

/*
 * 关闭连接，立即返回；收尾后本端与对端都收到 CONDUIT_TYPE_INVALIDATED。reason 随之带给对端，可为 NULL。
 *   GRACEFUL ：不再接收新消息；本端发出的在途、待发与收到未回复的都处理完才真正关闭
 *   IMMEDIATE：收到未回复的由库代回 INVALIDATED + reason；在途立即以 INVALIDATED 结束
 * 优雅关闭进行中再调 IMMEDIATE 即升级；其余重复调用为空操作。请求型客户端不通知服务端（服务端无状态）。
 *
 * peer 连接的接受与拒绝：库 create 出 peer 时引用计数为 1（库自己持有），投递 PEER
 * 事件；回调返回后库 release 一次。
 *   - 回调中 retain 了 → 计数仍 >0，连接存活，此后开始投递消息（接受）
 *   - 回调中未 retain   → 计数归零，库向对端发出断开并销毁连接（拒绝）
 * 想附带拒绝原因就先 invalidate 再返回；不想附带直接返回即可。
 * 库保证在 PEER 回调返回之前不向该 peer 投递任何消息，故设 handler 不存在竞态。
 */
typedef enum {
    CONDUIT_INVALIDATE_GRACEFUL  = 1, /* 手头的做完再关 */
    CONDUIT_INVALIDATE_IMMEDIATE = 2, /* 手头的立即以错误结束 */
} conduit_invalidate_mode_t;

CONDUIT_API void conduit_connection_invalidate(conduit_connection_t self, conduit_invalidate_mode_t mode, NSDictionary *reason);

#ifdef __BLOCKS__
CONDUIT_API void conduit_connection_set_handler(conduit_connection_t self, conduit_handler_t handler);
#endif
CONDUIT_API void conduit_connection_set_handler_f(conduit_connection_t self,
                                                  conduit_handler_func_t handler,
                                                  void *context);

/*
 * 发送标志，可按位组合；0 即单向、队列满时在超时内等空位。
 *   NEEDS_REPLY：要对端回复。
 *   NO_WAIT    ：对端队列满时立即以 PEER_NOT_RECEIVING 失败，不等空位（如服务端推通知，积压由业务处理）。
 * 不论带不带 NO_WAIT，遇对端队列满都投递 BUSY，腾出空位投递 READY。
 */
typedef enum __attribute__((flag_enum)) {
    CONDUIT_SEND_NEEDS_REPLY = 1u << 0,
    CONDUIT_SEND_NO_WAIT     = 1u << 1,
} conduit_send_flags_t;

/*
 * 同步发送，阻塞到有结果。返回值归调用方所有，用完 conduit_object_release。
 *   不要回复：NULL 表示已送达对端队列，非 NULL 是 error
 *   要回复  ：message 是回复，error 是失败
 * 「已送达」只代表进了对端队列，不代表对端已处理。
 * timeout_ms 是整次的预算（等空位 + 等回复共用），0 即 CONDUIT_TIMEOUT_DEFAULT_MS。
 * 在本连接的串行队列上同步要回复，回复无人接收，立即以 WOULD_DEADLOCK 失败。
 */
CONDUIT_API conduit_object_t conduit_connection_send(conduit_connection_t self,
                                                     conduit_message_t message,
                                                     conduit_send_flags_t flags,
                                                     uint32_t timeout_ms);

/* 异步发送，立即返回，不占线程等空位或回复。completion 的取值规则同上；不关心结果时传 NULL。
   queue 为 NULL 时回调落在本连接的串行队列上。同一连接上的异步发送按调用顺序送出。 */
#ifdef __BLOCKS__
CONDUIT_API void conduit_connection_send_async(conduit_connection_t self,
                                               conduit_message_t message,
                                               conduit_send_flags_t flags,
                                               uint32_t timeout_ms,
                                               dispatch_queue_t queue,
                                               conduit_reply_t completion);
#endif
CONDUIT_API void conduit_connection_send_async_f(conduit_connection_t self,
                                                 conduit_message_t message,
                                                 conduit_send_flags_t flags,
                                                 uint32_t timeout_ms,
                                                 dispatch_queue_t queue,
                                                 conduit_reply_func_t completion,
                                                 void *context);

/* 对端 pid 由内核审计令牌给出，不可伪造；bundle id 在握手时据此推导并留存。
   对端不是 App（如守护进程）或无权查询其路径（如本端在沙盒内）时 bundle id 为 NULL。 */
CONDUIT_API pid_t conduit_connection_get_peer_pid(conduit_connection_t self);
CONDUIT_API NSString *conduit_connection_get_peer_bundle_id(conduit_connection_t self);

/* 对端建连时带的 context，可伪造，只作业务参数；无则为 NULL。 */
CONDUIT_API NSDictionary *conduit_connection_get_peer_context(conduit_connection_t self);

#ifdef __cplusplus
}
#endif

#endif /* CONDUIT_CONNECTION_H */
