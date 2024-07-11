#include <stdlib.h>

#include "raylib.h"

#include "common.h"


#define TILE_SIZE 100
#define TILE_COUNT_X 4
#define TILE_COUNT_Y 4
#define TILE_COUNT (TILE_COUNT_X * TILE_COUNT_Y)
#define SPACING 15.0f
#define PADDING 20.0f
#define SCORE_DISPLAY_HEIGHT 60.0f
#define SCORE_DISPLAY_MARGIN 15.0f
#define TEXT_SIZE (TILE_SIZE * 0.7f)
#define WINDOW_WIDTH (TILE_COUNT_X * TILE_SIZE + (TILE_COUNT_X + 1) * SPACING + 2 * PADDING)
#define WINDOW_HEIGHT (TILE_COUNT_Y * TILE_SIZE + (TILE_COUNT_Y + 1) * SPACING + 3 * PADDING + SCORE_DISPLAY_HEIGHT)


const Color COLOUR_BACKGROUND = {.r = 250, .g = 248, .b = 239, .a = 255};
const Color COLOUR_BOARD_BACKGROUND = {.r = 187, .g = 173, .b = 160, .a = 255};
const Color COLOUR_TEXT = {.r = 119, .g = 110, .b = 101, .a = 255};
const Color COLOUR_TEXT_ALT = {.r = 249, .g = 246, .b = 242, .a = 255};
const Color COLOUR_TEXT_DISPLAY = {.r = 238, .g = 228, .b = 218, .a = 255};
const Color COLOUR_TILES[] = {
    {.r = 205, .g = 193, .b = 180, .a = 255}, // Empty
    {.r = 238, .g = 228, .b = 218, .a = 255}, // 2
    {.r = 238, .g = 225, .b = 201, .a = 255}, // 4
    {.r = 243, .g = 178, .b = 122, .a = 255}, // 8
    {.r = 246, .g = 150, .b = 100, .a = 255}, // 16
    {.r = 247, .g = 124, .b = 95,  .a = 255}, // 32
    {.r = 247, .g = 95,  .b = 59,  .a = 255}, // 64
    {.r = 234, .g = 207, .b = 118, .a = 255}, // 128
    {.r = 237, .g = 203, .b = 103, .a = 255}, // 256
    {.r = 236, .g = 200, .b = 90,  .a = 255}, // 512
    {.r = 231, .g = 194, .b = 87,  .a = 255}, // 1024
    {.r = 232, .g = 190, .b = 78,  .a = 255}, // 2048
    {.r = 60,  .g = 58,  .b = 50,  .a = 255}  //...
};


static u32 PowerOf2(i32 exponent) {
    if (exponent < 0 || exponent > 32) {
        return 0;
    }

    u32 result = 1;
    while (exponent--) {
        result *= 2;
    }
    return result;
}

static bool IsBoardFull(i32 *board) {
    for (i32 i = 0; i < TILE_COUNT; ++i) {
        if (board[i] == 0) {
            return false;
        }
    }
    return true;
}

static i32 GetRandomFreeTile(i32 *board) {
    if (IsBoardFull(board, TILE_COUNT)) {
        return -1;
    }

    i32 index;
    do {
        index = GetRandomValue(0, TILE_COUNT - 1);
    } while (board[index] != 0);
    return index;
}

static bool CanMove(i32 *board) {
    if (!IsBoardFull(board)) {
        return true;
    }

    for (i32 y = 0; y < TILE_COUNT_Y; ++y) {
        for (i32 x = 0; x < TILE_COUNT_X; ++x) {
            i32 index = y * TILE_COUNT_X + x;
            if ((y > 0 && board[index] == board[index - TILE_COUNT_X]) || (y < TILE_COUNT_Y - 1 && board[index] == board[index + TILE_COUNT_X]) ||
                (x > 0 && board[index] == board[index - 1]) || (x < TILE_COUNT_X - 1 && board[index] == board[index + 1])) {
                return true;
            }
        }
    }
    return false;
}

static bool HandleMovement_Up(i32 index) {
    return index >= TILE_COUNT_X;
}

static bool HandleMovement_Down(i32 index) {
    return index < TILE_COUNT - TILE_COUNT_X;
}

static bool HandleMovement_Left(i32 index) {
    return index % TILE_COUNT_X != 0;
}

static bool HandleMovement_Right(i32 index) {
    return index % TILE_COUNT_X != (TILE_COUNT_X - 1);
}

static void HandleMovement(i32 *board, i32 index, i32 offset, bool (*condition)(i32), bool *didMove, i32 *score) {
    while ((*condition)(index) && board[index] != 0 && (board[index + offset] == 0 || board[index + offset] == board[index])) {
        if (board[index + offset] > 0) {
            *score += PowerOf2(board[index + offset] + 1);
            board[index + offset] = -(board[index + offset] + 1);
            board[index] = 0;
            break;
        }

        board[index + offset] = board[index];
        board[index] = 0;
        index += offset;
        *didMove = true;
    }
}

static bool Move(i32 *board, i32 *score) {
    bool didMove = false;

    if (IsKeyPressed(KEY_UP)) {
        for (i32 y = 1; y < TILE_COUNT_Y; ++y) {
            for (i32 x = 0; x < TILE_COUNT_X; ++x) {
                i32 index = y * TILE_COUNT_X + x;
                HandleMovement(board, index, -TILE_COUNT_X, &HandleMovement_Up, &didMove, score);
            }
        }
    } else if (IsKeyPressed(KEY_DOWN)) {
        for (i32 y = TILE_COUNT_Y - 2; y >= 0; --y) {
            for (i32 x = 0; x < TILE_COUNT_X; ++x) {
                i32 index = y * TILE_COUNT_X + x;
                HandleMovement(board, index, TILE_COUNT_X, &HandleMovement_Down, &didMove, score);
            }
        }
    } else if (IsKeyPressed(KEY_RIGHT)) {
        for (i32 x = TILE_COUNT_X - 2; x >= 0; --x) {
            for (i32 y = 0; y < TILE_COUNT_Y; ++y) {
                i32 index = y * TILE_COUNT_X + x;
                HandleMovement(board, index, 1, &HandleMovement_Right, &didMove, score);
            }
        }
    } else if (IsKeyPressed(KEY_LEFT)) {
        for (i32 x = 1; x < TILE_COUNT_X; ++x) {
            for (i32 y = 0; y < TILE_COUNT_Y; ++y) {
                i32 index = y * TILE_COUNT_X + x;
                HandleMovement(board, index, -1, &HandleMovement_Left, &didMove, score);
            }
        }
    } 

    for (i32 i = 0; i < TILE_COUNT; ++i) {
        if (board[i] < 0) {
            board[i] *= -1;
        }
    }

    return didMove;
}

static void DisplayScores(Font font, i32 score, i32 highscore) {
    const f32 displaysSpacing = 12.0f;
    const f32 numberHeight = 35.0f;
    const f32 textHeight = 20.0f;
    const f32 minWidth = 70.0f;

    const char *highscoreStr = TextFormat("%d", highscore);
    f32 highscoreStrWidth = MeasureTextEx(font, highscoreStr, numberHeight, 0.0f).x;
    Rectangle highscoreDisplay = { .width = highscoreStrWidth + 2 * SCORE_DISPLAY_MARGIN};
    if (highscoreDisplay.width < minWidth) {
        highscoreDisplay.width = minWidth;
    }
    highscoreDisplay.x = WINDOW_WIDTH - PADDING - highscoreDisplay.width;
    highscoreDisplay.y = PADDING;
    highscoreDisplay.height = SCORE_DISPLAY_HEIGHT;
    Vector2 highscoreStrPos = {highscoreDisplay.x + highscoreDisplay.width / 2 - highscoreStrWidth / 2, PADDING + SCORE_DISPLAY_HEIGHT - numberHeight - 3.0f};

    f32 highscoreLabelWidth = MeasureTextEx(font, "BEST", textHeight, 0.0f).x;

    const char *scoreStr = TextFormat("%d", score);
    f32 scoreStrWidth = MeasureTextEx(font, scoreStr, numberHeight, 0.0f).x;
    Rectangle scoreDisplay = { .width = scoreStrWidth + 2 * SCORE_DISPLAY_MARGIN};
    if (scoreDisplay.width < minWidth) {
        scoreDisplay.width = minWidth;
    }
    scoreDisplay.x = highscoreDisplay.x - displaysSpacing - scoreDisplay.width;
    scoreDisplay.y = PADDING;
    scoreDisplay.height = SCORE_DISPLAY_HEIGHT;
    Vector2 scoreStrPos = {scoreDisplay.x + scoreDisplay.width / 2 - scoreStrWidth / 2, highscoreStrPos.y};

    f32 scoreLabelWidth = MeasureTextEx(font, "SCORE", textHeight, 0.0f).x;

    DrawRectangleRounded(scoreDisplay, 0.15f, 3, COLOUR_BOARD_BACKGROUND);
    DrawTextEx(font, scoreStr, scoreStrPos, numberHeight, 0.0f, COLOUR_TEXT_ALT);
    DrawTextEx(font, "SCORE", (Vector2) { scoreDisplay.x + scoreDisplay.width / 2 - scoreLabelWidth / 2, scoreStrPos.y - textHeight + 4.0f }, 
        textHeight, 0.0f, COLOUR_TEXT_DISPLAY);

    DrawRectangleRounded(highscoreDisplay, 0.15f, 3, COLOUR_BOARD_BACKGROUND);
    DrawTextEx(font, highscoreStr, highscoreStrPos, numberHeight, 0.0f, COLOUR_TEXT_ALT);
    DrawTextEx(font, "BEST", (Vector2) { highscoreDisplay.x + highscoreDisplay.width / 2 - highscoreLabelWidth / 2,
        highscoreStrPos.y - textHeight + 4.0f }, textHeight, 0.0f, COLOUR_TEXT_DISPLAY);
}

int main() {
    Rectangle boardBackground = {
        .x = PADDING,
        .y = 2 * PADDING + SCORE_DISPLAY_HEIGHT,
        .width = TILE_COUNT_X * TILE_SIZE + (TILE_COUNT_X + 1) * SPACING,
        .height = TILE_COUNT_Y * TILE_SIZE + (TILE_COUNT_Y + 1) * SPACING
    };

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "2048");
    SetTargetFPS(60);

    i32 board[TILE_COUNT] = {0};
    board[GetRandomFreeTile(board, TILE_COUNT)] = 1;
    board[GetRandomFreeTile(board, TILE_COUNT)] = 1;

    i32 score = 0;
    i32 highscore = 0;
    char *highscoreStr = LoadFileText("data/highscore.txt");
    if (highscoreStr) {
        highscore = atoi(highscoreStr);
    }

    Font font = LoadFontEx("assets/fonts/ClearSans-Bold.ttf", 2 * TEXT_SIZE, NULL, 0);

    while (!WindowShouldClose()) {
        // CONTINUE HERE! Sliding animation next?
        if (Move(board, &score)) {
            board[GetRandomFreeTile(board, TILE_COUNT)] = GetRandomValue(1, 2);
            if (IsBoardFull(board, TILE_COUNT) && !CanMove(board)) {
                board[0] = 13; // TODO: "Play again?"
                if (score > highscore) {
                    SaveFileText("data/highscore.txt", TextFormat("%d", score));
                }
            }
        }

        // Render

        BeginDrawing();

        ClearBackground(COLOUR_BACKGROUND);

        DrawRectangleRounded(boardBackground, 0.04f, 4, COLOUR_BOARD_BACKGROUND);

        f32 tileY = boardBackground.y + SPACING;
        for (i32 y = 0; y < TILE_COUNT_Y; ++y) {
            f32 tileX = boardBackground.x + SPACING;
            for (i32 x = 0; x < TILE_COUNT_X; ++x) {
                i32 tile = board[y * TILE_COUNT_X + x];

                DrawRectangle(tileX, tileY, TILE_SIZE, TILE_SIZE, COLOUR_TILES[tile <= 12 ? tile : 12]);

                if (tile != 0) {
                    u32 value = PowerOf2(tile);
                    Color colour = tile >= 3 ? COLOUR_TEXT_ALT : COLOUR_TEXT;
                    f32 size = tile <= 6 ? TEXT_SIZE : tile <= 9 ? TEXT_SIZE * 0.85f : tile <= 13 ? TEXT_SIZE * 0.65f : TEXT_SIZE * 0.55f;
                    const char *str = TextFormat("%d", value);
                    Vector2 strSize = MeasureTextEx(font, str, size, 0);
                    DrawTextEx(font, str, (Vector2) { tileX + TILE_SIZE / 2 - strSize.x / 2, tileY + TILE_SIZE / 2 - strSize.y / 2 }, size, 0, colour);
                }


                tileX += TILE_SIZE + SPACING;
            }
            tileY += TILE_SIZE + SPACING;
        }

        DisplayScores(font, score, highscore);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}