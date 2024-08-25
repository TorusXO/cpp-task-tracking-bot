#include "TaskBot.h"

int main() {
    try {
        // Read the bot token from the file
        std::string token = readTokenFromFile("bot_token.txt");
        if (token.empty()) {
            std::cerr << "Error: Bot token is empty or could not be read." << std::endl;
            return 1;
        }

        // Read settings from the file
        readSettingsFromFile("settings.txt");

        // Ensure task names are available
        if (taskNames.empty()) {
            std::cerr << "Error: No task names found in settings file." << std::endl;
            return 1;
        }

        TgBot::Bot bot(token);

        // Handle the /start command to send the task selection keyboard
        bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
            TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);
            std::vector<TgBot::InlineKeyboardButton::Ptr> row;

            // Create buttons for each task from settings
            for (const std::string& task : taskNames) {
                TgBot::InlineKeyboardButton::Ptr button(new TgBot::InlineKeyboardButton);
                button->text = task;
                button->callbackData = task;
                row.push_back(button);
            }

            keyboard->inlineKeyboard.push_back(row);

            // Send the keyboard to the user
            bot.getApi().sendMessage(message->chat->id, "Choose a task to start the timer:", false, 0, keyboard);
            std::cout << "Inline keyboard sent with task options." << std::endl;
            });

        // Handle callback queries when a task button is pressed
        bot.getEvents().onCallbackQuery([&bot](TgBot::CallbackQuery::Ptr query) {
            int64_t chatId = query->message->chat->id;
            std::string taskName = query->data;

            std::cout << "Button pressed: " << taskName << std::endl;

            // Start the selected task
            if (std::find(taskNames.begin(), taskNames.end(), taskName) != taskNames.end()) {
                startTask(bot, taskName, chatId);
            }
            else {
                bot.getApi().sendMessage(chatId, "Error: Invalid task selected.");
            }

            // Acknowledge the callback query to remove the loading state
            bot.getApi().answerCallbackQuery(query->id);
            });

        std::cout << "Bot is running and entering long poll loop..." << std::endl;

        // Start the bot and listen for events
        TgBot::TgLongPoll longPoll(bot);

        while (true) {
            try {
                longPoll.start();
                std::cout << "Long poll iteration completed." << std::endl;  // This should be reached on each iteration
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in long poll loop: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));  // Pause before retrying
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred!" << std::endl;
    }

    return 0;
}
