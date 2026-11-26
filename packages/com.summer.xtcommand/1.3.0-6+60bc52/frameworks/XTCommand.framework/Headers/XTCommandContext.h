//
//  XTCommandContext.h
//  XTCommand —— handler 里读参数、输出结果
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class XTCommand;

@interface XTCommandContext : NSObject

/// 实际执行的命令
@property (nonatomic, readonly) XTCommand *command;
/// 命令路径,如 @[@"awetool", @"video", @"like"]
@property (nonatomic, readonly) NSArray<NSString *> *commandPath;
/// 没被解析的参数(需 allowExtras)
@property (nonatomic, readonly) NSArray<NSString *> *remainingArguments;
/// 是否 JSON 输出(--json,或 --ai / iagent / MCP 调用隐含)
@property (nonatomic, readonly, getter=isJSONMode) BOOL JSONMode;
/// 是否 AI 模式(--ai):面向机器,含 JSON + 无色。命令可据此裁剪输出。isAIMode 为真时 isJSONMode 必为真。
@property (nonatomic, readonly, getter=isAIMode) BOOL AIMode;

// 读取:当前命令里找不到时往父命令找;key 见 -[XTOption key]
- (nullable NSString *)stringForOption:(NSString *)key;
- (NSInteger)integerForOption:(NSString *)key;
- (double)doubleForOption:(NSString *)key;
- (BOOL)boolForOption:(NSString *)key;
/// 出现次数(-vvv → 3)
- (NSInteger)countForOption:(NSString *)key;
- (NSArray *)arrayForOption:(NSString *)key;
/// 不确定类型时用:NSString / NSNumber / NSArray,没有值返回 nil
- (nullable id)valueForOption:(NSString *)key;
/// 用户是否显式给了(区分"用的是默认值")
- (BOOL)isProvided:(NSString *)key;

// 输出
/// 普通输出(stdout);JSON 模式下不输出
- (void)print:(NSString *)format, ... NS_FORMAT_FUNCTION(1, 2);
/// 错误 / 警告(stderr)
- (void)printError:(NSString *)format, ... NS_FORMAT_FUNCTION(1, 2);
/// 结构化结果:JSON 模式输出 JSON,否则输出给人看的格式
- (void)output:(id)result;
/// 输出错误并返回退出码(handler 里 return [ctx failWithMessage:… code:…])
- (int)failWithMessage:(NSString *)message code:(int)code;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
