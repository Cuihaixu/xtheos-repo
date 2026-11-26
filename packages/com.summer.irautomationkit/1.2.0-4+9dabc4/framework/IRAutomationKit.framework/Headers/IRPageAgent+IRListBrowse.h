//
//  IRPageAgent+IRListBrowse.h
//  IRAutomationKit
//
//  列表浏览 (BROWSE.md): 在一个列表上跑 "询问 → 操作" 的循环。一个场景 = 一个 flow 事件, 底层就是 registerAction:builder:
//  (标签 browse.flow, 签名 = flow.arg 声明的参数), 调用和别的事件一样: IRPageNode(pageID, diag).invokeWith(@"browse_ad", 参数)。
//  - 三个阶段: 查看 (browse.view) → 滑动 (browse.slide) → 退出 (browse.exit); 每一步只能从带这个阶段标签、当前可用的事件里选
//  - 步骤 block 拿两个参数: 这一步 (IRListBrowseStep: 阶段 / 可用事件 / 当前条, 在它上面做决定) + 上下文 (IRListBrowseContext: 这个列表, 整个浏览一份)
//  - 每一步的决定只有两种: 做 (step.invoke / invokeWith) / 跳过 (step.skip); 先问服务器 (IRListBrowseRemote), 服务器不管再调 onStep
//  - 阶段推进 (框架管): 查看做了 → 再问查看; 查看跳过 → 滑动; 滑动做了 → 查看; 滑动跳过 → 退出; 退出做了 / 跳过 → 结束
//  - 失败 (事件失败 / 查看、滑动阶段执行后不在列表页 / 这一步没给决定 / 服务器选的事件不可用 / 等列表停稳超时) 交给异常 block,
//    底层是 IRFlowSession (重试 / 跳过 / 终止)。退出阶段的事件会离开列表页, 不检查: 执行成功就结束, 停在哪由调用方处理
//  - 每一步开始前: info.list_state.scrolling 为真就等列表停稳 (页面没提供不等); 会滚动列表的事件也要停稳再返回 (约定)
//  - 统计 (见 IRListBrowseContext.h): 通用 list.general / 业务 list.visibleItems 等 / 计算 list.computed; 跨范围统计用代理 flow.delegate
//  - 结果 (flow 事件的 context.result): @{ exit: {action, params} 或 null, context: {general, computed, … 含全部 history} }
//
//      [self registerListBrowse:IRIdentityMake(@"browse_ad", @"广告场景浏览")
//                  contextClass:AWBSearchListBrowseContext.class
//                       builder:^(IRListBrowseBuilder *flow) {
//          flow.arg(@"max_items", IRSchemaTypeInteger).setDefaultValue(@15).setComment(@"最多看几条");
//          flow.onStep(^(IRListBrowseStep *step, AWBSearchListBrowseContext *list) {
//              BOOL enough = list.computed.exposed.integerValue >= list.intParam(@"max_items");
//              if (enough && !step.at(IRListBrowseStageExit)) { step.skip(); return; }   // 查看 / 滑动都跳过 → 走到退出
//              if (step.at(IRListBrowseStageView)) {
//                  if (step.item.watched == 0) step.invokeWith(@"watch_item", @{ @"seconds": @3 });
//                  else step.skip();                                                         // 这条看完了
//              }
//              if (step.at(IRListBrowseStageSlide)) {
//                  list.listState.reachedEnd ? step.skip() : step.invoke(@"next_item");
//              }
//              if (step.at(IRListBrowseStageExit)) {
//                  step.invoke(@"back");
//              }
//          }, nil);                                                                           // 异常 block 传 nil = 默认处理
//      }];
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRPageAgent.h>
#import <IRAutomationKit/IRIdentity.h>
#import <IRAutomationKit/IRValueSchema.h>
#import <IRAutomationKit/IRFlowException.h>
#import <IRAutomationKit/IRListBrowseRemote.h>
#import <IRAutomationKit/IRListBrowseDelegate.h>

NS_ASSUME_NONNULL_BEGIN

@class IRListBrowseContext, IRListBrowseStep;

/// 阶段 = 事件标签 (ACTION.md 第 39 项): 页面事件打上对应的标签, 才会在这个阶段出现
typedef NSString *IRListBrowseStage NS_TYPED_ENUM;
FOUNDATION_EXPORT IRListBrowseStage const IRListBrowseStageView;    ///< "browse.view": 查看 (看 N 秒 / 点开 / 评论 / 广告 …), 一次一个动作
FOUNDATION_EXPORT IRListBrowseStage const IRListBrowseStageSlide;   ///< "browse.slide": 滑动 (下一个 / 跳到第 n 条 …)
FOUNDATION_EXPORT IRListBrowseStage const IRListBrowseStageExit;    ///< "browse.exit": 退出 (back / 切单双列 / 过滤词 / 换词 …)

/// 其它标签
FOUNDATION_EXPORT NSString *const IRListBrowseTagInfo;   ///< "browse.info": 只读查询事件, 数据按事件名放进 context.info (如 current_item / visible_items / layout)
FOUNDATION_EXPORT NSString *const IRListBrowseTagFlow;   ///< "browse.flow": flow 事件自动带上; availableActions(@[它]) = 当前页面支持的场景 + 参数表

/// 统计标签 (BROWSE.md 统计三层): 查看类事件打上, 做成功一次算当前条一次
FOUNDATION_EXPORT NSString *const IRListBrowseTagClick;      ///< "browse.click": 点击 (如 open_item / open_ad); 当前条是广告记广告点击
FOUNDATION_EXPORT NSString *const IRListBrowseTagExposure;   ///< "browse.exposure": 曝光 = 停留观看 (如 watch_item); 同一条只算一次

/// 步骤 block: 每一步调一次; 必须在 step 上 做 (invoke / invokeWith) / 跳过 (skip) 里选一个, 且只选一次
typedef void (^IRListBrowseStepBlock)(IRListBrowseStep *step, __kindof IRListBrowseContext *list);
/// 异常 block: 同 IRFlowSession 的 onException, 多给出错的这一步 (步开始前就失败, 如页面不对, 为 nil) 和上下文; 不处理 = 终止
typedef void (^IRListBrowseExceptionBlock)(IRFlowException *exception, IRListBrowseStep *_Nullable step, __kindof IRListBrowseContext *list);

/// 构建器: B 定义参数 + C 写步骤
@interface IRListBrowseBuilder : NSObject

/// 参数 (= flow 事件的签名入参; 同 IRActionBuilder.arg): 默认必填, .setDefaultValue 后可选; 服务器按签名覆盖
@property (nonatomic, readonly) IRValueSchema *(^arg)(NSString *name, IRSchemaType type);

/// 步骤 + 异常 (必需, 只能调一次): exception 传 nil = 默认处理 —— 回到列表页 (list.backToList) 后重试这一步, 回不去终止
@property (nonatomic, readonly) void (^onStep)(IRListBrowseStepBlock step, IRListBrowseExceptionBlock _Nullable exception);

/// 这个场景的远程决策者 (单独指定, 如搜索场景交给搜索管理器); 不设 = 用全局的 [IRListBrowseRemote sharedRemote].decider
@property (nonatomic, strong, nullable) id<IRListBrowseRemoteDecider> remoteDecider;

/// 这个场景的代理 (统计通知 + 每步决策前给业务数据, 见 IRListBrowseDelegate.h); 不设就不通知
@property (nonatomic, strong, nullable) id<IRListBrowseDelegate> delegate;

/// 这个场景不问服务器 (即使设置了远程决策者; 默认 NO)
@property (nonatomic, readonly) void (^localOnly)(BOOL localOnly);

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

@interface IRPageAgent (IRListBrowse)

/// 注册一个场景的 flow 事件 (在 +load 里调用; 同 registerAction:builder: 的规则)
/// - Parameters:
///   - identity: A 事件名 + 说明, 如 IRIdentityMake(@"browse_ad", @"广告场景浏览")
///   - contextClass: 上下文类 (IRListBrowseContext 或子类; nil = 基类), 每次调用框架实例化一个
///   - builder: 参数 + 步骤
/// - 没给 onStep / onStep 调两次 / contextClass 不是 IRListBrowseContext 的子类: Debug 断言, Release 日志 + 拒绝注册
+ (void)registerListBrowse:(IRIdentity *)identity
              contextClass:(nullable Class)contextClass
                   builder:(NS_NOESCAPE void (^)(IRListBrowseBuilder *flow))builder;

@end

NS_ASSUME_NONNULL_END
