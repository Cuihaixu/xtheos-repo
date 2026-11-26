//
//  IRWeightItem.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/20.
//

#import <Foundation/Foundation.h>
#import "IRWeightFactor.h"
#import "IRWeightParams.h"
//#import "IRWeightFactor.h"
//#import "IRWeightParams.h"

NS_ASSUME_NONNULL_BEGIN

@class IRWeightGroup;
@interface IRWeightItem : NSObject

/// 所在权重组
@property (nonatomic, weak, readonly) IRWeightGroup *group;

/// 权重标识
@property (nonatomic, copy, readonly) NSString *itemID;

/// 权重项说明
@property (nonatomic, copy, readonly) NSString *comment;

/// 引入因素
@property (nonatomic, strong, readonly) NSArray<NSString *> *includeFactors;

/// 环境因素
@property (nonatomic, strong, readonly) NSArray<IRWeightFactor *> *environmentFactors;

/// 权重值
@property (nonatomic, assign, readonly) NSInteger value;

/// 动态权重约束
@property (nonatomic, assign, readonly) double maxAdjustmentPercent;

/// 关闭权重项
@property (nonatomic, assign, readonly, getter=isDisable) BOOL disable;

/// 选中次数
@property (nonatomic, assign, readonly) NSUInteger selectedCount;

/// 目标概率（基于基础权重值，0-100）
@property (nonatomic, assign, readonly) double targetProbability;

/// 实际概率（基于历史统计，0-100)
@property (nonatomic, assign, readonly) double actualProbability;

/// 权重自定义参数
@property (nonatomic, strong, readonly) IRWeightParams *params;   // 修 (WEIGHT.md #7): CHR 是 assign 的对象指针

@end


NS_ASSUME_NONNULL_END
