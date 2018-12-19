from pymongo import MongoClient
from multiprocessing import Process
import subprocess
import pprint
import os
import shutil

from src import config
from src import utils

class Experience:
    def __init__(self):
        self.mongo = Process(target=Experience.__start_mongo_server__, args=())
        self.mongo.start()
        self.client = MongoClient()
        self.db = self.client["wazuhl"]
        self.approved = self.db["approved"]
        self.waiting = self.db["waiting"]

    @staticmethod
    def __start_mongo_server__():
        dbpath = config.get_mongodb()
        utils.check_executable("mongod")
        subprocess.call(["mongod", "--dbpath", dbpath])

    def fill_records(self, value):
        new_records = []
        for item in self.waiting.find():
            record = {}
            record["state"] = item["state"]
            record["values"] = item["values"]
            record["values"][item["index"]] = value
            new_records.append(record)
        self.approved.insert_many(new_records)