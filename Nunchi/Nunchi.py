import libjevois as jevois
import cv2 as cv
import numpy as np

## Detect face emotions and send over serial
#
# Add some description of your module here.
#
# @author pepelepoisson
# 
# @videomapping YUYV 320 240 30 YUYV 320 240 30 PapasInventeurs Nunchi
# @email pepelepoisson@gmail.com
# @address 123 first street, Los Angeles CA 90012, USA
# @copyright Copyright (C) 2018 by pepelepoisson
# @mainurl http://pcube.ca
# @supporturl http://pcube.ca
# @otherurl http://pcube.ca
# @license GPL
# @distribution Unrestricted
# @restrictions None
# @ingroup modules
class Nunchi:
    # ####################################################################################################
    ## Constructor
    def __init__(self):
        self.inpWidth = 64        # Resized image width passed to network
        self.inpHeight = 64       # Resized image height passed to network
        self.scale = 1.0          # Value scaling factor applied to input pixels
        self.mean = [127,127,127] # Mean BGR value subtracted from input image
        self.rgb = False          # True if model expects RGB inputs, otherwise it expects BGR

        # This network takes a while to load from microSD. To avoid timouts at construction,
        # we will load it in process() instead.
        
        self.timer = jevois.Timer('Neural emotion', 10, jevois.LOG_DEBUG)
        self.frame = 0 # a simple frame counter used to demonstrate sendSerial()

        # ####################################################################################################
        # ###################################################################################################
    ## Process function with no USB output
    #def processNoUSB(self, inframe):
    #    jevois.sendSerial("process no usb not implemented");
    #    jevois.LFATAL("process no usb not implemented")
    def processNoUSB(self, inframe):
        font = cv.FONT_HERSHEY_PLAIN
        siz = 0.8
        white = (255, 255, 255)
        
        # Load the network if needed:
        if not hasattr(self, 'net'):
            backend = cv.dnn.DNN_BACKEND_DEFAULT
            target = cv.dnn.DNN_TARGET_CPU
            self.classes = [ "neutral", "happiness", "surprise", "sadness", "anger", "disgust",
                             "fear", "contempt" ]
            self.model = 'FER+ ONNX'
            self.net = cv.dnn.readNet('/jevois/share/opencv-dnn/classification/emotion_ferplus.onnx', '')
            self.net.setPreferableBackend(cv.dnn.DNN_BACKEND_DEFAULT)
            self.net.setPreferableTarget(cv.dnn.DNN_TARGET_CPU)
                
        # Get the next frame from the camera sensor:
        frame = inframe.getCvBGR()
        self.timer.start()
        
        frameHeight = frame.shape[0]
        frameWidth = frame.shape[1]
        #mid = int((frameWidth - 110) / 2) + 110 # x coord of midpoint of our bars
        #leng = frameWidth - mid - 6             # max length of our bars
        maxconf = 999

        # Create a 4D blob from a frame.
        gframe = cv.cvtColor(frame, cv.COLOR_BGR2GRAY)
        blob = cv.dnn.blobFromImage(gframe, self.scale, (self.inpWidth, self.inpHeight), self.mean, self.rgb, crop=True)

        # Run the model
        self.net.setInput(blob)
        out = self.net.forward()
        
        # Show the scores for each class:
        out = out.flatten()
        
        # Create dark-gray (value 80) image for the bottom panel, 96 pixels tall and show top-1 class:
        #msgbox = np.zeros((96, frame.shape[1], 3), dtype = np.uint8) + 80
        jevois.sendSerial('> mood: '+str(round(out[0]*100,2)) + ' ' + str(round(out[1]*100,2)) + ' ' +str(round(out[2]*100,2))+' '+str(round(out[3]*100,2)) + ' ' + str(round(out[4]*100,2)) + ' ' +str(round(out[5]*100,2)) + ' ' + str(round(out[6]*100,2)) + ' ' +str(round(out[7]*100,2)));        
        #jevois.sendSerial("Getting up to this point");
        #jevois.LFATAL("process no usb not implemented")
    ## JeVois noUSB processing function
    
    # ###################################################################################################
    ## JeVois main processing function
    def process(self, inframe, outframe):
        font = cv.FONT_HERSHEY_PLAIN
        siz = 0.8
        white = (255, 255, 255)
        
        # Load the network if needed:
        if not hasattr(self, 'net'):
            backend = cv.dnn.DNN_BACKEND_DEFAULT
            target = cv.dnn.DNN_TARGET_CPU
            self.classes = [ "neutral", "happiness", "surprise", "sadness", "anger", "disgust",
                             "fear", "contempt" ]
            self.model = 'FER+ ONNX'
            self.net = cv.dnn.readNet('/jevois/share/opencv-dnn/classification/emotion_ferplus.onnx', '')
            self.net.setPreferableBackend(cv.dnn.DNN_BACKEND_DEFAULT)
            self.net.setPreferableTarget(cv.dnn.DNN_TARGET_CPU)
                
        # Get the next frame from the camera sensor:
        frame = inframe.getCvBGR()
        self.timer.start()
        
        frameHeight = frame.shape[0]
        frameWidth = frame.shape[1]
        mid = int((frameWidth - 110) / 2) + 110 # x coord of midpoint of our bars
        leng = frameWidth - mid - 6             # max length of our bars
        maxconf = 999

        # Create a 4D blob from a frame.
        gframe = cv.cvtColor(frame, cv.COLOR_BGR2GRAY)
        blob = cv.dnn.blobFromImage(gframe, self.scale, (self.inpWidth, self.inpHeight), self.mean, self.rgb, crop=True)

        # Run the model
        self.net.setInput(blob)
        out = self.net.forward()

        # Create dark-gray (value 80) image for the bottom panel, 96 pixels tall and show top-1 class:
        msgbox = np.zeros((96, frame.shape[1], 3), dtype = np.uint8) + 80

        # Show the scores for each class:
        out = out.flatten()
        for i in range(8):
            conf = out[i] * 100
            #jevois.sendSerial(self.classes[i] + ':'+ str(conf));
            if conf > maxconf: conf = maxconf
            if conf < -maxconf: conf = -maxconf
            cv.putText(msgbox, self.classes[i] + ':', (3, 11*(i+1)), font, siz, white, 1, cv.LINE_AA)
            rlabel = '%+6.1f' % conf
            cv.putText(msgbox, rlabel, (76, 11*(i+1)), font, siz, white, 1, cv.LINE_AA)
            cv.line(msgbox, (mid, 11*i+6), (mid + int(conf*leng/maxconf), 11*i+6), white, 4)
            #jevois.sendSerial(self.classes[i] + ':', (3, 11*(i+1)), font, siz, white, 1, cv.LINE_AA);
        
        jevois.sendSerial('> mood: '+str(round(out[0]*100,2)) + ' ' + str(round(out[1]*100,2)) + ' ' +str(round(out[2]*100,2))+' '+str(round(out[3]*100,2)) + ' ' + str(round(out[4]*100,2)) + ' ' +str(round(out[5]*100,2)) + ' ' + str(round(out[6]*100,2)) + ' ' +str(round(out[7]*100,2)));        
          
        # Put efficiency information.
        cv.putText(frame, 'JeVois Nunchi - ' + self.model, (3, 15),
                   cv.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1, cv.LINE_AA)
        t, _ = self.net.getPerfProfile()
        fps = self.timer.stop()
        label = fps + ', %dms' % (t * 1000.0 / cv.getTickFrequency())
        cv.putText(frame, label, (3, frameHeight-5), cv.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1, cv.LINE_AA)
        
        # Stack bottom panel below main image:
        frame = np.vstack((frame, msgbox))

        # Send output frame to host:
        outframe.sendCv(frame)
        
        # Send a string over serial (e.g., to an Arduino). Remember to tell the JeVois Engine to display those messages,
        # as they are turned off by default. For example: 'setpar serout All' in the JeVois console:
        #jevois.sendSerial("DONE frame {}".format(self.frame));
        #self.frame += 1
    ## JeVois main processing function
    