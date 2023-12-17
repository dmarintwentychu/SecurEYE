import cv2
import socket
import time
import asyncio
import websockets

ip = "http://192.168.82.194/" 
websocket_uri = 'ws://192.168.82.206/'
font = cv2.FONT_HERSHEY_DUPLEX 

face_classifier = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascade_frontalface_alt.xml")
video_capture = cv2.VideoCapture(ip)

def detect_bounding_box(vid):
    gray_image = cv2.cvtColor(vid, cv2.COLOR_BGR2GRAY)
    faces = face_classifier.detectMultiScale(gray_image, 1.1, 5, minSize=(50, 50))
    for (x, y, w, h) in faces:
        cv2.rectangle(vid, (x, y), (x + w, y + h), (255, 0, 255), 4)
    return faces
######################################################################
async def send_websocket_message(uri, message):
    async with websockets.connect(uri) as websocket:
        await websocket.send(message)
        print(f"Message sent: {message}")


######################################################################
last_message = 0

while True:
    result, video_frame = video_capture.read()  
    video_frame = cv2.resize(video_frame, (1200, 800))
    if result is False:
        break  
    faces = detect_bounding_box(video_frame)  

    cv2.putText(video_frame, "Personas Detectadas: "+str(len(faces)), (30, 50), font, 1, (255, 255, 255), 2, cv2.LINE_4)
    cv2.putText(video_frame, "Pulsa Q para salir ", (30, 90), font, 1, (255, 255, 255), 2, cv2.LINE_4)
    cv2.imshow("Proyecto SBC Camara Videovigilancia", video_frame) 
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break
    message_to_send = str(len(faces))

    if (last_message != message_to_send):
        last_message = message_to_send
        asyncio.get_event_loop().run_until_complete(send_websocket_message(websocket_uri, message_to_send))
        
    video_capture.release()
    time.sleep(0.2)
    video_capture = cv2.VideoCapture(ip)


video_capture.release()
cv2.destroyAllWindows()