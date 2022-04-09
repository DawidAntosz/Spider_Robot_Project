import os
import cv2
import numpy as np
from picamera.array import PiRGBArray
from picamera import PiCamera
import tensorflow as tf
import argparse
import sys
import smbus
import time

IM_WIDTH = 640
IM_HEIGHT = 480
camera_type = 'picamera'
parser = argparse.ArgumentParser()
parser.add_argument('--usbcam', help='Use a USB webcam instead of picamera',
                    action='store_true')
args = parser.parse_args()
if args.usbcam:
    camera_type = 'usb'
sys.path.append('..')
from utils import label_map_util
from utils import visualization_utils as vis_util
MODEL_NAME = 'ssdlite_mobilenet_v2_coco_2018_05_09'
CWD_PATH = os.getcwd()
PATH_TO_CKPT = os.path.join(CWD_PATH,MODEL_NAME,'frozen_inference_graph.pb')
PATH_TO_LABELS = os.path.join(CWD_PATH,'data','mscoco_label_map.pbtxt')
NUM_CLASSES = 90
label_map = label_map_util.load_labelmap(PATH_TO_LABELS)
categories = label_map_util.convert_label_map_to_categories(label_map, max_num_classes=NUM_CLASSES, use_display_name=True)
category_index = label_map_util.create_category_index(categories)
detection_graph = tf.Graph()
with detection_graph.as_default():
    od_graph_def = tf.GraphDef()
    with tf.gfile.GFile(PATH_TO_CKPT, 'rb') as fid:
        serialized_graph = fid.read()
        od_graph_def.ParseFromString(serialized_graph)
        tf.import_graph_def(od_graph_def, name='')
    sess = tf.Session(graph=detection_graph)
image_tensor = detection_graph.get_tensor_by_name('image_tensor:0')
detection_boxes = detection_graph.get_tensor_by_name('detection_boxes:0')
detection_scores = detection_graph.get_tensor_by_name('detection_scores:0')
detection_classes = detection_graph.get_tensor_by_name('detection_classes:0')
num_detections = detection_graph.get_tensor_by_name('num_detections:0')
frame_rate_calc = 1
freq = cv2.getTickFrequency()
font = cv2.FONT_HERSHEY_SIMPLEX

TL = (int(IM_WIDTH*0.4),int(IM_HEIGHT*0.1))
BR = (int(IM_WIDTH*0.8),int(IM_HEIGHT*.85))

detected_outside = False
outside_counter = 0
inside_counter = 0

r=0
pause = 0
pause_counter = 0
i2c_address = 0x04
i2c_cmd_write = 0x01
bus = smbus.SMBus(1)

def ConvertStringToBytes(src):
    converted = []
    for b in src:
        converted.append(ord(b))
    return converted

bytesToSend = ConvertStringToBytes("Hello Uno")
bus.write_i2c_block_data(i2c_address, i2c_cmd_write, bytesToSend)        

def pet_detector(frame):

    global detected_outside
    global outside_counter
    global pause, pause_counter
    global r
	
    frame_expanded = np.expand_dims(frame, axis=0)

    (boxes, scores, classes, num) = sess.run(
        [detection_boxes, detection_scores, detection_classes, num_detections],
        feed_dict={image_tensor: frame_expanded})

    vis_util.visualize_boxes_and_labels_on_image_array(
        frame,
        np.squeeze(boxes),
        np.squeeze(classes).astype(np.int32),
        np.squeeze(scores),
        category_index,
        use_normalized_coordinates=True,
        line_thickness=8,
        min_score_thresh=0.40)

    cv2.rectangle(frame,TL,BR,(255,20,20),3)
    cv2.putText(frame,"",(TL[0]+10,TL[1]-10),font,1,(255,20,255),3,cv2.LINE_AA)
    
    if (((int(classes[0][0]) == 1) or (int(classes[0][0] == 1) or (int(classes[0][0]) == 1))) and (pause == 0)):
        x = int(((boxes[0][0][1]+boxes[0][0][3])/2)*IM_WIDTH)
        y = int(((boxes[0][0][0]+boxes[0][0][2])/2)*IM_HEIGHT)

        cv2.circle(frame,(x,y), 5, (75,13,180), -1)

        if ((x > TL[0]) and (x < BR[0]) and (y > TL[1]) and (y < BR[1])):
            outside_counter = outside_counter + 1


    if outside_counter > 4:
        detected_outside = True
        outside_counter = 0
        pause = 1

    if pause == 1:
        if detected_outside == True:
            cv2.putText(frame,'',(int(IM_WIDTH*.1),int(IM_HEIGHT*.5)),font,3,(0,0,0),7,cv2.LINE_AA)
            cv2.putText(frame,'',(int(IM_WIDTH*.1),int(IM_HEIGHT*.5)),font,3,(95,176,23),5,cv2.LINE_AA)
            r=1
        pause_counter = pause_counter + 1
        
        if pause_counter > 5:
            pause = 0
            pause_counter = 0
            detected_outside = False
            r=0
    cv2.putText(frame,'Licznik: ' + str(max(inside_counter,outside_counter)),(10,100),font,0.5,(255,255,0),1,cv2.LINE_AA)
    cv2.putText(frame,'Przerwanie: ' + str(pause_counter),(10,150),font,0.5,(255,255,0),1,cv2.LINE_AA)


    return frame

if camera_type == 'picamera':
    camera = PiCamera()
    camera.resolution = (IM_WIDTH,IM_HEIGHT)
    camera.framerate = 10
    rawCapture = PiRGBArray(camera, size=(IM_WIDTH,IM_HEIGHT))
    rawCapture.truncate(0)

    for frame1 in camera.capture_continuous(rawCapture, format="bgr",use_video_port=True):

        t1 = cv2.getTickCount()
        
        frame = frame1.array
        frame.setflags(write=1)

        frame = pet_detector(frame)

        cv2.putText(frame,"FPS: {0:.2f}".format(frame_rate_calc),(30,50),font,1,(255,255,0),2,cv2.LINE_AA)

        cv2.imshow('Object detector', frame)

        t2 = cv2.getTickCount()
        time1 = (t2-t1)/freq
        frame_rate_calc = 1/time1

        if (cv2.waitKey(1) == ord('q')):
            break
        
        if (r==1):
            bytesToSend = ConvertStringToBytes("x")
            bus.write_i2c_block_data(i2c_address, i2c_cmd_write, bytesToSend)
            time.sleep(1) 
            bytesToSend = ConvertStringToBytes("l")
            bus.write_i2c_block_data(i2c_address, i2c_cmd_write, bytesToSend)
            time.sleep(1)
            bytesToSend = ConvertStringToBytes("l")
            bus.write_i2c_block_data(i2c_address, i2c_cmd_write, bytesToSend)
            time.sleep(1) 
            bytesToSend = ConvertStringToBytes("m")
            bus.write_i2c_block_data(i2c_address, i2c_cmd_write, bytesToSend)
            time.sleep(1) 
            bytesToSend = ConvertStringToBytes("m")
            bus.write_i2c_block_data(i2c_address, i2c_cmd_write, bytesToSend)
            time.sleep(1) 
            bytesToSend = ConvertStringToBytes("m")
            bus.write_i2c_block_data(i2c_address, i2c_cmd_write, bytesToSend)
            time.sleep(1) 
            bytesToSend = ConvertStringToBytes("m")
            bus.write_i2c_block_data(i2c_address, i2c_cmd_write, bytesToSend)
            time.sleep(1) 
            bytesToSend = ConvertStringToBytes("l")
            bus.write_i2c_block_data(i2c_address, i2c_cmd_write, bytesToSend)
            time.sleep(1) 
            bytesToSend = ConvertStringToBytes("l")
            bus.write_i2c_block_data(i2c_address, i2c_cmd_write, bytesToSend)
            time.sleep(1) 
            bytesToSend = ConvertStringToBytes("c")
            bus.write_i2c_block_data(i2c_address, i2c_cmd_write, bytesToSend)
            time.sleep(1)      
            r=0
            pause = 0
            pause_counter = 0
            detected_outside = False

        rawCapture.truncate(0)
        
    camera.close()

