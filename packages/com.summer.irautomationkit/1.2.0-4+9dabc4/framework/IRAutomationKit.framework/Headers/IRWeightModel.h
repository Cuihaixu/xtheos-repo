//
//  IRWeightModel.h
//  IRAutomationKit
//
//  权重模块的 JSON 编解码 (WEIGHT.md "依赖怎么处理"): 只实现 CHR 权重模块用到的那几条 CHRModel 规则, 保证导出 / 导入的
//  JSON 和 CHR 逐字段一致 (CHR 服务端的配置直接能用)。不是通用模型库。
//  - 参与编解码的属性: 本类和父类 (到 NSObject 为止) 里同时有 getter 和 setter 的属性; 只读的计算属性不参与
//  - modelCustomPropertyMapper: 属性名 → JSON 键 (可以多个属性映射到同一个键)
//  - modelPropertyBlacklist: 不参与的属性
//  - modelContainerPropertyGenericClass: 数组 / 集合 / 字典里元素的模型类 (解码用)
//  - 编码: nil 跳过; block 跳过; 数值 → NSNumber; 模型对象递归; 数组 / 集合 → 数组; 字典递归
//  - 解码: 先 [cls new] 再按属性赋值; 属性类型是模型类时递归; 容器按元素类转换; JSON 里没有的键不动
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// 参与 JSON 编解码的类 (方法都是可选的, 同 CHRModel)
@protocol IRModel <NSObject>
@optional
+ (nullable NSDictionary<NSString *, id> *)modelCustomPropertyMapper;
+ (nullable NSDictionary<NSString *, id> *)modelContainerPropertyGenericClass;
+ (nullable NSArray<NSString *> *)modelPropertyBlacklist;
@end

@interface NSObject (IRModel)

/// JSON (字典) → 模型; 不是字典返回 nil
+ (nullable instancetype)ir_modelWithJSON:(id)json;

/// 模型 → JSON 对象 (字典); 可直接给 NSJSONSerialization
- (nullable id)ir_modelToJSONObject;

@end

NS_ASSUME_NONNULL_END
