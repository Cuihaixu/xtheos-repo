//
//  IRSchemaValueSerializable.h
//  IRAutomationKit
//
//  Created by cu on 2026/2/2.
//

#import <Foundation/Foundation.h>
#import "IRValueSchema.h"
#import "IRWeightValue.h"

NS_ASSUME_NONNULL_BEGIN

@interface NSString (IRSchemaValueSerializable)<IRSchemaValueSerializable>

@end

@interface NSNumber (IRSchemaValueSerializable)<IRSchemaValueSerializable>

@end

@interface IRWeightValue (IRSchemaValueSerializable)<IRSchemaValueSerializable>

@end



NS_ASSUME_NONNULL_END
