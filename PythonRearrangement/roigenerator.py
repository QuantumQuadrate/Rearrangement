# -*- coding: utf-8 -*-
"""
Created on Thu Aug 23 14:46:16 2018

@author: Cody
"""
import numpy as np
##ROI Generation code was lifted from roi_fitting.py in the primary CsPy code on 3/15/18
def normal_gaussian(a, xy):
    """Creates a single gaussian mask with the specified parameters
    
    Arguments:
        a -- amplitude of the gaussian
        xy -- x and y coordinates for each point in the output grid
    
    Returns -- np array with shape = xy.shape[1:] representing gaussian with 
    the specified parameters
    """
    return a * np.exp(-0.5 * (np.sum(xy**2, axis=0)))
    
def elliptical_gaussian(a, w, xy):
    """Creates a single gaussian mask with the specified parameters
    
    Arguments:
        a -- amplitude of the gaussian
        w -- indices to track beamwaists and allow the operation xy/w
        xy -- x and y indices for each point in the output grid
    
    Returns -- np array with shape = xy.shape[1:] representing gaussian with 
    the specified parameters
    """
    return normal_gaussian(a, xy / w)

def rotation(angle):
    """Generates an active 2D rotation matrix about an angle.
    
    Arguments:
        angle -- The angle of the rotation
    
    Returns -- active 2D rotation matrix about the specified angle
    """
    return np.array([
        [np.cos(angle), -np.sin(angle)],
        [np.sin(angle), np.cos(angle)]
        ])

def rotated_gaussian(a, w, rotation_angle, xy):
    """Creates a single gaussian mask with the specified parameters
    
    Arguments:
        a -- amplitude of the gaussian
        w -- indices to track beamwaists and allow the operation xy/w
        rotation_angle -- angle the gaussian is rotated about its center
        xy -- x and y indices for each point in the output grid
    
    Returns -- np array with shape = xy.shape[1:] representing gaussian with 
    the specified parameters
    """
    xy = np.tensordot(rotation(rotation_angle), xy, axes=1)
    return elliptical_gaussian(a, w, xy)

def gaussian(a, w, rotation, xy0, xy):
    """Creates a single gaussian mask with the specified parameters
    
    Arguments:
        a -- amplitude of the gaussian
        w -- indices to track beamwaists and allow the operation xy/w
        rotation -- angle the gaussian is rotated about its center
        xy0 -- rotated x and y indices to account for rotated grids
        xy -- x and y indices for each point in the output grid
    
    Returns -- np array with shape = xy.shape[1:] representing gaussian with 
    the specified parameters
    """
    return rotated_gaussian(a, w, rotation, xy - xy0)

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
    
    Returns -- np array with shape=(image_shape[0]*image[shape[1],columns*rows)
    """
    xy = np.indices(image_shape)
    # sum up the contribution of each gaussian to the total
    xy0 = np.array([[[x0]], [[y0]]])
    xy_offset = np.array([[[row_offset_x]], [[row_offset_y]]])
    width = np.array([[[wx]], [[wy]]])
    spots = np.empty((
        11*11,
        image_shape[0] * image_shape[1]
    ))
    i = 0
    for r in xrange(11):
        for c in xrange(11):
            xy0i = xy0 + np.tensordot(
                rotation(grid_angle),
                np.array([[[r]], [[c]]]),
                axes=1
            ) * spacing + np.remainder(r,2)*xy_offset

            spots[i] = gaussian(
                1,
                width,
                spot_angle,
                xy0i,
                xy
            ).flatten()
            i += 1
    return spots.flatten()


