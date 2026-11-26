//
//  IRTimeRangeValue.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/30.
//

#import "IRWeightValue.h"

NS_ASSUME_NONNULL_BEGIN

@interface IRTimeRange : IRWeightValue

@property (nonatomic, assign) NSUInteger min;
@property (nonatomic, assign) NSUInteger max;

- (NSUInteger)randomValue;

@end

NS_INLINE IRTimeRange *IRTimeRangeMake(NSUInteger min, NSUInteger max) {
    IRTimeRange *value = [IRTimeRange new];
    value.min = min;
    value.max = max;
    return value;
}

NS_ASSUME_NONNULL_END
