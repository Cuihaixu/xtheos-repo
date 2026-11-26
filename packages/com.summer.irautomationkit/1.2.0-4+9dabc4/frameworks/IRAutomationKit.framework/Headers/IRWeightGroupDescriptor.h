//
//  IRWeightGroupDescriptor.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/27.
//

#import <Foundation/Foundation.h>
#import "IRWeightDescriptor.h"
#import "IRWeightItemDescriptor.h"
#import "IRIdentityType.h"

NS_ASSUME_NONNULL_BEGIN

@protocol IRWeightGroupDescriptor <NSObject>

@property (nonatomic, copy, readonly) NSString *spaceID;
@property (nonatomic, copy, readonly) NSString *groupID;
@property (nonatomic, strong, readonly) NSDictionary<NSString *, IRWeightItemDescriptor *> *itemDescriptors;

@end

@protocol IRWeightGroupDescriptorBuilder <IRWeightGroupDescriptor, IRWeightDescriptorBuilder>

@property (nonatomic, readonly) id<IRWeightItemDescriptorBuilder>(^registerItem)(id<IRIdentityType>itemID, NSInteger weightValue);

@end

@interface IRWeightGroupDescriptor : IRWeightDescriptor <IRWeightGroupDescriptor>

@end

NS_ASSUME_NONNULL_END
