# C++ 接口

```cpp
#include <XTCommand/CLI11.hpp>     // 只能在 .cpp / .mm 里
```

CLI11 v2.7.2 + XTCommand 补丁。原生 CLI11 代码照样能用。**不要再单独引入另一份 CLI11**(同一 `CLI` 命名空间会符号冲突)。

## 入口

```cpp
return CLI::xt_run(app, argc, argv);                              // 代替 CLI11_PARSE
return CLI::xt_run(app, argc, argv, [&] { return run(); });       // 解析成功后执行,返回值 = 退出码
```

`xt_run` 负责:`argv[1] == "--xt-describe"` 时只输出描述;摘掉 `--json`(`--` 之后不动),之后 `CLI::xt::json_mode()` 为真;
保留名检查(`--json`、`--xt-*`,违反返回 100);异常转退出码(`ParseError` → `app.exit(e)`,其它 `CLI::Error` → 它的退出码,`std::exception` → 1)。

回调里可以 `throw CLI::RuntimeError(3)`(退出码 3,不打印)、`throw CLI::ValidationError("名", "说明")`(105)。

## 扩展方法

```cpp
opt->input_path() / output_path() / input_directory() / output_directory();   // 文件参数
opt->preferred_location(CLI::Location::Mac);     // 建议放 Mac(只写进给 AI 的说明)
opt->choices({"fast", "safe"});                  // 校验 + 导出 enum(别用 check(CLI::IsMember))
opt->range(1, 100);                              // 校验 + 导出 minimum/maximum(别用 check(CLI::Range))
opt->agent_description("给 AI 的说明");

app.mount("dump", "apps dump");                  // 源相对调用它的 App(根上 = 从根算);目标是 iagent 绝对路径
sub->agent_visible(false);
sub->agent_description("给 AI 的说明");
sub->tags({"interactive", "mutation"});          // 命令标签(面向 AI 意图匹配与分类)
app.enable_ai_mode(true);                        // 默认 true:自动注入 --ai、--skill 与 --export-skills(仅根命令)
app.add_shortcut("dp", "dump -b com.demo --plain", "快捷导出"); // 快捷代号/预设宏
CLI::xt::describe(app, ok);                      // 拿到导出 JSON 字符串
CLI::xt::export_skill_markdown(app, ok);         // 单文件 Agent Skill Markdown
CLI::xt::export_skill_dir(app, dir, ok, err);    // 规范化 Skill 目录(SKILL.md + references/)
```

## 示例

```cpp
CLI::App app{"导出工具", "dumper"};
app.require_subcommand(1, 1);
auto *dump = app.add_subcommand("dump", "导出 App");
std::string bundle, out;
int count = 1;
dump->add_option("-b,--bundle", bundle, "bundle id")->required();
dump->add_option("-o,--output", out, "输出文件")->output_path()->preferred_location(CLI::Location::Mac);
dump->add_option("-c,--count", count, "次数")->range(1, 100)->capture_default_str();
app.mount("dump", "apps dump");
return CLI::xt_run(app, argc, argv, [&] { /* … */ return 0; });
```

## 坑

- `add_flag("--x", desc)` 里 `desc` 是 `std::string` 左值 → 被当成结果变量,说明丢失。用 `add_flag("--x")->description(desc)`。
- 子命令要用父命令的选项(继承)→ 子命令 `fallthrough()`;但开了 fallthrough 的子命令收不到 extras。
- 子命令里 `--` 默认表示"本子命令结束";要让 `--` 之后留给本命令:`sub->allow_extras()->prefix_command(CLI::PrefixCommandMode::SeparatorOnly)`。

## 帮助(要填什么见 help.md,排版由框架统一生成)

```cpp
app.xt_version("1.2.0");                                   // 根命令必填:--version + 根帮助带版本
app.authors({"cu", "alice"});                               // 作者:简介行最多 3 个,--version 全部
like->example("awetool video like -c 3", "当前视频点赞 3 次"); // 每一级都写
app.example_block("部署流程", {"awetool a", "awetool b"}, "多步协作场景");
like->help_detail("补充说明,`反引号` 高亮");
like->skill_trigger("TRIGGER when: …");
like->prerequisites({"目标设备在线"});
like->output_schema("{\"liked\":2}");
group->default_action("不带子命令时列出会话");                // 既有回调又有子命令时写
app.help_logo("…");                                         // 可选,根命令品牌字符画
opt->metavar("<COUNT>");
```
`CLI::xt_run` 会自动装上统一的帮助格式和出错格式,不要自己设 `formatter()`。
