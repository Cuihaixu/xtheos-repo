//
//  01-interrupt-agent.m
//  IRAutomationKit 示例 [EX-INT-01] —— <IRInterruptHandling> 弹窗中断与 back 事件复用
//

#import <IRAutomationKit/IRAutomationKit.h>

@interface AWBInterruptDemoAgent : IRPageAgent <IRInterruptHandling>
@end

@implementation AWBInterruptDemoAgent

+ (void)load {
    [self registerAgent:^(id<IRPageAgentRegistration> registration) {
        registration.bindClass = @"AWEUpgradeDialogViewController";
        registration.comment = @"弹窗示例";
    }];

    // 关闭逻辑直接写在 back 事件里，禁止抽成 -closeDialogWithDiag: 私有方法
    [self registerAction:IRIdentityMake(@"back", @"关闭弹窗")
                   block:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
        do {
            context.agent.query.views(@"UIButton").axLabels(@"关闭*").visible().first().element.tap(diag);
            if (diag.hasError) {
                break;
            }
            app.waitUntil(^BOOL(IRScreenState *screen) {
                return screen.interruptAgent != (id)context.page;
            }, 4.0, diag);
        } while (NO);
        return diag.noError;
    }];
}

// 感知队列轻量判定（只读内存/chain，严禁发耗时 IPC）
- (BOOL)shouldInterruptInContext:(IRChainContext *)context {
    return YES;
}

// 中断任务线程上直接复用 back 事件
- (BOOL)handleInterrupt:(IRInterruptContext *)context {
    IRDiagnostics *diag = [IRDiagnostics diagnosticsWithDomain:@"automated.interrupt"];
    return [self invokeAction:@"back" params:nil result:NULL diag:diag];
}

@end
