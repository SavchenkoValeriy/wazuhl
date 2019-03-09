from pymongo import MongoClient
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

    def approve(self, value):
        new_records = []
        last_index = 0
        for trace in self.waiting.find({}):
            R = value
            itertrace = iter(reversed(trace["trace"]))
            terminal = next(itertrace)
            new_records.append(_form_eligibility_trace_return(terminal,
                                                              R, True))
            for item in itertrace:
                new_records.append(_form_eligibility_trace_return(item, R))
                R = new_records[-1]["value"]
            logging.debug("Assigning returns [%s] to the trace",
                          " -> ".join([str(i["value"])
                                       for i in new_records[last_index:]]))
            last_index = len(new_records)

        if new_records:
            # sometimes waiting-for-approval collection is empty
            # TODO: investigate why
            self.approved.insert_many(new_records)
        self.waiting.delete_many({})

def _form_eligibility_trace_return(element, value, is_terminal=False):
    record = {}
    record["state"] = element["state"]
    record["context"] = element["context"]
    record["index"] = element["index"]
    if is_terminal:
        record["value"] = value
    else:
        record["value"] = config.get_lambda() * value + \
            (1 - config.get_lambda()) * element["value"]
    return record
