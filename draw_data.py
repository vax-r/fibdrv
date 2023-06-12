import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("data.csv", header=0)
plt.xlabel("nth Fibonacci")
plt.ylabel("time(ns)")

df.plot()
plt.savefig("data.png")







