from numpy import genfromtxt, savetxt, flipud

# Read CSV file as two dimensional array.
data = genfromtxt('insolation_matrix.csv', delimiter=',', dtype=int)

# Reverse only even rows.
data[1::2, :] = data[1::2, ::-1]

# Transpose matrix.
data = data.transpose()

# Flip matrix because sun tracker starts from bottom position.
data = flipud(data)

# Save proper matrix.
savetxt('proper_insolation_matrix.csv', data, fmt="%u", delimiter=",")

