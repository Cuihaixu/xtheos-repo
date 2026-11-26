//
//  IRWeightDescriptor.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/27.
//

#import <Foundation/Foundation.h>
#import "IRWeightModel.h"
#import "IRDiagnostics.h"
#import "IRWeightFactor.h"
#import "IRWeightDefines.h"
#import "IRIdentityType.h"

NS_ASSUME_NONNULL_BEGIN

@protocol IRWeightDescriptorBuilder <NSObject>

@property (nonatomic, copy) NSString *comment;

- (void)registerFactor:(IRWeightFactor *)factor forName:(id<IRIdentityType>)name;

@end

@interface IRWeightDescriptor : NSObject<IRModel>

@property (nonatomic, copy, readonly) IRWeightRegion region;
@property (nonatomic, copy, readonly) NSString *comment;
@property (nonatomic, strong, readonly) NSDictionary<NSString *, IRWeightFactor *> *environmentFactors;

@end

NS_ASSUME_NONNULL_END
