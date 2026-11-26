//
//  IRIdentity.h
//  IRAutomationKit
//
//  标识 = 名字 + 说明。用于注册动作: [self registerAction:IRIdentityMake(@"watch_comment", @"打开评论") block:...]
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRIdentity : NSObject <NSCopying>

/// 名字 (不能为空)
@property (nonatomic, copy, readonly) NSString *identifier;

/// 说明 (可选), 日志里显示
@property (nonatomic, copy, readonly, nullable) NSString *comment;

+ (instancetype)identityWithIdentifier:(NSString *)identifier comment:(nullable NSString *)comment;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

/// 简写: IRIdentityMake(@"watch_comment", @"打开评论")
NS_INLINE IRIdentity *IRIdentityMake(NSString *identifier, NSString *_Nullable comment) {
    return [IRIdentity identityWithIdentifier:identifier comment:comment];
}

NS_ASSUME_NONNULL_END
