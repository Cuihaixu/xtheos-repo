//
//  IRWeightArray.h
//  IRAutomationKit
//
//  Created by cu on 2026/2/3.
//

#import "IRWeightValue.h"
#import "IRValueSchema.h"

NS_ASSUME_NONNULL_BEGIN

@interface IRWeightArray : IRWeightValue <IRSchemaValueSerializable>

@property (nonatomic, readonly) NSArray<id<IRSchemaValueSerializable>> *objects;

+ (instancetype)arrayWithObjects:(NSArray<id<IRSchemaValueSerializable>> *)objects;
- (instancetype)initWithObjects:(NSArray<id<IRSchemaValueSerializable>> *)objects;
- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
