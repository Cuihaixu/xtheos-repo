//
//  IRListBrowseDelegate.h
//  IRAutomationKit
//
//  列表浏览的代理 (BROWSE.md 统计三层): 跨范围的统计 (本次搜索 / 应用本次运行 / 今日 …) 归业务, 框架只通知。
//  协议只有一个方法, 事件用对象表示, 类型是字符串常量 (以后加类型不改协议)。代理在场景上设 (flow.delegate), 不设就不通知。
//  判定规则 (什么算展示 / 曝光 / 点击) 在框架里, 代理只收结果自己累加。
//  要回答的事件类型 (WillDecide): 代理往 event.result 写 (只放 JSON), 框架放进 list.extra (发服务器时也带上); 别的类型写了也忽略
//
//      - (void)listBrowse:(IRListBrowseContext *)list event:(IRListBrowseEvent *)event {
//          if ([event.type isEqualToString:IRListBrowseEventClick]) [self recordClick:event.items.firstObject];
//          else if ([event.type isEqualToString:IRListBrowseEventWillDecide])
//              event.result[@"ad_clicked_today"] = @([self clickedToday:event.step.item.key]);
//      }
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class IRListBrowseContext, IRListBrowseStep, IRListBrowseItem;

/// 事件类型
typedef NSString *IRListBrowseEventType NS_TYPED_EXTENSIBLE_ENUM;
FOUNDATION_EXPORT IRListBrowseEventType const IRListBrowseEventStart;       ///< 开始浏览 (prepareWithParams: 之后)
FOUNDATION_EXPORT IRListBrowseEventType const IRListBrowseEventWillDecide;  ///< 每一步决策前 (要回答: event.result → list.extra)
FOUNDATION_EXPORT IRListBrowseEventType const IRListBrowseEventShow;        ///< 展示: 新出现在屏幕上的条目 (items, 依赖业务的 visibleItems)
FOUNDATION_EXPORT IRListBrowseEventType const IRListBrowseEventExposure;    ///< 曝光: browse.exposure 的事件成功, 条目第一次 (items = 当前条)
FOUNDATION_EXPORT IRListBrowseEventType const IRListBrowseEventClick;       ///< 点击: browse.click 的事件成功 (items = 当前条)
FOUNDATION_EXPORT IRListBrowseEventType const IRListBrowseEventInteract;    ///< 互动: 查看阶段成功的事件 (含观看)
FOUNDATION_EXPORT IRListBrowseEventType const IRListBrowseEventStep;        ///< 每一步走完 (含跳过 / 失败; step 已填好结果)
FOUNDATION_EXPORT IRListBrowseEventType const IRListBrowseEventFinish;      ///< 结束 (info = flow 事件的结果)

@interface IRListBrowseEvent : NSObject
@property (nonatomic, copy, readonly) IRListBrowseEventType type;
@property (nonatomic, strong, readonly, nullable) IRListBrowseStep *step;
@property (nonatomic, copy, readonly, nullable) NSArray<IRListBrowseItem *> *items;
@property (nonatomic, copy, readonly, nullable) NSDictionary *info;
/// 代理要回答时写这里 (只放 JSON 类型); 框架只在要回答的类型上读
@property (nonatomic, strong, readonly) NSMutableDictionary *result;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
@end

@protocol IRListBrowseDelegate <NSObject>
/// 在执行 flow 的 UI 任务线程上同步调 (别做耗时的事); 抛异常只打日志, 不影响浏览
- (void)listBrowse:(IRListBrowseContext *)list event:(IRListBrowseEvent *)event;
@end

NS_ASSUME_NONNULL_END
