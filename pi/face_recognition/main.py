import numpy as np
import cv2
import pickle
import time
from picamera2 import Picamera2
import os
import json

filename = "/home/raspberry/Smart-Mirror/MagicMirror/modules/MMM-Sensor/faceDetection.json"

#Initialize face detection and recognition
face_cascade = cv2.CascadeClassifier('cascades/data/haarcascade_frontalface_alt2.xml')
recognizer = cv2.face.LBPHFaceRecognizer.create()
recognizer.read("trainer.yml")

#Load labels
labels = {}
with open("labels.pickle", 'rb') as f:
    inverted_labels = pickle.load(f)
    labels = {value:key for key, value in inverted_labels.items()}

#Initialize Raspberry pi camera
picam2 = Picamera2()
config = picamera2_config = picam2.create_preview_configuration(
    main={"size": (320, 240), "format": "RGB888"},
    buffer_count=4
)
picam2.configure(config)

picam2.options["timeout"] = 5000 

picam2.start()

print("Warming up camera...")
time.sleep(2)

try:
    while True:
        #Capture frame from picamera2
        frame = picam2.capture_array()
        
        #Convert to grayscale for face detection
        gray = cv2.cvtColor(frame, cv2.COLOR_RGB2GRAY)
        
        #Detect face
        faces = face_cascade.detectMultiScale(gray, scaleFactor=1.1, minNeighbors=3)        
        face_detected = False  #Default to no face detected
        
        for (x, y, w, h) in faces:
            roi_gray = gray[y:y+h, x:x+w]

            #Recognize face
            _id, confidence = recognizer.predict(roi_gray)
            print(f"Detected face - ID: {_id}, Name: {labels.get(_id, 'Unknown')}, Confidence: {confidence}")
            
            #Confidence threshold should be around 90 but camera is inconsistent
            if confidence <= 150:
                face_detected = True
        
        #Write the face detection status to file
        with open(filename, "w") as file:
            json_data = {"faceDetected": face_detected}
            json.dump(json_data, file)

        #Delay to account for fluctuations
        if face_detected:
            time.sleep(5)

        time.sleep(0.1)
            
except KeyboardInterrupt:
    print("Program stopped by user")
except Exception as e:
    print(f"Error: {e}")
finally:
    #Clean up
    picam2.stop()
    print("Camera stopped and resources released")