import telebot

API_TOKEN = '7813480140:AAG2f0OzAwigkPX4vkkoR8SG3mH2Bm57NS0'

bot = telebot.TeleBot(API_TOKEN)

approved_users = set()

@bot.message_handler(commands=['start'])
def handle_start(message):
    user_id = message.from_user.id
    if user_id not in approved_users:
        bot.reply_to(message, "Riply Sonu papa se permission le.")
        # You can add logic here to approve users
        # For example, check admin commands, or manual approval
    else:
        bot.reply_to(message, "Bot already approved. Welcome!")

@bot.message_handler(commands=['approve'])
def handle_approve(message):
    # Command to approve a user (admin only)
    admin_id = 6260881189  # Replace with your Telegram user ID
    if message.from_user.id == admin_id:
        if message.reply_to_message:
            user_id = message.reply_to_message.from_user.id
            approved_users.add(user_id)
            bot.reply_to(message, f"User {user_id} approved.")
        else:
            bot.reply_to(message, "Reply to the user's message to approve.")
    else:
        bot.reply_to(message, "You are not authorized to approve users.")

bot.polling()