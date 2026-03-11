#include <array>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

constexpr int kWidth = 10;
constexpr int kHeight = 20;
constexpr int kBlockSize = 4;

using Shape = std::array<std::array<int, kBlockSize>, kBlockSize>;

const std::array<Shape, 7> kTetrominoes = {
    // I
    Shape{{{{0, 0, 0, 0}}, {{1, 1, 1, 1}}, {{0, 0, 0, 0}}, {{0, 0, 0, 0}}}},
    // O
    Shape{{{{0, 2, 2, 0}}, {{0, 2, 2, 0}}, {{0, 0, 0, 0}}, {{0, 0, 0, 0}}}},
    // T
    Shape{{{{0, 3, 0, 0}}, {{3, 3, 3, 0}}, {{0, 0, 0, 0}}, {{0, 0, 0, 0}}}},
    // S
    Shape{{{{0, 4, 4, 0}}, {{4, 4, 0, 0}}, {{0, 0, 0, 0}}, {{0, 0, 0, 0}}}},
    // Z
    Shape{{{{5, 5, 0, 0}}, {{0, 5, 5, 0}}, {{0, 0, 0, 0}}, {{0, 0, 0, 0}}}},
    // J
    Shape{{{{6, 0, 0, 0}}, {{6, 6, 6, 0}}, {{0, 0, 0, 0}}, {{0, 0, 0, 0}}}},
    // L
    Shape{{{{0, 0, 7, 0}}, {{7, 7, 7, 0}}, {{0, 0, 0, 0}}, {{0, 0, 0, 0}}}},
};

const std::array<std::string, 8> kCellChars = {
    "  ", "██", "▓▓", "▒▒", "░░", "▣▣", "▦▦", "▩▩"
};

struct Piece {
    Shape shape;
    int x = 3;
    int y = 0;
};

class TerminalRawMode {
public:
    TerminalRawMode() {
        tcgetattr(STDIN_FILENO, &orig_);
        termios raw = orig_;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }

    ~TerminalRawMode() {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_);
    }

private:
    termios orig_{};
};

class TetrisGame {
public:
    TetrisGame() : board_(kHeight, std::vector<int>(kWidth, 0)) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        spawnPiece();
    }

    void run() {
        TerminalRawMode raw_mode;

        using clock = std::chrono::steady_clock;
        auto last_fall = clock::now();
        auto fall_interval = std::chrono::milliseconds(450);

        while (!game_over_) {
            handleInput();

            auto now = clock::now();
            if (now - last_fall >= fall_interval) {
                tick();
                last_fall = now;
                if (level_ > 1) {
                    int speed = 450 - (level_ - 1) * 25;
                    fall_interval = std::chrono::milliseconds(std::max(100, speed));
                }
            }

            render();
            std::this_thread::sleep_for(std::chrono::milliseconds(18));
        }

        std::cout << "\n游戏结束！最终得分: " << score_ << "\n";
    }

private:
    std::vector<std::vector<int>> board_;
    Piece current_{};
    bool game_over_ = false;
    int score_ = 0;
    int lines_cleared_ = 0;
    int level_ = 1;

    void spawnPiece() {
        current_.shape = kTetrominoes[std::rand() % static_cast<int>(kTetrominoes.size())];
        current_.x = 3;
        current_.y = 0;

        if (collides(current_.shape, current_.x, current_.y)) {
            game_over_ = true;
        }
    }

    static Shape rotateClockwise(const Shape& shape) {
        Shape rotated{};
        for (int r = 0; r < kBlockSize; ++r) {
            for (int c = 0; c < kBlockSize; ++c) {
                rotated[c][kBlockSize - 1 - r] = shape[r][c];
            }
        }
        return rotated;
    }

    bool collides(const Shape& shape, int new_x, int new_y) const {
        for (int r = 0; r < kBlockSize; ++r) {
            for (int c = 0; c < kBlockSize; ++c) {
                if (!shape[r][c]) {
                    continue;
                }

                int board_x = new_x + c;
                int board_y = new_y + r;

                if (board_x < 0 || board_x >= kWidth || board_y >= kHeight) {
                    return true;
                }

                if (board_y >= 0 && board_[board_y][board_x] != 0) {
                    return true;
                }
            }
        }

        return false;
    }

    void lockPiece() {
        for (int r = 0; r < kBlockSize; ++r) {
            for (int c = 0; c < kBlockSize; ++c) {
                int cell = current_.shape[r][c];
                if (!cell) {
                    continue;
                }

                int board_x = current_.x + c;
                int board_y = current_.y + r;
                if (board_y >= 0 && board_y < kHeight && board_x >= 0 && board_x < kWidth) {
                    board_[board_y][board_x] = cell;
                }
            }
        }

        clearLines();
        spawnPiece();
    }

    void clearLines() {
        int cleared = 0;

        for (int row = kHeight - 1; row >= 0; --row) {
            bool full = true;
            for (int col = 0; col < kWidth; ++col) {
                if (board_[row][col] == 0) {
                    full = false;
                    break;
                }
            }

            if (!full) {
                continue;
            }

            board_.erase(board_.begin() + row);
            board_.insert(board_.begin(), std::vector<int>(kWidth, 0));
            ++cleared;
            ++row;
        }

        if (cleared == 0) {
            return;
        }

        lines_cleared_ += cleared;
        level_ = lines_cleared_ / 10 + 1;

        switch (cleared) {
            case 1: score_ += 100 * level_; break;
            case 2: score_ += 300 * level_; break;
            case 3: score_ += 500 * level_; break;
            case 4: score_ += 800 * level_; break;
            default: break;
        }
    }

    void tick() {
        if (!collides(current_.shape, current_.x, current_.y + 1)) {
            ++current_.y;
        } else {
            lockPiece();
        }
    }

    void move(int dx) {
        int next_x = current_.x + dx;
        if (!collides(current_.shape, next_x, current_.y)) {
            current_.x = next_x;
        }
    }

    void softDrop() {
        if (!collides(current_.shape, current_.x, current_.y + 1)) {
            ++current_.y;
            score_ += 1;
        }
    }

    void hardDrop() {
        while (!collides(current_.shape, current_.x, current_.y + 1)) {
            ++current_.y;
            score_ += 2;
        }
        lockPiece();
    }

    void rotate() {
        Shape rotated = rotateClockwise(current_.shape);
        if (!collides(rotated, current_.x, current_.y)) {
            current_.shape = rotated;
            return;
        }

        // 简单墙踢
        if (!collides(rotated, current_.x - 1, current_.y)) {
            current_.x -= 1;
            current_.shape = rotated;
        } else if (!collides(rotated, current_.x + 1, current_.y)) {
            current_.x += 1;
            current_.shape = rotated;
        }
    }

    bool hasInput() const {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        return select(STDIN_FILENO + 1, &set, nullptr, nullptr, &timeout) > 0;
    }

    void handleInput() {
        while (hasInput()) {
            char ch = 0;
            if (read(STDIN_FILENO, &ch, 1) <= 0) {
                return;
            }

            switch (ch) {
                case 'a':
                case 'A':
                    move(-1);
                    break;
                case 'd':
                case 'D':
                    move(1);
                    break;
                case 's':
                case 'S':
                    softDrop();
                    break;
                case 'w':
                case 'W':
                    rotate();
                    break;
                case ' ':
                    hardDrop();
                    break;
                case 'q':
                case 'Q':
                    game_over_ = true;
                    break;
                default:
                    break;
            }
        }
    }

    void render() const {
        auto frame = board_;

        for (int r = 0; r < kBlockSize; ++r) {
            for (int c = 0; c < kBlockSize; ++c) {
                int cell = current_.shape[r][c];
                if (!cell) {
                    continue;
                }
                int x = current_.x + c;
                int y = current_.y + r;
                if (x >= 0 && x < kWidth && y >= 0 && y < kHeight) {
                    frame[y][x] = cell;
                }
            }
        }

        std::cout << "\033[2J\033[H";  // 清屏 + 光标归位
        std::cout << "=== C++ 俄罗斯方块 ===\n";
        std::cout << "操作: A/D 左右, W 旋转, S 加速下落, 空格 硬降, Q 退出\n";
        std::cout << "分数: " << score_ << "   行数: " << lines_cleared_ << "   等级: " << level_ << "\n\n";

        std::cout << "┌";
        for (int i = 0; i < kWidth * 2; ++i) {
            std::cout << "─";
        }
        std::cout << "┐\n";

        for (int row = 0; row < kHeight; ++row) {
            std::cout << "│";
            for (int col = 0; col < kWidth; ++col) {
                std::cout << kCellChars[frame[row][col]];
            }
            std::cout << "│\n";
        }

        std::cout << "└";
        for (int i = 0; i < kWidth * 2; ++i) {
            std::cout << "─";
        }
        std::cout << "┘\n";
        std::cout.flush();
    }
};

}  // namespace

int main() {
    TetrisGame game;
    game.run();
    return 0;
}
