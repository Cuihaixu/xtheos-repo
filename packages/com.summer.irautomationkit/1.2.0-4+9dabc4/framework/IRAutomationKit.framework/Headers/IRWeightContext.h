//
//  IRWeightContext.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/21.
//

#import <Foundation/Foundation.h>
#import "IRWeightPath.h"

NS_ASSUME_NONNULL_BEGIN

@protocol IRWeightContext <NSObject>

/// 获取因素强度
/// - Parameter name: 因素名称
- (int)factorStrengthForName:(NSString *)name;

/// 获取需要禁用权重项标识
/// - Parameter path: 权重组路径
- (NSSet<NSString *> *)disabledItemIDsWithPath:(IRWeightGroupPath *)path;

@end

NS_ASSUME_NONNULL_END
