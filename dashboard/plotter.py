import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

class Plotter:
    def __init__(self, pic_folder="static/"):
        sns.set()
        sns.set_style("whitegrid")
        sns.set_context("paper")
        self.pic_folder = pic_folder
        self.fsize = (9, 4)

    def plot_curve_plot(self, x, y, filename, xlabel, ylabel):
        plt.figure(figsize=self.fsize)
        plt.plot(x, y)
        plt.grid()
        plt.xlabel(xlabel)
        plt.ylabel(ylabel)
        plt.savefig("{}{}".format(self.pic_folder, filename), bbox_inches='tight')

    def plot_learning_rate(self, time=np.random.rand((10)), loss=np.random.rand((10))):
        self.plot_curve_plot(time, loss, "learning_rate_plot.svg", "t, epochs", "Loss")

    def plot_mean_q(self, time=np.random.rand((10)), q=np.random.rand(10)):
        self.plot_curve_plot(time, q, "mean_q_plot.svg", "t, epochs", "Average Q")

    def plot_mean_reward(self, time=np.random.rand((10)), reward=np.random.rand(10)):
        self.plot_curve_plot(time, reward, "mean_reward_plot.svg", "t, epochs", "Average reward per episode")

    def plot_baselines(self, o2=[0, 0], model=[0, 0]):
        # TODO: Add usual curves instead of bar chart.
        fig, ax = plt.subplots(figsize=self.fsize)
        ax = sns.barplot([1, 2, 4, 5], [o2[0], model[0], o2[1], model[1]])
        ax.set_xticks(range(4))
        ax.set_xticklabels(["Compile: -O2", "Compile: model", "Run: -O2", "Run: model"])
        ax.set_ylabel("t, ms")
        ax.legend()
        plt.savefig("{}baselines.svg".format(self.pic_folder), bbox_inches='tight')
