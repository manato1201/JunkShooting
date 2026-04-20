# JunkShooting — Script・アーキテクチャドキュメント

> 使用言語: C++ / Python  
> 制作期間: 1ヵ月（90時間）  
> 制作時期: 2025/07〜2025/08  
> 制作形態: 個人制作

---

## 目次

1. [Scriptコード抜粋](#1-scriptコード抜粋)
2. [システム全体像](#2-システム全体像)
3. [アーキテクチャ詳細](#3-アーキテクチャ詳細)
   - [ディレクトリ構成](#31-ディレクトリ構成)
   - [データ構造設計](#32-データ構造設計)
   - [外部ライブラリ依存](#33-外部ライブラリ依存)
   - [GoFデザインパターン対応表](#34-gofデザインパターン対応表)
   - [既知の問題・要改善点](#35-既知の問題要改善点)

---

## 1. Scriptコード抜粋

### ① 非同期音声認識 — `shootingGame.cpp`

PythonのGoogle Speech APIをC++から外部プロセスとして起動し、`std::thread::detach()`でメインループ（60FPS）をブロックせずに音声認識を実行。認識結果は`std::mutex`で保護した共有変数で受け渡す。

```cpp
void runPythonScriptAsync() {
    isRunning = true;
    std::thread([] {
        // Python を別プロセスで起動（ウィンドウ非表示）
        STARTUPINFOW si{};
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        std::wstring cmd = L"Pvoice.exe";
        if (CreateProcessW(NULL, &cmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE); // 完了まで待機
            CloseHandle(pi.hProcess);
        }
        // 結果を mutex 保護して書き出す
        std::ifstream file("voice_output.txt");
        std::string result;
        std::getline(file, result);
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            recognitionResult = result;
        }
        isRunning = false;
    }).detach(); // ← メインループから切り離す
}
```

**設計ポイント:**
- `std::thread::detach()` でメインループを60FPS維持したまま音声処理を非同期実行
- `SW_HIDE` でPythonのコンソールウィンドウをユーザーから隠す
- `std::lock_guard<std::mutex>` でスレッド間のデータ競合を防止
- レイテンシ 1〜3秒は「切替の緊張感」としてゲームデザインに組み込み

---

### ② FloodFill + PNG書き出し — `shootingGame.cpp`

BFSアルゴリズムで塗りつぶしを実装。背景との差分比較でアルファ値を自動生成し、80×64 PNGとして書き出してゲームのテクスチャに反映する。

```cpp
// FloodFill（BFSによる塗りつぶし）
void FloodFill(int x, int y, uint targetCol, uint fillCol) {
    std::queue<std::pair<int,int>> q;
    q.push({x, y});
    while (!q.empty()) {
        auto [cx, cy] = q.front(); q.pop();
        if (GetPixel(cx, cy) != targetCol) continue;
        DrawPixel(cx, cy, fillCol);
        q.push({cx+1,cy}); q.push({cx-1,cy});
        q.push({cx,cy+1}); q.push({cx,cy-1});
    }
}

// 80×64 PNG 書き出し（背景差分でアルファ値を自動生成）
void SaveCanvasAsPng(int canvas) {
    // キャンバスと背景をそれぞれ縮小取得し RGBA 化
    // 背景との差が小さいピクセル → α=0（透明）
    outBuf[i*4+3] = (abs(dr)+abs(dg)+abs(db) < 8) ? 0 : 255;
    stbi_write_png("PL.png", 80, 64, 4, outBuf, 80*4);
}
```

---

### ③ CSV駆動スキルツリー — `shootingGame.cpp`

スキル定義・コスト・習得状態をすべてCSVで管理。ゲームロジックとデータを疎結合に保ち、スキル追加がノーコードで可能。

```cpp
struct Skill {
    std::string name, desc;
    int costNormal, costSpecial;
    bool unlocked;
    int parent;
    std::vector<int> exclude; // 同時取得不可スキルの index
};

// CSV に即時書き出し（習得ボタン押下 → 自動セーブ）
void SaveSkillTreeStatus(const char* filename) {
    std::ofstream ofs(filename);
    for (const auto& s : skillTree)
        ofs << s.name << "," << (s.unlocked ? 1 : 0) << "," << s.parent << endl;
}

// ロード時にプレイヤーへ即時反映
void ApplySkillTreeStatus() {
    for (auto& s : skillTree) {
        if (!s.unlocked) continue;
        if (s.name == "体力増加")    player.shield += 2;
        if (s.name == "スピード増加") player.vx += 1;
        if (s.name == "攻撃力増加")  attackBuffMultiplier += 1.0f;
    }
}
```

---

## 2. システム全体像

### シーン遷移フロー

シーン関数ポインタによるステートマシン構成。各シーンは独立した関数として定義し、`changeScene()` で切り替える。`timer` も同時リセットされる。

```
WinMain
└── メインループ（60FPS）
    ├── gpUpdateKey()       ← 全キー状態を中央ポーリング
    ├── gCurrentScene()     ← 現在のシーン関数を呼び出す
    └── changeScene()       ← シーン遷移（ポインタ差し替え + timer リセット）

シーン遷移図:
Title → Select → Graphic（描画エディタ） → SkillTree → Explanation → Play
                                                                          ↓
                                                               Clear または Over
```

| シーン | 内容 | 遷移条件 |
|--------|------|---------|
| Title | タイトル画面 | SPACEキー |
| Select | 初めから / 続きから選択 | 選択後 |
| Graphic | 自機描画エディタ（FloodFill等） | 保存（PNG書き出し） |
| SkillTree | スキル選択（CSV読み込み） | 決定 |
| Explanation | 操作説明・音声認識起動 | SPACEキー |
| Play | ゲーム本体（60FPS） | 敵全滅 or 自機撃破 |
| Clear / Over | クリア / ゲームオーバー | 自動 |

### 音声認識 + ゲームループの統合

```
メインループ（60FPS）
├── Q/Pキー押下
│   └── runPythonScriptAsync()
│       └── std::thread::detach()
│           ├── CreateProcessW → Pvoice.exe（Google API）
│           ├── WaitForSingleObject（Python完了待機）
│           └── voice_output.txt → std::mutex → recognitionResult
└── 毎フレーム
    └── isRunning==false かつ recognitionResult != ""
        └── 弾モード切替（BulletMode enum）
```

### 設計方針・学んだこと

| トピック | 内容 |
|---------|------|
| `#pragma region` 活用 | 機能ごとにファイルを分割しすぎた結果、継承関係が追えなくなったため再統合。KISS原則を実体験として理解 |
| 他作品への展開 | HSV変換のノウハウ → AdvancedVAT シェーダー開発へ。音声認識の課題（レイテンシ）→ WinMM コールバック方式（AdvancedVAT）で解決 |

---

## 3. アーキテクチャ詳細

### 3.1 ディレクトリ構成

```
JunkShooting/
├── SourceCode/
│   ├── shootingGame.cpp       # メイン実装（約2,300行）
│   │                          # シーン / エンティティ / 入力 / 描画 / CSV / 音声を包含
│   ├── shootingGame.h         # 構造体・関数宣言
│   ├── Pvoice.py              # Python音声認識スクリプト（Google Speech API）
│   │                          # C++ から CreateProcessW で呼び出し、結果を txt に書き出す
│   └── stb_image_write.h      # PNG書き出しライブラリ（外部・public domain）
└── build/                     # 実行バイナリと資産
    ├── ShootingGame.exe        # メイン実行ファイル
    ├── Pvoice.exe              # 音声認識実行ファイル（Pvoice.py から生成）
    ├── image/                  # グラフィックアセット
    ├── sound/                  # 音声アセット
    ├── skilltree.csv           # スキル定義・習得状態（永続化）
    └── points.csv              # ポイント永続化
```

---

### 3.2 データ構造設計

| 構造体 | 主なフィールド | 役割 |
|--------|--------------|------|
| `OBJECT` | `x,y,vx,vy,state,pattern,image,attack,shield,timer` | 全エンティティの基底構造体。`state==0` で非アクティブ（Object Pool管理） |
| `Player : OBJECT` | `name,attack,resilience,shield` | プレイヤー固有パラメータ。スキル習得で動的に変化 |
| `Enemy : OBJECT` | `boss3_phase,boss3_targetX/Y,boss3_shot_timer` | ボス3の有限状態マシン状態変数を内包 |
| `Bullet : OBJECT` | `mode,feature` | 弾の挙動モード（`NORMAL/CHASE/EXPLOSION/BLACKHALL/RAPIDFIRE`） |
| `Skill` | `name,desc,costNormal,costSpecial,unlocked,parent,exclude[]` | スキルツリーのノード。親子依存・排他制約（exclude リスト）を保持 |

---

### 3.3 外部ライブラリ依存

| ライブラリ | 用途 | 主な使用箇所 |
|-----------|------|------------|
| **DxLib（C++）** | グラフィックス・入力・サウンド・ウィンドウ管理 | `WinMain`、各シーン関数 全体 |
| **stb_image_write.h v1.16** | RGBAバッファ → PNG ファイル書き出し | `SaveCanvasAsPng()` |
| **speech_recognition（Python）** | Google Cloud Speech-to-Text API（ja-JP）6秒タイムアウト | `Pvoice.py` |
| **C++ STL** | `thread / mutex / queue / fstream / vector` | `runPythonScriptAsync` / `FloodFill` / CSV管理 |
| **Windows API** | `CreateProcessW`（プロセス生成）/ `STARTUPINFOW` | `runPythonScriptAsync()` |

---

### 3.4 GoFデザインパターン対応表

| # | パターン | 該当 | 実装箇所 | 説明 |
|---|---------|:----:|---------|------|
| 1 | **State** | ✅ | `gCurrentScene (SceneFunc*)` | 各シーン関数をステートとして扱い `changeScene()` で切り替え。`timer` も同時リセット |
| 2 | **Object Pool** | ✅ | `bullet[] / enemy[] / effect[]` | 固定配列＋`state==0` フラグで再利用。effect は循環インデックス（`eff_num % EFFECT_MAX`） |
| 3 | **Strategy** | ✅ | `moveBullet() / BulletMode enum` | 5種の弾挙動（`NORMAL/CHASE/EXPLOSION/BLACKHALL/RAPIDFIRE`）を if-else で切り替え |
| 4 | **Finite State Machine** | ✅ | `boss3_phase / Boss2 state変数` | ボス毎にフェーズ変数を保持し条件達成で状態遷移。Boss3は3フェーズサイクル |
| 5 | **Template Method** | △ | `WinMain メインループ` | 画面クリア→背景→エンティティ更新→HUD→シーン実行 の固定手順内にシーン処理を差し込む |
| 6 | **Observer（簡略）** | △ | `gpUpdateKey() / Key[]配列` | 全キー状態を中央集権的にポーリング。フレーム数で継続押下時間を管理 |
| 7 | **Data-Driven Design** | ✅ | `Skill構造体 + skilltree.csv` | ゲームパラメータをコードから分離しCSVで外部管理。スキル追加がノーコードで可能 |

> **凡例:** ✅ = GoFの定義に忠実な実装 / △ = 役割・意図が対応するが厳密なGoF実装ではない

---

### 3.5 既知の問題・要改善点

| 優先度 | 問題 | 場所 | 対応方針 |
|--------|------|------|---------|
| 高 | 2,300行の単一ファイル構成（低凝集・低分割） | `shootingGame.cpp` 全体 | `Scene / Entity / InputManager` 等クラス分割 |
| 高 | グローバル変数多用による状態の分散 | ファイルスコープ変数群 | カプセル化・クラス内 `static` 化 |
| 中 | 音声認識レイテンシ 1〜3秒 | `runPythonScriptAsync()` | WinMM コールバック方式へ移行（→ AdvancedVATで実現済み） |
| 中 | ハードコードパス `"voice_output.txt"` | `runPythonScriptAsync()` | 実行ディレクトリ相対 or 設定ファイル化 |
| 中 | 静的ローカル変数による状態保持 | `moveEnemy()` boss2/3 state変数 | 状態を `Enemy` 構造体フィールドへ移行 |
| 低 | `std::thread::detach()` で join なし | `runPythonScriptAsync()` | シャットダウン時に確実な終了処理を追加 |
| 低 | CSV パースにバリデーションなし | `LoadSkillTreeStatus()` | 不正データ時のフォールバック処理を実装 |

---

*このドキュメントは `JunkShooting_architecture.md` として管理しています。コード変更時は対応するセクションを更新してください。*
