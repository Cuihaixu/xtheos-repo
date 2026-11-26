//
//  IRWeightChain.h
//  IRAutomationKit
//
//  语法糖 (WEIGHT.md "语法糖"): 注册的链式写法, 底层还是 CHR 的 registerGroup / registerItem / setActionParams / registerFactor
//
//      [self registerWeight:^(id<IRWeightSpaceDescriptorBuilder> space) {
//          space.group(@"watch_or_skip", @"看还是划走").item(@"watch", 70).item(@"skip", 30).factor(@"boredom", 1.5);
//          space.group(@"dwell", @"看多久").item(@"glance", 45).value(IRTimeRangeMake(2, 5))
//                                         .item(@"long", 15).value(IRTimeRangeMake(12, 25)).comment(@"看进去了");
//          space.group(@"result_swipes", @"结果页滑几屏").item(@"one", 40).value(@1).item(@"two", 60).value(@2)
//              .groupFactor(IRWeightFactorHistoricalRebalanceName, 0);      // 组级因子 (如关掉历史回拨)
//      }];
//

#import <Foundation/Foundation.h>
#import "IRValueSchema.h"

NS_ASSUME_NONNULL_BEGIN

@protocol IRWeightItemChain;

/// 一个组: 往里加选项 / 组级因子
@protocol IRWeightGroupChain <NSObject>
/// 加一个选项 (同 registerItem): 名字 + 权重
@property (nonatomic, readonly) id<IRWeightItemChain> (^item)(NSString *name, NSInteger weight);
/// 组级因子 (同组的 registerFactor:forName:)
@property (nonatomic, readonly) id<IRWeightGroupChain> (^groupFactor)(NSString *name, float maxDelta);
@end

/// 刚加的选项: 设值 / 因子 / 说明, 也能接着 .item(…) 加下一个
@protocol IRWeightItemChain <IRWeightGroupChain>
/// 选项的值 (数字 / 字符串 / 时长区间), 存在选项参数 "value" 里; 选择后用 xxxValue 读
@property (nonatomic, readonly) id<IRWeightItemChain> (^value)(id<IRSchemaValueSerializable> value);
/// 选项挂一个因子并注册它 (= setIncludeFactors + registerFactor)
@property (nonatomic, readonly) id<IRWeightItemChain> (^factor)(NSString *name, float maxDelta);
/// 选项说明
@property (nonatomic, readonly) id<IRWeightItemChain> (^comment)(NSString *comment);
@end

NS_ASSUME_NONNULL_END
