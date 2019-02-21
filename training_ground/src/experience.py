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
        for item in self.waiting.find({}):
            record = {}
            record["state"] = item["state"]
            record["values"] = item["values"]
            record["values"][item["index"]] = value
            new_records.append(record)
        if new_records:
            # sometimes waiting-for-approval collection is empty
            # TODO: investigate why
            self.approved.insert_many(new_records)
        self.waiting.delete_many({})
