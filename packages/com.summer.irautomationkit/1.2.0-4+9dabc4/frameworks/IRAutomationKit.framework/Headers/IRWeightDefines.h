//
//  IRWeightDefines.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/27.
//

#ifndef IRWeightDefines_h
#define IRWeightDefines_h

#import <Foundation/Foundation.h>

FOUNDATION_EXPORT NSString * const IRWeightDefaultProfileID;


typedef NSString *IRWeightRegion NS_TYPED_ENUM;

FOUNDATION_EXPORT IRWeightRegion const IRWeightRegionManager;
FOUNDATION_EXPORT IRWeightRegion const IRWeightRegionSpace;
FOUNDATION_EXPORT IRWeightRegion const IRWeightRegionProfile;
FOUNDATION_EXPORT IRWeightRegion const IRWeightRegionGroup;
FOUNDATION_EXPORT IRWeightRegion const IRWeightRegionItem;


FOUNDATION_EXPORT NSErrorDomain const IRWeightErrorDomain;

typedef NS_ERROR_ENUM(IRWeightErrorDomain, IRWeightErrorCode) {
    IRWeightErrorCodeUnknown                = 0,
    IRWeightErrorCodeApplyWeightAdjust    = 100,
    
};

/// 权重模块的锁 (递归, 用于 @synchronized): 注册 / 取空间和组 / 选择 / 提交 / 应用调整 / 历史导入导出都在它下面 (WEIGHT.md 修 #2)
FOUNDATION_EXPORT id IRWeightLockToken(void);

#endif /* IRWeightDefines_h */
