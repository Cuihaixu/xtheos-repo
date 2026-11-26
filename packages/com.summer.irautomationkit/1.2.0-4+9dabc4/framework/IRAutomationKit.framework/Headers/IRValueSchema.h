//
//  IRValueSchema.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/7.
//

#import <Foundation/Foundation.h>
#import "IRWeightModel.h"
#import "IRDiagnostics.h"

NS_ASSUME_NONNULL_BEGIN

typedef NSString *IRSchemaType NS_EXTENSIBLE_STRING_ENUM;

FOUNDATION_EXPORT IRSchemaType const IRSchemaTypeInteger;
FOUNDATION_EXPORT IRSchemaType const IRSchemaTypeFloat;
FOUNDATION_EXPORT IRSchemaType const IRSchemaTypeString;
FOUNDATION_EXPORT IRSchemaType const IRSchemaTypeBool;
FOUNDATION_EXPORT IRSchemaType const IRSchemaTypeArray;
FOUNDATION_EXPORT IRSchemaType const IRSchemaTypeObject;

@protocol IRSchemaValueSerializable <NSObject>

@property (nonatomic, readonly) IRSchemaType ir_schemaType;

@end

@interface IRValueSchema : NSObject<IRModel>

@property (nonatomic, copy, readonly) IRValueSchema *(^setRequired)(BOOL required);
@property (nonatomic, copy, readonly) IRValueSchema *(^setComment)(NSString *format, ...);
@property (nonatomic, copy, readonly) IRValueSchema *(^setDefaultValue)(id<IRSchemaValueSerializable> value);

@property (nonatomic, copy) NSString *name;
@property (nonatomic, copy) IRSchemaType type;
@property (nonatomic, assign) BOOL required;
@property (nonatomic, strong) id<IRSchemaValueSerializable> defaultValue;
@property (nonatomic, copy) NSString *comment;


+ (instancetype)schema:(NSString *)name type:(IRSchemaType)type;
+ (instancetype)schema:(NSString *)name defaultValue:(id<IRSchemaValueSerializable>)value;

@end


NS_ASSUME_NONNULL_END
