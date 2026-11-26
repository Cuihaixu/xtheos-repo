//
//  IRWeightAdjust.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/27.
//

#import <Foundation/Foundation.h>
#import "IRWeightModel.h"
#import "IRDiagnostics.h"
#import "IRWeightManager.h"
#import "IRWeightDefines.h"

NS_ASSUME_NONNULL_BEGIN

@interface IRWeightAdjust : NSObject <IRModel>

@property (nonatomic, copy, readonly) IRWeightRegion region;
@property (nonatomic, copy) NSString *adjustID;
@property (nonatomic, strong) NSDictionary<NSString *, IRWeightFactor *> *environmentFactors;

/// 调用方附带的元数据（如 config_id / source / persona_id）
/// 框架不解释字段语义；apply 时会跟随 adjust 存到节点上；JSON 序列化时排除
/// apply 递归时会向下传播给未显式设置 metadata 的子 adjust
@property (nonatomic, copy, nullable) NSDictionary *metadata;

+ (nullable instancetype)adjustWithInfo:(NSDictionary *)info;

@end

@protocol IRWeightAdjust <NSObject>

- (BOOL)applyWeightAdjust:(__kindof IRWeightAdjust *)adjust diag:(IRDiagnostics *)diag;

@end

@interface IRWeightItem (WeightAdjust) <IRWeightAdjust>

@end

@interface IRWeightGroup (WeightAdjust) <IRWeightAdjust>

@end

@interface IRWeightSpace (WeightAdjust) <IRWeightAdjust>

@end

@interface IRWeightManager (WeightAdjust) <IRWeightAdjust>
/// 清掉所有已应用的调整, 回到注册时的默认值 (新增, CHR 没有; 连续应用多份配置前先调它, 避免旧配置残留)
- (void)resetWeightAdjust;
@end

#pragma mark - Applied Tracking

/// 查询节点上当前生效的 adjust 与 metadata。
/// 仅在 applyWeightAdjust: 成功后非 nil；apply 失败或目标节点不匹配时为 nil。
@interface IRWeightItem (AppliedTracking)
@property (nonatomic, strong, readonly, nullable) IRWeightAdjust *appliedAdjust;
@property (nonatomic, copy,   readonly, nullable) NSDictionary *appliedMetadata;
@end

@interface IRWeightGroup (AppliedTracking)
@property (nonatomic, strong, readonly, nullable) IRWeightAdjust *appliedAdjust;
@property (nonatomic, copy,   readonly, nullable) NSDictionary *appliedMetadata;
@end

@interface IRWeightSpace (AppliedTracking)
@property (nonatomic, strong, readonly, nullable) IRWeightAdjust *appliedAdjust;
@property (nonatomic, copy,   readonly, nullable) NSDictionary *appliedMetadata;
@end

@interface IRWeightManager (AppliedTracking)
@property (nonatomic, strong, readonly, nullable) IRWeightAdjust *appliedAdjust;
@property (nonatomic, copy,   readonly, nullable) NSDictionary *appliedMetadata;
@end

NS_ASSUME_NONNULL_END
