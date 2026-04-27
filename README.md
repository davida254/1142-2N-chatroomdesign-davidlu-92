# 多人管理聊天室系統

本專題以課程範例 `chatroom_serv1.c`、`chatroom_serv2.cpp` 的聊天室概念為基礎，擴充成具備多人同步、歷史訊息保存、文字截圖快照、管理員禁言與踢人功能的 TCP 聊天室。

## 功能

- 多人 TCP 聊天室
- Server 可廣播公告
- 歷史訊息保存到 `chat_history.txt`
- `/history` 查看最近歷史訊息
- `/snapshot` 建立聊天文字截圖快照 `snapshot_YYYYMMDD_HHMMSS.txt`
- `/admin 1234` 取得管理員權限
- `/mute 名稱 秒數` 暫時禁言
- `/mute 名稱 forever` 永久禁言，但使用者仍可留在聊天室觀看訊息
- `/unmute 名稱` 解除禁言
- `/kick 名稱` 踢出使用者
- `/users` 查看線上使用者

## 編譯

```bash
make
```

或：

```bash
gcc multi_server.c -o server
gcc multi_client.c -o client
```

## 執行

啟動 Server：

```bash
./server 8888
```

啟動 Client：

```bash
./client 127.0.0.1 8888 David
./client 127.0.0.1 8888 Amy
./client 127.0.0.1 8888 Bob
```

## 指令範例

一般使用者：

```text
/history
/snapshot
/users
exit
```

管理員：

```text
/admin 1234
/mute Bob 60
/mute Bob forever
/unmute Bob
/kick Bob
```

## 注意

`127.0.0.1` 是 localhost，代表在同一台電腦模擬多個 client。若要不同電腦連線，client 端要改成 Server 電腦在區域網路中的 IP。
# 1142-2N-chatroomdesign-davidlu-92
