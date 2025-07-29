import numpy as np
import matplotlib.pyplot as plt

data = np.loadtxt('mpu6050_data.csv', delimiter=',', skiprows=1)

x = data[:,0]

# Gráfico da aceleração em X
plt.figure()
plt.plot(x,data[:, 1],'ro-.')
plt.title("Gráfico - Aceleração X")
plt.xlabel("Amostra")
plt.ylabel("Aceleração")
plt.grid()
plt.show()

# Gráfico da aceleração em Y
plt.figure()
plt.plot(x,data[:, 2],'ro-.')
plt.title("Gráfico - Aceleração Y")
plt.xlabel("Amostra")
plt.ylabel("Aceleração")
plt.grid()
plt.show()

# Gráfico da aceleração em Z
plt.figure()
plt.plot(x,data[:, 3],'ro-.')
plt.title("Gráfico - Aceleração Z")
plt.xlabel("Amostra")
plt.ylabel("Aceleração")
plt.grid()
plt.show()

# Gráfico de giroscópio em X
plt.figure()
plt.plot(x,data[:, 4],'ro-.')
plt.title("Gráfico - Giroscópio X")
plt.xlabel("Amostra")
plt.ylabel("Velocidade")
plt.grid()
plt.show()

# Gráfico de giroscópio em Y
plt.figure()
plt.plot(x,data[:, 5],'ro-.')
plt.title("Gráfico - Giroscópio Y")
plt.xlabel("Amostra")
plt.ylabel("Velocidade")
plt.grid()
plt.show()

# Gráfico de giroscópio em Z
plt.figure()
plt.plot(x,data[:, 6],'ro-.')
plt.title("Gráfico - Giroscópio Z")
plt.xlabel("Amostra")
plt.ylabel("Velocidade")
plt.grid()
plt.show()