# Copilot CLI Workflow Instructions

## 目標：節省 Premium Request，最大化效率

每次收到任務時，**必須遵守以下三階段流程**：

---

## 階段一：規劃（Plan）

1. 使用 `sql` 工具將所有子任務以 TODO 形式記錄到資料庫（`todos` 表）。
2. 為每個 TODO 標記**複雜度等級**（在 `description` 欄位中標記）：
   - `[simple]`：純探索、讀取、搜尋、單行修正
   - `[medium]`：常規程式碼修改、功能實作、重構
   - `[complex]`：跨多檔案架構設計、複雜演算法、系統整合

範例 SQL：
```sql
INSERT INTO todos (id, title, description) VALUES
  ('explore-structure', '探索專案結構', '[simple] 了解目錄結構與主要檔案'),
  ('implement-feature', '實作 GIF 播放功能', '[medium] 在 src/ 下新增播放邏輯'),
  ('refactor-arch', '重構記憶體管理', '[complex] 跨多個模組重新設計 PSRAM 使用策略');
```

---

## 階段二：執行（Execute）

根據每個 TODO 的複雜度，使用 `task` 工具並指定對應模型：

| 複雜度     | 使用模型            | Premium 倍率 | 適用場景                           |
|------------|---------------------|--------------|------------------------------------|
| `[simple]` | `gpt-4.1`           | **0x (免費)**| 搜尋、讀檔、探索、grep、問答       |
| `[simple]` | `gpt-5-mini`        | **0x (免費)**| 同上，備選                         |
| `[medium]` | `claude-haiku-4.5`  | 0.33x        | 一般功能實作、小型重構、測試       |
| `[medium]` | `gpt-5.1-codex-mini`| 0.33x        | 同上，備選                         |
| `[complex]`| `claude-sonnet-4.6` | 1x           | 架構設計、複雜邏輯、多模組整合     |

**嚴格禁止**：
- 絕對不用 `claude-opus-4.*`（3x）或 `claude-opus-4.6-fast`（**30x！**）
- `[simple]`/`[medium]` 任務禁用任何 1x 以上模型

範例呼叫：
```
// simple → gpt-4.1 (0x，完全免費)
task(agent_type="explore", model="gpt-4.1", prompt="找出所有 .cpp 檔案中引用 PSRAM 的位置")

// medium → claude-haiku-4.5 (0.33x)
task(agent_type="task", model="claude-haiku-4.5", prompt="在 src/gif_player.cpp 實作 loadGifFromSD 函式")

// complex → claude-sonnet-4.6 (1x)
task(agent_type="general-purpose", model="claude-sonnet-4.6", prompt="重新設計記憶體管理架構，整合 PSRAM 與 DRAM 的分配策略")
```

執行完每個 TODO 後，立即更新其狀態：
```sql
UPDATE todos SET status = 'done' WHERE id = 'xxx';
```

---

## 階段三：Code Review

所有 TODO 完成後，**必須**呼叫 `code-review` agent 做最終審查：

```
task(agent_type="code-review", prompt="審查本次所有變更，只回報真正的 bug、安全漏洞或邏輯錯誤")
```

如果 code review 發現問題，根據問題複雜度選用適當模型修復，然後再做一次 review，直到通過為止。

---

## 快速參考

```
Simple tasks  → gpt-4.1 / gpt-5-mini      (0x，完全免費)
Medium tasks  → claude-haiku-4.5           (0.33x)
Complex tasks → claude-sonnet-4.6          (1x，謹慎使用)
Final review  → code-review agent          (高 signal-to-noise)
⛔ Never use  → claude-opus-4.*  (3x) / claude-opus-4.6-fast (30x!!)
```
