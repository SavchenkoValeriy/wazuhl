from pymongo import MongoClient

class DataReceiver:
    # TODO: Add Mongo loading.
    def __init__(self):
        self.client = MongoClient('mongo')
        self.db = self.client["wazuhl"]
        self.learning_log = self.db["learning_log"]
