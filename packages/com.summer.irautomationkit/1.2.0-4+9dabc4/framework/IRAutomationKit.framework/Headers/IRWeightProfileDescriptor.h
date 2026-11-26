//
//  IRWeightProfileDescriptor.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/29.
//

#import "IRWeightDescriptor.h"

NS_ASSUME_NONNULL_BEGIN

@class IRWeightGroupDescriptor;
@interface IRWeightProfileDescriptor : IRWeightDescriptor

@property (nonatomic, strong, readonly) NSDictionary<NSString *, IRWeightGroupDescriptor *> *groupDescriptors;

@end

NS_ASSUME_NONNULL_END
