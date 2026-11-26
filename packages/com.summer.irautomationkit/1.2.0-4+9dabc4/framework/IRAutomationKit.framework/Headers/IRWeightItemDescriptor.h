//
//  IRWeightItemDescriptor.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/27.
//

#import <Foundation/Foundation.h>
#import "IRWeightFactor.h"
#import "IRWeightDescriptor.h"
#import "IRValueSchema.h"

NS_ASSUME_NONNULL_BEGIN

@class IRWeightParams;
@protocol IRWeightItemDescriptor <NSObject>

/// 所属权重组
@property (nonatomic, copy, readonly) NSString *groupID;
/// 权重项标识
@property (nonatomic, copy, readonly) NSString *itemID;
/// 引入因素
@property (nonatomic, strong, readonly) NSArray<NSString *> *includeFactors;
/// 权重值
@property (nonatomic, assign, readonly) NSInteger value;
/// 因素影响最大调整百分比
@property (nonatomic, assign, readonly) double maxAdjustmentPercent;
/// 动作参数, 当在权重选中后传递给选中的动作参数
@property (nonatomic, strong, readonly) NSArray<IRValueSchema *> *actionParams;

@end

@protocol IRActionParamsBuilder <NSObject>

@property (nonatomic, copy, readonly) IRValueSchema *(^add)(NSString *name, IRSchemaType type);
@property (nonatomic, copy, readonly) IRValueSchema *(^addWithValue)(NSString *name, id defaultValue);

@end

@protocol IRWeightItemDescriptorBuilder <IRWeightItemDescriptor, IRWeightDescriptorBuilder>

@property (nonatomic, readonly) id<IRWeightItemDescriptorBuilder> (^setComment)(NSString *comment);
@property (nonatomic, readonly) id<IRWeightItemDescriptorBuilder> (^setIncludeFactors)(NSArray<NSString *> *factorNames);
@property (nonatomic, readonly) id<IRWeightItemDescriptorBuilder> (^setMaxAdjustmentPercent)(double percent);
@property (nonatomic, readonly) id<IRWeightItemDescriptorBuilder> (^setActionParams)(void(^)(id<IRActionParamsBuilder> params));
@property (nonatomic, readonly) id<IRWeightItemDescriptorBuilder> (^registerFactor)(NSString *name, IRWeightFactor *factor);

@end

@interface IRWeightItemDescriptor : IRWeightDescriptor <IRWeightItemDescriptor>

@end

NS_ASSUME_NONNULL_END
