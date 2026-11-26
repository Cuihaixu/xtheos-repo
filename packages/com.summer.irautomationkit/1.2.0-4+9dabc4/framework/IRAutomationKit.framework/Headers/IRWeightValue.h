//
//  IRWeightValue.h
//  IRAutomationKit
//
//  Created by cu on 2026/1/30.
//

#import <Foundation/Foundation.h>
#import "IRWeightModel.h"
#import "IRDiagnostics.h"

NS_ASSUME_NONNULL_BEGIN

typedef NSString IRValueType NS_TYPED_ENUM;

/// 类型信息构建协议
@protocol IRValueTypeInfoBuilder <NSObject>

/// 类型
@property (nonatomic, copy) IRValueType *type;
/// 类型注释
@property (nonatomic, copy) NSString *comment;

@end

/// 类型描述类
@interface IRValueTypeDescriptor : NSObject <IRModel>

@property (nonatomic, copy, readonly) IRValueType *type;
@property (nonatomic, copy, readonly) NSString *comment;

@end

@interface IRWeightValue : NSObject <IRModel>

/// 类型
@property (nonatomic, copy, readonly) IRValueType *type;

/// 构建实例
/// - Parameter info: 实例信息
+ (nullable __kindof instancetype)valueWithInfo:(NSDictionary *)info;

@end

@interface IRWeightValue (TypeRegister)

/// 全部已注册类型描述
@property (nonatomic, class, readonly) NSDictionary<IRValueType *, IRValueTypeDescriptor *> *typeDescriptors;

/// 注册类型
/// - Parameter block: 构建类型信息
+ (void)registerType:(void(^)(id<IRValueTypeInfoBuilder> info))block;

@end


NS_ASSUME_NONNULL_END
