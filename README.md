
# 关于大作业：五子棋游戏

## 项目简介

这是一个用 C 语言实现的五子棋游戏，支持在 Linux 环境中运行。五子棋是一种经典的双人策略棋盘游戏，目标是在15x15的网格上连接五颗棋子，横向、纵向或对角线。

## 功能特性

- 实时绘制五子棋棋盘
- 通过触摸和蓝牙与对手对战
- 菜单功能包括重新开始游戏和查看历史记录

## 技术栈

- C 语言
- Linux Framebuffer API
- 多点触摸输入
- 蓝牙通信

## 运行
- 根据实验指导手册完成两个开发板的配对
- 两个开发板分别使用rfcomm建立蓝牙socket并监听，且连接对方的信道
- 开发板A：
  ```bash
  rfcomm -r watch 0 1
  ```
  ```bash
  rfcomm -r connect 1 xx:xx:xx:xx:xx:xx 2
  ```
  开发板B：
  ```bash
  rfcomm -r watch 1 2
  ```
  ```bash
  rfcomm -r connect 0 xx:xx:xx:xx:xx:xx 1
  ```

