//
//  IRApplication+IRInput.h
//  IRAutomationKit
//
//  文字输入与键盘状态 (OPERATION.md 4.6), 全部走 CHRTouchKit。
//  - 输入 / 清空 / 回车是写操作: 只能在持有执行权的 UI 任务上调用, 应用必须在前台, 输入框必须已聚焦 (键盘已打开);
//    同步执行, 做完才返回; 失败原因写进 diag
//  - 输入可能持续好几秒: 中途来了中断 / 任务被取消 / 应用进后台, 会停在一个完整的字 / 词上, 返回 NO (diag 写明被打断),
//    不自动续打, 由业务按当时的页面决定重输还是继续
//  - keyboardVisible / keyboardInfo 是读操作, 任何线程都能调
//
//      app.inputText(@"足球", diag);
//      [app inputText:@"abc" mode:IRInputModeFast diag:diag];
//      NSString *text = app.keyboardInfo.text;     // 当前输入框的文字
//      app.keyboardReturn(diag);                   // 搜索 / 发送
//
//  输入监听 (输入中途看一眼页面, 比如候选词里已经有要的词就不用打完了):
//      __block BOOL hit = NO;
//      [app inputText:@"足球游戏" config:nil monitor:^BOOL(IRInputProgress *progress) {
//          hit = …;              // 只能读 (查询), 不能点: 在 CHRTouchKit 的回调线程上
//          return hit;           // YES = 停止后面的输入
//      } diag:diag];
//      if (hit) { … 点候选词 … }  // 回到 UI 任务里再点
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRApplication.h>
#import <IRAutomationKit/IRDiagnostics.h>
#import <IRAutomationKit/IRKeyboardInfo.h>

NS_ASSUME_NONNULL_BEGIN

/// 输入模式 (对应 CHRTouchKit 的三种预设)
typedef NS_ENUM(NSInteger, IRInputMode) {
    IRInputModeStandard = 0,    ///< 标准 (默认): 拟人自然, 偶有笔误、按词输入、带节奏停顿
    IRInputModeFast,            ///< 极速: 最快最稳, 无拟人 (零笔误、整词、无额外停顿)
    IRInputModeCareful,         ///< 谨慎: 慢而稳, 少笔误
};

/// 输入进度: 只在一个字 / 词上屏时有 (CHRTouchKit 的 subtokenCompleted / wordCommitted, 已上屏的字没变的不算)
@interface IRInputProgress : NSObject
/// 第几个上屏点, 从 1 开始
@property (nonatomic, readonly) NSUInteger step;
/// 已上屏的字
@property (nonatomic, copy, readonly) NSString *committedText;
/// 要输入的全文
@property (nonatomic, copy, readonly) NSString *text;
- (instancetype)init NS_UNAVAILABLE;
@end

/// 输入监听: 返回 YES = 停止后面的输入 (算成功, 输入返回 YES)。
/// 在 CHRTouchKit 的回调线程上调 (不是 UI 任务): 只能做读操作 (查询 / 读视图树), 不能点; 回调期间输入等着它。抛异常当作 NO
typedef BOOL (^IRInputMonitor)(IRInputProgress *progress);

@interface IRApplication (IRInput)

#pragma mark 写

/// 向当前焦点输入框输入 (标准模式)
/// - Returns: 是否输完 (被打断、校验不通过都返回 NO)
- (BOOL)inputText:(NSString *)text diag:(nullable IRDiagnostics *)diag;

/// 指定预设模式输入
- (BOOL)inputText:(NSString *)text mode:(IRInputMode)mode diag:(nullable IRDiagnostics *)diag;

/// 完整拟人配置输入: config 原样交给 CHRTouchInputTextWithConfig, 字段以 CHRTouchKit 为准 (CHRTouchManual.h:
/// preset / weights / durations / durationScale / emojiPool / submit …); nil = 标准模式整套配置
- (BOOL)inputText:(NSString *)text config:(nullable NSDictionary<NSString *, id> *)config diag:(nullable IRDiagnostics *)diag;

/// 带监听输入: 每个字 / 词上屏时调 monitor, 它返回 YES 就停在这里、返回 YES (被中断 / 取消停下的照旧返回 NO)。
/// 调用方要知道是不是被监听停下的, 看自己在回调里记的标记
- (BOOL)inputText:(NSString *)text config:(nullable NSDictionary<NSString *, id> *)config
          monitor:(nullable IRInputMonitor)monitor diag:(nullable IRDiagnostics *)diag;

/// 清空当前焦点输入框
- (BOOL)clearTextWithDiag:(nullable IRDiagnostics *)diag;

/// 回车: 触发当前焦点框的 return 动作 (搜索 / 发送 / 前往 / 换行, 由输入框决定)
- (BOOL)keyboardReturnWithDiag:(nullable IRDiagnostics *)diag;

#pragma mark 读

/// 键盘是否打开 (有焦点输入框)
@property (nonatomic, readonly, getter=isKeyboardVisible) BOOL keyboardVisible;

/// 键盘状态快照; 键盘没打开 / 服务不可用为 nil
@property (nonatomic, readonly, nullable) IRKeyboardInfo *keyboardInfo;

#pragma mark block 型 (转到上面的方法)

/// app.inputText(text, diag): 标准模式
@property (nonatomic, readonly) BOOL (^inputText)(NSString *text, IRDiagnostics *_Nullable diag);

/// app.clearText(diag)
@property (nonatomic, readonly) BOOL (^clearText)(IRDiagnostics *_Nullable diag);

/// app.keyboardReturn(diag)
@property (nonatomic, readonly) BOOL (^keyboardReturn)(IRDiagnostics *_Nullable diag);

@end

NS_ASSUME_NONNULL_END
