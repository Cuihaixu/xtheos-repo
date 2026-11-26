//
//  IRTimeRecorder.h
//  IRAutomationKit
//
//  分段计时 (拷贝自 CHRFoundation 的 CHRTimeRecorder, 前缀改为 IR, 实现未改)
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class IRTimingEntry, IRTimeRecorder;

@protocol IRTimeRecorderTimingsFormater <NSObject>

- (NSString *)timeRecorder:(IRTimeRecorder *)recorder formatTimings:(NSArray<IRTimingEntry *> *)timings;

@end

@interface IRTimingEntry : NSObject

@property (nonatomic, assign, readonly) uint64_t time;
@property (nonatomic, copy, readonly) NSString *message;
@property (nonatomic, assign, readonly) int depth;

@end

@interface IRTimeRecorder : NSObject

@property (nonatomic, readonly) id<IRTimeRecorderTimingsFormater> timingsFormater;

@property (nonatomic, copy, readonly) void(^pushTimedSection)(void);
@property (nonatomic, copy, readonly) void(^popTimedSection)(void);
@property (nonatomic, copy, readonly) void(^recordTime)(NSString *format, ...);
@property (nonatomic, copy, readonly) void(^clearTimings)(void);
@property (nonatomic, copy, readonly) NSString *(^formatTimings)(void);
@property (nonatomic, copy, readonly) NSString *(^formatAndClearTimings)(void);

+ (instancetype)timeRecorderWithFormater:(id<IRTimeRecorderTimingsFormater> __nullable)formater;

@end

NS_ASSUME_NONNULL_END
