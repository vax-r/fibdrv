import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

df_recur = pd.read_csv("data_recur.csv", header=0)
df_iter = pd.read_csv("data_iter.csv", header=0)
new_df = pd.concat([df_recur, df_iter])

plt.xlabel("nth Fibonacci")
plt.ylabel("time(ns)")

new_df.plot()
plt.savefig("data.png")







