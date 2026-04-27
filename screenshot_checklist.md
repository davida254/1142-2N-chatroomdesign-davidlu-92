# 截圖清單

請在 VS Code / WSL 執行時截圖，放進報告或 screenshots 資料夾。

1. `make` 編譯成功畫面。
2. Server 執行 `./server 8888` 畫面。
3. 三個 Client 分別執行：
   - `./client 127.0.0.1 8888 David`
   - `./client 127.0.0.1 8888 Amy`
   - `./client 127.0.0.1 8888 Bob`
4. 多人聊天同步畫面。
5. Server 輸入公告後，Client 收到 `[Server公告]` 的畫面。
6. Client 輸入 `/history` 顯示歷史訊息畫面。
7. Client 輸入 `/snapshot` 產生 `snapshot_時間.txt` 的畫面。
8. 管理員輸入 `/admin 1234` 取得權限畫面。
9. 管理員輸入 `/mute Bob 60`，Bob 被禁言但仍能收到訊息畫面。
10. 管理員輸入 `/mute Bob forever` 永久禁言畫面。
11. 管理員輸入 `/unmute Bob` 解除禁言畫面。
12. 管理員輸入 `/kick Bob` 踢人畫面。
