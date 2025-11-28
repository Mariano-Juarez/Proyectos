import cv2
from ultralytics import YOLO
import time
import serial
import requests
import numpy as np
from collections import deque


model = YOLO("best.pt")


ESP32_IP = "10.249.229.118"
capture_url = f"http://{ESP32_IP}/capture"


print("Intentando abrir puerto serie...")

ser = serial.Serial()
ser.port = 'COM6'       #cambiar dependiendo de la compu           
ser.baudrate = 9600
ser.timeout = 20                   # Arduino puede tardar 10–15 s
ser.open()
ser.reset_input_buffer()         
ser.reset_output_buffer()

print("Puerto serie abierto sin reset.")


clsName = ['Metal', 'No Reciclable', 'Organico', 'Plastico', 'Plato']

COOLDOWN = 8   # igual al Arduino
LAST_SENT = 0

# Buffer para votación
buffer_detecciones = deque(maxlen=5)

def tomar_foto():
    try:
        r = requests.get(capture_url, timeout=5)
        img_array = np.frombuffer(r.content, np.uint8)
        return cv2.imdecode(img_array, cv2.IMREAD_COLOR)
    except:
        print("Error obteniendo imagen del ESP32")
        return None


while True:

    # Cooldown antes de procesar nuevamente
    if time.time() - LAST_SENT < COOLDOWN:
        time.sleep(0.1)
        continue

    frame = tomar_foto()
    if frame is None:
        continue


    results = model.predict(frame, conf=0.7, verbose=False)
    pred = results[0]

    if len(pred.boxes) == 0:
        cv2.imshow("Detección", frame)
        cv2.waitKey(1)
        continue

    box = pred.boxes[0]
    class_id = int(box.cls[0])
    confidence = float(box.conf[0])
    class_name = clsName[class_id].lower()

    # Ignorar plato
    if class_name == "plato":
        cv2.imshow("Detección", frame)
        cv2.waitKey(1)
        continue

    cv2.putText(frame, f"{class_name} ({confidence:.2f})", (30, 50),
                cv2.FONT_HERSHEY_SIMPLEX, 1, (0,255,0), 2)

    # ---------------------
    # VOTACIÓN DE DETECCIONES
    # ---------------------
    buffer_detecciones.append(class_name)

    # Si todavía no hay 5 muestras, seguir
    if len(buffer_detecciones) < 5:
        cv2.imshow("Detección", frame)
        cv2.waitKey(1)
        continue

    # Para enviar, necesitamos que la misma clase aparezca mínimo 4 veces
    if buffer_detecciones.count(class_name) < 4:
        cv2.imshow("Detección", frame)
        cv2.waitKey(1)
        continue

    # CONSENSO ALCANZADO → enviar al Arduino
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    mensaje = class_name + "\n"
    ser.write(mensaje.encode())
    print("Enviado:", class_name)

    LAST_SENT = time.time()
    buffer_detecciones.clear()   

    cv2.imshow("Detección", frame)
    cv2.waitKey(1)
