//
//  IRUIElement.h
//  IRAutomationKit
//
//  元素 (OPERATION.md 3.6 / 3.7): 抽象节点 = 表达式, 对应 CHRUIElement。只有取结果时才执行
//  (count / exists / forEach / node(s) / 取值 / 操作); 存下来的元素每次使用都重新执行一遍, 按那时的模式和刷新点。
//
//  三种来源, 任选其一 (同 CHRUIElement 的 uiObject / backingQuery / parent):
//  - UI 对象: 视图节点 (node.element)、控制器 (页面 Agent 的 element)、窗口、应用 (app.element);
//    查询和操作时换算到视图 (同 CHR 的 objectView): 控制器取 view, 应用取 keyWindow。应用 / Agent 这类活对象每次执行都现拍 (元素不缓存执行结果, 不可变)
//  - 查询: query.element, 结果可以是 0..n 个; 在它上面继续 .query, 先执行并要求唯一 (CHR: "上一层过滤器不唯一")
//  - 父元素: 同 CHR 的 elementWithParent: (用途待补, 先保留结构)
//  多个结果: 查询元素就代表多个; 要一次解析出来反复用, 用 query.elements(diag) 得到 NSArray<IRUIElement *>,
//  每一项都是持有单个节点的元素 (同 CHR 的 allElements)
//  个数要求放在使用的地方: 单个的值和操作 (frame / text / tap …) 要求正好一个, count / exists / forEach / elements 不限;
//  不满足时失败, diag 写明数量和前几个节点的类名 / frame (同 CHR)
//  链对象不为 nil (同 CHR): 元素 / 查询永远不为 nil, 它持有的 UI 对象可以为 nil (如入口拍不到); 每个 block 执行前先判断,
//  是 nil 就写 diag ("元素对应的视图为空" …), 不崩。节点 / Agent 是具体对象, 可以为 nil, 不在链上流动
//  刷新: 元素本身不刷新; 从它开始的查询按 mode (默认 cache) 和链上的 refresh() 决定在哪里重拍 (IRUIElementQuery.h)
//
//      IRUIElement *comment = app.query.axLabels(@"评论").element;
//      comment.tap(diag);
//      NSArray<IRUIElement *> *cells = app.query.views(@"UITableView").visibleCells(nil).elements(diag);
//      for (IRUIElement *cell in cells) {                       // 每一项持有单个节点, 不再重新查
//          NSString *title = cell.query.views(@"UILabel").first().element.text(nil);
//      }
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <IRAutomationKit/IRDiagnostics.h>
#import <IRAutomationKit/IRUIElementQuery.h>
#import <IRAutomationKit/IRIdentity.h>
#import <IRAutomationKit/IRScrollMonitor.h>   // IRScrollAxis
#import <IRAutomationKit/IRApplication+IRInput.h>   // IRInputMonitor

@class IRScrollOperation;
@class IRDragGesture;
@class IRListScroller;

NS_ASSUME_NONNULL_BEGIN

@class IRUINode;
@class IRUIElementQuery;
@class IRUIElement;
@class IRUIElementActionContext;
@class IRApplication;
@class IRIdentity;

/// 元素动作的处理 (同 Agent 动作的写法, ACTION.md 第 21 项): app = 坐标操作都在它上面 (= context.app);
/// context = 这次动作的现场; diag = 调用方的诊断 (= context.diag)。返回成功 / 失败, 失败原因写进 diag
typedef BOOL (^IRUIElementActionBlock)(IRApplication *app, IRUIElementActionContext *context, IRDiagnostics *diag);

/// 一次元素动作的现场 (对应 Agent 动作的 IRActionContext)
@interface IRUIElementActionContext : NSObject
/// 动作名 + 说明
@property (nonatomic, readonly) IRIdentity *identity;
/// 被操作的元素
@property (nonatomic, readonly) IRUIElement *element;
/// 框架先解析出的唯一节点 (不是正好一个时不会进 block, 原因已写进 diag)
@property (nonatomic, readonly) IRUINode *node;
/// 所属应用: 点击 / 长按 / 滑动 / 输入这些坐标操作都在它上面 (OPERATION.md 4.2)
@property (nonatomic, readonly) IRApplication *app;
/// 这次调用的诊断 (调用方传入的; 传 nil 时框架临时建一个)
@property (nonatomic, readonly) IRDiagnostics *diag;
/// 调用时传入的参数
@property (nonatomic, copy, readonly) NSDictionary<NSString *, id> *params;
/// 取必需参数: 取不到在 diag 上报错, 返回 nil
@property (nonatomic, copy, readonly) id _Nullable (^getParam)(NSString *name);
/// 取可选参数: 取不到返回默认值
@property (nonatomic, copy, readonly) id _Nullable (^findParam)(NSString *name, id _Nullable defaultValue);
/// 要返回数据时写这里 (调用方从 invokeAction 的 result 出参拿回)
@property (nonatomic, strong, nullable) id result;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
@end

@interface IRUIElement : NSObject

#pragma mark 查询

/// 从这个元素开始查询: 范围是它的子树 (查询元素先执行, 要求正好一个)
@property (nonatomic, readonly) IRUIElementQuery *query;

/// 从这个元素开始的查询的默认刷新模式 (默认 cache); 查询上的 mode(…) / refreshWith(…) 优先
@property (nonatomic) IRUIRefreshMode mode;

#pragma mark 集合

/// 个数 (查询元素这时才解析)
@property (nonatomic, readonly) NSUInteger (^count)(IRDiagnostics *_Nullable diag);

/// 有没有
@property (nonatomic, readonly) BOOL (^exists)(IRDiagnostics *_Nullable diag);

/// 逐个遍历 (查询元素这时执行一次): 每一个都是持有单个节点的具体元素
@property (nonatomic, readonly) void (^forEach)(void (^block)(IRUIElement *element, NSUInteger index, BOOL *stop),
                                                IRDiagnostics *_Nullable diag);

/// 对应的视图节点: 正好一个才返回, 否则 nil, 原因写进 diag (取节点上更底层的能力: 远程读取 / agent / detach)
@property (nonatomic, readonly) IRUINode *_Nullable (^node)(IRDiagnostics *_Nullable diag);

/// 全部节点 (0..n; 查询元素这时执行一次)
@property (nonatomic, readonly) NSArray<IRUINode *> *(^nodes)(IRDiagnostics *_Nullable diag);

#pragma mark 单个的值 (要求正好一个; 不唯一 / 没有时返回空值, 原因写进 diag)

/// 屏幕坐标
@property (nonatomic, readonly) CGRect (^frame)(IRDiagnostics *_Nullable diag);

/// 点击位置 (visibleRect 的中心)
@property (nonatomic, readonly) CGPoint (^tapPoint)(IRDiagnostics *_Nullable diag);

/// 可见文字 (树里没有的按需远程补读)
@property (nonatomic, readonly) NSString *_Nullable (^text)(IRDiagnostics *_Nullable diag);

/// accessibilityLabel (按需填辅助信息并远程取)
@property (nonatomic, readonly) NSString *_Nullable (^axLabel)(IRDiagnostics *_Nullable diag);

#pragma mark 调试 (画在远程画板 RemoteCanvas 上: 所有应用之上, 不挡触摸; 坐标就是节点的竖屏 points)

/// 全局开关 (总闸, 默认 YES): 关掉后所有调试输出 (画框 / 点击标记 / HUD / 调试日志) 都静默
@property (class, nonatomic) BOOL debugEnabled;

/// 实例开关 (配置, 不是状态; 创建时继承所属应用的 app.debugEnabled, 默认 NO, 这里设了就覆盖): 全局和实例都开时,
/// 这个元素的操作自动显示 —— 点击位置标记 + HUD 文字 (如 "tap 推荐") + 每一步数量的调试日志。
/// 从它查出来的元素 (query.element / elements / wait 的结果) 继承这个开关。
/// 三层: IRUIElement.debugEnabled (总闸, 最高) > element.debugEnabled (实例) > app.debugEnabled (应用默认)
@property (nonatomic) BOOL debugEnabled;

/// 在元素区域短暂高亮, 显示 durationMillis 毫秒 (0 = 默认约 500ms); 背后是查询的先解析出第一个结果。显式调用只看全局开关
@property (nonatomic, readonly) void (^debugHighlight)(NSUInteger durationMillis);

/// 在元素区域画红色线框并标上文字, 显示 durationMillis 毫秒 (0 = 默认约 500ms); text 为空不显示文字。显式调用只看全局开关
@property (nonatomic, readonly) void (^debugOutline)(NSUInteger durationMillis, NSString *_Nullable text);

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

/// 元素动作 (在分类里实现: IRUIElementAction.m)
@interface IRUIElement (IRUIElementAction)

#pragma mark 动作 (同 Agent 的动作模式: 注册 + invokeAction; 要求正好一个; 只能在持有执行权的 UI 任务上调用)
//
//  分层: App 上是底层的坐标操作 (tapAt / longPressAt / 滑动 / 输入, OPERATION.md 4.2); 元素动作是它的封装 ——
//  解析出唯一节点 → 调试显示 → 执行注册的 block (block 里用 app 的坐标操作); Agent 动作是业务 (打开评论 …), 内部用元素动作。
//  元素上只有一张全局动作表; 同名后注册的覆盖前面的 (打日志), SDK 默认的 tap / longPress 也能这样替换。
//  元素不挂 userInfo (元素不可变): 动作要的数据在调用时经 params 传。
//
//  用户扩展三步: 1. 分类  2. +load 里注册动作  3. (可选) 封装快捷 block (带自己的前缀, 免得撞名)
//
//      @implementation IRUIElement (AwemeActions)
//      + (void)load {
//          [self registerAction:IRIdentityMake(@"doubleTap", @"双击") block:^BOOL(IRApplication *app, IRUIElementActionContext *context, IRDiagnostics *diag) {
//              CGPoint p = context.node.tapPoint;
//              return [app tapAt:p diag:diag] && [app tapAt:p diag:diag];
//          }];
//      }
//      - (BOOL (^)(IRDiagnostics *))aweme_doubleTap {
//          return ^BOOL(IRDiagnostics *diag) { return [self invokeAction:@"doubleTap" params:nil result:NULL diag:diag]; };
//      }
//      @end

/// 注册动作 (+load 里调): 全局一张表, 同名后注册的覆盖前面的 (打日志)
+ (void)registerAction:(IRIdentity *)identity block:(IRUIElementActionBlock)block;

/// 已注册的动作 (调试 / 列出能力用)
@property (class, nonatomic, readonly) NSArray<IRIdentity *> *actions;

/// 调用动作: 只能在持有执行权的 UI 线程上; 框架先解析出唯一节点 (不唯一失败) → 调试显示 → 执行 block →
/// 开始 / 结束各打一条 verbose 日志 (同 Agent 动作); 没注册这个动作失败。result 拿回 context.result
- (BOOL)invokeAction:(NSString *)actionID params:(nullable NSDictionary<NSString *, id> *)params
              result:(id _Nullable *_Nullable)result diag:(nullable IRDiagnostics *)diag;

/// 简写: 没有参数和返回值
- (BOOL)invokeAction:(NSString *)actionID diag:(nullable IRDiagnostics *)diag;

#pragma mark 内置动作的快捷 block (SDK 在 +load 里注册, 可以覆盖)

/// = invokeAction(@"tap"): 节点要几何可见, 点 tapPoint (visibleRect 的中心)
@property (nonatomic, readonly) BOOL (^tap)(IRDiagnostics *_Nullable diag);

/// = invokeAction(@"longPress", @{@"duration": 秒}): 节点要几何可见, 长按 tapPoint
@property (nonatomic, readonly) BOOL (^longPress)(NSTimeInterval duration, IRDiagnostics *_Nullable diag);

/// = invokeAction(@"typeText", @{@"text": 文字}): 往这个输入框输入 (标准模式)。已聚焦 (键盘开着且它是第一响应者) 直接输入;
/// 否则先点它 (要几何可见)、等键盘打开 (默认 3 秒) 再输入。输入本身走 App 的 inputText (OPERATION.md 4.6), 可能持续几秒、被中断打断。
/// 更多参数用 invokeAction: mode (IRInputMode) / clear (先清空) / timeout (等键盘)
@property (nonatomic, readonly) BOOL (^typeText)(NSString *text, IRDiagnostics *_Nullable diag);

/// = invokeAction(@"typeText", @{@"text": 文字, @"clear": @YES}): 先清空再输入
@property (nonatomic, readonly) BOOL (^clearAndTypeText)(NSString *text, IRDiagnostics *_Nullable diag);

/// = invokeAction(@"typeText", @{@"text": 文字, @"monitor": 监听}): 带监听输入, 每个字 / 词上屏时调 monitor, 返回 YES 停止后面的输入 (算成功)。
/// monitor 在 CHRTouchKit 的回调线程上: 只能查询, 不能点 (见 IRApplication+IRInput.h)。要先清空用 invokeAction 加 clear
@property (nonatomic, readonly) BOOL (^typeTextWithMonitor)(NSString *text, IRInputMonitor _Nullable monitor, IRDiagnostics *_Nullable diag);

#pragma mark 滚动

/// 滚动操作 (SCROLL.md 第 22 项): 要求正好一个元素且是 UIScrollView (或子类), 否则 nil, 原因写 diag。
/// = [IRScrollOperation operationWithScrollView:[IRUIScrollView …] diag:diag]; 一个对象 = 一次滚动
@property (nonatomic, readonly) IRScrollOperation *_Nullable (^scrollOperation)(IRDiagnostics *_Nullable diag);

/// 按比例滚一段 (SCROLL.md 第 46 项): 距离 = min + (max − min) × |percent| (|percent| 超过 1 按 1), percent 的正负是方向 ——
/// 竖直: 正 = 往下看 (偏移变大), 负 = 往上看; 水平: 正 = 往右看, 负 = 往左看。用滚动操作 (策略 auto, 允许偏差 = 距离的 10%), 滚完停在区间里返回 YES。
/// 要求正好一个元素且是 UIScrollView; 水平滚动暂不支持 (滚动操作只做竖直, ir-fh1.13), 返回 NO 并写 diag
///
///     listElement.scrollVertical(0.3, 200, 800, diag);    // 往下看 380pt
///     listElement.scrollVertical(-0.5, 200, 800, diag);   // 往上看 500pt
@property (nonatomic, readonly) BOOL (^scroll)(IRScrollAxis axis, CGFloat percent, CGFloat min, CGFloat max, IRDiagnostics *_Nullable diag);
/// = scroll(IRScrollAxisVertical, …): 正 = 往下看, 负 = 往上看
@property (nonatomic, readonly) BOOL (^scrollVertical)(CGFloat percent, CGFloat min, CGFloat max, IRDiagnostics *_Nullable diag);
/// = scroll(IRScrollAxisHorizontal, …): 正 = 往右看, 负 = 往左看 (暂不支持)
@property (nonatomic, readonly) BOOL (^scrollHorizontal)(CGFloat percent, CGFloat min, CGFloat max, IRDiagnostics *_Nullable diag);

/// 照 UIKit 滚动 API 的列表滚动 (SCROLL.md 第 49 项): 要求正好一个元素且是 UITableView / UICollectionView, 否则 nil, 原因写 diag。
/// 这个 diag 就是之后每次滚动写错误的 diag (滚动方法返回 BOOL, 失败后要不要 clearError 由调用方决定)。
/// = [IRListScroller scrollerWithScrollView:[IRUIScrollView …] diag:diag]
@property (nonatomic, readonly) IRListScroller *_Nullable (^scroller)(IRDiagnostics *_Nullable diag);

/// 合成拖动手势 (GESTURE.md): 要求正好一个元素且几何可见, 否则 nil, 原因写 diag。起点 = 元素可见部分的中心, state.view = 这个元素
@property (nonatomic, readonly) IRDragGesture *_Nullable (^dragGesture)(IRDiagnostics *_Nullable diag);

@end

NS_ASSUME_NONNULL_END
