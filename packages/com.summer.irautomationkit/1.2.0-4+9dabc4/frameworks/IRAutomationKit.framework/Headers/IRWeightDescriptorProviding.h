//
//  IRWeightDescriptorProviding.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/27.
//

#import <Foundation/Foundation.h>
#import "IRWeightDescriptor.h"
#import "IRWeightManager.h"

NS_ASSUME_NONNULL_BEGIN

/// A lightweight capability protocol for objects that can expose
/// their weight configuration as a descriptor tree.
///
/// The returned descriptor is a **definition/template** of what this
/// object supports (structure + configurable fields), **not** runtime
/// data nor server adjustments.
@protocol IRWeightDescriptorProviding <NSObject>

/// Returns a descriptor object that describes the receiver’s weight
/// configuration (the “what can be configured” view).
///
/// - Returns:
///   A concrete subclass of `IRWeightDescriptor`:
///   - `IRWeightManagerDescriptor` when called on `IRWeightManager`
///   - `IRWeightSpaceDescriptor`  when called on `IRWeightSpace`
///   - `IRWeightGroupDescriptor`  when called on `IRWeightGroup`
///   - `IRWeightItemDescriptor`   when called on `IRWeightItem`
///
/// The descriptor is suitable for:
/// - Inspecting supported spaces/groups/items
/// - Generating configuration templates
/// - Debugging or tooling that needs a structural view
- (__kindof IRWeightDescriptor *)weightDescriptor;

@end

/// Provides a descriptor for the entire weight manager (all spaces).
@interface IRWeightManager (WeightDescriptor) <IRWeightDescriptorProviding>

@end

/// Provides a descriptor for a single weight space (optionally per profile).
@interface IRWeightSpace (WeightDescriptor) <IRWeightDescriptorProviding>

@end

/// Provides a descriptor for a single weight group (and its items).
@interface IRWeightGroup (WeightDescriptor) <IRWeightDescriptorProviding>

@end

/// Provides a descriptor for a single weight item.
@interface IRWeightItem (WeightDescriptor) <IRWeightDescriptorProviding>

@end

NS_ASSUME_NONNULL_END
