//
//  IRPageAgent+IRWeight.h
//  IRAutomationKit
//
//  权重和页面 Agent 的对接 (WEIGHT.md "和 PageAgent 的对接", 照 CHRPageAgent (CHRWeight)):
//  - 空间 ID = 页面 Agent 的类名 (同 CHR, 和 CHR 服务端配置兼容)
//  - Agent 自己就是选择时的 Context: 默认强度 0、不禁用; 业务的页面 Agent 重写这两个方法, 按当前情况给
//  - 分类在权重模块里, 单向依赖 SDK; SDK 核心不知道权重模块
//
//      + (void)load {
//          [self registerWeight:^(id<IRWeightSpaceDescriptorBuilder> space) {
//              [space registerGroup:IRIdentityMake(@"watch_or_skip", @"看还是划走") builder:^(id<IRWeightGroupDescriptorBuilder> group) {
//                  group.registerItem(IRIdentityMake(@"watch", nil), 75);
//                  group.registerItem(IRIdentityMake(@"skip", nil), 25).setIncludeFactors(@[ @"type_boredom" ]);
//              }];
//          }];
//      }
//      - (int)factorStrengthForName:(NSString *)name { … }
//      IRWeightSelection *s = [self weightSelect:@"watch_or_skip"];
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRPageAgent.h>
#import "IRDiagnostics.h"
#import "IRWeightContext.h"
#import "IRWeightSpace.h"
#import "IRWeightSelection.h"
#import "IRWeightSpaceDescriptor.h"

NS_ASSUME_NONNULL_BEGIN

@interface IRPageAgent (IRWeight) <IRWeightContext>

/// 注册这个页面 Agent 类的权重空间 (空间 ID = 类名); 写在 +load 里
+ (void)registerWeight:(void (^)(id<IRWeightSpaceDescriptorBuilder> space))block;

/// 这个页面的权重空间 (没注册为 nil)
@property (nonatomic, readonly, nullable) IRWeightSpace *weightSpace;

/// 在这个页面的空间里选择 (Context 是 Agent 自己, 自动提交); 组不存在返回 nil
- (nullable IRWeightSelection *)weightSelect:(NSString *)groupID;

/// 同上, 选择过程写进 diag 的 verbose
- (nullable IRWeightSelection *)weightSelect:(NSString *)groupID diag:(nullable IRDiagnostics *)diag;

/// Context 默认实现: 强度 0 (业务重写)
- (int)factorStrengthForName:(NSString *)name;

/// Context 默认实现: 不禁用 (业务重写)
- (NSSet<NSString *> *)disabledItemIDsWithPath:(IRWeightGroupPath *)path;

@end

NS_ASSUME_NONNULL_END
