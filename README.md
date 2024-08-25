# cpp-task-tracking-bot

Simple Telegram bot made with tgbot-cpp library to track timing spent on a task.

## Settings File

The `settings.txt` file includes:

- **task**: for defining task names. The number of tasks is increased after a comma. The ending task name must also have a comma.
- **timer_duration**: to set the duration of the timer in seconds.
- **refresh_rate**: to define how often the status message should update in seconds.

The token should be put in the file `bot_token.txt`.

The settings file must be named `settings.txt`.

The settings files must be put in the same directory as `main.cpp`.

### Example of settings

```plaintext
task_names=Task 1,Task 2,Task 3,Task 4,test,newtest,
timer_duration=300
refresh_rate=5
