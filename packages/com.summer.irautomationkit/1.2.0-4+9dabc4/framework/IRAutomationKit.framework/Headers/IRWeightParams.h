//
//  IRWeightParams.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/29.
//

#import <Foundation/Foundation.h>
#import "IRWeightModel.h"
#import "IRTimeRange.h"

NS_ASSUME_NONNULL_BEGIN

@interface IRWeightParams : NSObject <IRModel>

- (void)setValue:(__kindof IRWeightValue *)value forParamKey:(NSString *)key;
- (__kindof IRWeightValue *)valueForParamKey:(NSString *)key;

@property (nonatomic, readonly) IRTimeRange *timeRange;
- (void)setTimeRange:(IRTimeRange *)timeRange comment:(NSString *)comment;

@end

NS_ASSUME_NONNULL_END
