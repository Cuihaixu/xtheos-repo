//
//  IRWeightFactor.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/21.
//

#import <Foundation/Foundation.h>
#import "IRWeightModel.h"
#import "IRDiagnostics.h"

NS_ASSUME_NONNULL_BEGIN

@interface IRWeightFactor : NSObject <IRModel>

/// 因素注释
@property (nonatomic, copy) NSString *comment;

/// 最大调整幅度（相对比例，-1.0 ~ 1.0）
/// > 0 只加权：delta ∈ [0,  maxDelta]
/// < 0 只减权：delta ∈ [minDelta, 0]，其中 minDelta = maxDelta
@property (nonatomic, readonly) float maxDelta;

/// 是否为正向因素
@property (nonatomic, readonly) BOOL positive;

/// value 为正 → 正向因素（加权）；value 为负 → 负向因素（减权）
+ (instancetype)factorWithMaxDelta:(float)value;

/// 使用力度获取因素调整权重值
/// - Parameter strength: 因素力度 0 ~ 100
///   maxDelta > 0: 0 → 0，100 → +maxDelta
///   maxDelta < 0: 0 → 0，100 → maxDelta(负)
- (float)deltaForStrength:(int)strength;

@end

/// 历史回调因素常量名称
FOUNDATION_EXTERN NSString *IRWeightFactorHistoricalRebalanceName;

@protocol IRWeightFactorChain <NSObject>

/// 父级提供者
@property (nonatomic, weak, readonly, nullable) id<IRWeightFactorChain> parent;

/// 当前提供者因素列表
@property (nonatomic, readonly) NSDictionary<NSString *, IRWeightFactor *> *currentFactors;

/// 查询因素对象
/// - Parameter name: 因素名称
- (IRWeightFactor *)factorForName:(NSString *)name;

@end

NS_ASSUME_NONNULL_END
