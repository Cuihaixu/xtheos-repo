//
//  IRWeightPath.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/28.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRWeightPath : NSObject

@end

@interface IRWeightSpacePath : IRWeightPath

@property (nonatomic, copy, readonly) NSString *spaceID;

@end


@interface IRWeightProfilePath : IRWeightSpacePath

@property (nonatomic, copy, readonly) NSString *profileID;

@end


@interface IRWeightGroupPath : IRWeightProfilePath

@property (nonatomic, copy, readonly) NSString *groupID;

@end


@interface IRWeightItemPath : IRWeightGroupPath

@property (nonatomic, copy, readonly) NSString *itemID;

@end

IRWeightSpacePath *IRWeightSpacePathMake(NSString *spaceID);
IRWeightProfilePath *IRWeightProfilePathMake(NSString *profileID, NSString *spaceID);
IRWeightGroupPath *IRWeightGroupPathMake(NSString *groupID, NSString *_Nullable profileID, NSString *_Nullable spaceID);
IRWeightItemPath *IRWeightItemPathMake(NSString *itmeID, NSString *groupID, NSString *_Nullable profileID, NSString *_Nullable spaceID);

NS_ASSUME_NONNULL_END
