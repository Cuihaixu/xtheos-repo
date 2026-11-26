//
//  IRActionBuilder.h
//  IRAutomationKit
//
//  动作构建器 (ACTION.md 第 29 项, 同 CHR 的 registerAction:builder:): 注册时需要哪部分就设置哪部分, 以后加配置项只加字段。
//  不需要签名的简单动作用 registerAction:block: 即可 (= 只有 perform 的简写)
//
//      [self registerAction:IRIdentityMake(@"search_guess", @"点击猜你想搜") builder:^(IRActionBuilder *action) {
//          [action setTypeEncodingBlock:^(IRTypeEncoding *encoding) {
//              encoding.arg(@"keyword", IRSchemaTypeString).setComment(@"关键词");      // 入参 (默认必填)
//              encoding.result(IRSchemaTypeString).setComment(@"点的词");             // 返回值: 声明了 = 支持查询模式
//          }];
//          action.available(YES);                                               // 支持可用性询问 (默认不支持 = 总是可用)
//          action.tags(@[ @"browse.view" ]);                                    // 标签 (可多个): 按标签查当前可用事件
//          [action setPerformBlock:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
//              if (context.isAvailable) { return …能不能做…; }
//              if (context.isQuery)     { context.result = …; return YES; }
//              …执行…
//          }];
//      }];
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRActionContext.h>
#import <IRAutomationKit/IRValueSchema.h>

NS_ASSUME_NONNULL_BEGIN

@class IRApplication;

/// 动作的执行体: 返回 YES 成功; NO 失败 (失败原因记在 diag)
/// app / diag 就是 context.app / context.diag, 动作里最常用, 直接作为参数给出
typedef BOOL (^IRActionBlock)(IRApplication *app, IRActionContext *context, IRDiagnostics *diag);

/// 参数与返回值编码器: 在 [action setTypeEncodingBlock:^(IRTypeEncoding *encoding) { ... }] 内集中声明
@interface IRTypeEncoding : NSObject

/// 声明一个入参 (默认必填; .setDefaultValue(值) 后变可选), 返回参数描述继续链式设置
@property (nonatomic, readonly) IRValueSchema *(^addArgument)(NSString *name, IRSchemaType type);

/// 同 addArgument 简写
@property (nonatomic, readonly) IRValueSchema *(^arg)(NSString *name, IRSchemaType type);

/// 按默认值自动推导类型并设为可选入参 (如 encoding.argWith(@"direction", @"up"))
@property (nonatomic, readonly) IRValueSchema *(^argWith)(NSString *name, id<IRSchemaValueSerializable> defaultValue);

/// 声明返回值: 声明了 = 支持查询模式 (invokeQuery); 返回描述继续链式设置
@property (nonatomic, readonly) IRValueSchema *(^result)(IRSchemaType type);

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

@interface IRActionBuilder : NSObject

/// 集中声明入参和返回值 (推荐写法): [action setTypeEncodingBlock:^(IRTypeEncoding *encoding) { ... }];
- (void)setTypeEncodingBlock:(NS_NOESCAPE void (^)(IRTypeEncoding *encoding))block;

/// setTypeEncodingBlock: 的点语法写法
@property (nonatomic, readonly) void (^typeEncoding)(NS_NOESCAPE void (^block)(IRTypeEncoding *encoding));

/// 已作废：禁止在 IRActionBuilder 上直接调用，改用 [action setTypeEncodingBlock:^(IRTypeEncoding *encoding) { encoding.arg(...); }]
@property (nonatomic, readonly) IRValueSchema *(^arg)(NSString *name, IRSchemaType type) __attribute__((unavailable("已作废，改用 setTypeEncodingBlock:")));

/// 已作废：禁止在 IRActionBuilder 上直接调用，改用 [action setTypeEncodingBlock:^(IRTypeEncoding *encoding) { encoding.result(...); }]
@property (nonatomic, readonly) IRValueSchema *(^result)(IRSchemaType type) __attribute__((unavailable("已作废，改用 setTypeEncodingBlock:")));

/// 声明支持可用性询问: YES 时页面节点会用可用性模式问它 (context.isAvailable); 不声明 = 总是可用
@property (nonatomic, readonly) void (^available)(BOOL supported);

/// 标签 (可多个, 可调多次累加, 去重): SDK 只存不管含义; IRPageNode availableActions(tags) 按标签查当前可用事件
@property (nonatomic, readonly) void (^tags)(NSArray<NSString *> *tags);

/// 执行体 (必需): 三种模式都走它, 用 context.mode 区分
- (void)setPerformBlock:(IRActionBlock)block;

/// 已作废：禁止使用点语法，改用 [action setPerformBlock:...]
@property (nonatomic, readonly) void (^perform)(IRActionBlock block) __attribute__((unavailable("已作废，改用 setPerformBlock:")));

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
