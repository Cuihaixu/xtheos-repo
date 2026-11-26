// 同步网络库 SDK。调用方自行选择后台线程；所有函数在当前线程阻塞至网络/磁盘操作完成。
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN
#ifndef CHR_TOUCH_KIT_API
#define CHR_TOUCH_KIT_API __attribute__((visibility("default")))
#endif

/// 使用完整 API Base URL 创建或取得本地网络配置；URL 可包含 /v1 等挂载路径。返回稳定 configurationId。
CHR_TOUCH_KIT_API NSString * _Nullable CHRNetworkLibraryLoadConfiguration(NSString *URL, NSError * _Nullable * _Nullable error);
/// 同步获取该配置服务端的全部网络库摘要（NSArray<NSDictionary *>）。
CHR_TOUCH_KIT_API NSArray<NSDictionary *> * _Nullable CHRNetworkLibraryFetchLibraries(NSString *configurationId, NSError * _Nullable * _Nullable error);
/// 本地镜像里某网络库的 revision（无镜像返回 0）。库级增量用：与列表返回的 revision 比对，相同即可跳过同步。
CHR_TOUCH_KIT_API uint64_t CHRNetworkLibraryLocalRevision(NSString *serverLibraryId);
/// 同步进度回调：step/total 为阶段序号，phase 为阶段名（网络库基础信息 / 手势记录 / 回放方案）。
typedef void (^CHRNetworkLibrarySyncProgress)(NSInteger step, NSInteger total, NSString *phase);
/// 同步指定服务端库；成功返回本地网络镜像 libraryId。
CHR_TOUCH_KIT_API NSString * _Nullable CHRNetworkLibrarySync(NSString *configurationId, NSString *serverLibraryId, NSError * _Nullable * _Nullable error);
/// 带进度回调的同步。progress 可为 nil；在各阶段开始时回调（当前线程）。
CHR_TOUCH_KIT_API NSString * _Nullable CHRNetworkLibrarySyncWithProgress(NSString *configurationId, NSString *serverLibraryId, CHRNetworkLibrarySyncProgress _Nullable progress, NSError * _Nullable * _Nullable error);
/// 列出本机已添加的全部网络配置（服务器），每项 @{ @"configurationId": ..., @"url": ... }。
CHR_TOUCH_KIT_API NSArray<NSDictionary *> *CHRNetworkLibraryConfigurations(void);
/// 列出本机已订阅（添加）的全部网络库，跨所有服务器。每项
/// @{ @"configurationId", @"url", @"serverLibraryId", @"name" }。用于"同步全部已添加网络库"。
CHR_TOUCH_KIT_API NSArray<NSDictionary *> *CHRNetworkLibrarySubscribedLibraries(void);

/// 同步全部已订阅网络库（库级增量：未变的库跳过）。整套循环在 chr:netlib 服务里执行，
/// 本函数只发一次 IPC。返回汇总 @{ @"synced", @"skipped", @"failed", @"results":[…] }；失败返回 nil。
CHR_TOUCH_KIT_API NSDictionary * _Nullable CHRNetworkLibrarySyncAll(NSError * _Nullable * _Nullable error);

NS_ASSUME_NONNULL_END
