//
//  IRWeightSelection.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/28.
//

#import <Foundation/Foundation.h>
#import "IRParamsAccessor.h"

NS_ASSUME_NONNULL_BEGIN

@class IRWeightGroup, IRWeightItem;

@interface IRWeightSelection : NSObject

@property (nonatomic, assign, readonly) BOOL isCommit;

@property (nonatomic, strong, readonly) IRWeightItem *item;

@property (nonatomic, strong, readonly) IRParamsAccessor *params;

- (void)commit;
- (void)rollback;

@end

NS_ASSUME_NONNULL_END

