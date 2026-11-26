//
//  01-remote-model.m
//  IRAutomationKit 示例 [EX-MODEL-01] —— IRObject 强类型远程映射骨架
//

#import <IRAutomationKit/IRAutomationKit.h>

@interface AWBDemoAweme : IRObject
@property (nonatomic, readonly, nullable) NSString *itemID;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
@end

@implementation AWBDemoAweme

+ (void)load {
    // 注册后由 IRemoteKit 自动返回 AWBDemoAweme 实例；严禁自定义 +awemeWithObject: 等工厂方法
    [IRObject registerSubclass:self forRemoteClass:@"AWEAwemeModel"];
}

- (NSString *)itemID {
    return [self ivarNamed:@"_itemID"].value.stringValue;
}

@end

// 在 PageAgent 中使用：直接从 ivarNamed:@"_model" 拿到强类型映射对象，并用属性缓存
// id model = [self.remoteObject ivarNamed:@"_model"].value.objectValue;
// if ([model isKindOfClass:AWBDemoAweme.class]) { ... }
