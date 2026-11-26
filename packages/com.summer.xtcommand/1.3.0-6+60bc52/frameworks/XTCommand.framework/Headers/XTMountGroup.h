//
//  XTMountGroup.h
//  XTCommand —— 挂载组:把命令按组挂到 iagent,带组说明和标签
//
//  组可多层(从父组派生子组)。iagent 帮助里按组展示:组说明 + 代表性标签 + 命令数。
//  预设公共组(apps/screen/log…)的说明由 iagent 内置,这里设的 summary 对预设组忽略;
//  自定义组(插件自己起的名)用这里的 summary。
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface XTMountGroup : NSObject

/// 这个组的代表性标签(总体介绍,如 @[@"安装", @"卸载"]);iagent 汇总各插件标签展示。
@property (nonatomic, copy, nullable) NSArray<NSString *> *tags;

/// 派生子组:iagent 路径 = 父组路径 + name。summary 仅自定义组生效。
- (XTMountGroup *)groupNamed:(NSString *)name summary:(nullable NSString *)summary;

/// 把 source 子树挂进这个组。source 相对开组的那个命令(可多段);iagent 路径 = 组路径 + source。
- (void)mount:(NSString *)source;
/// 挂并改名 / 改层级:iagent 路径 = 组路径 + name(name 可多段,如 @"E F"),执行时还原成 source。
- (void)mount:(NSString *)source as:(NSString *)name;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
