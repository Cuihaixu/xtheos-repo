//
//  IRWeightDescriptorSignature.h
//  IRAutomationKit
//
//  Created by Codex on 2026/5/7.
//

#import <Foundation/Foundation.h>
#import "IRWeightDescriptor.h"
#import "IRWeightManager.h"
#import "IRWeightManagerDescriptor.h"

NS_ASSUME_NONNULL_BEGIN

@interface IRWeightDescriptor (IRStructureSignature)

/// Stable structural signature of the descriptor node.
///
/// Runtime/default tunable values such as weight value, disable state,
/// comments and current include factors are intentionally not included.
- (NSString *)ir_structureSignature;

@end

@interface IRWeightManagerDescriptor (IRWeightDataVersion)

/// Client-side data version for the full weight descriptor tree.
- (NSString *)ir_weightDataVersion;

@end

@interface IRWeightManager (IRWeightDataVersion)

/// Client-side data version exported with raw weight configuration.
- (NSString *)ir_weightDataVersion;

@end

NS_ASSUME_NONNULL_END
