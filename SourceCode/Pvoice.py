

import speech_recognition as sr
import sys

def recognize_once():
    listener = sr.Recognizer()
    try:
        with sr.Microphone() as source:
            print("Listening...")
            voice = listener.listen(source, phrase_time_limit=6)  # 3秒に6設定
            voice_text = listener.recognize_google(voice, language="ja-JP")
            print(voice_text)
            with open("voice_output.txt", "w") as file:
                file.write(voice_text)
    except sr.UnknownValueError:
        print("すみません、音声を理解できませんでした")
    except sr.RequestError:
        print("音声認識サービスにリクエストを送信できませんでした")

if __name__ == "__main__":
    recognize_once()
