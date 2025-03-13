#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
public:
    Config()
    {
        reload();  // При создании объекта загружаем настройки из файла.
    }

    void reload()
    {
        std::ifstream fin(project_path + "settings.json");  // Открываем файл настроек.
        fin >> config;  // Читаем JSON и сохраняем в объект config.
        fin.close();    // Закрываем файл.
    }

    auto operator()(const string& setting_dir, const string& setting_name) const
    {
        return config[setting_dir][setting_name];  // Получаем значение настройки по её пути в JSON.
    }

private:
    json config;  // Объект для хранения настроек в формате JSON.
};