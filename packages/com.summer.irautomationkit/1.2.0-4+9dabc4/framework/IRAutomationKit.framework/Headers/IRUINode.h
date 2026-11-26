//
//  IRUINode.h
//  IRAutomationKit
//
//  视图节点 (OPERATION.md 3.6): 进程里没有 UIView, 由节点代替。节点树里只有视图 (UIWindow 也是视图);
//  控制器 / 应用不是节点, 由元素 (IRUIElement) 表达, 用时换算到视图。
//  一个节点就是以它为根的一棵子树: 范围、刷新、分离只在节点上定义一次 (没有单独的"树"类型)。
//
//  - 树根可变: 从入口拿到的 (app.node / agent.node(diag)) 和 detach() 的结果是树根, refresh 整棵重拍、换一份新快照, 节点对象不换
//  - 树内节点不可变: 从树根查出来的 / parent / children 就是某份快照里的一个位置; 树根刷新后它仍是拿到时的值, 不失效也不跟着变;
//    对它 refresh 失败, 要先 detach() 成为新树的根
//  - 一棵树始终是同一时刻: 快照不可变, 刷新是整份换
//  - 远程调用只在节点上: 树根刷新时拍新快照; 按需读 (补读文字 / label / hitTest) 也由节点发起。直接读属性不触发远程, 没算过的为 nil / 0
//  - 节点是具体对象, 可以为 nil, 不在链上流动; 链式写法走元素 (node.query… = node.element.query…), 元素永远不为 nil
//  - 节点是底层接口: 平时用元素 (node.element.query… / app.element.query…), 要用更底层的能力 (agent、按需远程读) 再拿节点
//
//      IRUINode *root  = app.node;                                                 // 拍整屏, 得到树根
//      IRUINode *panel = root.query.views(@"Panel").element.node(diag);            // 树内节点
//      panel.refresh(diag);                                                        // 失败: 不是树根
//      panel = panel.detach();                                                     // 分离成新树的根 (不重拍)
//      panel.refresh(diag);                                                        // 成功: 只拍 panel 的子树
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <IRemoteKit/IRTypes.h>
#import <IRAutomationKit/IRDiagnostics.h>

NS_ASSUME_NONNULL_BEGIN

@class IRUIElement;
@class IRUIElementQuery;
@class IRViewAgent;

/// accessibilityLabel 的来源 (OPERATION.md 3.5)
typedef NS_ENUM(NSInteger, IRAccessibilityLabelSource) {
    IRAccessibilityLabelSourceUnknown = 0,   ///< 还没算 (辅助信息没填充)
    IRAccessibilityLabelSourceNone,          ///< 没有 label
    IRAccessibilityLabelSourceStored,        ///< 设置: setAccessibilityLabel: 存成关联对象 (按对象, 纯读内存判断)
    IRAccessibilityLabelSourceComputed,      ///< 重写: 类重写 accessibilityLabel 现算 (按类判断)
};

@interface IRUINode : NSObject

#pragma mark 树根与分离

/// 是不是树根 (入口拿到的、detach() 的结果); 只有树根能 refresh
@property (nonatomic, readonly, getter=isRoot) BOOL root;

/// 树根: 调远程整棵重拍 (以这个视图为根), 换上新快照, 节点对象不换; 之前从它查出来的节点仍是旧值。
/// 树内节点调用失败 ("只有树根能刷新, 先 detach()"); 视图已不在 / 拍不到 → 失败, 原因写进 diag
- (BOOL)refreshWithDiag:(nullable IRDiagnostics *)diag;

/// node.refresh(diag)
@property (nonatomic, readonly) BOOL (^refresh)(IRDiagnostics *_Nullable diag);

/// 从当前树分离出来, 成为一棵新树的根: 不重拍 (数据就是当前快照里以它为根的那部分), 原树不受影响; 要新数据再 refresh
@property (nonatomic, readonly) IRUINode *(^detach)(void);

/// 所在快照的拍摄时间 (树根 refresh 后更新; 树内节点是拿到时那份的)
@property (nonatomic, readonly) CFAbsoluteTime timestamp;

#pragma mark 基础 (拍树时就有)

/// 类名 (Swift 类带模块前缀, 如 AiJiaSuClientIos.FVHomeMessageView)
@property (nonatomic, copy, readonly) NSString *className;

/// 远程视图地址
@property (nonatomic, readonly) IRRemoteAddress address;

/// 深度 (树根为 0)
@property (nonatomic, readonly) NSUInteger depth;

/// 父节点 (同一份快照, 不超出这棵树); 树根为 nil
@property (nonatomic, readonly, nullable) IRUINode *parent;

/// 子节点 (同一份快照, 树序)
@property (nonatomic, copy, readonly) NSArray<IRUINode *> *children;

/// 屏幕坐标 (统一为竖屏 points, 与触摸一致; 横屏应用是转过的坐标)
@property (nonatomic, readonly) CGRect frame;

/// hidden / alpha / clipsToBounds / userInteractionEnabled (自身的值, 不含祖先)
@property (nonatomic, readonly, getter=isHidden) BOOL hidden;
@property (nonatomic, readonly) CGFloat alpha;
@property (nonatomic, readonly) BOOL clipsToBounds;
@property (nonatomic, readonly, getter=isUserInteractionEnabled) BOOL userInteractionEnabled;

#pragma mark 可见性 (本地算, 拍树时就有; OPERATION.md 3.6 第 1 层)

/// 几何可见的部分: hidden / alpha / 尺寸 / 在屏幕内 / 与所有 clipsToBounds 祖先取交集; 不可见为 CGRectNull
@property (nonatomic, readonly) CGRect visibleRect;

/// 露出来的比例 (visibleRect 面积 / frame 面积), 0 = 不可见
@property (nonatomic, readonly) CGFloat visibleRatio;

/// 点击位置: visibleRect 的中心 (不用 frame 中心, 被裁掉一部分也点在露出来的地方)
@property (nonatomic, readonly) CGPoint tapPoint;

#pragma mark 文字与辅助信息

/// 可见文字: 树里打印出的 (UILabel 的 text) 为免费; 其余 (YYLabel / 富文本等) fetchText 取过之后才有; 没有为 nil
@property (nonatomic, copy, readonly, nullable) NSString *text;

/// accessibilityLabel 的来源 (辅助信息按需填充, 整份快照一次)
@property (nonatomic, readonly) IRAccessibilityLabelSource axLabelSource;

/// accessibilityLabel 文字 (只对候选远程取); 没取过 / 没有为 nil
@property (nonatomic, copy, readonly, nullable) NSString *axLabel;

/// 取文字: 树里有的直接返回; 其余按类判断能不能响应 text / attributedText, 能响应的远程读 (约 0.5ms), 缓存在节点上; 没有为 nil
- (nullable NSString *)fetchTextWithDiag:(nullable IRDiagnostics *)diag;

/// 取 accessibilityLabel: 按需先填辅助信息 (整份快照一次), 有 label 才远程取文字, 缓存在节点上; 没有为 nil
- (nullable NSString *)fetchAXLabelWithDiag:(nullable IRDiagnostics *)diag;

/// node.fetchText(diag) / node.fetchAXLabel(diag)
@property (nonatomic, readonly) NSString *_Nullable (^fetchText)(IRDiagnostics *_Nullable diag);
@property (nonatomic, readonly) NSString *_Nullable (^fetchAXLabel)(IRDiagnostics *_Nullable diag);

/// 点得到 (OPERATION.md 3.6 第 3 层): 在 tapPoint 远程 hitTest, 命中自己或子视图才算; 键盘打开时直接返回 NO (hitTest 可能有副作用)
- (BOOL)isHittableWithDiag:(nullable IRDiagnostics *)diag;

#pragma mark Agent 与元素

/// 远程视图的代理 (Agent 池: 同一个视图同一个实例), 可注册 / 调用动作
@property (nonatomic, readonly, nullable) __kindof IRViewAgent *agent;

/// 以这个节点为 UI 对象的元素 (同 CHR 的 view.element)
@property (nonatomic, readonly) IRUIElement *element;

/// = element.query (入口简写): 从这个节点开始查询, 范围是它的子树
@property (nonatomic, readonly) IRUIElementQuery *query;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
