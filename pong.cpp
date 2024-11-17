#include <cstdio>
#include <chrono>
#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <map>
#include <random>
#include <queue>
#include <stdexcept>
#include <string>
#include <unistd.h>

extern "C"
{
#include <tfblib/tfblib.h>
#include <tfblib/tfb_colors.h>
}

using namespace std::chrono_literals;

const auto& BACKGROUND = tfb_black;
const auto& FOREGROUND = tfb_white;
const auto INTERVAL = 100ms;
const auto POINTS = 5;
const auto SPEED_FACTOR = 0.02f;


class Input
{
public:
    enum class Key
    {
        P1_UP,
        P1_DOWN,
        P2_UP,
        P2_DOWN,
    };

    Input(const std::string& deviceFile)
     : mDev(nullptr)
    {
        int fd = open(deviceFile.c_str(), O_RDONLY|O_NONBLOCK);
        if (fd < 0) {
                fprintf(stderr, "Could not open input device %s: %s\n", deviceFile.c_str(), strerror(-fd));
                exit(1);
        }
        int rc = libevdev_new_from_fd(fd, &mDev);
        if (rc < 0) {
                fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
                exit(1);
        }

        printf("Input device name: \"%s\"\n", libevdev_get_name(mDev));

        if (!libevdev_has_event_type(mDev, EV_KEY)) {
                printf("This device does not look like a keyboard\n");
                exit(1);
        }

        std::queue<Key> unmappedKeys({Key::P2_DOWN, Key::P1_UP, Key::P1_DOWN, Key::P2_UP});
        for (unsigned int code = 0; code < KEY_MAX; code++)
        {
            if (libevdev_has_event_code(mDev, EV_KEY, code))
            {
                const auto key = unmappedKeys.front();
                printf("Using key code %s for key %d\n", libevdev_event_code_get_name(EV_KEY, code), (int)key);
                mKeyMapping[code] = key;
                unmappedKeys.pop();
                if (unmappedKeys.empty())
                    break;
            }
        }

        if (not unmappedKeys.empty())
        {
            throw(std::runtime_error("Not enough events found"));
        }
    }

    ~Input()
    {
        if (mDev != nullptr)
        {
            libevdev_free(mDev);
            mDev = nullptr;
        }
    }

    void process()
    {
        bool done = false;
        while (not done) {
            struct input_event ev;
            int rc = libevdev_next_event(mDev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            if ((rc == 0) and (ev.type == EV_KEY) and mKeyMapping.find(ev.code) != mKeyMapping.end())
            {
                const auto key = mKeyMapping[ev.code];
                mState[key] = ev.value != 0;
                printf("Event: %d %d\n", (int)key, ev.value);
            }
            if (rc != LIBEVDEV_READ_STATUS_SYNC and rc !=  LIBEVDEV_READ_STATUS_SUCCESS)
                done = true;
        }
    }

    bool isKeyActive(Key key)
    {
        return mState[key];
    }
private:
    libevdev *mDev;
    std::map<Key, bool> mState;
    std::map<unsigned int, Key> mKeyMapping;
};


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

    void moveBall(int& dx, int& dy)
    {
        auto newX = mBallX + dx;
        auto newY = mBallY + dy;
        // upper wall
        if (newY < 0)
        {
            dy = -dy;
            newY = - newY;
        }

        // lower wall
        if (newY > mHeight)
        {
            dy = -dy;
            newY = 2 * mHeight - newY;
        }

        // left paddle
        const auto paddle1Pos = 2 * mPaddleWidth;
        if (mBallX > paddle1Pos and newX <= paddle1Pos and 2 * std::abs(newY - mPlayer1) <= mPaddleHeight)
        {
            dx = -dx;
            newX = (2 * paddle1Pos) - newX;
        }
        // right paddle
        const auto paddle2Pos = mWidth - 2 * mPaddleWidth;
        if (mBallX < paddle2Pos and newX >= paddle2Pos and 2 * std::abs(newY - mPlayer2) <= mPaddleHeight)
        {
            dx = -dx;
            newX = (2 * paddle2Pos) - newX;
        }

        mBallX = newX;
        mBallY = newY;
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

    int score(int dx, int dy)
    {
        const auto paddle1Pos = 2 * mPaddleWidth;
        const auto paddle2Pos = mWidth - 2 * mPaddleWidth;

        const auto newX = mBallX + dx;
        if (mBallX < paddle1Pos and newX <= 0)
            return 2;
        if (mBallX > paddle2Pos and newX >= mWidth)
            return 1;
        return 0;
    }

    bool gameOver()
    {
        return (mScore1>= POINTS or mScore2 >= POINTS);
    }

    void resetBall()
    {
        mBallX = mWidth / 2;
        mBallY = mHeight / 2;;
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

    const size_t mWidth;
    const size_t mHeight;
    const size_t mPaddleHeight;
    const size_t mPaddleWidth;
    const size_t mBallSize;
    int mPlayer1;
    int mPlayer2;
    int mScore1;
    int mScore2;
    int mBallX;
    int mBallY;
};


class PongGame
{
public:
    PongGame(size_t width, size_t height, Input& input) :
        mField(width, height),
        mInput(input),
        mSpeed(width * SPEED_FACTOR),
        mRand(std::random_device()())
    { }

    void run()
    {
        resetSpeed();

        mField.move1(10);
        mField.move2(-20);
        mField.score1();
        mField.score2();
        mField.score2();

        while (not mField.gameOver())
        {
            const auto lastExecution = std::chrono::high_resolution_clock::now();
            tick();
            tfb_clear_screen(BACKGROUND);
            mField.draw();
            tfb_flush_window();

            const auto now = std::chrono::high_resolution_clock::now();
            const auto next = lastExecution + INTERVAL;
            const auto sleepTime = next - now;
            const auto sleepUs = std::chrono::duration_cast<std::chrono::microseconds>(sleepTime).count();

            if (sleepUs > 0)
                usleep(sleepUs);
            else
                printf("delayed frame (%ldus)\n", -sleepUs);
        }
    }

private:
    void tick()
    {
        switch (mField.score(mSpeedx, mSpeedy))
        {
            case 1:
                mField.score1();
                resetSpeed();
                mField.resetBall();
                break;
            case 2:
                mField.score2();
                resetSpeed();
                mField.resetBall();
                break;
        }
        mField.moveBall(mSpeedx, mSpeedy);
        mInput.process();
        int m1 = 0;
        int m2 = 0;
        if (mInput.isKeyActive(Input::Key::P1_DOWN))
            mField.move1(1);
        if (mInput.isKeyActive(Input::Key::P1_UP))
            mField.move1(-1);
        if (mInput.isKeyActive(Input::Key::P2_DOWN))
            mField.move2(1);
        if (mInput.isKeyActive(Input::Key::P2_UP))
            mField.move2(-1);
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
    Input& mInput;
    const float mSpeed;
    int mSpeedx;
    int mSpeedy;
    std::mt19937 mRand;
};


int main(int argc, char **argv)
{
    char* fbdev = nullptr;
    std::string evdev("/dev/input/event0");
    if (argc > 1)
        fbdev = argv[1];
    if (argc > 2)
        evdev = argv[2];

    Input i(evdev);

    int rc;
    rc = tfb_acquire_fb(TFB_FL_NO_TTY_KD_GRAPHICS | TFB_FL_USE_DOUBLE_BUFFER, fbdev, nullptr);

    if (rc != TFB_SUCCESS) {
        fprintf(stderr, "tfb_acquire_fb() failed: %s\n", tfb_strerror(rc));
        return 1;
    }

    PongGame game(tfb_screen_width(), tfb_screen_height(), i);

    game.run();

    sleep(1);
    tfb_clear_screen(BACKGROUND);
//    tfb_flush_window();

    tfb_release_fb();
    return 0;
}
