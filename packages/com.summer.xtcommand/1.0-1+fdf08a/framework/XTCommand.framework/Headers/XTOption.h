//
//  XTOption.h
//  XTCommand —— 选项 / 开关 / 位置参数,以及选项组
//

#import <Foundation/Foundation.h>
#import <XTCommand/XTCommandTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class XTValidator;

@interface XTOption : NSObject

/// 读取值时用的名字:有长名取长名(--count → count),否则短名,位置参数取它自己的名字
@property (nonatomic, copy, readonly) NSString *key;
@property (nonatomic, readonly) XTValueType type;
@property (nonatomic, readonly, getter=isFlag) BOOL flag;
@property (nonatomic, readonly, getter=isPositional) BOOL positional;

/// 必填
@property (nonatomic) BOOL required;
/// 默认值(NSString / NSNumber)
@property (nonatomic, strong, nullable) id defaultValue;
/// 命令行没给时读这个环境变量
@property (nonatomic, copy, nullable) NSString *envName;
/// 可给多次 / 多个值,读取时用 -[XTCommandContext arrayForOption:]
@property (nonatomic) BOOL multiple;
/// 一次需要几个值(如坐标 --point 0.5 0.5 → 2);>1 时按数组读取
@property (nonatomic) NSInteger expectedCount;
/// 只能取这几个值
@property (nonatomic, copy, nullable) NSArray<NSString *> *choices;
/// 出现多次时怎么取值;TakeAll 时按数组读取(同 multiple)
@property (nonatomic) XTMultiplePolicy multiplePolicy;
/// 选项名不区分大小写
@property (nonatomic) BOOL ignoreCase;
/// 帮助里放进哪个分组
@property (nonatomic, copy, nullable) NSString *helpGroup;
/// 帮助里的占位名,如 @"<COUNT>";不设按类型显示 <TEXT> / <INT> / <FLOAT> / <PATH> / <DIR>
@property (nonatomic, copy, nullable) NSString *metavar;
/// 子命令里也能用(如 awetool video like --device d1)
@property (nonatomic) BOOL inherited;

/// 给 AI 看的说明(不填用 summary)
@property (nonatomic, copy, nullable) NSString *agentSummary;
/// 文件参数建议放在哪(只写进给 AI 的说明)
@property (nonatomic) XTLocation preferredLocation;

- (void)addValidator:(XTValidator *)validator;
- (void)addTransform:(XTValidator *)transform;
/// 给了本选项就必须给 other
- (void)needs:(XTOption *)other;
/// 本选项与 other 不能同时给
- (void)excludes:(XTOption *)other;

- (instancetype)init NS_UNAVAILABLE;

@end

/// 选项组:帮助里归为一组,并可限制组内最少 / 最多给几个
@interface XTOptionGroup : NSObject

@property (nonatomic, copy, readonly) NSString *name;
/// 组内至少给几个(0 = 不限)
@property (nonatomic) NSInteger minSelected;
/// 组内最多给几个(0 = 不限;1 = 互斥)
@property (nonatomic) NSInteger maxSelected;

- (XTOption *)addOption:(NSString *)names summary:(NSString *)summary type:(XTValueType)type;
- (XTOption *)addFlag:(NSString *)names summary:(NSString *)summary;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
