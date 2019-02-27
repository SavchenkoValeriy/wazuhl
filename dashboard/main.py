from flask import Flask, render_template

from plotter import Plotter
import numpy as np

app = Flask(__name__)
app.config['SEND_FILE_MAX_AGE_DEFAULT'] = 0

plotter = Plotter()

@app.route("/", methods=['GET'])
def main_page():
    plotter.plot_learning_rate(range(100), np.random.rand(100))
    plotter.plot_mean_reward(range(100), np.random.rand(100))
    plotter.plot_mean_q(range(100), np.random.rand(100))
    plotter.plot_baselines([5.0, 6.3], [10, 10])
    return render_template('main.html')

if __name__ == '__main__':
    app.run()