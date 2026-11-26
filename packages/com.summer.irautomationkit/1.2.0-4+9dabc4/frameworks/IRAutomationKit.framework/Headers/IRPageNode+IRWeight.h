//
//  IRPageNode+IRWeight.h
//  IRAutomationKit
//
//  语法糖 (WEIGHT.md "语法糖"): 在页面节点上做权重选择 —— 和 invoke 同一套:
//  找页面 (节点的页面正在显示 → 用它; 不在显示 → 用最近一次识别到的这个页面 (app recentPageAgent:);
//  任意页面的节点只用当前显示的页面) → 用页面 Agent 当 Context 选择 (业务重写的 factorStrengthForName: / disabledItemIDsWithPath: 照样生效)
//  → 选择过程写进节点的 diag。失败 (页面不对 / 组不存在): 原始写法的节点返回空的选择结果, 原因在 diag;
//  会话建的节点先交给 onException (重试 / 跳过 / 终止), 跳过返回空的选择结果 (或 skipWith 给的选择结果)。
//  返回值永远不是 nil, 可以直接接 .name / .xxxValue / .is(…)
//
//      if (feed.weight(@"watch_or_skip").is(@"watch")) {
//          feed.wait(feed.weight(@"dwell").doubleValue);
//      }
//      NSInteger swipes = page.weight(@"result_swipes").integerValue;
//
//  weightOrValue(组, 默认值): 不算失败 —— 页面找不到 / 组不存在 / 选项全被禁用时直接用默认值, 只写一条 warning, 不交给 onException:
//
//      int swipes = feed.weightOrValue(@"swipes", @3).intValue;
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRPageNode.h>
#import "IRWeightSelection.h"
#import "IRWeightSelection+IRWeightValue.h"

NS_ASSUME_NONNULL_BEGIN

#pragma push_macro("IRPageNode")
#undef IRPageNode   // 函数式宏 IRPageNode(…) 会吃掉分类的 "("
@interface IRPageNode (IRWeight)

/// 在节点页面的权重空间里选择 (Context 是页面 Agent, 自动提交); 页面不要求正在显示, 见上
@property (nonatomic, readonly) IRWeightSelection *(^weight)(NSString *groupID);

/// 同 weight, 但不会失败: 没选中时选择结果的 value / xxxValue 读 defaultValue (任意对象, 如 @3、@"fast"、IRTimeRange)
@property (nonatomic, readonly) IRWeightSelection *(^weightOrValue)(NSString *groupID, id _Nullable defaultValue);

@end
#pragma pop_macro("IRPageNode")

NS_ASSUME_NONNULL_END
