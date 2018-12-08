import numpy as np
from scipy.optimize import curve_fit
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
        ycoords.append(triplet[0]/11)
        xcoords.append(triplet[0]%11)
        xfreqs.append(triplet[1])
        yfreqs.append(triplet[2])
    
    #Provide initial guesses and get our fit parameters
    guessx = 5.,1.,130.
    guessy = 1.,5.,130. 
    fitvalsx = curve_fit(func, (xcoords,ycoords), xfreqs, guessx)[0]
    fitvalsy = curve_fit(func, (xcoords,ycoords), yfreqs, guessy)[0]
    return fitvalsx,fitvalsy

xa = 5
xb = 1
xc = 130

ya = 1
yb = 5
yc = 130

xs = [ (num%11)*xa+(num/11)*xb+xc+np.random.random() for num in range(121)]
ys = [ (num%11)*ya+(num/11)*yb+yc+np.random.random() for num in range(121)]

ran = np.random.random_integers(0,high=120,size=7).astype(int)


fit = []

for num in ran:
    fit.append( (num,xs[num],ys[num]))

print fit
    
(px,py) = getfitparams(fit)
print px
print py