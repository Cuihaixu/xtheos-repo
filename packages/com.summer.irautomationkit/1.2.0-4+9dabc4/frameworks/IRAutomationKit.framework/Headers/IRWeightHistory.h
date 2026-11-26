//
//  IRWeightHistory.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/27.
//

#import <Foundation/Foundation.h>
#import "IRWeightModel.h"
#import "IRDiagnostics.h"
#import "IRWeightDefines.h"
#import "IRWeightManager.h"


NS_ASSUME_NONNULL_BEGIN

@interface IRWeightHistory : NSObject <IRModel>

/// 存储历史的区域
@property (nonatomic, copy, readonly) IRWeightRegion region;

/// 历史标识
@property (nonatomic, copy, readonly) NSString *historyID;

/// 工厂方法, 自动识别数据区域类型, 返回子类对象。
/// - Parameter info: 历史数据
+ (nullable instancetype)historyWithInfo:(NSDictionary *)info;

/// 创建权重历史实例
/// - Parameter historyID: 历史标识
+ (instancetype)historyWithHistoryID:(NSString *)historyID;

@end

@protocol IRWeightHistoryCoding <NSObject>

/// 导出权重历史
- (__kindof IRWeightHistory *)exportWeightHistory;

/// 导入权重历史
- (BOOL)importWeightHistory:(__kindof IRWeightHistory *)history;

@end

@interface IRWeightItem (WeightHistory)<IRWeightHistoryCoding>

@end

@interface IRWeightGroup (WeightHistory)<IRWeightHistoryCoding>

@end

@interface IRWeightSpace (WeightHistory)<IRWeightHistoryCoding>

@end

@interface IRWeightManager (WeightHistory)<IRWeightHistoryCoding>

@end



NS_ASSUME_NONNULL_END
