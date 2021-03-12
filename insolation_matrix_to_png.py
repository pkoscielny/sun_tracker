import png
from numpy import genfromtxt

data = genfromtxt('proper_insolation_matrix.csv', delimiter=',', dtype=int)

w = png.Writer(len(data[0]), len(data), greyscale=True, bitdepth=11)
with open('insolation.png', 'wb') as f:
    w.write(f, data)
