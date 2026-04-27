# 網路程式設計作業報告：多人管理聊天室系統

實作方案：Bottom-up，修改課程範例 `chatroom_serv1.c`、`chatroom_serv2.cpp` 的聊天室概念，並使用 AI 輔助完成擴充。

## 一、實作功能

| 功能 | 說明 |
|---|---|
| 多人聊天室 | Server 可同時接受多個 Client 連線，任一使用者送出訊息後，Server 會廣播給所有線上使用者。 |
| Server 廣播公告 | Server 端可以直接輸入公告，所有 Client 都會收到 `[Server公告]` 訊息。 |
| 歷史訊息保存 | 所有聊天內容與系統管理事件會寫入 `chat_history.txt`，方便事後查詢與紀錄保存。 |
| 截圖 / 文字快照 | 使用者可輸入 `/snapshot`，Server 會把目前聊天紀錄輸出成 `snapshot_時間.txt`，作為聊天室文字截圖快照。 |
| 管理員禁言 | 使用 `/admin 1234` 取得權限後，可使用 `/mute 名稱 秒數` 或 `/mute 名稱 forever` 禁言使用者。被禁言者不能發言，但仍能留在聊天室觀看訊息。 |
| 解除禁言 | 管理員可用 `/unmute 名稱` 解除指定使用者的禁言狀態。 |
| 踢人 | 管理員可用 `/kick 名稱` 將指定使用者踢出聊天室並關閉其 socket 連線。 |

## 二、應用案例

此系統可應用於簡易客服聊天室、課堂互動系統、區域網路討論室或多人即時訊息測試。老師或管理者可作為 Server 發布公告，使用者以 Client 身分加入聊天室。當有人違規發言時，管理員可暫時禁言、永久禁言或踢出使用者。

## 三、修改方式與架構

原課程範例主要展示 TCP socket 的一對一通訊。本專題將其擴充為多人聊天室，核心修改包含：

- 以 `Client` 結構紀錄 `fd`、`name`、`is_admin`、`mute_until`。
- 使用 `clients[]` 陣列保存所有線上使用者的 socket。
- 使用 `select()` 同時監聽 server socket、所有 client socket 與 server 鍵盤輸入。
- 新增指令解析邏輯，支援 `/history`、`/snapshot`、`/admin`、`/mute`、`/unmute`、`/kick`、`/users`。
- 新增 `save_history()` 將聊天訊息與管理事件寫入檔案。

## 四、程式檔案

| 檔案 | 用途 |
|---|---|
| `multi_server.c` | Server 主程式，負責 TCP 監聽、多人連線、廣播、歷史保存、截圖快照、管理員禁言與踢人。 |
| `multi_client.c` | Client 主程式，負責連線到 Server、傳送訊息、接收聊天室廣播。 |
| `Makefile` | 自動編譯 server 與 client。 |
| `README.md` | 使用說明與功能整理。 |
| `chat_history.txt` | 執行後產生，保存歷史聊天紀錄。 |
| `snapshot_YYYYMMDD_HHMMSS.txt` | 執行 `/snapshot` 後產生，作為聊天室文字截圖快照。 |

## 五、執行方式

```bash
make
./server 8888
./client 127.0.0.1 8888 David
./client 127.0.0.1 8888 Amy
./client 127.0.0.1 8888 Bob
```

## 六、管理指令

| 指令 | 功能 |
|---|---|
| `/help` | 查看指令 |
| `/history` | 查看最近歷史訊息 |
| `/snapshot` | 建立聊天文字截圖快照 |
| `/users` | 查看線上使用者 |
| `/admin 1234` | 取得管理員權限 |
| `/mute Bob 60` | 禁言 Bob 60 秒 |
| `/mute Bob forever` | 永久禁言 Bob，但 Bob 仍可接收訊息 |
| `/unmute Bob` | 解除 Bob 禁言 |
| `/kick Bob` | 踢出 Bob |
| `exit` | 離開聊天室 |

## 七、流程圖

```text
啟動 Server
  ↓
socket() 建立 TCP socket
  ↓
bind() 綁定 Port
  ↓
listen() 等待 Client
  ↓
select() 同時監聽 Server、Client、鍵盤輸入
  ↓
新 Client 連線 → accept() → 存入 clients[] → 廣播加入訊息
Client 傳訊息 → recv() → 判斷是否為指令
一般聊天 → 檢查禁言 → save_history() → broadcast()
管理指令 → 檢查管理員權限 → mute / unmute / kick / history / snapshot
Server 輸入 → 產生 [Server公告] → save_history() → broadcast()
```

## 八、與作業系統 Network Stack 的互動

本程式透過 socket API 與作業系統的 network stack 互動。程式中的 `socket()`、`bind()`、`listen()`、`accept()`、`connect()`、`send()`、`recv()`、`close()` 都是系統呼叫或系統提供的網路介面。應用程式將資料交給 Socket API，再由作業系統處理 TCP、IP 與網路介面。

```text
聊天室程式 → Socket API → OS Network Stack → TCP → IP → 網路介面 / localhost
```

## 九、範例執行結果說明

- Server 顯示「多人管理聊天室 Server 已啟動」。
- 多個 Client 可同時以不同名稱加入聊天室。
- David 發送訊息後，Amy 與 Bob 皆可收到。
- Server 輸入公告後，所有 Client 會收到 `[Server公告]`。
- 輸入 `/history` 可查看 `chat_history.txt` 中最近歷史訊息。
- 輸入 `/snapshot` 可產生 `snapshot_時間.txt`。
- 管理員輸入 `/mute Bob 60` 後，Bob 不能發言，但仍能接收聊天室訊息。
- 管理員輸入 `/kick Bob` 後，Bob 的 socket 連線會被 Server 關閉。

## 十、AI 輔助使用說明

本專題使用 ChatGPT 輔助分析課程範例、設計功能、產生程式碼、除錯與整理報告。使用 AI 的範圍包含多人聊天室架構、`select()` 監聽邏輯、歷史訊息保存、文字快照、管理員禁言與踢人功能，以及 README 與報告內容整理。

## 十一、結論

本專題成功將原本課程範例的一對一聊天室擴充為多人管理聊天室。系統具備多人同步聊天、Server 公告、歷史訊息保存、文字截圖快照、管理員禁言與踢人功能。透過 `select()` 監聽多個 socket，可以在單一 Server 程式中管理多個 Client，並展示 TCP socket 與作業系統 network stack 的互動方式。
