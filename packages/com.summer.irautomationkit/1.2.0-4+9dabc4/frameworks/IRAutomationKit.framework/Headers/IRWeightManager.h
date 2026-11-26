//
//  IRWeightManager.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/20.
//

#import <Foundation/Foundation.h>
#import "IRWeightGroup.h"
#import "IRWeightFactor.h"
#import "IRWeightSpace.h"
#import "IRWeightSpaceDescriptor.h"
//#import "IRWeightGroup.h"

//#import "IRWeightGroup.h"
//#import "IRWeightFactor.h"
//#import "IRWeightSpace.h"
//#import "IRWeightSpaceDescriptor.h"
#import "IRIdentityType.h"

NS_ASSUME_NONNULL_BEGIN

/// The central entry point of the weight system.
///
/// `IRWeightManager` is responsible for:
///  - Registering global weight factors (shared across all spaces).
///  - Registering weight spaces via `IRWeightSpaceDescriptor`.
///  - Providing access to `IRWeightSpace` instances at runtime.
///
/// It does **not** deal with runtime statistics, server adjustments,
/// or history persistence directly; those concerns are handled by
/// dedicated modules (adjust/history/descriptor) built on top of
/// this manager.
@interface IRWeightManager : NSObject

/// Returns the shared weight manager instance.
/// This is the main entry point for registering spaces and global factors.
+ (instancetype)sharedManager;

/// Registers a global weight factor.
/// Global factors are available to all spaces/groups/items through the
/// `IRWeightFactorProvider` lookup chain.
///
/// - Parameters:
///   - factor: The factor instance to register. Must not be nil.
///   - name:   The unique name of the factor. Must not be nil.
- (void)registerFactor:(IRWeightFactor *)factor
               forName:(id<IRIdentityType>)name;

/// Registers a weight space using a descriptor builder.
/// The descriptor describes the static structure (groups/items) and
/// default configuration for the given space.
///
/// - Parameters:
///   - spaceID: A unique identifier for the space.
///   - block:   A builder block that configures the
///              `IRWeightSpaceDescriptor` for the given spaceID.
- (void)registerSpace:(id<IRIdentityType>)spaceID
              builder:(void(^)(id<IRWeightSpaceDescriptorBuilder> space))block;

/// Returns the weight space for the given space identifier, if any,
/// using the default profile if applicable.
///
/// - Parameter spaceID: The identifier of the space.
/// - Returns: The corresponding `IRWeightSpace` instance, or nil if
///            the spaceID has not been registered.
- (nullable IRWeightSpace *)spaceForID:(NSString *)spaceID;

/// Returns the weight space for the given space and profile identifiers, if any.
///
/// - Parameters:
///   - spaceID:   The identifier of the space.
///   - profileID: The profile identifier within the space. Pass nil to
///                refer to the default profile.
/// - Returns: The corresponding `IRWeightSpace` instance for the given
///            space/profile combination, or nil if it does not exist.
- (nullable IRWeightSpace *)spaceForID:(NSString *)spaceID
                              profileID:(NSString * _Nullable)profileID;

@end

NS_ASSUME_NONNULL_END
