#include <iostream>
#include <vector>
#include <raylib.h>
#include <raymath.h>
#include <cmath>

struct Pipe
{
    Rectangle topRec;
    Rectangle bottomRec;
    bool passed;
};

void DrawTextureTiling(Texture2D texture, float posX, float posY, float width, float height, float scrollOffset, Color tint)
{
    if (texture.id <= 0 || width <= 0 || height <= 0)
        return;

    float tileX = fmodf(scrollOffset, (float)texture.width);

    if (tileX > 0)
        tileX -= texture.width;

    for (float x = tileX; x < width; x += texture.width)
        DrawTextureV(texture, {posX + x, posY}, tint);
}

int main(int, char **)
{
    const int SCREEN_WIDTH = 870;
    const int SCREEN_HEIGHT = 512;

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "C++ Flappy Bird");
    SetTargetFPS(300);

    const Texture2D BACKGROUND_TEXTURE = LoadTexture("./assets/sprites/background.png");
    const Texture2D GROUND_TEXTURE = LoadTexture("./assets/sprites/ground.png");
    const Texture2D GAMEOVER_TEXTURE = LoadTexture("./assets/sprites/gameover.png");
    const Texture2D PIPE_TEXTURE = LoadTexture("./assets/sprites/pipe.png");
    const Texture2D PLAYER_TEXTURE = LoadTexture("./assets/sprites/yellowbird.png");

    if (BACKGROUND_TEXTURE.id == 0 || GROUND_TEXTURE.id == 0 || GAMEOVER_TEXTURE.id == 0 || PIPE_TEXTURE.id == 0 || PLAYER_TEXTURE.id == 0)
    {
        TraceLog(LOG_ERROR, "Error loading textures.");
        CloseWindow();
        return 1;
    }

    InitAudioDevice();
    Sound pointSound = LoadSound("./assets/audio/point.wav");
    Sound hitSound = LoadSound("./assets/audio/hit.wav");
    Sound wingSound = LoadSound("./assets/audio/wing.wav");

    const float GROUND_HEIGHT = static_cast<float>(GROUND_TEXTURE.height);
    const float GROUND_Y = static_cast<float>(SCREEN_HEIGHT) - GROUND_HEIGHT;
    Rectangle groundCollisionRec = {0, GROUND_Y, static_cast<float>(SCREEN_WIDTH), GROUND_HEIGHT};

    const int GAMEOVER_X = (SCREEN_WIDTH / 2) - (GAMEOVER_TEXTURE.width / 2);
    const int GAMEOVER_Y = (SCREEN_HEIGHT / 2) - GAMEOVER_TEXTURE.height * 1.75f;

    const float PLAYER_JUMP_FORCE = -400.0f;
    const float GRAVITY = 1100.0f;
    const float PLAYER_RADIUS = 17.0f;

    const float PIPE_SPEED = 200.0f;

    const int MIN_GAP_SIZE = 100;
    const int MAX_GAP_SIZE = 175;
    const int PIPE_SPACING = 256;
    const int PIPE_Y_MARGIN = 60;

    Vector2 playerPos = {static_cast<float>(SCREEN_WIDTH) / 4.0f, static_cast<float>(SCREEN_HEIGHT) / 2.0f};
    float yVelocity = 0.0f;
    float backgroundScrollOffset = 0.0f;
    float groundScrollOffset = 0.0f;
    bool gameOver = false;

    int currentScore = 0;
    int highScore = 0;

    std::vector<Pipe> pipes;

    auto SpawnPipe = [&](float xPos)
    {
        int randomGap = GetRandomValue(MIN_GAP_SIZE, MAX_GAP_SIZE);

        int gapY = GetRandomValue(PIPE_Y_MARGIN, (int)(GROUND_Y - randomGap - PIPE_Y_MARGIN));

        Rectangle top = {
            xPos,
            static_cast<float>(gapY - PIPE_TEXTURE.height),
            static_cast<float>(PIPE_TEXTURE.width),
            static_cast<float>(PIPE_TEXTURE.height)};
        Rectangle bottom = {
            xPos,
            static_cast<float>(gapY + randomGap),
            static_cast<float>(PIPE_TEXTURE.width),
            static_cast<float>(PIPE_TEXTURE.height)};
        return Pipe{top, bottom, false};
    };

    pipes.push_back(SpawnPipe(static_cast<float>(SCREEN_WIDTH)));

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        if (!gameOver)
        {
            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                yVelocity = PLAYER_JUMP_FORCE;
                PlaySound(wingSound);
            }

            yVelocity += GRAVITY * deltaTime;
            playerPos.y += yVelocity * deltaTime;

            backgroundScrollOffset = fmodf(backgroundScrollOffset - (PIPE_SPEED * 0.5f * deltaTime), static_cast<float>(BACKGROUND_TEXTURE.width));
            groundScrollOffset = fmodf(groundScrollOffset - (PIPE_SPEED * deltaTime), static_cast<float>(GROUND_TEXTURE.width));

            if (!pipes.empty() && pipes.back().topRec.x < static_cast<float>(SCREEN_WIDTH) - PIPE_SPACING)
                pipes.push_back(SpawnPipe(pipes.back().topRec.x + static_cast<float>(PIPE_SPACING)));

            for (auto &p : pipes)
            {
                p.topRec.x -= PIPE_SPEED * deltaTime;
                p.bottomRec.x -= PIPE_SPEED * deltaTime;

                if (!p.passed && p.topRec.x + p.topRec.width < playerPos.x)
                {
                    p.passed = true;
                    currentScore++;
                    PlaySound(pointSound);

                    if (currentScore > highScore)
                        highScore = currentScore;
                }
            }

            if (!pipes.empty() && pipes.front().topRec.x < -static_cast<float>(PIPE_TEXTURE.width))
                pipes.erase(pipes.begin());

            if (playerPos.y - PLAYER_RADIUS <= 0.0f)
            {
                playerPos.y = PLAYER_RADIUS;
                yVelocity = 0;
                gameOver = true;
                PlaySound(hitSound);
            }

            if (CheckCollisionCircleRec(playerPos, PLAYER_RADIUS, groundCollisionRec))
            {
                playerPos.y = GROUND_Y - PLAYER_RADIUS;
                yVelocity = 0;
                gameOver = true;
                PlaySound(hitSound);
            }

            for (auto &p : pipes)
            {
                if (CheckCollisionCircleRec(playerPos, PLAYER_RADIUS, p.topRec) || CheckCollisionCircleRec(playerPos, PLAYER_RADIUS, p.bottomRec))
                {
                    gameOver = true;
                    PlaySound(hitSound);
                    break;
                }
            }
        }
        else
        {
            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                playerPos = {static_cast<float>(SCREEN_WIDTH) / 4.0f, static_cast<float>(SCREEN_HEIGHT) / 2.0f};
                yVelocity = 0.0f;
                pipes.clear();
                pipes.push_back(SpawnPipe(static_cast<float>(SCREEN_WIDTH)));
                gameOver = false;
                backgroundScrollOffset = 0.0f;
                groundScrollOffset = 0.0f;
                currentScore = 0;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawTextureTiling(BACKGROUND_TEXTURE, 0, 0, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT), backgroundScrollOffset, WHITE);

        for (auto &p : pipes)
        {
            Rectangle pipeSourceRec = {0.0f, 0.0f, static_cast<float>(PIPE_TEXTURE.width), static_cast<float>(PIPE_TEXTURE.height)};

            Rectangle topPipeDest = {
                p.topRec.x + p.topRec.width / 2,
                p.topRec.y + p.topRec.height / 2,
                p.topRec.width,
                p.topRec.height};

            DrawTexturePro(PIPE_TEXTURE,
                           pipeSourceRec,
                           topPipeDest,
                           {static_cast<float>(PIPE_TEXTURE.width) / 2.0f,
                            static_cast<float>(PIPE_TEXTURE.height) / 2.0f},
                           180.0f,
                           WHITE);

            DrawTextureRec(PIPE_TEXTURE, pipeSourceRec, {p.bottomRec.x, p.bottomRec.y}, WHITE);
        }

        DrawTextureTiling(GROUND_TEXTURE, 0, GROUND_Y, static_cast<float>(SCREEN_WIDTH), GROUND_HEIGHT, groundScrollOffset, WHITE);

        float targetPlayerAngle;
        float currentPlayerAngle;

        if (!gameOver)
        {
            targetPlayerAngle = Clamp(yVelocity, -64, 64);
            currentPlayerAngle = Lerp(currentPlayerAngle, targetPlayerAngle, deltaTime);
        }

        DrawTextureEx(PLAYER_TEXTURE,
                      {(playerPos.x - static_cast<float>(PLAYER_TEXTURE.width) / 2.0f),
                       (playerPos.y - static_cast<float>(PLAYER_TEXTURE.height) / 2.0f)},
                      currentPlayerAngle, 1, WHITE);

        char scoreText[32];
        sprintf(scoreText, "Score: %d", currentScore);
        DrawText(scoreText, 20, 20, 30, WHITE);

        char highScoreText[32];
        sprintf(highScoreText, "High Score: %d", highScore);
        DrawText(highScoreText, 20, 60, 20, WHITE);

        if (gameOver)
        {
            DrawTexture(GAMEOVER_TEXTURE, GAMEOVER_X, GAMEOVER_Y, WHITE);

            const char *restartText = "Press SPACE or CLICK to Restart";
            int textWidth = MeasureText(restartText, 20);
            DrawText(restartText, SCREEN_WIDTH / 2 - textWidth / 2, GAMEOVER_Y + GAMEOVER_TEXTURE.height + 20, 20, WHITE);
        }

        EndDrawing();
    }

    UnloadTexture(BACKGROUND_TEXTURE);
    UnloadTexture(GROUND_TEXTURE);
    UnloadTexture(GAMEOVER_TEXTURE);
    UnloadTexture(PIPE_TEXTURE);
    UnloadTexture(PLAYER_TEXTURE);
    CloseWindow();
    return 0;
}