//
//  IRScreenState.h
//  IRAutomationKit
//
//  感知结果: 页面感知每轮产出一份, 不可变。只回答两个问题:
//  1. 当前显示的是哪个页面 → pageAgent
//  2. 链上 (叶子 → 根) 第一个需要自动处理的是哪个 Agent → interruptAgent
//  页面有没有变化由使用方拿两次结果自己比。
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRResponderChain.h>
#import <IRAutomationKit/IRViewControllerHierarchy.h>
#import <IRAutomationKit/IRInterruptHandling.h>

NS_ASSUME_NONNULL_BEGIN

@class IRAgent;
@class IRPageAgent;

@interface IRScreenState : NSObject

/// 生效序号 (每轮 +1)
@property (nonatomic, readonly) uint64_t sequence;

/// 本轮覆盖到的请求票号: 开始扫描前已发出的请求 (票号 <= ticket) 都由本轮满足
@property (nonatomic, readonly) uint64_t ticket;

/// 本轮的链 (正式接口: 可直接取遮罩等节点); 扫描失败为 nil
@property (nonatomic, readonly, nullable) IRResponderChain *chain;

/// 本轮的 VC 层级树: 键盘打开时代替 hitTest (hitTest 会让部分页面收起键盘), 这时 chain 为 nil; hitTest 模式为 nil
@property (nonatomic, readonly, nullable) IRViewControllerHierarchy *pvc;

/// 问题 1: 当前显示的页面; 链上没有 VC / 扫描失败为 nil
@property (nonatomic, readonly, nullable) IRPageAgent *pageAgent;

/// 正在切换页面: 这一轮认不出页面, pageAgent 沿用上一轮的。系统转场 (push / pop / present / dismiss) 期间一直沿用;
/// 其它情况 (自定义容器左右滑动等) 连续 2 轮认不出才报告未知页面
@property (nonatomic, readonly, getter=isTransitioning) BOOL transitioning;

/// 当前显示的是已知页面 (pageAgent 是注册过页面标识的页面级控制器)
@property (nonatomic, readonly, getter=isKnownPage) BOOL knownPage;

/// 问题 2: 链上第一个需要自动处理的 Agent; 没有 / 未询问 (自动化关闭) 为 nil
@property (nonatomic, readonly, nullable) IRAgent<IRInterruptHandling> *interruptAgent;

/// 产出时间 (CFAbsoluteTime)
@property (nonatomic, readonly) CFAbsoluteTime timestamp;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
