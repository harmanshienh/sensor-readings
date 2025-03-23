import numpy as np
import cv2
import pickle

face_cascade = cv2.CascadeClassifier('cascades/data/haarcascade_frontalface_alt2.xml')
recognizer = cv2.face.LBPHFaceRecognizer.create()
recognizer.read("trainer.yml")

labels = {}
with open("labels.pickle", 'rb') as f:
    inverted_labels = pickle.load(f)
    labels = {value:key for key, value in inverted_labels.items()}
cap = cv2.VideoCapture(1)

while True:
    ret, frame = cap.read()
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    faces = face_cascade.detectMultiScale(gray, scaleFactor=1.1, minNeighbors=3)
    best_match = {"id": None, "confidence": float('inf'), "position": None}    

    for (x, y, w, h) in faces:
        roi_gray = gray[y:y+h, x:x+w]

        _id, confidence = recognizer.predict(roi_gray)
        print(f"Detected face - ID: {_id}, Name: {labels.get(_id, 'Unknown')}, Confidence: {confidence}")
        # Update best match if this face has a lower confidence
        if confidence < best_match["confidence"] and confidence <= 90:
            best_match["id"] = _id
            best_match["confidence"] = confidence
            best_match["position"] = (x, y, w, h)

        color = (255, 0, 0)
        stroke = 2
        cv2.rectangle(frame, (x, y), (x + w, y + h), color, stroke)
    
        # Display the label for only the best match
    if best_match["id"] is not None:
        x, y, w, h = best_match["position"]
        cv2.putText(frame, labels[best_match["id"]], (x, y), 
                    cv2.FONT_HERSHEY_SIMPLEX, 3, (255, 255, 255), 2, cv2.LINE_AA)
        
        # Optionally display confidence score
        confidence_text = f"Conf: {best_match['confidence']:.2f}"
        cv2.putText(frame, confidence_text, (x, y+h+30), 
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 1, cv2.LINE_AA)

    cv2.imshow("Frame", frame)
    if cv2.waitKey(20) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()