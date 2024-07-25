#include <stdio.h>
#include <stdlib.h>

#include "cJSON.h"
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

#define BUTTON_NEW_GAME_WIDTH 130.0f
#define BUTTON_NEW_GAME_HEIGHT 45.0f
#define BUTTON_NEW_GAME_TEXT_SIZE 28.0f

#define TEXT_SIZE_TILE_0 (TILE_SIZE * 0.7f)
#define TEXT_SIZE_TILE_1 (TEXT_SIZE_TILE_0 * 0.85f)
#define TEXT_SIZE_TILE_2 (TEXT_SIZE_TILE_0 * 0.65f)
#define TEXT_SIZE_TILE_3 (TEXT_SIZE_TILE_0 * 0.55f)

#define GAME_OVER_FADE_IN_DURATION 1.0f
#define TEXT_SIZE_GAME_OVER 80.0f
#define GAME_OVER_TEXT_OFFSET 150.0f
#define BUTTON_TRY_AGAIN_WIDTH 150.0f
#define BUTTON_TRY_AGAIN_HEIGHT 50.0f
#define BUTTON_TRY_AGAIN_OFFSET 250.0f
#define BUTTON_TRY_AGAIN_TEXT_SIZE 30.0f

#define BUTTONS_KEYBINDS_OFFSET_X 30.0f
#define BUTTONS_KEYBINDS_START_X (WINDOW_WIDTH / 2.0f)
#define BUTTONS_KEYBINDS_START_Y (BOARD_BACKGROUND.y + 15.0f)
#define BUTTONS_KEYBINDS_POSITION_DELTA ((TILE_SIZE + TILE_SPACING) / 2.0f)
#define BUTTONS_KEYBINDS_TEXT_SIZE 30.0f
#define BUTTONS_KEYBINDS_TEXT_MARGIN 10.0f
#define BUTTONS_KEYBINDS_HEIGHT 43.0f

#define OPTIONS_TIMER_DURATION 0.2f
#define KEY_BINDINGS_COUNT 4

#define WINDOW_WIDTH (TILE_COUNT_X * TILE_SIZE + (TILE_COUNT_X + 1) * TILE_SPACING + 2 * BOARD_PADDING)
#define WINDOW_HEIGHT (TILE_COUNT_Y * TILE_SIZE + (TILE_COUNT_Y + 1) * TILE_SPACING + 3 * BOARD_PADDING + SCORE_DISPLAY_HEIGHT)

#define TILE_MOVE_DURATION 0.12f
#define TILE_COMBINE_DURATION 0.1f
#define TILE_COMBINE_DELTA_SIZE 20.0f

#define COLOUR_TILES_COUNT 13

#define FONT_SIZE 80.0f


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
const Color COLOUR_GAME_OVER_OVERLAY = {.r = 245, .g = 235, .b = 225, .a = 170};

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
    f32 combinedTimer;
    i32 newTile;
} Board;

typedef struct Button {
    Rectangle rectangle;
    Color colour;

    Font font;
    const char *text;
    Vector2 textPosition;
    f32 textSize;
    Color textColour;

    bool isActive;
} Button;

typedef struct Keybinds {
    union {
        struct {
            i32 up;
            i32 down;
            i32 left;
            i32 right;
        };
        i32 binds[KEY_BINDINGS_COUNT];
    };
} Keybinds;


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
    if (IsBoardFull(board)) {
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

static bool Move(Board *board, Keybinds *keybinds, i32 *score) {
    bool didMove = false;

    Board newBoard = {.movingTiles.timer = TILE_MOVE_DURATION};
    for (i32 i = 0; i < TILE_COUNT; ++i) {
        newBoard.board[i] = board->board[i];
    }

    if (IsKeyPressed(keybinds->up)) {
        MoveUp(&newBoard, &didMove, score);
    } else if (IsKeyPressed(keybinds->down)) {
        MoveDown(&newBoard, &didMove, score);
    } else if (IsKeyPressed(keybinds->left)) {
        MoveLeft(&newBoard, &didMove, score);
    } else if (IsKeyPressed(keybinds->right)) {
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

static void DisplayNewTile(Board *board) {
    f32 t = (TILE_MOVE_DURATION - board->movingTiles.timer) / TILE_MOVE_DURATION;

    f32 size = TILE_SIZE * t;

    i32 indexX = board->newTile % TILE_COUNT_X;
    i32 indexY = board->newTile / TILE_COUNT_X;

    f32 x = BOARD_BACKGROUND.x + TILE_SPACING + indexX * (TILE_SIZE + TILE_SPACING) + TILE_SIZE / 2 - size / 2;
    f32 y = BOARD_BACKGROUND.y + TILE_SPACING + indexY * (TILE_SIZE + TILE_SPACING) + TILE_SIZE / 2 - size / 2;

    i32 tile = board->board[board->newTile];

    DrawRectangle(x, y, size, size, COLOUR_TILES[tile]);
}

static void DisplayMovingTiles(Board *board, Font font) {
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

static Vector2 GetTextPositionCentred(Rectangle rect, Font font, const char *text, f32 textSize) {
    Vector2 textDimensions = MeasureTextEx(font, text, textSize, 0.0f);
    return (Vector2) {
        .x = rect.x + rect.width / 2 - textDimensions.x / 2,
        .y = rect.y + rect.height / 2 - textDimensions.y / 2
    };
}

static void DisplayGameOver(Font font, f32 timer, Button *buttonTryAgain) {
    f32 t = (GAME_OVER_FADE_IN_DURATION - timer) / GAME_OVER_FADE_IN_DURATION;

    Color colourOverlay = COLOUR_GAME_OVER_OVERLAY;
    colourOverlay.a *= t;
    Color colourText = COLOUR_TEXT;
    colourText.a *= t;
    Color colourButton = buttonTryAgain->colour;
    colourButton.a *= t;
    Color colourButtonText = buttonTryAgain->textColour;
    colourButtonText.a *= t;

    DrawRectangleRounded(BOARD_BACKGROUND, 0.04f, 4, colourOverlay);

    const char *str = "Game Over";
    Vector2 strSize = MeasureTextEx(font, str, TEXT_SIZE_GAME_OVER, 0.0f);
    Vector2 strPos = {
        .x = BOARD_BACKGROUND.x + BOARD_BACKGROUND.width / 2 - strSize.x / 2,
        .y = BOARD_BACKGROUND.y + GAME_OVER_TEXT_OFFSET
    };
    DrawTextEx(font, str, strPos, TEXT_SIZE_GAME_OVER, 0.0f, colourText);

    DrawRectangleRounded(buttonTryAgain->rectangle, 0.2f, 4, colourButton);
    DrawTextEx(buttonTryAgain->font, buttonTryAgain->text, buttonTryAgain->textPosition, buttonTryAgain->textSize, 0.0f, colourButtonText);
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

static void DisplayButtons(Button *newGame, Button *options, Texture2D *optionsSymbol, f32 optionsTimer) {
    DrawRectangleRounded(newGame->rectangle, 0.3f, 4, newGame->colour);
    DrawTextEx(newGame->font, newGame->text, newGame->textPosition, newGame->textSize, 0.0f, newGame->textColour);

    DrawRectangleRounded(options->rectangle, 0.3f, 4, options->colour);

    if (optionsTimer == 0.0f || optionsTimer == OPTIONS_TIMER_DURATION) {
        DrawTexture(*optionsSymbol, options->rectangle.x + options->rectangle.width / 2.0f - optionsSymbol->width / 2.0f, 
            options->rectangle.y + options->rectangle.height / 2.0f - optionsSymbol->height / 2.0f, WHITE);
    } else {
        f32 t = (OPTIONS_TIMER_DURATION - optionsTimer) / OPTIONS_TIMER_DURATION;
        f32 rotation = 60.0f * t;

        Rectangle symbolSrc = {
            .x = 0.0f,
            .y = 0.0f,
            .width = optionsSymbol->width,
            .height = optionsSymbol->height
        };
        Rectangle symbolDst = {
            .x = options->rectangle.x + options->rectangle.width / 2.0f,
            .y = options->rectangle.y + options->rectangle.height / 2.0f,
            .width = symbolSrc.width,
            .height = symbolSrc.height
        };
        Vector2 origin = {
            .x = symbolSrc.width / 2,
            .y = symbolSrc.height / 2
        };
        DrawTexturePro(*optionsSymbol, symbolSrc, symbolDst, origin, rotation, WHITE);
    }
}

static void DisplayCombinedTiles(Board *board, Font font) {
    f32 t = (TILE_COMBINE_DURATION - board->combinedTimer) / TILE_COMBINE_DURATION;
    t = -4.0f * t * (t - 1.0f);
    f32 deltaSize = TILE_COMBINE_DELTA_SIZE * t;
    for (i32 y = 0; y < TILE_COUNT_Y; ++y) {
        for (i32 x = 0; x < TILE_COUNT_X; ++x) {
            i32 index = y * TILE_COUNT_X + x;
            if (board->combinedTiles[index]) {
                i32 tile = board->board[index];

                f32 tileX = BOARD_BACKGROUND.x + TILE_SPACING + x * (TILE_SIZE + TILE_SPACING);
                f32 tileY = BOARD_BACKGROUND.y + TILE_SPACING + y * (TILE_SIZE + TILE_SPACING);

                DrawRectangle(tileX  - deltaSize / 2, tileY  - deltaSize / 2, TILE_SIZE + deltaSize, TILE_SIZE + deltaSize, COLOUR_TILES[tile]);

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
                size += deltaSize;

                const char *str = TextFormat("%d", value);
                Vector2 strSize = MeasureTextEx(font, str, size, 0);
                Vector2 strPos = {
                    .x = tileX + TILE_SIZE / 2 - strSize.x / 2, 
                    .y = tileY + TILE_SIZE / 2 - strSize.y / 2
                };

                DrawTextEx(font, str, strPos, size, 0, colour);
            }
        }
    }
}

static void DisplayOptions(Button *buttonsKeybinds, f32 optionsTimer, i32 buttonToBindIndex) {
    f32 t = (OPTIONS_TIMER_DURATION - optionsTimer) / OPTIONS_TIMER_DURATION;

    Color colourOverlay = COLOUR_GAME_OVER_OVERLAY;
    colourOverlay.a *= t;

    DrawRectangleRounded(BOARD_BACKGROUND, 0.04f, 4, colourOverlay);

    const char *labelsText[KEY_BINDINGS_COUNT] = {"Up", "Down", "Left", "Right"};
    for (i32 i = 0; i < KEY_BINDINGS_COUNT; ++i) {
        Color colour = buttonsKeybinds[i].colour;
        colour.a *= t;
        Color textColour = buttonsKeybinds[i].textColour;
        textColour.a *= t;

        if (i == buttonToBindIndex) {
            Vector2 textDimensions = MeasureTextEx(buttonsKeybinds[i].font, "[Press new key]", buttonsKeybinds[i].textSize, 0.0f);
            Rectangle rectangle = buttonsKeybinds[i].rectangle;
            rectangle.width = textDimensions.x + 2 * BUTTONS_KEYBINDS_TEXT_MARGIN;
            Vector2 textPosition = {
                .x = rectangle.x + rectangle.width / 2 - textDimensions.x / 2,
                .y = rectangle.y + rectangle.height / 2 - textDimensions.y / 2,
            };
            DrawRectangleRounded(rectangle, 0.3f, 4, colour);
            DrawTextEx(buttonsKeybinds[i].font, "[Press new key]", textPosition, buttonsKeybinds[i].textSize, 0.0f, textColour);
        } else {
            DrawRectangleRounded(buttonsKeybinds[i].rectangle, 0.3f, 4, colour);
            DrawTextEx(buttonsKeybinds[i].font, buttonsKeybinds[i].text, buttonsKeybinds[i].textPosition, buttonsKeybinds[i].textSize, 
                0.0f, textColour);
        }  

        Vector2 labelTextDimensions = MeasureTextEx(buttonsKeybinds[i].font, labelsText[i], BUTTONS_KEYBINDS_TEXT_SIZE, 0.0f);
        Vector2 labelTextPosition = {
            .x = BUTTONS_KEYBINDS_START_X - BUTTONS_KEYBINDS_OFFSET_X - labelTextDimensions.x,
            .y = buttonsKeybinds[i].rectangle.y
        };
        DrawTextEx(buttonsKeybinds[i].font, labelsText[i], labelTextPosition, BUTTONS_KEYBINDS_TEXT_SIZE, 0, colour);
    }
}

static cJSON *LoadJSON(const char *path) {
    const char *jsonStr = LoadFileText(path);
    if (jsonStr == NULL) {
        return NULL;
    }

    cJSON *json = cJSON_Parse(jsonStr);

    UnloadFileText(jsonStr);

    return json;
}

static i32 LoadHighscore(const char *path) {
    cJSON *json = LoadJSON(path);
    if (json == NULL) {
        return 0;
    }

    cJSON *player = cJSON_GetObjectItem(json, "player");
    i32 highscore = cJSON_GetObjectItem(player, "highscore")->valueint;

    free(json);

    return highscore;
}

static bool SaveHighscore(const char *path, i32 val) {
    cJSON *json = LoadJSON(path);
    if (json == NULL) {
        return false;
    }

    cJSON *player = cJSON_GetObjectItem(json, "player");
    cJSON *highscore = cJSON_GetObjectItem(player, "highscore");
    cJSON_SetIntValue(highscore, val);

    char *str = cJSON_Print(json);

    free(json);

    if (!SaveFileText(path, str)) {
        return false;
    }

    free(str);

    return true;
}

// TODO: More sophisticated button logic
static bool IsButtonPressed(Button *button) {
    if (!button->isActive) {
        return false;
    }

    Vector2 mousePos = GetMousePosition();
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, button->rectangle);
}

static const char *KeyCodeToString(i32 key) {
    switch (key) {
        case(KEY_APOSTROPHE):    return "'";            // '
        case(KEY_COMMA):         return ",";            // ,
        case(KEY_MINUS):         return "-";            // -
        case(KEY_PERIOD):        return ".";            // .
        case(KEY_SLASH):         return "/";            // /
        case(KEY_ZERO):          return "0";            // 0
        case(KEY_ONE):           return "1";            // 1
        case(KEY_TWO):           return "2";            // 2
        case(KEY_THREE):         return "3";            // 3
        case(KEY_FOUR):          return "4";            // 4
        case(KEY_FIVE):          return "5";            // 5
        case(KEY_SIX):           return "6";            // 6
        case(KEY_SEVEN):         return "7";            // 7
        case(KEY_EIGHT):         return "8";            // 8
        case(KEY_NINE):          return "9";            // 9
        case(KEY_SEMICOLON):     return ";";            // ;
        case(KEY_EQUAL):         return "=";            // =
        case(KEY_A):             return "A";            // A | a
        case(KEY_B):             return "B";            // B | b
        case(KEY_C):             return "C";            // C | c
        case(KEY_D):             return "D";            // D | d
        case(KEY_E):             return "E";            // E | e
        case(KEY_F):             return "F";            // F | f
        case(KEY_G):             return "G";            // G | g
        case(KEY_H):             return "H";            // H | h
        case(KEY_I):             return "I";            // I | i
        case(KEY_J):             return "J";            // J | j
        case(KEY_K):             return "K";            // K | k
        case(KEY_L):             return "L";            // L | l
        case(KEY_M):             return "M";            // M | m
        case(KEY_N):             return "N";            // N | n
        case(KEY_O):             return "O";            // O | o
        case(KEY_P):             return "P";            // P | p
        case(KEY_Q):             return "Q";            // Q | q
        case(KEY_R):             return "R";            // R | r
        case(KEY_S):             return "S";            // S | s
        case(KEY_T):             return "T";            // T | t
        case(KEY_U):             return "U";            // U | u
        case(KEY_V):             return "V";            // V | v
        case(KEY_W):             return "W";            // W | w
        case(KEY_X):             return "X";            // X | x
        case(KEY_Y):             return "Y";            // Y | y
        case(KEY_Z):             return "Z";            // Z | z
        case(KEY_LEFT_BRACKET):  return "[";            // [
        case(KEY_BACKSLASH):     return "\\";           // '\'
        case(KEY_RIGHT_BRACKET): return "]";            // ]
        case(KEY_GRAVE):         return "`";            // `
        case(161):               return "World 1";      // non-US
        case(162):               return "World 2";      // non-US
        case(KEY_SPACE):         return "Space";        // Space
        case(KEY_ENTER):         return "Enter";        // Enter
        case(KEY_TAB):           return "Tab";          // Tab
        case(KEY_BACKSPACE):     return "Backspace";    // Backspace
        case(KEY_INSERT):        return "Insert";       // Ins
        case(KEY_DELETE):        return "Delete";       // Del
        case(KEY_RIGHT):         return "Arrow right";  // Cursor right
        case(KEY_LEFT):          return "Arrow left";   // Cursor left
        case(KEY_DOWN):          return "Arrow down";   // Cursor down
        case(KEY_UP):            return "Arrow up";     // Cursor up
        case(KEY_PAGE_UP):       return "Page up";      // Page up
        case(KEY_PAGE_DOWN):     return "Page down";    // Page down
        case(KEY_HOME):          return "Home";         // Home
        case(KEY_END):           return "End";          // End
        case(KEY_CAPS_LOCK):     return "Caps lock";    // Caps lock
        case(KEY_SCROLL_LOCK):   return "Scroll lock";  // Scroll down
        case(KEY_F1):            return "F1";           // F1
        case(KEY_F2):            return "F2";           // F2
        case(KEY_F3):            return "F3";           // F3
        case(KEY_F4):            return "F4";           // F4
        case(KEY_F5):            return "F5";           // F5
        case(KEY_F6):            return "F6";           // F6
        case(KEY_F7):            return "F7";           // F7
        case(KEY_F8):            return "F8";           // F8
        case(KEY_F9):            return "F9";           // F9
        case(KEY_F10):           return "F10";          // F10
        case(KEY_F11):           return "F11";          // F11
        case(KEY_F12):           return "F12";          // F12
        case(KEY_LEFT_SHIFT):    return "Left shift";   // Shift left
        case(KEY_LEFT_CONTROL):  return "Left ctrl";    // Control left
        case(KEY_LEFT_ALT):      return "Left alt";     // Alt left
        case(KEY_LEFT_SUPER):    return "Left super";   // Super left
        case(KEY_RIGHT_SHIFT):   return "Right shift";  // Shift right
        case(KEY_RIGHT_CONTROL): return "Right ctrl";   // Control right
        case(KEY_RIGHT_ALT):     return "Right alt";    // Alt right
        case(KEY_RIGHT_SUPER):   return "Right super";  // Super right
        case(KEY_KB_MENU):       return "KB menu";      // KB menu
        case(KEY_KP_0):          return "Keypad 0";     // Keypad 0
        case(KEY_KP_1):          return "Keypad 1";     // Keypad 1
        case(KEY_KP_2):          return "Keypad 2";     // Keypad 2
        case(KEY_KP_3):          return "Keypad 3";     // Keypad 3
        case(KEY_KP_4):          return "Keypad 4";     // Keypad 4
        case(KEY_KP_5):          return "Keypad 5";     // Keypad 5
        case(KEY_KP_6):          return "Keypad 6";     // Keypad 6
        case(KEY_KP_7):          return "Keypad 7";     // Keypad 7
        case(KEY_KP_8):          return "Keypad 8";     // Keypad 8
        case(KEY_KP_9):          return "Keypad 9";     // Keypad 9
        case(KEY_KP_DECIMAL):    return "Keypad .";     // Keypad .
        case(KEY_KP_DIVIDE):     return "Keypad /";     // Keypad /
        case(KEY_KP_MULTIPLY):   return "Keypad *";     // Keypad *
        case(KEY_KP_SUBTRACT):   return "Keypad -";     // Keypad -
        case(KEY_KP_ADD):        return "Keypad +";     // Keypad +
        case(KEY_KP_ENTER):      return "Keypad enter"; // Keypad Enter
        case(KEY_KP_EQUAL):      return "Keypad equal"; // Keypad Equal

        default:                 return TextFormat("Undefined: %d", key);
    }
}

int main() {
    //SetConfigFlags(FLAG_MSAA_4X_HINT); // Doesn't do anything?
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "2048");
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

    Image icon = LoadImage("assets/icon.png");
    SetWindowIcon(icon);

    Font font = LoadFontEx("assets/fonts/ClearSans-Bold.ttf", FONT_SIZE, NULL, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    Texture2D optionsSymbol = LoadTexture("assets/options_symbol.png");
    SetTextureFilter(optionsSymbol, TEXTURE_FILTER_BILINEAR);

    Board board = {.newTile = -1};
    board.board[GetRandomFreeTile(&board.board)] = 1;
    board.board[GetRandomFreeTile(&board.board)] = 1;

    i32 score = 0;
    i32 highscore = LoadHighscore("assets/data.json");

    bool isGameOver = false;
    f32 gameOverFadeInTimer = GAME_OVER_FADE_IN_DURATION;

    bool isOptionsMenuOpen = false;
    f32 optionsTimer = OPTIONS_TIMER_DURATION;

    Button buttonTryAgain = {
        .rectangle = {
            .x = BOARD_BACKGROUND.x + BOARD_BACKGROUND.width / 2 - BUTTON_TRY_AGAIN_WIDTH / 2,
            .y = BOARD_BACKGROUND.y + BUTTON_TRY_AGAIN_OFFSET,
            .width = BUTTON_TRY_AGAIN_WIDTH,
            .height = BUTTON_TRY_AGAIN_HEIGHT
        },
        .colour = COLOUR_TEXT,
        .font = font,
        .text = "Try again?",
        .textSize = BUTTON_TRY_AGAIN_TEXT_SIZE,
        .textColour = COLOUR_TEXT_ALT,
        .isActive = false
    };
    buttonTryAgain.textPosition = GetTextPositionCentred(buttonTryAgain.rectangle, buttonTryAgain.font, buttonTryAgain.text, buttonTryAgain.textSize);

    Button buttonNewGame = {
        .rectangle = {
            .x = BOARD_BACKGROUND.x,
            .y = BOARD_BACKGROUND.y - BOARD_PADDING - BUTTON_NEW_GAME_HEIGHT,
            .width = BUTTON_NEW_GAME_WIDTH,
            .height = BUTTON_NEW_GAME_HEIGHT
    },
        .colour = COLOUR_TEXT,
        .font = font,
        .text = "New game",
        .textSize = BUTTON_NEW_GAME_TEXT_SIZE,
        .textColour = COLOUR_TEXT_ALT,
        .isActive = true
    };
    buttonNewGame.textPosition = GetTextPositionCentred(buttonNewGame.rectangle, buttonNewGame.font, buttonNewGame.text, buttonNewGame.textSize);

    Button buttonOptions = {
        .rectangle = {
            .x = buttonNewGame.rectangle.x + buttonNewGame.rectangle.width + SCORE_DISPLAY_SPACING,
            .y = buttonNewGame.rectangle.y,
            .width = BUTTON_NEW_GAME_HEIGHT,
            .height = BUTTON_NEW_GAME_HEIGHT
    },
        .colour = COLOUR_TEXT,
        .font = font,
        .text = "",
        .textSize = 0,
        .textColour = BLANK,
        .isActive = true
    };
    buttonOptions.textPosition = GetTextPositionCentred(buttonOptions.rectangle, buttonOptions.font, buttonOptions.text, buttonOptions.textSize);

    Keybinds keybinds = {
        .up = KEY_UP,
        .down = KEY_DOWN,
        .left = KEY_LEFT,
        .right = KEY_RIGHT
    };
    i32 buttonToBindIndex = -1;

    Button buttonsKeybinds[KEY_BINDINGS_COUNT];
    for (i32 i = 0; i < KEY_BINDINGS_COUNT; ++i) {
        buttonsKeybinds[i] = (Button){
            .colour = COLOUR_TEXT,
            .font = font,
            .text = KeyCodeToString(keybinds.binds[i]),
            .textSize = BUTTONS_KEYBINDS_TEXT_SIZE,
            .textColour = COLOUR_TEXT_ALT,
            .isActive = false
        };
        Vector2 textDimensions = MeasureTextEx(buttonsKeybinds[i].font, buttonsKeybinds[i].text, buttonsKeybinds[i].textSize, 0.0f);
        buttonsKeybinds[i].rectangle = (Rectangle){
            .x = BUTTONS_KEYBINDS_START_X + BUTTONS_KEYBINDS_OFFSET_X,
            .y = BUTTONS_KEYBINDS_START_Y + i * BUTTONS_KEYBINDS_POSITION_DELTA,
            .width = MaxF32(textDimensions.x + 2 * BUTTONS_KEYBINDS_TEXT_MARGIN, BUTTONS_KEYBINDS_HEIGHT),
            .height = BUTTONS_KEYBINDS_HEIGHT
        };
        buttonsKeybinds[i].textPosition = GetTextPositionCentred(buttonsKeybinds[i].rectangle, buttonsKeybinds[i].font, 
            buttonsKeybinds[i].text, buttonsKeybinds[i].textSize);
    }

    while (!WindowShouldClose()) {
        // CONTINUE HERE! Implement the actual button logic for the keybinds
        if (IsKeyDown(KEY_ONE)) {
            i32 key = 0;
            i32 newKey = 0;
            while ((newKey = GetKeyPressed()) != 0) {
                key = newKey;
            }
            if (key != 0 && key != KEY_ONE) {
                keybinds.up = key;
            }
        }

        if (IsButtonPressed(&buttonOptions) && !isGameOver) {
            isOptionsMenuOpen = !isOptionsMenuOpen;

            if (isOptionsMenuOpen) {
                for (i32 i = 0; i < KEY_BINDINGS_COUNT; ++i) {
                    buttonsKeybinds[i].isActive = true;
                }
            } else {
                for (i32 i = 0; i < KEY_BINDINGS_COUNT; ++i) {
                    buttonsKeybinds[i].isActive = false;
                }

                if (buttonToBindIndex != -1) {
                    keybinds.binds[buttonToBindIndex] = 0;

                    buttonsKeybinds[buttonToBindIndex].text = "";
                    buttonsKeybinds[buttonToBindIndex].rectangle.width = BUTTONS_KEYBINDS_HEIGHT;

                    buttonToBindIndex = -1;
                }
            }
        }

        if (isOptionsMenuOpen) {
            optionsTimer -= GetFrameTime();
            if (optionsTimer < 0.0f) {
                optionsTimer = 0.0f;
            }

            for (i32 i = 0; i < KEY_BINDINGS_COUNT; ++i) {
                if (IsButtonPressed(&buttonsKeybinds[i])) {
                    buttonToBindIndex = i;
                }
            }

            if (buttonToBindIndex != -1) {
                i32 key = 0;
                i32 newKey = 0;
                while ((newKey = GetKeyPressed()) != 0) {
                    key = newKey;
                }
                if (key != 0) {
                    keybinds.binds[buttonToBindIndex] = key;

                    buttonsKeybinds[buttonToBindIndex].text = KeyCodeToString(key);
                    f32 textWidth = MeasureTextEx(buttonsKeybinds[buttonToBindIndex].font, buttonsKeybinds[buttonToBindIndex].text, 
                        buttonsKeybinds[buttonToBindIndex].textSize, 0.0f).x;
                    buttonsKeybinds[buttonToBindIndex].rectangle.width = MaxF32(textWidth + 2 * BUTTONS_KEYBINDS_TEXT_MARGIN, BUTTONS_KEYBINDS_HEIGHT);
                    buttonsKeybinds[buttonToBindIndex].textPosition = GetTextPositionCentred(buttonsKeybinds[buttonToBindIndex].rectangle, 
                        buttonsKeybinds[buttonToBindIndex].font, buttonsKeybinds[buttonToBindIndex].text, buttonsKeybinds[buttonToBindIndex].textSize);

                    buttonToBindIndex = -1;
                }
            }
        } else {
            optionsTimer += GetFrameTime();
            if (optionsTimer > OPTIONS_TIMER_DURATION) {
                optionsTimer = OPTIONS_TIMER_DURATION;
            }
        }

        board.movingTiles.timer -= GetFrameTime();
        if (board.movingTiles.timer <= 0.0f) {
            board.movingTiles.timer = 0.0f;
            board.movingTiles.count = 0;

            if (board.newTile != -1) {
                board.combinedTimer = TILE_COMBINE_DURATION;
            }

            board.newTile = -1;
        }

        board.combinedTimer -= GetFrameTime();
        if (board.combinedTimer < 0.0f) {
            board.combinedTimer = -1.0f;
        }

        if (isGameOver) {
            gameOverFadeInTimer -= GetFrameTime();
            if (gameOverFadeInTimer < 0.0f) {
                gameOverFadeInTimer = 0.0f;
            }
        }

        if (IsButtonPressed(&buttonNewGame) && !(isGameOver && gameOverFadeInTimer > 0.0f)) {
            board = (Board){0};
            board.board[GetRandomFreeTile(&board.board)] = 1;
            board.board[GetRandomFreeTile(&board.board)] = 1;

            if (score > highscore) {
                SaveHighscore("assets/data.json", score);
                highscore = score;
            }
            score = 0;

            gameOverFadeInTimer = GAME_OVER_FADE_IN_DURATION;

            isGameOver = false;
        }

        if (IsButtonPressed(&buttonTryAgain) && gameOverFadeInTimer == 0.0f) {
            board = (Board){0};
            board.board[GetRandomFreeTile(&board.board)] = 1;
            board.board[GetRandomFreeTile(&board.board)] = 1;

            highscore = MaxI32(score, highscore);
            score = 0;

            gameOverFadeInTimer = GAME_OVER_FADE_IN_DURATION;

            isGameOver = false;

            buttonTryAgain.isActive = false;
        }

        if (!isGameOver && !isOptionsMenuOpen && Move(&board, &keybinds, &score)) {
            board.newTile = GetRandomFreeTile(board.board);
            board.board[board.newTile] = GetRandomValue(1, 2);
            if (IsBoardFull(&board.board) && !CanMove(&board.board)) {
                isGameOver = true;

                board.movingTiles.timer = 0.0f;

                buttonTryAgain.isActive = true;

                if (score > highscore) {
                    SaveHighscore("assets/data.json", score);
                }
            }
        }

        // Render

        BeginDrawing();

        ClearBackground(COLOUR_BACKGROUND);

        DisplayBoard(&board, font);

        if (board.combinedTimer > 0.0f) {
            DisplayCombinedTiles(&board, font);
        } else {
            if (board.newTile != -1) {
                DisplayNewTile(&board);
            }

            DisplayMovingTiles(&board, font);
        }

        if (isGameOver) {
            DisplayGameOver(font, gameOverFadeInTimer, &buttonTryAgain);
        }

        DisplayScores(font, score, highscore);

        DisplayButtons(&buttonNewGame, &buttonOptions, &optionsSymbol, optionsTimer);

        // TODO: Custom symbols for some keys? (like the arrow keys, etc.)
        if (optionsTimer < OPTIONS_TIMER_DURATION) {
            DisplayOptions(buttonsKeybinds, optionsTimer, buttonToBindIndex);
        }

        EndDrawing();
    }

    if (score > highscore) {
        SaveHighscore("assets/data.json", score);
    }

    UnloadTexture(optionsSymbol);
    UnloadFont(font);

    CloseWindow();

    return 0;
}