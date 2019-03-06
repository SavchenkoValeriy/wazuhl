from pymongo import MongoClient, DESCENDING
import logging

class DataReceiver:
    def __init__(self):
        self.client = MongoClient('mongo')
        self.db = self.client["wazuhl"]
        self.rewards = self.db["rewards"]

    def get_latest_rewards(self, last_n=0):
        query = self.rewards.find(sort=[("step", DESCENDING)],
                                  limit=last_n, projection={"reward": 1,
                                                            "step": 1})
        steps_rewards = ((record["step"], record["reward"]) for record in query)
        if not steps_rewards:
            return [], []
        steps, rewards = zip(*steps_rewards)
        logging.info(steps)
        logging.info(rewards)
        return steps, rewards