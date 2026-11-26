//
//  IRUIElementQuery.h
//  IRAutomationKit
//
//  查询链 (OPERATION.md 3.7, 精确层, 对应 CHRUIElementQuery): 从一个元素开始 (element.query), 是表达式, 取结果时才执行。
//  收尾: element 不执行, 返回"这条查询"的元素 (使用时才执行; 取值 / 操作要求唯一); elements(diag) 立即执行成元素数组
//  核心是一层层过滤 (同 CHR): 每一步是一个单步查询对象 (持有上一步 + 变换 block), 从上一步的结果出发产出 0..n 个节点;
//  个数要求由使用方检查 (tap / 取值要正好一个, elements 不限 …)。命名: 产出一批节点的向下询问用复数, 条件是过滤
//  - 向下询问 (复数, 含自己): views / texts / axLabels —— 从当前每个节点出发, 在它自己和全部子孙里找 (同 CHR), 去重、树序
//  - 过滤: text / axLabel / …Matches / inRect / visible / hittable / interactive / filter / matching —— 只留下当前这批里满足的;
//    连续的过滤之间按代价从低到高重排 (每个条件声明要用哪些节点属性, 代价和取法由节点层决定); 远程读只对当前这批
//  - 跳转: 换一批节点 (子节点 / 子孙 / 父节点 / 祖先 / 兄弟 / 可见 cell);
//    不超出起点的范围: parent / ancestor 往上最多到起点节点本身, siblings 只在起点范围里找
//  - 刷新点: 链头和每两步之间。在一个刷新点刷新 = 对当前这批每个节点 detach + refresh (祖先也在这批里时只拍祖先);
//    刷不刷按优先级: refreshWith(…) / refresh() (只管所在的刷新点) > mode(…) (整次执行) > element.mode > cache
//    mode 是表达式的属性, 不是链上的一步: 写多次最后一次生效, 和位置无关
//  - 默认不按可见性筛 (同 CHR): 看得见看不见的都留下, 要可见的显式写 visible() / visibleMode(…) 过滤; tap 等操作本来就检查可见; 结果按树序
//  - 歧义默认是错误 (同 CHR): 在查询元素上操作 / 取值时要求正好一个, 0 个或多个都失败并在 diag 写明数量和各自的类名 / frame;
//    要从多个里挑, 在链上写 first() / last() / at(i)。执行时每一步的数量记进 diag 的 verbose (如 "views(UIButton) → 8 个")
//  - 文字匹配三层: 常用 texts / axLabels (向下询问)、text / axLabel (过滤), * 通配 → 正则 textMatches / axLabelMatches → 谓词 matching
//
//  * 通配: "评论" 完全相同; "*评论" 结尾; "相关搜索*" 开头; "*评论*" 包含; "*条*评论*" 按顺序包含几段。
//  只支持 *, 字面上的 * 写成 \*; 区分大小写 (要忽略用正则)
//
//      IRUIElement *comment = app.query.views(@"AWEFeedVideoButton").axLabel(@"评论").element;  // 不执行 (app.query = app.element.query)
//      comment.tap(diag);                                                                     // 这时才执行, 要求唯一
//      app.query.views(@"UIButton").text(@"评论").element;                                     // 向下找按钮, 再留下文字是"评论"的
//      app.query.texts(@"关注").first().element.tap(diag);                                     // 有多个, 显式取第一个
//      app.query.views(@"Panel").first().refresh().texts(@"AAA").first().element.tap(diag);    // 只重拍 Panel 的子树
//      app.query.mode(IRUIRefreshModeRefresh).views(@"AFCView").refreshWith(NO).views(@"UIButton").element;
//      app.query.views(@"AWEFeedVideoButton").filter(^BOOL(IRUIElement *e) {                   // 自定义: 子树里有 "…万" 的那个
//          return e.query.texts(@"*万").element.exists(nil);
//      }).element;
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <IRAutomationKit/IRDiagnostics.h>

NS_ASSUME_NONNULL_BEGIN

@class IRUIElement;

/// 可见性档次 (OPERATION.md 3.6), 递进: 高一档包含低一档的判断, 成本也更高
typedef NS_ENUM(NSInteger, IRViewVisibility) {
    IRViewVisibilityGeometric = 1,    ///< 几何可见 (= visible()): 隐藏 / 透明 / 尺寸 / 在屏幕内 / 被裁剪; 本地免费
    IRViewVisibilityHittable = 2,     ///< 点得到: 再在 tapPoint 远程 hitTest, 命中自己或子视图才算; 只对候选, 键盘打开时都不算
};

/// 刷新模式: 每个刷新点默认刷不刷 (OPERATION.md 3.6 "刷新: 刷新点 + 模式")
typedef NS_ENUM(NSInteger, IRUIRefreshMode) {
    IRUIRefreshModeCache = 0,     ///< 默认: 不刷新, 用起点所在那棵树现有的数据
    IRUIRefreshModeRefresh,       ///< 每个刷新点都刷新 (适合短链; 长链用 refreshWith(NO) 关掉不需要的)
};

@interface IRUIElementQuery : NSObject

#pragma mark 刷新

/// 这次执行的刷新模式 (表达式的属性, 不是一步): 写多次最后一次生效, 作用于整次执行的所有刷新点; 覆盖 element.mode
@property (nonatomic, readonly) IRUIElementQuery *(^mode)(IRUIRefreshMode mode);

/// 在这个刷新点刷新 (= refreshWith(YES)): 对当前这批每个节点 detach + refresh, 后面的步骤在新数据上执行
@property (nonatomic, readonly) IRUIElementQuery *(^refresh)(void);

/// 这个刷新点刷不刷, 覆盖 mode (如 refresh 模式下 refreshWith(NO) 关掉这一个)
@property (nonatomic, readonly) IRUIElementQuery *(^refreshWith)(BOOL refresh);

#pragma mark 向下询问 (复数; 从当前每个节点出发, 在它自己和全部子孙里找, 去重、树序)

/// 类名, 含子类 (沿类链判断, 纯内存, 按类缓存); Swift 类可以只写模块后面那部分; nil = 不限
@property (nonatomic, readonly) IRUIElementQuery *(^views)(NSString *_Nullable className);

/// 可见文字, * 通配 (树里没打印的远程补读, 只在这个范围里读, 缓存在节点上)
@property (nonatomic, readonly) IRUIElementQuery *(^texts)(NSString *pattern);

/// accessibilityLabel, * 通配
@property (nonatomic, readonly) IRUIElementQuery *(^axLabels)(NSString *pattern);

#pragma mark 过滤 (只留下当前这批里满足的; 连续的过滤之间按代价重排, 远程读只对当前这批)

/// 自己的可见文字, * 通配
@property (nonatomic, readonly) IRUIElementQuery *(^text)(NSString *pattern);

/// 自己的可见文字, 正则 (NSRegularExpression)
@property (nonatomic, readonly) IRUIElementQuery *(^textMatches)(NSString *regex);

/// 自己的 accessibilityLabel, * 通配
@property (nonatomic, readonly) IRUIElementQuery *(^axLabel)(NSString *pattern);

/// 自己的 accessibilityLabel, 正则
@property (nonatomic, readonly) IRUIElementQuery *(^axLabelMatches)(NSString *regex);

/// visibleRect 与区域相交 (屏幕坐标)
@property (nonatomic, readonly) IRUIElementQuery *(^inRect)(CGRect rect);

/// 可见: 只留下当前这批里几何可见的 (= visibleMode(IRViewVisibilityGeometric); 便宜, 相对准确)。默认不筛, 要可见的显式写
@property (nonatomic, readonly) IRUIElementQuery *(^visible)(void);

/// 指定可见性档次: 只留下当前这批里达到这一档的
@property (nonatomic, readonly) IRUIElementQuery *(^visibleMode)(IRViewVisibility visibility);

/// = visibleMode(IRViewVisibilityHittable)
@property (nonatomic, readonly) IRUIElementQuery *(^hittable)(void);

/// 接收触摸 (自己和所有祖先的 userInteractionEnabled 都是 YES)
@property (nonatomic, readonly) IRUIElementQuery *(^interactive)(void);

/// 自定义判断 (高度自定义的出口, 同 CHR filter): block 拿到每个候选的元素 (持有单个节点), 元素和它的节点的能力都能用 ——
/// 属性 (e.node(nil)…)、在子树里再查 (e.query…)、按需取贵的信息 (fetchText / fetchAXLabel / isHittable)、经 agent 远程读数据。
/// 在连续的过滤里最后执行, 只对前面条件筛剩的候选跑
@property (nonatomic, readonly) IRUIElementQuery *(^filter)(BOOL (^block)(IRUIElement *element));

/// 谓词: 属性名就是节点的属性 (className / text / axLabel / frame …); 执行前分析用到的属性, 需要的辅助信息整份快照先填好
@property (nonatomic, readonly) IRUIElementQuery *(^matching)(NSPredicate *predicate);

#pragma mark 跳转 (换一批节点)

/// 列表 / 集合视图里当前可见的 cell (按视觉顺序); 输入要正好一个列表, 多个报错; className 为 cell 类名, nil = 不限
@property (nonatomic, readonly) IRUIElementQuery *(^visibleCells)(NSString *_Nullable className);

/// 当前结果的全部子孙
@property (nonatomic, readonly) IRUIElementQuery *(^descendants)(void);

/// 当前结果的直接子节点
@property (nonatomic, readonly) IRUIElementQuery *(^children)(void);

/// 当前结果的父节点 (不超出起点节点)
@property (nonatomic, readonly) IRUIElementQuery *(^parent)(void);

/// 当前结果往上第一个类名匹配的祖先 (含子类; 最多到起点节点)
@property (nonatomic, readonly) IRUIElementQuery *(^ancestor)(NSString *className);

/// 当前结果的兄弟节点 (同一个父节点, 不含自己)
@property (nonatomic, readonly) IRUIElementQuery *(^siblings)(void);

#pragma mark 切片 (缩小结果; first / last / at 用来从多个里显式挑一个)

/// 只留第一个
@property (nonatomic, readonly) IRUIElementQuery *(^first)(void);

/// 只留最后一个
@property (nonatomic, readonly) IRUIElementQuery *(^last)(void);

/// 只留第 index 个 (从 0 开始); 越界为空
@property (nonatomic, readonly) IRUIElementQuery *(^at)(NSUInteger index);

/// 前 count 个
@property (nonatomic, readonly) IRUIElementQuery *(^prefix)(NSUInteger count);

/// 后 count 个
@property (nonatomic, readonly) IRUIElementQuery *(^suffix)(NSUInteger count);

/// 从 start 开始取 length 个 (自动裁剪到合法范围)
@property (nonatomic, readonly) IRUIElementQuery *(^range)(NSUInteger start, NSUInteger length);

#pragma mark 收尾

/// 这条查询的元素 (不执行): 使用时才执行 (操作 / 取值要求正好一个); 可以存下来反复使用, 每次都按那时的模式和刷新点重新执行
@property (nonatomic, readonly) IRUIElement *element;

/// 立即执行成元素 (0..n 个, 同一份快照; 同 CHR 的 allElements): 每一项持有单个节点, 之后在它们上面取信息不再重新查
@property (nonatomic, readonly) NSArray<IRUIElement *> *(^elements)(IRDiagnostics *_Nullable diag);

/// 等到正好一个: 每轮在链头刷新 (不管模式, 其余刷新点照常) 再查, 正好一个即返回持有该节点的元素;
/// 不返回 nil: 超时返回空元素 (exists 为 NO, 操作失败), diag 写明最后一轮的数量
@property (nonatomic, readonly) IRUIElement *(^wait)(NSTimeInterval timeout, IRDiagnostics *_Nullable diag);

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
