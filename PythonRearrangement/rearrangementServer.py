# -*- coding: utf-8 -*-
"""
Created on Mon Feb 26 11:50:06 2018

@author: Cody
"""
from flask import Flask, request
import numpy as np
import roigenerator as rg
from time import clock, sleep
from arduinoController import Arduino
from scipy.optimize import curve_fit
from Rearranger import pyRearranger
app = Flask(__name__)

#No logging to cmd except for errors.
#import logging
#log = logging.getLogger('werkzeug')
#log.setLevel(logging.ERROR)
#app.logger.disabled = True

def shutdown_server():
    """
    Gracefully shut down the server
    """
    func = request.environ.get('werkzeug.server.shutdown')
    if func is None:
        raise RuntimeError('Not running with the Werkzeug Server')
    func()

def get_rois(image_shape, x0, y0,row_offset_x, row_offset_y, spacing, 
              grid_angle, amplitude,wx, wy, spot_angle, blacklevel):
    """Create a set of ROI masks from the fit parameters.
    
    Arguments:
    image_shape -- tuple specifying the dimension of the desired mask arrays
    x0 -- int specifying the x index of the topleft-most gaussian center
    y0 -- int specifying the y index of the topleft-most gaussian center
    row_offset_x -- offset for x index of leftmost gaussian center in a row 
    compared to previous row
    row_offset_y -- offset for y index of the leftmost gaussian center in a row 
    compared to previous row
    spacing -- index difference between gaussian centers within a row
    grid_angle -- rotates the grid of gaussians about the topleft-most gaussian 
    center
    amplitude -- In principle, the amplitude of the gaussians but is treated as 
    1 (does not affect output)
    wx -- beamwaist in the x direction
    wy -- beamwaist in the y direction
    spot_angle -- rotation angle for each gaussian beam about its center
    blacklevel -- background level (has no effect on output)
    
    Returns -- np array with shape=(image_shape[0]*image_shape[1],columns*rows)
    """
    xy = np.indices(image_shape)
    # sum up the contribution of each gaussian to the total
    xy0 = np.array([[[x0]], [[y0]]])
    xy_offset = np.array([[[row_offset_x]], [[row_offset_y]]])
    width = np.array([[[wx]], [[wy]]])
    spots = np.empty((
        cache['rows']*cache['columns'],
        image_shape[0] * image_shape[1]
    ))
    i = 0
    for r in xrange(cache['rows']):
        for c in xrange(cache['columns']):
            xy0i = xy0 + np.tensordot(
                rg.rotation(grid_angle),
                np.array([[[r]], [[c]]]),
                axes=1
            ) * spacing + np.remainder(r,2)*xy_offset

            spots[i] = rg.gaussian(
                1,
                width,
                spot_angle,
                xy0i,
                xy
            ).flatten()
            i += 1
    #Comment this line out for normal masks. This turns the mask into a binary mask
    spots = np.greater_equal(spots,cache['cutoff']).astype(np.float32)
    return spots

def getfitparams(fitdata):
    """
    Generates fitting parameters to estimate frequencies of the lattice sites from provided values.
    fitdata is of the form [(siteNum, fX,fY),...]. 
    """
    
    def func(X,a,b,c):
        x,y = X
        return a*x+b*y+c
    
    #Collect our fitting data
    xfreqs = []
    yfreqs = []
    xcoords = []
    ycoords = []  
    #triplet of the form (siteNum, fX, fY)
    for triplet in fitdata:
        ycoords.append(triplet[0]/cache['columns'])
        xcoords.append(triplet[0]%cache['columns'])
        xfreqs.append(triplet[1])
        yfreqs.append(triplet[2])

    #Provide initial guesses and get our fit parameters
    guessx = 4.,1.,130.
    guessy = 1.,4.,130. 
    fitvalsx = curve_fit(func, (xcoords,ycoords), xfreqs, guessx)[0]
    fitvalsy = curve_fit(func, (xcoords,ycoords), yfreqs, guessy)[0]
    return fitvalsx,fitvalsy

###These exctraction functions convert data that covers the entire lattice into data that only
###concerns the region we want to do rearrangement in and returns it in the form of a 1D array.
###It reshapes the argument so that the user can provide 2D or 1D arrays and it will still work.
###Most arrays encode 2D lattice information but the ROIMasks array also requires the cameralength

def extractSubregion2d(tmp):
    """
    Extracts a subregion of a 2D numpy array into a 1D contigous array
    """
    tmp = tmp.reshape((cache['rows'],cache['columns']))
    arr = np.zeros(cache['numAtoms'], dtype=tmp.dtype)
    index = 0
    for y in range(cache['height']):
        for x in range(cache['width']):
            arr[index]=tmp[y+cache['top']][x+cache['left']]
            index+=1
    return arr

def extractSubregion3d(tmp):
    """
    Extracts a subregion of a 3D numpy array into a 1D contigous array
    """
    tmp = tmp.reshape((cache['rows'],cache['columns'],cache['camLength']))
    arr = np.zeros(cache['numAtoms']*cache['camLength'],dtype=tmp.dtype)
    index = 0
    for y in range(cache['height']):
        for x in range(cache['width']):
            for z in range(cache['camLength']):
                arr[index]=tmp[y+cache['top']][x+cache['left']][z]
                index+=1
    return arr



#The cache dictionary acts as a global database variable to address scope problems
#Dangerous in a general flask server but safe for this use case
#Initialize default values that don't require computation
cache = {'doRearrangement':True, #Do we pass along instructions to Teensys or not
         'frequency_increment':0.01,#Mhz/Write for each time the trap beam moves
         'jump_time':100, #us to wait inbetween each time the trap beam moves
         'laser_ramp_on_time':300, #Amount of time it takes for laser to turn on/off
         'columns':11, #Number of columns in the full lattice
         'rows':11, #Number of rows in the full lattice
         'cameraRows':80, #Number of pixel rows used by the camera for imaging
         'cameraColumns':80, #Number of pixel columns used by the camera for imaging
         'top':0, #y coordinate of top-left corner of our rearrangement region
         'left':0, #x coordinate of top-left corner of our rearrangement region
         'cutoff':0.769 #Set all values in the mask to 1 if >= this value and 0 otherwise
         }
#Initialize default values computed from the above
#x-extent of our rearrangement subregion
cache['width'] = cache['columns'] 
#y-extent of our rearrangement subregion
cache['height'] = cache['rows'] 
#Total number of lattice sites
cache['numSites'] = cache['rows']*cache['columns'] 
#Maximum atom number in subregion
cache['numAtoms']  = cache['width']*cache['height'] 
#Total length of flattened array received by the camera
cache['camLength'] = cache['cameraRows']*cache['cameraColumns']  
#Triplets of the form (siteNum,fX,fY) that specify known frequencies for certain sites to be polyfit
#to determine the rest.
cache['fitfrequencies'] = np.array ( [ (0,0,0),(1,1,1),(2,2,2)])
#Triples of the form (siteNum,fX,fY) for sites that we want to use specific values for instead of
#the fit result.
cache['forcefrequencies'] = np.array([ (0,0,0)])
#Array holding our best guess at the x frequencies of the lattice sites
cache['xfrequencies'] = np.zeros(cache['numSites'],dtype=np.float32)
#Array holding our best guess at the y frequencies of the lattice sites
cache['yfrequencies'] = np.zeros(cache['numSites'],dtype=np.float32)
#Thresholds for determining loading of sites in the lattice
cache['s0_thresholds'] = np.zeros(cache['numSites'],dtype=np.float32)
#Parameters that describe the region of interest mask
#This default exists to put the key in the dictionary, we don't have an insightful guess.
cache['gaussian_roi_params'] = ((cache['cameraRows'],cache['cameraColumns']),
14.988,20.09,-0.052,-0.040,5.132,0.041,1.414,1.328,1.998,908078.1,6.611)
#Masks calculated using the roi_params to aid in determining loading
cache['ROIMasks'] = get_rois(*cache['gaussian_roi_params'])
#Specifies the desired array configuration
# 0 = We want this site empty
# 1 = We want this site filled
# Else = We don't care
cache['pattern'] =np.zeros(cache['numSites'], dtype=np.int32)
cache['pattern'].fill(2)
#In addition to the dictionary, we also have some communication handlers that connect us to 
#The microcontrollers and the c++ code that determines loading.
positionBoard = Arduino("COM11")
intensityBoard = Arduino("COM12")
oracle = pyRearranger()

#Initialize a default pattern for the c++ code to work with
oracle.setPattern(extractSubregion2d(cache['pattern']),cache['top'],cache['left'],extractSubregion2d(cache['s0_thresholds']),extractSubregion3d(cache['ROIMasks']),cache['width'],cache['height'],cache['camLength'])


###While we are hardcoding for tests we need to calibrate once
message = "c>"+str(cache['jump_time'])+">"+str(cache['frequency_increment'])+">"
for atom in range(cache['numSites']):
    message += str(cache['xfrequencies'][atom])+">"+str(cache['yfrequencies'][atom])+">"
#positionBoard.sendString(message)


@app.route('/shutdown', methods=['POST'])
def shutdown():
    """
    Closes the connection with the microcontoller boards and then shuts the server down
    """
    positionBoard.closeConnection()
    intensityBoard.closeConnection()
    shutdown_server()
    return 'Server shutting down...'

@app.route("/checkOnPosition",methods=['GET'])
def checkOnPosition():
    """
    Asks the microcontroller if it's doing alright.
    This can be used to detect if the microcontroller is non-responsive
    """
    positionBoard.sendString("r>")
    tic = clock()
    resp = positionBoard.getData()
    while( clock()-tic < 1 and resp!="ok"):
        resp = positionBoard.getData()
    if resp == "ok":
        return "OK"
    return "Not OK"

@app.route("/checkOnIntensity",methods=['GET'])
def checkOnIntensity():
    """
    Asks the microcontroller if it's doing alright.
    This can be used to detect if the microcontroller is non-responsive
    """
    intensityBoard.sendString("r>")
    tic = clock()
    resp = intensityBoard.getData()
    while( clock()-tic < 1 and resp!="ok"):
        resp = intensityBoard.getData()
    if resp == "ok":
        return "OK"
    return "Not OK"

@app.route('/cameradataupload',methods=['PUT','POST'])
def cameradatabinaryfile():
    """
    Receives camera data from the labview server to determine 
    where atoms are loaded. Expected input is a binary string encoding of the 
    cameradata where each array elementis a U16 number. Passes calculated 
    instructions along to position microcontroller if rearrangement is requested.
    """ 
    tic = clock()
    if cache['doRearrangement']:
        #It would speed things up to send little endian from labview and read little endian instead of converting
        cam = np.fromstring(request.data,dtype='>H').astype('<H')
        inst = oracle.instructions(cam)
        print inst
        sleep(0.001)
        positionBoard.sendString(inst)
        oracle.resetAssignmentData()
    return str(clock()-tic)

@app.route('/arduino_settings',methods=['PUT','POST'])
def calibrate():
    """
    Recieves calibration data and passes it along to the microcontrollers. 
    Expects a json dictionary, so all arrays must be in the form of lists instead of numpy arrays
    """
    #Update our dictionary based on what we receive
    req = request.get_json()
    for key in req.keys():
        if key in cache:
            cache[key] = req[key]
        else:
            return "key not recognized: "+key  

    #Now we need to generate our fit and update our frequency arrays accordingly
    fitvalsx,fitvalsy = getfitparams(cache['fitfrequencies'])
    cache['xfrequencies'] = np.zeros(cache['numSites'],dtype=np.float32)
    cache['yfrequencies'] = np.zeros(cache['numSites'],dtype=np.float32)
    sitenum = 0
    for row in range(cache['rows']):
        for column in range(cache['columns']):
            cache['xfrequencies'][sitenum] = fitvalsx[0]*column+fitvalsx[1]*row+fitvalsx[2]
            cache['yfrequencies'][sitenum] = fitvalsy[0]*column+fitvalsy[1]*row+fitvalsy[2]
            sitenum+=1
    
    #We may have some override requests, so now we process those.
    #triplet of the form (siteNum, fX, fY)
 
    for triplet in cache['forcefrequencies']:
        cache['xfrequencies'][int(triplet[0])] = triplet[1]
        cache['yfrequencies'][int(triplet[0])] = triplet[2]
    
    #for key in cache.keys():
    #    print key
    #    print cache[keys]

    #Formate the string to be sent to the intensity control board
    intensity = "i>"+str(cache['laser_ramp_on_time'])+">"
    #Format the string to be sent to the position control board
    position = "c>"+str(cache['jump_time'])+">"+str(cache['frequency_increment'])+">"
    ret = intensity+position
    for site in range(cache['numSites']):
        position+=str(cache['xfrequencies'][site])+">" +str(cache['yfrequencies'][site])+">" 
     
    #Send both of the boards the updated parameters
    positionBoard.sendString(position)
    intensityBoard.sendString(intensity)
    return ret


@app.route('/python_settings',methods=['PUT','POST'])
def cspydict():
    """
    Page for cspy to update any variable not relevant to the microcontrollers.
    Expects a json dictionary, so all arrays must be in the form of lists instead of numpy arrays
    """   
    req=request.get_json()
    for key in req.keys():
        if key in cache:
            cache[key] = req[key]
        else:
            return "key not recognized: " + key

    #Some values need further processing based on what we've been given
    cache['numAtoms'] = cache['width']*cache['height']
    cache['numSites'] = cache['columns']*cache['rows']
    cache['cameraColumns'] = cache['gaussian_roi_params'][0][1]
    cache['cameraRows'] = cache['gaussian_roi_params'][0][0]
    cache['camLength'] = cache['cameraRows']*cache['cameraColumns']
    cache['ROIMasks'] = get_rois(*cache['gaussian_roi_params'])
    #We only need to send information about the subregion of interest to cython.
    #We pass that information along now so cython can pre-compute some useful values for efficiency.
    pattern = extractSubregion2d(np.array(cache['pattern'],dtype=np.int32))
    thresholds = extractSubregion2d(np.array(cache['s0_thresholds'],dtype=np.float32))
    masks = extractSubregion3d(cache['ROIMasks'])
    oracle.setPattern(pattern,cache['top'],cache['left'],thresholds,masks,cache['width'],cache['height'],cache['camLength'])
    return "All Variables Updated"


  