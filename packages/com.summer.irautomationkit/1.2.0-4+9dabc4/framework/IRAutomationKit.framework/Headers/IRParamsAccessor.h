//
//  IRParamsAccessor.h
//  IRAutomationKit
//
//  Created by cu on 2026/2/3.
//

#import <Foundation/Foundation.h>

@class IRTimeRange;

NS_ASSUME_NONNULL_BEGIN

@interface IRParamsAccessor : NSObject

@property (nonatomic, readonly) NSDictionary *params;

+ (instancetype)accessorWithParams:(NSDictionary *)params;

- (id)findValue:(NSString *)name defaultValue:(id)value;
- (id)getValue:(NSString *)name;

/// 选项的时长参数 (键 "timeRange"; CHR 里是 CHRParamsAccessor (CHRWeight) 分类, 这里并进类本身)
@property (nonatomic, readonly, nullable) IRTimeRange *timeRange;

@end

NS_ASSUME_NONNULL_END
