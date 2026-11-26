//
//  IRWeightGroup.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/20.
//

#import <Foundation/Foundation.h>
#import "IRDiagnostics.h"
#import "IRWeightItem.h"
#import "IRWeightContext.h"
#import "IRWeightFactor.h"
#import "IRWeightSelection.h"


//#import "IRWeightItem.h"
//#import "IRWeightContext.h"
//#import "IRWeightFactor.h"
//#import "IRWeightSelection.h"

NS_ASSUME_NONNULL_BEGIN
@class IRWeightSpace;
@interface IRWeightGroup : NSObject <IRWeightFactorChain>

/// 权重组名称
@property (nonatomic, readonly) NSString *groupID;

/// 权重空间
@property (nonatomic, weak, readonly) IRWeightSpace *space;

/// 权重选择次数
@property (nonatomic, readonly) NSInteger totalUnitCount;

/// 获取权重条目
/// - Parameter itemID: 条目标识
- (nullable IRWeightItem *)itemForID:(NSString *)itemID;

/// 权重选择
/// - Parameter context: 上下文信息
- (nullable IRWeightSelection *)selectWithContext:(id<IRWeightContext>)context;

/// 权重选择
/// - Parameters:
///   - context: 权重上下文对象
///   - commit: 是否自动提交 (需要根据任务执行结果更新权重选中次数 commit == NO)
- (nullable IRWeightSelection *)selectWithContext:(id<IRWeightContext>)context commit:(BOOL)commit;
/// 同上, 选择过程写进 diag 的 verbose (CHR 里是 printf; 传 nil 不写)
- (nullable IRWeightSelection *)selectWithContext:(id<IRWeightContext>)context commit:(BOOL)commit diag:(nullable IRDiagnostics *)diag;

@end


NS_ASSUME_NONNULL_END
