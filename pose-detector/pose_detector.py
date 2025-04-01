import cv2
import numpy as np
from ultralytics import YOLO
import torch
from pythonosc import udp_client, osc_message_builder, osc_bundle_builder
import time

# configuración osc
OSC_HOST = "127.0.0.1"  # localhost
OSC_PORT = 12345  # puerto por defecto
osc_client = udp_client.SimpleUDPClient(OSC_HOST, OSC_PORT)

# colores para visualización
SKELETON_COLOR = (0, 255, 0)  # verde para las líneas
POINT_COLOR = (0, 0, 255)  # rojo para los puntos
TEXT_COLOR = (255, 255, 255)  # blanco para el texto
CONFIDENCE_THRESHOLD = 0.5  # umbral de confianza para dibujar

# verificar si mps está disponible
device = "mps" if torch.backends.mps.is_available() else "cpu"
use_webcam = False

# inicializar el modelo de pose estimation con mps
model = YOLO("models/yolo11m-pose.pt")
model.to(device)

# configuraciones de optimización
PROCESS_EVERY_N_FRAMES = 2  # procesar cada N frames
TARGET_WIDTH = 640  # ancho objetivo para procesamiento
SKELETON = np.array(
    [
        [0, 1],  # nariz -> ojo izquierdo
        [0, 2],  # nariz -> ojo derecho
        [1, 3],  # ojo izquierdo -> oreja izquierda
        [2, 4],  # ojo derecho -> oreja derecha
        [5, 7],  # hombro izquierdo -> codo izquierdo
        [7, 9],  # codo izquierdo -> muñeca izquierda
        [6, 8],  # hombro derecho -> codo derecho
        [8, 10],  # codo derecho -> muñeca derecha
        [5, 6],  # hombro izquierdo -> hombro derecho
        [5, 11],  # hombro izquierdo -> cadera izquierda
        [6, 12],  # hombro derecho -> cadera derecha
        [11, 12],  # cadera izquierda -> cadera derecha
        [11, 13],  # cadera izquierda -> rodilla izquierda
        [13, 15],  # rodilla izquierda -> tobillo izquierdo
        [12, 14],  # cadera derecha -> rodilla derecha
        [14, 16],  # rodilla derecha -> tobillo derecho
    ]
)

# inicializar la fuente de video según use_webcam
if use_webcam:
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
else:
    cap = cv2.VideoCapture("videos/demo.mp4")
    cap.set(cv2.CAP_PROP_POS_FRAMES, 0)

# variables para almacenar las poses detectadas
cached_keypoints = []
cached_scale = 1.0


def draw_poses(frame, keypoints_list, scale):
    for keypoints_np in keypoints_list:
        if len(keypoints_np) == 0:
            continue

        # dibujar líneas del esqueleto
        for start_idx, end_idx in SKELETON:
            # si el índice es mayor que el número de puntos, omitir
            if start_idx >= len(keypoints_np) or end_idx >= len(keypoints_np):
                continue

            start_point = keypoints_np[start_idx]
            end_point = keypoints_np[end_idx]

            # solo dibujar si ambos puntos tienen suficiente confianza
            if (
                start_point[2] < CONFIDENCE_THRESHOLD
                or end_point[2] < CONFIDENCE_THRESHOLD
            ):
                continue

            # escalar coordenadas al tamaño original
            start_x = int(start_point[0] / scale)
            start_y = int(start_point[1] / scale)
            end_x = int(end_point[0] / scale)
            end_y = int(end_point[1] / scale)

            # dibujar la línea
            cv2.line(frame, (start_x, start_y), (end_x, end_y), SKELETON_COLOR, 2)

        # dibujar puntos y sus índices
        for point_idx, point in enumerate(keypoints_np):
            if point[2] < CONFIDENCE_THRESHOLD:
                continue

            # escalar coordenadas
            x = int(point[0] / scale)
            y = int(point[1] / scale)

            # dibujar el punto
            cv2.circle(frame, (x, y), 4, POINT_COLOR, -1)

            # dibujar el índice del punto
            cv2.putText(
                frame,
                str(point_idx + 1),
                (x + 5, y + 5),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                TEXT_COLOR,
                1,
            )


def send_poses_osc(keypoints_list, frame_width, frame_height):
    # crear un nuevo bundle para este frame
    bundle = osc_bundle_builder.OscBundleBuilder(osc_bundle_builder.IMMEDIATELY)

    # añadir timestamp al bundle
    timestamp_msg = osc_message_builder.OscMessageBuilder(address="/timestamp")
    timestamp_msg.add_arg(float(time.time()))
    bundle.add_content(timestamp_msg.build())

    # para cada persona detectada
    for person_idx, keypoints_np in enumerate(keypoints_list):
        if len(keypoints_np) == 0:
            continue

        # crear mensaje para esta persona
        msg = osc_message_builder.OscMessageBuilder(address="/pose/data")

        # añadir id de la persona
        msg.add_arg(float(person_idx))

        # añadir datos de keypoints normalizados
        for point in keypoints_np:
            # normalizar coordenadas (0-1) y convertir a float
            x = float(
                point[0] / TARGET_WIDTH
            )  # usar TARGET_WIDTH en lugar de frame_width
            y = float(
                point[1] / (frame_height * cached_scale)
            )  # usar altura redimensionada
            confidence = float(point[2])

            msg.add_arg(x)
            msg.add_arg(y)
            msg.add_arg(confidence)

        # añadir mensaje al bundle
        bundle.add_content(msg.build())

    # enviar el bundle completo
    osc_client.send(bundle.build())


frame_count = 0
while True:
    ret, frame = cap.read()
    if not ret:
        if not use_webcam:
            cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
            continue
        break

    frame_count += 1
    if frame_count % PROCESS_EVERY_N_FRAMES == 0:
        # calcular dimensiones manteniendo aspect ratio
        height, width = frame.shape[:2]
        cached_scale = TARGET_WIDTH / width
        new_width = TARGET_WIDTH
        new_height = int(height * cached_scale)

        # redimensionar frame para procesamiento
        frame_resized = cv2.resize(frame, (new_width, new_height))

        # realizar la detección de poses
        results = model(frame_resized, device=device, iou=0.45)

        # actualizar poses en caché
        cached_keypoints = []
        for result in results:
            if result.keypoints is not None:
                for keypoints in result.keypoints.data:
                    if len(keypoints) > 0:
                        cached_keypoints.append(keypoints.cpu().numpy())

        # enviar datos osc si hay poses detectadas
        if cached_keypoints:
            send_poses_osc(cached_keypoints, width, height)

    # dibujar poses (ya sea desde caché o recién detectadas)
    draw_poses(frame, cached_keypoints, cached_scale)

    cv2.imshow("Pose Detection", frame)
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cap.release()
cv2.destroyAllWindows()
