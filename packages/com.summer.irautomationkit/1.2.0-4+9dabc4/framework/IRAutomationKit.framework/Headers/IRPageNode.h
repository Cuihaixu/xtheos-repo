//
//  IRPageNode.h
//  IRAutomationKit
//
//  页面节点 (FLOW.md): 流程层的原始写法, 很薄的一层封装 —— 期望页面 + 绑定的诊断。
//  屏蔽"取当前页面 → 比标识 → 找接收者 → 调它的动作 (带 node.diag) → 记结果"; 动作层 (Agent) 是底层, 不判断页面。
//  - 找接收者 (沿响应链, ACTION.md 第 24 项): 页面自己 (含继承) → 下面的子控制器 (叶子 → 页面) → 上面的容器 (页面 → 根),
//    第一个实现了的接 (context.agent 是它, context.page 是当前页面); 都没有就失败。只看屏幕中心命中的这条链:
//    下面盖着的页面、预加载的 cell 不在链上; 上面只有包着页面的容器 (首页导航栏的搜索、tab 栏这类共用按钮注册在容器上)
//  - 页面标识完全相等, 一个标识只代表一个页面; nil / @"" = 任意页面 (当前是哪个就交给哪个)
//  - 任意页面也要求当前显示的是页面 (有页面标识): 认不出页面一律失败, 不调用
//  - 页面不对立刻失败, 不等
//  - 只能在持有执行权的 UI 任务线程上用 (同动作层); 应用取当前 UI 任务的 application
//  - 诊断归调用方管: 节点不自动清错误; 一步失败处理完还要接着用, 先 clearError()
//  - 统一的失败处理 (重试 / 跳过 / 终止) 是上面流程会话 (IRFlowSession) 的事: session.page(pageID) 建的节点失败先交给 onException
//
//      IRPageNode *feed = IRPageNode(@"feed.recommend", diag);     // diag 传 nil = 节点自己建
//      feed.loop(@"watch", 5, ^(NSUInteger index, NSUInteger total, BOOL *stop) {
//          if (!feed.invoke(@"watch_video")) { *stop = YES; return; }
//      });
//      if (feed.invoke(@"video_info")) { NSDictionary *info = feed.result; }
//      IRPageNode(nil, diag).invoke(@"back");                      // 当前页面能 back 就执行
//      if (feed.diag.hasError) { ... feed.diag.errorMessage ...; feed.clearError(); }
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRDiagnostics.h>
#import <IRAutomationKit/IRIdentity.h>
#import <IRAutomationKit/IRActionSignature.h>

NS_ASSUME_NONNULL_BEGIN

@class IRAgent, IRScreenState, IRApplication, IRPageNodeLoopResult;

/// 循环/轮询/等待执行结果 (支持链式 .onExhausted)
@interface IRPageNodeLoopResult : NSObject
/// 链式耗尽回调: 仅当轮询耗尽所有次数/超时且未提前 *stop 时回调; 回调内可自定义 diag 报错
@property (nonatomic, readonly) IRPageNodeLoopResult * (^onExhausted)(void (^NS_NOESCAPE block)(void));
/// 本次执行是否无错误 (= diag.noError)
@property (nonatomic, readonly) BOOL ok;
/// 本次执行是否有错误 (= diag.hasError)
@property (nonatomic, readonly) BOOL hasError;
/// 本次执行是否无错误 (= diag.noError)
@property (nonatomic, readonly) BOOL noError;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
@end

/// 当前页面能调的一个动作 (+[IRPageNode availableActionsInScreen:] 的一项)
@interface IRAvailableAction : NSObject
/// 动作名 + 说明
@property (nonatomic, readonly) IRIdentity *identity;
/// 接收者: 页面自己, 或链上的子控制器 / 容器
@property (nonatomic, readonly) IRAgent *agent;
/// 签名 (入参 / 返回值 / 支持的模式 / 标签; 没声明时是空签名): 调试工具据此显示参数、找能查询的事件
@property (nonatomic, readonly) IRActionSignature *signature;
/// 查询数据 (ACTION.md 第 39 项): 按标签查时, 声明了返回值的事件用查询模式调一次 (不带参数), context.result 放这里
/// (如 "能点哪几条 cell"、"有哪些过滤词"); 没声明返回值 / 查询失败 / 不按标签查 (列全部) 为 nil
@property (nonatomic, readonly, nullable) id userInfo;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
@end

@interface IRPageNode : NSObject

/// 构造
/// - Parameters:
///   - pageID: 期望的页面标识 (完全相等); nil / @"" = 任意页面
///   - diag: 绑定的诊断, 之后每次调用都记在它上面; 传 nil 则节点自己建一个
+ (instancetype)nodeWithPageID:(nullable NSString *)pageID diag:(nullable IRDiagnostics *)diag;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

/// 期望的页面标识 (nil = 任意页面; 传 @"" 也存成 nil)
@property (nonatomic, copy, readonly, nullable) NSString *pageID;

/// 绑定的诊断: 调用失败的原因读它
@property (nonatomic, strong, readonly) IRDiagnostics *diag;

/// 最近一次 invoke / invokeWith 的 context.result (失败或动作没给结果为 nil; 每次调用开始时清掉)
@property (nonatomic, strong, readonly, nullable) id result;

#pragma mark 页面

/// 当前显示的是不是它 (任意页面: 识别出了页面就是); 读, 不写诊断
@property (nonatomic, readonly) BOOL (^isVisible)(void);

/// 尝试退回本页面 (若当前已在本页面直接返回 YES; 否则以裸节点连续执行 back 最多 5 次, 间隔 0.5s 直到回到 self.pageID);
/// 若 self.pageID 为 nil (任意页面节点) 或未回到本页, 返回 NO (不走 session.onException 防递归)
@property (nonatomic, readonly) BOOL (^backToSelf)(void);

/// 同 backToSelf, 可指定最大尝试次数与间隔
- (BOOL)backToSelfWithMaxAttempts:(NSUInteger)maxAttempts interval:(NSTimeInterval)interval;

#pragma mark 调用动作

/// 检查当前页面 → 找接收者 (页面自己 → 下面的子控制器 → 上面的容器) → [接收者 invokeAction:… diag:self.diag];
/// 页面不对 / 链上都没有 / 动作失败都返回 NO, 原因在 diag
@property (nonatomic, readonly) BOOL (^invoke)(NSString *actionID);
@property (nonatomic, readonly) BOOL (^invokeWith)(NSString *actionID, NSDictionary<NSString *, id> *_Nullable params);

#pragma mark 查询 / 可用性 (ACTION.md 第 32-37 项)

/// 查询: 同 invoke 找接收者, 用查询模式调 (事件里 context.isQuery 时把数据写进 result 就返回, 不动手);
/// 返回 result, 失败 nil (原因在 diag; 签名没声明返回值 = 不支持查询)。失败处理同 invoke (会话里交给 onException)
@property (nonatomic, readonly) id _Nullable (^invokeQuery)(NSString *actionID);
@property (nonatomic, readonly) id _Nullable (^invokeQueryWith)(NSString *actionID, NSDictionary<NSString *, id> *_Nullable params);

/// 按标签查当前可用事件 (ACTION.md 第 39 项): 检查当前页面 (不对返回空, 原因记成警告) → 同找接收者的顺序遍历
/// (页面 → 下面的子控制器 → 上面的容器, 同名只留会接的) → 带任一个标签的 → 声明了可用性的问可用性 (不可用跳过) →
/// 声明了返回值的用查询模式调一次, 结果放 userInfo。tags 为 nil / 空 = 不过滤、不查询。按名字排序
@property (nonatomic, readonly) NSArray<IRAvailableAction *> *(^availableActions)(NSArray<NSString *> *_Nullable tags);

/// 可用性: 当前是不是这个页面、链上有没有可用的接收者 (签名声明了可用性的会用可用性模式问)。只回答, 不交给会话;
/// 不可用的原因记成警告 (不算错误), 读 diag.warnings
@property (nonatomic, readonly) BOOL (^available)(NSString *actionID);
@property (nonatomic, readonly) BOOL (^availableWith)(NSString *actionID, NSDictionary<NSString *, id> *_Nullable params);

#pragma mark 循环 (每一轮开始前检查当前页面; 页面不对: 停止, 原因写 diag)

/// 循环 count 次 (count <= 0 = 不进循环; 无限循环用 repeat); *stop = YES 提前结束
@property (nonatomic, readonly) void (^loop)(NSString *_Nullable name, NSInteger count,
                                             void (^NS_NOESCAPE body)(NSUInteger index, NSUInteger total, BOOL *stop));

/// 一直重复, 由 *stop = YES 结束
@property (nonatomic, readonly) void (^repeat)(NSString *_Nullable name, void (^NS_NOESCAPE body)(NSUInteger iteration, BOOL *stop));

#pragma mark 条件循环与等待 (自带画板活动提示、页面依赖检测、任务检查点)

/// 带重试次数和间隔的条件循环 (非 Session 场景下最常用, 支持链式 .onExhausted):
/// - 每一轮开始前检查当前页面 (页面不对: 停止, 原因写 diag);
/// - 画板自动显示 "%@ 1/N" -> 失败重试时自动显示 "重试 %@ 2/N";
/// - 每次尝试前清空上一轮 diag 错误; 间隔走 task sleep (过检查点, 响应取消与中断);
/// - *stop = YES 提前终止循环 (无论是达标正常退出还是中途报致命异常退出);
/// - 次数耗尽且未提前 *stop 时: 默认写入失败诊断，或由链式 .onExhausted(^{ ... }) 自定义写入。
@property (nonatomic, readonly) IRPageNodeLoopResult * (^repeatUntil)(NSString *_Nullable name, NSUInteger count, NSTimeInterval interval,
                                                                      void (^NS_NOESCAPE body)(NSUInteger attempt, NSUInteger total, BOOL *stop));

/// 带超时时长的条件等待 (每隔 interval 轮询一次, 支持链式 .onExhausted):
/// - 每一轮开始前检查当前页面 (页面不对: 停止, 原因写 diag);
/// - 自动更新 RemoteCanvas 画板等待进度 (例如: "0.5/5.0s");
/// - 每次尝试前清空上一轮 diag 错误; 间隔走 task sleep (过检查点);
/// - *stop = YES 提前成功或终止等待;
/// - 超时且未提前 *stop 时: 默认写入超时诊断，或由链式 .onExhausted(^{ ... }) 自定义写入。
@property (nonatomic, readonly) IRPageNodeLoopResult * (^waitUntil)(NSString *_Nullable name, NSTimeInterval timeout, NSTimeInterval interval,
                                                                    void (^NS_NOESCAPE body)(BOOL *stop));

/// 带重试次数和间隔的条件循环
- (IRPageNodeLoopResult *)repeatNamed:(nullable NSString *)name
                                count:(NSUInteger)count
                             interval:(NSTimeInterval)interval
                                 body:(void (^NS_NOESCAPE)(NSUInteger attempt, NSUInteger total, BOOL *stop))body;

/// 带超时时长的条件等待
- (IRPageNodeLoopResult *)waitNamed:(nullable NSString *)name
                            timeout:(NSTimeInterval)timeout
                           interval:(NSTimeInterval)interval
                               body:(void (^NS_NOESCAPE)(BOOL *stop))body;

#pragma mark 等待

/// 等几秒 = 当前 UI 任务的 sleep: (过检查点: 取消 / 中断照常生效); 不检查页面
@property (nonatomic, readonly) void (^wait)(NSTimeInterval seconds);

#pragma mark 查询

/// 工具用 (任何线程, app.queue 除外; 不需要 UI 任务): 在这一轮屏幕上找接收者并用查询模式调, 返回 result, 失败 nil (原因写 diag)
+ (nullable id)queryAction:(NSString *)actionID params:(nullable NSDictionary<NSString *, id> *)params
                  inScreen:(nullable IRScreenState *)screen diag:(nullable IRDiagnostics *)diag;

/// 应用当前页面能调的动作 (同 availableActions(tags), 用 app 当前这一轮的感知结果): 页面的 (含继承) + 下面的子控制器的 +
/// 上面的容器的; 只列可用的, 同名只留会接的那个, 按名字排序; tags 不为空时只列带任一个标签的, 并查询数据放 userInfo。
/// 不检查期望页面、不需要 UI 任务 (给调试工具用, 如 amt ops; app.queue 上除外); 认不出页面返回空
+ (NSArray<IRAvailableAction *> *)availableActionsInApplication:(IRApplication *)app tags:(nullable NSArray<NSString *> *)tags;

#pragma mark 诊断

/// 最近一次调用是否有错误 (= self.diag.hasError)
@property (nonatomic, readonly) BOOL hasError;

/// 最近一次调用是否没有错误 (= self.diag.noError)
@property (nonatomic, readonly) BOOL noError;

/// 最近一次调用的错误信息 (= self.diag.errorMessage)
@property (nonatomic, copy, readonly, nullable) NSString *errorMessage;

/// 最近一次调用的错误码 (= self.diag.errorCode)
@property (nonatomic, readonly) NSInteger errorCode;

/// 清掉 diag 上的错误 (= diag.clearError()): 一步失败处理完, 还要接着用这个节点时调
@property (nonatomic, readonly) void (^clearError)(void);

#pragma mark 主动终止与断言

/// 主动终止并报错:
/// - 在 Session 中: 触发会话异常并在回调中报告 (带当前节点的 pageID), 中止后续流程;
/// - 在普通非 Session 中: 写入失败原因到绑定的 diag
@property (nonatomic, readonly) void (^raise)(NSString *reason);

/// 带标识的主动终止并报错
@property (nonatomic, readonly) void (^raiseNamed)(NSString *name, NSString *reason);

/// 前置断言: condition 为 NO 时触发 raise 终止流程并报错
@property (nonatomic, readonly) void (^ensure)(BOOL condition, NSString *reason);

/// 带标识的前置断言: condition 为 NO 时触发 raise 终止流程并报错
@property (nonatomic, readonly) void (^ensureNamed)(NSString *name, BOOL condition, NSString *reason);

- (void)raiseReason:(NSString *)reason;
- (void)raiseNamed:(nullable NSString *)name reason:(NSString *)reason;
- (void)raiseWithCode:(NSInteger)code name:(nullable NSString *)name reason:(NSString *)reason;

- (void)ensure:(BOOL)condition reason:(NSString *)reason;
- (void)ensureNamed:(nullable NSString *)name condition:(BOOL)condition reason:(NSString *)reason;

@end

/// IRPageNode(pageID, diag) = [IRPageNode nodeWithPageID:pageID diag:diag]
/// (函数式宏: 只有后面跟 "(" 才展开, 不影响 IRPageNode *feed 这样的类型名;
///  要给 IRPageNode 写分类 / 类扩展时先 #pragma push_macro("IRPageNode") + #undef, 写完 pop_macro)
#define IRPageNode(_pageID, _diag) [IRPageNode nodeWithPageID:(_pageID) diag:(_diag)]   // 参数名不能叫 diag: 会把选择子里的 diag: 也替换掉

NS_ASSUME_NONNULL_END
