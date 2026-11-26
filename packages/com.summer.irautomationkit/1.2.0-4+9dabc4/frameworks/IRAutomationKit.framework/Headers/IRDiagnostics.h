//
//  IRDiagnostics.h
//  IRAutomationKit
//
//  诊断: 一次操作配一个, 记录失败原因、过程日志。拷贝自 CHRFoundation 的 CHRDiagnostics (Cuihaixu/CHRFoundation b7b8dd0),
//  前缀改为 IR, 是 SDK 里的独立模块 (含分段计时 IRTimeRecorder)。与原版的差别 (ACTION.md 第 22 项):
//  - 不复制 activityEnter / activityLeave / activityScope: 原版只有声明没有实现, 调用即崩
//  - 不复制 ref / IRDiagnosticsOutRef / IRDiagnosticsMake: 原版 ref 声明与实现名 def 不一致; 诊断一律显式传对象, 以后有需求再加
//  - 控制台输出改走 os_log (subsystem com.summer.irautomation.diag, category = domain), 不依赖 ilog;
//    环境变量 IR_DIAG_STDERR=1 时同时打到标准错误
//  ⚠️ 待定 (ACTION.md 第 20 项): 一个诊断只能记一个错误, 第二次 error 会断言 (与原版一致)
//
//  用法: 由调用方显式传入, 一个诊断贯穿 流程 → 动作 → app 操作:
//
//      IRDiagnostics *diag = [IRDiagnostics diagnosticsWithDomain:@"浏览流程"];
//      diag.verbose(@"开始");
//      if (![feed invokeAction:@"watch_comment" params:nil result:NULL diag:diag]) {
//          NSLog(@"%@", diag.errorDescription());
//      }
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class IRDiagnostics;
@protocol IRDiagnosticsLogger <NSObject>
@optional

- (void)diagnostics:(IRDiagnostics *)diag setErrorCode:(NSInteger)code message:(NSString *)message;
- (void)diagnostics:(IRDiagnostics *)diag appendWarning:(NSString *)message;
- (void)diagnostics:(IRDiagnostics *)diag appendVerbose:(NSString *)message;

- (void)diagnostics:(IRDiagnostics *)diag scopeEnterAtParentDiag:(IRDiagnostics *)parent description:(NSString *)desc;
- (void)scopeLeaveAtdiagnostics:(IRDiagnostics *)diag;

@end

@protocol IRDiagnosticsUniqueIDMaker <NSObject>

- (id)makerUniqueIDInDiagnostics:(IRDiagnostics *)diag;

@end

@interface IRDiagnosticsConsoleOutput : NSObject <IRDiagnosticsLogger>

@end

@interface IRDiagnostics : NSObject

@property (nonatomic, readonly) id uniqueID;
@property (nonatomic, strong) id<IRDiagnosticsUniqueIDMaker> uniqueIDMaker;

@property (nonatomic, readonly) NSString *domain;
@property (nonatomic, readonly) NSInteger errorCode;
@property (nonatomic, readonly) NSString *errorMessage;
@property (nonatomic, readonly) NSString *prefix;
@property (nonatomic, readonly) NSArray<id<IRDiagnosticsLogger>> *outputs;
@property (nonatomic, readonly) NSDictionary<NSString *, id> *userInfo;
@property (nonatomic, readonly) NSArray<NSString *> *warnings;


@property (nonatomic, readonly) BOOL hasError;
@property (nonatomic, readonly) BOOL noError;
@property (nonatomic, assign) BOOL isVerbose;



@property (nonatomic, copy, readonly) void(^error)(NSInteger code, NSString *format, ...);
@property (nonatomic, copy, readonly) NSString *(^errorDescription)(void);
@property (nonatomic, copy, readonly) void(^warning)(NSString *format, ...);
@property (nonatomic, copy, readonly) void(^verbose)(NSString *format, ...);
@property (nonatomic, copy, readonly) void(^clearError)(void);
@property (nonatomic, copy, readonly) void(^clearWarnings)(void);
@property (nonatomic, copy, readonly) void(^setPrefix)(NSString *format, ...);
@property (nonatomic, copy, readonly) void(^clearPrefix)(void);
@property (nonatomic, copy, readonly) NSError *(^toError)(void);


- (void)addDiagnosticsOutput:(id<IRDiagnosticsLogger>)output;
- (void)removeDiagnosticsOutput:(id<IRDiagnosticsLogger>)output;

- (void)setValue:(id)value forUserInfoKey:(NSString *)key;
- (id)valueForUserInfoKey:(NSString *)key;

+ (instancetype)diagnosticsWithDomain:(NSString *)domain;

@end



NS_INLINE IRDiagnostics *IRFunctionDiagnostics(void) {
    NSString *domain = [NSString stringWithFormat:@"%s", __FUNCTION__];
    return [IRDiagnostics diagnosticsWithDomain:domain];
}

NS_ASSUME_NONNULL_END
