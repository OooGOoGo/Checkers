﻿#pragma once

enum class Response
{
    OK,      // Успешное выполнение действия (например, ход завершён).
    BACK,    // Запрос на отмену хода или возврат к предыдущему состоянию.
    REPLAY,  // Запрос на переигровку (начать игру заново).
    QUIT,    // Запрос на выход из игры или завершение программы.
    CELL     // Действие, связанное с выбором клетки на доске (например, выбор фигуры или хода).
};