//
//  IRWeightSpaceDescriptor.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/27.
//

#import <Foundation/Foundation.h>
#import "IRWeightDescriptor.h"
#import "IRWeightGroupDescriptor.h"
#import "IRWeightProfileDescriptor.h"
#import "IRIdentityType.h"
#import "IRWeightChain.h"

NS_ASSUME_NONNULL_BEGIN

@protocol IRWeightSpaceDescriptor <NSObject>

@property (nonatomic, copy, readonly) NSString *spaceID;
@property (nonatomic, strong, readonly) NSArray<NSString *> *profileIDs;
@property (nonatomic, strong, readonly) NSDictionary<NSString *, IRWeightGroupDescriptor *> *groupDescriptors;

@end

@protocol IRWeightSpaceDescriptorBuilder <IRWeightSpaceDescriptor, IRWeightDescriptorBuilder>

- (void)registerProfile:(id<IRIdentityType>)profileID;
- (void)registerGroup:(id<IRIdentityType>)groupID builder:(void(^)(id<IRWeightGroupDescriptorBuilder> group))block;

/// 语法糖: 链式注册一个组 (= registerGroup:builder:), 见 IRWeightChain.h
@property (nonatomic, readonly) id<IRWeightGroupChain> (^group)(NSString *groupID, NSString *_Nullable comment);

@end

@interface IRWeightSpaceDescriptor : IRWeightDescriptor <IRWeightSpaceDescriptor>

@end

NS_ASSUME_NONNULL_END
