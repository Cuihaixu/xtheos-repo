//
//  IRListBrowseContext.h
//  IRAutomationKit
//
//  列表浏览的 步 和 上下文 (BROWSE.md), 步骤 block 两个参数各拿一个:
//  - 步 IRListBrowseStep: 这一轮 —— 一次询问 → 决定 → 执行。读这一步看到的 (阶段 / 可用事件 / info / 当前条), 在它上面做决定
//    (invoke / invokeWith / skip, 写法同 IRPageNode, 期望页面固定是调起 flow 事件的列表页); 执行后框架填上结果, 变成一条记录
//  - 上下文 IRListBrowseContext: 这个列表 —— 整个浏览一份; 业务可继承加自己的字段和计算指标, 注册时传上下文类
//  - 统计三层 (BROWSE.md 统计三层):
//    通用指标 general —— 框架自己记, 一定有 (时间 / 步数 / 事件次数 / 互动 / 决策来源 / 异常 / 结束);
//    业务指标 —— 框架定结构, 业务用 browse.info 事件填 (current_item → step.item / visible_items → visibleItems /
//      list_items → allItems / list_state + layout → listState), 没填就是 nil (step.item 例外: 没填是空条目, itemID nil、index -1);
//    计算指标 computed —— 框架用业务指标算 (展示 / 曝光 / 点击 分广告和普通内容、点击率、填充率 …), 用到的业务指标缺了就是 nil。
//    点击 / 曝光靠事件标签: browse.click / browse.exposure 的事件做成功一次, 算当前条一次
//  - 发服务器 (和本地一样): @{ context: 上下文.dictionaryValue (history 只带最近 N 步), step: 这一步.dictionaryValue }
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRDiagnostics.h>
#import <IRAutomationKit/IRPageNode.h>
#import <IRAutomationKit/IRPageAgent+IRListBrowse.h>

NS_ASSUME_NONNULL_BEGIN

#pragma mark - 业务指标 (框架定结构, 业务填)

/// 一个条目: 来自 browse.info 事件的 JSON (current_item / visible_items / list_items 里的一项)。
/// 位置的含义框架定; itemID 是业务的 ID, 框架只当不透明字符串用来区分 / 计数 (没有就退回用位置)
@interface IRListBrowseItem : NSObject
@property (nonatomic, readonly) NSInteger section;                  ///< JSON section, 没有 0
@property (nonatomic, readonly) NSInteger index;                    ///< JSON index, 没有 -1
@property (nonatomic, copy, readonly, nullable) NSString *itemID;   ///< JSON item_id
@property (nonatomic, readonly) BOOL hardAd, softAd;                ///< JSON hard_ad / soft_ad
@property (nonatomic, readonly) BOOL isAd;                          ///< 硬广或软广 (JSON is_ad 优先)
@property (nonatomic, readonly) double visibleRatio;                ///< JSON visible_ratio, 没有 0
@property (nonatomic, copy, readonly) NSDictionary *extra;          ///< 原始 JSON (业务随便放: 标题 / 作者 / 类型 …); 没有为空字典
/// 区分条目用的键: itemID, 没有就 "#section-index"; 都没有 nil
@property (nonatomic, copy, readonly, nullable) NSString *key;
/// 框架跨步累计 (按 key): 这一条上查看阶段花了几秒 / 做过这个事件没有
@property (nonatomic, readonly) NSTimeInterval watched;
@property (nonatomic, readonly) BOOL (^did)(NSString *action);
- (NSDictionary<NSString *, id> *)dictionaryValue;   ///< = extra + 位置
@end

/// 列表状态: 来自 browse.info 的 list_state (+ layout); 业务没提供为 nil
@interface IRListBrowseListState : NSObject
@property (nonatomic, readonly) BOOL scrolling;                     ///< 在滚 (每步开始前框架等它停)
@property (nonatomic, readonly) BOOL reachedEnd;                    ///< 到底了
@property (nonatomic, readonly) NSInteger loadedCount;              ///< 已加载多少条, 没有 -1
@property (nonatomic, copy, readonly, nullable) NSString *layout;   ///< 单双列等 (layout 事件)
@property (nonatomic, copy, readonly) NSDictionary *extra;          ///< 原始 JSON
- (NSDictionary<NSString *, id> *)dictionaryValue;
@end

#pragma mark - 通用指标 (框架记)

@interface IRListBrowseGeneral : NSObject
@property (nonatomic, readonly) NSInteger steps;                                             ///< 走了几步
@property (nonatomic, copy, readonly) NSDictionary<IRListBrowseStage, NSNumber *> *stageSteps;   ///< 各阶段几步
@property (nonatomic, readonly) NSInteger rounds;                                            ///< 查看了几轮 (滑动成功次数 + 1)
@property (nonatomic, readonly) NSInteger consecutiveSkips;                                  ///< 连续跳过了几次
@property (nonatomic, copy, readonly) NSDictionary<NSString *, NSNumber *> *actionCounts;    ///< 各事件做了几次 (成功)
@property (nonatomic, copy, readonly) NSDictionary<NSString *, NSNumber *> *actionFailures;  ///< 各事件失败几次
@property (nonatomic, copy, readonly) NSDictionary<NSString *, NSNumber *> *actionSeconds;   ///< 各事件累计用时
@property (nonatomic, readonly) NSInteger interactions;                                      ///< 互动: 查看阶段成功的事件 (含观看)
@property (nonatomic, readonly) NSInteger serverDecisions, localDecisions;                   ///< 谁决定的
@property (nonatomic, readonly) NSInteger remoteFailures;                                    ///< 远程决策失败几次 (累计)
@property (nonatomic, readonly) BOOL remoteDisabled;                                         ///< 连续失败熔断了 (这次浏览不再问)
@property (nonatomic, readonly) NSInteger exceptions, retries, backToList;                   ///< 异常几次 / 重试几次 / 回列表页几次
@property (nonatomic, copy, readonly, nullable) NSString *finish;                            ///< 怎么结束: exit (做了退出事件) / skip / abort; 还没结束 nil
@property (nonatomic, copy, readonly, nullable) NSString *exitAction;                        ///< 退出事件
- (NSDictionary<NSString *, id> *)dictionaryValue;
@end

#pragma mark - 计算指标 (框架用业务指标算; 依赖缺了 nil)

@interface IRListBrowseComputed : NSObject
/// 展示: 出现在屏幕上的不同条目 (依赖 visibleItems)
@property (nonatomic, strong, readonly, nullable) NSNumber *shown, *adShown, *contentShown;
/// 曝光: browse.exposure 的事件成功过的不同条目 (依赖 item)
@property (nonatomic, strong, readonly, nullable) NSNumber *exposed, *adExposed, *contentExposed;
/// 点击: browse.click 的事件成功次数 (依赖 item; 当前条是广告记广告点击)
@property (nonatomic, strong, readonly, nullable) NSNumber *clicks, *adClicks, *contentClicks;
/// 点击率 = 点击 / 展示, 或 点击 / 曝光 (分母为 0 / nil 时 nil)
@property (nonatomic, strong, readonly, nullable) NSNumber *adClickRate, *contentClickRate;
@property (nonatomic, strong, readonly, nullable) NSNumber *adClickRateByExposure, *contentClickRateByExposure;
/// 填充率 = 广告展示 / 展示
@property (nonatomic, strong, readonly, nullable) NSNumber *fillRate;
/// 最深到第几条 (依赖 item / visibleItems) / 列表总条数、还剩几条没看到 (依赖 allItems)
@property (nonatomic, strong, readonly, nullable) NSNumber *deepest, *total, *remaining;
/// 条目的键 (itemID / 位置): 展示过 / 曝光过 / 点过的 (按出现顺序; 依赖缺了 nil)
@property (nonatomic, copy, readonly, nullable) NSArray<NSString *> *shownKeys, *adShownKeys, *exposedKeys, *clickedKeys;
- (NSDictionary<NSString *, id> *)dictionaryValue;
@end

#pragma mark - 步

@interface IRListBrowseStep : NSObject

#pragma mark 这一步看到的 (框架填)

@property (nonatomic, copy, readonly) IRListBrowseStage stage;
/// 当前是不是这个阶段
@property (nonatomic, readonly) BOOL (^at)(IRListBrowseStage stage);
/// 这一阶段可用的事件 (含查询数据 userInfo) / 能不能做这个事件
@property (nonatomic, copy, readonly) NSArray<IRAvailableAction *> *actions;
@property (nonatomic, readonly) BOOL (^available)(NSString *action);
/// browse.info 事件的原始数据 (按事件名, JSON)
@property (nonatomic, copy, readonly) NSDictionary<NSString *, id> *info;
/// 当前这一条 (current_item; 业务没提供是空条目: itemID nil、index -1)
@property (nonatomic, strong, readonly) IRListBrowseItem *item;
/// 第几步 (从 1) / 开始后第几秒
@property (nonatomic, readonly) NSInteger index;
@property (nonatomic, readonly) NSTimeInterval time;

#pragma mark 决定 (步骤 block 里调用; 选一个且只选一次)

/// 做: 在这一阶段的可用事件里执行 (不在 → 异常); 执行完检查还在列表页, 不在 → 异常
@property (nonatomic, readonly) BOOL (^invoke)(NSString *action);
@property (nonatomic, readonly) BOOL (^invokeWith)(NSString *action, NSDictionary<NSString *, id> *_Nullable params);
/// 跳过: 这一阶段不做 (查看 → 这条看完了; 滑动 → 不滑了; 退出 → 结束, 停在列表页)
@property (nonatomic, readonly) void (^skip)(void);

#pragma mark 结果 (执行后框架填)

@property (nonatomic, copy, readonly, nullable) NSString *action;   ///< nil = 跳过 / 还没决定
@property (nonatomic, copy, readonly, nullable) NSDictionary *params;
@property (nonatomic, readonly) BOOL skipped;
@property (nonatomic, copy, readonly, nullable) NSString *by;       ///< server / local; 还没决定为 nil
@property (nonatomic, readonly) BOOL ok;
/// 做的事件带的标签 (统计用: browse.click / browse.exposure …); 跳过为空
@property (nonatomic, copy, readonly) NSArray<NSString *> *actionTags;

- (NSDictionary<NSString *, id> *)dictionaryValue;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

#pragma mark - 上下文

@interface IRListBrowseContext : NSObject

/// 场景 (flow 事件名) / 参数 (已补默认值, 含服务器覆盖); 取值容错: 取不到 / 类型不对返回默认值
@property (nonatomic, copy, readonly) NSString *scene;
@property (nonatomic, copy, readonly) NSDictionary<NSString *, id> *params;
@property (nonatomic, readonly) id _Nullable (^param)(NSString *name);
@property (nonatomic, readonly) NSInteger (^intParam)(NSString *name);
@property (nonatomic, readonly) double (^doubleParam)(NSString *name);
/// 列表页 / 开始后几秒 / 执行过的步 (按顺序, 结果已填好; 不含当前这一步)
@property (nonatomic, copy, readonly) NSString *pageID;
@property (nonatomic, readonly) NSTimeInterval elapsed;
@property (nonatomic, copy, readonly) NSArray<IRListBrowseStep *> *history;

/// 通用指标 (一定有)
@property (nonatomic, strong, readonly) IRListBrowseGeneral *general;
/// 业务指标 (最近一步的; 业务没提供为 nil)
@property (nonatomic, copy, readonly, nullable) NSArray<IRListBrowseItem *> *visibleItems;
@property (nonatomic, copy, readonly, nullable) NSArray<IRListBrowseItem *> *allItems;
@property (nonatomic, strong, readonly, nullable) IRListBrowseListState *listState;
/// 计算指标
@property (nonatomic, strong, readonly) IRListBrowseComputed *computed;

/// 代理给的业务数据 (每步决策前代理在 WillDecide 事件里写 event.result; 没有代理 / 没写为 nil)
@property (nonatomic, copy, readonly, nullable) NSDictionary<NSString *, id> *extra;
/// 松散 (JSON): 调用方给的上层信息 (调用参数里的 outer) / 步骤 block 自己存的 (跨步保留)
@property (nonatomic, copy, readonly) NSDictionary<NSString *, id> *outer;
@property (nonatomic, strong, readonly) NSMutableDictionary<NSString *, id> *user;
/// 诊断 (= flow 事件的 diag)
@property (nonatomic, strong, readonly) IRDiagnostics *diag;

/// 异常 block 里用: 当前不是列表页就一路 back (最多 3 次), 回到返回 YES
@property (nonatomic, readonly) BOOL (^backToList)(void);

#pragma mark 子类重写 (业务自定义计算指标在这里算)

/// 开始前: 从调用参数取自己的字段 (super 先调)
- (void)prepareWithParams:(NSDictionary<NSString *, id> *)params;
/// 每走完一步: 更新自己的字段 (super 先调; step 已填好结果, 框架的指标已更新)
- (void)didStep:(IRListBrowseStep *)step;
/// 序列化 (发服务器 / 日志 / 结果): super 的 + 自己的; 只放 JSON 类型
- (NSDictionary<NSString *, id> *)dictionaryValue;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
