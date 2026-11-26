//
//  IRWeightMark.h
//  IRAutomationKit
//
//  Created by cu on 2026/5/9.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// 为 space/group/item.comment 拼接服务端可解析的 [[key: value]] 标记前缀。
///
/// 终止接口 `.comment(@"...")` 返回 NSString，可直接传给 IRIdentityMake。
///
/// 用法示例：
/// @code
///   IRWeightMark.group
///       .groupTuning()
///       .level(10)
///       .tags(@[@"feed", @"comment"])
///       .comment(@"评论打开页比率");
///   // -> @"[[group_tuning: true]][[level: 10]][[tags: feed,comment]]评论打开页比率"
/// @endcode
@interface IRWeightMark : NSObject

/// 为 space.comment 创建标记 builder。
+ (IRWeightMark *)space;

/// 为 group.comment 创建标记 builder。
+ (IRWeightMark *)group;

/// 为 item.comment 创建标记 builder。
+ (IRWeightMark *)item;

/// 底层 KeyValue：追加任意 `[[key: value]]` 标记。
@property (nonatomic, readonly) IRWeightMark *(^kv)(NSString *key, NSString *value);

/// 语法糖：标记当前 group 进入 AI 调参上下文，等价 `kv(@"group_tuning", @"true")`。
@property (nonatomic, readonly) IRWeightMark *(^groupTuning)(void);

/// 语法糖：重要程度，等价 `kv(@"level", @"10")`。
@property (nonatomic, readonly) IRWeightMark *(^level)(NSInteger level);

/// 语法糖：标签数组，等价 `kv(@"tags", @"a,b")`；空数组不发射 marker。
@property (nonatomic, readonly) IRWeightMark *(^tags)(NSArray<NSString *> *tags);

/// 终止接口：拼出最终 comment 文本。返回 NSString 而非 IRWeightMark，链式止于此。
@property (nonatomic, readonly) NSString *(^comment)(NSString *commentText);

@end

NS_ASSUME_NONNULL_END
