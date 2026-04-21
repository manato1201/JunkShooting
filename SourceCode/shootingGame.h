#pragma once

// 構造体の宣言
struct OBJECT // 自機や敵機用
{
    int x;     // x座標
    int y;     // y座標
    int vx;    // x軸方向の速さ
    int vy;    // y軸方向の速さ
    int state;   // 存在するか
    int pattern; // 敵機の動きのパターン
    int image;   // 画像
    int wid;     // 画像の幅（ピクセル数）
    int hei;     // 画像の高さ
    int attack;  // 攻撃力
    int shield;  // シールド（耐久力）
    int timer;   // タイマー
};

struct Player : public OBJECT {
    int name;
    int attack;
    int resilience;
    int shield;
};

struct Enemy : public OBJECT {
    int boss3_phase;
    int boss3_targetX, boss3_targetY;
    int boss3_shot_timer;
};

struct Bullet : public OBJECT {
    int mode;
    int feature;
};


// 関数プロトタイプ宣言
// ここにプロトタイプ宣言を記述する




// ==== ゲーム初期化・共通処理 ====
void initGame(void);
void scrollBG(int spd);
void initVariable(void);
void drawImage(int img, int x, int y);
void drawText(int x, int y, const char* txt, int val, int col, int siz);
void drawTextC(int x, int y, const char* txt, int col, int siz);
void drawParameter(void);

// ==== プレイヤー・弾 ====
void movePlayer(void);
void setBullet(void);
void moveBullet(void);

// ==== 敵・ダメージ処理 ====
int setEnemy(int x, int y, int vx, int vy, int ptn, int img, int sld);
void moveEnemy(void);
void damageEnemy(OBJECT* enemyp, int dmg);
void stageMap(void);

// ==== エフェクト・アイテム ====
void setEffect(int x, int y, int ptn);
void drawEffect(void);
void setItem(void);
void moveItem(void);

// ==== 音声認識／画像処理 ====
void runPythonScriptAsync(void);
void SpeechRecognize(void);
void changeImageColor(const char* filePath, int& imgHandle, int r, int g, int b);

// ==== Scene ====
void sceneTitle(void);
void sceneSelect(void);

void sceneGraphic(void);
void sceneSkillTree(void);
void sceneExplanation(void);
void scenePlay(void);
void sceneOver(void);
void sceneClear(void);

// ==== スキルツリー ====
void ApplySkillTreeStatus(void);
void LoadSkillNamesAndUnlocks(void);
void SaveSkillTreeStatus(const char* filename);
void LoadSkillTreeStatus(const char* filename);
void UpdateAttackBuffTimer(void);
void SavePointsToCSV(void);
void LoadPointsFromCSV(void);


// ==== Graphic ====
void SaveCanvasAsPng(int canvas);
void HSVtoRGB(float h, float s, float v, int& r, int& g, int& b);
void FloodFill(int x, int y, unsigned int targetCol, unsigned int fillCol);
void DrawPenShape(int x, int y, int penSize, int penShape, int color);
