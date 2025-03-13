#pragma once
#include <stdlib.h>

typedef int8_t POS_T;  // Тип для хранения координат (8-битное целое число со знаком)

// Структура, представляющая ход в игре
struct move_pos
{
    POS_T x, y;             // Координаты начальной позиции (откуда)
    POS_T x2, y2;           // Координаты конечной позиции (куда)
    POS_T xb = -1, yb = -1; // Координаты битой фигуры (если есть, иначе -1)

    // Конструктор для обычного хода (без битья фигуры)
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }

    // Конструктор для хода с битьём фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // Оператор сравнения двух ходов
    bool operator==(const move_pos& other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }

    // Оператор неравенства двух ходов
    bool operator!=(const move_pos& other) const
    {
        return !(*this == other);
    }
};