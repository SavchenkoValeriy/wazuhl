from flask import Flask, render_template

from plotter import Plotter
from data_receiver import DataReceiver
import numpy as np
import logging

logging.basicConfig()
logging.getLogger().setLevel(logging.DEBUG)

app = Flask(__name__)
app.config['SEND_FILE_MAX_AGE_DEFAULT'] = 0

plotter = Plotter()
receiver = DataReceiver()


def preprocess_reward(x):
    return x if x >= -10.0 else -10.0


@app.route("/", methods=['GET'])
def main_page():
    steps, rewards = receiver.get_latest_rewards()
    rewards = list(map(preprocess_reward, rewards))
    logging.info(steps)
    logging.info(rewards)
    plotter.plot_mean_reward(steps, rewards)
    return render_template('main.html')

if __name__ == '__main__':
    app.run(host='0.0.0.0')
