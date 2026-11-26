#ifndef CONDUIT_OBJC_H
#define CONDUIT_OBJC_H

/*
 * L3 对象层：按使用场景抽象，连接、消息、激活、引用计数全部内隐。
 *
 * 两套互相独立、不继承的接口：
 *   请求型   CDTServer / CDTClient / CDTRequest / CDTResponse
 *            一发一收，服务名相当于 URL，服务端无状态
 *   长连接型 CDTServiceListener / CDTConnection / CDTMessage
 *            双向对等，服务端持有连接并可主动发；回调以代理为主，分类另给 block 形式
 *   通知     CDTNotificationCenter / CDTNotification / CDTNotificationToken
 *            谁都能发，按 key 订阅，尽力送达
 *   共享存储 CDTUserDefaults / CDTUserDefaultsObserverToken
 *            全设备共享的键值存储，照 NSUserDefaults，可按键限读写、可监听变化
 *
 * 需要完全控制连接与事件的用 <Conduit/connection.h>，
 * 需要直接操作端口的用 <Conduit/bootstrap.h>。
 */

#import <Foundation/Foundation.h>
#include <Conduit/defs.h>

NS_ASSUME_NONNULL_BEGIN

/*
 * 通信层错误：domain 即 @"com.summer.conduit"，code 取 conduit_code_t，
 * localizedDescription 为人读描述；对端给的原因（invalidateWithReason: 的字典）平铺在 userInfo 里。
 */
CONDUIT_EXTERN NSErrorDomain const CDTErrorDomain;

/* code 为 CONDUIT_INVALIDATED 时的失效原因（NSNumber）：CONDUIT_PEER_GONE / CONDUIT_PEER_CLOSED / CONDUIT_CLOSED */
CONDUIT_EXTERN NSErrorUserInfoKey const CDTInvalidationCauseErrorKey;

/* 库自动填充的 header 字段。用户设置同名字段会在发送时被覆盖。 */
CONDUIT_EXTERN NSString * const CDTReservedHeaderFieldPrefix;   /* @"CDT-" */
CONDUIT_EXTERN NSString * const CDTSenderPIDHeaderField;
CONDUIT_EXTERN NSString * const CDTSenderBundleIDHeaderField;
CONDUIT_EXTERN NSString * const CDTSenderPathHeaderField;
CONDUIT_EXTERN NSString * const CDTSentAtHeaderField;
CONDUIT_EXTERN NSString * const CDTReceivedAtHeaderField;
CONDUIT_EXTERN NSString * const CDTRespondedAtHeaderField;
CONDUIT_EXTERN NSString * const CDTResponderPIDHeaderField;
CONDUIT_EXTERN NSString * const CDTRequestIDHeaderField;
CONDUIT_EXTERN NSString * const CDTProtocolVersionHeaderField;

@class CDTResponse;


#pragma mark - 请求

CONDUIT_API @interface CDTRequest : NSObject

/* body 含非 plist 类型、或服务名非法时返回 nil */
+ (nullable instancetype)requestWithService:(NSString *)serviceName body:(NSDictionary *)body;

@property (nonatomic, readonly) NSString *serviceName;
@property (nonatomic, readonly) NSDictionary *body;

/* 0 表示用 CDTClient.defaultTimeout */
@property (nonatomic) NSTimeInterval timeout;

/* send 为 YES，post 为 NO；由库填写 */
@property (nonatomic, readonly) BOOL needsResponse;

/* 值只能是字符串，一个字段一个值，键大小写敏感 */
@property (nonatomic, readonly) NSDictionary<NSString *, NSString *> *allHeaderFields;
- (void)setValue:(nullable NSString *)value forHeaderField:(NSString *)field;
- (nullable NSString *)valueForHeaderField:(NSString *)field;

/* 以下仅服务端侧有值，内核提供，不可伪造 */
@property (nonatomic, readonly) pid_t senderPID;
@property (nonatomic, readonly, nullable) NSString *senderBundleID;

@end


#pragma mark - 响应

CONDUIT_API @interface CDTResponse : NSObject

/* 同步：结果当场就有 */
+ (instancetype)responseWithBody:(NSDictionary *)body;

/*
 * 异步：先返回占位，handler 随即返回、线程立刻释放。
 * block 同步执行，其中只应发起异步操作、不应阻塞；有结果时调 complete。
 * complete 只生效一次；超时仍未调用时库自动回超时错误，此后调用无效。
 */
+ (instancetype)asyncResponse:(void (^)(void (^complete)(CDTResponse *response)))block;

@property (nonatomic, readonly) NSDictionary *body;

/* 只装通信层失败；为空即通信成功。业务成败一律由 body 表达。 */
@property (nonatomic, readonly, nullable) NSError *error;

@property (nonatomic, readonly) NSDictionary<NSString *, NSString *> *allHeaderFields;
- (void)setValue:(nullable NSString *)value forHeaderField:(NSString *)field;
- (nullable NSString *)valueForHeaderField:(NSString *)field;

@end


#pragma mark - 服务端

CONDUIT_API @interface CDTServer : NSObject

@property (class, nonatomic, readonly) CDTServer *sharedServer;

/*
 * 注册即启动，注册表自己持有服务，用户无需持有任何对象。
 * queue 决定 handler 在哪个队列执行，也就决定并发度；传 nil 用库为该服务建的并发队列。
 *   串行队列 —— 按到达顺序逐条处理，需要保序时用；慢请求会拖住后面的
 *   并发队列 —— 可在 handler 内阻塞，各请求互不影响；每个在途请求占一条线程
 * 同时处理中的请求至多 16 个（asyncResponse: 返回即不占名额），满即暂停接收、由内核排队。
 * 单向请求（对端 post 来的）同样走 handler，此时 request.needsResponse 为 NO，返回值被忽略。
 */
- (BOOL)registerService:(NSString *)name
                  queue:(nullable dispatch_queue_t)queue
                handler:(CDTResponse *(^)(CDTRequest *request))handler
                  error:(NSError **)error;

/* 优雅注销：不再接新请求，已收下的处理完再关；收尾前服务名仍被占用 */
- (void)unregisterService:(NSString *)name;

@property (nonatomic, readonly) NSArray<NSString *> *registeredServices;

@end


#pragma mark - 客户端

CONDUIT_API @interface CDTClient : NSObject

@property (class, nonatomic, readonly) CDTClient *sharedClient;

/*
 * 连接由库按服务名缓存：没有或已失效即新建，建不通即报错。请求发出后失败如实报错，不自动重发。
 */

/* 同步，阻塞到有响应或失败 */
- (CDTResponse *)sendRequest:(CDTRequest *)request;

/* 异步，不占线程等回复；queue 传 nil 时回调在主队列 */
- (void)sendRequest:(CDTRequest *)request
              queue:(nullable dispatch_queue_t)queue
         completion:(void (^)(CDTResponse *response))completion;

/* 单向，不等响应。当场可判定的失败经 error 返回；发出之后的失败不再报告 */
- (BOOL)postRequest:(CDTRequest *)request error:(NSError **)error;

/* 请求未指定 timeout 时采用，默认 5 秒 */
@property (nonatomic) NSTimeInterval defaultTimeout;

@end



#pragma mark - 长连接型

@class CDTServiceListener;
@class CDTConnection;

/*
 * 连接只报告能不能发，不替业务存消息；发不出去的消息存不存、怎么压由业务定。
 * 连接不可逆：进入 Invalidated 即终结，不复活；要继续通信就新建连接。
 */
typedef NS_ENUM(NSInteger, CDTConnectionState) {
    CDTConnectionStateSetup       = 0,   /* 尚未激活 */
    CDTConnectionStateReady       = 1,   /* 可发 */
    CDTConnectionStateBusy        = 2,   /* 对端队列满；发送默认在超时内等空位，腾出空位后回到 Ready */
    CDTConnectionStateInvalidated = 3,   /* 终态，之后不再有任何回调 */
};


#pragma mark 消息

CONDUIT_API @interface CDTMessage : NSObject

/* body 含非 plist 类型时返回 nil */
+ (nullable instancetype)messageWithBody:(NSDictionary *)body;

@property (nonatomic, readonly) NSDictionary *body;

/* 仅发送时生效；0 表示用默认 5 秒。等空位与等回复共用这一个预算 */
@property (nonatomic) NSTimeInterval timeout;

/* 仅发送时生效。YES 即对端队列满时立即以 PEER_NOT_RECEIVING 失败、不等空位（如推通知，积压由业务处理） */
@property (nonatomic) BOOL failsImmediatelyWhenBusy;

@property (nonatomic, readonly) NSDictionary<NSString *, NSString *> *allHeaderFields;
- (void)setValue:(nullable NSString *)value forHeaderField:(NSString *)field;
- (nullable NSString *)valueForHeaderField:(NSString *)field;

@end


#pragma mark 代理

/*
 * reply：当场调即同步回，带走稍后调即异步回，单向消息不必调。
 * 只生效一次；block 释放时若从未调用，库自动回「对端未回复」，对端立即失败。
 */

@protocol CDTServiceListenerDelegate <NSObject>
@optional

/* 回调中持有 connection 即接受，不持有即拒绝；要附带原因先 invalidateWithReason:。须同步持有 */
- (void)listener:(CDTServiceListener *)listener didAcceptConnection:(CDTConnection *)connection;

- (void)listener:(CDTServiceListener *)listener
      connection:(CDTConnection *)connection
didReceiveMessage:(CDTMessage *)message
           reply:(void (^)(CDTMessage *reply))reply;

/*
 * 只报激活之后的变化，Ready 与 Busy 可能高频来回，回调里不要做重活。
 * error 仅在 Invalidated 时非空：客户端退出、客户端主动关闭、本端关闭。
 */
- (void)listener:(CDTServiceListener *)listener
      connection:(CDTConnection *)connection
  didChangeState:(CDTConnectionState)state
           error:(nullable NSError *)error;

@end


@protocol CDTConnectionDelegate <NSObject>
@optional

- (void)connection:(CDTConnection *)connection
 didReceiveMessage:(CDTMessage *)message
             reply:(void (^)(CDTMessage *reply))reply;

/*
 * 只报激活之后的变化，Ready 与 Busy 可能高频来回，回调里不要做重活。
 * error 仅在 Invalidated 时非空：PEER_GONE 为服务端退出，可新建连接重试；
 * 被服务端踢掉、本端关闭则不应再连。
 */
- (void)connection:(CDTConnection *)connection
    didChangeState:(CDTConnectionState)state
             error:(nullable NSError *)error;

@end


#pragma mark 服务监听（服务端）

/*
 * 每条连接一条串行队列：同一连接的接入、消息、状态依次投递，连接之间并发。
 * 接入回调返回前该连接的消息不投递；Invalidated 之后该连接不再有任何回调。
 * 回调慢只拖住本连接：积压满后对端收到 Busy。跨连接共享状态由业务自行加锁或传串行 queue。
 */
CONDUIT_API @interface CDTServiceListener : NSObject

- (instancetype)initWithServiceName:(NSString *)name;
- (instancetype)init NS_UNAVAILABLE;

@property (nonatomic, readonly) NSString *serviceName;
@property (nonatomic, weak, nullable) id<CDTServiceListenerDelegate> delegate;

/* 各连接回调队列的目标队列。nil 即连接间并发；传串行队列即所有连接的回调串行。激活后不可改 */
@property (nonatomic, nullable) dispatch_queue_t queue;

/* 注册服务名，开始接受连接。名字被占、引荐不可用经 error 返回 */
- (BOOL)activate:(NSError **)error;

/* 停止监听并注销服务名；已建立的连接不受影响 */
- (void)invalidate;

@end


#pragma mark 连接（两端对等）

CONDUIT_API @interface CDTConnection : NSObject

/* 客户端创建。只构造，不连；context 随握手交给服务端 */
- (instancetype)initWithServiceName:(NSString *)name context:(nullable NSDictionary *)context;
- (instancetype)init NS_UNAVAILABLE;

@property (nonatomic, readonly) NSString *serviceName;

/* 客户端侧。服务端接入的连接其事件走监听器的代理，此处设置无效 */
@property (nonatomic, weak, nullable) id<CDTConnectionDelegate> delegate;
@property (nonatomic, nullable) dispatch_queue_t queue;

/* 客户端：同步完成引荐查询与握手，成功即 Ready。服务不存在、被拒绝、模式不符经 error 返回 */
- (BOOL)activate:(NSError **)error;

@property (nonatomic, readonly) CDTConnectionState state;

/* 对端身份。前两项内核提供不可伪造；context 为客户端自带，可伪造 */
@property (nonatomic, readonly) pid_t peerPID;
@property (nonatomic, readonly, nullable) NSString *peerBundleID;
@property (nonatomic, readonly, nullable) NSDictionary *peerContext;

/*
 * 主动发给对端，两端通用。对端队列满时在 message.timeout 内等空位，
 * failsImmediatelyWhenBusy 为 YES 则立即以 PEER_NOT_RECEIVING 失败；Invalidated 时立即以 INVALIDATED 失败。
 * 在本连接的回调里同步 sendMessage: 立即以 WOULD_DEADLOCK 失败，改用异步形式。
 */
- (nullable CDTMessage *)sendMessage:(CDTMessage *)message error:(NSError **)error;

- (void)sendMessage:(CDTMessage *)message
              queue:(nullable dispatch_queue_t)queue
         completion:(void (^)(CDTMessage * _Nullable reply, NSError * _Nullable error))completion;

- (BOOL)postMessage:(CDTMessage *)message error:(NSError **)error;

/*
 * 优雅关闭：不再接收新消息，在途、待发、收到未回复的处理完再关；reason 平铺在对端失效 error 的 userInfo 里。
 * 要立即终止就直接释放连接，对端得 PEER_CLOSED。
 */
- (void)invalidateWithReason:(nullable NSDictionary *)reason;

@end


#pragma mark 分类：block 形式

/*
 * block 以代理实现：设置任一 block 即把 delegate 设为自身，再转调 block。
 * 只有一个出口——delegate 属性，最后设置者生效；代理与 block 整体二选一，不按事件混用，
 * 设了任一 block 后未设的那几个即不处理，不会落回原代理。
 */

@interface CDTServiceListener (Blocks)
@property (nonatomic, copy, nullable) void (^acceptHandler)(CDTConnection *connection);
@property (nonatomic, copy, nullable) void (^messageHandler)(CDTConnection *connection,
                                                             CDTMessage *message,
                                                             void (^reply)(CDTMessage *reply));
@property (nonatomic, copy, nullable) void (^stateUpdateHandler)(CDTConnection *connection,
                                                                 CDTConnectionState state,
                                                                 NSError * _Nullable error);
@end

@interface CDTConnection (Blocks)
@property (nonatomic, copy, nullable) void (^messageHandler)(CDTMessage *message, void (^reply)(CDTMessage *reply));
@property (nonatomic, copy, nullable) void (^stateUpdateHandler)(CDTConnectionState state, NSError * _Nullable error);
@end

#pragma mark - 通知

/*
 * 轻量、尽力送达，同 Darwin 通知。连接、重连、重新注册全在库内：首次使用时自动连上，
 * 断开后每秒重连，连上后自动重发全部注册。服务端不在时也可注册，连上后生效。
 */

/* 订阅方收不动时，服务端怎么处理发给这次注册的通知 */
typedef NS_ENUM(NSInteger, CDTNotificationPolicy) {
    CDTNotificationPolicyCoalesce = 0,   /* 只留最新一条（默认） */
    CDTNotificationPolicyDrop     = 1,
    CDTNotificationPolicyHold     = 2,   /* 全留，超上限丢最旧，恢复后按序补发；上限由服务端定 */
};

CONDUIT_API @interface CDTNotification : NSObject
@property (nonatomic, readonly) NSString *name;
@property (nonatomic, readonly, nullable) NSDictionary *userInfo;

/* 发布方身份，调试用 */
@property (nonatomic, readonly) pid_t senderPID;
@property (nonatomic, readonly, nullable) NSString *senderBundleID;
@end

/* 一次注册的凭据，取消注册用 */
CONDUIT_API @interface CDTNotificationToken : NSObject
@property (nonatomic, readonly) NSString *name;
@end

CONDUIT_API @interface CDTNotificationCenter : NSObject

+ (CDTNotificationCenter *)defaultCenter;
- (instancetype)init NS_UNAVAILABLE;

/* 无返回值；服务端不在时丢弃并打警告，库不缓存 */
- (void)postNotificationName:(NSString *)name userInfo:(nullable NSDictionary *)userInfo;

/* 每调一次即一份独立注册；queue 为 nil 时在库的后台串行队列回调 */
- (CDTNotificationToken *)addObserverForName:(NSString *)name
                                       queue:(nullable dispatch_queue_t)queue
                                  usingBlock:(void (^)(CDTNotification *notification))block;

- (CDTNotificationToken *)addObserverForName:(NSString *)name
                                      policy:(CDTNotificationPolicy)policy
                                       queue:(nullable dispatch_queue_t)queue
                                  usingBlock:(void (^)(CDTNotification *notification))block;

- (void)removeObserver:(CDTNotificationToken *)token;

@end

#pragma mark - 偏好设置

/*
 * 全设备共享的键值存储，按「域 + 键」组织，值限 plist 类型。读本进程缓存，不跨进程；
 * 写入立即在本进程生效，再由服务端保存并推给其他进程。连接、重连、重新同步全在库内，
 * 服务端不在时照常读写，连上后自动补发。
 *
 * 域名用插件自己的安装包名：库不校验域归属，不同插件用了同一域名即共享同一份数据。
 * 权限按进程身份、挂在每个键上，用于防误改而非安全边界：同一宿主里的插件身份相同。
 */

/* 一次监听的凭据 */
CONDUIT_API @interface CDTUserDefaultsObserverToken : NSObject
@property (nonatomic, readonly) NSString *domain;
@property (nonatomic, readonly) NSString *key;
@end

CONDUIT_API @interface CDTUserDefaults : NSObject

/*
 * 同一进程同一域返回同一实例；域名为空或过长时返回 nil。
 * 某域在本进程首次取实例时同步拉取该域数据，至多等 1 秒；服务端不在时立即返回，连上后再补齐并触发监听。
 */
+ (nullable instancetype)userDefaultsWithDomain:(NSString *)domain;
- (instancetype)init NS_UNAVAILABLE;

@property (nonatomic, readonly) NSString *domain;

/* 本进程在名单里的写法：有 bundle id 用 bundle id，否则为可执行文件路径（以越狱根为基准） */
@property (class, nonatomic, readonly) NSString *currentProcessIdentity;

/* 无读权限的键表现为不存在；键不存在时落到本进程默认值 */
- (nullable id)objectForKey:(NSString *)key;
- (nullable NSString *)stringForKey:(NSString *)key;
- (nullable NSArray *)arrayForKey:(NSString *)key;
- (nullable NSArray<NSString *> *)stringArrayForKey:(NSString *)key;
- (nullable NSDictionary<NSString *, id> *)dictionaryForKey:(NSString *)key;
- (nullable NSData *)dataForKey:(NSString *)key;
- (NSInteger)integerForKey:(NSString *)key;
- (float)floatForKey:(NSString *)key;
- (double)doubleForKey:(NSString *)key;
- (BOOL)boolForKey:(NSString *)key;

/* 有读权限的全部键值，含本进程默认值 */
- (NSDictionary<NSString *, id> *)dictionaryRepresentation;

/*
 * 无返回值，失败打警告：值非 plist 类型或过大当场不生效；
 * 服务端判无权限时本进程的值回到服务端的值，并触发一次监听回调。
 * value 为 nil 即删除。不改该键已有的读写名单。
 */
- (void)setObject:(nullable id)value forKey:(NSString *)key;
- (void)setInteger:(NSInteger)value forKey:(NSString *)key;
- (void)setFloat:(float)value forKey:(NSString *)key;
- (void)setDouble:(double)value forKey:(NSString *)key;
- (void)setBool:(BOOL)value forKey:(NSString *)key;
- (void)removeObjectForKey:(NSString *)key;

/*
 * 写值并同时设定读写名单；名单为 nil 或空即不限制。成员写法见 currentProcessIdentity。
 * 只有写名单为空、或本进程在写名单里时才允许。completion 在服务端保存后（或确定失败时）
 * 调用，error 的 code 取 conduit_code_t；服务端不在时等连上后再判，不阻塞调用方。
 * queue 为 nil 时在库的后台串行队列回调。
 */
- (void)setObject:(id)value
           forKey:(NSString *)key
          readers:(nullable NSArray<NSString *> *)readers
          writers:(nullable NSArray<NSString *> *)writers
            queue:(nullable dispatch_queue_t)queue
       completion:(nullable void (^)(NSError * _Nullable error))completion;

/* 只在本进程生效，不写入共享数据；多次调用合并，后设覆盖先设 */
- (void)registerDefaults:(NSDictionary<NSString *, id> *)defaults;

/*
 * 该键被任一进程（含本进程）改变或删除后回调。oldValue / newValue 为改变前后的生效值（含本进程默认值，
 * 无则 nil），只在两者不同时回调；oldValue 是本进程上次看到的值，合并掉的中间值不出现。
 * 短时间多次变化可能合并，最后一次回调的 newValue 必为最终值。无读权限的键不会回调。
 * 每调一次即一份独立监听；queue 为 nil 时在库的后台串行队列回调。
 */
- (CDTUserDefaultsObserverToken *)addObserverForKey:(NSString *)key
                                              queue:(nullable dispatch_queue_t)queue
                                         usingBlock:(void (^)(NSString *key, id _Nullable oldValue, id _Nullable newValue))block;

/* 返回后该监听不再开始新的回调（已排队未执行的也不执行）；在别的队列上正在执行的那次不受影响 */
- (void)removeObserver:(CDTUserDefaultsObserverToken *)token;

/* 移除本进程对该键的全部监听，含同进程里别处注册的（同进程同域是同一实例），返回后的保证同上。删除键本身不会移除监听 */
- (void)removeAllObserversForKey:(NSString *)key;

@end

NS_ASSUME_NONNULL_END

#endif /* CONDUIT_OBJC_H */
