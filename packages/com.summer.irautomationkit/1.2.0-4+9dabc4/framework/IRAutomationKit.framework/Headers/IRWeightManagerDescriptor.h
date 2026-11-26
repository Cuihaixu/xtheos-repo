//
//  IRWeightManagerDescriptor.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/27.
//

#import "IRWeightDescriptor.h"
#import "IRIdentityType.h"

NS_ASSUME_NONNULL_BEGIN

@class IRWeightSpaceDescriptor;
@protocol IRWeightSpaceDescriptorBuilder;

@interface IRWeightManagerDescriptor : IRWeightDescriptor

/// 当前 descriptor 的可调结构版本，等价于 `ir_weightDataVersion`。导出 raw config 顶层 `data_version`。
@property (nonatomic, copy, readonly) NSString *dataVersion;

@property (nonatomic, strong, readonly) NSDictionary<NSString *, IRWeightSpaceDescriptor *> *spaceDescriptors;

- (void)registerSpace:(id<IRIdentityType>)spaceID builder:(void(^)(id<IRWeightSpaceDescriptorBuilder> space))block;

@end

NS_ASSUME_NONNULL_END
