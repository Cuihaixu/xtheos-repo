//
//  IRWeightSelection+IRWeightValue.h
//  IRAutomationKit
//
//  语法糖 (WEIGHT.md "语法糖"): 从选择结果上取名字和值。
//  - name: 选中的选项名 (itemID)
//  - xxxValue: 读选中选项上的值 (注册时 .value(…) 设的, 存在选项参数 "value" 里), 按需要的类型隐式转换;
//    没设值时退回读选项名 (.item(@"3", 40) 的 intValue 就是 3)
//  - 时长区间 (IRTimeRange) 的值: doubleValue 在区间里取一个带小数的数, intValue / integerValue 取一个整数
//  - 没选中 (组不存在 / 全部禁用 / 页面不对后跳过): name 为 nil, 数值为 0, boolValue 为 NO, is(…) 为 NO;
//    用 weightOrValue(…, 默认值) 取的选择结果没选中时, value / xxxValue 读默认值 (name 仍为 nil, is(…) 仍为 NO)
//
//      feed.weight(@"watch_or_skip").is(@"watch")
//      feed.weight(@"dwell").doubleValue            // 时长区间里取一个秒数
//      page.weight(@"result_swipes").intValue
//

#import <Foundation/Foundation.h>
#import "IRWeightSelection.h"

NS_ASSUME_NONNULL_BEGIN

@interface IRWeightSelection (IRWeightValue)

/// 选中的选项名 (没选中为 nil)
@property (nonatomic, readonly, nullable) NSString *name;

/// 选中选项上的原始值 (没设值时是选项名; 没选中为默认值, 没给默认值为 nil)
@property (nonatomic, readonly, nullable) id value;

@property (nonatomic, readonly, nullable) NSString *stringValue;
@property (nonatomic, readonly) int intValue;
@property (nonatomic, readonly) NSInteger integerValue;
@property (nonatomic, readonly) double doubleValue;
@property (nonatomic, readonly) BOOL boolValue;

/// 选中的是不是这个选项 (按名字)
@property (nonatomic, readonly) BOOL (^is)(NSString *name);

@end

NS_ASSUME_NONNULL_END
