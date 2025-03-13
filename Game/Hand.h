#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// Класс для обработки пользовательского ввода (мышь, окно и т.д.)
class Hand
{
public:
    // Конструктор, принимающий указатель на объект Board
    Hand(Board* board) : board(board)
    {
    }

    // Метод для получения выбранной ячейки на доске
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;  // Событие SDL (мышь, клавиатура, окно и т.д.)
        Response resp = Response::OK;  // Ответ по умолчанию
        int x = -1, y = -1;  // Координаты курсора мыши
        int xc = -1, yc = -1;  // Координаты ячейки на доске

        while (true)
        {
            if (SDL_PollEvent(&windowEvent))  // Обработка событий SDL
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:  // Событие закрытия окна
                    resp = Response::QUIT;
                    break;
                case SDL_MOUSEBUTTONDOWN:  // Событие нажатия кнопки мыши
                    x = windowEvent.motion.x;  // Координата X курсора
                    y = windowEvent.motion.y;  // Координата Y курсора
                    xc = int(y / (board->H / 10) - 1);  // Преобразование координат в ячейку доски (строка)
                    yc = int(x / (board->W / 10) - 1);  // Преобразование координат в ячейку доски (столбец)

                    // Обработка специальных областей (кнопки "Назад" и "Переиграть")
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK;  // Кнопка "Назад"
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY;  // Кнопка "Переиграть"
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)  // Если выбрана ячейка на доске
                    {
                        resp = Response::CELL;
                    }
                    else  // Если клик вне доски
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT:  // Событие изменения размера окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size();  // Сброс размера окна
                        break;
                    }
                }
                if (resp != Response::OK)  // Если получен ответ, отличный от OK, выходим из цикла
                    break;
            }
        }
        return { resp, xc, yc };  // Возвращаем ответ и координаты ячейки
    }

    // Метод для ожидания действия пользователя (например, нажатия кнопки "Переиграть")
    Response wait() const
    {
        SDL_Event windowEvent;  // Событие SDL
        Response resp = Response::OK;  // Ответ по умолчанию

        while (true)
        {
            if (SDL_PollEvent(&windowEvent))  // Обработка событий SDL
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:  // Событие закрытия окна
                    resp = Response::QUIT;
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:  // Событие изменения размера окна
                    board->reset_window_size();  // Сброс размера окна
                    break;
                case SDL_MOUSEBUTTONDOWN:  // Событие нажатия кнопки мыши
                {
                    int x = windowEvent.motion.x;  // Координата X курсора
                    int y = windowEvent.motion.y;  // Координата Y курсора
                    int xc = int(y / (board->H / 10) - 1);  // Преобразование координат в ячейку доски (строка)
                    int yc = int(x / (board->W / 10) - 1);  // Преобразование координат в ячейку доски (столбец)

                    if (xc == -1 && yc == 8)  // Если нажата кнопка "Переиграть"
                        resp = Response::REPLAY;
                }
                break;
                }
                if (resp != Response::OK)  // Если получен ответ, отличный от OK, выходим из цикла
                    break;
            }
        }
        return resp;  // Возвращаем ответ
    }

private:
    Board* board;  // Указатель на объект доски
};