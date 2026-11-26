//
//  IRActionSignature.h
//  IRAutomationKit
//
//  事件签名 (ACTION.md 第 30、31、33 项): 事件的"函数签名" —— 入参 (名字、类型、说明、是否必填、默认值) + 返回值 (类型、说明)
//  + 支持哪些模式。同 CHR 的 CHRTypeEncoding; 参数描述复用权重模块的 IRValueSchema (= CHRValueSchema, CHR 的权重项也用它)。
//  - 静态接口: 能序列化 (ir_modelToJSONObject 同字段), 导出"页面 → 事件 → 入参 / 返回值"的报告; 调试工具据此生成输入界面
//  - 调用前: 补默认值、检查必填、检查 (并转换) 类型; 调用后: 检查返回值类型, 查询模式下不能是 nil (空值用 NSNull)
//  - 声明了返回值 = 支持查询模式; 声明了可用性 = 支持可用性询问
//  在构建器里声明 (registerAction:builder:):
//
//      action.arg(@"keyword", IRSchemaTypeString).setComment(@"关键词");                  // 默认必填
//      action.arg(@"wait", IRSchemaTypeFloat).setDefaultValue(@8).setComment(@"等几秒");   // 有默认值 = 可选
//      action.result(IRSchemaTypeString).setComment(@"点的词");                            // 支持查询
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRDiagnostics.h>
#import <IRAutomationKit/IRValueSchema.h>

NS_ASSUME_NONNULL_BEGIN

/// 事件签名
@interface IRActionSignature : NSObject

/// 入参 (声明顺序)
@property (nonatomic, copy, readonly) NSArray<IRValueSchema *> *arguments;
/// 返回值 (名字固定 "result"); nil = 没声明 (不支持查询模式)
@property (nonatomic, strong, readonly, nullable) IRValueSchema *result;
/// 声明了可用性 (支持可用性询问)
@property (nonatomic, readonly) BOOL declaresAvailable;
/// 标签 (ACTION.md 第 39 项, 注册时 action.tags 声明; 可多个, 没声明为空数组): SDK 只存不管含义, 业务按标签分类查事件
/// (如列表浏览的 browse.view / browse.slide / browse.exit), 见 IRPageNode availableActions
@property (nonatomic, copy, readonly) NSArray<NSString *> *tags;

/// 按名字取入参描述
- (nullable IRValueSchema *)argumentNamed:(NSString *)name;

/// 调用前: 补默认值、检查必填、检查并转换类型 (如字符串 "3" → 整数 3)。每次调用返回一份新的 (补的默认值不带给别的接收者)。
/// 不通过返回 nil, 原因写 diag ("search: 缺少必填参数 'keyword' (关键词)")
- (nullable NSDictionary<NSString *, id> *)prepareParams:(nullable NSDictionary<NSString *, id> *)params
                                                actionID:(NSString *)actionID diag:(IRDiagnostics *)diag;

/// 调用后: 检查返回值。query = YES 时 nil 不通过 (空值用 NSNull); 有值时类型要符合声明。不通过返回 NO, 原因写 diag
- (BOOL)checkResult:(nullable id)result query:(BOOL)query actionID:(NSString *)actionID diag:(IRDiagnostics *)diag;

/// 序列化: @{arguments: [参数 JSON], result?: {…}, available: BOOL, tags: [字符串]}; 参数 JSON 同 IRValueSchema 的 ir_modelToJSONObject
- (NSDictionary<NSString *, id> *)dictionaryRepresentation;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
