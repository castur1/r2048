#include <stdlib.h>

#include "raylib.h"

#include "common.h"


#define TILE_COUNT_X 4
#define TILE_COUNT_Y 4
#define TILE_COUNT (TILE_COUNT_X * TILE_COUNT_Y)

#define TILE_SIZE 100
#define TILE_SPACING 15.0f
#define BOARD_PADDING 20.0f

#define SCORE_DISPLAY_HEIGHT 60.0f
#define SCORE_DISPLAY_MARGIN 15.0f
#define SCORE_DISPLAY_SPACING 12.0f
#define SCORE_DISPLAY_NUMBER_HEIGHT 35.0f
#define SCORE_DISPLAY_TEXT_HEIGHT 20.0f
#define SCORE_DISPLAY_MIN_WIDTH 70.0f

#define TEXT_SIZE_TILE_0 (TILE_SIZE * 0.7f)
#define TEXT_SIZE_TILE_1 (TEXT_SIZE_TILE_0 * 0.85f)
#define TEXT_SIZE_TILE_2 (TEXT_SIZE_TILE_0 * 0.65f)
#define TEXT_SIZE_TILE_3 (TEXT_SIZE_TILE_0 * 0.55f)

#define WINDOW_WIDTH (TILE_COUNT_X * TILE_SIZE + (TILE_COUNT_X + 1) * TILE_SPACING + 2 * BOARD_PADDING)
#define WINDOW_HEIGHT (TILE_COUNT_Y * TILE_SIZE + (TILE_COUNT_Y + 1) * TILE_SPACING + 3 * BOARD_PADDING + SCORE_DISPLAY_HEIGHT)

#define TILE_MOVE_DURATION 0.15f

#define COLOUR_TILES_COUNT 13


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

const Rectangle BOARD_BACKGROUND = {
    .x = BOARD_PADDING,
    .y = 2 * BOARD_PADDING + SCORE_DISPLAY_HEIGHT,
    .width = TILE_COUNT_X * TILE_SIZE + (TILE_COUNT_X + 1) * TILE_SPACING,
    .height = TILE_COUNT_Y * TILE_SIZE + (TILE_COUNT_Y + 1) * TILE_SPACING
};


typedef struct Moving_tiles {
    i32 startIndices[TILE_COUNT];
    i32 endIndices[TILE_COUNT];
    i32 count;
    f32 timer;
} Moving_tiles;

typedef struct Board {
    i32 board[TILE_COUNT];
    Moving_tiles movingTiles;
    bool combinedTiles[TILE_COUNT];
    i32 newTile;
} Board;


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

static bool IsTileMoving(i32 index, Moving_tiles *tiles) {
    for (i32 i = 0; i < tiles->count; ++i) {
        if (tiles->endIndices[i] == index) {
            return true;
        }
    }

    return false;
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

static void HandleMovement(Board *board, i32 index, i32 offset, bool (*condition)(i32), bool *didMove, i32 *score) {
    board->movingTiles.startIndices[board->movingTiles.count] = index;

    while ((*condition)(index) && board->board[index] != 0 && !board->combinedTiles[index + offset] && 
        (board->board[index + offset] == 0 || board->board[index + offset] == board->board[index])) {
        *didMove = true;
        index += offset;

        if (board->board[index] > 0) {
            *score += PowerOf2(board->board[index] + 1);
            board->board[index] += 1;
            board->board[index - offset] = 0;

            board->combinedTiles[index] = true;

            break;
        }

        board->board[index] = board->board[index - offset];
        board->board[index - offset] = 0;
    }

    if (board->movingTiles.startIndices[board->movingTiles.count] != index) {
        board->movingTiles.endIndices[board->movingTiles.count] = index;
        ++board->movingTiles.count;
    }
}

static void MoveUp(Board *board, bool *didMove, i32 *score) {
    for (i32 y = 1; y < TILE_COUNT_Y; ++y) {
        for (i32 x = 0; x < TILE_COUNT_X; ++x) {
            i32 index = y * TILE_COUNT_X + x;
            HandleMovement(board, index, -TILE_COUNT_X, &HandleMovement_Up, didMove, score);
        }
    }
}

static void MoveDown(Board *board, bool *didMove, i32 *score) {
    for (i32 y = TILE_COUNT_Y - 2; y >= 0; --y) {
        for (i32 x = 0; x < TILE_COUNT_X; ++x) {
            i32 index = y * TILE_COUNT_X + x;
            HandleMovement(board, index, TILE_COUNT_X, &HandleMovement_Down, didMove, score);
        }
    }
}

static void MoveLeft(Board *board, bool *didMove, i32 *score) {
    for (i32 x = 1; x < TILE_COUNT_X; ++x) {
        for (i32 y = 0; y < TILE_COUNT_Y; ++y) {
            i32 index = y * TILE_COUNT_X + x;
            HandleMovement(board, index, -1, &HandleMovement_Left, didMove, score);
        }
    }
}

static void MoveRight(Board *board, bool *didMove, i32 *score) {
    for (i32 x = TILE_COUNT_X - 2; x >= 0; --x) {
        for (i32 y = 0; y < TILE_COUNT_Y; ++y) {
            i32 index = y * TILE_COUNT_X + x;
            HandleMovement(board, index, 1, &HandleMovement_Right, didMove, score);
        }
    }
}

static bool Move(Board *board, i32 *score) {
    bool didMove = false;

    Board newBoard = {.movingTiles.timer = TILE_MOVE_DURATION};
    for (i32 i = 0; i < TILE_COUNT; ++i) {
        newBoard.board[i] = board->board[i];
    }

    if (IsKeyPressed(KEY_UP)) {
        MoveUp(&newBoard, &didMove, score);
    } else if (IsKeyPressed(KEY_DOWN)) {
        MoveDown(&newBoard, &didMove, score);
    } else if (IsKeyPressed(KEY_LEFT)) {
        MoveLeft(&newBoard, &didMove, score);
    } else if (IsKeyPressed(KEY_RIGHT)) {
        MoveRight(&newBoard, &didMove, score);
    } 

    if (didMove) {
        *board = newBoard;
    }

    return didMove;
}

static DrawTileNumber(i32 tile, f32 tileX, f32 tileY, Font font) {
    u32 value = PowerOf2(tile);
    Color colour = tile > 2 ? COLOUR_TEXT_ALT : COLOUR_TEXT;
    f32 size = 
        tile < 7 ? // 2 digits
            TEXT_SIZE_TILE_0 : 
            tile < 10 ? // 3 digits
                TEXT_SIZE_TILE_1 : 
                tile < 14 ? // 4 digits
                    TEXT_SIZE_TILE_2 : 
                    TEXT_SIZE_TILE_3; // 5+ digits

    const char *str = TextFormat("%d", value);
    Vector2 strSize = MeasureTextEx(font, str, size, 0);
    Vector2 strPos = {
        .x = tileX + TILE_SIZE / 2 - strSize.x / 2, 
        .y = tileY + TILE_SIZE / 2 - strSize.y / 2
    };

    DrawTextEx(font, str, strPos, size, 0, colour);
}

static void DisplayBoard(Board *board, Font font) {
    DrawRectangleRounded(BOARD_BACKGROUND, 0.04f, 4, COLOUR_BOARD_BACKGROUND);

    f32 tileY = BOARD_BACKGROUND.y + TILE_SPACING;
    for (i32 y = 0; y < TILE_COUNT_Y; ++y) {
        f32 tileX = BOARD_BACKGROUND.x + TILE_SPACING;
        for (i32 x = 0; x < TILE_COUNT_X; ++x) {
            i32 tileIndex = y * TILE_COUNT_X + x;
            if (IsTileMoving(tileIndex, &board->movingTiles) || tileIndex == board->newTile) {
                DrawRectangle(tileX, tileY, TILE_SIZE, TILE_SIZE, COLOUR_TILES[0]);
                tileX += TILE_SIZE + TILE_SPACING;
                continue;
            }

            i32 tile = board->board[tileIndex];

            DrawRectangle(tileX, tileY, TILE_SIZE, TILE_SIZE, COLOUR_TILES[tile < COLOUR_TILES_COUNT ? tile : (COLOUR_TILES_COUNT - 1)]);

            if (tile != 0) {
                DrawTileNumber(tile, tileX, tileY, font);
            }

            tileX += TILE_SIZE + TILE_SPACING;
        }
        tileY += TILE_SIZE + TILE_SPACING;
    }
}

static void DisplayMovingTiles(Board *board, Font font) {
    // CONTINUE HERE! Make them pop when combining. 
    f32 t = (TILE_MOVE_DURATION - board->movingTiles.timer) / TILE_MOVE_DURATION;
    for (i32 i = 0; i < board->movingTiles.count; ++i) {
        i32 startIndexX = board->movingTiles.startIndices[i] % TILE_COUNT_X;
        i32 startIndexY = board->movingTiles.startIndices[i] / TILE_COUNT_X;

        f32 startX = BOARD_BACKGROUND.x + TILE_SPACING + startIndexX * (TILE_SIZE + TILE_SPACING);
        f32 startY = BOARD_BACKGROUND.y + TILE_SPACING + startIndexY * (TILE_SIZE + TILE_SPACING);

        i32 endIndexX = board->movingTiles.endIndices[i] % TILE_COUNT_X;
        i32 endIndexY = board->movingTiles.endIndices[i] / TILE_COUNT_X;

        f32 endX = BOARD_BACKGROUND.x + TILE_SPACING + endIndexX * (TILE_SIZE + TILE_SPACING);
        f32 endY = BOARD_BACKGROUND.y + TILE_SPACING + endIndexY * (TILE_SIZE + TILE_SPACING);

        f32 tileX = startX + t * (endX - startX);
        f32 tileY = startY + t * (endY - startY);

        i32 tile = board->board[board->movingTiles.endIndices[i]];

        Color colour1 = COLOUR_TILES[MinI32(tile, COLOUR_TILES_COUNT - 1)];
        Color colour2 = colour1;

        bool shouldRender = false;
        if (board->combinedTiles[board->movingTiles.endIndices[i]]) {
            tile -= 1;

            colour1 = COLOUR_TILES[MinI32(tile, COLOUR_TILES_COUNT - 1)];

            shouldRender = true;
            for (i32 j = 0; j < board->movingTiles.count; ++j) {
                if (j != i && board->movingTiles.endIndices[j] == board->movingTiles.endIndices[i]) {
                    shouldRender = false;
                    break;
                }
            }
        }

        Color colour = {
            .r = colour1.r + t * (colour2.r - colour1.r),
            .g = colour1.g + t * (colour2.g - colour1.g),
            .b = colour1.b + t * (colour2.b - colour1.b),
            255
        };

        if (shouldRender) {
            DrawRectangle(endX, endY, TILE_SIZE, TILE_SIZE, colour);
            DrawTileNumber(tile, endX, endY, font);
        }

        DrawRectangle(tileX, tileY, TILE_SIZE, TILE_SIZE, colour);
        DrawTileNumber(tile, tileX, tileY, font);
    }
}

static void DisplayScores(Font font, i32 score, i32 highscore) {
    const char *highscoreStr = TextFormat("%d", highscore);

    f32 highscoreStrWidth = MeasureTextEx(font, highscoreStr, SCORE_DISPLAY_NUMBER_HEIGHT, 0.0f).x;

    Rectangle highscoreDisplay;
    highscoreDisplay.width = MaxF32(highscoreStrWidth + 2 * SCORE_DISPLAY_MARGIN, SCORE_DISPLAY_MIN_WIDTH);
    highscoreDisplay.height = SCORE_DISPLAY_HEIGHT;
    highscoreDisplay.x = WINDOW_WIDTH - BOARD_PADDING - highscoreDisplay.width;
    highscoreDisplay.y = BOARD_PADDING;

    Vector2 highscoreStrPos = {
        .x = highscoreDisplay.x + highscoreDisplay.width / 2 - highscoreStrWidth / 2, 
        .y = BOARD_PADDING + SCORE_DISPLAY_HEIGHT - SCORE_DISPLAY_NUMBER_HEIGHT - 3.0f
    };

    f32 highscoreLabelWidth = MeasureTextEx(font, "BEST", SCORE_DISPLAY_TEXT_HEIGHT, 0.0f).x;
    Vector2 highscoreLabelPos = {
        .x = highscoreDisplay.x + highscoreDisplay.width / 2 - highscoreLabelWidth / 2,
        .y = highscoreStrPos.y - SCORE_DISPLAY_TEXT_HEIGHT + 4.0f
    };

    DrawRectangleRounded(highscoreDisplay, 0.15f, 3, COLOUR_BOARD_BACKGROUND);
    DrawTextEx(font, highscoreStr, highscoreStrPos, SCORE_DISPLAY_NUMBER_HEIGHT, 0.0f, COLOUR_TEXT_ALT);
    DrawTextEx(font, "BEST", highscoreLabelPos, SCORE_DISPLAY_TEXT_HEIGHT, 0.0f, COLOUR_TEXT_DISPLAY);

    const char *scoreStr = TextFormat("%d", score);

    f32 scoreStrWidth = MeasureTextEx(font, scoreStr, SCORE_DISPLAY_NUMBER_HEIGHT, 0.0f).x;

    Rectangle scoreDisplay;
    scoreDisplay.width = MaxF32(scoreStrWidth + 2 * SCORE_DISPLAY_MARGIN, SCORE_DISPLAY_MIN_WIDTH);
    scoreDisplay.height = SCORE_DISPLAY_HEIGHT;
    scoreDisplay.x = highscoreDisplay.x - SCORE_DISPLAY_SPACING - scoreDisplay.width;
    scoreDisplay.y = BOARD_PADDING;

    Vector2 scoreStrPos = {
        .x = scoreDisplay.x + scoreDisplay.width / 2 - scoreStrWidth / 2, 
        .y = highscoreStrPos.y
    };

    f32 scoreLabelWidth = MeasureTextEx(font, "SCORE", SCORE_DISPLAY_TEXT_HEIGHT, 0.0f).x;
    Vector2 scoreLabelPos = {
        .x = scoreDisplay.x + scoreDisplay.width / 2 - scoreLabelWidth / 2,
        .y = scoreStrPos.y - SCORE_DISPLAY_TEXT_HEIGHT + 4.0f
    };

    DrawRectangleRounded(scoreDisplay, 0.15f, 3, COLOUR_BOARD_BACKGROUND);
    DrawTextEx(font, scoreStr, scoreStrPos, SCORE_DISPLAY_NUMBER_HEIGHT, 0.0f, COLOUR_TEXT_ALT);
    DrawTextEx(font, "SCORE", scoreLabelPos, SCORE_DISPLAY_TEXT_HEIGHT, 0.0f, COLOUR_TEXT_DISPLAY);
}

int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "2048");
    SetTargetFPS(60);

    Font font = LoadFontEx("assets/fonts/ClearSans-Bold.ttf", 2 * TEXT_SIZE_TILE_0, NULL, 0);

    Board board = {0};
    board.board[GetRandomFreeTile(&board.board, TILE_COUNT)] = 1;
    board.board[GetRandomFreeTile(&board.board, TILE_COUNT)] = 1;

    i32 score = 0;
    i32 highscore = 0;
    char *highscoreStr = LoadFileText("data/highscore.txt");
    if (highscoreStr) {
        highscore = atoi(highscoreStr);
    }

    while (!WindowShouldClose()) {
        board.movingTiles.timer -= GetFrameTime();
        if (board.movingTiles.timer <= 0.0f) {
            board.movingTiles.timer = 0.0f;
            board.movingTiles.count = 0;
            board.newTile = -1;
        }

        if (Move(&board, &score)) {
            board.newTile = GetRandomFreeTile(board.board, TILE_COUNT);
            board.board[board.newTile] = GetRandomValue(1, 2);
            if (IsBoardFull(&board.board, TILE_COUNT) && !CanMove(&board.board)) {
                board.board[0] = 13; // TODO: "Play again?"
                if (score > highscore) {
                    SaveFileText("data/highscore.txt", TextFormat("%d", score));
                }
            }
        }

        // Render

        BeginDrawing();

        ClearBackground(COLOUR_BACKGROUND);

        DisplayBoard(&board, font);

        if (board.newTile != -1) {
            f32 t = (TILE_MOVE_DURATION - board.movingTiles.timer) / TILE_MOVE_DURATION;

            f32 size = TILE_SIZE * t;

            i32 indexX = board.newTile % TILE_COUNT_X;
            i32 indexY = board.newTile / TILE_COUNT_X;

            f32 x = BOARD_BACKGROUND.x + TILE_SPACING + indexX * (TILE_SIZE + TILE_SPACING) + TILE_SIZE / 2 - size / 2;
            f32 y = BOARD_BACKGROUND.y + TILE_SPACING + indexY * (TILE_SIZE + TILE_SPACING) + TILE_SIZE / 2 - size / 2;

            i32 tile = board.board[board.newTile];

            DrawRectangle(x, y, size, size, COLOUR_TILES[tile]);
        }

        DisplayMovingTiles(&board, font);

        DisplayScores(font, score, highscore);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}