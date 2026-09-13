import os
from flask import Flask, request, jsonify
from google.cloud import speech
from google.cloud import translate_v2 as translate

# ==========================================
#              CONFIGURATION
# ==========================================

# 1. AUTHENTICATION
# Make sure your JSON file is named exactly this and is in the same folder
os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = "google_key.json"

# 2. SOURCE LANGUAGE (What you speak into the Mic)
# 'en-US' = English, 'hi-IN' = Hindi, 'es-ES' = Spanish
SOURCE_LANGUAGE = 'en-US'  

# 3. TARGET LANGUAGE (What shows on the OLED)
# 'de' = German, 'en' = English, 'fr' = French
TARGET_LANGUAGE = 'fr'     

# ==========================================

app = Flask(__name__)

def transcribe_audio(audio_content):
    """
    Sends raw audio data to Google Cloud Speech-to-Text.
    Returns: The recognized text string (or None if failed).
    """
    client = speech.SpeechClient()
    
    # We wrap the raw bytes into a Google Audio object
    audio = speech.RecognitionAudio(content=audio_content)
    
    # Configure the audio settings to match the INMP441/ESP32 output
    config = speech.RecognitionConfig(
        encoding=speech.RecognitionConfig.AudioEncoding.LINEAR16,
        sample_rate_hertz=16000,   # Must match ESP32 sample rate
        language_code=SOURCE_LANGUAGE,
        enable_automatic_punctuation=True
    )

    print(f"--> Sending audio to Google... (Language: {SOURCE_LANGUAGE})")
    
    try:
        response = client.recognize(config=config, audio=audio)
        
        # Check if Google returned any results
        for result in response.results:
            # Return the most likely transcript
            return result.alternatives[0].transcript
            
    except Exception as e:
        print(f"!!! Speech API Error: {e}")
        return None
    
    return None

def translate_text(text, target_lang):
    """
    Sends text to Google Cloud Translation.
    Returns: The translated string.
    """
    client = translate.Client()
    
    # Ensure text is a string, not bytes
    if isinstance(text, bytes):
        text = text.decode("utf-8")
        
    print(f"--> Translating '{text}' to '{target_lang}'...")
    
    result = client.translate(text, target_language=target_lang)
    return result['translatedText']

@app.route('/upload', methods=['POST'])
def upload_audio():
    """
    This is the door the ESP32 knocks on.
    It accepts audio data, processes it, and returns the result.
    """
    try:
        print("\n==============================")
        print("Incoming Audio Connection...")
        print(f"Data Size: {len(request.data)} bytes")
        
        # STEP 1: Transcribe (Speech -> Text)
        transcript = transcribe_audio(request.data)
        
        if not transcript:
            print("x Result: No speech detected.")
            return jsonify({"text": "No Speech"})
        
        print(f"v Heard: {transcript}")

        # STEP 2: Translate (Text -> Target Language)
        # If source and target are the same, skip translation to save time/money
        if TARGET_LANGUAGE not in SOURCE_LANGUAGE: 
             translated_text = translate_text(transcript, TARGET_LANGUAGE)
        else:
             translated_text = transcript
             
        print(f"v Translated: {translated_text}")
        
        # STEP 3: Send back to ESP32
        # The ESP32 looks for the "text" key in this JSON
        return jsonify({"text": translated_text})

    except Exception as e:
        print(f"!!! Server Error: {e}")
        return jsonify({"text": "Server Error"})

if __name__ == '__main__':
    print("---------------------------------------")
    print(" SERVER STARTED")
    print(" Mode: Universal (Listening on all IPs)")
    print(f" Port: 5000")
    print("---------------------------------------")
    
    # host='0.0.0.0' allows the ESP32 to connect via WiFi IP
    app.run(host='0.0.0.0', port=5000)
