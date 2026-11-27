//
//  IRDebugOptions.h — 调试 dump(IRObject(Debug))的可扩展配置容器(L1)
//
//  对标 IRCallOptions:per-call 配置都塞这里,加字段不破坏接口 → "无限扩展"。
//  当前只有 `source`(数据来源:静态自解析 / 远程 runtime)。将来 format/includeInherited/
//  maxDepth/filter 等都往这里加,IRObject(Debug) 的入口不变。
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

/// dump 的数据来源。
typedef NS_ENUM(NSInteger, IRDebugSource) {
    /// 自解析目标 `class_ro`(编译期元数据)——**免 worker、默认**;看不到运行时动态加的 category/swizzle。
    IRDebugSourceStatic = 0,
    /// 远程调 ObjC runtime(`class_copyMethodList` 等)——需 worker 作用域;反映realized 类(含 category/swizzle)。
    IRDebugSourceRuntime,
};

/// `IRObject(Debug)` 的 dump 配置。字段按需扩展,接口不膨胀。
@interface IRDebugOptions : NSObject

/// 数据来源,默认 `IRDebugSourceStatic`。
@property (nonatomic) IRDebugSource source;

/// 是否沿继承链列出父类成员(`pmethods`)。默认 `NO`——只列本类(对齐 Facebook Chisel)。
/// 置 `YES` 则一路到根(含 `NSObject`)。注:`pivars` 恒含继承(对象物理布局本就含祖先 ivar)。
@property (nonatomic) BOOL inherited;

/// `pivars` 是否对对象型 ivar **远程取值兜底**(默认 `NO`)。
/// 关:只做**纯内存**增强(字符串内容、不可变集合条目数——免 worker、无副作用),其余显示 `<类名 @地址>`。
/// 开:对**可变集合**远程调 `count`、其它对象远程调 `description`——**需 worker/目标前台**,受调用超时保护。
@property (nonatomic) BOOL resolveValues;

/// `classdump` 是否按名称字母序排列 ivar / property / method(默认 `NO`)。
@property (nonatomic) BOOL sort;

/// `classdump` 是否输出 `{ ... }` 成员变量块(默认 `YES`)。
@property (nonatomic) BOOL showIvars;

/// `classdump` 是否输出 `@property` 声明(默认 `YES`)。
@property (nonatomic) BOOL showProperties;

/// `classdump` 是否输出类方法与实例方法声明(默认 `YES`)。
@property (nonatomic) BOOL showMethods;

/// `classdump` 是否在右侧输出逆向增强注释(IMP / IDA 镜像偏移 / ivar 偏移,默认 `YES`)。
@property (nonatomic) BOOL showAddresses;

/// `classdump` 是否启用 ANSI 终端语法高亮着色(默认 `NO`;写文件时恒关)。
@property (nonatomic) BOOL colorize;

/// `classdump` 递归展开属性/ivar 所引用的非系统关联类的深度(默认 `0` 不展开)。
@property (nonatomic) NSInteger depsDepth;

/// 全默认配置(static,只本类)。
+ (instancetype)defaultOptions;

@end

NS_ASSUME_NONNULL_END
