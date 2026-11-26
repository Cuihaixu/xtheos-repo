//
//  IRAgentRegistration.h
//  IRAutomationKit
//
//  注册配置协议: +registerAgent: 的 block 里填写。block 结束后检查完备性:
//    Debug: NSAssert; Release: 日志 + 拒绝注册
//  每种 Agent 有自己的配置协议 (继承这个): IRPageAgentRegistration / IRViewAgentRegistration / IRWindowAgentRegistration /
//  IRApplicationAgentRegistration / IRResponderAgentRegistration, 写在各自的 Agent 头文件里
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol IRAgentRegistration <NSObject>

/// 绑定的远程类名 (必填): 同一个应用里一个类只能注册一次
@property (nonatomic, copy, nullable) NSString *bindClass;

/// 标签 (辅助: 分组、筛选、日志)
@property (nonatomic, copy, nullable) NSString *tag;

/// 描述
@property (nonatomic, copy, nullable) NSString *comment;

/// 所属应用; 不设 = 继承 +bundleIDs
@property (nonatomic, copy, nullable) NSArray<NSString *> *bundleIDs;

@end

NS_ASSUME_NONNULL_END
