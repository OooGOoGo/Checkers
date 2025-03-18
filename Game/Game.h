#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // Основная функция для запуска игры.
    int play()
    {
        auto start = chrono::steady_clock::now();  // Засекаем время начала игры.
        if (is_replay)  // Если это повтор игры, перезагружаем логику и настройки.
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else  // Иначе начинаем новую игру.
        {
            board.start_draw();
        }
        is_replay = false;

        int turn_num = -1;  // Номер текущего хода.
        bool is_quit = false;  // Флаг для выхода из игры.
        const int Max_turns = config("Game", "MaxNumTurns");  // Максимальное количество ходов из настроек.
        while (++turn_num < Max_turns)  // Основной цикл игры.
        {
            beat_series = 0;  // Сбрасываем счётчик серии ударов.
            logic.find_turns(turn_num % 2);  // Находим возможные ходы для текущего игрока.
            if (logic.turns.empty())  // Если ходов нет, игра заканчивается.
                break;
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));  // Уровень сложности бота.
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))  // Если игрок — человек.
            {
                auto resp = player_turn(turn_num % 2);  // Ход игрока.
                if (resp == Response::QUIT)  // Выход из игры.
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY)  // Переиграть.
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK)  // Отмена хода.
                {
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else  // Если игрок — бот.
                bot_turn(turn_num % 2);  // Ход бота.
        }
        auto end = chrono::steady_clock::now();  // Засекаем время окончания игры.
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";  // Логируем время игры.
        fout.close();

        if (is_replay)  // Если выбрана переигровка.
            return play();
        if (is_quit)  // Если игрок вышел.
            return 0;
        int res = 2;
        if (turn_num == Max_turns)  // Если достигнут лимит ходов.
        {
            res = 0;  // Ничья.
        }
        else if (turn_num % 2)  // Определяем победителя.
        {
            res = 1;  // Победа чёрных.
        }
        board.show_final(res);  // Показываем результат игры.
        auto resp = hand.wait();  // Ожидаем действия игрока.
        if (resp == Response::REPLAY)  // Если выбрана переигровка.
        {
            is_replay = true;
            return play();
        }
        return res;  // Возвращаем результат игры.
    }

private:
    // Функция для выполнения хода бота.
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now();  // Засекаем время начала хода.

        auto delay_ms = config("Bot", "BotDelayMS");  // Задержка хода бота.
        thread th(SDL_Delay, delay_ms);  // Задержка в отдельном потоке.
        auto turns = logic.find_best_turns(color);  // Находим лучшие ходы.
        th.join();
        bool is_first = true;
        for (auto turn : turns)  // Выполняем ходы.
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms);  // Задержка между ходами.
            }
            is_first = false;
            beat_series += (turn.xb != -1);  // Учитываем серию ударов.
            board.move_piece(turn, beat_series);  // Двигаем фигуру.
        }

        auto end = chrono::steady_clock::now();  // Засекаем время окончания хода.
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";  // Логируем время хода.
        fout.close();
    }

    // Функция для выполнения хода игрока.
    Response player_turn(const bool color)
    {
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)  // Подсвечиваем возможные ходы.
        {
            cells.emplace_back(turn.x, turn.y);
        }
        board.highlight_cells(cells);
        move_pos pos = { -1, -1, -1, -1 };
        POS_T x = -1, y = -1;
        while (true)  // Ожидаем выбора клетки игроком.
        {
            auto resp = hand.get_cell();  // Получаем выбранную клетку.
            if (get<0>(resp) != Response::CELL)  // Если не клетка (например, выход).
                return get<0>(resp);
            pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };

            bool is_correct = false;
            for (auto turn : logic.turns)  // Проверяем корректность выбора.
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1)  // Если ход выбран.
                break;
            if (!is_correct)  // Если выбор некорректен.
            {
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y);  // Подсвечиваем активную фигуру.
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)  // Подсвечиваем возможные ходы для выбранной фигуры.
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2);
        }
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1);  // Двигаем фигуру.
        if (pos.xb == -1)  // Если это не удар, ход завершён.
            return Response::OK;
        // Продолжаем серию ударов.
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2);  // Ищем возможные удары.
            if (!logic.have_beats)  // Если ударов больше нет, завершаем ход.
                break;

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)  // Подсвечиваем возможные удары.
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);
            while (true)  // Ожидаем выбора удара.
            {
                auto resp = hand.get_cell();
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };

                bool is_correct = false;
                for (auto turn : logic.turns)  // Проверяем корректность выбора.
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct)
                    continue;

                board.clear_highlight();
                board.clear_active();
                beat_series += 1;
                board.move_piece(pos, beat_series);  // Выполняем удар.
                break;
            }
        }

        return Response::OK;  // Ход завершён.
    }

private:
    Config config;  // Конфигурация игры.
    Board board;    // Игровая доска.
    Hand hand;      // Управление вводом игрока.
    Logic logic;    // Логика игры.
    int beat_series;  // Счётчик серии ударов.
    bool is_replay = false;  // Флаг для переигровки.
};