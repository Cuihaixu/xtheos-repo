//
//  IRKeyboardInfo.h
//  IRAutomationKit
//
//  键盘状态快照 (app.keyboardInfo, 取自 CHRTouchCopyKeyboardState): 读取那一刻当前焦点输入框和键盘的状态, 之后不再变
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRKeyboardInfo : NSObject

/// 整框文字 = contextBefore + markedText + contextAfter
@property (nonatomic, copy, readonly) NSString *text;

/// 光标前的文字 (光标在末尾时就是整框文字)
@property (nonatomic, copy, readonly) NSString *contextBefore;

/// 组合态文字 (正在打的拼音, 如 "zu qiu" / "足qiu"); 没有为 nil
@property (nonatomic, copy, readonly, nullable) NSString *markedText;

/// 光标后的文字
@property (nonatomic, copy, readonly) NSString *contextAfter;

/// 选中的文字
@property (nonatomic, copy, readonly) NSString *selectedText;

/// 光标位置 (= 光标前的字符数)
@property (nonatomic, readonly) NSUInteger cursorIndex;

/// 是否密码框
@property (nonatomic, readonly, getter=isSecureEntry) BOOL secureEntry;

/// return 键类型: default / go / join / next / route / search / send / done / continue …
@property (nonatomic, copy, readonly) NSString *returnKeyType;

/// return 键当前是否可点 (空框且自动禁用 return 时为 NO)
@property (nonatomic, readonly) BOOL returnKeyEnabled;

/// 当前输入法标识, 如 zh_Hans-Pinyin
@property (nonatomic, copy, readonly) NSString *language;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
