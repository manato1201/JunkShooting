#define _CRT_SECURE_NO_WARNINGS
#include "DxLib.h"
#include "shootingGame.h" // ヘッダーファイルをインクルード
#include <stdlib.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>
#include <sstream>
#include <cstdlib>  // system関数を使用するために必要
#include <string>
#include <vector>
#include <windows.h>
#include <cmath>
#include <queue>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#pragma region 定数定義

// 定数の定義
const int WIDTH = 1200, HEIGHT = 720; // ウィンドウの幅と高さのピクセル数
const int FPS = 60; // フレームレート
const int IMG_ENEMY_MAX = 9; // 敵の画像の枚数（種類）
const int BULLET_MAX = 100; // 自機が発射する弾の最大数
const int ENEMY_MAX = 100; // 敵機の数の最大値
const int STAGE_DISTANCE = FPS * 60; // ステージの長さ
const int PLAYER_SHIELD_MAX = 8; // 自機のシールドの最大値
const int EFFECT_MAX = 100; // エフェクトの最大数
const int ITEM_TYPE = 3; // アイテムの種類
const int HART_TYPE = 3;//ハート
const int BOSS_TYPE = 3;//ボス
const int WEAPON_LV_MAX = 10; // 武器レベルの最大値
const int PLAYER_SPEED_MAX = 20; // 自機の速さの最大値



#pragma endregion

#pragma region enum定義

enum
{
    ENE_BULLET,
    ENE_ZAKO1,
    ENE_ZAKO2,
    ENE_ZAKO3,
    ENE_WALL,
    ENE_BOSS1,
    ENE_BOSS2,
    ENE_BOSS3,
    ENE_HART
}; // 敵機の種類

enum
{
    EFF_EXPLODE,
    EFF_RECOVER,
    EFF_SHOT_EXPLODE,
    EFF_MODE_EXPLOSION,
    EFF_MODE_BLACKHALL,
    EFF_MODE_CHASE,
    EFF_MODE_RAPIDFIRE
}; // エフェクトの種類


enum
{
    NORMAL,
    EXPLOSION,
    BLACKHALL,
    CHASE,
    RAPIDFIRE
}; // 弾を分けるための列挙定数

#pragma endregion

#pragma region グローバル定数定義

// グローバル変数
// ここでゲームに用いる変数や配列を定義する
int imgGalaxy, imgFloor, imgWallL, imgWallR; // 背景画像
int imgSelect, imgEasy, imgNormal, imgHard, imgGO, imgGC1, imgGC2, imgGC3, img35;//背景画像２
int imgFighter, imgBullet; // 自機と自機の弾の画像
int imgEnemy[IMG_ENEMY_MAX]; // 敵機の画像

int imgEnemyBoss;//ボス

int imgExplosion; // 爆発演出の画像
int imgExplosion2;
int imgItem; // アイテムの画像
int imgHart;//クリアポイント
int bgm, jinOver, jinClear, seExpl, seItem, seShot; // 音の読み込み用
int se35_1, se35_2, se35_3, se35_4, se35_5, se35_6, se35_7;
bool se35F, se35FF = false;
int distance = 0; // ステージ終端までの距離
int bossIdx = 0; // ボスを代入した配列のインデックス
int stage = 1; // ステージ
int score = 0; // スコア
int hisco = 10000; // ハイスコア
int noDamage = 0; // 無敵状態
int weaponLv = 1; // 自機の武器のレベル（同時に発射される弾数）
//int scene = TITLE; // シーンを管理
int mode = NORMAL;//弾の種類を管理

int timer = 0; // 時間の進行を管理

struct OBJECT player; // 自機用の構造体変数
struct OBJECT bullet[BULLET_MAX]; // 弾用の構造体の配列
struct Enemy enemy[ENEMY_MAX]; // 敵機用の構造体の配列
struct OBJECT effect[EFFECT_MAX]; // エフェクト用の構造体の配列
struct OBJECT item; // アイテム用の構造体変数

std::atomic<bool> isRunning(false);  // スクリプト実行中かどうか
std::mutex resultMutex;             // 結果へのアクセスを保護するためのミューテックス
std::string recognitionResult = ""; // 認識結果

float normalPoint = 0;
int specialPoint = 0;

#pragma endregion

// ---- ここから関数ポインタ型で状態遷移を管理 ----
using SceneFunc = void(*)();
void sceneTitle();
void sceneSelect();
void sceneGraphic();
void sceneSkillTree();
void sceneExplanation();
void scenePlay();
void sceneOver();
void sceneClear();

SceneFunc gCurrentScene = nullptr;
void changeScene(SceneFunc next) {
    gCurrentScene = next;
    timer = 0;
}

bool isCurrentScene(SceneFunc f) {
    return gCurrentScene == f;
}

#pragma region どのくらい押されているかを検知するための関数
//どのくらい押されているかを検知するための関数

int Key[256]; // キーが押されているフレーム数を格納する

// キーの入力状態を更新する
int gpUpdateKey() {
    char tmpKey[256]; // 現在のキーの入力状態を格納する
    GetHitKeyStateAll(tmpKey); // 全てのキーの入力状態を得る
    for (int i = 0; i < 256; i++) {
        if (tmpKey[i] != 0) { // i番のキーコードに対応するキーが押されていたら
            Key[i]++;     // 加算
        }
        else {              // 押されていなければ
            Key[i] = 0;   // 0にする
        }
    }
    return 0;

}
#pragma endregion






int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    SetWindowText("JunkShooting"); // ウィンドウのタイトル
    SetGraphMode(WIDTH, HEIGHT, 32); // ウィンドウの大きさとカラービット数の指定
    ChangeWindowMode(TRUE); // ウィンドウモードで起動
    if (DxLib_Init() == -1) return -1; // ライブラリ初期化 エラーが起きたら終了
    SetBackgroundColor(0, 0, 0); // 背景色の指定
    SetDrawScreen(DX_SCREEN_BACK); // 描画面を裏画面にする

    initGame(); // 初期化用の関数を呼び出す
    initVariable(); // 【仮】ゲームを完成させる際に呼び出し位置を変える
    distance = STAGE_DISTANCE; // 【記述位置は仮】ステージの長さを代入

    
    changeScene(sceneTitle); // 初期状態をタイトルにセット

#pragma region メインループ

    while (1) // メインループ
    {
        ClearDrawScreen(); // 画面をクリアする

        // ゲームの骨組みとなる処理を、ここに記述する
        int spd = 3; // スクロールの速さ
        if (isCurrentScene(scenePlay) && distance == 0) spd = 0; // ボス戦はスクロール停止
        scrollBG(spd); // 背景のスクロール
        moveEnemy(); // 敵機の制御
        moveBullet(); // 弾の制御
        moveItem(); // アイテムの制御
        drawEffect(); // エフェクト
        

        timer++; // タイマーをカウント
        if (gCurrentScene) gCurrentScene();

        if (gCurrentScene == scenePlay) {
            drawText(10, 30, "モード", score, 0xffffff, 30);
            drawText(WIDTH - 260, HEIGHT - 40, "難易度:Lv%02d", stage, 0xffffff, 30);
        }

        gpUpdateKey();
        ScreenFlip();
        WaitTimer(1000 / FPS);
        if (ProcessMessage() == -1) break;
        if (CheckHitKey(KEY_INPUT_ESCAPE) == 1) break;
    }
    DxLib_End();
    return 0;
        
}







// ここから下に自作した関数を記述する

// ==== 各シーン関数 ====

#pragma region Scene:TITLE

void sceneTitle() {
    drawTextC(WIDTH * 0.5, HEIGHT * 0.3, "JunkShooting", 0xffffff, 80);
    drawTextC(WIDTH * 0.5, HEIGHT * 0.7, "Press SPACE to Game start.", 0xffffff, 30);
    
    if (Key[KEY_INPUT_SPACE] == 1) {
        initVariable();
        changeScene(sceneSelect);
    }
}

#pragma endregion


#pragma region Scene:SELECT
// セレクト画面（初めから／続きから）
void sceneSelect() {
    DrawGraph(0, 0, imgSelect, FALSE);

    // ボタンの位置とサイズ
    const int btnW = 280, btnH = 80;
    const int btnY = HEIGHT / 2;
    const int leftBtnX = WIDTH / 4 - btnW / 2;
    const int rightBtnX = WIDTH * 3 / 4 - btnW / 2;

    // ボタン描画
    DrawBox(leftBtnX, btnY, leftBtnX + btnW, btnY + btnH, GetColor(40, 80, 180), TRUE);
    drawTextC(leftBtnX + btnW / 2, btnY + btnH / 2, "初めから", 0xffffff, 42);

    DrawBox(rightBtnX, btnY, rightBtnX + btnW, btnY + btnH, GetColor(60, 140, 80), TRUE);
    drawTextC(rightBtnX + btnW / 2, btnY + btnH / 2, "続きから", 0xffffff, 42);

    // マウスクリック判定
    int mx, my, btn;
    GetMousePoint(&mx, &my);
    btn = GetMouseInput();

    static bool prevLMB = false;
    bool nowLMB = btn & MOUSE_INPUT_LEFT;

    if (!prevLMB && nowLMB) {
        // 左（初めから）
        if (mx >= leftBtnX && mx < leftBtnX + btnW && my >= btnY && my < btnY + btnH) {
            changeScene(sceneGraphic);
        }
        // 右（続きから）
        if (mx >= rightBtnX && mx < rightBtnX + btnW && my >= btnY && my < btnY + btnH) {
            changeScene(sceneSkillTree);
        }
    }
    prevLMB = nowLMB;
}
#pragma endregion



#pragma region Scene:GRAPHIC

const int BG_W = 800, BG_H = 720, UI_W = 400, UI_H = 720;
static int bgPattern = -1;

#pragma region 保存処理

void SaveCanvasAsPng(int canvas) {
    const int SAVE_W = 80, SAVE_H = 64;
    unsigned int* buf = new unsigned int[SAVE_W * SAVE_H];
    unsigned int* bgBuf = new unsigned int[SAVE_W * SAVE_H];

    // 1. まず背景のみで縮小
    int bgHandle = MakeScreen(SAVE_W, SAVE_H, TRUE);
    SetDrawScreen(bgHandle);
    DrawExtendGraph(0, 0, SAVE_W, SAVE_H, bgPattern, FALSE);
    for (int y = 0; y < SAVE_H; ++y)
        for (int x = 0; x < SAVE_W; ++x)
            bgBuf[y * SAVE_W + x] = GetPixel(x, y);
    DeleteGraph(bgHandle);

    // 2. キャンバス（描画後）で縮小
    int tmpHandle = MakeScreen(SAVE_W, SAVE_H, TRUE);
    SetDrawScreen(tmpHandle);
    DrawExtendGraph(0, 0, SAVE_W, SAVE_H, canvas, FALSE);
    for (int y = 0; y < SAVE_H; ++y)
        for (int x = 0; x < SAVE_W; ++x)
            buf[y * SAVE_W + x] = GetPixel(x, y);

    // 3. RGBA化
    unsigned char outBuf[SAVE_W * SAVE_H * 4];
    for (int i = 0; i < SAVE_W * SAVE_H; ++i) {
        unsigned int col = buf[i], bgcol = bgBuf[i];
        outBuf[i * 4 + 0] = (col >> 16) & 0xFF;
        outBuf[i * 4 + 1] = (col >> 8) & 0xFF;
        outBuf[i * 4 + 2] = col & 0xFF;
        // 色の近さで判定（8くらい許容）
        int dr = (col >> 16 & 0xFF) - (bgcol >> 16 & 0xFF);
        int dg = (col >> 8 & 0xFF) - (bgcol >> 8 & 0xFF);
        int db = (col & 0xFF) - (bgcol & 0xFF);
        if (abs(dr) + abs(dg) + abs(db) < 8)
            outBuf[i * 4 + 3] = 0;
        else
            outBuf[i * 4 + 3] = 255;
    }
    stbi_write_png("PL.png", SAVE_W, SAVE_H, 4, outBuf, SAVE_W * 4);
    //MessageBoxA(NULL, "保存しました(output.png)", "Info", MB_OK);
    delete[] buf;
    delete[] bgBuf;
    DeleteGraph(tmpHandle);
}
#pragma endregion

#pragma region 色相

void HSVtoRGB(float h, float s, float v, int& r, int& g, int& b) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
    float m = v - c;
    float r1, g1, b1;
    if (h < 60) { r1 = c; g1 = x; b1 = 0; }
    else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
    else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
    else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
    else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
    else { r1 = c; g1 = 0; b1 = x; }
    r = (int)((r1 + m) * 255);
    g = (int)((g1 + m) * 255);
    b = (int)((b1 + m) * 255);
}

#pragma endregion

#pragma region 塗りつぶし

void FloodFill(int x, int y, unsigned int targetCol, unsigned int fillCol) {
    if (targetCol == fillCol) return;
    std::queue<std::pair<int, int>> q;
    std::vector<std::vector<bool>> visited(BG_W, std::vector<bool>(BG_H, false));
    q.push({ x, y });
    while (!q.empty()) {
        auto [cx, cy] = q.front(); q.pop();
        if (cx < 0 || cx >= BG_W || cy < 0 || cy >= BG_H) continue;
        if (visited[cx][cy]) continue;
        unsigned int nowCol = GetPixel(cx, cy);
        if (nowCol != targetCol) continue;
        DrawPixel(cx, cy, fillCol);
        visited[cx][cy] = true;
        q.push({ cx + 1, cy });
        q.push({ cx - 1, cy });
        q.push({ cx, cy + 1 });
        q.push({ cx, cy - 1 });
    }
}

#pragma endregion

#pragma region ペンの形状


void DrawPenShape(int x, int y, int penSize, int penShape, int color) {
    switch (penShape) {
    case 0: DrawCircle(x, y, penSize, color, TRUE); break; // CIRCLE
    case 1: DrawBox(x - penSize, y - penSize, x + penSize, y + penSize, color, TRUE); break; // SQUARE
    case 2: {
        int x0 = x, y0 = y - penSize;
        int x1 = x - penSize, y1 = y + penSize;
        int x2 = x + penSize, y2 = y + penSize;
        DrawTriangle(x0, y0, x1, y1, x2, y2, color, TRUE);
    } break;


    case 3: // DIAMOND（ひし形）
    {
        for (int dy = -penSize; dy <= penSize; ++dy) {
            for (int dx = -penSize; dx <= penSize; ++dx) {
                if (abs(dx) + abs(dy) <= penSize) {
                    DrawPixel(x + dx, y + dy, color);
                }
            }
        }
    }
    break;
    }
}

#pragma endregion


void sceneGraphic() {
    // --- static変数 ---
    static bool initialized = false;
    static int canvas = -1;
    static int penSize = 10, penShape = 0, toolType = 0;
    static float hue = 0, sat = 1, val = 1;
    static int penR = 255, penG = 0, penB = 0;
    static int oldX = -1, oldY = -1;
    static bool justFilled = false, justGrad = false;
    static int checker = -1;
    static bool showSaved = false;
    static int saveTimer = 0;

    // ---- サイズ調整（中央寄せ） ----
    
    
    const int DRAW_X = 0, DRAW_Y = 0;           // お絵描きエリア左上
    const int UI_X = BG_W + 16, UI_Y = 12;      // UIエリア左上

    // HSV領域
    const int hsvBarY = UI_Y + 94, hsvBarW = 256, hsvBarH = 32;
    const int hsvSVY = hsvBarY + hsvBarH + 21, hsvSVW = 256, hsvSVH = 144;

    // --- 初期化 ---
    if (!initialized) {
        bgPattern = LoadGraph("bg_pattern.png");
        checker = MakeScreen(32, 32, TRUE);
        SetDrawScreen(checker);
        for (int y = 0; y < 32; ++y) for (int x = 0; x < 32; ++x)
            DrawPixel(x, y, ((x / 8 + y / 8) % 2) ? GetColor(220, 220, 220) : GetColor(160, 160, 160));
        SetDrawScreen(DX_SCREEN_BACK);
        canvas = MakeScreen(BG_W, BG_H, TRUE);
        SetDrawScreen(canvas);
        for (int y = 0; y < BG_H; y += 32) for (int x = 0; x < BG_W; x += 32)
            DrawGraph(x, y, checker, FALSE);
        SetDrawScreen(DX_SCREEN_BACK);
        HSVtoRGB(hue, sat, val, penR, penG, penB);
        initialized = true;
        showSaved = false;
    }

    // --- 背景とUI ---
    DrawBox(0, 0, WIDTH, HEIGHT, GetColor(0, 0, 0), TRUE);  // 黒背景
    DrawGraph(DRAW_X, DRAW_Y, bgPattern, TRUE);             // 背景パターン
    DrawGraph(DRAW_X, DRAW_Y, canvas, FALSE);               // キャンバス

    // --- タイトル・サイズ ---
    DrawBox(UI_X, UI_Y, UI_X + UI_W, UI_Y + UI_H, GetColor(24, 24, 36), TRUE);
    DrawFormatString(UI_X, UI_Y, GetColor(255, 255, 255), "自分の機体を描こう");
    DrawFormatString(UI_X, UI_Y + 48, GetColor(255, 255, 255), "SIZE:%d", penSize);



    // --- 色相バー（0～360°） ---
    for (int i = 0; i < hsvBarW; ++i) {
        float h = 360.0f * i / (hsvBarW - 1);
        int r, g, b; HSVtoRGB(h, 1, 1, r, g, b);
        DrawBox(UI_X + i, hsvBarY, UI_X + i + 1, hsvBarY + hsvBarH, GetColor(r, g, b), TRUE);
    }
    // 色相選択
    int mx, my, btn; GetMousePoint(&mx, &my); btn = GetMouseInput();
    if ((btn & MOUSE_INPUT_LEFT) && mx >= UI_X && mx < UI_X + hsvBarW && my >= hsvBarY && my < hsvBarY + hsvBarH) {
        hue = 360.0f * (mx - UI_X) / (hsvBarW - 1);
        HSVtoRGB(hue, sat, val, penR, penG, penB);
    }
    // --- SVエリア ---
    for (int y = 0; y < hsvSVH; ++y) for (int x = 0; x < hsvSVW; ++x) {
        float s = x / (float)(hsvSVW - 1);
        float v = 1 - y / (float)(hsvSVH - 1);
        int r, g, b; HSVtoRGB(hue, s, v, r, g, b);
        DrawPixel(UI_X + x, hsvSVY + y, GetColor(r, g, b));
    }
    if ((btn & MOUSE_INPUT_LEFT) && mx >= UI_X && mx < UI_X + hsvSVW && my >= hsvSVY && my < hsvSVY + hsvSVH) {
        sat = (mx - UI_X) / (float)(hsvSVW - 1);
        val = 1 - (my - hsvSVY) / (float)(hsvSVH - 1);
        HSVtoRGB(hue, sat, val, penR, penG, penB);
    }
    // カラープレビュー
    DrawBox(UI_X, hsvSVY + hsvSVH + 12, UI_X + 64, hsvSVY + hsvSVH + 44, GetColor(penR, penG, penB), TRUE);



    // --- ペンサイズスライダー ---
    int sliderY = hsvSVY + hsvSVH + 55;
    DrawBox(UI_X, sliderY, UI_X + 128, sliderY + 10, GetColor(60, 60, 60), TRUE);
    int penSliderX = UI_X + 6 + (int)(((penSize - 2) / 28.0f) * 116.0f);
    DrawBox(penSliderX, sliderY - 4, penSliderX + 8, sliderY + 14, GetColor(255, 255, 255), TRUE);
    if ((btn & MOUSE_INPUT_LEFT) && mx >= UI_X && mx < UI_X + 128 && my >= sliderY && my < sliderY + 10)
        penSize = 2 + (int)(((mx - UI_X) / 116.0f) * 28.0f);

    // --- ペン形状 ---
    const char* shapeLabels[] = { "●", "■", "▲", "◆" };
    int shapeY = sliderY + 40;
    for (int i = 0; i < 4; ++i) {
        int bx = UI_X + i * 70, by = shapeY, bw = 48, bh = 48;
        DrawBox(bx, by, bx + bw, by + bh, penShape == i ? GetColor(100, 200, 255) : GetColor(80, 80, 80), TRUE);
        DrawFormatString(bx , by + 5, GetColor(255, 255, 255), shapeLabels[i]);
        if ((btn & MOUSE_INPUT_LEFT) && mx >= bx && mx < bx + bw && my >= by && my < by + bh) penShape = i;
    }

    // --- ツールボタン ---
    const char* toolLabels[] = { "PEN", "ERASER", "FILL", "GRAD_FINE", "GRAD_ROUGH" };
    int toolY = shapeY + 50;
    for (int i = 0; i < 5; ++i) {
        int bx = UI_X, by = toolY + i * 40, bw = 256, bh = 48;
        DrawBox(bx, by, bx + bw, by + bh, toolType == i ? GetColor(80, 140, 220) : GetColor(70, 70, 70), TRUE);
        DrawFormatString(bx + 12, by + 6, GetColor(255, 255, 255), toolLabels[i]);
        if ((btn & MOUSE_INPUT_LEFT) && mx >= bx && mx < bx + bw && my >= by && my < by + bh) toolType = i;
    }

    // --- 保存・次へ ---
    int saveY = UI_Y + UI_H - 68;
    DrawBox(UI_X, saveY, UI_X + 82, saveY + 52, GetColor(60, 70, 170), TRUE);
    DrawFormatString(UI_X , saveY + 8, GetColor(255, 255, 255), "保存");
    DrawBox(UI_X + 154, saveY, UI_X + 232, saveY + 52, GetColor(170, 70, 70), TRUE);
    DrawFormatString(UI_X + 150, saveY + 8, GetColor(255, 255, 255), "次へ");

    
    
    // --- 保存処理（TestGraphic流） ---
    if ((btn & MOUSE_INPUT_LEFT) && mx >= UI_X && mx < UI_X + 82 && my >= saveY && my < saveY + 52) {
        SaveCanvasAsPng(canvas);
        showSaved = true; saveTimer = 60;
    }
    if (showSaved && saveTimer-- > 0)
        DrawFormatString(UI_X+30, saveY -320, GetColor(255, 255, 100), "保存しました！");
    if (saveTimer == 0) showSaved = false;

    // --- 次の処理 ---
    if ((btn & MOUSE_INPUT_LEFT) && mx >= UI_X + 154 && mx < UI_X + 232 && my >= saveY && my < saveY + 52) {
        initialized = false;
        changeScene(sceneSkillTree);
    }

    // --- お絵描きエリアへの座標変換 ---
    int canvasX = mx - DRAW_X, canvasY = my - DRAW_Y;
    bool inCanvas = (0 <= canvasX && canvasX < BG_W && 0 <= canvasY && canvasY < BG_H);


    if (inCanvas && (btn & MOUSE_INPUT_LEFT)) {
        SetDrawScreen(canvas);
        if (toolType == 0) {
            if (oldX != -1 && oldY != -1) {
                for (float t = 0; t < 1.0; t += 0.04f) {
                    int px = (int)(oldX + (canvasX - oldX) * t), py = (int)(oldY + (canvasY - oldY) * t);
                    DrawPenShape(px, py, penSize, penShape, GetColor(penR, penG, penB));
                }
            }
            else {
                DrawPenShape(canvasX, canvasY, penSize, penShape, GetColor(penR, penG, penB));
            }
        }
        if (toolType == 1) {
            for (int dy = -penSize; dy <= penSize; ++dy)
                for (int dx = -penSize; dx <= penSize; ++dx)
                    if (dx * dx + dy * dy <= penSize * penSize)
                        DrawPixel(canvasX + dx, canvasY + dy, ((canvasX + dx) / 8 + (canvasY + dy) / 8) % 2 ? GetColor(220, 220, 220) : GetColor(160, 160, 160));
        }
        if (toolType == 2 && !justFilled) {
            unsigned int tgt = GetPixel(canvasX, canvasY);
            unsigned int newCol = GetColor(penR, penG, penB);
            if (tgt != newCol) FloodFill(canvasX, canvasY, tgt, newCol);
            justFilled = true;
        }
        if (toolType == 3 && !justGrad) {
            for (int x = 0; x < BG_W; ++x) for (int y = 0; y < BG_H; ++y) {
                float t = x / (float)(BG_W - 1);
                int r, g, b; HSVtoRGB(hue + t * 120, sat, val, r, g, b);
                DrawPixel(x, y, GetColor(r, g, b));
            }
            justGrad = true;
        }
        if (toolType == 4 && !justGrad) {
            int block = 32;
            for (int x = 0; x < BG_W; x += block) for (int y = 0; y < BG_H; y += block) {
                float t = x / (float)(BG_W - 1);
                int r, g, b; HSVtoRGB(hue + t * 120, sat, val, r, g, b);
                DrawBox(x, y, x + block, y + block, GetColor(r, g, b), TRUE);
            }
            justGrad = true;
        }
        SetDrawScreen(DX_SCREEN_BACK);
        oldX = canvasX; oldY = canvasY;
    }
    else {
        oldX = oldY = -1;
        justFilled = justGrad = false;
    }
}
#pragma endregion


#pragma region Scene:SKILLTREE




void SavePointsToCSV() {
    std::ofstream ofs("points.csv");
    ofs << normalPoint << "," << specialPoint << std::endl;
}

void LoadPointsFromCSV() {
    std::ifstream ifs("points.csv");
    if (!ifs) return;
    std::string line, token;
    if (getline(ifs, line)) {
        std::istringstream iss(line);
        getline(iss, token, ','); normalPoint = atof(token.c_str());
        getline(iss, token, ','); specialPoint = atoi(token.c_str());
    }
}



std::vector<std::string> skillNames;
std::vector<bool> unlockedMode = { false, false, false, false }; // CHASE,RAPIDFIRE,EXPLOSION ,BLACKHall 
// 攻撃バフ管理
int attackBuffTimer = 0;
float attackBuffMultiplier = 1.0f;


void LoadSkillNamesAndUnlocks() {
    skillNames.clear();
    unlockedMode = { false, false, false, false };
    std::ifstream fin("skills.csv");
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;
        // CSV: 技名,EXPLOSION,BLACKHALL,CHASE,RAPIDFIRE
        // 例: チャージ,0,1,0,0
        std::stringstream ss(line);
        std::string word;
        std::getline(ss, word, ',');
        skillNames.push_back(word);

        for (int i = 0; i < 4; i++) {
            std::getline(ss, word, ',');
            if (word == "1") unlockedMode[i] = true;
        }
    }
    fin.close();
}





struct Skill {
    std::string name;
    std::string desc;
    int costNormal;
    int costSpecial;
    bool unlocked;
    int parent;                // 親スキル（-1なら最初から選択可）
    std::vector<int> exclude;  // 同時に取得できないskillのindex
};

std::vector<Skill> skillTree = {
    // 0: 体力アップ   1: スピードUP   2: 攻撃力UP   3: 弾分岐ノード
    {"体力増加",      "最大HPが+2される",        5, 0, false, -1, {}},
    {"スピード増加",  "移動速度が+1される",        6, 0, false, 0, {}},
    {"攻撃力増加",    "攻撃力が+2される",         7, 0, false, 0, {}},
    {"弾モード選択",  "弾モード分岐解放", 0, 0, false, 0, {}},

    // 4: 追尾弾  5: 連射弾  6: 拡散弾  7: 引き寄せ弾
    {"追尾",    "追尾弾が使用可能",        8, 2, false, 3, {5,6,7}},
    {"連射",    "連射弾が使用可能",        12, 4, false, 3, {4,6,7}},
    {"拡散",    "拡散弾が使用可能",        18, 6, false, 3, {4,5,7}},
    {"ブラックホール", "引き寄せ弾が使用可能",    40, 20, false, 3, {4,5,6}},

    //// 以下、追加ノード例
    //{"弾数強化",     "同時発射弾数+1",          10, 0, false, 2, {}},
    //{"弾速度強化",   "弾の速度+10",             10, 1, false, 2, {}},
    //{"ボム解放",     "ボムが使用可能",           12, 1, false, 1, {}},
    //{"シールド強化", "シールド+1",               7, 0, false, 1, {}}
};


int selectedSkill = -1;
bool justUnlocked = false;

void SaveSkillTreeStatus(const char* filename) {
    std::ofstream ofs(filename);
    if (!ofs) return;
    ofs << normalPoint << "," << specialPoint << std::endl;
    for (const auto& s : skillTree) {
        ofs << s.name << "," << s.desc << "," << s.costNormal << "," << s.costSpecial << ","
            << (s.unlocked ? 1 : 0) << "," << s.parent << std::endl;
    }
}
void LoadSkillTreeStatus(const char* filename) {
    std::ifstream ifs(filename);
    if (!ifs) return;
    std::string line, token;
    if (getline(ifs, line)) {
        std::istringstream iss(line);
        getline(iss, token, ','); normalPoint = atoi(token.c_str());
        getline(iss, token, ','); specialPoint = atoi(token.c_str());
    }
    int idx = 0;
    while (getline(ifs, line) && idx < (int)skillTree.size()) {
        std::istringstream iss(line);
        getline(iss, token, ','); skillTree[idx].name = token;
        getline(iss, token, ','); skillTree[idx].desc = token;
        getline(iss, token, ','); skillTree[idx].costNormal = atoi(token.c_str());
        getline(iss, token, ','); skillTree[idx].costSpecial = atoi(token.c_str());
        getline(iss, token, ','); skillTree[idx].unlocked = (atoi(token.c_str()) != 0);
        getline(iss, token, ','); skillTree[idx].parent = atoi(token.c_str());
        ++idx;
    }
}


void sceneSkillTree() {

    static bool skillTreeLoaded = false;
    if (!skillTreeLoaded) {
        std::ifstream test("skilltree.csv");
        if (test.good()) {
            LoadSkillTreeStatus("skilltree.csv");
            LoadPointsFromCSV();
        }
        else {
            SaveSkillTreeStatus("skilltree.csv"); // 初期状態で新規作成
            SavePointsToCSV();
        }
        test.close();
        skillTreeLoaded = true;
    }

    // 背景
    DrawBox(0, 0, 1200, 720, GetColor(210, 230, 240), TRUE);
    DrawFormatString(700, 60, GetColor(30, 30, 60), "通常: %.0f  特殊: %d", normalPoint, specialPoint);

    // --- ツリー表示（シンプルな縦＋分岐）
    int baseX = 220, baseY = 100, nodeW = 360, nodeH = 80, gapY = 100, gapX = 190;
    int bx[16], by[16]; // 座標保存用
    for (int i = 0; i < (int)skillTree.size(); ++i) {
        // ノード位置を決定（例：分岐はX座標をずらす）
        if (i <= 3) { bx[i] = baseX; by[i] = baseY + gapY * i; }
        else if (i >= 4 && i <= 7) { // 弾分岐
            bx[i] = baseX + gapX * ((i - 4) - 1); by[i] = by[3] + gapY;
        }
        else if (i >= 8) { bx[i] = baseX + gapX; by[i] = baseY + gapY * (i - 6); }

        // 線描画
        if (skillTree[i].parent >= 0) {
            DrawLine(bx[i] + nodeW / 2, by[i], bx[skillTree[i].parent] + nodeW / 2, by[skillTree[i].parent] + nodeH, GetColor(80, 80, 80));
        }
        // ノード描画
        int col = skillTree[i].unlocked ? GetColor(130, 220, 255) : GetColor(240, 240, 250);
        if (i == selectedSkill) col = GetColor(255, 180, 80);
        DrawBox(bx[i], by[i], bx[i] + nodeW, by[i] + nodeH, col, TRUE);
        DrawBox(bx[i], by[i], bx[i] + nodeW, by[i] + nodeH, GetColor(80, 80, 80), FALSE);
        DrawFormatString(bx[i] + 8, by[i] + 30, GetColor(0, 0, 0), "%s", skillTree[i].name.c_str());

        // ノードクリックで選択
        int mx, my; GetMousePoint(&mx, &my);
        int btn = GetMouseInput();
        if ((btn & MOUSE_INPUT_LEFT) && mx >= bx[i] && mx < bx[i] + nodeW && my >= by[i] && my < by[i] + nodeH) {
            selectedSkill = i;
        }
    }

    // 選択中パネル
    int panelX = 600, panelY = 120;
    DrawBox(panelX, panelY, panelX + 500, panelY + 300, GetColor(255, 255, 255), TRUE);
    DrawBox(panelX, panelY, panelX + 500, panelY + 300, GetColor(90, 90, 90), FALSE);

    if (selectedSkill >= 0) {
        auto& s = skillTree[selectedSkill];
        DrawFormatString(panelX + 20, panelY + 20, GetColor(30, 30, 80), "[ %s ]", s.name.c_str());
        DrawFormatString(panelX + 20, panelY + 70, GetColor(40, 40, 40), "%s", s.desc.c_str());
        DrawFormatString(panelX + 20, panelY + 120, GetColor(80, 40, 0), "必要: 通常%d 特殊%d", s.costNormal, s.costSpecial);
        DrawFormatString(panelX + 20, panelY + 160, GetColor(20, 80, 40), "状態: %s", s.unlocked ? "習得済み" : "未習得");

        // 相互排他チェック（弾モード分岐）
        bool excluded = false;
        if (selectedSkill >= 4 && selectedSkill <= 7) {
            for (int e : s.exclude) {
                if (skillTree[e].unlocked) excluded = true;
            }
        }
        // 習得ボタン
        int bx2 = panelX + 280, by2 = panelY + 220, bw = 210, bh = 70;
        int btnCol = (s.unlocked ? GetColor(180, 180, 180)
            : (normalPoint >= s.costNormal && specialPoint >= s.costSpecial && !excluded ? GetColor(70, 180, 80) : GetColor(170, 90, 90)));
        DrawBox(bx2, by2, bx2 + bw, by2 + bh, btnCol, TRUE);
        DrawBox(bx2, by2, bx2 + bw, by2 + bh, GetColor(40, 40, 40), FALSE);
        DrawFormatString(bx2 + 16, by2 + 16, GetColor(255, 255, 255), s.unlocked ? "習得済" : (excluded ? "選択不可" : "強化する"));

        int mx, my; GetMousePoint(&mx, &my);
        int btn = GetMouseInput();
        static bool prevBtn = false;
        // 習得ボタン押し判定
        if ((btn & MOUSE_INPUT_LEFT) && !prevBtn && !s.unlocked &&
            mx >= bx2 && mx < bx2 + bw && my >= by2 && my < by2 + bh &&
            !excluded && (s.parent == -1 || skillTree[s.parent].unlocked) &&
            normalPoint >= s.costNormal && specialPoint >= s.costSpecial)
        {
            normalPoint -= s.costNormal;
            specialPoint -= s.costSpecial;
            s.unlocked = true;
            justUnlocked = true;
            SaveSkillTreeStatus("skilltree.csv"); // ★強化したら自動セーブ！
        }
        prevBtn = (btn & MOUSE_INPUT_LEFT);

        if (justUnlocked) {
            DrawFormatString(panelX + 20, panelY + 250, GetColor(255, 90, 90), "強化しました！");
            if (btn == 0) justUnlocked = false;
        }
    }

    // --- ボタン座標 ---
    int btnSkillX = 30, btnSkillY = HEIGHT - 70, btnW = 320, btnH = 50;
    int btnOkX = WIDTH - btnW - 500, btnOkY = HEIGHT - 70;


    DrawBox(btnOkX, btnOkY, btnOkX + btnW, btnOkY + btnH, GetColor(240,100,80), TRUE);
    DrawFormatString(btnOkX+120, btnOkY+10, GetColor(255,255,255), "決定");

    // --- マウス判定 ---
    int mx, my, btn; GetMousePoint(&mx, &my); btn = GetMouseInput();

    // 決定ボタン
    if ((btn & MOUSE_INPUT_LEFT) && mx >= btnOkX && mx <= btnOkX+btnW && my >= btnOkY && my <= btnOkY+btnH) {
        changeScene(sceneExplanation);
    }
}
#pragma endregion

#pragma region Scene:EXPLANATION

//操作説明画面
void sceneExplanation() {
    SpeechRecognize();
    
    DrawGraph(0, 0, imgSelect, FALSE);
    drawTextC(WIDTH * 0.5, HEIGHT * 0.3, "操作説明", 0x000000, 80);
    drawTextC(WIDTH * 0.5, HEIGHT * 0.5, "移動　　：WASD or アローキー\n音声認識：Q or P\n必殺技の発動\n発射　　：SPACE", 0xff0000, 40);
    drawTextC(WIDTH * 0.5, HEIGHT * 0.8, "Press SPACE to Select start.", 0xee82ee, 30);

    if (Key[KEY_INPUT_SPACE] == 1) {
        timer = 0;
        ApplySkillTreeStatus();
        changeScene(scenePlay);
    }
}

#pragma endregion

#pragma region Scene:PLAY


void UpdateAttackBuffTimer() {
    if (attackBuffTimer > 0) {
        attackBuffTimer--;
        if (attackBuffTimer == 0) attackBuffMultiplier = 1.0f;
    }
}


// ゲームプレイ画面
void scenePlay() {
    stageMap();
    drawParameter();
    movePlayer();
    SpeechRecognize();
    UpdateAttackBuffTimer();

    if (distance == STAGE_DISTANCE) {
        srand(stage);
        PlaySoundMem(bgm, DX_PLAYTYPE_LOOP);
    }
    if (distance > 0) distance--;

    if (300 < distance && distance % 20 == 0) {
        int x = 1300;
        int y = 100 + rand() % (HEIGHT - 200);
        if (stage == 1) {
            setEnemy(x, y, -3, 0, ENE_ZAKO3, imgEnemy[ENE_ZAKO3], 1);
        }
        else if (stage == 2) {
            setEnemy(x, y, -4, 0, ENE_ZAKO2, imgEnemy[ENE_ZAKO2], 3);
            setEnemy(x, y, -4, 0, ENE_ZAKO1, imgEnemy[ENE_ZAKO1], 3);
        }
        else if (stage >= 3) {
            int e = 1 + rand() % 2;
            if (e == ENE_ZAKO2) setEnemy(x, y, -4, 0, ENE_ZAKO2, imgEnemy[ENE_ZAKO2], 3);
            else setEnemy(x, y, -5, 0, ENE_ZAKO1, imgEnemy[ENE_ZAKO1], 1);
        }
    }
    if (distance == 1) {
        if (stage == 1) bossIdx = setEnemy(1300, HEIGHT / 3, 2, 0, ENE_BOSS1, imgEnemy[ENE_BOSS1], 200);
        else if (stage == 2) { printfDx("boss2 set img=%d\n", imgEnemy[ENE_BOSS2]); bossIdx = setEnemy(1300, HEIGHT / 3, 3, 0, ENE_BOSS2, imgEnemy[ENE_BOSS2], 200); }
        else if (stage == 3) bossIdx = setEnemy(1300, HEIGHT / 3, 4, 0, ENE_BOSS3, imgEnemy[ENE_BOSS3], 200);
    }

    if (distance % 800 == 1) setItem();

    if (player.shield == 0) {
        StopSoundMem(bgm);
        timer = 0;
        changeScene(sceneOver);
        return;
    }

    // クリア条件判定
    if (enemy[bossIdx].state == 0 && distance == 0) {
        changeScene(sceneClear);
        return;
    }
}
#pragma endregion


#pragma region Scene:OVER
// ゲームオーバー
void sceneOver() {
    if (timer < FPS * 5) {
        if (timer % 7 == 0) setEffect(player.x + rand() % 81 - 40, player.y + rand() % 81 - 40, EFF_EXPLODE);
    }
    else if (timer == FPS * 3) {
        PlaySoundMem(jinOver, DX_PLAYTYPE_BACK);
    }
    else {
        DrawGraph(0, 0, imgGO, FALSE);
        drawTextC(WIDTH * 0.5, HEIGHT * 0.3, "失敗", 0xff0000, 80);
        drawTextC(WIDTH * 0.5, HEIGHT * 0.5, "貴様はスクラップ行きだ\nRキーでタイトルに戻る", 0xff0000, 80);
    }
    if (Key[KEY_INPUT_R] > 1) {
        StopSoundMem(jinOver);
        SavePointsToCSV();
        changeScene(sceneTitle);
    }
    
}
#pragma endregion


#pragma region Scene:CLEAR
// ステージクリア
void sceneClear() {
    movePlayer();
    if (timer < FPS * 5) {
        if (timer % 7 == 0) setEffect(enemy[bossIdx].x + rand() % 201 - 100, enemy[bossIdx].y + rand() % 201 - 100, EFF_EXPLODE);
    }
    else if (timer == FPS * 5) {
        PlaySoundMem(jinClear, DX_PLAYTYPE_BACK);
    }
    else if(stage <= 2){
        DrawGraph(0, 0, imgGalaxy, FALSE);
        drawTextC(WIDTH * 0.5, HEIGHT * 0.3, "討伐成功", 0x00ffff, 80);
        drawTextC(WIDTH * 0.5, HEIGHT * 0.5, "次のステージへ\n\tRキーで挑戦", 0xff0000, 80);
    }
    else {
        DrawGraph(0, 0, imgGalaxy, FALSE);
        drawTextC(WIDTH * 0.5, HEIGHT * 0.3, "完全CLEAR", 0x00ffff, 80);
        drawTextC(WIDTH * 0.5, HEIGHT * 0.5, "タイトルへ\n\tRキーで戻る", 0xff0000, 80);
    }
    if (Key[KEY_INPUT_R] > 1 && stage <= 2) {
        StopSoundMem(jinClear);
        SavePointsToCSV();
        stage = 3;
        distance = STAGE_DISTANCE;
        changeScene(scenePlay);
    }
    else if (Key[KEY_INPUT_R] > 1 && stage > 2) {
        StopSoundMem(jinClear);
        SavePointsToCSV();
        stage = 1;
        distance = STAGE_DISTANCE;
        changeScene(sceneTitle);
    }
    
}
#pragma endregion

// ==== ゲーム初期化・共通処理 ====

#pragma region 初期化関数

void initGame(void)
{
    // 背景用の画像の読み込み
    imgGalaxy = LoadGraph("image/bg0.png");//タイトル、クリア


    imgSelect = LoadGraph("image/bgR.png");//操作説明、セレクト
    imgEasy = LoadGraph("image/bgE.png");//Easy
    imgNormal = LoadGraph("image/bgN.png");//Normal
    imgHard = LoadGraph("image/bgH.png");//Hard
    imgGO = LoadGraph("image/bgGO.png");//GameOver
    imgGC1 = LoadGraph("image/bg0.png");//GameClear1
    imgGC2 = LoadGraph("image/bg0.png");//GameClear2
    imgGC3 = LoadGraph("image/bg0.png");//GameClear3
    

    // 自機と自機の弾の画像の読み込み
    imgFighter = LoadGraph("image/PL.png");
    imgBullet = LoadGraph("image/bullet.png");

    // 敵機の画像の読み込み
    for (int i = 0; i < IMG_ENEMY_MAX; i++) {
        char file[] = "image/enemy*.png";
        file[11] = (char)('0' + i);
        imgEnemy[i] = LoadGraph(file);
    }

    // その他の画像の読み込み
    imgExplosion = LoadGraph("image/explosion.png"); // 爆発演出
    imgItem = LoadGraph("image/item.png"); // アイテム

    // サウンドの読み込みと音量設定
    bgm = LoadSoundMem("sound/MikkE_.mp3");
    jinOver = LoadSoundMem("sound/03_蠢く_L_.mp3");
    jinClear = LoadSoundMem("sound/result.mp3");
    seExpl = LoadSoundMem("sound/explosion.mp3");
    seItem = LoadSoundMem("sound/item.mp3");
    seShot = LoadSoundMem("sound/shot.mp3");


    ChangeVolumeSoundMem(128, bgm);
    ChangeVolumeSoundMem(128, jinOver);
    ChangeVolumeSoundMem(128, jinClear);


    changeImageColor("image/enemy0.png", imgEnemy[0], 255, 55, 255); // 赤を強調
    changeImageColor("image/enemy1.png", imgEnemy[1], 255, 55, 255); // 赤を強調
    changeImageColor("image/enemy2.png", imgEnemy[2], 55, 55, 255); // 赤を強調
    changeImageColor("image/bullet.png", imgBullet, 255, 255, 255); // 赤を強調
}
#pragma endregion

#pragma region スクロール

// 背景のスクロール
void scrollBG(int spd)
{
    static int galaxyY, gameX; // スクロール位置を管理する変数（静的記憶領域に保持される）

    if (isCurrentScene(sceneTitle)) {
        galaxyY = (galaxyY + spd) % HEIGHT; // 星空（宇宙）
        DrawGraph(0, galaxyY - HEIGHT, imgGalaxy, FALSE);
        DrawGraph(0, galaxyY, imgGalaxy, FALSE);
    }
    

    
    if (isCurrentScene(scenePlay)) {
        gameX -= spd; // 星空（宇宙）
        if (gameX <= -WIDTH)gameX += WIDTH;
        if (stage == 1) {
            DrawGraph(gameX, 0, imgEasy, FALSE);
            DrawGraph(gameX + WIDTH, 0, imgEasy, FALSE);
        }
        else if (stage == 2) {
            DrawGraph(gameX, 0, imgNormal, FALSE);
            DrawGraph(gameX + WIDTH, 0, imgNormal, FALSE);
        }
        else if (stage == 3) {
            DrawGraph(gameX, 0, imgHard, FALSE);
            DrawGraph(gameX + WIDTH, 0, imgHard, FALSE);
        }
    }
        


}
#pragma endregion

#pragma region 初期値を代入する

// ゲーム開始時の初期値を代入する関数
void initVariable(void)
{
    player.x = WIDTH / 2;
    player.y = HEIGHT / 2;
    player.vx = 5;
    player.vy = 5;
    player.shield = PLAYER_SHIELD_MAX;
    GetGraphSize(imgFighter, &player.wid, &player.hei); // 自機の画像の幅と高さを代入
    for (int i = 0; i < ENEMY_MAX; i++) enemy[i].state = 0; // 全ての敵機を存在しない状態に
    score = 0;
    stage = 1;
    noDamage = 0;
    weaponLv = 1;
    distance = STAGE_DISTANCE;
    mode = NORMAL;
    drawText(10, 60, "NORMAL", score, 0xffffff, 30);
    
    
}
#pragma endregion

#pragma region 能力強化反映関数

// スキルツリー状態をplayerに反映（ゲーム開始時やスキル習得時に呼ぶ）
void ApplySkillTreeStatus() {
    // デフォルト値
    player.shield = PLAYER_SHIELD_MAX;
    player.vx = 5;
    player.vy = 5;
    weaponLv = 1;
    // スキルツリーを参照
    for (auto& s : skillTree) {
        if (!s.unlocked) continue;
        if (s.name == "体力増加") {
            player.shield += 2;
            //if (player.shield > PLAYER_SHIELD_MAX) player.shield = PLAYER_SHIELD_MAX;
        }
        if (s.name == "スピード増加") {
            player.vx += 1;
            player.vy += 1;
            if (player.vx > PLAYER_SPEED_MAX) player.vx = PLAYER_SPEED_MAX;
        }
        if (s.name == "攻撃力増加") {
            // 攻撃力（バフ倍率に加算）例
            attackBuffMultiplier += 1.0f; // 50%増し
        }
        
    }
}
#pragma endregion

#pragma region ステージマップ関数


void stageMap(void)
{
    int mx = 30, my = 10; // マップの表示位置（上側に移動）
    int wi = WIDTH - 60, he = 20; // マップの幅と高さ（横長に変更）
    int pos = (WIDTH - 80) * (STAGE_DISTANCE - distance) / STAGE_DISTANCE; // 自機の進行位置
    SetDrawBlendMode(DX_BLENDMODE_SUB, 128); // 減算による描画の重ね合わせ
    DrawBox(mx, my, mx + wi, my + he, 0xffffff, TRUE); // マップの枠内を描画
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // ブレンドモードを解除
    DrawBox(mx - 1, my - 1, mx + wi + 1, my + he + 1, 0xffffff, FALSE); // 枠線
    DrawBox(mx + pos, my, mx + pos + 20, my + he, 0x0080ff, TRUE); // 自機の位置を表示

}
#pragma endregion



// ==== プレイヤー・弾 ====

#pragma region プレイヤーの動作関数

// 自機を動かす関数(斜め移動の正規化機能追加)
void movePlayer(void)
{
    static char oldSpcKey; // 1つ前のスペースキーの状態を保持する変数
    static int countSpcKey; // スペースキーを押し続けている間、カウントアップする変数

    float rate = 1.0f;
    //移動量の倍率係数
    if (Key[KEY_INPUT_LEFT] > 0 || Key[KEY_INPUT_RIGHT] > 0) {
        if (Key[KEY_INPUT_UP] > 0 || Key[KEY_INPUT_DOWN] > 0) {
            //斜めの場合は、移動量の倍率係数を０．７１に設定
            rate = 0.707f;
        }
    }

    //実際のx,y座標の移動量
    int moveX = player.vx * rate;
    int moveY = player.vy * rate;



    if (CheckHitKey(KEY_INPUT_UP) || CheckHitKey(KEY_INPUT_W)) { // 上キー
        player.y -= moveY;
        if (player.y < 30) player.y = 30;
    }
    if (CheckHitKey(KEY_INPUT_DOWN) || CheckHitKey(KEY_INPUT_S)) { // 下キー
        player.y += moveY;
        if (player.y > HEIGHT - 30) player.y = HEIGHT - 30;
    }
    if (CheckHitKey(KEY_INPUT_LEFT) || CheckHitKey(KEY_INPUT_A)) { // 左キー
        player.x -= moveX;
        if (player.x < 30) player.x = 30;
    }
    if (CheckHitKey(KEY_INPUT_RIGHT) || CheckHitKey(KEY_INPUT_D)) { // 右キー
        player.x += moveX;
        if (player.x > WIDTH - 30) player.x = WIDTH - 30;
    }


    if (CheckHitKey(KEY_INPUT_SPACE)) { // スペースキー



        if (mode == RAPIDFIRE) {
            if (oldSpcKey == 0) setBullet(); // 押した瞬間、発射
            else if (countSpcKey % 7 == 0) setBullet(); // 一定間隔で発射
            countSpcKey++;
        }
        else if (mode == CHASE) {
            if (oldSpcKey == 0) setBullet(); // 押した瞬間、発射
            else if (countSpcKey % 15 == 0) setBullet(); // 一定間隔で発射
            countSpcKey++;
        }
        else if (mode == EXPLOSION) {
            if (oldSpcKey == 0) setBullet(); // 押した瞬間、発射
            else if (countSpcKey % 25 == 0) setBullet(); // 一定間隔で発射
            countSpcKey++;
        }
        else if (mode == BLACKHALL) {
            if (oldSpcKey == 0) setBullet(); // 押した瞬間、発射
            else if (countSpcKey % 45 == 0) setBullet(); // 一定間隔で発射
            countSpcKey++;
        }
        else {
            if (oldSpcKey == 0) setBullet(); // 押した瞬間、発射
            else if (countSpcKey % 20 == 0) setBullet(); // 一定間隔で発射
            countSpcKey++;
        }

    }
    oldSpcKey = CheckHitKey(KEY_INPUT_SPACE); // スペースキーの状態を保持
    if (noDamage > 0) noDamage--; // 無敵時間のカウント
    if (noDamage % 4 < 2) drawImage(imgFighter, player.x, player.y); // 自機の描画
}

#pragma endregion

#pragma region
//タマのセット、発射

void setBullet(void)
{// EXPLOSION（拡散）時は弾数を5発で固定
    int nShots = (mode == EXPLOSION) ? 5 : weaponLv;

    OBJECT* bulletp = &bullet[0];
    for (int n = 0; n < nShots; n++) {
        int x = player.x;
        int y = player.y - (nShots - 1) * 5 + n * 10;
        for (int i = 0; i < BULLET_MAX; i++, bulletp++) {
            if (bulletp->state == 0) {
                bulletp->x = x;
                bulletp->y = y;

                if (mode == EXPLOSION) {
                    // 5WAY扇状発射
                    float spread = 45.0f; // 扇の全体角度（度数 例: 45度）
                    float angle = (-spread / 2.0f) + (spread / (nShots - 1)) * n; // -22.5〜+22.5
                    float rad = angle * 3.141592f / 180.0f;
                    bulletp->vx = (int)(40 * cos(rad));
                    bulletp->vy = (int)(40 * sin(rad));
                }
                else {
                    bulletp->vx = 40;
                    bulletp->vy = 0;
                }
                bulletp->state = 1;

                
                break;
            }
        }
    }
    PlaySoundMem(seShot, DX_PLAYTYPE_BACK); // 効果音
}
#pragma endregion

#pragma region 弾の動作関数

void moveBullet(void)
{

    if (mode == CHASE)
    {

        for (int i = 0; i < BULLET_MAX; i++)
        {
            OBJECT* bulletp = &bullet[i];
            if (bulletp->state == 0) continue; // 弾が発射されていない場合はスキップ

            // 追尾処理
            OBJECT* nearestEnemy = nullptr;
            float minDist = FLT_MAX;

            // 追尾対象（ENE_ZAKO1, ENE_ZAKO2）の敵を探す
            for (int j = 0; j < ENEMY_MAX; j++)
            {
                OBJECT* enemyp = &enemy[j];
                //  if (enemyp->state == 0 || (enemyp->pattern != ENE_ZAKO1 && enemyp->pattern != ENE_ZAKO2)) continue;

                float dx = enemyp->x - bulletp->x;
                float dy = enemyp->y - bulletp->y;
                float dist = dx * dx + dy * dy;

                if (dist < minDist)
                {
                    minDist = dist;
                    nearestEnemy = enemyp;
                }
            }

            // 最も近い敵を追尾
            if (nearestEnemy)
            {
                float dx = nearestEnemy->x - bulletp->x;
                float dy = nearestEnemy->y - bulletp->y;
                float magnitude = sqrt(dx * dx + dy * dy);
                if (magnitude > 0) {
                    bulletp->vx = (int)(40 * (dx / magnitude));  // 追尾方向に速度を設定
                    bulletp->vy = (int)(40 * (dy / magnitude));
                }
            }

            // 弾の移動
            bulletp->x += bulletp->vx;
            bulletp->y += bulletp->vy;

            // 弾が画面外に出た場合、状態を無効にする
            if (bulletp->x < -50 || bulletp->x > WIDTH + 50 || bulletp->y < -50 || bulletp->y > HEIGHT + 50)
            {
                bulletp->state = 0;
            }

            // 弾の描画
            drawImage(imgBullet, bulletp->x, bulletp->y);
        }
    }

    else if (mode == EXPLOSION)
    {
        // 拡散（扇状）発射。通常はsetBulletでvx,vyに角度を設定するが、ここでは弾の動きを通常弾と同じでOK
        OBJECT* bulletp = &bullet[0];
        for (int i = 0; i < BULLET_MAX; i++, bulletp++) {
            if (bulletp->state == 0) continue;
            bulletp->x += bulletp->vx;
            bulletp->y += bulletp->vy;
            drawImage(imgBullet, bulletp->x, bulletp->y);
            if (bulletp->x > 1300 || bulletp->x < -50 || bulletp->y < -50 || bulletp->y > HEIGHT + 50) bulletp->state = 0;
        }
    }
    else if (mode == BLACKHALL)
    {
        // ブラックホール：弾が敵を吸い寄せる＋当たった敵は即死
        OBJECT* bulletp = &bullet[0];
        for (int i = 0; i < BULLET_MAX; i++, bulletp++) {
            if (bulletp->state == 0) continue;

            // 1. 弾の位置を進める
            bulletp->x += bulletp->vx;
            bulletp->y += bulletp->vy;

            // 2. すべての敵を吸い寄せ
            for (int j = 0; j < ENEMY_MAX; ++j) {
                OBJECT* enemyp = &enemy[j];
                if (enemyp->state == 0) continue;

                // 距離計算
                float dx = bulletp->x - enemyp->x;
                float dy = bulletp->y - enemyp->y;
                float dist = sqrtf(dx * dx + dy * dy);

                // 一定範囲内なら吸引（近づける）
                if (dist > 1.0f && dist < 400.0f) {
                    float pull = 4.0f; // 吸引力
                    enemyp->x += (int)(pull * dx / dist);
                    enemyp->y += (int)(pull * dy / dist);
                }

                // 衝突判定（重なったら即死）
                if (dist < 40.0f) {
                    enemyp->shield = 0;
                    enemyp->state = 0;
                    setEffect(enemyp->x, enemyp->y, EFF_EXPLODE); // 爆発演出
                }
            }

            drawImage(imgBullet, bulletp->x, bulletp->y);
            if (bulletp->x > 1300 || bulletp->x < -50 || bulletp->y < -50 || bulletp->y > HEIGHT + 50) bulletp->state = 0;
        }
    }

    else
    {
        OBJECT* bulletp = &bullet[0];
        for (int i = 0; i < BULLET_MAX; i++, bulletp++) {
            if (bulletp->state == 0) continue; // 空いている配列なら処理しない
            bulletp->x += bulletp->vx; // ┬ 座標を変化させる
            bulletp->y += bulletp->vy; // ┘
            drawImage(imgBullet, bulletp->x, bulletp->y); // 弾の描画
            if (bulletp->x > 1300) bulletp->state = 0; // 画面外に出たら、存在しない状態にする
        }
    }

}
#pragma endregion

// ==== 敵・ダメージ処理 ====

#pragma region 敵のセット

// 敵機をセットする
int setEnemy(int x, int y, int vx, int vy, int ptn, int img, int sld)
{
    Enemy* enemyp = &enemy[0];
    if (ptn == ENE_BOSS3) {
        enemyp->boss3_phase = 0;
        enemyp->boss3_targetX = WIDTH / 2;
        enemyp->boss3_targetY = HEIGHT / 2;
        enemyp->boss3_shot_timer = 0;
    }
    for (int i = 0; i < ENEMY_MAX; i++, enemyp++) {
        if (enemyp->state == 0) {
            enemyp->x = x;
            enemyp->y = y;
            enemyp->vx = vx;
            enemyp->vy = vy;
            enemyp->state = 1;
            enemyp->pattern = ptn;
            enemyp->image = img;
            enemyp->shield = sld * stage + (rand() % 6 + 3);; // ステージが進むほど敵が固くなる
            GetGraphSize(img, &enemyp->wid, &enemyp->hei); // 画像の幅と高さを代入
            return i;
        }
    }
    
    return -1;
}
#pragma endregion

#pragma region 敵の動作

// 敵機を動かす
void moveEnemy(void)
{
    Enemy* enemyp = &enemy[0];
    for (int i = 0; i < ENEMY_MAX; i++, enemyp++) {
        if (enemyp->state == 0) continue; // 空いている配列なら処理しない

        if (enemyp->pattern == ENE_BOSS1) // ボス機
        {
            // ボスが画面右側から左方向へ登場
            if (enemyp->x > WIDTH - 300 && enemyp->vx > 0)
            {
                enemyp->vx = -3; // 左方向に移動
            }

            // 一定位置まで移動後、上下に動きながら攻撃
            if (enemyp->x <= WIDTH - 300)
            {
                enemyp->vx = 0; // 横移動を停止

                // 上下の移動方向を制御
                if (enemyp->y >= HEIGHT - 100) // 下端で反転
                    enemyp->vy = -2;
                else if (enemyp->y <= 100) // 上端で反転
                    enemyp->vy = 2;

                // 弾発射
                if (rand() % 50 == 0) // 一定間隔で攻撃
                {
                    for (int bx = -2; bx <= 2; bx++) // 左方向に扇状の弾を発射
                    {
                        setEnemy(enemyp->x - 50, enemyp->y, -5 + bx * 2, bx, ENE_BULLET, imgEnemy[ENE_BULLET], 0);
                    }
                }
            }
        }

        // --- ENE_BOSS2：ウィンドウ端で回転→突撃ボス ---
        static float boss2_angle = 0.0f;
        static bool boss2_rotating = false;
        static int boss2_wait = 0;
        if (enemyp->pattern == ENE_BOSS2) {
            // 端に到達 or 初期状態
            if (!boss2_rotating && (enemyp->x <= 50 || enemyp->x >= WIDTH - 50 || enemyp->y <= 50 || enemyp->y >= HEIGHT - 50 || enemyp->vx == 0)) {
                boss2_rotating = true;
                boss2_angle = (float)(rand() % 360) * 3.14159f / 180.0f; // ランダムな角度
                boss2_wait = 30 + rand() % 40; // 少し待ってから発射
                enemyp->vx = enemyp->vy = 0;
            }
            // 回転演出
            if (boss2_rotating) {
                boss2_wait--;
                // 回転演出用エフェクト（任意）
                // DrawCircle(enemyp->x, enemyp->y, 60, 0xff6666, FALSE);
                if (boss2_wait <= 0) {
                    // 新たな方向に突撃
                    enemyp->vx = (int)(7 * cos(boss2_angle));
                    enemyp->vy = (int)(7 * sin(boss2_angle));
                    boss2_rotating = false;
                }
            }
            // 弾とBOSS2の当たり判定
            if (enemyp->shield > 0) {
                for (int j = 0; j < BULLET_MAX; j++) {
                    if (bullet[j].state == 0) continue;
                    int dx = abs((int)(enemyp->x - bullet[j].x));
                    int dy = abs((int)(enemyp->y - bullet[j].y));
                    if (dx < enemyp->wid / 2 && dy < enemyp->hei / 2) {
                        bullet[j].state = 0;
                        damageEnemy(enemyp, 1);
                    }
                }
            }
            // プレイヤーとの当たり判定
            if (noDamage == 0) {
                int dx = abs(enemyp->x - player.x);
                int dy = abs(enemyp->y - player.y);
                if (dx < enemyp->wid / 2 + player.wid / 2 && dy < enemyp->hei / 2 + player.hei / 2) {
                    if (player.shield > 0) player.shield--;
                    noDamage = FPS;
                    damageEnemy(enemyp, 1);
                }
            }

            // 壁についたら再度回転
            enemyp->x += enemyp->vx;
            enemyp->y += enemyp->vy;
            drawImage(enemyp->image, enemyp->x, enemyp->y);
            continue; // 他の処理スキップ
        }

        // --- ENE_BOSS3：移動→ばらまき→移動を繰り返すボス ---
        static int boss3_phase = 0; // 0:中央移動 1:目標地点移動 2:弾発射
        static int boss3_targetX = WIDTH / 2, boss3_targetY = HEIGHT / 2;
        static int boss3_shot_timer = 0;
        if (enemyp->pattern == ENE_BOSS3) {
            int frameW = 240;
            int frameH = 240;
            int maxFrame = 4;
            int animFrame = (enemyp->timer / 4) % maxFrame;
            DrawRectGraph(
                enemyp->x - frameW / 2,
                enemyp->y - frameH / 2,
                animFrame * frameW, 0, frameW, frameH,
                imgEnemy[ENE_BOSS3], TRUE, FALSE
            );
            enemyp->timer++;

            // ここからは「enemyp->boss3_phase」など自分用に
            if (enemyp->boss3_phase == 0) {
                // 中央へ移動
                enemyp->boss3_targetX = WIDTH / 2;
                enemyp->boss3_targetY = HEIGHT / 2;
                float dx = enemyp->boss3_targetX - enemyp->x;
                float dy = enemyp->boss3_targetY - enemyp->y;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist > 10) {
                    enemyp->vx = (int)(5 * dx / dist);
                    enemyp->vy = (int)(5 * dy / dist);
                }
                else {
                    enemyp->vx = enemyp->vy = 0;
                    enemyp->boss3_phase = 1;
                    enemyp->boss3_targetX = 100 + rand() % (WIDTH - 200);
                    enemyp->boss3_targetY = 100 + rand() % (HEIGHT - 200);
                }
            }
            else if (enemyp->boss3_phase == 1) {
                // ランダム地点へ移動
                float dx = enemyp->boss3_targetX - enemyp->x;
                float dy = enemyp->boss3_targetY - enemyp->y;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist > 10) {
                    enemyp->vx = (int)(6 * dx / dist);
                    enemyp->vy = (int)(6 * dy / dist);
                }
                else {
                    enemyp->vx = enemyp->vy = 0;
                    enemyp->boss3_phase = 2;
                    enemyp->boss3_shot_timer = 0;
                }
            }
            else if (enemyp->boss3_phase == 2) {
                // 弾をばらまく
                if (enemyp->boss3_shot_timer == 0) {
                    int nShot = 16;
                    for (int k = 0; k < nShot; k++) {
                        float theta = 2.0f * 3.14159f * k / nShot;
                        int vx = (int)(8 * cos(theta));
                        int vy = (int)(8 * sin(theta));
                        setEnemy(enemyp->x, enemyp->y, vx, vy, ENE_BULLET, imgEnemy[ENE_BULLET], 0);
                    }
                }
                enemyp->boss3_shot_timer++;
                if (enemyp->boss3_shot_timer > 50) {
                    enemyp->boss3_phase = 1;
                    enemyp->boss3_targetX = 100 + rand() % (WIDTH - 200);
                    enemyp->boss3_targetY = 100 + rand() % (HEIGHT - 200);
                }
            }

            enemyp->x += enemyp->vx;
            enemyp->y += enemyp->vy;
            
            if (enemyp->shield > 0) {
                for (int j = 0; j < BULLET_MAX; j++) {
                    if (bullet[j].state == 0) continue;
                    int dx = abs((int)(enemyp->x - bullet[j].x));
                    int dy = abs((int)(enemyp->y - bullet[j].y));
                    if (dx < enemyp->wid / 2 && dy < enemyp->hei / 2) {
                        bullet[j].state = 0;
                        damageEnemy(enemyp, 1);
                    }
                }
            }
            // プレイヤーとの当たり判定
            if (noDamage == 0) {
                int dx = abs(enemyp->x - player.x);
                int dy = abs(enemyp->y - player.y);
                if (dx < enemyp->wid / 2 + player.wid / 2 && dy < enemyp->hei / 2 + player.hei / 2) {
                    if (player.shield > 0) player.shield--;
                    noDamage = FPS;
                    damageEnemy(enemyp, 1);
                }
            }

            continue;
        }

        // --- ZAKO1: プレイヤーをゆっくり追尾
        if (enemyp->pattern == ENE_ZAKO1) {
            float dx = player.x - enemyp->x;
            float dy = player.y - enemyp->y;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist > 0.1f) {
                enemyp->vx = (int)(6 * dx / dist);  // 2はスピード
                enemyp->vy = (int)(6 * dy / dist);
            }

            enemyp->x += enemyp->vx;
            enemyp->y += enemyp->vy;
            drawImage(enemyp->image, enemyp->x, enemyp->y);
            // --- 当たり判定 ---
            if (enemyp->shield > 0) {
                for (int j = 0; j < BULLET_MAX; j++) {
                    if (bullet[j].state == 0) continue;
                    int dx = abs((int)(enemyp->x - bullet[j].x));
                    int dy = abs((int)(enemyp->y - bullet[j].y));
                    if (dx < enemyp->wid / 2 && dy < enemyp->hei / 2) {
                        bullet[j].state = 0;
                        damageEnemy(enemyp, 1);
                    }
                }
            }
            if (noDamage == 0) {
                int dx = abs(enemyp->x - player.x);
                int dy = abs(enemyp->y - player.y);
                if (dx < enemyp->wid / 2 + player.wid / 2 && dy < enemyp->hei / 2 + player.hei / 2) {
                    if (player.shield > 0) player.shield--;
                    noDamage = FPS;
                    damageEnemy(enemyp, 1);
                }
            }
            continue;
        }

        // --- ZAKO2: ジグザグ移動
        if (enemyp->pattern == ENE_ZAKO2) {
            enemyp->vx = -4; // 左へ移動（速度調整は適宜）
            enemyp->vy = (int)(4 * sin(enemyp->timer / 10.0)); // ジグザグ
            enemyp->x += enemyp->vx;
            enemyp->y += enemyp->vy;
            drawImage(enemyp->image, enemyp->x, enemyp->y);
            enemyp->timer++; // timerをカウント
            // --- 当たり判定 ---
            if (enemyp->shield > 0) {
                for (int j = 0; j < BULLET_MAX; j++) {
                    if (bullet[j].state == 0) continue;
                    int dx = abs((int)(enemyp->x - bullet[j].x));
                    int dy = abs((int)(enemyp->y - bullet[j].y));
                    if (dx < enemyp->wid / 2 && dy < enemyp->hei / 2) {
                        bullet[j].state = 0;
                        damageEnemy(enemyp, 1);
                    }
                }
            }
            if (noDamage == 0) {
                int dx = abs(enemyp->x - player.x);
                int dy = abs(enemyp->y - player.y);
                if (dx < enemyp->wid / 2 + player.wid / 2 && dy < enemyp->hei / 2 + player.hei / 2) {
                    if (player.shield > 0) player.shield--;
                    noDamage = FPS;
                    damageEnemy(enemyp, 1);
                }
            }
            continue;
        }

        enemyp->x += enemyp->vx; //┬敵機の移動
        enemyp->y += enemyp->vy; //┘
        drawImage(enemyp->image, enemyp->x, enemyp->y); // 敵機の描画
        // 画面外に出たか？
        if (enemyp->x < -200 || WIDTH + 200 < enemyp->x || enemyp->y < -200 || HEIGHT + 200 < enemyp->y) enemyp->state = 0;
        // 当たり判定のアルゴリズム
        if (enemyp->shield > 0) // ヒットチェックを行う敵機（弾以外）
        {
            for (int j = 0; j < BULLET_MAX; j++) { // 自機の弾とヒットチェック
                if (bullet[j].state == 0) continue;
                int dx = abs((int)(enemyp->x - bullet[j].x)); //┬中心座標間のピクセル数
                int dy = abs((int)(enemyp->y - bullet[j].y)); //┘
                if (dx < enemyp->wid / 2 && dy < enemyp->hei / 2) // 接触しているか
                {
                    bullet[j].state = 0; // 弾を消す
                    damageEnemy(enemyp, 1); // 敵にダメージ
                }
            }
        }
        if (noDamage == 0) // 無敵状態でない時、自機とヒットチェック
        {
            int dx = abs(enemyp->x - player.x); //┬中心座標間のピクセル数
            int dy = abs(enemyp->y - player.y); //┘
            if (dx < enemyp->wid / 2 + player.wid / 2 && dy < enemyp->hei / 2 + player.hei / 2)
            {
                if (player.shield > 0) player.shield--; // シールドを減らす
                noDamage = FPS; // 無敵状態をセット
                damageEnemy(enemyp, 1); // 敵にダメージ
            }
        }
    }
}
#pragma endregion

#pragma region 敵の体力処理

// 敵機のシールドを減らす（ダメージを与える）
void damageEnemy(OBJECT* enemyp, int dmg)
{
    SetDrawBlendMode(DX_BLENDMODE_ADD, 192); // 加算による描画の重ね合わせ
    DrawCircle(enemyp->x, enemyp->y, (enemyp->wid + enemyp->hei) / 4, 0xff0000, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // ブレンドモードを解除
    score += 100; // スコアの加算
    if (score > hisco) hisco = score; // ハイスコアの更新
    enemyp->shield -= dmg * attackBuffMultiplier;; // シールドを減らす
    if (enemyp->shield <= 0)
    {
        enemyp->state = 0; // シールド0以下で消す
        setEffect(enemyp->x, enemyp->y, EFF_EXPLODE); // 爆発演出
        // 雑魚撃破時のみ
        if (enemyp->pattern == ENE_ZAKO1 || enemyp->pattern == ENE_ZAKO2 || enemyp->pattern == ENE_ZAKO3) {
            normalPoint += 0.05f;
        }
        
        
        if (enemyp->pattern == ENE_BOSS1 || enemyp->pattern == ENE_BOSS2 || enemyp->pattern == ENE_BOSS3) {
            int bonus = 2; // デフォルト2
            if (stage == 2) bonus = 4;
            else if (stage == 3) bonus = 6;
            specialPoint += bonus;
            StopSoundMem(bgm);
            timer = 0;
            changeScene(sceneClear);
            
        }
        
    }
}
#pragma endregion



// ==== エフェクト・アイテム ====

#pragma region エフェクトのセット関数

// エフェクトのセット
void setEffect(int x, int y, int ptn)
{
    static int eff_num;
    effect[eff_num].x = x;
    effect[eff_num].y = y;
    effect[eff_num].state = 1;
    effect[eff_num].pattern = ptn;
    effect[eff_num].timer = 0;
    eff_num = (eff_num + 1) % EFFECT_MAX;
    if (ptn == EFF_EXPLODE) PlaySoundMem(seExpl, DX_PLAYTYPE_BACK); // 効果音
}
#pragma endregion

#pragma region エフェクト描画

// エフェクトの描画
void drawEffect(void)
{
    int ix;
    OBJECT* effectp = &effect[0];
    for (int i = 0; i < EFFECT_MAX; i++, effectp++)
    {
        if (effectp->state == 0) continue;
        switch (effectp->pattern) // エフェクトごとに処理を分ける
        {
        case EFF_EXPLODE: // 爆発演出
            ix = effectp->timer * 128; // 画像の切り出し位置
            DrawRectGraph(effectp->x - 64, effectp->y - 64, ix, 0, 128, 128, imgExplosion, TRUE, FALSE);
            effectp->timer++;
            if (effectp->timer == 7) effectp->state = 0;
            break;

        case EFF_RECOVER: // 回復演出
            if (effectp->timer < 30) // 加算による描画の重ね合わせ
                SetDrawBlendMode(DX_BLENDMODE_ADD, effectp->timer * 8);
            else
                SetDrawBlendMode(DX_BLENDMODE_ADD, (60 - effectp->timer) * 8);
            for (int i = 3; i < 8; i++) DrawCircle(player.x, player.y, (player.wid + player.hei) / i, 0x7fffd4, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // ブレンドモードを解除
            effectp->timer++;
            if (effectp->timer == 60) effectp->state = 0;
            break;



        case EFF_MODE_EXPLOSION: // 切り替え爆破
            if (effectp->timer < 30) // 加算による描画の重ね合わせ
                SetDrawBlendMode(DX_BLENDMODE_ADD, effectp->timer * 8);
            else
                SetDrawBlendMode(DX_BLENDMODE_ADD, (60 - effectp->timer) * 8);
            for (int i = 3; i < 8; i++) DrawCircle(player.x, player.y, (player.wid + player.hei) / i, 0xfa0700, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // ブレンドモードを解除
            effectp->timer++;
            if (effectp->timer == 60) effectp->state = 0;
            break;

        case EFF_MODE_BLACKHALL: // 切り替えブラックホール
            if (effectp->timer < 30) // 加算による描画の重ね合わせ
                SetDrawBlendMode(DX_BLENDMODE_ADD, effectp->timer * 8);
            else
                SetDrawBlendMode(DX_BLENDMODE_ADD, (60 - effectp->timer) * 8);
            for (int i = 3; i < 8; i++) DrawCircle(player.x, player.y, (player.wid + player.hei) / i, 0x000000, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // ブレンドモードを解除
            effectp->timer++;
            if (effectp->timer == 60) effectp->state = 0;
            break;

        case EFF_MODE_CHASE: // 切り替え追尾
            if (effectp->timer < 30) // 加算による描画の重ね合わせ
                SetDrawBlendMode(DX_BLENDMODE_ADD, effectp->timer * 8);
            else
                SetDrawBlendMode(DX_BLENDMODE_ADD, (60 - effectp->timer) * 8);
            for (int i = 3; i < 8; i++) DrawCircle(player.x, player.y, (player.wid + player.hei) / i, 0x00ccff, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // ブレンドモードを解除
            effectp->timer++;
            if (effectp->timer == 60) effectp->state = 0;
            break;


        case EFF_MODE_RAPIDFIRE: // 切り替え連射
            if (effectp->timer < 30) // 加算による描画の重ね合わせ
                SetDrawBlendMode(DX_BLENDMODE_ADD, effectp->timer * 8);
            else
                SetDrawBlendMode(DX_BLENDMODE_ADD, (60 - effectp->timer) * 8);
            for (int i = 3; i < 8; i++) DrawCircle(player.x, player.y, (player.wid + player.hei) / i, 0x999999, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // ブレンドモードを解除
            effectp->timer++;
            if (effectp->timer == 60) effectp->state = 0;
            break;

        }

    }
}
#pragma endregion

#pragma region アイテムセット関数

// アイテムをセット
void setItem(void)
{
    item.x = (WIDTH / 4) * (1 + rand() % 3);
    item.y = -16;
    item.vx = 15;
    item.vy = 1;
    item.state = 1;
    item.timer = 0;
}
#pragma endregion

#pragma region アイテム処理関数


void moveItem(void)
{
    if (item.state == 0) return;
    item.x += item.vx;
    item.y += item.vy;
    if (item.timer % 60 < 30)
        item.vx -= 1;
    else
        item.vx += 1;
    if (item.y > HEIGHT + 16) item.state = 0;
    item.pattern = (item.timer / 120) % ITEM_TYPE; // 現在、どのアイテムになっているか
    item.timer++;
    DrawRectGraph(item.x - 20, item.y - 16, item.pattern * 40, 0, 40, 32, imgItem, TRUE, FALSE);
    if (isCurrentScene(sceneOver)) return; // ゲームオーバー画面では回収できない
    int dis = (item.x - player.x) * (item.x - player.x) + (item.y - player.y) * (item.y - player.y);
    if (dis < 60 * 60) // アイテムと自機とのヒットチェック（円による当たり判定）
    {
        item.state = 0;
        if (item.pattern == 0) // スピードアップ
        {
            if (player.vx < PLAYER_SPEED_MAX)
            {
                player.vx += 3;
                player.vy += 3;
            }
        }
        if (item.pattern == 1) // シールド回復
        {
            if (player.shield < PLAYER_SHIELD_MAX) player.shield++;
            setEffect(player.x, player.y, EFF_RECOVER); // 回復エフェクトを表示
        }
        if (item.pattern == 2) // 武器レベルアップ
        {
            if (weaponLv < WEAPON_LV_MAX) weaponLv++;
        }
        PlaySoundMem(seItem, DX_PLAYTYPE_BACK); // 効果音
    }
}
#pragma endregion



// ==== 画像・文字列表示所為 ====

#pragma region 画像表示関数

// 中心座標を指定して画像を表示する関数
void drawImage(int img, int x, int y)
{
    int w, h;
    GetGraphSize(img, &w, &h);
    DrawGraph(x - w / 2, y - h / 2, img, TRUE);
}
#pragma endregion

#pragma region 影付き文字列を表示する関数

// 影を付けた文字列と値を表示する関数
void drawText(int x, int y, const char* txt, int val, int col, int siz)
{
    SetFontSize(siz); // フォントの大きさを指定
    DrawFormatString(x + 1, y + 1, 0x000000, txt, val); // 黒で文字列を表示
    DrawFormatString(x, y, col, txt, val); // 引数の色で文字列を表示
}
#pragma endregion

#pragma region プレイヤーパラメータ表示関数

// 自機に関するパラメーターを表示
void drawParameter(void)
{
    int x = 10, y = HEIGHT - 30; // 表示位置
    DrawBox(x, y, x + PLAYER_SHIELD_MAX * 30, y + 20, 0x000000, TRUE);
    for (int i = 0; i < player.shield; i++) // シールドのメーター
    {
        int r = 128 * (PLAYER_SHIELD_MAX - i) / PLAYER_SHIELD_MAX; // RGB値を計算
        int g = 255 * i / PLAYER_SHIELD_MAX;
        int b = 160 + 96 * i / PLAYER_SHIELD_MAX;
        DrawBox(x + 2 + i * 30, y + 2, x + 28 + i * 30, y + 18, GetColor(r, g, b), TRUE);
    }
    drawText(x, y - 25, "HP : %02d", player.shield, 0xffffff, 20); // シールド値
    drawText(x, y - 50, "攻撃力 Lv %02d", weaponLv, 0xffffff, 20); // 武器レベル
    drawText(x, y - 75, "スピード Lv %02d", player.vx, 0xffffff, 20); // 移動速度
}
#pragma endregion


#pragma region 文字列表示

// 文字列をセンタリングして表示する関数
void drawTextC(int x, int y, const char* txt, int col, int siz)
{
    SetFontSize(siz);
    int strWidth = GetDrawStringWidth(txt, strlen(txt));
    x -= strWidth / 2;
    y -= siz / 2;
    DrawString(x + 1, y + 1, txt, 0x000000);
    DrawString(x, y, txt, col);
}
#pragma endregion

#pragma region 色変化関数
//Imageの色彩を変化させ、マゼンタカラーを透過する関数

void changeImageColor(const char* filePath, int& imgHandle, int r, int g, int b) {
    // ソフトイメージとして画像をロード
    int softImageHandle = LoadSoftImage(filePath);
    if (softImageHandle == -1) {
        printfDx("ソフトイメージのロードに失敗しました\n");
        return;
    }

    // ソフトイメージのサイズを取得
    int imgWidth, imgHeight;
    GetSoftImageSize(softImageHandle, &imgWidth, &imgHeight);

    // ピクセルごとに色を変更
    for (int y = 0; y < imgHeight; y++) {
        for (int x = 0; x < imgWidth; x++) {
            int a, r_, g_, b_;
            // ピクセルの色を取得
            if (GetPixelSoftImage(softImageHandle, x, y, &r_, &g_, &b_, &a) == -1) {
                printfDx("ピクセルデータの取得に失敗しました\n");
                continue;
            }

            // マゼンタカラーの場合は透過処理
            if (r_ == 255 && g_ == 0 && b_ == 255) {
                a = 0; // α値を0に設定（完全透明）
            }
            else {
                // 色を変更 (指定された倍率で調整)
                r_ = (r_ * r) / 255;
                g_ = (g_ * g) / 255;
                b_ = (b_ * b) / 255;
            }

            // 変更後の色を設定
            if (DrawPixelSoftImage(softImageHandle, x, y, r_, g_, b_, a) == -1) {
                printfDx("ピクセルデータの書き込みに失敗しました\n");
            }
        }
    }

    // ソフトイメージを新しい画像に変換
    int newImgHandle = CreateGraphFromSoftImage(softImageHandle);
    if (newImgHandle == -1) {
        printfDx("ソフトイメージから画像への変換に失敗しました\n");
        DeleteSoftImage(softImageHandle);
        return;
    }

    // 元の画像ハンドルを削除し、新しいハンドルに置き換え
    DeleteGraph(imgHandle);
    imgHandle = newImgHandle;

    // ソフトイメージを解放
    DeleteSoftImage(softImageHandle);
}
#pragma endregion



// ==== 入力フレーム管理・音声認識処理 ====



#pragma region 音声認識実行関数


void runPythonScriptAsync(void) 
{
    
    isRunning = true;  // 実行中フラグを設定
    std::thread([]
        {
            STARTUPINFOW si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;  // コマンドプロンプトを非表示
            ZeroMemory(&pi, sizeof(pi));

            std::wstring command = L"Pvoice.exe";
            if (CreateProcessW(NULL, &command[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) 
            {
                // スクリプトが終了するのを待つ
                WaitForSingleObject(pi.hProcess, INFINITE);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }

            // 認識結果を読み取る
            std::ifstream file("voice_output.txt");
            std::string result;
            if (file.is_open()) 
            {
                std::getline(file, result);
                file.close();
            }
            else 
            {
                result = "エラー: 結果ファイルが見つかりません";
            }

            // 結果をスレッド間で共有
            {
                std::lock_guard<std::mutex> lock(resultMutex);
                recognitionResult = result;
            }

            isRunning = false;  // 実行完了フラグを解除
        }).detach();  // スレッドをデタッチ
}
#pragma endregion


#pragma region 音声認識実行後処理関数

// 前後空白・改行除去
std::string trim(const std::string& str) {
    const char* ws = " \t\n\r";
    size_t start = str.find_first_not_of(ws);
    size_t end = str.find_last_not_of(ws);
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

// 認識語やスキル名を正規化
std::string normalizeModeName(const std::string& word) {
    std::string w = trim(word);
    // ひらがな対応も入れたければ下記追加
    if (w == "連射" || w == "連写" || w == "れんしゃ" || w == "電車") return "連射";
    if (w == "追尾" || w == "ついび") return "追尾";
    if (w == "拡散" || w == "かくさん") return "拡散";
    if (w == "ブラックホール" || w == "ぶらっくほーる") return "ブラックホール";
    return w;
}

// skilltree.csvの1列目リストからアンロック済みのモードを取得
std::vector<std::string> GetUnlockedModes(const std::string& skilltreePath = "skilltree.csv") {
    std::vector<std::string> unlocked;
    std::ifstream ifs(skilltreePath);
    std::string line;
    int y = 120; // 画面出力用Y座標
    while (std::getline(ifs, line)) {
        std::vector<std::string> cols;
        std::stringstream ss(line);
        std::string col;
        // タブ区切りで全部配列に格納
        while (std::getline(ss, col, ',')) {
            cols.push_back(col);
        }
        //// 画面に列数・内容を表示
        //std::string msg = std::to_string((int)cols.size()) + "列: ";
        //for (auto& v : cols) msg += "[" + v + "]";
        //drawTextC(800, y, msg.c_str(), 0xff0000, 24);
        //y += 32;

        // 5列目が1なら解放済み
        if (cols.size() >= 5 && cols[4] == "1") {
            unlocked.push_back(cols[0]);
        }
    }
    return unlocked;
}


// 音声認識を使用する用の関数
void SpeechRecognize(void)
{
    static int voiceInputFrame = 0;

    if ((Key[KEY_INPUT_Q] == 1 || Key[KEY_INPUT_P] == 1) && !isRunning) {
        isRunning = true;
        voiceInputFrame = 0;
        runPythonScriptAsync();
    }

    if (!isRunning) {
        voiceInputFrame++;
        if (voiceInputFrame > 300) { // 10フレームは受け付ける
            isRunning = false; // ここで「受付終了」にするなど
        }
        std::string resultCopy;
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            resultCopy = recognitionResult;
        }
        drawTextC(WIDTH / 2, 60, resultCopy.c_str(), 0x00aaff, 32);

        if (!resultCopy.empty()) {
            std::string recognized = normalizeModeName(resultCopy);

            struct ModeMap {
                const char* name;
                int modeType;
                int effType;
                int colR, colG, colB;
                const char* label;
            };
            const ModeMap modeMaps[] = {
                {"拡散", EXPLOSION, EFF_MODE_EXPLOSION, 500, 300, 30, "EXPLOSION"},
                {"ブラックホール", BLACKHALL, EFF_MODE_BLACKHALL, 30, 500, 300, "BLACKHALL"},
                {"追尾", CHASE, EFF_MODE_CHASE, 55, 355, 500, "CHASE"},
                {"連射", RAPIDFIRE, EFF_MODE_RAPIDFIRE, 300, 300, 300, "RAPIDFIRE"},
            };

            auto unlockedModes = GetUnlockedModes();
            bool found = false;

            //// --- アンロック済みモードリストを表示（デバッグ用） ---
            //int debugY = 100;
            //drawTextC(180, debugY, "アンロック済み弾モード", 0x00bb77, 24);
            //for (size_t i = 0; i < unlockedModes.size(); ++i) {
            //    drawTextC(180, debugY + 28 + 24 * i, unlockedModes[i].c_str(), 0x0077ff, 22);
            //}

            for (const auto& map : modeMaps) {
                if (recognized == map.name) {
                    // 解放済みならOK
                    if (std::find(unlockedModes.begin(), unlockedModes.end(), map.name) != unlockedModes.end()) {
                        mode = map.modeType;
                        setEffect(player.x, player.y, map.effType);
                        changeImageColor("image/bullet.png", imgBullet, map.colR, map.colG, map.colB);
                        drawText(10, 60, map.label, score, 0xffffff, 30);
                         attackBuffTimer = FPS * 10;
                         attackBuffMultiplier = 2.0f;
                        found = true;
                    }
                    else if (!unlockedModes.empty()) {
                        // 解放済みのどれか（優先度：先頭）で強制切り替え＋ペナルティ
                        for (const auto& fallback : modeMaps) {
                            if (std::find(unlockedModes.begin(), unlockedModes.end(), fallback.name) != unlockedModes.end()) {
                                mode = fallback.modeType;
                                setEffect(player.x, player.y, fallback.effType);
                                changeImageColor("image/bullet.png", imgBullet, fallback.colR, fallback.colG, fallback.colB);
                                drawText(10, 60, fallback.label, score, 0xffffff, 30);
                                attackBuffTimer = FPS * 10;
                                attackBuffMultiplier = 0.5f;
                                drawTextC(WIDTH / 2, HEIGHT - 120, "未解放なので他の弾モードへ", 0xff4444, 28);
                                drawTextC(WIDTH / 2, HEIGHT - 80, "ペナルティ：10秒間攻撃半減", 0xff2222, 40);
                                found = true;
                                break;
                            }
                        }
                    }
                    else {
                        // 全部未解放
                        drawTextC(WIDTH / 2, HEIGHT - 120, "このスキルは未解放です", 0xff4444, 28);
                        attackBuffTimer = FPS * 10;
                        attackBuffMultiplier = 0.5f;
                        drawTextC(WIDTH / 2, HEIGHT - 80, "ペナルティ：10秒間攻撃半減", 0xff2222, 40);
                        found = true;
                    }
                    break;
                }
            }

            // どれにも当てはまらなければNORMAL
            if (!found) {
                mode = NORMAL;
                changeImageColor("image/bullet.png", imgBullet, 255, 255, 255);
                drawText(10, 60, "NORMAL", score, 0xffffff, 30);
            }
        }
    }
}

#pragma endregion




