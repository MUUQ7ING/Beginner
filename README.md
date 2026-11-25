# C++ 俄罗斯方块小游戏

这是一个使用 **C++** 编写的终端版俄罗斯方块，支持基本玩法：移动、旋转、软降、硬降、消行与计分。

## 编译

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic src/tetris.cpp -o tetris
```

## 运行

```bash
./tetris
```

## 操作说明

- `A` / `D`：左右移动
- `W`：旋转方块
- `S`：软降（加速下落）
- `Space`：硬降（直接落到底）
- `Q`：退出游戏

> 提示：该版本基于 Linux/macOS 终端的 `termios` 实现非阻塞输入。
