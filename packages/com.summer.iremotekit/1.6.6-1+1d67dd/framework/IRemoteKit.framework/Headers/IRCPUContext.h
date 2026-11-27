//
//  IRCPUContext.h — ARM64 CPU 寄存器上下文快照 + 单个寄存器视图对象(L0)
//
//  跨模块通用的 ARM64 寄存器上下文模型:
//    - IRRemoteThread.cpu: 远程线程寄存器快照与挂起态安全回写;
//    - IRWatchEvent.cpu:   观察点/断点命中瞬间的目标线程寄存器快照;
//    - 自定义 ABI 调用:    读写 x0..x28 / v0..v31 及目标 scratch 内存。
//
//  提供两种访问形态:
//    1. 点语法对象视图 (IRRegister):
//       cpu.x0.value / cpu.x0.intValue / cpu.x0.cstrValue / cpu.x0.objectValue / cpu.pc.symbol
//    2. 集合遍历与下标:
//       for (IRRegister *reg in cpu.registers) { ... }
//       IRRegister *r = cpu[@"x0"];
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRSession;
@class IRObject;
@class IRSymbolInfo;
@class IRCPUContext;

/// 单个 ARM64 寄存器视图对象。
/// 绑定在所属 `IRCPUContext` 上: 读/写 `.value` 等属性会直通底层 `IRCPUContext` 对应的寄存器槽位。
@interface IRRegister : NSObject

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;

/// 所属 CPU 上下文。
@property (nonatomic, weak, readonly, nullable) IRCPUContext *context;

/// 寄存器名称 (`"x0"`..`"x28"`, `"fp"`, `"lr"`, `"sp"`, `"pc"`, `"cpsr"`, `"v0"`..`"v31"`)。
@property (nonatomic, readonly) NSString *name;

/// 原始 64 位无符号值 / 远程地址 (可读可写，修改会直接更新所属 `IRCPUContext`；
/// `arm64e` 下读取 `pc`/`lr`/`fp`/`sp` 时自动返回已剥除 PAC 签名的有效地址)。
@property (nonatomic) uint64_t value;

#pragma mark - 标量与浮点视图(可读可写)

/// 按有符号 64 位整数读写。
@property (nonatomic) int64_t intValue;
/// 按布尔值读写 (`value != 0`)。
@property (nonatomic) BOOL boolValue;
/// 按双精度浮点 (`double`) 解释低 64 位读写。
@property (nonatomic) double doubleValue;
/// 按单精度浮点 (`float`) 解释低 32 位读写。
@property (nonatomic) float floatValue;

#pragma mark - 跨进程指针解引用视图(免 worker 纯内存读, 按值缓存)

/// 将当前寄存器值视作远程 `const char *` 指针，纯内存安全读取 UTF-8 C 字符串 (最长 1024 字节)。
/// 亦可用于在 OC 方法入口读取 `cpu.x1.cstrValue` (`SEL` 选择子名)。地址无效或非合法 UTF-8 返回 `nil`。
@property (nonatomic, readonly, nullable) NSString *cstrValue;

/// 将当前寄存器值视作远程 Objective-C 对象指针，先经纯内存两级 `isa` 校验确认其为合法活对象后，
/// 返回会话复用池中的 `IRObject *` (`Weak` 借用视图)。若寄存器内是普通整数或无效指针，安全返回 `nil`。
@property (nonatomic, readonly, nullable) IRObject *objectValue;

/// 将当前寄存器值视作代码/函数地址，纯内存反查其所属镜像模块与符号 (`Module!Symbol + offset`)。
@property (nonatomic, readonly, nullable) IRSymbolInfo *symbol;

/// 从该寄存器值指向的远程地址读取指定字节数的数据；不可读返回 `nil`。
- (nullable NSData *)readDataWithLength:(NSUInteger)length;

/// 人类可读单行摘要 (含十六进制值，并自动附带符号 / OC 对象类名 / C 字符串注解)。
- (NSString *)formattedDescription;

@end

/// ARM64 CPU 寄存器上下文快照。
@interface IRCPUContext : NSObject

/// 关联的目标会话 (用于寄存器跨进程指针解引用与 scratch 分配)。
@property (nonatomic, weak, readonly, nullable) IRSession *session;

#pragma mark - 通用寄存器点语法 (x0–x28)

@property (nonatomic, readonly) IRRegister *x0;
@property (nonatomic, readonly) IRRegister *x1;
@property (nonatomic, readonly) IRRegister *x2;
@property (nonatomic, readonly) IRRegister *x3;
@property (nonatomic, readonly) IRRegister *x4;
@property (nonatomic, readonly) IRRegister *x5;
@property (nonatomic, readonly) IRRegister *x6;
@property (nonatomic, readonly) IRRegister *x7;
@property (nonatomic, readonly) IRRegister *x8;
@property (nonatomic, readonly) IRRegister *x9;
@property (nonatomic, readonly) IRRegister *x10;
@property (nonatomic, readonly) IRRegister *x11;
@property (nonatomic, readonly) IRRegister *x12;
@property (nonatomic, readonly) IRRegister *x13;
@property (nonatomic, readonly) IRRegister *x14;
@property (nonatomic, readonly) IRRegister *x15;
@property (nonatomic, readonly) IRRegister *x16;
@property (nonatomic, readonly) IRRegister *x17;
@property (nonatomic, readonly) IRRegister *x18;
@property (nonatomic, readonly) IRRegister *x19;
@property (nonatomic, readonly) IRRegister *x20;
@property (nonatomic, readonly) IRRegister *x21;
@property (nonatomic, readonly) IRRegister *x22;
@property (nonatomic, readonly) IRRegister *x23;
@property (nonatomic, readonly) IRRegister *x24;
@property (nonatomic, readonly) IRRegister *x25;
@property (nonatomic, readonly) IRRegister *x26;
@property (nonatomic, readonly) IRRegister *x27;
@property (nonatomic, readonly) IRRegister *x28;

#pragma mark - 控制与栈帧寄存器 (fp / lr / sp / pc / cpsr)

/// 帧指针 `x29` (`fp`)。
@property (nonatomic, readonly) IRRegister *fp;
/// 链接寄存器 `x30` (`lr`，返回地址)。
@property (nonatomic, readonly) IRRegister *lr;
/// 栈指针 (`sp`)。
@property (nonatomic, readonly) IRRegister *sp;
/// 程序计数器 (`pc`)。
@property (nonatomic, readonly) IRRegister *pc;
/// 程序状态寄存器 (`cpsr`)。
@property (nonatomic, readonly) IRRegister *cpsr;

#pragma mark - 浮点/向量寄存器点语法 (v0–v31)

@property (nonatomic, readonly) IRRegister *v0;
@property (nonatomic, readonly) IRRegister *v1;
@property (nonatomic, readonly) IRRegister *v2;
@property (nonatomic, readonly) IRRegister *v3;
@property (nonatomic, readonly) IRRegister *v4;
@property (nonatomic, readonly) IRRegister *v5;
@property (nonatomic, readonly) IRRegister *v6;
@property (nonatomic, readonly) IRRegister *v7;
@property (nonatomic, readonly) IRRegister *v8;
@property (nonatomic, readonly) IRRegister *v9;
@property (nonatomic, readonly) IRRegister *v10;
@property (nonatomic, readonly) IRRegister *v11;
@property (nonatomic, readonly) IRRegister *v12;
@property (nonatomic, readonly) IRRegister *v13;
@property (nonatomic, readonly) IRRegister *v14;
@property (nonatomic, readonly) IRRegister *v15;
@property (nonatomic, readonly) IRRegister *v16;
@property (nonatomic, readonly) IRRegister *v17;
@property (nonatomic, readonly) IRRegister *v18;
@property (nonatomic, readonly) IRRegister *v19;
@property (nonatomic, readonly) IRRegister *v20;
@property (nonatomic, readonly) IRRegister *v21;
@property (nonatomic, readonly) IRRegister *v22;
@property (nonatomic, readonly) IRRegister *v23;
@property (nonatomic, readonly) IRRegister *v24;
@property (nonatomic, readonly) IRRegister *v25;
@property (nonatomic, readonly) IRRegister *v26;
@property (nonatomic, readonly) IRRegister *v27;
@property (nonatomic, readonly) IRRegister *v28;
@property (nonatomic, readonly) IRRegister *v29;
@property (nonatomic, readonly) IRRegister *v30;
@property (nonatomic, readonly) IRRegister *v31;

#pragma mark - OC 遍历与下标访问

/// 全部通用与控制寄存器有序列表 (`@[x0..x28, fp, lr, sp, pc, cpsr]`)，支持 `for (IRRegister *reg in cpu.registers)`。
@property (nonatomic, readonly) NSArray<IRRegister *> *registers;

/// 全部浮点寄存器有序列表 (`@[v0..v31]`)。
@property (nonatomic, readonly) NSArray<IRRegister *> *floatRegisters;

/// 按序号 `0..28` 取通用寄存器 `x0..x28` (`29` 返回 `fp`，`30` 返回 `lr`)；越界返回 `nil`。
- (nullable IRRegister *)registerAtIndex:(NSUInteger)index;

/// 按序号 `0..31` 取浮点寄存器 `v0..v31`；越界返回 `nil`。
- (nullable IRRegister *)floatRegisterAtIndex:(NSUInteger)index;

/// 按名字下标访问 (`cpu[@"x0"]`, `cpu[@"fp"]`, `cpu[@"x29"]`, `cpu[@"lr"]`, `cpu[@"x30"]`, `cpu[@"sp"]`, `cpu[@"pc"]`, `cpu[@"cpsr"]`, `cpu[@"v0"]`, `cpu[@"d0"]`)。
- (nullable IRRegister *)objectForKeyedSubscript:(NSString *)name;

/// 格式化输出全部通用与控制寄存器摘要表。
- (NSString *)formattedDescription;
/// 转为 JSON 友好的字典表示。
- (NSDictionary<NSString *, id> *)dictionaryRepresentation;

#pragma mark - 兼容原始索引读写与 Scratch 内存接口

/// 读写通用寄存器 x0–x28（`index` 0–28）。
- (void)setGeneralRegister:(NSUInteger)index to:(uint64_t)value;
- (uint64_t)generalRegister:(NSUInteger)index;
/// 读写浮点寄存器 v0–v31 低 64 位（`index` 0–31）。
- (void)setFloatRegister:(NSUInteger)index toBits:(uint64_t)bits;
- (uint64_t)floatRegisterBits:(NSUInteger)index;
/// 在目标内分配一块 RW scratch（本次执行后自动回收）；失败返回 0。
- (IRRemoteAddress)allocateScratchWithSize:(NSUInteger)size;
/// 目标内存写（`address` 是远程地址,不可本地解引用）。
- (BOOL)writeBytes:(const void *)bytes length:(NSUInteger)length toAddress:(IRRemoteAddress)address;
/// 目标内存读；失败返回 `nil`。
- (nullable NSData *)readDataWithLength:(NSUInteger)length fromAddress:(IRRemoteAddress)address;

@end

NS_ASSUME_NONNULL_END
