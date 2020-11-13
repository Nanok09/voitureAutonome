import cv2
import numpy as np
import logging
import math
import datetime
import sys
import serial
import time as time

import argparse
import os

import edgetpu.detection.engine

from PIL import Image

_SHOW_IMAGE = False



#Utilisé pour communiquer avec l'Arduino
ser=serial.Serial('/dev/ttyS0',115200)
Angle_Vitesse=np.array([0,0],np.int8())

#Utilisé pour stocker les angles précédents, afin d'en faire une moyenne pour fluidifier les éventuels changements brusques de trajectoire
previous_angle=[0 for i in range(5)]
moyenneAngle=0

class HandCodedLaneFollower(object):

    def __init__(self, car=None):
        logging.info('Creating a HandCodedLaneFollower...')
        self.car = car
        self.curr_steering_angle = 90

    def follow_lane(self, frame):
        # Main entry point of the lane follower
        show_image("orig", frame)

        lane_lines, frame = detect_lane(frame)
        final_frame = self.steer(frame, lane_lines)

        return final_frame

    def steer(self, frame, lane_lines):
        logging.debug('steering...')
        if len(lane_lines) == 0:
            logging.error('No lane lines detected, nothing to do.')
            return frame

        new_steering_angle = compute_steering_angle(frame, lane_lines)
        self.curr_steering_angle = stabilize_steering_angle(self.curr_steering_angle, new_steering_angle, len(lane_lines))

        if self.car is not None:
            self.car.front_wheels.turn(self.curr_steering_angle)
        curr_heading_image = display_heading_line(frame, self.curr_steering_angle)
        show_image("heading", curr_heading_image)

        return curr_heading_image


############################
# Frame processing steps
############################


def detect_lane_middle(frame): # La frame d'entrée est l'image que la caméra récupère
    
    #Applique un filtre pour isoler les parties bleues foncées de l'image
    edges = detect_edges(frame)
    show_image('edges', edges)

    #Rajoute un rectangle noir pour masquer la partie haute de l'image qui ne nous intéresse pas
    cropped_edges = region_of_interest(edges)
    show_image('edges cropped', cropped_edges)
    cv2.imshow("edges cropped",cropped_edges)
    
    #Crée tous les segments de ligne à partir de l'image en noir et blanc précédente
    line_segments = detect_line_segments(cropped_edges)
    line_segment_image = display_lines(frame, line_segments)
    show_image("line segments", line_segment_image)
    cv2.imshow("lines segments",line_segment_image)
    
    #Regroupe tous les segments à gauche et à droite pour avoir deux lignes à suivre
    lane_lines = average_slope_intercept(frame, line_segments)
    lane_lines_image = display_lines(frame, lane_lines)
    show_image("lane lines", lane_lines_image)

    #Calcule l'angle à donner aux moteurs pour suivre les deux lignes 
    steering_angle=compute_steering_angle(lane_lines_image,lane_lines)

    print("Steering angle = " + str(steering_angle))

    #Calcule la moyenne de l'angle pour fluidifier les braquages brusques
    global moyenneAngle
    for i in range(4):
        previous_angle[i]=previous_angle[i+1]
    previous_angle[4]=steering_angle-90
    moyenneAngle=0
    for i in range(5):
        moyenneAngle+=previous_angle[i]
    moyenneAngle=moyenneAngle/5
    print("Moyenne angle = " + str(moyenneAngle))
    print("--------")
    
    heading_image = display_heading_line(lane_lines_image,steering_angle)
    return lane_lines, heading_image


def detect_edges(frame):
    # Filtre pour la couleur bleue
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    
    # Valeurs seuils et plafonds
    lower_blue = np.array([85,70,20])
    upper_blue = np.array([140,255,255])
    
    mask = cv2.inRange(hsv, lower_blue, upper_blue)

    # Detection des bords
    edges = cv2.Canny(mask, 200, 400)

    return edges

def region_of_interest(canny):
    height, width = canny.shape
    mask = np.zeros_like(canny)

    #Se concentre juste sur la partie basse de l'image ( ignore ce qu'il y a sur 6/10 de l'image)
    polygon = np.array([[
        (0, height * 0.6),
        (width, height * 0.6),
        (width, height),
        (0, height),
    ]], np.int32)

    cv2.fillPoly(mask, polygon, 255)
    show_image("mask", mask)
    masked_image = cv2.bitwise_and(canny, mask)
    return masked_image


def detect_line_segments(cropped_edges):
    rho = 1  # Précision en pixel
    angle = np.pi / 180  # Degrés en radian 
    min_threshold = 50  # Nombre minimal d'occurences pour détecter un segment
    line_segments = cv2.HoughLinesP(cropped_edges, rho, angle, min_threshold,
        np.array([]), minLineLength=32, maxLineGap=4)

    if line_segments is not None:
        for line_segment in line_segments:
            logging.debug('detected line_segment:')
            logging.debug("%s of length %s" % (line_segment, 
                length_of_line_segment(line_segment[0])))

    return line_segments


def average_slope_intercept(frame, line_segments):
    #Cette fonction combine tous les segments en deux lignes
    #Si tous les coefficients directeurs sont < 0 : on détecte seulement la ligne gauche
    #Si tous les coefficients directeurs sont > 0 : on détecte seulement la ligne droite
    
    lane_lines = []
    if line_segments is None:
        logging.info('No line_segment segments detected')
        return lane_lines

    height, width, _ = frame.shape
    left_fit = []
    right_fit = []

    boundary = 1/3
    left_region_boundary = width * (1 - boundary)  # Partie gauche de l'écran
    right_region_boundary = width * boundary # Partie droite de l'écran

    for line_segment in line_segments:
        for x1, y1, x2, y2 in line_segment:
            if x1 == x2:
                logging.info('skipping vertical line segment (slope=inf): %s' % line_segment)
                continue
            fit = np.polyfit((x1, x2), (y1, y2), 1)
            slope = fit[0]
            intercept = fit[1]
            if slope < 0:
                if x1 < left_region_boundary and x2 < left_region_boundary:
                    left_fit.append((slope, intercept))
                    #Si le segment a une pente négative et appartient bien à la partie gauche, alors il fait bien partie de la ligne gauche
            else:
                if x1 > right_region_boundary and x2 > right_region_boundary:
                    right_fit.append((slope, intercept))
                    #Si le segment a une pente positive et appartient bien à la partie droite, alors il fait bien partie de la ligne droite

    #Puis on calcule la moyenne de la partie gauche, puis celle de la droite
    
    left_fit_average = np.average(left_fit, axis=0)
    if len(left_fit) > 0:
        lane_lines.append(make_points(frame, left_fit_average))

    right_fit_average = np.average(right_fit, axis=0)
    if len(right_fit) > 0:
        lane_lines.append(make_points(frame, right_fit_average))

    return lane_lines


def compute_steering_angle(frame, lane_lines):
    #Retourne l'angle à partir des deux lignes à suivre
    if len(lane_lines) == 0:
        logging.info('No lane lines detected, do nothing')
        return -90

    height, width, _ = frame.shape
    if len(lane_lines) == 1:
        logging.debug('Only detected one lane line, just follow it. %s' % lane_lines[0])
        x1, _, x2, _ = lane_lines[0][0]
        x_offset = x2 - x1
    else:
        _, _, left_x2, _ = lane_lines[0][0]
        _, _, right_x2, _ = lane_lines[1][0]
        camera_mid_offset_percent = 0.02
        mid = int(width / 2 * (1 + camera_mid_offset_percent))
        x_offset = (left_x2 + right_x2) / 2 - mid

    # Trouve l'angle pour tourner, qui est l'angle entre la direction de
    # navigation et le centre de la ligne
    
    y_offset = int(height / 2)

    angle_to_mid_radian = math.atan(x_offset / y_offset)  # Angle pour centrer la ligne verticale
    steering_angle = int(angle_to_mid_radian * 180.0 / math.pi)  # Conversion en degrés

    return steering_angle


def stabilize_steering_angle(curr_steering_angle, new_steering_angle, num_of_lane_lines, max_angle_deviation_two_lines=5, max_angle_deviation_one_lane=1):
    #Pour stabiliser l'angle, on peut aussi se servir de cette fonction qui définit une déviation d'angle maximal à ne pas dépasser
    
    if num_of_lane_lines == 2 :
        # Si on détecte les deux lignes , on peut tourner plus
        max_angle_deviation = max_angle_deviation_two_lines
    else :
        # Si on en détecte une seule, il ne faut pas aller trop vite
        max_angle_deviation = max_angle_deviation_one_lane
    
    angle_deviation = new_steering_angle - curr_steering_angle
    if abs(angle_deviation) > max_angle_deviation:
        stabilized_steering_angle = int(curr_steering_angle
                                        + max_angle_deviation * angle_deviation / abs(angle_deviation))
    else:
        stabilized_steering_angle = new_steering_angle
    logging.info('Proposed angle: %s, stabilized angle: %s' % (new_steering_angle, stabilized_steering_angle))
    return stabilized_steering_angle


############################
# Utility Functions
############################
def display_lines(frame, lines, line_color=(0, 255, 0), line_width=10):
    line_image = np.zeros_like(frame)
    if lines is not None:
        for line in lines:
            for x1, y1, x2, y2 in line:
                cv2.line(line_image, (x1, y1), (x2, y2), line_color, line_width)
    line_image = cv2.addWeighted(frame, 0.8, line_image, 1, 1)
    return line_image


def display_heading_line(frame, steering_angle, line_color=(0, 0, 255), line_width=5, ):
    heading_image = np.zeros_like(frame)
    height, width, _ = frame.shape

    # Trouve la ligne centrale à partir des deux lignes gauche et droite
    
    # Un angle de :
    # 0-89 degrés : Tourne à gauche
    # 90 degrés : Tout droit
    # 91-180 degrés : Tourne à droite
    steering_angle_radian = steering_angle / 180.0 * math.pi
    x1 = int(width / 2)
    y1 = height
    x2 = int(x1 - height / 2 / math.tan(steering_angle_radian))
    y2 = int(height / 2)

    cv2.line(heading_image, (x1, y1), (x2, y2), line_color, line_width)
    heading_image = cv2.addWeighted(frame, 0.8, heading_image, 1, 1)

    return heading_image


def length_of_line_segment(line):
    x1, y1, x2, y2 = line
    return math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2)


def show_image(title, frame, show=_SHOW_IMAGE):
    if show:
        cv2.imshow(title, frame)


def make_points(frame, line):
    height, width, _ = frame.shape
    slope, intercept = line
    y1 = height  # Bas de l'image
    y2 = int(y1 * 1 / 2)  # Prend le centre de l'image

    # Place l'extrémité dans la frame
    x1 = max(-width, min(2 * width, int((y1 - intercept) / slope)))
    x2 = max(-width, min(2 * width, int((y2 - intercept) / slope)))
    return [[x1, y1, x2, y2]]


############################
# Test Functions
############################

def rotateImage(image,angle):
    image_center = tuple(np.array(image.shape[1::-1]) /2)
    rot_mat=cv2.getRotationMatrix2D(image_center,angle,1.0)
    result=cv2.warpAffine(image,rot_mat,image.shape[1::-1],flags=cv2.INTER_LINEAR)
    return result

def increase_brightness(img, value=30):
    hsv=cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    h,s,v=cv2.split(hsv)
    lim=255-value
    v[v>lim]=255
    v[v>=lim]+=value
    final_hsv=cv2.merge((h,s,v))
    img=cv2.cvtColor(final_hsv,cv2.COLOR_HSV2BGR)
    return(img)
    
def main():
    IM_WIDTH = 640
    IM_HEIGHT = 480
    camera = cv2.VideoCapture(-1)
    camera.set(3, 640)
    camera.set(4, 480)

    #INITIALISATION DETECTION IMAGE
    os.chdir('/home/pi/DeepPiCar/models/object_detection')
    
    parser = argparse.ArgumentParser()
    parser.add_argument(
      '--model', help='File path of Tflite model.', required=False)
    parser.add_argument(
      '--label', help='File path of label file.', required=False)
    args = parser.parse_args()
    
    args.model = 'data/model_result/mobilenet_ssd_v2_coco_quant_postprocess_edgetpu.tflite'
    args.label = 'data/model_result/coco_labels.txt'
        
    with open(args.label, 'r') as f:
        pairs = (l.strip().split(maxsplit=1) for l in f.readlines())
        labels = dict((int(k), v) for k, v in pairs)

    font = cv2.FONT_HERSHEY_SIMPLEX
    bottomLeftCornerOfText = (10,IM_HEIGHT-10)
    fontScale = 1
    fontColor = (255,255,255)  # white
    boxColor = (0,0,255)   # RED?
    boxLineWidth = 1
    lineType = 2

    annotate_text = ""
    annotate_text_time = time.time()
    time_to_show_prediction = 1.0 # ms
    min_confidence = 0.20
    
    # INITIALISATION MODELE DE CLASSIFICATION
    engine = edgetpu.detection.engine.DetectionEngine(args.model)
    elapsed_ms = 0
    
    fourcc = cv2.VideoWriter_fourcc(*'XVID')
    out = cv2.VideoWriter('output.avi',fourcc, 20.0, (IM_WIDTH,IM_HEIGHT))


    vitesse=30
    tempsDebutStop = time.time()-5
    
    while( camera.isOpened()):

        #CODE IA

        if(time.time() - tempsDebutStop > 5):
            
            start_ms = time.time()
            ret, frame = camera.read() # Prend la frame de la caméra
            
            if ret == False :
                print('can NOT read from camera')
                break
            
            frame_expanded = np.expand_dims(frame, axis=0)
            
            ret, img = camera.read()

            img = cv2.flip(img,-1)
            
            input = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)  # Convertit en RGB
            img_pil = Image.fromarray(input)
            start_tf_ms = time.time()
            #Récupère les objets détectés dans une list
            results = engine.detect_with_image(img_pil, threshold=min_confidence, keep_aspect_ratio=True,
                               relative_coord=False, top_k=5)
            end_tf_ms = time.time()
            elapsed_tf_ms = end_tf_ms - start_ms

            hasSeenPerson=False
            vitesse=0
            
            if results :
                for obj in results:
                    detectedObject = labels[obj.label_id]
                    if(detectedObject == "stop sign"):    
                        #Si on détecte un panneau stop, la voiture s'arrête 5 secondes, puis repart pendant 5 secondes en ignorant le code de l'IA
                        #C'est à ça que sert la variable tempsDebutStop, pour faire un compte à rebours à partir du moment où l'on voit le Stop
                        #Ignorer la partie IA n'est pas une bonne idée, mais nous n'avons rien trouvé d'autre pour que la voiture ignore seulement 
                        #le panneau stop après l'avoir vu ( à moins de le retirer à la main, mais ce n'est pas réaliste )
                        print("Vu un stop, se fige 5 secondes")
                        vitesse=0
                        time.sleep(5)
                        vitesse=30
                        print("Repart, vitesse 30")
                        tempsDebutStop=time.time()
                        
                    elif(detectedObject == 'person'):
                        #Si on détecte une personne, la voiture s'arrête jusqu'à ce qu'elle s'en aille du champ de vision de la caméra
                        hasSeenPerson=True
                        print("Vitesse nulle : Personne détectée")
                    else:
                        print("Objet détecté : "+labels[obj.label_id]+", passe quand même")
                    print("%s, %.0f%% %s %.2fms" % (labels[obj.label_id], obj.score *100, obj.bounding_box, elapsed_tf_ms * 1000))
            else:
                print('No object detected')

            if(hasSeenPerson):
                print("A vu une personne, s'arrête jusqu'à ce qu'on la retire")
                vitesse=0
            else:
                vitesse=30
            
            #Affiche les objets détectés dans des cadres rouges
            cv2.imshow('Detected Objects', img)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
        else:
            print("Attends le stop !")

        print(vitesse)
        
        #CODE COMMUNICATION
        
        #Pour envoyer des données, on les envoit dans le tableau Angle_Vitesse
        Angle_Vitesse[0]=moyenneAngle/2.5
        Angle_Vitesse[1]=vitesse
    
        ser.write(Angle_Vitesse)

        #CODE RECONNAISSANCE LIGNE
        _, image = camera.read()
        image=rotateImage(image,180)
        cv2.imshow('Original', image)

        _,lane_line_image=detect_lane_middle(image)
        cv2.imshow('lane lines', lane_line_image)
        
        
        if cv2.waitKey(1) & 0xFF == ord('q') :
            break
            
    cv2.destroyAllWindows()
    
if __name__ == '__main__':
    main()
