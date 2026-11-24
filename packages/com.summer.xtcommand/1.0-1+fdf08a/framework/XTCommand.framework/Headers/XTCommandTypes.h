//
//  XTCommandTypes.h
//  XTCommand —— 公共枚举
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// 参数值的类型
typedef NS_ENUM(NSInteger, XTValueType) {
    XTValueString = 0,      ///< NSString
    XTValueInteger,         ///< NSInteger
    XTValueDouble,          ///< double
    XTValueInputPath,       ///< 输入文件(手机路径;MCP 调用时可来自 Mac,自动推到手机)
    XTValueOutputPath,      ///< 输出文件(手机路径;MCP 调用时可指定放 Mac,执行完自动拷回)
    XTValueInputDirectory,  ///< 输入目录
    XTValueOutputDirectory, ///< 输出目录
};

/// 同一选项出现多次时怎么取值
typedef NS_ENUM(NSInteger, XTMultiplePolicy) {
    XTMultiplePolicyError = 0, ///< 报错(默认)
    XTMultiplePolicyTakeLast,  ///< 取最后一次
    XTMultiplePolicyTakeFirst, ///< 取第一次
    XTMultiplePolicyTakeAll,   ///< 全部保留
    XTMultiplePolicyJoin,      ///< 用换行拼成一个字符串
};

/// 渲染帮助的选项(helpTextWithOptions:)
typedef NS_OPTIONS(NSUInteger, XTHelpOptions) {
    XTHelpOptionNone    = 0,
    XTHelpOptionNoColor = 1 << 0, ///< 无色(出纯文本版)
    XTHelpOptionAIMode  = 1 << 1, ///< AI 模式(纯文本版,同 --ai)
};

/// 文件参数建议放在哪(只写进给 AI 的说明,不强制)
typedef NS_ENUM(NSInteger, XTLocation) {
    XTLocationDevice = 0,
    XTLocationMac,
};

NS_ASSUME_NONNULL_END
