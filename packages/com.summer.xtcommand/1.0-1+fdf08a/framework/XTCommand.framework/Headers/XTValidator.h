//
//  XTValidator.h
//  XTCommand —— 参数校验 / 转换
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// 校验通过返回 nil,不通过返回错误提示
typedef NSString *_Nullable (^XTValidatorBlock)(NSString *value);

@interface XTValidator : NSObject

/// 数值范围(含两端)
+ (instancetype)rangeFrom:(double)min to:(double)max;
/// 文件必须存在
+ (instancetype)existingFile;
/// 目录必须存在
+ (instancetype)existingDirectory;
/// 文件或目录必须存在
+ (instancetype)existingPath;
/// 路径必须不存在
+ (instancetype)nonexistentPath;
/// IPv4 地址
+ (instancetype)IPv4;

/// 自定义校验
+ (instancetype)validatorWithName:(NSString *)name block:(XTValidatorBlock)block;

/// 转换:把输入按表换成别的值(配合 -[XTOption addTransform:] 使用)
+ (instancetype)mapping:(NSDictionary<NSString *, NSString *> *)map ignoreCase:(BOOL)ignoreCase;

/// 两个都要通过
- (XTValidator *)and:(XTValidator *)other;
/// 通过任意一个即可
- (XTValidator *)or:(XTValidator *)other;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
