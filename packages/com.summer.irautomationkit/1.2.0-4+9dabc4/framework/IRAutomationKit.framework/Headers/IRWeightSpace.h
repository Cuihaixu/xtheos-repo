//
//  IRWeightSpace.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/23.
//

#import <Foundation/Foundation.h>
#import "IRWeightGroup.h"
#import "IRWeightFactor.h"

NS_ASSUME_NONNULL_BEGIN

/// A logical weight space, typically mapped to a page or module,
/// optionally scoped by a profile (entry point / scenario).
///
/// This is a runtime view for a given `(spaceID, profileID)` pair:
///  - `spaceID` is stable across all profiles of the same space.
///  - `profileID` identifies a specific configuration profile within that space.
///  - Groups and items are accessed through this object.
@interface IRWeightSpace : NSObject

/// The logical identifier of the space (e.g. "feed", "comment").
/// This value is shared by all profiles of the same space.
@property (nonatomic, readonly) NSString *spaceID;

/// The profile identifier within this space.
/// `nil` or @"default" usually represents the default profile.
@property (nonatomic, readonly, nullable) NSString *profileID;

/// Returns a `IRWeightSpace` view for the given profile within the same space.
///
/// - Parameter profileID:
///   - `nil` / @"default": the default profile of this space.
///   - other: a specific profile ID registered for this space.
/// - Returns: The corresponding `IRWeightSpace` instance, or `nil` if the
///            requested profile is not available.
- (nullable IRWeightSpace *)profileSpaceForID:(nullable NSString *)profileID;

/// Returns the weight group for the given group identifier, if any.
///
/// - Parameter groupID: The identifier of the group within this space/profile.
/// - Returns: The corresponding `IRWeightGroup` instance, or `nil` if the
///            group does not exist.
- (nullable IRWeightGroup *)groupForID:(NSString *)groupID;

@end

NS_ASSUME_NONNULL_END
