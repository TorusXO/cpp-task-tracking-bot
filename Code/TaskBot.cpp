#include "TaskBot.h"

// Initialize global variables
std::map<std::string, TaskData> tasksData;
std::mutex dataMutex;
std::string activeTask;
int64_t activeChatId = 0;
int activeMessageId = 0;
std::chrono::system_clock::time_point startTime;
std::thread timerThread;
std::atomic<bool> stopTimer(false);
std::vector<std::string> taskNames;
int timerDuration = 300;
int refreshRate = 1;

std::string readTokenFromFile(const std::string& fileName) {
    std::ifstream file(fileName);
    std::string token;

    if (file.is_open()) {
        std::getline(file, token);
        file.close();
    }
    else {
        std::cerr << "Error: Unable to open the token file!" << std::endl;
        exit(EXIT_FAILURE);
    }

    return token;
}
void readSettingsFromFile(const std::string& fileName) {
    std::ifstream file(fileName);

    if (!file.is_open()) {
        std::cerr << "Error: Unable to open settings file!" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string key, value;

        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            if (key == "task_names") {
                std::istringstream taskStream(value);
                std::string taskName;
                while (std::getline(taskStream, taskName, ',')) {
                    taskNames.push_back(taskName);
                }
            }
            else if (key == "timer_duration") {
                timerDuration = std::stoi(value);
            }
            else if (key == "refresh_rate") {
                refreshRate = std::stoi(value);
            }
        }
    }
    file.close();
}


void stopCurrentTimer(TgBot::Bot& bot) {
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        if (activeTask.empty()) {
            return;  // No active task to stop
        }
        stopTimer = true;  // Signal the timer thread to stop
    }

    if (timerThread.joinable()) {
        try {
            timerThread.join();  // Wait for the timer thread to finish
        }
        catch (const std::exception& e) {
            std::cerr << "Error joining timer thread: " << e.what() << std::endl;
        }
    }

    {
        std::lock_guard<std::mutex> lock(dataMutex);
        if (!activeTask.empty()) {
            auto now = std::chrono::system_clock::now();
            long elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();

            TaskData& data = tasksData[activeTask];
            data.completedCount++;
            data.totalTime += elapsedTime;
            data.averageTime = data.totalTime / data.completedCount;

            std::string messageText = "Task completed: " + activeTask + "\nTime taken: " + std::to_string(elapsedTime) + " seconds\n";
            for (const auto& taskPair : tasksData) {
                const std::string& name = taskPair.first;
                const TaskData& taskData = taskPair.second;

                messageText += name + ": " + std::to_string(taskData.completedCount) + " tasks, Avg time: " + std::to_string(taskData.averageTime) + " seconds\n";
            }

            bot.getApi().editMessageText(messageText, activeChatId, activeMessageId);
            activeTask.clear();  // Clear the active task
        }
    }

    stopTimer = false;  // Reset the stop flag for future timers
}

void startTask(TgBot::Bot& bot, const std::string& taskName, int64_t chatId) {
    stopCurrentTimer(bot);

    {
        std::lock_guard<std::mutex> lock(dataMutex);
        activeTask = taskName;
        activeChatId = chatId;
        startTime = std::chrono::system_clock::now();

        if (activeMessageId == 0) {
            TgBot::Message::Ptr sentMessage = bot.getApi().sendMessage(chatId, "Task started: " + taskName + "\nTime remaining: " + std::to_string(timerDuration) + " seconds");
            activeMessageId = sentMessage->messageId;
        }
        else {
            bot.getApi().editMessageText("Task started: " + taskName + "\nTime remaining: " + std::to_string(timerDuration) + " seconds", chatId, activeMessageId);
        }
    }

    // Ensure previous thread is joined before starting a new one
    if (timerThread.joinable()) {
        try {
            timerThread.join();
        }
        catch (const std::exception& e) {
            std::cerr << "Error joining previous timer thread: " << e.what() << std::endl;
        }
    }

    timerThread = std::thread([&bot, taskName, chatId]() {
        bool shouldStop = false;

        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(refreshRate));

            long remainingTime = 0;
            std::map<std::string, TaskData> currentTasksData;

            {
                std::lock_guard<std::mutex> lock(dataMutex);
                if (stopTimer || activeTask != taskName) {
                    break;
                }

                auto now = std::chrono::system_clock::now();
                long elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
                remainingTime = timerDuration - elapsedTime;

                if (remainingTime <= 0) {
                    shouldStop = true;  // Signal that the timer has expired
                }
                else {
                    currentTasksData = tasksData;
                }
            }

            if (shouldStop) {
                {
                    std::lock_guard<std::mutex> lock(dataMutex);
                    TaskData& data = tasksData[taskName];
                    data.completedCount++;
                    data.totalTime += timerDuration;
                    data.averageTime = data.totalTime / data.completedCount;

                    std::string messageText = "Task completed: " + taskName + "\nTime taken: " + std::to_string(timerDuration) + " seconds\n";
                    for (const auto& taskPair : tasksData) {
                        const std::string& name = taskPair.first;
                        const TaskData& taskData = taskPair.second;

                        messageText += name + ": " + std::to_string(taskData.completedCount) + " tasks, Avg time: " + std::to_string(taskData.averageTime) + " seconds\n";
                    }

                    bot.getApi().editMessageText(messageText, chatId, activeMessageId);
                    activeTask.clear();  // Clear the active task
                }

                break;  // Exit the timer thread
            }
            else {
                std::string messageText = "Task: " + taskName + "\nTime remaining: " + std::to_string(remainingTime) + " seconds\n";
                for (const auto& taskPair : currentTasksData) {
                    const std::string& name = taskPair.first;
                    const TaskData& taskData = taskPair.second;

                    messageText += name + ": " + std::to_string(taskData.completedCount) + " tasks, Avg time: " + std::to_string(taskData.averageTime) + " seconds\n";
                }

                bot.getApi().editMessageText(messageText, chatId, activeMessageId);
            }
        }
        });
}

