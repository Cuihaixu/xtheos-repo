//
//  IRIdentityType.h
//  IRAutomationKit
//
//  Created by cu on 2026/2/2.
//

#import <Foundation/Foundation.h>
#import "IRIdentity.h"

NS_ASSUME_NONNULL_BEGIN

@protocol IRIdentityType <NSObject>

@end

typedef struct {
    NSString *identity;
    NSString *comment;
} IRIdentityValue;

BOOL IRIdentityGetValue(id<IRIdentityType> object, IRIdentityValue *value);

@interface NSString (IRIdentityType) <IRIdentityType>

@end

@interface IRIdentity (IRIdentityType) <IRIdentityType>

@end

NS_ASSUME_NONNULL_END
