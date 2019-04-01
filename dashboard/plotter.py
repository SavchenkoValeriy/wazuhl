import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns


class Plotter:
    def __init__(self, pic_folder="static/"):
        sns.set()
        sns.set_style("whitegrid")
        sns.set_context("paper")
        self.pic_folder = pic_folder
        self.fsize = (9, 4)

    def plot_curve_plot(self, x, y, filename, xlabel, ylabel, average_over=1):
        plt.figure(figsize=self.fsize)
        plt.xlabel(xlabel)
        plt.ylabel(ylabel)
        x = [i - i % average_over for i in x]
        sns.lineplot(x, y)
        plt.savefig("{}{}".format(self.pic_folder, filename),
                    bbox_inches='tight')

    def plot_learning_rate(self, time=np.random.rand((10)),
                           loss=np.random.rand((10))):
        self.plot_curve_plot(time, loss, "learning_rate_plot.svg",
                             "t, epochs", "Loss")

    def plot_mean_q(self, time=np.random.rand((10)), q=np.random.rand(10)):
        self.plot_curve_plot(time, q, "mean_q_plot.svg", "t, epochs",
                             "Average Q")

    def plot_mean_reward(self, time=np.random.rand((10)),
                         reward=np.random.rand(10)):
        self.plot_curve_plot(time, reward, "mean_reward_plot.svg", "t, epochs",
                             "Average reward per episode", 300)

    def plot_baselines(self, baselines_compile, baselines_run, model_compile,
                       model_run, steps):
        # TODO: Add usual curves instead of bar chart.
        plt.figure(figsize=self.fsize)
        plt.plot(steps, baselines_compile, label="baseline compile")
        plt.plot(steps, baselines_run, label="baseline run")
        plt.plot(steps, model_compile, label="model compile")
        plt.plot(steps, model_run, label="model run")
        plt.grid()
        plt.legend()
        plt.xlabel("Steps")
        plt.ylabel("Wall time, s")
        plt.savefig("{}baselines.svg".format(self.pic_folder),
                    bbox_inches='tight')
