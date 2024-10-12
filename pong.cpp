#include <stdio.h>
#include <unistd.h>
#include <string>
#include <random>

extern "C"
{
#include <tfblib/tfblib.h>
#include <tfblib/tfb_colors.h>
}

const auto& BACKGROUND = tfb_black;
const auto& FOREGROUND = tfb_white;

class PongField
{
public:
    PongField(size_t width, size_t height)
    : mWidth(width), mHeight(height),
      mPaddleHeight(mHeight / 5), mPaddleWidth(mPaddleHeight / 10),
      mBallSize(mHeight / 60),
      mPlayer1(mHeight / 2), mPlayer2(mHeight / 2),
      mScore1(0), mScore2(0)
    {
        resetBall();
    }

    void draw()
    {
        drawPaddle(mPaddleWidth, mPlayer1);
        drawPaddle(mWidth - 2 * mPaddleWidth, mPlayer2);
        drawLine();
        drawScore();
        drawBall();
    }

    void moveBall(int dx, int dy)
    {
        mBallX += dx;
        mBallY += dy;
    }

    void move1(int dist)
    {
        mPlayer1 += dist;
    }

    void move2(int dist)
    {
        mPlayer2 += dist;
    }

    void score1()
    {
        mScore1++;
    }

    void score2()
    {
        mScore2++;
    }

    bool wouldScore(int dx, int dy)
    {
        const auto newX = mBallX + dx;
        if (newX <= 0 or newX >= mWidth)
            return true;
        return false;
    }
private:

    void drawPaddle(size_t x, size_t y)
    {
        const auto paddleX = x;
        const auto paddleY = y - (mPaddleHeight / 2);
        tfb_fill_rect(paddleX, paddleY, mPaddleWidth, mPaddleHeight, FOREGROUND);
    }

    void drawLine()
    {
        for (auto y = 0; y < mHeight; y += mHeight / 20)
        {
            tfb_draw_vline(mWidth /  2, y, mHeight / 40, FOREGROUND);
        }
    }

    void drawScore()
    {
        const auto score1(std::to_string(mScore1));
        const auto score2(std::to_string(mScore2));
        tfb_draw_xcenter_string_scaled(mWidth * 0.45, 10, FOREGROUND, BACKGROUND, 2, 2, score1.c_str());
        tfb_draw_xcenter_string_scaled(mWidth * 0.55, 10, FOREGROUND, BACKGROUND, 2, 2, score2.c_str());
    }

    void drawBall()
    {
        tfb_fill_rect(mBallX - mBallSize / 2, mBallY - mBallSize / 2, mBallSize, mBallSize, FOREGROUND);
    }

    void resetBall()
    {
        mBallX = mWidth / 2;
        mBallY = mHeight / 2;;
    }

    const size_t mWidth;
    const size_t mHeight;
    const size_t mPaddleHeight;
    const size_t mPaddleWidth;
    const size_t mBallSize;
    size_t mPlayer1;
    size_t mPlayer2;
    int mScore1;
    int mScore2;
    int mBallX;
    int mBallY;
};

class PongGame
{
public:
    PongGame(size_t width, size_t height) :
        mField(width, height),
        mSpeed(width * 0.01f),
        mRand(std::random_device()())
    { }

    void run()
    {
        resetSpeed();

        tfb_clear_screen(BACKGROUND);
        mField.draw();
        tfb_flush_window();

        sleep(1);
        mField.move1(10);
        mField.move2(-20);
        mField.score1();
        mField.score2();
        mField.score2();

        while (not mField.wouldScore(mSpeedx, mSpeedy))
        {
            tick();
            tfb_clear_screen(BACKGROUND);
            mField.draw();
            tfb_flush_window();
        }


    }

private:
    void tick()
    {
        mField.moveBall(mSpeedx, mSpeedy);
    }

    void resetSpeed()
    {
        mSpeedx = static_cast<int>(mSpeed);
        mSpeedy = random(mSpeed);
    }

    float random(float limit)
    {
        std::uniform_real_distribution<float> dist;
        return dist(mRand) * limit * 2 - limit;
    }

    PongField mField;
    const float mSpeed;
    int mSpeedx;
    int mSpeedy;
    std::mt19937 mRand;
};


int main(int argc, char **argv)
{
    char* fbdev = nullptr;
    if (argc > 1)
        fbdev = argv[1];

    int rc;
    rc = tfb_acquire_fb(TFB_FL_NO_TTY_KD_GRAPHICS | TFB_FL_USE_DOUBLE_BUFFER, fbdev, nullptr);

    if (rc != TFB_SUCCESS) {
        fprintf(stderr, "tfb_acquire_fb() failed: %s\n", tfb_strerror(rc));
        return 1;
    }

    PongGame game(tfb_screen_width(), tfb_screen_height());

    game.run();

    sleep(1);
    tfb_clear_screen(BACKGROUND);
//    tfb_flush_window();

    tfb_release_fb();
    return 0;
}
