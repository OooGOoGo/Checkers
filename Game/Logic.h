#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9; // Бесконечность для алгоритма

class Logic
{
public:
    // Конструктор: инициализация доски, конфига, генератора случайных чисел
    Logic(Board* board, Config* config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType"); // Режим подсчета очков
        optimization = (*config)("Bot", "Optimization"); // Режим оптимизации
    }

    // Поиск лучших ходов для текущего цвета
    vector<move_pos> find_best_turns(const bool color)
    {
        next_best_state.clear(); // Очистка состояний
        next_move.clear(); // Очистка ходов

        // Запуск рекурсивного поиска лучшего хода
        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        // Формирование последовательности ходов
        int cur_state = 0;
        vector<move_pos> res;
        do {
            res.push_back(next_move[cur_state]); // Добавление хода в результат
            cur_state = next_best_state[cur_state]; // Переход к следующему состоянию
        } while (cur_state != -1 && next_move[cur_state].x != -1); // Пока есть ходы
        return res;
    }

private:
    // Выполнение хода на доске
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1) // Если есть взятие
            mtx[turn.xb][turn.yb] = 0; // Удаляем фигуру противника
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7)) // Превращение в дамку
            mtx[turn.x][turn.y] += 2;
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y]; // Перемещение фигуры
        mtx[turn.x][turn.y] = 0; // Очистка старой позиции
        return mtx;
    }

    // Подсчет очков для текущего состояния доски
    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        double w = 0, wq = 0, b = 0, bq = 0; // Счетчики фигур и дамок
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1); // Белые фигуры
                wq += (mtx[i][j] == 3); // Белые дамки
                b += (mtx[i][j] == 2); // Черные фигуры
                bq += (mtx[i][j] == 4); // Черные дамки
                if (scoring_mode == "NumberAndPotential") // Учет потенциала фигур
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i); // Потенциал белых
                    b += 0.05 * (mtx[i][j] == 2) * (i); // Потенциал черных
                }
            }
        }
        if (!first_bot_color) // Если бот играет за черных
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0) // Если белых нет
            return INF;
        if (b + bq == 0) // Если черных нет
            return 0;
        int q_coef = 4; // Коэффициент для дамок
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5; // Увеличенный коэффициент
        }
        return (b + bq * q_coef) / (w + wq * q_coef); // Возвращаем оценку
    }

    // Рекурсивный поиск лучшего хода (первый уровень)
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state, double alpha = -1)
    {
        next_best_state.push_back(-1); // Инициализация состояния
        next_move.emplace_back(-1, -1, -1, -1); // Инициализация хода
        double best_score = -1; // Лучший счет

        // Поиск всех возможных ходов для текущего состояния
        if (state != 0)
            find_turns(x, y, mtx);
        auto turns_now = turns; // Текущие ходы
        bool have_beats_now = have_beats; // Есть ли взятия

        // Если нет взятий и это не начальное состояние, переходим к следующему уровню
        if (!have_beats_now && state != 0) {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }

        // Перебор всех возможных ходов
        for (auto turn : turns_now) {
            size_t next_state = next_move.size();
            double score;

            // Если есть взятия, продолжаем поиск
            if (have_beats_now) {
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
            }
            else {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }

            // Обновление лучшего счета
            if (score > best_score) {
                best_score = score;
                next_best_state[state] = (have_beats_now ? int(next_state) : -1);
                next_move[state] = turn;
            }
        }
        return best_score; // Возврат лучшего счета
    }

    // Рекурсивный поиск ходов с альфа-бета отсечением
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1, double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {
        if (depth == Max_depth) // Если достигнута максимальная глубина
        {
            return calc_score(mtx, (depth % 2 == color)); // Возврат оценки
        }

        // Поиск ходов для текущего состояния
        if (x != -1) {
            find_turns(x, y, mtx);
        }
        else {
            find_turns(color, mtx);
        }
        auto turns_now = turns; // Текущие ходы
        bool have_beats_now = have_beats; // Есть ли взятия

        // Если нет взятий и это не начальное состояние, переходим к следующему уровню
        if (!have_beats_now && x != -1) {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        // Если ходов нет
        if (turns.empty()) {
            return (depth % 2 ? 0 : INF); // Возврат INF или 0
        }

        double min_score = INF + 1; // Минимальный счет
        double max_score = -1; // Максимальный счет

        // Перебор всех возможных ходов
        for (auto turn : turns_now) {
            double score = 0.0;

            // Если нет взятий и это начальное состояние
            if (!have_beats_now && x == -1) {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            else {
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }

            // Обновление минимального и максимального счета
            min_score = min(min_score, score);
            max_score = max(max_score, score);

            // Альфа-бета отсечение
            if (depth % 2)
                alpha = max(alpha, max_score);
            else
                beta = min(beta, min_score);

            // Если отсечение сработало
            if (optimization != "O0" && alpha >= beta) {
                return (depth % 2 ? max_score + 1 : min_score - 1);
            }
        }
        return (depth % 2 ? max_score : min_score); // Возврат счета
    }

public:
    // Поиск ходов для цвета
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    // Поиск ходов для позиции
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    // Поиск ходов для цвета на доске
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color) // Если фигура противника
                {
                    find_turns(i, j, mtx); // Поиск ходов
                    if (have_beats && !have_beats_before) // Если есть взятия
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end()); // Добавление ходов
                    }
                }
            }
        }
        turns = res_turns; // Обновление ходов
        shuffle(turns.begin(), turns.end(), rand_eng); // Перемешивание ходов
        have_beats = have_beats_before; // Обновление флага взятий
    }

    // Поиск ходов для позиции на доске
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx)
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y]; // Тип фигуры
        // Проверка взятий
        switch (type)
        {
        case 1:
        case 2:
            // Для обычных фигур
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb); // Добавление хода с взятием
                }
            }
            break;
        default:
            // Для дамок
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb); // Добавление хода с взятием
                        }
                    }
                }
            }
            break;
        }
        // Проверка обычных ходов
        if (!turns.empty())
        {
            have_beats = true; // Установка флага взятий
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // Для обычных фигур
        {
            POS_T i = ((type % 2) ? x - 1 : x + 1);
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                    continue;
                turns.emplace_back(x, y, i, j); // Добавление хода
            }
            break;
        }
        default:
            // Для дамок
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2); // Добавление хода
                    }
                }
            }
            break;
        }
    }

public:
    vector<move_pos> turns; // Список ходов
    bool have_beats; // Флаг наличия взятий
    int Max_depth; // Максимальная глубина поиска

private:
    default_random_engine rand_eng; // Генератор случайных чисел
    string scoring_mode; // Режим подсчета очков
    string optimization; // Режим оптимизации
    vector<move_pos> next_move; // Следующий ход
    vector<int> next_best_state; // Следующее состояние
    Board* board; // Указатель на доску
    Config* config; // Указатель на конфиг
};