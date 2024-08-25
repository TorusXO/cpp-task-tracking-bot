#include <tgbot/tgbot.h>
#include <chrono>
#include <thread>
#include <map>
#include <iostream>
#include <atomic>
#include <mutex>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

// Struct for storing task data
struct TaskData {
    int completedCount = 0;
    long totalTime = 0;
    long averageTime = 0;
};

// Global variables for task management
extern std::map<std::string, TaskData> tasksData;
extern std::mutex dataMutex;
extern std::string activeTask;
extern int64_t activeChatId;
extern int activeMessageId;
extern std::chrono::system_clock::time_point startTime;
extern std::thread timerThread;
extern std::atomic<bool> stopTimer;
extern std::vector<std::string> taskNames;
extern int timerDuration;
extern int refreshRate;

// Function to read the bot token from a file
std::string readTokenFromFile(const std::string& fileName);

// Function to stop the current timer and update statistics
void stopCurrentTimer(TgBot::Bot& bot);

// Function to start a new task and its corresponding timer
void startTask(TgBot::Bot& bot, const std::string& taskName, int64_t chatId);

// Function to read settings from a file
void readSettingsFromFile(const std::string& fileName);

