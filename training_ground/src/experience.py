from pymongo import MongoClient, DESCENDING
from multiprocessing import Process
import subprocess
import pprint
import os
import shutil
import logging

from src import config
from src import utils

class Experience:
    def __init__(self):
        logging.info("Connecting to 'mongo' for experience")
        self.client = MongoClient('mongo')
        self.db = self.client["wazuhl"]
        self.approved = self.db["approved"]
        self.waiting = self.db["waiting"]
        self.rewards = self.db["rewards"]
        self.update_rewards_counter()

    def update_rewards_counter(self):
        self.rewards_counter = 0
        last_reward = [entry["step"] for entry in
                       self.rewards.find(sort=[("step", DESCENDING)],
                                         limit=1, projection={"step": 1})]
        if last_reward:
            self.rewards_counter = last_reward[0]

    def approve_trace(self, value):
        new_records = []
        last_index = 0
        self.rewards.insert_one({"step": self.rewards_counter, "reward": value})
        self.rewards_counter += 1
        for trace in self.waiting.find({}):
            R = value
            itertrace = iter(reversed(trace["trace"]))
            terminal = next(itertrace)
            new_records.append(_form_eligibility_trace_return(terminal,
                                                              R, True))
            for item in itertrace:
                new_records.append(_form_eligibility_trace_return(item, R))
                R = new_records[-1]["reward"]
            logging.debug("Assigning returns [%s] to the trace",
                          " -> ".join([str(i["reward"])
                                       for i in new_records[last_index:]]))
            last_index = len(new_records)

        if new_records:
            # sometimes waiting-for-approval collection is empty
            # TODO: investigate why
            self.approved.insert_many(new_records)
        self.waiting.delete_many({})

    def approve(self, value):
        new_records = []
        for item in self.waiting.find({}):
            record = {}
            record.update(item)
            record["reward"] = value
            new_records.append(record)
            logging.debug("Inserting value %s at index %d in the memory",
                          value, item["index"])
        if new_records:
            # sometimes waiting-for-approval collection is empty
            # TODO: investigate why
            self.approved.insert_many(new_records)
        self.waiting.delete_many({})


def _form_eligibility_trace_return(element, value, is_terminal=False):
    record = {}
    record.update(element)
    if is_terminal:
        record["reward"] = value
    else:
        record["reward"] = config.get_lambda() * value + \
            (1 - config.get_lambda()) * element["reward"]
    return record
